#include "game.h"
#include "networking.h"

/*
*   Prints out an info message received from the server.
*/
void print_message(char* message) {
    printf("Info: %s\n", message);
}

/*
*   Stores the cards supplied in the message in the player's hand.
*/
void store_hand(char* cards, Player* player) {
    // Initial hand size of 13 cards
    player->hand = malloc(sizeof(Card) * 13);
    player->lastPlay = malloc(sizeof(Card));
    int j = 0;
    for (int i = 0; i < (strlen(cards) - 1); i++) {
        player->hand[j].rank = cards[i++];
        player->hand[j].suit = cards[i];
        j++;
    }
    player->cardCount = 13;
}

/*
*   Comparison function to return the rank of highest importance.
*/
int compare_ranks(char xp, char yp) {
    char xc;
    char yc;
    switch (xp) {
        case 'J':
            xc = 85;
            break;
        case 'Q':
            xc = 86;
            break;
        case 'K':
            xc = 87;
            break;
        case 'A':
            xc = 88;
            break;
        default:
            xc = xp;
    }
    switch (yp) {
        case 'J':
            yc = 85;
            break;
        case 'Q':
            yc = 86;
            break;
        case 'K':
            yc = 87;
            break;
        case 'A':
            yc = 88;
            break;
        default:
            yc = yp;
    }
    if (yc < xc) {
        return -1;
    } else if (yc == xc) {
        return 0;
    } else {
        return 1;
    }
}

/*
*   Compares the string names of two players.
*/
int compare_players(const void *x, const void *y) {
    return (strcmp(*(const char **)x, *(const char **)y));
}

/*
*   Compares the values of two cards, to be used in the qsort function.
*/
int compare(const void *x, const void *y) {
    const char xp = (*(const char **)x)[0];
    const char yp = (*(const char **)y)[0];
    return compare_ranks(xp, yp);
}

/*
* Print the current card content of the player's hand to stdout.
*/
void print_cards(Player* player) {
    char** cards = malloc(sizeof(char*) * player->cardCount);
    for (int i = 0; i < player->cardCount; i++) {
        cards[i] = card_to_string(&player->hand[i]);
    }
    qsort(cards, player->cardCount, sizeof(cards[0]), &compare);
    char hearts[14], diamonds[14], spades[14], clubs[14];
    int h = 0, d = 0, s = 0, c = 0;
    for (int i = 0; i < player->cardCount; i++) {
        switch (cards[i][1]) {
            case 'H':
                hearts[h++] = cards[i][0];
                break;
            case 'D':
                diamonds[d++] = cards[i][0];
                break;
            case 'S':
                spades[s++] = cards[i][0];
                break;
            case 'C':
                clubs[c++] = cards[i][0];
                break;
        }
    }
    hearts[h] = '\0';
    spades[s] = '\0';
    diamonds[d] = '\0';
    clubs[c] = '\0';
    printf("S:");
    for (int i = 0; i < strlen(spades); i++) {
        printf(" %c", spades[i]);
    }
    printf("\nC:");
    for (int i = 0; i < strlen(clubs); i++) {
        printf(" %c", clubs[i]);
    }
    printf("\nD:");
    for (int i = 0; i < strlen(diamonds); i++) {
        printf(" %c", diamonds[i]);
    }
    printf("\nH:");
    for (int i = 0; i < strlen(hearts); i++) {
        printf(" %c", hearts[i]);
    }
    printf("\n");
    for (int i = 0; i < player->cardCount; i++) {
        free(cards[i]);
    }
    free(cards);
}

