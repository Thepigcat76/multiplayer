// REMEMBER: Sending a vec2 might not have worked because of little/big endian
// (look at jdh video)
#ifdef SURTUR_BUILD_WIN
#define Rectangle winapiIsSoOldAndGrossSoMangleIt_Rectangle
#define CloseWindow winapiIsSoOldAndGrossSoMangleIt_CloseWindow
#define ShowCursor winapiIsSoOldAndGrossSoMangleIt_ShowCursor
#define LoadImage winapiIsSoOldAndGrossSoMangleIt_LoadImage
#define DrawText winapiIsSoOldAndGrossSoMangleIt_DrawText
#define DrawTextEx winapiIsSoOldAndGrossSoMangleIt_DrawTextEx
#define PlaySound winapiIsSoOldAndGrossSoMangleIt_PlaySound

#include <winsock2.h>
#include "windows.h"

#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef PlaySound
#endif

#ifdef SURTUR_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <stdio.h>
#endif
#include "../include/client.h"
#include "../include/server.h"
#include <string.h>

int main(int argc, char *argv[]) {
#ifdef SURTUR_BUILD_WIN
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    fprintf(stderr, "WSAStartup failed\n");
    return 1;
  }
#endif
  if (argc > 1 && strcmp(argv[1], "--client") == 0) {
    client_run();
  } else {
    server_run();
  }
#ifdef SURTUR_BUILD_WIN
  WSACleanup();
#endif
  return 0;
}