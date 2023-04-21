#include "lmic.h"

#include "sht35.h"

static struct {
    uint8_t buf[6];

    osjobcb_t cb;

    int* pstatus;
    sht35_data_t* pdata;
} sht35;

#define SHT35_I2C_ADDR	(0x44 << 1)

#define LSR_ROUND(v,a) (((v) + (1 << ((a) - 1))) >> (a))

uint8_t crc8(const uint8_t* data, int len) {

    const uint8_t POLYNOMIAL = 0x31;
    uint8_t crc = 0xFF;

    for (int j = len; j; --j) {
        crc ^= *data++;

        for (int i = 8; i; --i) {
            crc = (crc & 0x80)
                  ? (crc << 1) ^ POLYNOMIAL
                  : (crc << 1);
        }
    }
    return crc;
}

static void read_func (osjob_t* job) {
    static enum {
	INIT, WAIT, READ,DONE,
    } state;

    switch (state) {
	case INIT:
	    // trigger T and H measurement
		uint16_t cmd = SHT35_SINGLE_LOW_REP_NO_CLOCK_STRETCH;
	    sht35.buf[1] = cmd & 0xFF;
	    sht35.buf[0] = (cmd >> 8) & 0xFF;
	    i2c_xfer_ex(SHT35_I2C_ADDR, sht35.buf, 2, 0, ms2osticks(500), job, read_func, sht35.pstatus);
	    break;
	case WAIT:
	    if (*sht35.pstatus != I2C_OK) {
			goto error;
	    }
        os_setApproxTimedCallback(job, os_getTime() + ms2osticks(500), read_func);
		break;
	case READ:
    // measure temperature
	    i2c_xfer_ex(SHT35_I2C_ADDR, sht35.buf, 0, 6, ms2osticks(500), job, read_func, sht35.pstatus);
	    break;
	case DONE:
	    if (*sht35.pstatus != I2C_OK) {
		goto error;
	    }
		uint8_t crc_t, crc_h;
		crc_t = crc8(sht35.buf, 2);
		crc_h = crc8(sht35.buf + 3, 2);
		if(crc_t != sht35.buf[2] || crc_h != sht35.buf[5])
			goto error;

		uint16_t t_raw, h_raw;
		t_raw = (sht35.buf[0]<<8) | sht35.buf[1];
		h_raw = (sht35.buf[3]<<8) | sht35.buf[4];

	    // parse temperature
	    // T = -45 + 175 * s / 2^16 - 1
	    sht35.pdata->T = -450 + (t_raw * 1750 >> 16);

	    // parse humidity
	    // H = 100 * s / 2^16 - 1
	    // sht35.pdata->H = (h_raw * 100) >> 16;
	    sht35.pdata->H = LSR_ROUND(h_raw * 100 ,16);
	    *sht35.pstatus = SHT35_OK;
	    goto done;
    }
    state += 1;
    return;

error:
    *sht35.pstatus = SHT35_ERROR;
done:
    state = INIT;
    os_setCallback(job, sht35.cb);
}

void sht35_read (osjob_t* job, osjobcb_t cb, int* pstatus, sht35_data_t* pdata) {
    sht35.cb = cb;
    sht35.pstatus = pstatus;
    sht35.pdata = pdata;
    os_setCallback(job, read_func);
}
