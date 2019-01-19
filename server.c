#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "networking.h"
#include "pending.h"

void validate_arguments(int, char**);
void read_deck_file(char*, Server*);
void wait_for_players(int);
void add_player_to_game(Player*, char*);
void check_for_full_games(void);
void* start_game(void*);
void send_welcome_message(FILE*);
void deal_cards(Game*);
void increment_game_deck(Game*);
void initiate_bidding(Game*);
int check_for_eligibility(Game*);
int calculate_contract_points(Card*);
int get_winning_bidder_index(Game*);
void print_teams(Game*);
void send_to_players(Game*, char, char*, int);
int play_trick(Game*, int);
int get_trick_winner(Card**, Game*, char);
void set_points(Game*);
int check_points(Game*);
int check_for_empty_hand(Game*);
void reorder_players(Game*);
void create_player(int);
void check_for_eof(Game*, char*, int);
void get_players_bid(Game*, Card*, Card*, int, int);

// Global instance of the server
Server* server;
// Stores all of the pending games before they are started
PendingGame* pendingGameList;

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);

    // Validate that all of the input arguments are valid
    validate_arguments(argc, argv);

    // Allocate the global server struct instance
    server = malloc(sizeof(Server));

    // Store the server greeting message
    server->greeting = strdup(argv[2]);

    // Initialise the pending game list to zero
    pendingGameList = malloc(sizeof(PendingGame));
    pendingGameList = NULL;

    // Check for a valid deck file and read in the contents
    read_deck_file(argv[3], server);

    // Open a listening socket on the supplied port.
    int fdServer = open_listen(atoi(argv[1]));

    // Wait for incoming client connections
    wait_for_players(fdServer);
}

/*
*   Validate the command line input arguments.
*/
void validate_arguments(int argc, char** argv) {
    if (argc != 4) {
        // Throw usage error (exit(1))
        fprintf(stderr, "Usage: serv499 port greeting deck\n");
        exit(1);
    }

    char** remainder = malloc(sizeof(char**));
    int port = strtol(argv[1], remainder, 10);
    if (strcmp(*remainder, "") || port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        exit(4);
    }
    free(remainder);
}

/*
*   Reads multiple decks from a file and stores them in the server.
*/
void read_deck_file(char* deckFile, Server* server) {
    char** decks = NULL;
    int deckCount = 0;

    FILE* fp = fopen(deckFile, "r");
    // Cannot open deck file
    if (fp == NULL) {
        fprintf(stderr, "Deck Error\n");
        exit(6);
    }
    // Loop through each line (deck) until there is no more
    while (1) {
        // Add enough memory for one whole deck + null terminator
        char* deck = malloc(sizeof(char) * 105);
        if (fgets(deck, 105, fp) == NULL) {
            free(deck);
            break;
        }
        // Check if there are 52 cards in the deck
        if (strlen(deck) != 104) {
            fprintf(stderr, "Deck Error\n");
            exit(6);
        }
        if (!validate_deck(deck)) {
            fprintf(stderr, "Deck Error\n");
            exit(6);
        }

        decks = realloc(decks, sizeof(decks) + strlen(deck) + 1);
        decks[deckCount] = malloc(strlen(deck) + 1);
        memcpy(decks[deckCount++], deck, strlen(deck) + 1);
        free(deck);
        fgetc(fp);
    }
    server->decks = decks;
    server->deckCount = deckCount;
}

/*
*   Wait for players (clients) to connect and forwards them through to games.
*/
void wait_for_players(int fdServer) {
    int maxFD = fdServer;
    int newFD;

    fd_set baseSet, readSet;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    char hostname[124];

    FD_ZERO(&baseSet);
    FD_SET(fdServer, &baseSet);
    FD_SET(STDIN_FILENO, &baseSet);

    while (1) {
        readSet = baseSet;
        /* Block until any activity happens on the file descriptors */
        if (select(maxFD + 1, &readSet, NULL, NULL, NULL) < 0) {
            //exit(5);
        }
        // If we get here something happened
        for (int fd = maxFD; fd >= 0; fd--) {
            if (FD_ISSET(fd, &readSet)) {
                if (fd == fdServer) {
                    // New connect request
                    fromAddrSize = sizeof(struct sockaddr_in);
                    newFD = accept(fdServer, (struct sockaddr*)&fromAddr,
                            &fromAddrSize);
                    if (newFD < 0) {
                        exit(5);
                    }
                    // Add new file descriptor to our master set
                    FD_SET(newFD, &baseSet);
                    // Update max if necessary
                    if (newFD > maxFD) {
                        maxFD = newFD;
                    }
                    int error = getnameinfo((struct sockaddr*)&fromAddr,
                            fromAddrSize, hostname, 124, NULL, 0, 0);
                    if (error) {
                        exit(5);
                    } else {
                        create_player(newFD);
                    }
                }
            }
        }
    }
}

