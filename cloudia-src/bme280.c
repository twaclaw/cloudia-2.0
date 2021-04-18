#include "lmic.h"
#include "hw.h"
#include "bme280.h"

#define LEND_INT16(low, high) ((int16_t)((low) | (high << 8)))
#define LEND_UINT16(low, high) ((uint16_t)((low) | (high << 8)))

static struct
{
	uint8_t buf[BME280_BUFF_SIZE];
	osjobcb_t cb;
	int *pstatus;
	bme280_calib_t *pcalib;
	bme280_raw_data_t *pdata;
	conf_t *pconf;
} bme280;

bme280_calib_t _calib = {0};

static void config_func(osjob_t *job)
{
	bme280.pcalib = &_calib;
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
		i2c_xfer_ex(BME280_ADDR, bme280.buf, 1, BME280_N_CALIB_U8, BME280_I2C_TIMEOUT_MS, job, config_func, bme280.pstatus);
		break;
	case STORE_CALIB_COEFFS:
		if (*bme280.pstatus != I2C_OK)
		{
			goto config_error;
		}

		int i = 0;
		bme280.pcalib->dig_T1 = LEND_UINT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_T2 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_T3 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P1 = LEND_UINT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P2 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P3 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P4 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P5 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P6 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P7 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P8 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_P9 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_H1 = bme280.buf[i++];
		bme280.pcalib->dig_H2 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_H3 = bme280.buf[i++];
		bme280.pcalib->dig_H4 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_H5 = LEND_INT16(bme280.buf[i++], bme280.buf[i++]);
		bme280.pcalib->dig_H6 = bme280.buf[i++];

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

// int i;
// static void read_func(osjob_t *job)
// {
// 	static enum {
// 		INIT,
// 		CALIB_DATA_DONE
// 	} state;
// 	switch (state)
// 	{
// 	case INIT:
// 		debug_printf("INIT\n");
// 		// buf[0] = BME280_REG_CTRL_MEAS;
// 		buf[0] = BME280_REG_CTRL_MEAS;
// 		// buf[1] = 1 << 2;
// 		i2c_xfer_ex(BME280_ADDR, buf, 1, 1, ms2osticks(1500), job, read_func, status);
// 		break;
// 	case CALIB_DATA_DONE:
// 		debug_printf("CALIB_DATA_DONE\n");
// 		for (i=0;i<2;i++)
// 			debug_printf("0x%x\r\n", buf[i]);
// 		break;
// 	}
// 	state += 1;
// }

void bme280_config(osjob_t *job, osjobcb_t cb, int *pstatus, conf_t *pconf)
{
	bme280.cb = cb;
	bme280.pstatus = pstatus;
	bme280.pconf = pconf;
	os_setCallback(job, config_func);
}
