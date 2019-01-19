#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char rank;
    char suit;
} Card;

typedef struct {
    int id;
    char* name;
    Card* hand;
    int cardCount;
    FILE* readFD;
    FILE* writeFD;
    Card* lastPlay;
    int eligible;
} Player;

typedef struct {
    int fd;
    int ipAddress;
    int port;
    char* greeting;
    int deckCount;
    char** decks;
} Server;

typedef struct {
    char* name;
    Player* players;
    int playerCount;
    int currentDeck;
    char trumps;
    int contractGoal;
    int contractPoints;
    int contractTeam;
    int team1Wins;
    int team2Wins;
    int team1Points;
    int team2Points;
} Game;

void print_message(char*);
void store_hand(char*, Player*);
int compare(const void*, const void*);
void print_cards(Player*);
void ask_for_bid(char*, Player*);
int is_higher(Card*, Card*);
int is_higher_bid(Card*, Card*);
char* card_to_string(Card*);
void read_card_input(Card*);
void read_card_input_from_string(Card*, char*);
void ask_for_play(char*, Player*);
int is_in_hand(Card*, Player*);
int match_cards(Card*, Card*);
int is_in_suit(Card*, char);
void remove_card_from_hand(Player*, Card*);
int is_valid_bid(Card*);
int is_valid_card(Card*);
int validate_deck(char*);
int can_play_suit(Player*, char);
char* create_message(char, char*);
int compare_players(const void*, const void*);
int ends_with(const char*, const char*);
