#include "../include/server.h"
#include "../include/packet.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static Server SERVER = {.game = {.players = 0, .player_positions = {0}},
                        .server_mutex = PTHREAD_MUTEX_INITIALIZER};

static void *server_logic(void *args) {
  int server_fd = ((int *)args)[0];

  while (1) {
    const char *message = "Hello from server\n";
    // send(client_fd, message, strlen(message), 0);
    sleep(1); // Send every 1 second
  }
  return NULL;
}

static void handle_packet(Packet packet) {
  switch (packet.type) {
  case PACKET_BIDIR_SET_POS: {
    pthread_mutex_lock(&SERVER.server_mutex);
    {
      int player_id = packet.var.bidir_set_pos.player_id;
      Vec2i pos = packet.var.bidir_set_pos.pos;
      SERVER.game.player_positions[player_id] = pos;
      for (int i = 0; i < SERVER.game.players; i++) {
        if (i != player_id) {
      printf("Sent pos x: %d, y: %d to client\n", pos.x, pos.y);
          packet_send(
              SERVER.client_addresses[i],
              (Packet){
                  .type = PACKET_BIDIR_SET_POS,
                  .var = {
                      .bidir_set_pos = {
                          .player_id = player_id,
                          .pos = SERVER.game.player_positions[player_id]}}});
        }
      }
    }
    pthread_mutex_unlock(&SERVER.server_mutex);
    break;
  }
  default: {
    break;
  }
  }
}

static void *server_listener(void *args) {
  int server_addr = ((int *)args)[0];

  while (1) {
    if (SERVER.game.players > 0) {
      for (int i = 0; i < SERVER.game.players; i++) {
        Packet packet = packet_receive(SERVER.client_addresses[i]);
        handle_packet(packet);

        if (packet.type == PACKET_ERROR) {
          continue;
        }

        printf("Received packet: %d from: %d out of %d players\n", packet.type,
               SERVER.client_addresses[i], SERVER.game.players);
      }
    }

    // TODO: Fix Disconnect
    // if (bytes <= 0) {
    //  printf("Disconnected.\n");
    //  break;
    //}
  }
  return NULL;
}

static void *server_player_join_listener(void *args) {
  int server_addr = ((int *)args)[0];
  bool server_full = false;

  while (1) {
    printf("Waiting to accept new player\n");
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd =
        accept(server_addr, (struct sockaddr *)&client_addr, &client_len);
    printf("NEW PLAYER JOINED\n");

    if (client_fd < 0) {
      perror("accept failed");
      close(server_addr);
      return NULL;
    }
    printf("client id valid\n");

    pthread_mutex_lock(&SERVER.server_mutex);
    {
      int players = SERVER.game.players;
      int player_id = players;
      SERVER.game.players++;
      printf("players: %d\n", players);
      SERVER.client_addresses[player_id] = client_fd;

      packet_send(
          client_fd,
          (Packet){.type = PACKET_S2C_PLAYER_JOIN,
                   .var = {.s2c_player_join = {.player_id = player_id}}});
      packet_send(client_fd,
                  (Packet){.type = PACKET_S2C_GAME_SYNC,
                           .var = {.s2c_game_sync = {.game = SERVER.game}}});
      for (int i = 0; i < players; i++) {
        packet_send(SERVER.client_addresses[i],
                    (Packet){.type = PACKET_S2C_NEW_PLAYER_JOINED,
                             .var = {.s2c_new_player_joined = {
                                         .player_id = player_id}}});
      }

      printf("Client connected!\n");
    }
    pthread_mutex_unlock(&SERVER.server_mutex);
  }
  return NULL;
}

static int server_start() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket failed");
    return -1;
  }

  SERVER.server_address = server_fd;

  // Allow address reuse
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_addr.s_addr = INADDR_ANY,
                             .sin_port = htons(PORT)};

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind failed");
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, 10) < 0) {
    perror("listen failed");
    close(server_fd);
    return -1;
  }

  printf("Server listening on port %d\n", PORT);
  return server_fd;
}

void server_run() {
  int server_addr = server_start();
  if (server_addr == -1) {
    perror("Failed to launch server");
    return;
  }

  pthread_t logic_thread;
  pthread_t networking_listener_thread;
  pthread_t networking_player_join_thread;

  int args[] = {server_addr};
  if (pthread_create(&networking_player_join_thread, NULL,
                     server_player_join_listener, args)) {
    perror("Failed to create networking player join thread");
    exit(1);
  }

  if (pthread_create(&networking_listener_thread, NULL, server_listener,
                     args)) {
    perror("Failed to create networking listener thread");
    exit(1);
  }

  if (pthread_create(&logic_thread, NULL, server_logic, args)) {
    perror("Failed to create logic thread");
    exit(1);
  }

  pthread_join(networking_player_join_thread, NULL);
  pthread_join(networking_listener_thread, NULL);
  pthread_join(logic_thread, NULL);
  printf("Game has finished\n");

  close(server_addr);
}
