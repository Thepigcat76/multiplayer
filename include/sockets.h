#pragma once

#ifdef SURTUR_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <poll.h>
#endif
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct pollfd sockets_pollfd;

#define MAX_CLIENTS 32

typedef struct {
    void *buffer;
    uint64_t buffer_size;
} SocketDataBuffer;

typedef 
#ifdef SURTUR_BUILD_WIN
struct pollfd
#else
struct pollfd
#endif
PollClient;

typedef void (*ConnectionHandleFunc)(int32_t client_addr);

int32_t sockets_open_server(char *ip_address, uint32_t port);

int32_t sockets_connect_to_server(char *ip_address, uint32_t port);

// Returns the amount of bytes or -1 if sending fails
int64_t sockets_send(int32_t socket_addr, SocketDataBuffer buf, int32_t flags);

// Returns the amount of bytes or -1 if receiving fails
int64_t sockets_receieve(int32_t socket_addr, SocketDataBuffer buf, int32_t flags);

// Returns the address of the accepted client or -1 if accepting fails
int64_t sockets_server_accept_client(int32_t socket_addr);

int64_t sockets_server_poll_clients(PollClient *polling_clients, uint64_t polling_clients_amount, int32_t timeout);

void sockets_server_handle_poll(int64_t result);

void sockets_close(int32_t socket_addr);