/*
*   Creates a new player to add to a game.
*/
void create_player(int newFD) {
    Player* player = malloc(sizeof(Player));
    player->writeFD = fdopen(newFD, "w");
    player->readFD = fdopen(newFD, "r");
    player->name = read_socket_message(player->readFD);
    char* gameName = read_socket_message(player->readFD);
    send_welcome_message(player->writeFD);
    add_player_to_game(player, gameName);
}

/*
*   Adds a connected player (client) to a game if it exists, otherwise
*   creates a new game.
*/
void add_player_to_game(Player* player, char* gameName) {
    PendingGame* pg;
    if ((pg = search_game_in_list(gameName, pendingGameList)) != NULL) {
        // The game exists so append player to game
        pg->game->playerCount++;
        player->id = pg->game->playerCount;
        pg->game->players[(pg->game->playerCount) - 1] = *player;
    } else {
        // The game does not exist, so create it and add to list
        Game* game = malloc(sizeof(Game));
        game->players = malloc(sizeof(Player) * 4);
        game->name = gameName;
        game->playerCount = 1;
        game->players[0] = *player;
        player->id = 1;
        if (NULL == pendingGameList) {
            // If this is the first game created, make it the head of the
            // linked list
            pendingGameList = add_to_list(game, NULL);
        } else {
            // Otherwise we don't care so add it on the end
            add_to_list(game, pendingGameList);
        }
    }
    check_for_full_games();
}

/*
*   Iterates through the pending games and checks if any of them have
*   the full 4 players needed to start the game.
*/
void check_for_full_games(void) {
    pthread_t threadId;
    PendingGame* pg;
    // Gets a completed game. Because this is called after every player
    // addition, there can only be one game to possible fill at a time.
    if ((pg = check_if_complete(pendingGameList)) != NULL) {
        Game* game = pg->game;
        pendingGameList = delete_game_from_list(game->name, pendingGameList);
        pthread_create(&threadId, NULL, start_game, (void*)(Game*)game);
        pthread_detach(threadId);
    }
}

/*
*   The main game thread.
*/
void* start_game(void* arg) {
    Game* game = (Game*)arg;
    int currentPlayer;
    // Initialise some stuff
    game->currentDeck = 0;
    game->team1Points = 0;
    game->team2Points = 0;
    // Print the informational team message
    reorder_players(game);
    print_teams(game);

    // Main Game Loop
    while (!check_points(game)) {
        game->team1Wins = 0;
        game->team2Wins = 0;
        // Deal cards
        deal_cards(game);
        // Initiate bidding
        initiate_bidding(game);
        // Select the player who won bidding to start first
        currentPlayer = get_winning_bidder_index(game);
        // Main hand loop
        while (1) {
            if (check_for_empty_hand(game)) {
                break;
            }
            int winner = play_trick(game, currentPlayer);
            currentPlayer = winner;
        }
        set_points(game);
        char pointsMsg[1028];
        sprintf(pointsMsg, "Team 1=%d, Team 2=%d", game->team1Points,
                game->team2Points);
        send_to_players(game, 'M', pointsMsg, -1);
    }
    send_to_players(game, 'O', "", -1);
    for (int i = 0; i < 4; i++) {
        fclose(game->players[i].readFD);
        fclose(game->players[i].writeFD);
    }
    pthread_exit(NULL);
}

/*
*   Check if any team has surpassed 499.
*/
int check_points(Game* game) {
    if (game->contractTeam == 1) {
        if (game->team1Points > 499) {
            send_to_players(game, 'M', "Winner is Team 1", -1);
            return 1;
        } else if (game->team1Points < -499) {
            send_to_players(game, 'M', "Winner is Team 2", -1);
            return 1;
        }
    } else {
        if (game->team2Points > 499) {
            send_to_players(game, 'M', "Winner is Team 2", -1);
            return 1;
        } else if (game->team2Points < -499) {
            send_to_players(game, 'M', "Winner is Team 1", -1);
            return 1;
        }
    }
    return 0;
}

