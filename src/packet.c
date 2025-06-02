#include "../include/packet.h"
#include "../include/bytebuf.h"
#include <stdio.h>
#include <sys/socket.h>

static void byte_buf_send(int addr, ByteBuf buf) {
  send(addr, buf.bytes, buf.writer_index, 0);
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
    break;
  }
  case PACKET_C2S_SET_POS: {
    byte_buf_write_byte(buf, packet.var.c2s_set_pos.player_id);
    Vec2i pos = packet.var.c2s_set_pos.pos;
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
    return (Packet){.type = type, .var = {.s2c_player_join = player_id}};
  }
  case PACKET_S2C_NEW_PLAYER_JOINED: {
    return (Packet){.type = type};
  }
  case PACKET_C2S_SET_POS: {
    int player_id = byte_buf_read_byte(buf);
    Vec2i pos;
    pos.x = byte_buf_read_int(buf);
    pos.y = byte_buf_read_int(buf);
    return (Packet){
        .type = type,
        .var = {.c2s_set_pos = {.player_id = player_id, .pos = pos}}};
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
  default:
    return (Packet){.type = PACKET_ERROR};
  }
}

void packet_send(int addr, Packet packet) {
  ByteBuf buf = {.writer_index = 0, .reader_index = 0, .capacity = 512};
  packet_encode(packet, &buf);
  byte_buf_send(addr, buf);
}

Packet packet_receive(int addr) {
  ByteBuf buf = {
      .reader_index = 0,
      .writer_index = 0,
      .capacity = 512,
  };
  ssize_t bytes = recv(addr, &buf.bytes, buf.capacity, 0);
  if (bytes <= 0) {
    perror("Failed to receive packet");
    return (Packet){.type = PACKET_ERROR};
  }
  buf.len = bytes;
  return packet_decode(&buf);
}
