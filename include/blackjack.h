#ifndef BLACKJACK_H
#define BLACKJACK_H

#include "deck.h"
#include <stddef.h>

#define MAX_HAND_CARDS 12

typedef struct 
{
    Card cards[MAX_HAND_CARDS];
    int count;
} Hand;

void hand_init(Hand *hand);
void hand_add_card(Hand *hand, Card card);
int hand_value(const Hand *hand);
int hand_is_blackjack(const Hand *hand);
int hand_is_bust(const Hand *hand);
void hand_to_string(const Hand *hand, char *buf, size_t bufsize);

#endif /* BLACKJACK_H */
