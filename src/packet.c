#include "../include/sockets.h"
#ifdef SURTUR_BUILD_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif
#include "../include/packet.h"
#include "../include/bytebuf.h"
#include <stdio.h>

static void byte_buf_send(int addr, ByteBuf buf) {
  sockets_send(addr, (SocketDataBuffer){buf.bytes, buf.writer_index}, 0);
}

static void packet_fmt(Packet packet, int addr, bool serverbound,
                       bool is_client, char *buf) {
  char *prefix = ">>>";
  char postfix[10];
  if (serverbound && is_client) {
    prefix = "<<<";
    sprintf(postfix, "TO %d", addr);
  } else if (!serverbound && is_client) {
    prefix = ">>>";
    sprintf(postfix, "FROM %d", addr);
  } else if (serverbound && !is_client) {
    prefix = ">>>";
    sprintf(postfix, "FROM %d", addr);
  } else if (!serverbound && !is_client) {
    prefix = "<<<";
    sprintf(postfix, "TO %d", addr);
  }
  if (is_client && !serverbound) {
  }

  switch (packet.type) {
  case PACKET_ERROR: {
    sprintf(buf, "%s [PACKET_ERROR] %s", prefix, postfix);
    break;
  }
  case PACKET_S2C_GAME_SYNC: {
    sprintf(buf, "%s [PACKET_GAME_SYNC]{players=%d,player_positions=...} %s",
            prefix, packet.var.s2c_game_sync.game.players, postfix);
    break;
  }
  case PACKET_S2C_PLAYER_JOIN: {
    sprintf(buf, "%s [PLAYER_JOIN]{player=%d} %s", prefix,
            packet.var.s2c_player_join.player_id, postfix);
    break;
  }
  case PACKET_S2C_NEW_PLAYER_JOINED: {
    sprintf(buf, "%s [NEW_PLAYER_JOINED]{player=%d} %s", prefix,
            packet.var.s2c_new_player_joined.player_id, postfix);
    break;
  }
  case PACKET_BIDIR_SET_POS: {
    sprintf(buf, "%s [SET_POS]{player=%d,pos={x=%d,y=%d}} %s", prefix,
            packet.var.bidir_set_pos.player_id, packet.var.bidir_set_pos.pos.x,
            packet.var.bidir_set_pos.pos.y, postfix);
    break;
  }
  }
}

static void packet_encode(Packet packet, ByteBuf *buf) {
  byte_buf_write_byte(buf, packet.type);
  switch (packet.type) {
  case PACKET_ERROR: {
    break;
  }
  case PACKET_S2C_PLAYER_JOIN: {
    int player_id = packet.var.s2c_player_join.player_id;
    byte_buf_write_byte(buf, player_id);
    break;
  }
  case PACKET_S2C_NEW_PLAYER_JOINED: {
    int player_id = packet.var.s2c_new_player_joined.player_id;
    byte_buf_write_byte(buf, player_id);
    break;
  }
  case PACKET_BIDIR_SET_POS: {
    byte_buf_write_byte(buf, packet.var.bidir_set_pos.player_id);
    Vec2i pos = packet.var.bidir_set_pos.pos;
    byte_buf_write_int(buf, pos.x);
    byte_buf_write_int(buf, pos.y);
    break;
  }
  case PACKET_S2C_GAME_SYNC: {
    Game game = packet.var.s2c_game_sync.game;
    int players = game.players;
    byte_buf_write_byte(buf, players);
    Vec2i *positions = game.player_positions;
    for (int i = 0; i < players; i++) {
      byte_buf_write_int(buf, positions[i].x);
      byte_buf_write_int(buf, positions[i].y);
    }
    break;
  }
  }
}

static Packet packet_decode(ByteBuf *buf) {
  int type = byte_buf_read_byte(buf);
  switch (type) {
  case PACKET_S2C_PLAYER_JOIN: {
    int player_id = byte_buf_read_byte(buf);
    return (Packet){.type = type,
                    .var = {.s2c_player_join = {.player_id = player_id}}};
  }
  case PACKET_S2C_NEW_PLAYER_JOINED: {
    int player_id = byte_buf_read_byte(buf);
    return (Packet){.type = type,
                    .var = {.s2c_new_player_joined = {.player_id = player_id}}};
  }
  case PACKET_BIDIR_SET_POS: {
    int player_id = byte_buf_read_byte(buf);
    Vec2i pos;
    pos.x = byte_buf_read_int(buf);
    pos.y = byte_buf_read_int(buf);
    return (Packet){
        .type = type,
        .var = {.bidir_set_pos = {.player_id = player_id, .pos = pos}}};
  }
  case PACKET_S2C_GAME_SYNC: {
    Game game = {};
    game.players = byte_buf_read_byte(buf);
    for (int i = 0; i < game.players; i++) {
      game.player_positions[i].x = byte_buf_read_int(buf);
      game.player_positions[i].y = byte_buf_read_int(buf);
    }
    return (Packet){.type = type, .var = {.s2c_game_sync = {.game = game}}};
  }
  default: {
    printf("ERROR DECODING\n");
    return (Packet){.type = PACKET_ERROR};
  }
  }
}

void packet_send(int addr, Packet packet, bool is_client) {
  ByteBuf buf = {.writer_index = 0, .reader_index = 0, .capacity = 512};
  packet_encode(packet, &buf);
  // Send: 2-byte length + data
  uint16_t len = buf.writer_index;
  uint8_t header[2] = {len >> 8, len & 0xFF};
  sockets_send(addr, (SocketDataBuffer){header, 2}, 0);
  sockets_send(addr, (SocketDataBuffer){buf.bytes, len}, 0);
  char print_buf[256];
  packet_fmt(packet, addr, is_client, is_client, print_buf);
  printf("%s\n", print_buf);
}

Packet packet_receive(int addr, bool is_client) {
  uint8_t len_buf[2];
  ssize_t n =
      sockets_receieve(addr, (SocketDataBuffer){len_buf, 2}, MSG_WAITALL);
  if (n != 2) {
    perror("Failed to read length");
    return (Packet){.type = PACKET_ERROR};
  }

  uint16_t len = (len_buf[0] << 8) | len_buf[1];
  if (len > 512) {
    fprintf(stderr, "Packet too long: %u\n", len);
    return (Packet){.type = PACKET_ERROR};
  }

  ByteBuf buf = {.reader_index = 0, .writer_index = len, .capacity = 512};
  n = sockets_receieve(addr, (SocketDataBuffer){buf.bytes, len}, MSG_WAITALL);
  if (n != len) {
    perror("Failed to read full packet");
    return (Packet){.type = PACKET_ERROR};
  }

  buf.writer_index = 0;

  Packet packet = packet_decode(&buf);

  char print_buf[256];
  packet_fmt(packet, addr, !is_client, is_client, print_buf);
  printf("%s\n", print_buf);

  return packet;
}
