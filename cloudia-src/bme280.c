/*
  ____ _     ___  _   _ ____  _       
 / ___| |   / _ \| | | |  _ \(_) __ _ 
| |   | |  | | | | | | | | | | |/ _` |
| |___| |__| |_| | |_| | |_| | | (_| |
 \____|_____\___/ \___/|____/|_|\__,_|

 Bosch BME280 library
*/

#include "lmic.h"
#include "hw.h"
#include "bme280.h"

#define LEND_INT16(low, high) ((int16_t)((low) | (high << 8)))
#define LEND_UINT16(low, high) ((uint16_t)((low) | (high << 8)))

// Global variables
static bme280_calib_t _calib = {0};
static bme280_raw_data_t _raw_data = {0};
static int32_t T_high_res, T_high_res_adjust = 0; /* High resolution temperature used to compensate H and P*/

static struct
{
	uint8_t buf[BME280_BUFF_SIZE];
	osjobcb_t cb;
	int *pstatus;
	bme280_calib_t *pcalib;
	bme280_raw_data_t *praw_data;
	bme280_data_t *pdata;
	conf_t *pconf;
} bme280;

static void config_func(osjob_t *job)
{
	static enum {
		INIT,
		CONF_H,
		CONF_T_AND_P,
		CONF_FILTER,
		READ_CALIB_COEFFS,
		STORE_CALIB_COEFFS,
	} state;
	switch (state)
	{
	case INIT:
		bme280.buf[0] = BME280_REG_CHIPID;
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 1, 1, BME280_I2C_TIMEOUT_MS, job, config_func, bme280.pstatus);
		break;
	case CONF_H:
		if (*bme280.pstatus != I2C_OK || bme280.buf[0] != BME280_CHIP_ID)
		{
			goto config_error;
		}

		bme280.buf[0] = BME280_REG_CTRL_HUMIDITY; //reg 0xF2
		if ((bme280.pconf->options) & CONF_OPT_H_ENABLED_MASK)
		{
			bme280.buf[1] = BME280_CTRL_MEAS_OVERSMPL_1;
		}
		else
		{
			bme280.buf[1] = BME280_CTRL_MEAS_SKIP;
		}
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 2, 0, BME280_I2C_TIMEOUT_MS, job, config_func, bme280.pstatus);
		break;
	case CONF_T_AND_P:
		if (*bme280.pstatus != I2C_OK)
		{
			goto config_error;
		}
		uint8_t ctrl_meas = BME280_CTRL_SLEEP_MODE; // bits 1:0
		if ((bme280.pconf->options) & CONF_OPT_T_ENABLED_MASK)
		{
			ctrl_meas |= (BME280_CTRL_MEAS_OVERSMPL_1 << 5);
		}
		if ((bme280.pconf->options) & CONF_OPT_P_ENABLED_MASK)
		{
			ctrl_meas |= (BME280_CTRL_MEAS_OVERSMPL_1 << 2);
		}

		bme280.buf[0] = BME280_REG_CTRL_MEAS;
		bme280.buf[1] = ctrl_meas;
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 2, 0, BME280_I2C_TIMEOUT_MS, job, config_func, bme280.pstatus);
		break;
	case CONF_FILTER:
		if (*bme280.pstatus != I2C_OK)
		{
			goto config_error;
		}
		bme280.buf[0] = BME280_REG_CONFIG;
		bme280.buf[1] = BME280_CONFIG_FILTER_OFF;
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 2, 0, BME280_I2C_TIMEOUT_MS, job, config_func, bme280.pstatus);
		break;
	case READ_CALIB_COEFFS:
		if (*bme280.pstatus != I2C_OK)
		{
			goto config_error;
		}
		bme280.buf[0] = BME280_REG_DIG_T1;
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 1, BME280_CALIB_NREGS, BME280_I2C_TIMEOUT_MS, job, config_func, bme280.pstatus);
		break;
	case STORE_CALIB_COEFFS:
		if (*bme280.pstatus != I2C_OK)
		{
			goto config_error;
		}

		bme280.pcalib->dig_T1 = LEND_UINT16(bme280.buf[0], bme280.buf[1]);
		bme280.pcalib->dig_T2 = LEND_INT16(bme280.buf[2], bme280.buf[3]);
		bme280.pcalib->dig_T3 = LEND_INT16(bme280.buf[4], bme280.buf[5]);
		bme280.pcalib->dig_P1 = LEND_UINT16(bme280.buf[6], bme280.buf[7]);
		bme280.pcalib->dig_P2 = LEND_INT16(bme280.buf[8], bme280.buf[9]);
		bme280.pcalib->dig_P3 = LEND_INT16(bme280.buf[10], bme280.buf[11]);
		bme280.pcalib->dig_P4 = LEND_INT16(bme280.buf[12], bme280.buf[13]);
		bme280.pcalib->dig_P5 = LEND_INT16(bme280.buf[14], bme280.buf[15]);
		bme280.pcalib->dig_P6 = LEND_INT16(bme280.buf[16], bme280.buf[17]);
		bme280.pcalib->dig_P7 = LEND_INT16(bme280.buf[18], bme280.buf[19]);
		bme280.pcalib->dig_P8 = LEND_INT16(bme280.buf[20], bme280.buf[21]);
		bme280.pcalib->dig_P9 = LEND_INT16(bme280.buf[22], bme280.buf[23]);
		bme280.pcalib->dig_H1 = bme280.buf[24];
		bme280.pcalib->dig_H2 = LEND_INT16(bme280.buf[25], bme280.buf[26]);
		bme280.pcalib->dig_H3 = bme280.buf[27];
		bme280.pcalib->dig_H4 = LEND_INT16(bme280.buf[28], bme280.buf[29]);
		bme280.pcalib->dig_H5 = LEND_INT16(bme280.buf[30], bme280.buf[31]);
		bme280.pcalib->dig_H6 = bme280.buf[32];

		goto config_done;
	}

	state += 1;
	return;

