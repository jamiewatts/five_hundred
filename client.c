/*
* Jamie Watts (43177039)
*
* client.c
* Usage: client499 name game port [host]
* A client to connect to the 499 game server.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "networking.h"

char* validate_arguments(int, char**);
struct in_addr* convert_hostname(char*);
void read_message(char*, Player*);
void send_server_information(FILE*, char*, char*);
char get_message_type(char*);
void play_game(Player*);
void play_tricks(Player*);

// Global instance of the current player
Player* player;

int main(int argc, char *argv[]) {
    struct in_addr* ipAddress;
    int port;
    int fd;
    // Validate port range and arguments
    char* hostname = validate_arguments(argc, argv);
    player = malloc(sizeof(Player));
    ipAddress = convert_hostname(hostname);
    port = atoi(argv[3]);

    player->name = argv[1];

    fd = connect_to(ipAddress, port);

    player->readFD = fdopen(fd, "r");
    player->writeFD = fdopen(fd, "w");

    send_server_information(player->writeFD, player->name, argv[2]);

    // Read in three informational messages
    for (int i = 0; i < 3; i++) {
        char* msg = read_socket_message(player->readFD);
        char type = get_message_type(msg);
        if (type != 'M') {
            fprintf(stderr, "Protocol Error.\n");
            exit(6);
        }
        free(msg);
    }

    play_game(player);

    return 0;
}

/*
*   Main gameplay loop.
*/
void play_game(Player* player) {
    while (1) {
        // Store hand
        while (1) {
            char* msg = read_socket_message(player->readFD);
            char type = get_message_type(msg);
            if (type != 'H' && type != 'M') {
                fprintf(stderr, "Protocol Error.\n");
                exit(6);
            } else if (type == 'H') {
                memmove(msg, msg + 1, strlen(msg));
                store_hand(msg, player);
                print_cards(player);
                free(msg);
                break;
            }
            free(msg);
        }
        while (1) {
            char* msg = read_socket_message(player->readFD);
            char type = get_message_type(msg);
            if (type == 'T') {
                free(msg);
                break;
            } else if (type != 'B' && type != 'M') {
                fprintf(stderr, "Protocol Error.\n");
                exit(6);
            } else if (type == 'M') {
                // Do nothing
            } else {
                memmove(msg, msg + 1, strlen(msg));
                ask_for_bid(msg, player);
            }
            free(msg);
        }

        play_tricks(player);
    }
}

/*
*   Play the tricks in a single hand.
*/
void play_tricks(Player* player) {
    for (int i = 0; i < 13; i++) {
        char* msg = read_socket_message(player->readFD);
        char type = get_message_type(msg);
        if (type != 'L' && type != 'P' && type != 'M') {
            fprintf(stderr, "Protocol Error.\n");
            exit(6);
        } else if (type == 'M') {
            i--;
        } else {
            memmove(msg, msg + 1, strlen(msg));
            ask_for_play(msg, player);
            free(msg);
            while (1) {
                msg = read_socket_message(player->readFD);
                type = get_message_type(msg);
                memmove(msg, msg + 1, strlen(msg));
                if (type == 'M') {

                } else if (type == 'A') {
                    remove_card_from_hand(player, player->lastPlay);
                    free(msg);
                    break;
                } else {
                    fprintf(stderr, "Protocol Error.\n");
                    exit(6);
                }
            }
        }
    }
}

/*
*   Validate the input arguments given to the client.
*   Returns the hostname of the server
*/
char* validate_arguments(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        // Throw usage error (exit(1))
        fprintf(stderr, "Usage: client499 name game port [host]\n");
        exit(1);
    }
    char** remainder = malloc(sizeof(char**));
    int port = strtol(argv[3], remainder, 10);
    if (strcmp(*remainder, "") || port < 1 || port > 65535 ||
            !strcmp(argv[1], "") || !strcmp(argv[2], "")) {
        // Throw invalid arguments error (exit(4))
        fprintf(stderr, "Invalid Arguments.\n");
        exit(4);
    }
    free(remainder);
    char* hostname;
    if (argc == 4) {
        hostname = malloc(sizeof("localhost"));
        hostname = "localhost";
    } else {
        hostname = malloc(sizeof(argv[4]));
        hostname = argv[4];
    }
    return hostname;
}

/*
 * Send the initial name and game information to the server.
 */
void send_server_information(FILE* fd, char* name, char* game) {
    send_socket_message(fd, name);
    send_socket_message(fd, game);
}
/*
*   Convert a hostname string to an IP address.
*/
struct in_addr* convert_hostname(char* hostname) {
    struct in_addr* ipAddress = hostname_to_ip(hostname);
    if (!ipAddress) {
        fprintf(stderr, "Bad Server.\n");
        exit(2);
    }
    return ipAddress;
}

/*
*   Gets the message type sent from the server.
*/
char get_message_type(char* message) {
    char messageType = message[0];
    if (messageType == 'M') {
        memmove(message, message + 1, strlen(message));
        print_message(message);
        if (ends_with(message, "disconnected early")) {
            exit(0);
        }
        return messageType;
    } else if (messageType == 'O') {
        exit(0);
    } else {
        //if (!strcmp(message, "EOF")) {
        //    fprintf(stderr, "Protocol Error.\n");
        //    exit(6);
        //}
        return messageType;
    }

}
