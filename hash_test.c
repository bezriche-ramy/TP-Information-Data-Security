/*
 * Hash Test - Verify if two passwords produce the same hash
 */
#include <stdio.h>
#include <string.h>

// Same hash function as auth_system.c
unsigned long hash_password(const char *str, const char *salt) {
    unsigned long hash = 5381;
    char combined[20];
    snprintf(combined, sizeof(combined), "%s%s", str, salt);
    
    int c;
    const char *ptr = combined;
    while ((c = *ptr++))
        hash = ((hash << 5) + hash) + c;
    
    return hash;
}

int main() {
    const char *salt = "70095";
    
    // The password from auth_system signup
    const char *real_password = "abcabc3H";
    
    // The password GPU found
    const char *gpu_found = "abcabati";
    
    unsigned long hash_real = hash_password(real_password, salt);
    unsigned long hash_gpu = hash_password(gpu_found, salt);
    
    printf("Testing hash collision:\n\n");
    printf("Password 1: '%s' + salt '%s'\n", real_password, salt);
    printf("  Hash: %lu\n\n", hash_real);
    
    printf("Password 2: '%s' + salt '%s'\n", gpu_found, salt);
    printf("  Hash: %lu\n\n", hash_gpu);
    
    printf("Stored hash in file: 10542718414494321297\n\n");
    
    if (hash_real == hash_gpu) {
        printf("⚠️  COLLISION DETECTED! Both passwords have the SAME hash!\n");
    } else {
        printf("✅ Hashes are DIFFERENT - there's a bug in GPU implementation\n");
        printf("   Difference: %ld\n", (long)(hash_real - hash_gpu));
    }
    
    return 0;
}
