#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <stdint.h>


#define MAX_TEXT_SIZE 10000
#define MAX_NONCE_SIZE 10


typedef struct {
    char text[MAX_TEXT_SIZE];
    int text_length;
    int nonce_length;
    int difficulty;  
    char charset[65];
    int charset_size;
} WorkConfig;

typedef struct {
    char start_nonce[MAX_NONCE_SIZE];
    char end_nonce[MAX_NONCE_SIZE];
} WorkRange;

uint64_t calculate_hash(const char* text, int text_len, const char* nonce, int nonce_len);

int verify_difficulty(uint64_t hash, int difficulty);

void number_to_nonce(uint64_t num, char* nonce, int nonce_len, const char* charset, int charset_size);

uint64_t nonce_to_number(const char* nonce, int nonce_len, const char* charset, int charset_size);

int increment_nonce(char* nonce, int nonce_len, const char* charset, int charset_size);

#endif