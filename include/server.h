#pragma once

#include "../include/sockets.h"
#include "game.h"
#include <pthread.h>

typedef struct {
    int server_address;
    int client_addresses[MAX_PLAYERS];
    Game game;
    pthread_mutex_t game_server_mutex;
} GameServer;

void server_run();
