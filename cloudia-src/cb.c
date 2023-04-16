#include "cb.h"
void cb_reset(cb_t *cb)
{
    cb->head = 0;
    cb->tail = 0;
}

void cb_add(cb_t *cb, cloudia_t *source)
{
    cb->head = cb->head < CB_SIZE - 1 ? cb->head + 1 : 0;
    if (cb->head == cb->tail)
    {
        //Advance tail and rewrite
        cb->tail = cb->tail < CB_SIZE - 1 ? cb->tail + 1 : 0;
    }

    cb->data[cb->head].bme280.T = source->bme280.T;
    cb->data[cb->head].bme280.H = source->bme280.H;
    cb->data[cb->head].bme280.P = source->bme280.P;
    // cb->data[cb->head].ina219.current = source->ina219.current;
}

int cb_get(cb_t *cb, cloudia_t *dest, uint8_t offset)
{
    /*Get offset-th most recent position; position @ (head - offset) % CB_SIZE*/
    if (cb->head == cb->tail)
    {
        return -1; // empty
    }
    int curr_size = 0;
    int idx;
    for(idx=cb->tail;idx != cb->head; idx = idx < CB_SIZE - 1 ? idx + 1: 0)
        curr_size++;

    if (offset > CB_SIZE - 1 || offset > curr_size)
    {
        return -2;
    }

    *dest = cb->data[idx];
    return 0;
}