#include "../include/bytebuf.h"
#include <raylib.h>
#include <stdio.h>
#include <string.h>

void byte_buf_write_byte(ByteBuf *buf, uint8_t byte) {
  buf->bytes[buf->writer_index++] = byte;
}

void byte_buf_write_int(ByteBuf *buf, int32_t integer) {
  if (buf->writer_index + sizeof(int32_t) > buf->capacity) {
    return;
  }

  for (int i = sizeof(int32_t) - 1; i >= 0; i--) {
    uint8_t byte = (integer >> (i * 8)) & 0xFF;
    byte_buf_write_byte(buf, byte);
  }
}

void byte_buf_write_string(ByteBuf *buf, char *str) {
  size_t len = strlen(str);
  byte_buf_write_int(buf, (int32_t)len);
  for (int i = 0; i < len; i++) {
    byte_buf_write_byte(buf, str[i]);
  }
}

uint8_t byte_buf_read_byte(ByteBuf *buf) {
  return buf->bytes[buf->reader_index++];
}

int32_t byte_buf_read_int(ByteBuf *buf) {
  int32_t integer = 0;

  for (int i = 0; i < sizeof(int32_t); i++) {
    uint8_t byte = byte_buf_read_byte(buf);
    integer = (integer << 8) | byte;
  }

  return integer;
}

void byte_buf_read_string(ByteBuf *buf, char *str_buf, int len) {
  for (int i = 0; i < len; i++) {
    uint8_t byte = byte_buf_read_byte(buf);
    str_buf[i] = byte;
  }
  str_buf[len] = '\0';
}
