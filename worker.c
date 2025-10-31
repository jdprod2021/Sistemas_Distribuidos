#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "hash_utils.h"

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : SERVER_IP;
    
    printf("[WORKER] Conectando al servidor %s:%d...\n", server_ip, PORT);
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creando socket");
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Dirección inválida");
        return 1;
    }
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error conectando");
        return 1;
    }
    
    printf("[WORKER] Conectado al servidor\n");
    

    WorkConfig config;
    recv(sock, &config, sizeof(WorkConfig), 0);
    
    printf("[WORKER] Configuración recibida:\n");
    printf("  - Texto: %d caracteres\n", config.text_length);
    printf("  - Relleno: %d caracteres\n", config.nonce_length);
    printf("  - Dificultad: %d ceros\n", config.difficulty);


    WorkRange range;
    recv(sock, &range, sizeof(WorkRange), 0);
    
    printf("[WORKER] Rango asignado: [%s - %s]\n", range.start_nonce, range.end_nonce);


    char current_nonce[MAX_NONCE_SIZE];
    strcpy(current_nonce, range.start_nonce);
    current_nonce[config.nonce_length] = '\0';
    
    uint64_t attempts = 0;
    uint64_t start_num = nonce_to_number(range.start_nonce, config.nonce_length, 
                                         config.charset, config.charset_size);
    uint64_t end_num = nonce_to_number(range.end_nonce, config.nonce_length, 
                                       config.charset, config.charset_size);
    
    printf("[WORKER] Comenzando búsqueda...\n");
    clock_t start_time = clock();
    
    int found = 0;
    for (uint64_t i = start_num; i <= end_num; i++) {
        number_to_nonce(i, current_nonce, config.nonce_length, 
                       config.charset, config.charset_size);
        
        uint64_t hash = calculate_hash(config.text, config.text_length, 
                                       current_nonce, config.nonce_length);
        
        attempts++;
        
        // Mostrar progreso cada 100000 intentos
        if (attempts % 100000 == 0) {
            double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
            double rate = attempts / elapsed;
            printf("[WORKER] Intentos: %llu | Relleno actual: %s | Rate: %.0f H/s\n", 
                   (unsigned long long)attempts, current_nonce, rate);
        }
        
        if (verify_difficulty(hash, config.difficulty)) {
            printf("\n[WORKER] ¡SOLUCIÓN ENCONTRADA!\n");
            printf("[WORKER] Relleno: %s\n", current_nonce);
            printf("[WORKER] Hash: %llu\n", (unsigned long long)hash);
            printf("[WORKER] Intentos: %llu\n", (unsigned long long)attempts);
            
            double elapsed = (double)(clock() - start_time) / CLOCKS_PER_SEC;
            printf("[WORKER] Tiempo: %.2f segundos\n", elapsed);
            
            // Enviar solución al servidor
            send(sock, current_nonce, config.nonce_length, 0);
            send(sock, &hash, sizeof(uint64_t), 0);
            
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printf("[WORKER] Rango completado sin encontrar solución\n");
    }
    
    // Esperar señal de stop del servidor
    char stop;
    recv(sock, &stop, 1, 0);
    
    close(sock);
    printf("[WORKER] Desconectado\n");
    
    return 0;
}