#ifndef DECK_H
#define DECK_H

#include <stddef.h>

#define DECK_SIZE 52

typedef struct 
{
    int rank;  /* 1–13 (1=Ace, 11=J, 12=Q, 13=K) */
    int suit;  /* 0–3 (Clubs, Diamonds, Hearts, Spades) */
} Card;

typedef struct 
{
    Card cards[DECK_SIZE];
    int top;  /* index of next card to deal */
} Deck;

void deck_init(Deck *deck);
void deck_shuffle(Deck *deck);
Card deck_deal(Deck *deck);
const char *card_to_string(const Card *card, char *buf, size_t bufsize);

#endif /* DECK_H */
