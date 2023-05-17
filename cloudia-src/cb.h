#ifndef _circular_buffer_
#define _circular_buffer_

#include "data.h"

#define CB_SIZE 250

typedef struct
{
    cloudia_t data[CB_SIZE];
    int head;
    int tail;
} cb_t;

void cb_reset(cb_t *cb);
void cb_add(cb_t *cb, cloudia_t *source);
int cb_get(cb_t *cb, cloudia_t *dest, uint8_t offset);

#endif //_circular_buffer_