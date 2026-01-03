/*
 * GPU Brute Force Password Cracker - FNV-1a Hash Version
 * Using CUDA for NVIDIA RTX 3050 Laptop GPU
 * 
 * Compile: nvcc -O3 -arch=sm_86 -o gpu_cracker gpu_cracker.cu
 * Run: ./gpu_cracker
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cuda_runtime.h>

#define MAX_PASSWORD_LEN 9
#define SALT_LENGTH 5
#define CHARSET_SIZE 66
#define BLOCK_SIZE 256
#define PASSWORDS_PER_THREAD 1000

// Character set: a-z, A-Z, 0-9, @#$&
__constant__ char d_charset[CHARSET_SIZE + 1] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$&";

// FNV-1a 64-bit constants
#define FNV_OFFSET_BASIS 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

// FNV-1a hash function (device version) - Matches CPU exactly
__device__ unsigned long long fnv1a_hash_device(const char *password, const char *salt, int pwd_len) {
    unsigned long long hash = FNV_OFFSET_BASIS;
    
    // Build combined string: password + salt
    char combined[20];
    int idx = 0;
    
    // Copy password
    for (int j = 0; j < pwd_len; j++) {
        combined[idx++] = password[j];
    }
    
    // Copy salt (always 5 chars)
    for (int j = 0; j < 5; j++) {
        combined[idx++] = salt[j];
    }
    combined[idx] = '\0';
    
    // FNV-1a hash algorithm
    const unsigned char *ptr = (const unsigned char *)combined;
    while (*ptr) {
        hash ^= *ptr++;
        hash *= FNV_PRIME;
    }
    
    return hash;
}

// Generate password from index
__device__ void index_to_password(unsigned long long idx, char *password, int len) {
    for (int i = len - 1; i >= 0; i--) {
        password[i] = d_charset[idx % CHARSET_SIZE];
        idx /= CHARSET_SIZE;
    }
    password[len] = '\0';
}

// GPU Kernel: Brute force passwords
__global__ void brute_force_kernel(unsigned long long start_idx, unsigned long long total_passwords,
                                   int *found, char *found_password, unsigned long long target_hash,
                                   int password_len, char *salt) {
    unsigned long long global_idx = blockIdx.x * blockDim.x + threadIdx.x;
    unsigned long long idx = start_idx + global_idx * PASSWORDS_PER_THREAD;
    
    char password[MAX_PASSWORD_LEN];
    
    // Each thread tries multiple passwords
    for (int i = 0; i < PASSWORDS_PER_THREAD && (idx + i) < total_passwords; i++) {
        if (*found) return;  // Early exit if another thread found it
        
        index_to_password(idx + i, password, password_len);
        unsigned long long hash = fnv1a_hash_device(password, salt, password_len);
        
        if (hash == target_hash) {
            *found = 1;
            // Copy found password
            for (int j = 0; j <= password_len; j++) {
                found_password[j] = password[j];
            }
            return;
        }
    }
}

// Host: FNV-1a hash function
unsigned long long host_fnv1a_hash(const char *str, const char *salt) {
    unsigned long long hash = FNV_OFFSET_BASIS;
    
    char combined[20];
    snprintf(combined, sizeof(combined), "%s%s", str, salt);
    
    const unsigned char *ptr = (const unsigned char *)combined;
    while (*ptr) {
        hash ^= *ptr++;
        hash *= FNV_PRIME;
    }
    return hash;
}

// Calculate total combinations
unsigned long long calculate_combinations(int length) {
    unsigned long long total = 1;
    for (int i = 0; i < length; i++) {
        total *= CHARSET_SIZE;
    }
    return total;
}

// Format time
void format_time(double seconds, char *buffer) {
    if (seconds < 60) {
        sprintf(buffer, "%.1fs", seconds);
    } else if (seconds < 3600) {
        sprintf(buffer, "%.1fm", seconds / 60.0);
    } else if (seconds < 86400) {
        sprintf(buffer, "%.1fh", seconds / 3600.0);
    } else {
        sprintf(buffer, "%.1fd", seconds / 86400.0);
    }
}

// Parse password.txt file
int parse_password_file(const char *username, char *salt, unsigned long long *hash) {
    FILE *file = fopen("password.txt", "r");
    if (!file) {
        printf("Error: Cannot open password.txt\n");
        return 0;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char stored_user[10];
        char stored_salt[10];
        unsigned long long stored_hash;
        
        if (sscanf(line, "%5[^:]:%5[^:]:%llu", stored_user, stored_salt, &stored_hash) == 3) {
            if (strcmp(stored_user, username) == 0) {
                strcpy(salt, stored_salt);
                *hash = stored_hash;
                fclose(file);
                return 1;
            }
        }
    }
    
    fclose(file);
    return 0;
}

int main(int argc, char *argv[]) {
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║   GPU BRUTE FORCE PASSWORD CRACKER (CUDA + FNV-1a)     ║\n");
    printf("║   Optimized for NVIDIA RTX 3050 Laptop GPU             ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n\n");
    
    // Check CUDA device
    int deviceCount;
    cudaGetDeviceCount(&deviceCount);
    if (deviceCount == 0) {
        printf("❌ No CUDA-capable GPU detected!\n");
        return 1;
    }
    
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    printf("🖥️  GPU Detected: %s\n", prop.name);
    printf("   CUDA Cores: %d\n", prop.multiProcessorCount * 128);
    printf("   Memory: %.1f GB\n", prop.totalGlobalMem / (1024.0 * 1024.0 * 1024.0));
    printf("   Compute Capability: %d.%d\n\n", prop.major, prop.minor);
    
    // Get username from user
    char username[10];
    printf("Enter target username: ");
    if (scanf("%s", username) != 1) {
        printf("Error reading username.\n");
        return 1;
    }
    
    // Parse password file
    char salt[SALT_LENGTH + 1];
    unsigned long long target_hash;
    
    if (!parse_password_file(username, salt, &target_hash)) {
        printf("❌ Username '%s' not found in password.txt\n", username);
        return 1;
    }
    
    printf("\n✅ Target found: %s\n", username);
    printf("   Salt: %s\n", salt);
    printf("   Target Hash: %llu\n", target_hash);
    
    // Get password length
    int password_len;
    printf("\nEnter password length to crack (1-8): ");
    if (scanf("%d", &password_len) != 1 || password_len < 1 || password_len > 8) {
        printf("Invalid length.\n");
        return 1;
    }
    
    unsigned long long total_combinations = calculate_combinations(password_len);
    printf("\n🎯 Attack Configuration:\n");
    printf("   Password Length: %d\n", password_len);
    printf("   Charset Size: %d\n", CHARSET_SIZE);
    printf("   Total Combinations: %llu\n", total_combinations);
    printf("   Hash Algorithm: FNV-1a 64-bit (collision resistant)\n");
    
    // Allocate device memory
    int *d_found;
    char *d_found_password;
    char *d_salt_ptr;
    
    cudaMalloc(&d_found, sizeof(int));
    cudaMalloc(&d_found_password, MAX_PASSWORD_LEN);
    cudaMalloc(&d_salt_ptr, SALT_LENGTH + 1);
    
    int h_found = 0;
    char h_found_password[MAX_PASSWORD_LEN] = {0};
    
    cudaMemcpy(d_found, &h_found, sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_salt_ptr, salt, SALT_LENGTH + 1, cudaMemcpyHostToDevice);
    
    // Calculate grid dimensions - more blocks for better GPU utilization
    int threads_per_block = BLOCK_SIZE;
    unsigned long long passwords_per_kernel = 10000000ULL;  // 10M passwords per kernel
    int blocks = (passwords_per_kernel / PASSWORDS_PER_THREAD + threads_per_block - 1) / threads_per_block;
    
    printf("\n🚀 Starting GPU Brute Force Attack...\n");
    printf("   Blocks: %d, Threads/Block: %d\n", blocks, threads_per_block);
    printf("   Passwords per kernel: %llu\n\n", passwords_per_kernel);
    
    time_t start_time = time(NULL);
    unsigned long long attempts = 0;
    int found = 0;
    
    // Main attack loop
    while (attempts < total_combinations && !found) {
        // Launch kernel
        brute_force_kernel<<<blocks, threads_per_block>>>(
            attempts, total_combinations, d_found, d_found_password, 
            target_hash, password_len, d_salt_ptr
        );
        
        cudaDeviceSynchronize();
        
        // Check if found
        cudaMemcpy(&h_found, d_found, sizeof(int), cudaMemcpyDeviceToHost);
        
        if (h_found) {
            cudaMemcpy(h_found_password, d_found_password, MAX_PASSWORD_LEN, cudaMemcpyDeviceToHost);
            found = 1;
            break;
        }
        
        attempts += passwords_per_kernel;
        
        // Progress update
        time_t now = time(NULL);
        double elapsed = difftime(now, start_time);
        if (elapsed > 0) {
            double speed = attempts / elapsed;
            double progress = (double)attempts / total_combinations * 100.0;
            unsigned long long remaining = total_combinations - attempts;
            double eta_seconds = remaining / speed;
            
            char eta_str[32];
            format_time(eta_seconds, eta_str);
            
            printf("\r  [GPU] %.4f%% | %llu attempts | Speed: %.0f/s | ETA: %s        ", 
                   progress, attempts, speed, eta_str);
            fflush(stdout);
        }
    }
    
    time_t end_time = time(NULL);
    double total_time = difftime(end_time, start_time);
    
    printf("\n\n");
    
    if (found) {
        printf("🎯 PASSWORD FOUND: %s\n", h_found_password);
        printf("   Time: %.1f seconds\n", total_time);
        if (total_time > 0) {
            printf("   Speed: %.0f passwords/second\n", attempts / total_time);
        }
        
        // Verify the found password
        unsigned long long verify_hash = host_fnv1a_hash(h_found_password, salt);
        if (verify_hash == target_hash) {
            printf("   ✅ Hash verification: PASSED\n");
        } else {
            printf("   ❌ Hash verification: FAILED (this shouldn't happen)\n");
        }
    } else {
        printf("❌ Password not found after %llu attempts.\n", attempts);
        printf("   Time: %.1f seconds\n", total_time);
    }
    
    // Cleanup
    cudaFree(d_found);
    cudaFree(d_found_password);
    cudaFree(d_salt_ptr);
    
    return 0;
}
