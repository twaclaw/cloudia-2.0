/*
  ____ _     ___  _   _ ____  _
 / ___| |   / _ \| | | |  _ \(_) __ _
| |   | |  | | | | | | | | | | |/ _` |
| |___| |__| |_| | |_| | |_| | | (_| |
 \____|_____\___/ \___/|____/|_|\__,_|
*/

#include "lmic.h"
#include "lwmux/lwmux.h"
#include "eefs/eefs.h"
#include "svcdefs.h"
#include "bme280.h"
#include "sht35.h"
#include "ina219.h"
#include "conf.h"
#include "cb.h"
#include "compress.h"

static lwm_job lj;
static osjob_t *mainjob;

static cloudia_t cloudia; // cb.h
static cb_t circ_buf;
static bool joined = false;
static int nsamples = 0;
static int meas_period = 0;

typedef struct
{
    int ndata; // number of samples to transmit
    bool pending;
    int offset;      // initial offset
    int curr_offset; // current offset for which the transmission is being attempted
} tx_state_t;

static tx_state_t tx_state;

static const uint8_t UFID_CONFIG[12] = {0xb0, 0xb8, 0xca, 0xe0, 0xc0, 0xe0, 0x9d, 0x16, 0x54, 0x2a, 0xea, 0xa0};

static conf_t config;

const char *conf_eefs_fn(const uint8_t *ufid)
{
    if (memcmp(ufid, UFID_CONFIG, sizeof(UFID_CONFIG)) == 0)
    {
        return "com.semtech.modem.startup";
    }
    else
    {
        return NULL;
    }
}

void conf_eefs_load(void)
{
    if (eefs_read(UFID_CONFIG, &config, sizeof(config)) != sizeof(config))
    {
        // memset(&config, 0, sizeof(config));
        debug_printf("Loading default configuration");
        memcpy(&config, &defaultcfg, sizeof(config));
        debug_printf("Loading default configuration r1: %d, r2: %d, r4: %d\n", config.r1, config.r2, config.r4);
    }
}

static void conf_save(void)
{
    ASSERT(eefs_save(UFID_CONFIG, &config, sizeof(config)) >= 0);
}

static void next(osjob_t *job);
static void sensor_loop(osjob_t *job);

static void txc(void)
{
    debug_printf("TXC\r\n");
    if (tx_state.curr_offset < tx_state.ndata)
    {
        tx_state.offset = tx_state.curr_offset;
        tx_state.pending = true;
    }
    else
    {
        tx_state.pending = false;
    }
    next(mainjob);
}

static bool tx(lwm_txinfo *txinfo)
{
    compress_t compress_buf;
    int16_t T0, T_diff;
    uint8_t H0;
    int8_t H_diff;
    uint16_t T_diff_max = 0;
    uint8_t H_diff_max = 0;
    uint8_t T_nbits, H_nbits;

    if (tx_state.pending)
    {
        int max_payload = LMIC_maxAppPayload();

        debug_printf("LMIC Max payload: %d\r\n", max_payload);
        cloudia_t dest;
        compress_reset(&compress_buf);

        int status = 0, offset = tx_state.offset;

        // estimate the maximum differences between consecutive
        // T and H values in the buffer
        status = cb_get(&circ_buf, &dest, offset); // get first value
        T0 = dest.sht35.T;
        H0 = dest.sht35.H;
        while ((status == 0 && offset < tx_state.ndata))
        {
            status = cb_get(&circ_buf, &dest, offset); // get meas group from circular buffer starting at offset i
            if (status)
            {
                T_diff = dest.sht35.T - T0;
                T_diff_max = T_diff >= 0 ? T_diff : -T_diff; // abs

                H_diff = dest.sht35.H - H0;
                H_diff_max = H_diff >= 0 ? H_diff : -H_diff; // abs
                if (T_diff > T_diff_max)
                    T_diff_max = T_diff;

                if (H_diff > H_diff_max)
                    H_diff_max = H_diff;

                T0 = dest.sht35.T;
                H0 = dest.sht35.H;
            }
            offset++;
        }

        while (T_diff_max)
        {
            T_nbits++;
            T_diff_max >>= 1;
        }

        while (H_diff_max)
        {
            H_nbits++;
            H_diff_max >>= 1;
        }

        status = 0, offset = tx_state.offset;
        while ((status == 0 && offset < tx_state.ndata) && (compress_buf.byte_ptr < COMPRESS_BUFF_SIZE && compress_buf.byte_ptr < max_payload - 5))
        {
            status = cb_get(&circ_buf, &dest, offset); // get meas group from circular buffer starting at offset i
            if (H_nbits > 8 || T_nbits > 8)
            {
                // Do not use differences
                // command 80 or 81
            }
            else
            {
                // command 82 or 83 (90 91)
                compress_add_with_sign(&compress_buf, dest.sht35.T, 11);
                compress_add(&compress_buf, dest.sht35.H, 7);
            }
            offset++;
        }
        debug_printf("Byte ptr: %d\r\n", compress_buf.byte_ptr);

        int j;
        for (j = 0; j < compress_buf.byte_ptr; j++)
        {
            // TODO: format package, add headers (config buffers with offset)
            txinfo->data[j] = compress_buf.buff[j];
        }
        debug_printf("Indices j: %d, max: %d", j, max_payload);
        txinfo->dlen = j;
        txinfo->port = 80;
        txinfo->txcomplete = txc;

        tx_state.curr_offset = offset;
    }
    else
    {
        debug_printf("Done with TX");
    }
    debug_printf("TX ....\r\n");
    return true;
}