config_error:
	*bme280.pstatus = BME280_ERROR;
config_done:
	state = INIT;
	os_setCallback(job, bme280.cb);
}

static void read_func(osjob_t *job)
{
	static enum {
		INIT,
		SET_MODE,
		MEAS_WAIT,
		MEAS_READY,
		MEAS_DECODE
	} state;
	switch (state)
	{
	case INIT:
		bme280.buf[0] = BME280_REG_CTRL_MEAS;
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 1, 1, BME280_I2C_TIMEOUT_MS, job, read_func, bme280.pstatus);
		break;
	case SET_MODE:
		bme280.buf[1] = bme280.buf[0] | BME280_CTRL_FORCE_MODE; // bits [1:0]
		bme280.buf[0] = BME280_REG_CTRL_MEAS;
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 2, 0, BME280_I2C_TIMEOUT_MS, job, read_func, bme280.pstatus);
		break;
	case MEAS_WAIT:
		if (*bme280.pstatus != I2C_OK)
		{
			goto read_error;
		}
		os_setApproxTimedCallback(job, os_getTime() + sec2osticks(1), read_func);
		break;

	case MEAS_READY:
		// TODO: check status register
		bme280.buf[0] = BME280_REG_PRESSURE;
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 1, BME280_SENSOR_NREGS, BME280_I2C_TIMEOUT_MS, job, read_func, bme280.pstatus);
		break;
	case MEAS_DECODE:
		if (*bme280.pstatus != I2C_OK)
		{
			goto read_error;
		}
		//pressure
		bme280.praw_data->P = bme280.buf[0];
		bme280.praw_data->P <<= 8;
		bme280.praw_data->P |= bme280.buf[1];
		bme280.praw_data->P <<= 8;
		bme280.praw_data->P |= bme280.buf[2];

		//temperature
		bme280.praw_data->T = bme280.buf[3];
		bme280.praw_data->T <<= 8;
		bme280.praw_data->T |= bme280.buf[4];
		bme280.praw_data->T <<= 8;
		bme280.praw_data->T |= bme280.buf[5];

		//humidity
		bme280.praw_data->H = bme280.buf[6];
		bme280.praw_data->H <<= 8;
		bme280.praw_data->H |= bme280.buf[7];

		bme280_compensate_T(bme280.praw_data, bme280.pcalib, bme280.pdata); //must be called first
		bme280_compensate_H(bme280.praw_data, bme280.pcalib, bme280.pdata);
		bme280_compensate_P(bme280.praw_data, bme280.pcalib, bme280.pdata);

		debug_printf("T:%d\tH:%d\tP:%d\r\n", bme280.pdata->T, bme280.pdata->H, bme280.pdata->P);
		goto read_done;
	}
	state += 1;
	return;
