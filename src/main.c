#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include "threads.c"

#include "raylib.h"

#define PORT 12345

static void server() {
  const int fd = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr))) {
    perror("bind error:");
    return;
  }

  socklen_t addr_len = sizeof(addr);
  getsockname(fd, (struct sockaddr *)&addr, &addr_len);
  printf("server is on port %d\n", (int)ntohs(addr.sin_port));

  if (listen(fd, 1)) {
    perror("listen error:");
    return;
  }

  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  const int cfd = accept(fd, (struct sockaddr *)&caddr, &caddr_len);

  char buf[1024];
  bool sent = false;

  while (true) {
    ssize_t bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
    if (bytes <= 0) {
      // connection closed or error
      break;
    }

    buf[bytes] = '\0'; // Null-terminate received string
    printf("client says:\n    %s\n", buf);
    if (!sent) {
      const char *msg = "Hello client";
      send(cfd, msg, strlen(msg) + 1, 0);
      // sent = true;
    }
  }

  close(cfd);
  close(fd);
}

static void client(int port) {
  InitWindow(800, 600, "Multiplayer test");
  SetTargetFPS(60);

  const int fd = socket(PF_INET, SOCK_STREAM, 0);

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port = htons((short)port);

  // connect to local machine at specified port
  inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

  // connect to server
  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr))) {
    perror("connect error:");
    return;
  }

  // say hey with send!

  const char *hello = "Initial hello";
  send(fd, hello, strlen(hello) + 1, 0);

  while (!WindowShouldClose()) {
    BeginDrawing();
    {
      ClearBackground(RAYWHITE);
      DrawCircle(400, 300, 40, RED);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
          CheckCollisionPointCircle(GetMousePosition(), (Vector2){400, 300},
                                    40)) {
        const char *msg = "Deez";
        send(fd, msg, strlen(msg) + 1, 0);
      }

      // Read from server
      char buf[1024];
      ssize_t bytes =
          recv(fd, buf, sizeof(buf) - 1, MSG_DONTWAIT); // non-blocking
      if (bytes > 0) {
        buf[bytes] = '\0';
        printf("server says: %s\n", buf);
      }
    }
    EndDrawing();
  }

  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc > 1 && !strcmp(argv[1], "--client")) {
    client(PORT);
  } else {
    server();
  }

  threads_example();

  return 0;
}