static void next(osjob_t *job)
{
    lwm_request_send(&lj, 0, tx);
}

void app_dl(int port, unsigned char *data, int dlen, unsigned int flags)
{
    debug_printf("DL[%d]: %h\r\n", port, data, dlen);
    if (port == CONF_PORT && dlen == sizeof(config))
    {
        memcpy(&config, data, dlen);
        conf_save();
        // TODO: restart FSM?
    }
}

void app_event(ev_t e)
{
    // TODO: implement exponential backoff for retrying
    switch (e)
    {
    case EV_JOINING:
        joined = false;
        break;

    case EV_JOINED:
        joined = true;
        break;

    case EV_JOIN_FAILED:
        joined = false;
        debug_printf("**************************************** JOIN FAILED\r\n");
        break;
    default:
        break;
    }
}
static void sensor_loop(osjob_t *job)
{

    static int status;

    static int samples = 0;

    static enum { INIT,
                  MEAS,
                  NEXT,
                  TRANSMIT,
    } state;

    switch (state)
    {
    case INIT:
        // if(!joined){
        //     //TODO: implement with back-off
        //     debug_printf("Waiting for device to join\r\n");
        //     os_setApproxTimedCallback(job, os_getTime() + sec2osticks(10), sensor_loop);
        //     break;
        // }
        // bme280_config(job, sensor_loop, &status, &config);

        os_setCallback(job, sensor_loop);
        state = MEAS;
        break;
    case MEAS:
        sht35_read(job, sensor_loop, &status, &cloudia.sht35);
        // bme280_read(job, sensor_loop, &status, &cloudia.bme280, &config);
        // ina219_read(job, sensor_loop, &status, &cloudia.ina219, &config);
        state = NEXT;
        break;
    case NEXT:
        if (status != 0)
        {
            state = INIT;
            os_setApproxTimedCallback(job, os_getTime() + sec2osticks(meas_period), sensor_loop);
            // TODO: fill out status bit for heart beat
            break;
        }

        if (++samples < nsamples)
        {
            state = MEAS;
            os_setApproxTimedCallback(job, os_getTime() + sec2osticks(meas_period), sensor_loop);
        }
        else
        {
            os_setCallback(job, sensor_loop);
            state = TRANSMIT;
        }

        debug_printf("Measuring: status %d, samples :%d\r\n", status, samples);
        debug_printf("T %d, H :%d\r\n", cloudia.sht35.T, cloudia.sht35.H);
        cb_add(&circ_buf, &cloudia);
        break;
    case TRANSMIT:
        tx_state.offset = 0;
        tx_state.ndata = nsamples;
        tx_state.pending = true;
        mainjob = job;
        next(mainjob);
        state = MEAS;
        os_setApproxTimedCallback(job, os_getTime() + sec2osticks(meas_period), sensor_loop);
        samples = 0;
        break;
    }
    return;
    // state += 1;
}

bool app_main(osjob_t *job)
{
    // TODO: DELETE ME:
    debug_printf("TODO: REMOVE ME: Loading default configuration\r\n");
    memcpy(&config, &defaultcfg, sizeof(config));
    debug_printf("Loading default configuration r1: %d, r2: %d, r4: %d\r\n", config.r1, config.r2, config.r4);

    nsamples = 100;  // config.r4 < CB_SIZE  ? config.r4 : CB_SIZE;
    meas_period = 1; // TODO: decode config.r3
    cb_reset(&circ_buf);

    // join network
    lwm_setmode(LWM_MODE_NORMAL);
    // debug_printf("LMIC: %d", getSf(LMIC.rps) + 6);
    // re-use current job
    // initiate first uplink
    // next(mainjob);
    os_setCallback(job, sensor_loop);
    // indicate that we are running
    return true;
}
