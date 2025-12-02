#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdarg.h>

#include "../include/blackjack.h"
#include "../include/deck.h"

#define MAX_PLAYERS 5
#define BUFFER_SIZE 512

typedef struct {
    int socket_fd;
    Hand hand;
    int active;      // 1 = connected and playing, 0 = free slot
} PlayerConn;

/* ---------- helpers for sending / receiving ---------- */

static ssize_t send_all(int fd, const char *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(fd, buf + sent, len - sent, 0);
        if (n <= 0) return n;
        sent += (size_t)n;
    }
    return (ssize_t)sent;
}

static int sendf(int fd, const char *fmt, ...) {
    char buf[BUFFER_SIZE];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n <= 0) return -1;
    return (send_all(fd, buf, (size_t)n) > 0) ? 0 : -1;
}

static int recv_line(int fd, char *buf, size_t bufsz) {
    size_t idx = 0;
    while (idx + 1 < bufsz) {
        char c;
        ssize_t r = recv(fd, &c, 1, 0);
        if (r <= 0) return -1;     // error or disconnect
        if (c == '\n') break;
        buf[idx++] = c;
    }
    buf[idx] = '\0';
    return (int)idx;
}

static void trim_newline(char *s) {
    size_t L = strlen(s);
    while (L > 0 && (s[L - 1] == '\n' || s[L - 1] == '\r')) {
        s[L - 1] = '\0';
        L--;
    }
}

/* ---------- player management ---------- */

static int count_active(PlayerConn *players) {
    int c = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
        if (players[i].active) c++;
    return c;
}

/* Accept as many pending connections as possible, non-blocking using select() */
static void accept_new_players(int listen_fd, PlayerConn *players) {
    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int rv = select(listen_fd + 1, &rfds, NULL, NULL, &tv);
        if (rv <= 0) {
            // no more pending connections
            break;
        }

        struct sockaddr_in cliaddr;
        socklen_t clen = sizeof(cliaddr);
        int cfd = accept(listen_fd, (struct sockaddr *)&cliaddr, &clen);
        if (cfd < 0) {
            perror("accept");
            break;
        }

        int placed = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].active) {
                players[i].socket_fd = cfd;
                players[i].active = 1;
                hand_init(&players[i].hand);
                char ipbuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cliaddr.sin_addr, ipbuf, sizeof(ipbuf));
                printf("Player %d connected from %s:%d\n",
                       i + 1, ipbuf, ntohs(cliaddr.sin_port));
                sendf(cfd, "WELCOME Player %d\n", i + 1);
                placed = 1;
                break;
            }
        }

        if (!placed) {
            sendf(cfd, "SERVER_FULL\n");
            close(cfd);
        }
    }
}

/* Blocking wait until at least one player is connected */
static void wait_for_first_player(int listen_fd, PlayerConn *players) {
    while (count_active(players) == 0) {
        printf("Waiting for players...\n");
        struct sockaddr_in cliaddr;
        socklen_t clen = sizeof(cliaddr);
        int cfd = accept(listen_fd, (struct sockaddr *)&cliaddr, &clen);
        if (cfd < 0) {
            perror("accept");
            continue;
        }

        // put into first free slot
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].active) {
                players[i].socket_fd = cfd;
                players[i].active = 1;
                hand_init(&players[i].hand);
                char ipbuf[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &cliaddr.sin_addr, ipbuf, sizeof(ipbuf));
                printf("Player %d connected from %s:%d\n",
                       i + 1, ipbuf, ntohs(cliaddr.sin_port));
                sendf(cfd, "WELCOME Player %d\n", i + 1);
                break;
            }
        }
        // After the first one, we'll grab any additional queued ones
        accept_new_players(listen_fd, players);
    }
}

/* ---------- game helpers ---------- */

static void send_initial_hands(PlayerConn *players, Hand *dealer) {
    char handbuf[256];
    char cardbuf[16];

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active) continue;
        hand_to_string(&players[i].hand, handbuf, sizeof(handbuf));
        card_to_string(&dealer->cards[0], cardbuf, sizeof(cardbuf));
        sendf(players[i].socket_fd, "DEALER_UP %s\n", cardbuf);
        sendf(players[i].socket_fd, "YOUR_HAND %s\n", handbuf);
    }
}