/*
*   Alters the appropriate team's points depending on the game result.
*/
void set_points(Game* game) {
    if (game->contractTeam == 1) {
        if (game->contractGoal > game->team1Wins) {
            game->team1Points -= game->contractPoints;
        } else {
            game->team1Points += game->contractPoints;
        }
    } else if (game->contractTeam == 2) {
        if (game->contractGoal > game->team2Wins) {
            game->team2Points -= game->contractPoints;
        } else {
            game->team2Points += game->contractPoints;
        }
    }
}

/*
* Plays a trick, being one card from each player.
*/
int play_trick(Game* game, int startingPlayer) {
    // Because starting player is an index we want to add 1
    int p = startingPlayer + 1;
    Card** cards = malloc(sizeof(Card*) * 4);
    char leadSuit;
    for (int i = 0; i < 4; i++) {
        if (startingPlayer > 3) {
            p = ((startingPlayer++) % 4);
        } else {
            p = startingPlayer++;
        }
        cards[p] = malloc(sizeof(Card));
        if (i == 0) {
            send_socket_message(game->players[p].writeFD, "L");
            char* response = read_socket_message(game->players[p].readFD);
            check_for_eof(game, response, p);
            read_card_input_from_string(cards[p], response);
            if (!is_valid_card(cards[p])) {
                fprintf(stderr, "server: bad card from client\n");
            } else {
                leadSuit = cards[p]->suit;
            }
        } else {
            char c[2];
            sprintf(c, "%c", leadSuit);
            char* msg = create_message('P', c);
            send_socket_message(game->players[p].writeFD, msg);
            free(msg);
            char* response = read_socket_message(game->players[p].readFD);
            check_for_eof(game, response, p);
            read_card_input_from_string(cards[p], response);
            if (!is_valid_card(cards[p])) {
                fprintf(stderr, "server: bad card from client\n");
            }
        }
        send_socket_message(game->players[p].writeFD, "A");
        game->players[p].cardCount--;
        char play[1024];
        sprintf(play, "%s plays %s", game->players[p].name,
                card_to_string(cards[p]));
        send_to_players(game, 'M', play, p);
    }
    return get_trick_winner(cards, game, leadSuit);
}

/*
*   Checks for eof given by a client.
*/
void check_for_eof(Game* game, char* response, int p) {
    if (!strcmp(response, "EOF")) {
        char errMsg[1028];
        sprintf(errMsg, "%s disconnected early",
                game->players[p].name);
        send_to_players(game, 'M', errMsg, -1);
    }
}

/*
*   Check if all the players have empty hands.
*/
int check_for_empty_hand(Game* game) {
    if (game->players[0].cardCount == 0 &&
            game->players[1].cardCount == 0 &&
            game->players[2].cardCount == 0 &&
            game->players[3].cardCount == 0) {
        return 1;
    } else {
        return 0;
    }
}

/*
*   Returns the player index of the winner of the last trick.
*/
int get_trick_winner(Card** cards, Game* game, char leadSuit) {
    int currentWinner = 0;
    Card* currentCard = NULL;
    for (int i = 0; i < 4; i++) {
        if (cards[i]->suit == leadSuit) {
            if (is_higher(currentCard, cards[i])) {
                if (currentCard == NULL) {
                    currentCard = malloc(sizeof(Card));
                }
                currentCard = cards[i];
                currentWinner = i;
            }
        }
    }
    for (int i = 0; i < 4; i++) {
        if (cards[i]->suit == game->trumps) {
            if ((currentCard->suit == leadSuit) &&
                    (leadSuit != game->trumps)) {
                currentCard = cards[i];
                currentWinner = i;
            } else if (is_higher(currentCard, cards[i])) {
                currentCard = cards[i];
                currentWinner = i;
            }
        }
    }
    free(currentCard);
    char msg[1028];
    sprintf(msg, "%s won", game->players[currentWinner].name);
    send_to_players(game, 'M', msg, -1);

    if (currentWinner == 0 || currentWinner == 2) {
        game->team1Wins++;
    } else {
        game->team2Wins++;
    }

    return currentWinner;
}

