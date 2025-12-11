/*
 * Authentication Server in C
 * HTTP Server with Signup/Signin endpoints
 * Implements account locking and banning mechanisms
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#define PORT 3000
#define MAX_USERS 100
#define USERNAME_LEN 5
#define PASSWORD_MIN_LEN 8
#define SALT_LEN 5
#define HASH_LEN 128
#define BUFFER_SIZE 8192

typedef struct {
    char username[USERNAME_LEN + 1];
    char salt[SALT_LEN + 1];
    char hash[HASH_LEN + 1];
    int failed_sets;
    int attempt_in_set;
    long long lock_until;
    int banned;
} User;

User users[MAX_USERS];
int user_count = 0;
int lock_durations[] = {5, 10, 15, 20}; // seconds

// Generate hash using PBKDF2
void hash_password(const char *password, const char *salt, char *output) {
    unsigned char hash[64];
    PKCS5_PBKDF2_HMAC(password, strlen(password),
                      (unsigned char *)salt, strlen(salt),
                      100000, EVP_sha256(), 64, hash);
    
    for (int i = 0; i < 64; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[128] = '\0';
}

// Generate random 5-digit salt
void generate_salt(char *salt) {
    int num = 10000 + rand() % 90000;
    sprintf(salt, "%d", num);
}

// Load users from file
void load_users() {
    FILE *fp = fopen("password", "r");
    if (!fp) {
        user_count = 0;
        return;
    }
    
    user_count = 0;
    char line[512];
    while (fgets(line, sizeof(line), fp) && user_count < MAX_USERS) {
        char *token = strtok(line, ",");
        if (!token) continue;
        strcpy(users[user_count].username, token);
        
        token = strtok(NULL, ",");
        if (!token) continue;
        strcpy(users[user_count].salt, token);
        
        token = strtok(NULL, ",");
        if (!token) continue;
        strcpy(users[user_count].hash, token);
        
        token = strtok(NULL, ",");
        users[user_count].failed_sets = token ? atoi(token) : 0;
        
        token = strtok(NULL, ",");
        users[user_count].attempt_in_set = token ? atoi(token) : 0;
        
        token = strtok(NULL, ",");
        users[user_count].lock_until = token ? atoll(token) : 0;
        
        token = strtok(NULL, ",\n\r");
        users[user_count].banned = (token && strcmp(token, "true") == 0) ? 1 : 0;
        
        user_count++;
    }
    fclose(fp);
}

// Save users to file
void save_users() {
    FILE *fp = fopen("password", "w");
    if (!fp) {
        perror("Failed to save users");
        return;
    }
    
    for (int i = 0; i < user_count; i++) {
        fprintf(fp, "%s,%s,%s,%d,%d,%lld,%s\n",
                users[i].username,
                users[i].salt,
                users[i].hash,
                users[i].failed_sets,
                users[i].attempt_in_set,
                users[i].lock_until,
                users[i].banned ? "true" : "false");
    }
    fclose(fp);
}

// Find user by username
User* find_user(const char *username) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return &users[i];
        }
    }
    return NULL;
}

// Validate username: 5 characters (a-z, A-Z, 0-9)
int validate_username(const char *username) {
    if (strlen(username) != USERNAME_LEN) return 0;
    for (int i = 0; i < USERNAME_LEN; i++) {
        if (!isalnum(username[i])) return 0;
    }
    return 1;
}

// Validate password: 8+ chars with lowercase, uppercase, digit, special (@#$&)
int validate_password(const char *password) {
    int len = strlen(password);
    if (len < PASSWORD_MIN_LEN) return 0;
    
    int has_lower = 0, has_upper = 0, has_digit = 0, has_special = 0;
    for (int i = 0; i < len; i++) {
        if (islower(password[i])) has_lower = 1;
        else if (isupper(password[i])) has_upper = 1;
        else if (isdigit(password[i])) has_digit = 1;
        else if (strchr("@#$&", password[i])) has_special = 1;
    }
    
    return has_lower && has_upper && has_digit && has_special;
}

// Get current time in milliseconds
long long current_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Extract JSON field value
void extract_json_field(const char *json, const char *field, char *output, int max_len) {
    char pattern[64];
    sprintf(pattern, "\"%s\"", field);
    const char *start = strstr(json, pattern);
    if (!start) {
        output[0] = '\0';
        return;
    }
    
    start = strchr(start + strlen(pattern), ':');
    if (!start) {
        output[0] = '\0';
        return;
    }
    start++;
    
    while (*start && isspace(*start)) start++;
    if (*start == '"') {
        start++;
        const char *end = strchr(start, '"');
        if (end) {
            int len = end - start;
            if (len >= max_len) len = max_len - 1;
            strncpy(output, start, len);
            output[len] = '\0';
            return;
        }
    }
    output[0] = '\0';
}

// Send HTTP response
void send_response(int client_fd, int status, const char *json_body) {
    char response[BUFFER_SIZE];
    const char *status_text = (status == 200) ? "OK" : 
                              (status == 400) ? "Bad Request" :
                              (status == 401) ? "Unauthorized" :
                              (status == 403) ? "Forbidden" :
                              (status == 429) ? "Too Many Requests" : "Error";
    
    int body_len = strlen(json_body);
    int header_len = sprintf(response,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n", status, status_text, body_len);
    
    memcpy(response + header_len, json_body, body_len);
    write(client_fd, response, header_len + body_len);
}

// Handle signup
void handle_signup(int client_fd, const char *body) {
    char username[64] = {0};
    char password[128] = {0};
    
    extract_json_field(body, "username", username, sizeof(username));
    extract_json_field(body, "password", password, sizeof(password));
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"username and password required\"}");
        return;
    }
    
    if (!validate_username(username)) {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"username must be 5 alphanumeric characters\"}");
        return;
    }
    
    if (!validate_password(password)) {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"password must be 8+ chars with lowercase, uppercase, digit and special (@#$&)\"}");
        return;
    }
    
    if (find_user(username)) {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"username already exists\"}");
        return;
    }
    
    if (user_count >= MAX_USERS) {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"server full\"}");
        return;
    }
    
    // Create new user
    User *new_user = &users[user_count++];
    strcpy(new_user->username, username);
    generate_salt(new_user->salt);
    hash_password(password, new_user->salt, new_user->hash);
    new_user->failed_sets = 0;
    new_user->attempt_in_set = 0;
    new_user->lock_until = 0;
    new_user->banned = 0;
    
    save_users();
    send_response(client_fd, 200, "{\"ok\":true,\"msg\":\"signup successful\"}");
}

// Handle signin
void handle_signin(int client_fd, const char *body) {
    char username[64] = {0};
    char password[128] = {0};
    
    extract_json_field(body, "username", username, sizeof(username));
    extract_json_field(body, "password", password, sizeof(password));
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"username and password required\"}");
        return;
    }
    
    User *user = find_user(username);
    if (!user) {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"user not found\"}");
        return;
    }
    
    if (user->banned) {
        send_response(client_fd, 403, "{\"ok\":false,\"msg\":\"account is banned\"}");
        return;
    }
    
    // Check password (no locking)
    char attempted_hash[HASH_LEN + 1];
    hash_password(password, user->salt, attempted_hash);
    
    if (strcmp(attempted_hash, user->hash) == 0) {
        // Success
        send_response(client_fd, 200, "{\"ok\":true,\"msg\":\"signin successful\"}");
        return;
    }
    
    // Wrong password
    send_response(client_fd, 401, "{\"ok\":false,\"msg\":\"wrong password\"}");
}

// Handle HTTP request
void handle_request(int client_fd) {
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    
    // Parse request
    char method[16], path[256];
    sscanf(buffer, "%s %s", method, path);
    
    // Handle OPTIONS for CORS
    if (strcmp(method, "OPTIONS") == 0) {
        char response[] = "HTTP/1.1 200 OK\r\n"
                         "Access-Control-Allow-Origin: *\r\n"
                         "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                         "Access-Control-Allow-Headers: Content-Type\r\n"
                         "Connection: close\r\n\r\n";
        write(client_fd, response, strlen(response));
        close(client_fd);
        return;
    }
    
    // Find body
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) body += 4;
    else body = "";
    
    if (strcmp(method, "POST") == 0) {
        if (strcmp(path, "/api/signup") == 0) {
            handle_signup(client_fd, body);
        } else if (strcmp(path, "/api/signin") == 0) {
            handle_signin(client_fd, body);
        } else {
            send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"unknown endpoint\"}");
        }
    } else {
        send_response(client_fd, 400, "{\"ok\":false,\"msg\":\"method not allowed\"}");
    }
    
    close(client_fd);
}

int main() {
    srand(time(NULL));
    load_users();
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }
    
    printf("Auth server listening on http://localhost:%d\n", PORT);
    printf("Users loaded: %d\n", user_count);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd < 0) {
            perror("Accept failed");
            continue;
        }
        
        handle_request(client_fd);
    }
    
    close(server_fd);
    return 0;
}
