#include "../include/blackjack.h"
#include <string.h>

void hand_init(Hand *hand) {
    hand->count = 0;
}

void hand_add_card(Hand *hand, Card card) {
    if (hand->count < MAX_HAND_CARDS) {
        hand->cards[hand->count++] = card;
    }
}

int hand_value(const Hand *hand) {
    int total = 0;
    int aces = 0;
    for (int i = 0; i < hand->count; i++) {
        int r = hand->cards[i].rank;
        if (r == 1) {
            aces++;
            total += 11;
        } else if (r >= 11 && r <= 13) {
            total += 10;
        } else {
            total += r;
        }
    }
    while (total > 21 && aces > 0) {
        total -= 10; /* downgrade an ace from 11 to 1 */
        aces--;
    }
    return total;
}

int hand_is_blackjack(const Hand *hand) {
    return (hand->count == 2 && hand_value(hand) == 21);
}

int hand_is_bust(const Hand *hand) {
    return hand_value(hand) > 21;
}

void hand_to_string(const Hand *hand, char *buf, size_t bufsize) {
    char tmp[32];
    buf[0] = '\0';
    for (int i = 0; i < hand->count; i++) {
        card_to_string(&hand->cards[i], tmp, sizeof(tmp));
        if (i > 0) strncat(buf, " ", bufsize - strlen(buf) - 1);
        strncat(buf, tmp, bufsize - strlen(buf) - 1);
    }
}