static void handle_player_turn(PlayerConn *player, Hand *dealer, Deck *deck) {
    char line[BUFFER_SIZE];
    char handbuf[256], cardbuf[16];

    if (hand_is_blackjack(&player->hand)) {
        sendf(player->socket_fd, "BLACKJACK\n");
        return;
    }

    while (1) {
        hand_to_string(&player->hand, handbuf, sizeof(handbuf));
        sendf(player->socket_fd, "YOUR_TURN\n");
        sendf(player->socket_fd, "HAND %s\n", handbuf);
        sendf(player->socket_fd, "PROMPT HIT or STAND\n");

        int r = recv_line(player->socket_fd, line, sizeof(line));
        if (r <= 0) {
            // disconnected during turn
            close(player->socket_fd);
            player->socket_fd = -1;
            player->active = 0;
            printf("Player disconnected during turn.\n");
            return;
        }
        trim_newline(line);
        for (char *p = line; *p; ++p) {
            *p = (char)toupper((unsigned char)*p);
        }

        if (strcmp(line, "HIT") == 0) {
            Card c = deck_deal(deck);
            hand_add_card(&player->hand, c);
            card_to_string(&c, cardbuf, sizeof(cardbuf));
            sendf(player->socket_fd, "HIT %s\n", cardbuf);

            if (hand_is_bust(&player->hand)) {
                sendf(player->socket_fd, "BUST %d\n", hand_value(&player->hand));
                break;
            }
            if (hand_value(&player->hand) == 21) {
                sendf(player->socket_fd, "STAND 21\n");
                break;
            }
            // otherwise loop again
        } else if (strcmp(line, "STAND") == 0) {
            sendf(player->socket_fd, "STAND %d\n", hand_value(&player->hand));
            break;
        } else {
            sendf(player->socket_fd, "UNKNOWN_COMMAND\n");
        }
    }
}

static void play_dealer_hand(Hand *dealer, Deck *deck) {
    while (hand_value(dealer) < 17) {
        hand_add_card(dealer, deck_deal(deck));
    }
}

static void send_results(PlayerConn *players, Hand *dealer) {
    char dealer_str[256];
    char handbuf[256];
    hand_to_string(dealer, dealer_str, sizeof(dealer_str));
    int dealer_val = hand_value(dealer);

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active) continue;
        int fd = players[i].socket_fd;

        hand_to_string(&players[i].hand, handbuf, sizeof(handbuf));
        int pv = hand_value(&players[i].hand);

        sendf(fd, "DEALER_HAND %s\n", dealer_str);
        sendf(fd, "DEALER_VALUE %d\n", dealer_val);
        sendf(fd, "PLAYER_VALUE %d\n", pv);

        if (pv > 21) {
            sendf(fd, "RESULT LOSE\n");
        } else if (dealer_val > 21) {
            sendf(fd, "RESULT WIN\n");
        } else if (pv > dealer_val) {
            sendf(fd, "RESULT WIN\n");
        } else if (pv < dealer_val) {
            sendf(fd, "RESULT LOSE\n");
        } else {
            sendf(fd, "RESULT PUSH\n");
        }

        // mark end of this round for the client
        sendf(fd, "ROUND_END\n");
    }
}

/* Play one full round with all currently active players */
static void play_round(PlayerConn *players, int listen_fd) {
    // fresh deck and dealer hand every round
    Deck deck;
    deck_init(&deck);
    deck_shuffle(&deck);

    Hand dealer;
    hand_init(&dealer);

    // reset all player hands
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active) {
            hand_init(&players[i].hand);
        }
    }

    // initial deal: 2 cards each active player, 2 to dealer
    for (int r = 0; r < 2; r++) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i].active) continue;
            Card c = deck_deal(&deck);
            hand_add_card(&players[i].hand, c);
        }
        Card dc = deck_deal(&deck);
        hand_add_card(&dealer, dc);
    }

    send_initial_hands(players, &dealer);

    // each player takes a turn
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active) continue;
        handle_player_turn(&players[i], &dealer, &deck);
    }

    // dealer then plays
    play_dealer_hand(&dealer, &deck);

    // send results to everyone
    send_results(players, &dealer);

    printf("Round finished.\n");
}

/* ---------- main ---------- */

int main(int argc, char *argv[]) {
    int port = 12345;
    if (argc >= 2) port = atoi(argv[1]);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, MAX_PLAYERS) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    printf("Blackjack dealer listening on port %d\n", port);

    PlayerConn players[MAX_PLAYERS];
    for (int i = 0; i < MAX_PLAYERS; i++) {
        players[i].socket_fd = -1;
        players[i].active    = 0;
        hand_init(&players[i].hand);
    }

    while (1) {
        if (count_active(players) == 0) {
            // Wait (blocking) for the first player to join
            wait_for_first_player(listen_fd, players);
        }

        // Accept any extra players that connected since last round
        accept_new_players(listen_fd, players);

        if (count_active(players) == 0) {
            // Could happen if they disconnected very quickly
            continue;
        }

        play_round(players, listen_fd);

        if (count_active(players) == 0) {
            printf("All players left. Shutting down.\n");
            break;
        }

        // Loop again for next round. New players that connected during
        // the round will be accepted at the top of the loop.
    }

    // Clean up
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].active && players[i].socket_fd >= 0) {
            close(players[i].socket_fd);
        }
    }
    close(listen_fd);
    return 0;
}

