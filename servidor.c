#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "hash_utils.h"

#define PORT 8080
#define MAX_WORKERS 10

// Estado global del servidor
typedef struct {
    WorkConfig config;
    int total_workers;
    int workers_connected;
    int solution_found;
    char solution_nonce[MAX_NONCE_SIZE];
    uint64_t solution_hash;
    pthread_mutex_t mutex;
} ServerState;

ServerState server_state;

// Carga el texto desde el archivo
int load_text(const char* filename, char* text, int max_size) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error abriendo archivo");
        return -1;
    }
    
    int len = fread(text, 1, max_size - 1, file);
    text[len] = '\0';
    fclose(file);
    
    printf("[SERVIDOR] Texto cargado: %d caracteres\n", len);
    return len;
}

// Maneja la conexión de un worker
void* handle_worker(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    
    pthread_mutex_lock(&server_state.mutex);
    int worker_id = server_state.workers_connected++;
    pthread_mutex_unlock(&server_state.mutex);
    
    printf("[SERVIDOR] Worker %d conectado\n", worker_id);
    
    // Enviar configuración al worker
    send(client_sock, &server_state.config, sizeof(WorkConfig), 0);
    
    // Calcular y enviar rango de trabajo
    uint64_t total_space = 1;
    for (int i = 0; i < server_state.config.nonce_length; i++) {
        total_space *= server_state.config.charset_size;
    }
    
    uint64_t range_size = total_space / server_state.total_workers;
    uint64_t start = worker_id * range_size;
    uint64_t end = (worker_id == server_state.total_workers - 1) ? 
                   total_space : (worker_id + 1) * range_size;
    
    WorkRange range;
    number_to_nonce(start, range.start_nonce, server_state.config.nonce_length, 
                    server_state.config.charset, server_state.config.charset_size);
    number_to_nonce(end - 1, range.end_nonce, server_state.config.nonce_length, 
                    server_state.config.charset, server_state.config.charset_size);
    
    printf("[SERVIDOR] Worker %d: rango [%s - %s]\n", worker_id, 
           range.start_nonce, range.end_nonce);
    
    send(client_sock, &range, sizeof(WorkRange), 0);
    
    // Esperar respuesta del worker
    char nonce[MAX_NONCE_SIZE];
    uint64_t hash;
    
    while (!server_state.solution_found) {
        int bytes = recv(client_sock, nonce, MAX_NONCE_SIZE, 0);
        if (bytes <= 0) break;
        
        recv(client_sock, &hash, sizeof(uint64_t), 0);
        
        pthread_mutex_lock(&server_state.mutex);
        if (!server_state.solution_found) {
            server_state.solution_found = 1;
            strcpy(server_state.solution_nonce, nonce);
            server_state.solution_hash = hash;
            
            printf("\n[SERVIDOR] ¡SOLUCIÓN ENCONTRADA!\n");
            printf("[SERVIDOR] Worker ganador: %d\n", worker_id);
            printf("[SERVIDOR] Relleno: %s\n", nonce);
            printf("[SERVIDOR] Hash: %llu\n", (unsigned long long)hash);
            printf("[SERVIDOR] Texto completo: %s%s\n", 
                   server_state.config.text, nonce);
        }
        pthread_mutex_unlock(&server_state.mutex);
        
        break;
    }
    
    // Notificar al worker que termine
    char stop = 1;
    send(client_sock, &stop, 1, 0);
    
    close(client_sock);
    printf("[SERVIDOR] Worker %d desconectado\n", worker_id);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Uso: %s <archivo.txt> <nonce_length> <difficulty> <num_workers>\n", argv[0]);
        printf("Ejemplo: %s texto.txt 4 2 3\n", argv[0]);
        return 1;
    }
    
    // Inicializar configuración
    memset(&server_state, 0, sizeof(ServerState));
    pthread_mutex_init(&server_state.mutex, NULL);
    
    // Cargar texto
    server_state.config.text_length = load_text(argv[1], server_state.config.text, MAX_TEXT_SIZE);
    if (server_state.config.text_length < 0) {
        return 1;
    }
    
    // Configurar parámetros
    server_state.config.nonce_length = atoi(argv[2]);
    server_state.config.difficulty = atoi(argv[3]);
    server_state.total_workers = atoi(argv[4]);
    
    // Charset: dígitos y letras mayúsculas
    strcpy(server_state.config.charset, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    server_state.config.charset_size = strlen(server_state.config.charset);
    
    printf("[SERVIDOR] Iniciando Proof of Work\n");
    printf("[SERVIDOR] Texto: %d caracteres\n", server_state.config.text_length);
    printf("[SERVIDOR] Longitud relleno: %d\n", server_state.config.nonce_length);
    printf("[SERVIDOR] Dificultad: %d ceros\n", server_state.config.difficulty);
    printf("[SERVIDOR] Workers esperados: %d\n", server_state.total_workers);
    printf("[SERVIDOR] Charset: %s (%d caracteres)\n", 
           server_state.config.charset, server_state.config.charset_size);
    
    // Crear socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Error creando socket");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        return 1;
    }
    
    if (listen(server_sock, MAX_WORKERS) < 0) {
        perror("Error en listen");
        return 1;
    }
    
    printf("[SERVIDOR] Escuchando en puerto %d...\n", PORT);
    
    // Aceptar workers
    pthread_t threads[MAX_WORKERS];
    for (int i = 0; i < server_state.total_workers; i++) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int* client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        
        if (*client_sock < 0) {
            perror("Error en accept");
            continue;
        }
        
        pthread_create(&threads[i], NULL, handle_worker, client_sock);
    }
    
    // Esperar a que todos terminen
    for (int i = 0; i < server_state.total_workers; i++) {
        pthread_join(threads[i], NULL);
    }
    
    close(server_sock);
    pthread_mutex_destroy(&server_state.mutex);
    
    printf("\n[SERVIDOR] Finalizando...\n");
    return 0;
}