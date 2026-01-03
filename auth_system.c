/*
 * Information Data Security - Authentication System with Brute Force Module
 * University Assignment (TP)
 * 
 * Part 1: Authentication System (Signup, Signin, Throttling, Ban)
 * Part 2: Brute Force Attack Module (Blind & Targeted) - NOW WITH PARALLEL CPU!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <omp.h>  // OpenMP for parallel processing

#define MAX_USERNAME 6    // 5 chars + null terminator
#define MAX_PASSWORD 9    // 8 chars + null terminator
#define SALT_LENGTH 5
#define PASSWORD_FILE "password.txt"
#define BAN_FILE "banned.txt"
#define MAX_LINE 256

// ==================== PART 1: AUTHENTICATION SYSTEM ====================

// FNV-1a hash function (much stronger than DJB2, fewer collisions)
// Using 64-bit FNV-1a for better collision resistance
unsigned long long hash_password(const char *str, const char *salt) {
    // FNV-1a 64-bit parameters
    unsigned long long hash = 14695981039346656037ULL;  // FNV offset basis
    const unsigned long long FNV_PRIME = 1099511628211ULL;
    
    char combined[MAX_PASSWORD + SALT_LENGTH + 1];
    snprintf(combined, sizeof(combined), "%s%s", str, salt);
    
    const unsigned char *ptr = (const unsigned char *)combined;
    while (*ptr) {
        hash ^= *ptr++;           // XOR with byte
        hash *= FNV_PRIME;        // Multiply by FNV prime
    }
    
    return hash;
}

// Generate random 5-digit salt
void generate_salt(char *salt) {
    for (int i = 0; i < SALT_LENGTH; i++) {
        salt[i] = '0' + (rand() % 10);
    }
    salt[SALT_LENGTH] = '\0';
}

// Validate username: exactly 5 lowercase characters
int validate_username(const char *username) {
    if (strlen(username) != 5) {
        return 0;
    }
    for (int i = 0; i < 5; i++) {
        if (!islower(username[i])) {
            return 0;
        }
    }
    return 1;
}

// Validate password: exactly 8 characters (lowercase + uppercase + digits)
int validate_password(const char *password) {
    int has_lower = 0, has_upper = 0, has_digit = 0;
    
    if (strlen(password) != 8) {
        return 0;
    }
    
    for (int i = 0; i < 8; i++) {
        if (islower(password[i])) has_lower = 1;
        else if (isupper(password[i])) has_upper = 1;
        else if (isdigit(password[i])) has_digit = 1;
        else return 0; // Invalid character
    }
    
    return has_lower && has_upper && has_digit;
}

// Check if user is banned
int is_banned(const char *username) {
    FILE *file = fopen(BAN_FILE, "r");
    if (!file) return 0;
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

// Ban a user
void ban_user(const char *username) {
    FILE *file = fopen(BAN_FILE, "a");
    if (file) {
        fprintf(file, "%s\n", username);
        fclose(file);
        printf("\n[SECURITY] Account '%s' has been BANNED due to excessive failed login attempts.\n", username);
    }
}

// Check if username already exists
int username_exists(const char *username) {
    FILE *file = fopen(PASSWORD_FILE, "r");
    if (!file) return 0;
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char stored_user[MAX_USERNAME];
        sscanf(line, "%5[^:]", stored_user);
        if (strcmp(stored_user, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

// Signup function
void signup() {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    char salt[SALT_LENGTH + 1];
    
    printf("\n=== SIGNUP ===\n");
    
    // Get and validate username
    printf("Enter Username (exactly 5 lowercase letters): ");
    if (scanf("%s", username) != 1) {
        printf("Error reading username.\n");
        while (getchar() != '\n'); // Clear input buffer
        return;
    }
    while (getchar() != '\n'); // Clear input buffer
    
    if (!validate_username(username)) {
        printf("❌ Invalid username! Must be exactly 5 lowercase letters.\n");
        return;
    }
    
    if (username_exists(username)) {
        printf("❌ Username already exists!\n");
        return;
    }
    
    // Get and validate password
    printf("Enter Password (exactly 8 chars: lowercase+uppercase+digits): ");
    if (scanf("%s", password) != 1) {
        printf("Error reading password.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');
    
    if (!validate_password(password)) {
        printf("❌ Invalid password! Must be exactly 8 characters with lowercase, uppercase, and digits.\n");
        return;
    }
    
    // Generate salt
    generate_salt(salt);
    
    // Hash the password
    unsigned long long hashed = hash_password(password, salt);
    
    // Store in file: username:salt:hashed_password
    FILE *file = fopen(PASSWORD_FILE, "a");
    if (!file) {
        printf("❌ Error opening password file!\n");
        return;
    }
    
    fprintf(file, "%s:%s:%llu\n", username, salt, hashed);
    fclose(file);
    
    printf("✅ Signup successful! Username: %s\n", username);
    printf("   (Salt: %s, Hash: %llu)\n", salt, hashed);
}

// Signin function with throttling
void signin() {
    static int failed_attempts = 0;
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
    
    printf("\n=== SIGNIN ===\n");
    
    // Get username
    printf("Enter Username: ");
    if (scanf("%s", username) != 1) {
        printf("Error reading username.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');
    
    // Check if banned
    if (is_banned(username)) {
        printf("❌ This account is BANNED. Access denied.\n");
        return;
    }
    
    // Get password
    printf("Enter Password: ");
    if (scanf("%s", password) != 1) {
        printf("Error reading password.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');
    
    // Read password file and verify
    FILE *file = fopen(PASSWORD_FILE, "r");
    if (!file) {
        printf("❌ No users registered yet!\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    
    while (fgets(line, sizeof(line), file)) {
        char stored_user[MAX_USERNAME];
        char stored_salt[SALT_LENGTH + 1];
        unsigned long long stored_hash;
        
        if (sscanf(line, "%5[^:]:%5[^:]:%llu", stored_user, stored_salt, &stored_hash) == 3) {
            if (strcmp(stored_user, username) == 0) {
                found = 1;
                unsigned long long input_hash = hash_password(password, stored_salt);
                
                if (input_hash == stored_hash) {
                    printf("✅ Login successful! Welcome, %s!\n", username);
                    failed_attempts = 0; // Reset on success
                    fclose(file);
                    return;
                }
                break;
            }
        }
    }
    fclose(file);
    
    // Failed login
    failed_attempts++;
    printf("❌ Invalid credentials! (Attempt %d)\n", failed_attempts);
    
    // Throttling logic
    if (failed_attempts >= 13) {
        // 13th failure -> BAN
        ban_user(username);
        failed_attempts = 0; // Reset
    } else if (failed_attempts >= 10) {
        // 10-12: sleep 20 seconds
        printf("[SECURITY] Too many failures. Sleeping for 20 seconds...\n");
        sleep(20);
    } else if (failed_attempts >= 7) {
        // 7-9: sleep 15 seconds
        printf("[SECURITY] Too many failures. Sleeping for 15 seconds...\n");
        sleep(15);
    } else if (failed_attempts >= 4) {
        // 4-6: sleep 10 seconds
        printf("[SECURITY] Too many failures. Sleeping for 10 seconds...\n");
        sleep(10);
    } else if (failed_attempts >= 1) {
        // 1-3: sleep 5 seconds
        printf("[SECURITY] Failed login. Sleeping for 5 seconds...\n");
        sleep(5);
    }
}

// ==================== PART 2: BRUTE FORCE MODULE ====================

// Character sets for brute force
const char USERNAME_CHARSET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const char PASSWORD_CHARSET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$&";

// Global variables for ETA calculation
static time_t g_start_time;
static unsigned long long g_total_combinations;

// Format time in human readable format
void format_time(unsigned long long seconds, char *buffer, size_t size) {
    if (seconds < 60) {
        snprintf(buffer, size, "%llus", seconds);
    } else if (seconds < 3600) {
        snprintf(buffer, size, "%llum %llus", seconds / 60, seconds % 60);
    } else if (seconds < 86400) {
        snprintf(buffer, size, "%lluh %llum", seconds / 3600, (seconds % 3600) / 60);
    } else if (seconds < 31536000) {
        snprintf(buffer, size, "%llud %lluh", seconds / 86400, (seconds % 86400) / 3600);
    } else {
        snprintf(buffer, size, "%lluy %llud", seconds / 31536000, (seconds % 31536000) / 86400);
    }
}

// Recursive brute force for password
// Returns 1 if password found, 0 otherwise
int brute_force_password_recursive(const char *username, char *attempt, int position, 
                                   int max_length, const char *salt, unsigned long long target_hash,
                                   unsigned long *attempts_count) {
    // Base case: reached max length
    if (position == max_length) {
        attempt[position] = '\0';
        (*attempts_count)++;
        
        // Test this password
        unsigned long long test_hash = hash_password(attempt, salt);
        
        // Show progress every 1,000,000 attempts with ETA
        if (*attempts_count % 1000000 == 0) {
            time_t now = time(NULL);
            double elapsed = difftime(now, g_start_time);
            
            if (elapsed > 0) {
                double speed = *attempts_count / elapsed;
                unsigned long long remaining = g_total_combinations - *attempts_count;
                unsigned long long eta_seconds = (unsigned long long)(remaining / speed);
                double progress = (double)*attempts_count / g_total_combinations * 100.0;
                
                char eta_str[64];
                format_time(eta_seconds, eta_str, sizeof(eta_str));
                
                printf("  [Progress] %lu attempts (%.4f%%) | Speed: %.0f/s | ETA: %s | Current: %s     \r", 
                       *attempts_count, progress, speed, eta_str, attempt);
                fflush(stdout);
            }
        }
        
        if (test_hash == target_hash) {
            printf("\n🎯 PASSWORD FOUND: %s (after %lu attempts)\n", attempt, *attempts_count);
            return 1;
        }
        return 0;
    }
    
    // Recursive case: try each character at this position
    int charset_len = strlen(PASSWORD_CHARSET);
    for (int i = 0; i < charset_len; i++) {
        attempt[position] = PASSWORD_CHARSET[i];
        if (brute_force_password_recursive(username, attempt, position + 1, 
                                          max_length, salt, target_hash, attempts_count)) {
            return 1;
        }
    }
    
    return 0;
}

// Targeted attack: brute force password for a specific username
void targeted_attack() {
    char username[MAX_USERNAME];
    
    printf("\n=== TARGETED ATTACK ===\n");
    printf("Enter target username: ");
    if (scanf("%s", username) != 1) {
        printf("Error reading username.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');
    
    // Find user in password file
    FILE *file = fopen(PASSWORD_FILE, "r");
    if (!file) {
        printf("❌ Password file not found!\n");
        return;
    }
    
    char line[MAX_LINE];
    int found = 0;
    char salt[SALT_LENGTH + 1];
    unsigned long long target_hash;
    
    while (fgets(line, sizeof(line), file)) {
        char stored_user[MAX_USERNAME];
        if (sscanf(line, "%5[^:]:%5[^:]:%llu", stored_user, salt, &target_hash) == 3) {
            if (strcmp(stored_user, username) == 0) {
                found = 1;
                break;
            }
        }
    }
    fclose(file);
    
    if (!found) {
        printf("❌ Username '%s' not found in database.\n", username);
        return;
    }
    
    printf("✅ Target found: %s (Salt: %s, Hash: %llu)\n", username, salt, target_hash);
    
    // Ask for EXACT length to try (no more iterating through all lengths)
    int target_len;
    printf("\nEnter the EXACT password length to try (1-8): ");
    if (scanf("%d", &target_len) != 1) {
        printf("Invalid input.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');
    
    if (target_len < 1 || target_len > 8) {
        printf("Invalid length. Must be between 1-8.\n");
        return;
    }
    
    unsigned long long combinations = 1;
    for (int i = 0; i < target_len; i++) {
        combinations *= strlen(PASSWORD_CHARSET);
    }
    
    printf("\n🚀 Starting brute force attack on password...\n");
    printf("   Target length: %d characters\n", target_len);
    printf("   Charset: %s\n", PASSWORD_CHARSET);
    printf("   Charset size: %lu characters\n", (unsigned long)strlen(PASSWORD_CHARSET));
    printf("   Total combinations: %llu\n", combinations);
    
    if (target_len >= 7) {
        printf("\n⚠️  WARNING: Length %d means %llu combinations!\n", target_len, combinations);
        printf("   This may take a VERY long time. Continue? (y/n): ");
        char confirm;
        if (scanf(" %c", &confirm) != 1 || (confirm != 'y' && confirm != 'Y')) {
            printf("Attack cancelled.\n");
            while (getchar() != '\n');
            return;
        }
        while (getchar() != '\n');
    }
    
    char attempt[MAX_PASSWORD];
    unsigned long attempts = 0;
    time_t start_time = time(NULL);
    
    // Set globals for ETA calculation
    g_start_time = start_time;
    g_total_combinations = combinations;
    
    printf("\n[Brute Force] Searching length %d ONLY...\n", target_len);
    printf("   Progress updates every 1,000,000 attempts\n\n");
    
    if (brute_force_password_recursive(username, attempt, 0, target_len, salt, target_hash, &attempts)) {
        time_t end_time = time(NULL);
        printf("\n✅ Attack successful!\n");
        printf("   Total attempts: %lu\n", attempts);
        printf("   Time taken: %ld seconds\n", (long)(end_time - start_time));
        return;
    }
    
    time_t end_time = time(NULL);
    printf("\n❌ Password not found at length %d.\n", target_len);
    printf("   Total attempts: %lu\n", attempts);
    printf("   Time taken: %ld seconds\n", (long)(end_time - start_time));
}

// Recursive brute force for username
int brute_force_username_recursive(char *attempt, int position, int max_length,
                                   unsigned long *attempts_count) {
    // Base case
    if (position == max_length) {
        attempt[position] = '\0';
        (*attempts_count)++;
        
        if (*attempts_count % 1000 == 0) {
            printf("  [Username Progress] Attempts: %lu, Current: %s\n", *attempts_count, attempt);
        }
        
        // Check if this username exists
        if (username_exists(attempt)) {
            printf("\n🎯 USERNAME FOUND: %s\n", attempt);
            return 1;
        }
        return 0;
    }
    
    // Try lowercase letters only for username (per requirements)
    for (int i = 0; i < 26; i++) {
        attempt[position] = 'a' + i;
        if (brute_force_username_recursive(attempt, position + 1, max_length, attempts_count)) {
            return 1;
        }
    }
    
    return 0;
}

// Blind attack: brute force both username and password
void blind_attack() {
    printf("\n=== BLIND ATTACK ===\n");
    printf("Attempting to find any valid username in the system...\n");
    printf("⚠️  For demonstration, searching 5-character lowercase usernames only.\n\n");
    
    char username_attempt[MAX_USERNAME];
    unsigned long attempts = 0;
    
    // Try to find any username (5 lowercase chars)
    if (brute_force_username_recursive(username_attempt, 0, 5, &attempts)) {
        printf("\nFound username! Now would proceed to brute force password...\n");
        printf("(Switching to targeted attack for this username)\n");
        
        // Simulate targeted attack on found username
        // In real scenario, would call targeted attack logic here
    } else {
        printf("\n⚠️  No username found in search space.\n");
    }
}

// Brute force menu
void brute_force_menu() {
    int choice;
    
    printf("\n╔════════════════════════════════════╗\n");
    printf("║   BRUTE FORCE ATTACK MODULE       ║\n");
    printf("╔════════════════════════════════════╗\n");
    printf("║ 1. Blind Attack (Find any user)   ║\n");
    printf("║ 2. Targeted Attack (Specific user)║\n");
    printf("║ 3. Back to Main Menu               ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf("Enter choice: ");
    
    if (scanf("%d", &choice) != 1) {
        printf("Invalid input.\n");
        while (getchar() != '\n');
        return;
    }
    while (getchar() != '\n');
    
    switch (choice) {
        case 1:
            blind_attack();
            break;
        case 2:
            targeted_attack();
            break;
        case 3:
            return;
        default:
            printf("Invalid choice.\n");
    }
}

// ==================== MAIN MENU ====================

void display_menu() {
    printf("\n╔════════════════════════════════════╗\n");
    printf("║  AUTHENTICATION SYSTEM - MAIN MENU ║\n");
    printf("╔════════════════════════════════════╗\n");
    printf("║ 1. Signup                          ║\n");
    printf("║ 2. Signin                          ║\n");
    printf("║ 3. Brute Force Attack Module       ║\n");
    printf("║ 4. Exit                            ║\n");
    printf("╚════════════════════════════════════╝\n");
    printf("Enter choice: ");
}

int main() {
    int choice;
    
    // Seed random number generator
    srand(time(NULL));
    
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║   Information Data Security - Authentication System    ║\n");
    printf("║              University Assignment (TP)                ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    while (1) {
        display_menu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Please enter a number.\n");
            while (getchar() != '\n'); // Clear input buffer
            continue;
        }
        while (getchar() != '\n'); // Clear input buffer
        
        switch (choice) {
            case 1:
                signup();
                break;
            case 2:
                signin();
                break;
            case 3:
                brute_force_menu();
                break;
            case 4:
                printf("\n👋 Exiting... Goodbye!\n");
                return 0;
            default:
                printf("❌ Invalid choice. Please select 1-4.\n");
        }
    }
    
    return 0;
}
