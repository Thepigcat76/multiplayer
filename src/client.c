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
                   CLIENT.game.player_positions[i].y, 20, (Color){.r = 100 * i + 50, .g = 255, .b = 200 * i, .a = 255});
                   DrawText(TextFormat("x: %d, y: %d", CLIENT.game.player_positions[i].x, CLIENT.game.player_positions[i].y),  CLIENT.game.player_positions[i].x,  CLIENT.game.player_positions[i].y, 10, BLACK);
      }

      if (IsKeyDown(KEY_W)) {
        CLIENT.game.player_positions[player_id].y -= 3;
        packet_send(
            sfd,
            (Packet){
                .type = PACKET_BIDIR_SET_POS,
                .var = {.bidir_set_pos = {
                            .player_id = player_id,
                            .pos = CLIENT.game.player_positions[player_id]}}});
      }
    }
    if (IsKeyDown(KEY_S)) {
      CLIENT.game.player_positions[player_id].y += 3;
      packet_send(
          sfd,
          (Packet){
              .type = PACKET_BIDIR_SET_POS,
              .var = {.bidir_set_pos = {
                          .player_id = player_id,
                          .pos = CLIENT.game.player_positions[player_id]}}});
    }
    if (IsKeyDown(KEY_A)) {
      CLIENT.game.player_positions[player_id].x -= 3;
      packet_send(
          sfd,
          (Packet){
              .type = PACKET_BIDIR_SET_POS,
              .var = {.bidir_set_pos = {
                          .player_id = player_id,
                          .pos = CLIENT.game.player_positions[player_id]}}});
    }
    if (IsKeyDown(KEY_D)) {
      CLIENT.game.player_positions[player_id].x += 3;
      packet_send(
          sfd,
          (Packet){
              .type = PACKET_BIDIR_SET_POS,
              .var = {.bidir_set_pos = {
                          .player_id = player_id,
                          .pos = CLIENT.game.player_positions[player_id]}}});
    }

    EndDrawing();
    pthread_mutex_unlock(&CLIENT.client_mutex);
  }

  CloseWindow();
  return NULL;
}

static void handle_packet(Packet packet) {
  switch (packet.type) {
  case PACKET_ERROR: {
    fprintf(stderr, "Error packet\n");
    break;
  }
  case PACKET_S2C_PLAYER_JOIN: {
    pthread_mutex_lock(&CLIENT.client_mutex);
    {
      CLIENT.player_id = packet.var.s2c_player_join.player_id;
      CLIENT.game.player_positions[CLIENT.player_id] = (Vec2i){.x = 0, .y = 0};
    }
    pthread_mutex_unlock(&CLIENT.client_mutex);
    break;
  }
  case PACKET_S2C_NEW_PLAYER_JOINED: {
    pthread_mutex_lock(&CLIENT.client_mutex);
    {
      int new_id = packet.var.s2c_new_player_joined.player_id;
      TraceLog(LOG_INFO, "New player joined at {x=%d,y=%d}, total players: %d", CLIENT.game.player_positions[new_id].x, CLIENT.game.player_positions[new_id].y, CLIENT.game.players + 1);
      CLIENT.game.player_positions[new_id] = (Vec2i){.x = 0, .y = 0};
      CLIENT.game.players++;
    }
    pthread_mutex_unlock(&CLIENT.client_mutex);
    break;
  }
  case PACKET_S2C_GAME_SYNC: {
    pthread_mutex_lock(&CLIENT.client_mutex);
    {
      CLIENT.game = packet.var.s2c_game_sync.game;
    }
    pthread_mutex_unlock(&CLIENT.client_mutex);
    break;
  }
  case PACKET_BIDIR_SET_POS: {
    pthread_mutex_lock(&CLIENT.client_mutex);
    {
      int player_id = packet.var.bidir_set_pos.player_id;
      if (player_id != CLIENT.player_id) {
        Vec2i pos = packet.var.bidir_set_pos.pos;
        CLIENT.game.player_positions[player_id] = pos;
        TraceLog(LOG_INFO, "POSITION SET {x=%d,y=%d}", CLIENT.game.player_positions[player_id].x, CLIENT.game.player_positions[player_id].y);
      }
    }
    pthread_mutex_unlock(&CLIENT.client_mutex);
    break;
  }
  }
}

void *run_client(void *args) {
  int server_addr = ((int *)args)[0];

  while (1) {
    TraceLog(LOG_INFO, "Awaiting packets");
    Packet packet = packet_receive(server_addr);
    handle_packet(packet);

    TraceLog(LOG_INFO, "Received packet: %d", packet.type);
  }
  close(server_addr);
  return NULL;
}

static int client_join_server() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket failed");
    return -1;
  }

  struct sockaddr_in server_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(PORT),
  };

  if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
    perror("invalid address");
    close(sock);
    return 1;
  }

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect failed");
    close(sock);
    return -1;
  }

  printf("Connected to server!\n");
  return sock;
}

void client_run() {
  int server_addr = client_join_server();

  if (server_addr == -1) {
    perror("Failed to connect to server");
    exit(1);
  }

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
