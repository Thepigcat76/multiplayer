#pragma once

#include <stdint.h>
#include <stdlib.h>

#define BYTE_CAPACITY 256

typedef struct {
    uint8_t bytes[BYTE_CAPACITY];
    size_t len;
} ByteBuf;
