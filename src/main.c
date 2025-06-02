#include <arpa/inet.h>
#include <pthread.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "raylib.h"

#include "bytebuf.h"

#define PORT 12345

static void send_bytebuf(int fd, ByteBuf *buf) {}

void *run_server(void *args) {
  int client_fd = ((int *)args)[0];
  int server_fd = ((int *)args)[1];

  while (1) {
    const char *message = "Hello from server\n";
    send(client_fd, message, strlen(message), 0);
    sleep(1); // Send every 1 second
  }

  close(client_fd);
  close(server_fd);
  return NULL;
}

void *run_server_listener(void *args) {
  int client_fd = ((int *)args)[0];

  char buffer[1024];
  while (1) {
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
      printf("Disconnected.\n");
      break;
    }
    buffer[bytes] = '\0';
    printf("Received: %s", buffer);
  }
  return NULL;
}

void multithreaded_server() {
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    perror("socket failed");
    return;
  }

  // Allow address reuse
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind failed");
    close(server_fd);
    return;
  }

  if (listen(server_fd, 1) < 0) {
    perror("listen failed");
    close(server_fd);
    return;
  }

  printf("Server listening on port %d\n", PORT);

  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    perror("accept failed");
    close(server_fd);
    return;
  }

  printf("Client connected!\n");

  pthread_t logic_thread;
  pthread_t networking_listener_thread;

  int args[] = {client_fd, server_fd};
  if (pthread_create(&logic_thread, NULL, run_server, args)) {
    perror("Failed to create logic thread");
    exit(1);
  }

  if (pthread_create(&networking_listener_thread, NULL, run_server_listener,
                     args)) {
    perror("Failed to create networking listener thread");
    exit(1);
  }

  pthread_join(logic_thread, NULL);
  pthread_join(networking_listener_thread, NULL);
  printf("Game has finished\n");
}

static bool send_pos = false;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct {
  int player_x;
  int player_y;
} Game;

static Game GAME = {.player_x = 0, .player_y = 0};

void *run_game(void *) {
  InitWindow(800, 600, "Hello");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);
      DrawCircle(GAME.player_x, GAME.player_y, 20, RED);
      // TraceLog(LOG_INFO, "Time: %d", (int)GetTime());
      if (IsKeyDown(KEY_W)) {
        GAME.player_y -= 3;
      }
      if (IsKeyDown(KEY_S)) {
        GAME.player_y += 3;
      }
      if (IsKeyDown(KEY_A)) {
        GAME.player_x -= 3;
      }
      if (IsKeyDown(KEY_D)) {
        GAME.player_x += 3;
      }

      if ((int)GetTime() % 3 == 0) {
        pthread_mutex_lock(&mutex);
        send_pos = true;
        // pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
      }
    }
    EndDrawing();
  }
  CloseWindow();
  return NULL;
}

void *run_client(void *) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("socket failed");
    return NULL;
  }

  struct sockaddr_in server_addr = {0};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

  if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect failed");
    close(sock);
    return NULL;
  }

  printf("Connected to server!\n");

  while (1) {
    char buffer[1024];
    ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) {
      printf("Disconnected.\n");
      break;
    }
    buffer[bytes] = '\0';
    printf("Received: %s", buffer);

    if (bytes > 4) {
      const char *msg = "Received your message\n";
      send(sock, msg, strlen(msg) + 1, 0);
    }

    // pthread_mutex_lock(&mutex);
    // TraceLog(LOG_INFO, "SET BLUE");
    //  pthread_cond_signal(&cond);
    // pthread_mutex_unlock(&mutex);
  }

  close(sock);
  return NULL;
}

void multithreaded_client() {
  pthread_t game_thread;
  pthread_t networking_thread;

  if (pthread_create(&game_thread, NULL, run_game, NULL)) {
    perror("Failed to create game thread");
    exit(1);
  }

  if (pthread_create(&networking_thread, NULL, run_client, NULL)) {
    perror("Failed to create networking thread");
    exit(1);
  }

  pthread_join(game_thread, NULL);
  pthread_join(networking_thread, NULL);
  printf("Game has finished\n");
}

int main(int argc, char *argv[]) {
  if (argc > 1 && strcmp(argv[1], "--client") == 0) {
    multithreaded_client();
  } else {
    multithreaded_server();
  }
  return 0;
}
