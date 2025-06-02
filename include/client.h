#pragma once

#include "game.h"
#include "pthread.h"
#include <bits/pthreadtypes.h>

typedef struct {
    int player_id;
    int server_address;
    Game game;
    pthread_mutex_t client_mutex;
} Client;

void client_run();