read_error:
	*bme280.pstatus = BME280_ERROR;

read_done:
	state = INIT;
	os_setCallback(job, bme280.cb);
}

void bme280_config(osjob_t *job, osjobcb_t cb, int *pstatus, conf_t *pconf)
{
	bme280.pcalib = &_calib;
	bme280.cb = cb;
	bme280.pstatus = pstatus;
	bme280.pconf = pconf;
	os_setCallback(job, config_func);
}

void bme280_read(osjob_t *job, osjobcb_t cb, int *pstatus, bme280_data_t *pdata, conf_t *pconf)
{
	bme280.praw_data = &_raw_data;
	bme280.cb = cb;
	bme280.pstatus = pstatus;
	bme280.pconf = pconf;
	bme280.pdata = pdata;
	os_setCallback(job, read_func);
}

int bme280_compensate_T(bme280_raw_data_t *raw_data, bme280_calib_t *calib, bme280_data_t *pdata)
{
	int32_t var1, var2;

	int32_t adc_T = raw_data->T;
	if (adc_T == 0x800000) // if T is disabled
		return -1;

	adc_T >>= 4;

	var1 = ((((adc_T >> 3) - ((int32_t)calib->dig_T1 << 1))) *
			((int32_t)calib->dig_T2)) >>
		   11;

	var2 = (((((adc_T >> 4) - ((int32_t)calib->dig_T1)) *
			  ((adc_T >> 4) - ((int32_t)calib->dig_T1))) >>
			 12) *
			((int32_t)calib->dig_T3)) >>
		   14;

	T_high_res = var1 + var2 + T_high_res_adjust;

	pdata->T = (T_high_res * 5 + 128) >> 8;
	return 0;
}

int bme280_compensate_P(bme280_raw_data_t *raw_data, bme280_calib_t *calib, bme280_data_t *pdata)
{
	/*
	Return pressure in Pascal
	pressure min = 30000
	pressure max = 110000

	bme280_compensate_T must be called first
	*/

	int64_t var1, var2, p;

	int32_t adc_P = raw_data->P;
	if (adc_P == 0x800000) // value in case pressure measurement was disabled
		return -1;

	adc_P >>= 4;

	var1 = ((int64_t)T_high_res) - 128000;
	var2 = var1 * var1 * (int64_t)calib->dig_P6;
	var2 = var2 + ((var1 * (int64_t)calib->dig_P5) << 17);
	var2 = var2 + (((int64_t)calib->dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)calib->dig_P3) >> 8) +
		   ((var1 * (int64_t)calib->dig_P2) << 12);
	var1 =
		(((((int64_t)1) << 47) + var1)) * ((int64_t)calib->dig_P1) >> 33;

	if (var1 == 0)
	{
		pdata->P = 0;
		return 0;
	}
	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (((int64_t)calib->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	var2 = (((int64_t)calib->dig_P8) * p) >> 19;

	p = ((p + var1 + var2) >> 8) + (((int64_t)calib->dig_P7) << 4);
	p >>= 8;
	pdata->P = (uint32_t)p;
	return 0;
}

int bme280_compensate_H(bme280_raw_data_t *raw_data, bme280_calib_t *calib, bme280_data_t *pdata)
{
	/*
    humidity_max = 100;
	bme280_compensate_T must be called first
	*/

	int32_t adc_H = raw_data->H;

	if (adc_H == 0x8000) // value in case humidity measurement was disabled
		return -1;

	int32_t v_x1_u32r;

	v_x1_u32r = (T_high_res - ((int32_t)76800));

	v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib->dig_H4) << 20) -
					(((int32_t)calib->dig_H5) * v_x1_u32r)) +
				   ((int32_t)16384)) >>
				  15) *
				 (((((((v_x1_u32r * ((int32_t)calib->dig_H6)) >> 10) *
					  (((v_x1_u32r * ((int32_t)calib->dig_H3)) >> 11) +
					   ((int32_t)32768))) >>
					 10) +
					((int32_t)2097152)) *
					   ((int32_t)calib->dig_H2) +
				   8192) >>
				  14));

	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
							   ((int32_t)calib->dig_H1)) >>
							  4));

	v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
	v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
	pdata->H = (uint32_t)(v_x1_u32r >> 12);
	pdata->H >>= 10;
	return 0;
}