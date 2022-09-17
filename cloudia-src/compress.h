#ifndef _compress_
#define _compress_

#include <inttypes.h>

#define COMPRESS_BUFF_SIZE 200
uint8_t MASK[] = {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

typedef struct {
    uint8_t buff[COMPRESS_BUFF_SIZE];
    uint8_t bit_ptr;
    uint16_t byte_ptr;
} compress_t;
#endif //_compress_