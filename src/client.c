#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT ----
#define BUFFER_SIZE ----

int main(int argc, char *argv[]) {
    char *server_ip = "----";
    
    if (argc > 1) {
        server_ip = argv[1];
    }
    
    printf("=== Blackjack TCP Client ===\n");
    printf("Connecting to server: %s:%d\n", server_ip, PORT);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }
    
    printf("Connected to blackjack server!\n");
    printf("Type commands, then press Enter.\n");
    printf("Examples: \"BET 50\", \"HIT\", \"STAND\", \"exit\" to quit.\n\n");
    
    char buffer[BUFFER_SIZE];
    
    while (1) {
        printf("You> ");
        fflush(stdout);

        if (!fgets(buffer, BUFFER_SIZE, stdin)) {
            // EOF or read error
            break;
        }
        
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("send");
            break;
        }
        
        if (strcmp(buffer, "exit\n") == 0 ||
            strcmp(buffer, "quit\n") == 0) {
            printf("Disconnecting...\n");
            break;
        }
        
        int bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            break;
        }
        
        buffer[bytes_received] = '\0';
        printf("Server> %s\n", buffer);
    }
    
    close(sockfd);
    printf("Connection closed. Goodbye!\n");
    return 0;
}