/*
*   Promps the player for a valid bid and continues to prompt until
*   one is given.
*/
void ask_for_bid(char* message, Player* player) {
    Card* card = malloc(sizeof(Card));
    int valid = 0;
    if (strlen(message) == 0) {
        while (!valid) {
            printf("Bid> ");
            read_card_input(card);
            if (is_valid_bid(card)) {
                send_socket_message(player->writeFD, card_to_string(card));
                valid = 1;
            }
        }
    } else {
        Card* baseCard = malloc(sizeof(Card));
        baseCard->rank = message[0];
        baseCard->suit = message[1];

        while (!valid) {
            printf("[%s] - Bid (or pass)> ", card_to_string(baseCard));
            fflush(stdout);
            read_card_input(card);
            if ((is_valid_bid(card) && is_higher_bid(baseCard, card)) ||
                    !strcmp(card_to_string(card), "PP")) {
                send_socket_message(player->writeFD, card_to_string(card));
                valid = 1;
            }
        }
        free(baseCard);
    }
    fflush(stdout);
    free(card);
}

/*
*   Checks if a inputted bid is valid.
*/
int is_valid_bid(Card* card) {
    if (!is_valid_card(card) || (card->rank < '4' || card->rank > '9')) {
        return 0;
    }
    return 1;
}

/*
*   Checks if an inputted card is valid.
*/
int is_valid_card(Card* card) {
    char* ranks = "AKQJT";
    char* suits = "HDSC";
    if (!strchr(suits, card->suit) || (card->rank < '2' ||
            (card->rank > '9' && !strchr(ranks, card->rank)))) {
        return 0;
    }
    return 1;
}

/*
*   Prompts the player for a valid card to play in the trick.
*/
void ask_for_play(char* message, Player* player) {
    Card* card = malloc(sizeof(Card));
    int valid = 0;
    print_cards(player);
    if (strlen(message) == 0) {
        while(!valid) {
            printf("Lead> ");
            read_card_input(card);
            if (!is_valid_card(card) || !is_in_hand(card, player)) {
                print_cards(player);
            } else {
                memcpy(player->lastPlay, card, sizeof(Card));
                send_socket_message(player->writeFD, card_to_string(card));
                valid = 1;
            }
        }

    } else {
        while (!valid) {
            printf("[%c] play> ", message[0]);
            read_card_input(card);
            if (((can_play_suit(player, message[0]) &&
                    (card->suit != message[0])) || !is_valid_card(card)) ||
                    !is_in_hand(card, player)) {
                print_cards(player);
            } else if (is_in_hand(card, player)) {
                memcpy(player->lastPlay, card, sizeof(Card));
                send_socket_message(player->writeFD, card_to_string(card));
                valid = 1;
            }
        }
    }
    free(card);
}

/*
*   Checks if a given card is present in the players hand.
*/
int is_in_hand(Card* card, Player* player) {
    for (int i = 0; i < player->cardCount; i++) {
        if (match_cards(card, &player->hand[i])) {
            return 1;
        }
    }
    return 0;
}

/*
*   Checks if a given suit is in the players hand.
*/
int can_play_suit(Player* player, char suit) {
    for (int i = 0; i < player->cardCount; i++) {
        if (suit == player->hand[i].suit) {
            return 1;
        }
    }
    return 0;
}

/*
*   Checks if two given cards are identical.
*/
int match_cards(Card* card1, Card* card2) {
    if (card1->rank == card2->rank && card1->suit == card2->suit) {
        return 1;
    } else {
        return 0;
    }
}

/*
*   Checks if a given card is in a particular suit.
*/
int is_in_suit(Card* card, char suit) {
    if (card->suit == suit) {
        return 1;
    }
    return 0;
}

/*
*   Convers a card to a printable string.
*/
char* card_to_string(Card* card) {
    char* buffer = malloc(sizeof(char) * 3);
    buffer[0] = card->rank;
    buffer[1] = card->suit;
    buffer[2] = '\0';
    return buffer;
}

