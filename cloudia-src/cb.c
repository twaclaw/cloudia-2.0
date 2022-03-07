#include "cb.h"
void cb_reset(cb_t *cb)
{
    cb->head = 0;
    cb->tail = 0;
    cb->all_filled = false;
}

void cb_add(cb_t *cb, cloudia_t *source)
{
    if (cb->head < CB_SIZE - 1)
    {
        cb->head++;
    }
    else
    {
        cb->head = 0;
        cb->tail = cb->tail < CB_SIZE - 1 ? cb->tail + 1 : 0;
        cb->all_filled = true;
    }

    cb->data[cb->head].bme280.T = source->bme280.T;
    cb->data[cb->head].bme280.H = source->bme280.H;
    cb->data[cb->head].bme280.P = source->bme280.P;
    cb->data[cb->head].ina219.current = source->ina219.current;
}

int cb_get(cb_t *cb, cloudia_t *dest, uint8_t offset)
{
    if (offset > CB_SIZE - 1)
    {
        return -2;
    }
    if (cb->head == cb->tail && !cb->all_filled)
    {
        return -1; // empty
    }
    int diff = cb->head - cb->tail;
    if (!cb->all_filled && diff < offset)
    {
        return -1; // empty or not enough data
    }

    diff = cb->head - offset;
    int index = diff >= 0 ? diff : CB_SIZE + diff;
    *dest = cb->data[index];
    return 0;
}