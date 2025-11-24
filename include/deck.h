#include <stdio.h>

typedef struct {
    int rank;
    char suit;
} Card;

typedef struct {
    Card cards[52];
    int *top;
} Deck;


void deck_init(Deck *deck) {
    char suits[4] = {'S', 'H', 'D', 'C'};
    for (int suit = 0; suit < 3; suit++) {
        for (int i = 1; i <= 13; i++) {
            deck->cards[i-1] = Card {i, suits[suit]};
        }
    }
}

void deck_shuffle(Deck *deck) {

}

Card deck_draw(Deck *deck) {

}

void deck_print_card(Card c) {

}