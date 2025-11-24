typedef struct {
    int sock, chips, bet;
    int hand[12], hand_size;
    int bust, blackjack;
} Player;

typedef struct {
    Player players[6];
    int num_players;
    int dealer_hand[12];
    int dealer_size;
    int deck[312], deck_top;
} Table;
