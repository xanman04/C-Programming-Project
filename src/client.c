/*
 * Simple Blackjack Client
 * Connects to a Blackjack server and plays the game based on server prompts.
 * 
 * Usage: ./client <server-ip> <port>
 * 
 * This client handles server messages, displays game state, and prompts the user for actions.
 */

 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctype.h>

#define BUFFER_SIZE 512

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s <server-ip> <port>\n", prog);
}

static int recv_line(int fd, char *buf, size_t bufsz) {
    size_t idx = 0;
    while (idx + 1 < bufsz) {
        char c;
        ssize_t r = recv(fd, &c, 1, 0);
        if (r <= 0) return -1;  // error or disconnect
        if (c == '\n') break;
        buf[idx++] = c;
    }
    buf[idx] = '\0';
    return (int)idx;
}

static int send_line(int fd, const char *s) {
    size_t len = strlen(s);
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, s + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    if (send(fd, "\n", 1, 0) <= 0) return -1;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", server_ip);
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Connected to blackjack server %s:%d\n", server_ip, port);
    printf("Waiting for rounds. Ctrl+C to quit.\n\n");

    char line[BUFFER_SIZE];

    while (1) {
        int r = recv_line(sock, line, sizeof(line));
        if (r <= 0) {
            printf("Connection closed by server.\n");
            break;
        }

        // Split into command + rest
        char *cmd  = strtok(line, " ");
        char *rest = strtok(NULL, "");

        if (!cmd) continue;

        if (strcmp(cmd, "WELCOME") == 0) {
            printf("%s %s\n", cmd, rest ? rest : "");
        }
        else if (strcmp(cmd, "SERVER_FULL") == 0) {
            printf("Server is full. Try again later.\n");
            break;
        }
        else if (strcmp(cmd, "DEALER_UP") == 0) {
            printf("\nDealer shows: %s\n", rest ? rest : "?");
        }
        else if (strcmp(cmd, "YOUR_HAND") == 0) {
            printf("Your initial hand: %s\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "YOUR_TURN") == 0) {
            printf("\n--- Your turn ---\n");
        }
        else if (strcmp(cmd, "HAND") == 0) {
            printf("Your hand: %s\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "PROMPT") == 0) {
            // PROMPT HIT or STAND
            char input[BUFFER_SIZE];
            printf("Hit or Stand? (h/s): ");
            fflush(stdout);
            if (!fgets(input, sizeof(input), stdin)) {
                // On input failure, default to STAND
                send_line(sock, "STAND");
                continue;
            }
            char c = (char)tolower((unsigned char)input[0]);
            if (c == 'h')
                send_line(sock, "HIT");
            else
                send_line(sock, "STAND");
        }
        else if (strcmp(cmd, "HIT") == 0) {
            printf("You drew: %s\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "BUST") == 0) {
            printf("You busted with %s.\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "STAND") == 0) {
            printf("You stand with %s.\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "BLACKJACK") == 0) {
            printf("Blackjack!\n");
        }
        else if (strcmp(cmd, "DEALER_HAND") == 0) {
            printf("\nDealer hand: %s\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "DEALER_VALUE") == 0) {
            printf("Dealer value: %s\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "PLAYER_VALUE") == 0) {
            printf("Your value: %s\n", rest ? rest : "");
        }
        else if (strcmp(cmd, "RESULT") == 0) {
            if (!rest) rest = "";
            if (strcmp(rest, "WIN") == 0)
                printf("\n>>> You WIN! 🎉\n");
            else if (strcmp(rest, "LOSE") == 0)
                printf("\n>>> You lose.\n");
            else if (strcmp(rest, "PUSH") == 0)
                printf("\n>>> Push (tie).\n");
            else
                printf("\n>>> Result: %s\n", rest);
        }
        else if (strcmp(cmd, "ROUND_END") == 0) {
            // IMPORTANT: don't exit, just wait for next round
            printf("\n--- Round finished. Waiting for next round... ---\n\n");
        }
        else if (strcmp(cmd, "UNKNOWN_COMMAND") == 0) {
            printf("Server did not understand your command.\n");
        }
        else {
            // Fallback: print unknown lines
            printf("%s", cmd);
            if (rest) printf(" %s", rest);
            printf("\n");
        }
    }

    close(sock);
    return 0;
}
