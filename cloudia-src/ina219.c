
/*
  ____ _     ___  _   _ ____  _       
 / ___| |   / _ \| | | |  _ \(_) __ _
| |   | |  | | | | | | | | | | |/ _` |
| |___| |__| |_| | |_| | |_| | | (_| |
 \____|_____\___/ \___/|____/|_|\__,_|

 INA219 library

 The solar cell is always in short circuit; therefore, the voltage and power
 are not considered (they are always 0).
*/

#include "lmic.h"
#include "hw.h"
#include "ina219.h"

const uint16_t config_shunt_triggered = INA219_CONFIG_GAIN_1_40MV |
                                        INA219_CONFIG_BVOLTRANGE_16V |
                                        INA219_CONFIG_SADCRES_12BIT_1S_532US |
                                        INA219_CONFIG_MODE_SVOLT_TRIGGERED;

const uint16_t config_powerdown = INA219_CONFIG_GAIN_1_40MV |
                                  INA219_CONFIG_BVOLTRANGE_16V |
                                  INA219_CONFIG_SADCRES_12BIT_1S_532US |
                                  INA219_CONFIG_MODE_POWERDOWN;

static struct
{
    uint8_t buf[INA219_BUFF_SIZE];
    osjobcb_t cb;
    int *pstatus;
    ina219_data_t *pdata;
    //conf_t *pconf;
} ina219;

static void config_func(osjob_t *job)
{
    /*
    Verifies if the chip is present. 
    Sets the chip in sleep mode.
    */

    static enum {
        INIT,
        SLEEP_DONE,
    } state;

    switch (state)
    {
    case INIT:
        /*
        Set calibration to max resolution: 16V, 400mA.
        calibration value = 0x2000 (see datasheet)
        */
        ina219.buf[0] = INA219_REG_CONFIG;
        ina219.buf[1] = (config_powerdown >> 8) & 0xFF;
        ina219.buf[2] = config_powerdown & 0xFF;
        i2c_xfer_ex(INA219_ADDR, ina219.buf, 3, 0, INA219_I2C_TIMEOUT_MS, job, config_func, ina219.pstatus);
        break;
    case SLEEP_DONE:
        if (*ina219.pstatus != I2C_OK)
        {
            goto config_error;
        }
        goto config_done;
    }
    state += 1;
    return;

config_error:
    *ina219.pstatus = INA219_ERROR;
config_done:
    state = INIT;
    os_setCallback(job, ina219.cb);
}

static void read_func(osjob_t *job)
{
    static enum {
        INIT,
        CONF,
        READ_SHUNT,
        SLEEP,
        DONE
    } state;

    switch (state)
    {
    case INIT:
        /*
        Set calibration to max resolution: 16V, 400mA.
        calibration value = 0x2000 (See datasheet)
        */
        ina219.buf[0] = INA219_REG_CALIB;
        ina219.buf[1] = 0x20;
        ina219.buf[2] = 0x00;
        i2c_xfer_ex(INA219_ADDR, ina219.buf, 3, 0, INA219_I2C_TIMEOUT_MS, job, config_func, ina219.pstatus);
        break;
    case CONF:
        if (*ina219.pstatus != I2C_OK)
        {
            goto read_error;
        }
        ina219.buf[0] = INA219_REG_CONFIG;
        ina219.buf[1] = (config_shunt_triggered >> 8) & 0xFF;
        ina219.buf[2] = config_shunt_triggered & 0xFF;
        i2c_xfer_ex(INA219_ADDR, ina219.buf, 3, 0, INA219_I2C_TIMEOUT_MS, job, config_func, ina219.pstatus);
        break;
    case READ_SHUNT:
        if (*ina219.pstatus != I2C_OK)
        {
            goto read_error;
        }
        ina219.buf[0] = INA219_REG_CURRENT;
        i2c_xfer_ex(INA219_ADDR, ina219.buf, 1, 2, INA219_I2C_TIMEOUT_MS, job, config_func, ina219.pstatus);
        break;
    case SLEEP:
        if (*ina219.pstatus != I2C_OK)
        {
            goto read_error;
        }
        // TODO: get current value
        debug_printf("Current: high: 0x%x, low: 0x%x", ina219.buf[0], ina219.buf[1]);
        // ina219.current = ...
        // current divider = 20
        ina219.buf[0] = INA219_REG_CONFIG;
        ina219.buf[1] = (config_powerdown >> 8) & 0xFF;
        ina219.buf[2] = config_powerdown & 0xFF;
        i2c_xfer_ex(INA219_ADDR, ina219.buf, 3, 0, INA219_I2C_TIMEOUT_MS, job, config_func, ina219.pstatus);

        break;
    case DONE:
        if (*ina219.pstatus != I2C_OK)
        {
            goto read_error;
        }
        goto read_done;
    }

    state += 1;
    return;

read_error:
    *ina219.pstatus = INA219_ERROR;
read_done:
    state = INIT;
    os_setCallback(job, ina219.cb);
}
void ina219_config(osjob_t *job, osjobcb_t cb, int *pstatus)
{
    ina219.cb = cb;
    ina219.pstatus = pstatus;
    os_setCallback(job, config_func);
}
void ina219_read(osjob_t *job, osjobcb_t cb, int *pstatus, ina219_data_t *pdata)
{
    ina219.cb = cb;
    ina219.pstatus = pstatus;
    ina219.pdata = pdata;
	os_setCallback(job, read_func);
}

