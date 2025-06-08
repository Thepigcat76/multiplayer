//! An array of bytes used for saving and loading data to and from
//! binary

#include <stdint.h>
#include <stdlib.h>

#define BYTE_SIZE 8

typedef struct {
  uint8_t bytes[512];
  size_t writer_index;
  size_t reader_index;
  size_t capacity;
} ByteBuf;

void byte_buf_write_byte(ByteBuf *buf, uint8_t byte);

void byte_buf_write_int(ByteBuf *buf, int32_t integer);

void byte_buf_write_string(ByteBuf *buf, char *str);

uint8_t byte_buf_read_byte(ByteBuf *buf);

int32_t byte_buf_read_int(ByteBuf *buf);

void byte_buf_read_string(ByteBuf *buf, char *str_buf, int len);

int byte_buf_to_bin(const ByteBuf *buf, char *str_buf);

void byte_buf_from_bin(ByteBuf *buf, const char *str_buf);

void byte_buf_from_file(ByteBuf *buf, const char *name);

void byte_buf_to_file(const ByteBuf *buf, const char *name);
