#ifndef _circular_buffer_
#define _circular_buffer_

#include "bme280.h"
#include "ina219.h"

#define CB_SIZE 16

typedef struct
{
    bme280_data_t bme280;
    // ina219_data_t ina219;
    // TODO: add wind and others
} cloudia_t;

typedef struct
{
    cloudia_t data[CB_SIZE];
    int head;
    int tail;
    bool all_filled;
} cb_t;

void cb_reset(cb_t *cb);
void cb_add(cb_t *cb, cloudia_t *source);
int cb_get(cb_t *cb, cloudia_t *dest, uint8_t offset);

#endif //_circular_buffer_