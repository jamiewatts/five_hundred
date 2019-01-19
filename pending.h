#include <stdlib.h>
#include <stdio.h>
#include "game.h"

struct GameList {
    Game* game;
    struct GameList* next;
};

typedef struct GameList PendingGame;

PendingGame* create_list(Game*);
PendingGame* add_to_list(Game*, PendingGame*);
PendingGame* get_last_game(PendingGame*);
PendingGame* search_game_in_list(char*, PendingGame*);
PendingGame* delete_game_from_list(char*, PendingGame*);
PendingGame* check_if_complete(PendingGame*);
