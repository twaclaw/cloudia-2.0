// Copyright (C) 2020-2020 Michael Kuyper. All rights reserved.
// Copyright (C) 2016-2019 Semtech (International) AG. All rights reserved.
//
// This file is subject to the terms and conditions defined in file 'LICENSE',
// which is part of this source code package.

#include "lmic.h"
#include "lwmux/lwmux.h"
#include "svcdefs.h"
#include "bme280.h"
#include "conf.h"

static lwm_job lj;
static osjob_t *mainjob;

static void next(osjob_t *job);

static void txc(void)
{
    os_setApproxTimedCallback(mainjob, os_getTime() + sec2osticks(5), next);
}

static bool tx(lwm_txinfo *txinfo)
{
    txinfo->data = (unsigned char *)"hello";
    txinfo->dlen = 5;
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

static void sensor_loop(osjob_t *job)
{
    static int status;
    static conf_t conf;
    conf.options = 0x03;
    conf.period = 0x45; // 5 seconds
    conf.buffer_size = 5;

    static enum {
        INIT,
        DONE,
    } state;
    switch(state){
        case INIT:
            bme280_config(job, sensor_loop, &status, &conf);
            break;
        case DONE:
            debug_printf("DONE\r\n");
            break;
    }
    state += 1;
}

bool app_main(osjob_t *job)
{
    debug_printf("Hello World!\r\n");
    // join network
    // lwm_setmode(LWM_MODE_NORMAL);
    // re-use current job
    mainjob = job;
    // initiate first uplink
		// hal_disableIRQs();
    //next(mainjob);
    os_setCallback(job, sensor_loop);
    // indicate that we are running
    return true;
}
