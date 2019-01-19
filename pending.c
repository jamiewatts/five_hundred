#include "pending.h"

/*
*   Creates a new linked list of pending games.
*/
PendingGame* create_list(Game* game) {
    PendingGame* gameList = (PendingGame*)malloc(sizeof(PendingGame));
    if (gameList == NULL) {
        fprintf(stderr, "Node creation failed\n");
        return NULL;
    }
    gameList->game = game;
    gameList->next = NULL;

    return gameList;
}

/*
*   Adds a new game to the pending list.
*/
PendingGame* add_to_list(Game* game, PendingGame* head) {
    if (head == NULL) {
        return create_list(game);
    }
    PendingGame* lastGame = get_last_game(head);
    PendingGame* gameList = (PendingGame*)malloc(sizeof(PendingGame));
    gameList->game = game;
    gameList->next = NULL;
    lastGame->next = gameList;
    return gameList;
}

/*
*   Returns the last game in the linked list of pending games.
*/
PendingGame* get_last_game(PendingGame* head) {
    PendingGame* curr = head;
    PendingGame* temp = NULL;
    while (curr->next != NULL) {
        temp = curr;
        curr = temp->next;
    }
    return curr;
}

/*
*   Searches for a given game name in the linked list of pending games.
*/
PendingGame* search_game_in_list(char* name, PendingGame* head) {
    if (head == NULL) {
        return NULL;
    }
    PendingGame* temp = NULL;
    PendingGame* curr = head;
    while (1) {
        if (!strcmp(curr->game->name, name)) {
            return curr;
        }
        if (curr->next == NULL) {
            break;
        }
        temp = curr;
        curr = temp->next;
    }
    return NULL;
}

/*
*   Delete the pending game with given game from the list of pending games
*/
PendingGame* delete_game_from_list(char* name, PendingGame* head) {
    PendingGame* temp = NULL;
    PendingGame* curr = head;
    while(1) {
        if (!strcmp(curr->game->name, name)) {
            if (!strcmp(curr->game->name, head->game->name)) {
                return curr->next;
            } else {
                temp->next = curr->next;
                return head;
            }
        } else if (curr->next == NULL) {
            return NULL;
        }
        temp = curr;
        curr = temp->next;
    }
}

/*
*   Check if any of the games in the list are complete and ready to start.
*/
PendingGame* check_if_complete(PendingGame* head) {
    if (head == NULL) {
        return NULL;
    }
    PendingGame* temp = NULL;
    PendingGame* curr = head;
    while (1) {
        if (curr->game->playerCount == 4) {
            return curr;
        }
        if (curr->next == NULL) {
            break;
        }
        temp = curr;
        curr = temp->next;
    }
    return NULL;
}
