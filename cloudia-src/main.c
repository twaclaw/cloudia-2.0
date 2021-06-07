/*
  ____ _     ___  _   _ ____  _       
 / ___| |   / _ \| | | |  _ \(_) __ _
| |   | |  | | | | | | | | | | |/ _` |
| |___| |__| |_| | |_| | |_| | | (_| |
 \____|_____\___/ \___/|____/|_|\__,_|
*/

#include "lmic.h"
#include "lwmux/lwmux.h"
#include "svcdefs.h"
#include "bme280.h"
#include "ina219.h"
#include "conf.h"

static lwm_job lj;
static osjob_t *mainjob;

static struct
{
    bme280_data_t bme280;
    ina219_data_t ina219;
} cloudia;

static void next(osjob_t *job);
static void sensor_loop(osjob_t *job);

static void txc(void)
{
    os_setApproxTimedCallback(mainjob, os_getTime() + sec2osticks(5), sensor_loop);
}

static bool tx(lwm_txinfo *txinfo)
{

    int max_payload = 1; //LMIC_maxAppPayload() -10;

    debug_printf("LMIC Max payload: %d\r\n", max_payload);
    // txinfo->data = (unsigned char *)"hello";
    for (int i = 0; i < max_payload; i++)
    {
        txinfo->data[i] = i;
    }
    txinfo->dlen = max_payload;
    txinfo->port = 15;
    txinfo->txcomplete = txc;
    return true;
}

static void next(osjob_t *job)
{
    lwm_request_send(&lj, 0, tx);
}

void app_dl(int port, unsigned char *data, int dlen, unsigned int flags)
{
    debug_printf("DL[%d]: %h\r\n", port, data, dlen);
}

void app_event(ev_t e)
{
    switch (e)
    {
    case EV_JOINING:
        debug_printf("***************************JOINING\r\n");
        break;

    case EV_JOINED:
        debug_printf("***********************************JOINED\r\n");
        break;

    case EV_JOIN_FAILED:
        debug_printf("**************************************** JOIN FAILED\r\n");
        break;
    default:
        break;
    }
}
static void sensor_loop(osjob_t *job)
{
    static int status;

    static conf_t conf;
    conf.r1 = 0x07;
    conf.r3 = 0x45; // 5 seconds
    conf.r4 = 55;

    uint8_t period = 5;
    static int8_t samples = 0;

    static enum { INIT,
                  MEAS,
                  NEXT,
                  TRANSMIT,
                  DONE,
    } state;
    switch (state)
    {
    case INIT:
        // bme280_config(job, sensor_loop, &status, &conf);
        ina219_config(job, sensor_loop, &status);
        state = MEAS;
        break;
    case MEAS:
        debug_printf("Measuring: status %d\r\n", status);
        // bme280_read(job, sensor_loop, &status, &cloudia.bme280, &conf);
        ina219_read(job, sensor_loop, &status, &cloudia.ina219, &conf);
        state = NEXT;
        break;
    case NEXT:
        //TODO: check status
        debug_printf("status: %d, current: 0x%x\r\n", status, cloudia.ina219.current);
        if (++samples < conf.r4)
        {
            state = MEAS;
            os_setApproxTimedCallback(job, os_getTime() + sec2osticks(period), sensor_loop);
        }
        else
        {
            os_setCallback(job, sensor_loop);
            // state = TRANSMIT;
        }
        break;
    case TRANSMIT:
        debug_printf("TRANSMITTING \r\n");
        mainjob = job;
        next(mainjob);
        state = MEAS;
        samples = 0;
        break;
    }
    return;
    // state += 1;
}

bool app_main(osjob_t *job)
{
    debug_printf("Hello World!\r\n");
    // join network
    lwm_setmode(LWM_MODE_NORMAL);
    // debug_printf("LMIC: %d", getSf(LMIC.rps) + 6);
    // re-use current job
    // initiate first uplink
    //next(mainjob);
    os_setCallback(job, sensor_loop);
    // indicate that we are running
    return true;
}
