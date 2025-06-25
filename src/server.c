#include "../include/server.h"
#include "../include/packet.h"
#include "../include/sockets.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static GameServer SERVER = {
    .game = {.players = 0, .player_positions = {0}},
    .game_server_mutex = PTHREAD_MUTEX_INITIALIZER,
};

static void *server_logic(void *args) {
  int server_fd = ((int *)args)[0];

  while (1) {
  }
  return NULL;
}

static void handle_packet(Packet packet) {
  switch (packet.type) {
  case PACKET_BIDIR_SET_POS: {
    pthread_mutex_lock(&SERVER.game_server_mutex);
    {
      int player_id = packet.var.bidir_set_pos.player_id;
      Vec2i pos = packet.var.bidir_set_pos.pos;
      SERVER.game.player_positions[player_id] = pos;
      printf("Players: %d\n", SERVER.game.players);
      for (int i = 0; i < SERVER.game.players; i++) {
        printf("SENT SET POS\n");
        packet_send(
            SERVER.client_addresses[i],
            (Packet){
                .type = PACKET_BIDIR_SET_POS,
                .var = {.bidir_set_pos =
                            {.player_id = player_id,
                             .pos = SERVER.game.player_positions[player_id]}}},
            false);
      }
    }
    pthread_mutex_unlock(&SERVER.game_server_mutex);
    break;
  }
  default: {
    break;
  }
  }
}

#define MAX_CLIENTS 32 // Adjust as needed

static void handle_connection(int32_t client_addr) {
  Packet packet = packet_receive(client_addr, false);
  if (packet.type == PACKET_ERROR) {
    fprintf(stderr, "Error or disconnect on fd %d\n", client_addr);
    // Optionally remove player or close connection here
    return;
  }

  handle_packet(packet);

  printf("Received packet: %d from: %d\n", packet.type, client_addr);
}

#define _INVALID_SOCKET (int)(~0)

static void *server_listener(void *args) {
  struct pollfd fds[MAX_CLIENTS];

  while (1) {
    pthread_mutex_lock(&SERVER.game_server_mutex);
    size_t client_addresses_amount = SERVER.game.players;

    for (int i = 0; i < client_addresses_amount; i++) {
      int s = SERVER.client_addresses[i];
      if (s == _INVALID_SOCKET || s == 0) {
        fprintf(stderr,
                "[DEBUG] Skipping invalid socket at index %d (fd=%lld)\n", i,
                (long long)s);
        fds[i].fd = _INVALID_SOCKET;
        fds[i].events = 0;
      } else {
        fds[i].fd = s;
        fds[i].events = POLLRDNORM;
      }
    }
    pthread_mutex_unlock(&SERVER.game_server_mutex);

    int poll_result = sockets_server_poll_clients(fds, client_addresses_amount, 100);

    sockets_server_handle_poll(poll_result);

    for (int i = 0; i < client_addresses_amount; i++) {
      if (fds[i].revents & POLLIN) {
        int client_fd = fds[i].fd;

        handle_connection(client_fd);
      }
      if (fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        fprintf(stderr, "Client %d disconnected or error\n", fds[i].fd);
        sockets_close(fds[i].fd);
        // You may want to mark the player slot as disconnected
      }
    }
  }
  return NULL;
}

static void *server_player_join_listener(void *args) {
  int server_addr = ((int *)args)[0];
  bool server_full = false;

  while (1) {
    printf("Waiting to accept new player\n");
    long client_fd = sockets_server_accept_client(server_addr);
    printf("client id valid\n");

    pthread_mutex_lock(&SERVER.game_server_mutex);
    {
      int player_id = SERVER.game.players++;
      printf("players: %d\n", SERVER.game.players);
      SERVER.client_addresses[player_id] = client_fd;

      packet_send(
          client_fd,
          (Packet){.type = PACKET_S2C_PLAYER_JOIN,
                   .var = {.s2c_player_join = {.player_id = player_id}}},
          false);
      packet_send(client_fd,
                  (Packet){.type = PACKET_S2C_GAME_SYNC,
                           .var = {.s2c_game_sync = {.game = SERVER.game}}},
                  false);
      for (int i = 0; i < SERVER.game.players; i++) {
        packet_send(SERVER.client_addresses[i],
                    (Packet){.type = PACKET_S2C_NEW_PLAYER_JOINED,
                             .var = {.s2c_new_player_joined = {.player_id =
                                                                   player_id}}},
                    false);
      }

      printf("Client connected!\n");
    }
    pthread_mutex_unlock(&SERVER.game_server_mutex);
  }
  return NULL;
}

static int server_start() {
  int sock = sockets_open_server(IP_ADDRESS, PORT);

  printf("Server listening on port %d\n", PORT);
  return sock;
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

  sockets_close(server_addr);
}
