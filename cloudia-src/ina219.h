/*
  ____ _     ___  _   _ ____  _       
 / ___| |   / _ \| | | |  _ \(_) __ _
| |   | |  | | | | | | | | | | |/ _` |
| |___| |__| |_| | |_| | |_| | | (_| |
 \____|_____\___/ \___/|____/|_|\__,_|

 INA219 library
*/

#ifndef _ina219_h_
#define _ina219_h_

#define INA219_ADDR (0x40 << 1)
#define INA219_ERROR -2
#define INA219_I2C_TIMEOUT_MS ms2osticks(500)
#define INA219_BUFF_SIZE 5

#define INA219_REG_CONFIG 0x00
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIB 0x05

/*
 CONFIG
 Shunt ADC resolution average: bits 6:3
 {N}S: shunt samples averaged together
 */
#define INA219_CONFIG_SADCRES_9BIT_1S_84US (0)
#define INA219_CONFIG_SADCRES_10BIT_1S_148US (1 << 3)
#define INA219_CONFIG_SADCRES_11BIT_1S_276US (2 << 3)
#define INA219_CONFIG_SADCRES_12BIT_1S_532US (3 << 3)
#define INA219_CONFIG_SADCRES_12BIT_2S_1060US (9 << 3)
#define INA219_CONFIG_SADCRES_12BIT_4S_2130US (10 << 3)
#define INA219_CONFIG_SADCRES_12BIT_8S_4260US (11 << 3)
#define INA219_CONFIG_SADCRES_12BIT_16S_8510US (12 << 3)
#define INA219_CONFIG_SADCRES_12BIT_32S_17MS (13 << 3)
#define INA219_CONFIG_SADCRES_12BIT_64S_34MS (14 << 3)
#define INA219_CONFIG_SADCRES_12BIT_128S_69MS (15 << 3)

/*
CONFIG
Gain: PG1 PG0: bits 12:11
*/
#define INA219_CONFIG_GAIN_1_40MV 0

/*
CONFIG
Bus ADC: bits 10:7
*/
#define INA219_CONFIG_BVOLTRANGE_16V 0

/*
CONFIG
Mode: bits 2:0
S: Shunt
B: Bus
Triggered: single shot conversion
*/

#define INA219_CONFIG_MODE_POWERDOWN 0x00
#define INA219_CONFIG_MODE_SVOLT_TRIGGERED 0x01
#define INA219_CONFIG_MODE_BVOLT_TRIGGERED 0x02
#define INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED 0x03
#define INA219_CONFIG_MODE_ADCOFF 0x04
#define INA219_CONFIG_MODE_SVOLT_CONTINUOUS 0x05
#define INA219_CONFIG_MODE_BVOLT_CONTINUOUS 0x06
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS 0x07

typedef struct {
  uint16_t current;
}ina219_data_t;

void ina219_config(osjob_t *job, osjobcb_t cb, int *pstatus);
void ina219_read(osjob_t *job, osjobcb_t cb, int *pstatus, ina219_data_t *pdata);
#endif
