#ifndef _bme280_h_
#define _bme280_h_

#include <inttypes.h>
#include "conf.h"

#define BME280_ADDR (0x76 << 1)
#define BME280_ERROR -1
#define BME280_I2C_TIMEOUT_MS ms2osticks(500)
#define BME280_CHIP_ID 0x60

//calib00..calib25 regs 0x88...0xA1
#define BME280_REG_DIG_T1 0x88
#define BME280_REG_DIG_T2 0x8A
#define BME280_REG_DIG_T3 0x8C
#define BME280_REG_DIG_P1 0x8E
#define BME280_REG_DIG_P2 0x90
#define BME280_REG_DIG_P3 0x92
#define BME280_REG_DIG_P4 0x94
#define BME280_REG_DIG_P5 0x96
#define BME280_REG_DIG_P6 0x98
#define BME280_REG_DIG_P7 0x9A
#define BME280_REG_DIG_P8 0x9C
#define BME280_REG_DIG_P9 0x9E
#define BME280_REG_DIG_H1 0xA1
#define BME280_REG_DIG_H2 0xE1
#define BME280_REG_DIG_H3 0xE3
#define BME280_REG_DIG_H4 0xE4
#define BME280_REG_DIG_H5 0xE5
#define BME280_REG_DIG_H6 0xE7

#define BME280_REG_CHIPID 0xD0
#define BME280_REG_VERSION 0xD1
#define BME280_REG_SOFTRESET 0xE0

//calib26..calib41 0xE1...0xF0
#define BME280_REG_CALIB26 0xE1

#define BME280_REG_HUMIDITY 0xFD
#define BME280_REG_TEMPERATURE 0xFA
#define BME280_REG_PRESSURE 0xF7
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_HUMIDITY 0xF2
#define BME280_REG_RESET 0xE0

enum
{
    BME280_CTRL_MEAS_SKIP = 0,
    BME280_CTRL_MEAS_OVERSMPL_1 = 1,
    BME280_CTRL_SLEEP_MODE = 0,
    BME280_CTRL_NORMAL_MODE = 3,
    BME280_CTRL_FORCE_MODE = 2,
    BME280_CONFIG_FILTER_OFF = 0
};

#define BME280_CALIB_NREGS 33
#define BME280_SENSOR_NREGS 8
#define BME280_BUFF_SIZE (BME280_CALIB_NREGS + 1)

typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib_t;

typedef struct
{
    uint32_t Tr;
    uint32_t Pr;
    uint16_t Hr;
} bme280_raw_data_t;

void bme280_config(osjob_t *job, osjobcb_t cb, int *pstatus, conf_t *pconf);
void bme280_read(osjob_t *job, osjobcb_t cb, int *pstatus, conf_t *pconf);
// void bme280_read(osjob_t *job, osjobcb_t cb, int *pstatus, sht20_data *pdata);

#endif
