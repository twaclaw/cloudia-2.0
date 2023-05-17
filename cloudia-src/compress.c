#include "compress.h"

uint8_t MASK[] = {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

#define min(a, b) \
    ({  __typeof__ (a) _a = (a);                \
        __typeof__ (b) _b = (b);                \
        _a < _b ? _a : _b; })

#define abs(a) \
    ({  __typeof__ (a) _a = (a);                \
        _a < 0 ? _a : -a; })

void compress_reset(compress_t *c)
{
    c->byte_ptr = 0;
    c->bit_ptr = 0;
    for (int i = 0; i < COMPRESS_BUFF_SIZE; i++)
        c->buff[i] = 0;
}

void compress_add(compress_t *c, uint32_t value, uint8_t nbits)
{
    while (nbits)
    {
        uint8_t shift_by = min(nbits, 8 - c->bit_ptr);
        uint8_t y = (uint8_t)(value & MASK[shift_by - 1]);
        y <<= c->bit_ptr;
        c->buff[c->byte_ptr] |= y;
        nbits -= shift_by;
        value >>= shift_by;

        if ((8 - c->bit_ptr) > shift_by)
        {
            c->bit_ptr += shift_by;
        }
        else
        {
            c->bit_ptr = 0;
            c->byte_ptr += 1;
        }
    }
}

void compress_add_with_sign(compress_t *c, int32_t value, uint8_t nbits)
{
    if (value < 0)
    {
        compress_add(c, 1, 1);
    }
    else
    {
        compress_add(c, 0, 1);
    }

    compress_add(c, abs(value), nbits - 1);
}