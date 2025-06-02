#pragma once

#include "game.h"

typedef struct{
    enum {
        // Returned if decoding/encoding failed
        PACKET_ERROR,
        // Send to the player that just joined
        PACKET_S2C_PLAYER_JOIN,
        // Send to all players
        PACKET_S2C_NEW_PLAYER_JOINED,
        // Send to n players/server
        PACKET_BIDIR_SET_POS,
        // Send to n players
        PACKET_S2C_GAME_SYNC,
    } type;
    union {
        struct {
            int player_id;
        } s2c_player_join;
        struct {
            int player_id;
        } s2c_new_player_joined;
        struct {
            int player_id;
            Vec2i pos;
        } bidir_set_pos;
        struct {
            Game game;
        } s2c_game_sync;
    } var;
} Packet;

void packet_send(int addr, Packet packet);

Packet packet_receive(int addr);
