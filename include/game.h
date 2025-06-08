#pragma once

#define MAX_PLAYERS 10

#define IP_ADDRESS "127.0.0.1"
#define PORT 12345

typedef struct {
    int x;
    int y;
} Vec2i;

typedef struct {
    int players;
    Vec2i player_positions[MAX_PLAYERS];
} Game;
