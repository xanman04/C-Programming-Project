#include "../include/deck.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void deck_init(Deck *deck) {
    int idx = 0;
    for (int suit = 0; suit < 4; suit++) {
        for (int rank = 1; rank <= 13; rank++) {
            deck->cards[idx].rank = rank;
            deck->cards[idx].suit = suit;
            idx++;
        }
    }
    deck->top = 0;
}

void deck_shuffle(Deck *deck) {
    static int seeded = 0;
    if (!seeded) {
        unsigned int seed = (unsigned int)(time(NULL) ^ (getpid() << 16));
        srand(seed);
        seeded = 1;
    }

    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card tmp = deck->cards[i];
        deck->cards[i] = deck->cards[j];
        deck->cards[j] = tmp;
    }
    deck->top = 0;
}

Card deck_deal(Deck *deck) {
    if (deck->top >= DECK_SIZE) {
        deck_shuffle(deck);
    }
    return deck->cards[deck->top++];
}

const char *card_to_string(const Card *card, char *buf, size_t bufsize) {
    const char *rank_s;
    char rank_buf[4];

    if (card->rank == 1) rank_s = "A";
    else if (card->rank == 11) rank_s = "J";
    else if (card->rank == 12) rank_s = "Q";
    else if (card->rank == 13) rank_s = "K";
    else {
        snprintf(rank_buf, sizeof(rank_buf), "%d", card->rank);
        rank_s = rank_buf;
    }

    char suit_c;
    switch (card->suit) {
        case 0: suit_c = 'C'; break;
        case 1: suit_c = 'D'; break;
        case 2: suit_c = 'H'; break;
        case 3: suit_c = 'S'; break;
        default: suit_c = '?'; break;
    }

    snprintf(buf, bufsize, "%s%c", rank_s, suit_c);
    return buf;
}
