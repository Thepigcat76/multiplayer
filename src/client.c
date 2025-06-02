#include "../include/client.h"
#include "../include/packet.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "raylib.h"

static Client CLIENT = {.game = {.players = 0, .player_positions = {0}},
                        .client_mutex = PTHREAD_MUTEX_INITIALIZER};

void *run_game(void *args) {
  int sfd = ((int *)args)[0];

  InitWindow(800, 600, "Hello");
  SetTargetFPS(60);

  int last_sent_time = -3; // So it sends immediately when time reaches 0

  while (!WindowShouldClose()) {
    pthread_mutex_lock(&CLIENT.client_mutex);
    int player_id = CLIENT.player_id;
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);
      for (int i = 0; i < CLIENT.game.players; i++) {
        DrawCircle(CLIENT.game.player_positions[i].x,
                   CLIENT.game.player_positions[i].y, 20, RED);
      }

      if (IsKeyDown(KEY_W))
        CLIENT.game.player_positions[player_id].y -= 3;
      if (IsKeyDown(KEY_S))
        CLIENT.game.player_positions[player_id].y += 3;
      if (IsKeyDown(KEY_A))
        CLIENT.game.player_positions[player_id].x -= 3;
      if (IsKeyDown(KEY_D))
        CLIENT.game.player_positions[player_id].x += 3;

      int current_time = (int)GetTime();
      if (current_time - last_sent_time >= 3) {
        packet_send(
            sfd,
            (Packet){
                .type = PACKET_C2S_SET_POS,
                .var = {.c2s_set_pos = {
                            .player_id = player_id,
                            .pos = CLIENT.game.player_positions[player_id]}}});
        last_sent_time = current_time;
      }
    }
    EndDrawing();
    pthread_mutex_unlock(&CLIENT.client_mutex);
  }

  CloseWindow();
  return NULL;
}

void *run_client(void *args) {
  int server_addr = ((int *)args)[0];

  while (1) {
    Packet packet = packet_receive(server_addr);
    if (packet.type == PACKET_ERROR) {
      printf("Disconnected.\n");
      break;
    }

    TraceLog(LOG_DEBUG, "Received packet: %d", packet.type);

    switch (packet.type) {
    case PACKET_ERROR: {
      break;
    }
    case PACKET_S2C_PLAYER_JOIN: {
      pthread_mutex_lock(&CLIENT.client_mutex);
      CLIENT.player_id = packet.var.s2c_player_join.player_id;
      pthread_mutex_unlock(&CLIENT.client_mutex);
      break;
    }
    case PACKET_S2C_NEW_PLAYER_JOINED: {
      pthread_mutex_lock(&CLIENT.client_mutex);
      int players = CLIENT.game.players++;
      for (int i = 0; i < players; i++) {
        CLIENT.game.player_positions[i].x =
            packet.var.s2c_new_player_joined.player_id;
      }
      pthread_mutex_unlock(&CLIENT.client_mutex);
      break;
    }
    case PACKET_S2C_GAME_SYNC: {
      pthread_mutex_lock(&CLIENT.client_mutex);
      CLIENT.game = packet.var.s2c_game_sync.game;
      pthread_mutex_unlock(&CLIENT.client_mutex);
      break;
    }
    case PACKET_C2S_SET_POS: {
      perror("Received client packet on client");
      break;
    }
    }
  }

  close(server_addr);
  return NULL;
}

static int client_join_server() {
  int client_address = socket(AF_INET, SOCK_STREAM, 0);
  if (client_address < 0) {
    perror("socket failed");
    return -1;
  }

  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  if (connect(client_address, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    perror("connect failed");
    close(client_address);
    return -1;
  }

  printf("Connected to server!\n");
  return client_address;
}

void client_run() {
  int server_addr = client_join_server();

  CLIENT.server_address = server_addr;

  pthread_t game_thread;
  pthread_t networking_thread;

  int args[] = {server_addr};
  if (pthread_create(&game_thread, NULL, run_game, args)) {
    perror("Failed to create game thread");
    exit(1);
  }

  if (pthread_create(&networking_thread, NULL, run_client, args)) {
    perror("Failed to create networking thread");
    exit(1);
  }

  pthread_join(game_thread, NULL);
  pthread_join(networking_thread, NULL);
  printf("Game has finished\n");
}