/*
*   Returns whether compCard is higher than baseCard.
*/
int is_higher(Card* baseCard, Card* compCard) {
    int suitValue;
    int baseSuitValue;
    if (baseCard == NULL) {
        return 1;
    } else {
        switch (baseCard->suit) {
            case 'H':
                baseSuitValue = 4;
                break;
            case 'D':
                baseSuitValue = 3;
                break;
            case 'C':
                baseSuitValue = 2;
                break;
            case 'S':
                baseSuitValue = 1;
                break;
        }
        switch (compCard->suit) {
            case 'H':
                suitValue = 4;
                break;
            case 'D':
                suitValue = 3;
                break;
            case 'C':
                suitValue = 2;
                break;
            case 'S':
                suitValue = 1;
                break;
        }
        if (suitValue > baseSuitValue) {
            return 1;
        } else if (suitValue < baseSuitValue) {
            return 0;
        } else {
            if (compare_ranks(compCard->rank, baseCard->rank) == -1) {
                return 1;
            } else {
                return 0;
            }
        }
    }
}

/*
*   Returns whether the bid compCard is higher than the bid baseCard.
*/
int is_higher_bid(Card* baseCard, Card* compCard) {
    int suitValue;
    int baseSuitValue;
    if (baseCard == NULL) {
        return 1;
    } else {
        switch (baseCard->suit) {
            case 'H':
                baseSuitValue = 4;
                break;
            case 'D':
                baseSuitValue = 3;
                break;
            case 'C':
                baseSuitValue = 2;
                break;
            case 'S':
                baseSuitValue = 1;
                break;
        }
        switch (compCard->suit) {
            case 'H':
                suitValue = 4;
                break;
            case 'D':
                suitValue = 3;
                break;
            case 'C':
                suitValue = 2;
                break;
            case 'S':
                suitValue = 1;
                break;
        }
        if (compCard->rank > baseCard->rank) {
            return 1;
        } else if (compCard->rank < baseCard->rank) {
            return 0;
        } else {
            if (suitValue > baseSuitValue) {
                return 1;
            } else {
                return 0;
            }
        }
    }
}

/*
*   Removes the given card from the players hand.
*/
void remove_card_from_hand(Player* player, Card* card) {
    int c;
    for (int i = 0; i < player->cardCount; i++) {
        if ((player->hand[i].rank == card->rank) &&
                (player->hand[i].suit == card->suit)) {
            c = i;
            for (int i = c; i < player->cardCount; i++) {
                player->hand[i] = player->hand[i+1];
            }
            player->cardCount--;
            return;
        }
    }
}

/*
*   Read a card input from stdin.
*/
void read_card_input(Card* card) {
    char c;
    if (scanf("%c%c%c", &card->rank, &card->suit, &c) != 3 || c != '\n') {
        // Bad input
        card->rank = 'B';
        card->suit = 'B';
        if (c != '\n') {
            while (getchar() != '\n') {

            }
        }
    }
}

/*
*   Convert a string representation of a card to a card.
*/
void read_card_input_from_string(Card* card, char* msg) {
    if (sscanf(msg, "%c%c", &card->rank, &card->suit) != 2) {
        // Bad input
        card->rank = 'B';
        card->suit = 'B';
    }
}

/*
*   Checks whether a given deck representation contains 52 valid cards.
*/
int validate_deck(char* deck) {
    for (int i = 0; i < 104; i++) {
        Card* card = malloc(sizeof(Card));
        card->rank = deck[i];
        card->suit = deck[++i];
        if (!is_valid_card(card)) {
            free(card);
            return 0;
        }
        free(card);
    }
    return 1;
}

/*
*   Checks whether a string ends with a suffix.
*/
int ends_with(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return 0;
    }
    size_t lenStr = strlen(str);
    size_t lenSuffix = strlen(suffix);
    if (lenSuffix > lenStr) {
        return 0;
    }
    return strncmp(str + lenStr - lenSuffix, suffix, lenSuffix) == 0;
}

/*
*   Creates a string representation of a command to be passed over the network
*/
char* create_message(char type, char* message) {
    char* msg = malloc(strlen(message) + 2);
    msg[0] = type;
    msg[1] = '\0';
    strcat(msg, message);
    return msg;
}
