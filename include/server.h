#pragma once

#include "game.h"
#include "pthread.h"
#include <bits/pthreadtypes.h>

typedef struct {
    int server_address;
    int client_addresses[MAX_PLAYERS];
    Game game;
    pthread_mutex_t server_mutex;
} Server;

void server_run();