/*
*   Reorders players in lexographical order.
*/
void reorder_players(Game* game) {
    char* players[4];
    int playerOrders[4];
    int k = 0;
    players[0] = game->players[0].name;
    players[1] = game->players[1].name;
    players[2] = game->players[2].name;
    players[3] = game->players[3].name;
    qsort(players, 4, sizeof(players[0]), &compare_players);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (players[i] == game->players[j].name) {
                playerOrders[k++] = j;
            }
        }
    }
    Player* tempPlayers = malloc(sizeof(Player) * 4);
    tempPlayers[0] = game->players[playerOrders[0]];
    tempPlayers[1] = game->players[playerOrders[1]];
    tempPlayers[2] = game->players[playerOrders[2]];
    tempPlayers[3] = game->players[playerOrders[3]];
    game->players[0] = tempPlayers[0];
    game->players[1] = tempPlayers[1];
    game->players[2] = tempPlayers[2];
    game->players[3] = tempPlayers[3];
}

/*
*   Prints the team names to all players.
*/
void print_teams(Game* game) {
    char team1[128];
    char team2[128];
    sprintf(team1, "Team1: %s, %s", game->players[0].name,
            game->players[2].name);
    sprintf(team2, "Team2: %s, %s", game->players[1].name,
            game->players[3].name);
    char* msg1 = create_message('M', team1);
    char* msg2 = create_message('M', team2);

    for (int i = 0; i < 4; i++) {
        send_socket_message(game->players[i].writeFD, msg1);
        send_socket_message(game->players[i].writeFD, msg2);
    }
}

/*
*   Sends a welcome message to all players.
*/
void send_welcome_message(FILE* fd) {
    char* welcome = create_message('M', server->greeting);
    send_socket_message(fd, welcome);
    free(welcome);
}

/*
*   Deals out a single hand to each player in the game.
*/
void deal_cards(Game* game) {
    char* deck = server->decks[game->currentDeck];
    char* p1 = malloc(sizeof(char) * 28);
    char* p2 = malloc(sizeof(char) * 28);
    char* p3 = malloc(sizeof(char) * 28);
    char* p4 = malloc(sizeof(char) * 28);
    int a = 0;
    int b = 0;
    int c = 0;
    int d = 0;

    for(int i = 0; i < 104; i++) {
        p1[a++] = deck[i++];
        p1[a++] = deck[i++];
        p2[b++] = deck[i++];
        p2[b++] = deck[i++];
        p3[c++] = deck[i++];
        p3[c++] = deck[i++];
        p4[d++] = deck[i++];
        p4[d++] = deck[i];
    }

    send_socket_message(game->players[0].writeFD,
            create_message('H', p1));
    send_socket_message(game->players[1].writeFD,
            create_message('H', p2));
    send_socket_message(game->players[2].writeFD,
            create_message('H', p3));
    send_socket_message(game->players[3].writeFD,
            create_message('H', p4));

    store_hand(p1, &game->players[0]);
    store_hand(p2, &game->players[1]);
    store_hand(p3, &game->players[2]);
    store_hand(p4, &game->players[3]);

    free(p1);
    free(p2);
    free(p3);
    free(p4);

    increment_game_deck(game);
}

/*
*   Increments the current game deck so that the next deck in the deckfile is
*   used.
*/
void increment_game_deck(Game* game) {
    game->currentDeck = (game->currentDeck + 1) % server->deckCount;
}

