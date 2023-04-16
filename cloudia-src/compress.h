#ifndef __compress__
#define __compress__

#include <inttypes.h>

#define COMPRESS_BUFF_SIZE 200

typedef struct {
    uint8_t buff[COMPRESS_BUFF_SIZE];
    uint8_t bit_ptr;
    uint16_t byte_ptr;
} compress_t;

void compress_reset(compress_t *c);
void compress_add(compress_t *c, uint32_t value, uint8_t nbits);
void compress_add_with_sign(compress_t *c, int32_t value, uint8_t nbits);
#endif //__compress__