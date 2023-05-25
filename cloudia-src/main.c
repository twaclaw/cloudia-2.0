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
#include "data.h"

static lwm_job lj;
static osjob_t *mainjob;

static cloudia_t cloudia; // data.h
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
    uint8_t T_nbits = 0, H_nbits = 0;

    uint8_t port;

    if (tx_state.pending)
    {
        int max_payload = LMIC_maxAppPayload();

        debug_printf("LMIC Max payload: %d\r\n", max_payload);
        cloudia_t dest;

        int status = 0, offset = tx_state.offset;

        // estimate the maximum differences between consecutive
        // T and H values in the buffer
        status = cb_get(&circ_buf, &dest, offset++); // get first value
        T0 = dest.sht35.T;
        H0 = dest.sht35.H;
        while ((status == 0 && offset < tx_state.ndata))
        {
            status = cb_get(&circ_buf, &dest, offset); // get meas group from circular buffer starting at offset i
            if (!status)
            {
                T_diff = dest.sht35.T - T0;
                T_diff = T_diff >= 0 ? T_diff : -T_diff; // abs

                H_diff = dest.sht35.H - H0;
                H_diff  = H_diff >= 0 ? H_diff : -H_diff; // abs

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

        offset = tx_state.offset;
        status = cb_get(&circ_buf, &dest, offset); // get first value
        T0 = dest.sht35.T;
        H0 = dest.sht35.H;

        // TODO: use_diffs can also be overwritten by the configuration
        bool use_diffs = true;
        if ((H_nbits > 8 || T_nbits > 8) || tx_state.ndata < 2)
            use_diffs = false;

        debug_printf("Hnbits: %d, Tnbits %d -> use_diffs: %d\r\n", H_nbits, T_nbits, use_diffs);

        // Define which command (port) to send
        int buff_idx = 0;
        uint8_t SR1, SR2, SR3;
        uint8_t batt_value = 0;

        // TODO: read batt value

        SR1 = CONF_SR1_TEN | CONF_SR1_HEN | ((1 & CONF_SR1_TRES_MASK) << CONF_SR1_TRES_BIT) | ((batt_value & CONF_SR1_BAT_MASK) << CONF_SR1_BAT_BIT);
        SR2 = config.r3;
        SR3 = ((T_nbits & CONF_SR3_TDIFF_MASK) << CONF_SR3_TDIFF_BIT) | ((H_nbits & CONF_SR3_HDIFF_MASK) << CONF_SR3_HDIFF_BIT);

        txinfo->data[buff_idx++] = SR1;
        if (tx_state.ndata == 1)
        {
            port = CONF_PORT_SINGLE_MEAS;
        }
        else if (!use_diffs)
        {
            txinfo->data[buff_idx++] = SR2;
            if (tx_state.offset == 0)
                port = CONF_PORT_MULT_MEAS_OFFSET_0;
            else
            {
                port = CONF_PORT_MULT_MEAS_OFFSET_GT_0;
                txinfo->data[buff_idx++] = tx_state.offset & 0xFF; // max offset (currently):255
            }
        }
        else
        {
            txinfo->data[buff_idx++] = SR2;
            txinfo->data[buff_idx++] = SR3;
            if (tx_state.offset == 0)
                port = CONF_PORT_MULT_MEAS_OFFSET_0_DIFFS;
            else
            {
                port = CONF_PORT_MULT_MEAS_OFFSET_GT_0_DIFFS;
                txinfo->data[buff_idx++] = tx_state.offset & 0xFF; // max offset (currently):255
            }
        }

        status = 0, offset = tx_state.offset;
        compress_reset(&compress_buf);

        // Add first values uncompressed.

        compress_add_with_sign(&compress_buf, (int32_t)T0, CONF_VAR_T_NBITS);
        compress_add(&compress_buf, (int32_t)H0, CONF_VAR_H_NBITS);
        offset++;

        while ((status == 0 && offset < tx_state.ndata) && (compress_buf.byte_ptr < COMPRESS_BUFF_SIZE && compress_buf.byte_ptr < max_payload - 5))
        {
            status = cb_get(&circ_buf, &dest, offset); // get meas group from circular buffer starting at offset i
            if (use_diffs)
            {
                T_diff = dest.sht35.T - T0;
                H_diff = dest.sht35.H - H0;
                T0 = dest.sht35.T;
                H0 = dest.sht35.H;

                compress_add_with_sign(&compress_buf, (int32_t)T_diff, T_nbits);
                compress_add_with_sign(&compress_buf, (int32_t)H_diff, H_nbits);
            }
            else
            {
                compress_add_with_sign(&compress_buf, (int32_t)dest.sht35.T, CONF_VAR_T_NBITS);
                compress_add(&compress_buf, (int32_t)dest.sht35.H, CONF_VAR_H_NBITS);
            }
            offset++;
        }

        int j;
        for (j = 0; j < compress_buf.byte_ptr + (compress_buf.bit_ptr > 1 ? 1 : 0); j++)
        {
            // TODO: format package, add headers (config buffers with offset)
            txinfo->data[j + buff_idx] = compress_buf.buff[j];
            debug_printf("BUF[%d]: %d\r\n", j, compress_buf.buff[j]);
        }
        txinfo->dlen = buff_idx + j;
        txinfo->port = port;
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

    nsamples = 5;     // config.r4 < CB_SIZE  ? config.r4 : CB_SIZE;
    meas_period = 10; // TODO: decode config.r3
    cb_reset(&circ_buf);

    // join network
    lwm_setmode(LWM_MODE_NORMAL);
    // debug_printf("LMIC: %d", getSf(LMIC.rps) + 6);
    // re-use current job
    // initiate first uplink
    // next(mainjob);
    // TODO: wait until joined before going to the sensor loop
    os_setCallback(job, sensor_loop);
    // indicate that we are running
    return true;
}
