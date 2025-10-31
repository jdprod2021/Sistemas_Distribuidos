#include "hash_utils.h"
#include <string.h>
#include <stdio.h>

#define PRIME 999999937ULL


uint64_t calculate_hash(const char* text, int text_len, const char* nonce, int nonce_len) {
    uint64_t hash = 0;
    int total_len = text_len + nonce_len;
    
    for (int i = 0; i < text_len; i++) {
        hash = (hash * 31 + (unsigned char)text[i]) % PRIME;
    }
    

    for (int i = 0; i < nonce_len; i++) {
        hash = (hash * 31 + (unsigned char)nonce[i]) % PRIME;
    }
    
    return hash;
}

int verify_difficulty(uint64_t hash, int difficulty) {
    for (int i = 0; i < difficulty; i++) {
        if (hash % 10 != 0) {
            return 0;
        }
        hash /= 10;
    }
    return 1;
}


void number_to_nonce(uint64_t num, char* nonce, int nonce_len, const char* charset, int charset_size) {
    for (int i = nonce_len - 1; i >= 0; i--) {
        nonce[i] = charset[num % charset_size];
        num /= charset_size;
    }
    nonce[nonce_len] = '\0';
}

uint64_t nonce_to_number(const char* nonce, int nonce_len, const char* charset, int charset_size) {
    uint64_t num = 0;
    
    for (int i = 0; i < nonce_len; i++) {
        int idx = -1;
        for (int j = 0; j < charset_size; j++) {
            if (charset[j] == nonce[i]) {
                idx = j;
                break;
            }
        }
        
        if (idx == -1) return 0;
        
        num = num * charset_size + idx;
    }
    
    return num;
}


int increment_nonce(char* nonce, int nonce_len, const char* charset, int charset_size) {
    for (int i = nonce_len - 1; i >= 0; i--) {

        int pos = -1;
        for (int j = 0; j < charset_size; j++) {
            if (charset[j] == nonce[i]) {
                pos = j;
                break;
            }
        }
        
        if (pos == -1) return 0;
        
    
        if (pos < charset_size - 1) {
            nonce[i] = charset[pos + 1];
            return 1;
        } else {
            nonce[i] = charset[0];
        }
    }
    
    return 0; 
}