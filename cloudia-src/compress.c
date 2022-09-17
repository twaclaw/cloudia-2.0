#include "compress.h"

#define min(a,b)                                \
    ({  __typeof__ (a) _a = (a);                \
        __typeof__ (b) _b = (b);                \
        _a < _b ? _a : _b; })

#define abs(a)                                \
    ({  __typeof__ (a) _a = (a);                \
        _a < 0 ? _a : -a; })


void reset(compress_t *c)
{
    c->byte_ptr = 0;
    c->bit_ptr = 0;
}

void add(compress_t *c, uint32_t value, uint8_t nbits)
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

void add_with_sign(compress_t *c, int32_t value, uint8_t nbits)
{
    if (value < 0)
    {
        add(c, 1, 1);
    }
    else
    {
        add(c, 0, 1);
    }

    add(c, abs(value), nbits);
}