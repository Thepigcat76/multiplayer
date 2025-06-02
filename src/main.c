// REMEMBER: Sending a vec2 might not have worked because of little/big endian
// (look at jdh video)

#include "../include/client.h"
#include "../include/server.h"
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc > 1 && strcmp(argv[1], "--client") == 0) {
    client_run();
  } else {
    server_run();
  }
  return 0;
}