/*
*   Initiaes the bidding phase of the game and sets the trumps.
*/
void initiate_bidding(Game* game) {
    int notMax = 1;
    int go = 1;
    int first = 1;
    Card* currentBid = malloc(sizeof(Card));
    Card* sentBid = malloc(sizeof(Card));
    for (int i = 0; i < 4; i++) {
        game->players[i].eligible = 1;
    }
    while (go && notMax) {
        for (int i = 0; i < 4; i++) {
            go = check_for_eligibility(game);

            if (game->players[i].eligible && go) {
                get_players_bid(game, currentBid, sentBid, i, first);
                first = 0;
            }

            if (currentBid->suit == 'H' &&
                    currentBid->rank == '9') {
                for (int j = 0; j < 4; j++) {
                    if (j != i) {
                        game->players[j].eligible = 0;
                    }
                    game->players[i].eligible = 1;
                }
                notMax = 0;
                break;
            }
        }
    }
    game->trumps = currentBid->suit;
    char goal[2];
    goal[0] = currentBid->rank;
    goal[1] = '\0';
    game->contractGoal = atoi(goal);
    game->contractPoints = calculate_contract_points(currentBid);
    int team = get_winning_bidder_index(game);
    if (team == 0 || team == 2) {
        game->contractTeam = 1;
    } else {
        game->contractTeam = 2;
    }
    char* msg = card_to_string(currentBid);
    send_to_players(game, 'T', msg, -1);
    free(msg);
    free(currentBid);
    free(sentBid);
}

/*
*   Gets an individual player's bid and parses it.
*/
void get_players_bid(Game* game, Card* currentBid, Card* sentBid, int i,
        int first) {
    char buff[1028];
    if (first) {
        first = 0;
        // First bid
        send_socket_message(game->players[i].writeFD, "B");
        char* response = read_socket_message(
                game->players[i].readFD);
        check_for_eof(game, response, i);
        read_card_input_from_string(sentBid, response);
        if (!is_valid_bid(sentBid)) {
            if (sentBid->rank == 'P' && sentBid->suit == 'P') {
                game->players[i].eligible = 0;
                sprintf(buff, "%s passes",
                        game->players[i].name);
                send_to_players(game, 'M', buff, i);
            } else {
                fprintf(stderr, "server: bad bid\n");
            }
        } else {
            memcpy(currentBid, sentBid, sizeof(Card));
            sprintf(buff, "%s bids %s", game->players[i].name,
                    card_to_string(currentBid));
            send_to_players(game, 'M', buff, i);
        }
    } else {
        send_socket_message(game->players[i].writeFD,
                create_message('B', card_to_string(currentBid)));
        char* response = read_socket_message(
                game->players[i].readFD);
        check_for_eof(game, response, i);
        read_card_input_from_string(sentBid, response);
        if (!is_valid_bid(sentBid) ||
                !is_higher_bid(currentBid, sentBid)) {
            if (sentBid->rank == 'P' && sentBid->suit == 'P') {
                game->players[i].eligible = 0;
                sprintf(buff, "%s passes",
                        game->players[i].name);
                send_to_players(game, 'M', buff, i);
            } else {
                fprintf(stderr, "server: bad bid\n");
            }
        } else {
            memcpy(currentBid, sentBid, sizeof(Card));
            sprintf(buff, "%s bids %s", game->players[i].name,
                    card_to_string(currentBid));
            send_to_players(game, 'M', buff, i);
        }
    }
}

/*
*   Checks if there is more than one player eligible for bidding.
*/
int check_for_eligibility(Game* game) {
    int p = 0;
    for (int i = 0; i < 4; i++) {
        if (game->players[i].eligible) {
            p++;
        }
    }
    if (p == 1) {
        return 0;
    } else {
        return 1;
    }
}

/*
*   Returns the player index of the winning bidder.
*/
int get_winning_bidder_index(Game* game) {
    for (int i = 0; i < 4; i++) {
        if (game->players[i].eligible) {
            return i;
        }
    }
    return -1;
}

/*
*   Calculates the amount of points a winning bid is worth.
*/
int calculate_contract_points(Card* bid) {
    int points;
    int rank;
    char* buff = malloc(2);
    buff[0] = bid->rank;
    buff[1] = '\0';
    rank = atoi(buff);
    switch (bid->suit) {
        case 'S':
            points = (rank - 4) * 50 + 20;
            break;
        case 'C':
            points = (rank - 4) * 50 + 30;
            break;
        case 'D':
            points = (rank - 4) * 50 + 40;
            break;
        case 'H':
            points = (rank - 4) * 50 + 50;
            break;
    }
    free(buff);
    return points;
}

/*
*   Sends a structured message to all players, excluding the player index
*   given as the exclude parameter.
*/
void send_to_players(Game* game, char type, char* message, int exclude) {
    for (int i = 0; i < 4; i++) {
        if (i != exclude) {
            char* msg = create_message(type, message);
            send_socket_message(game->players[i].writeFD, msg);
            free(msg);
        }
    }
}
