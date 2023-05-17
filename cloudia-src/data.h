#ifndef _data_h_
#define _data_h_

#include "bme280.h"
#include "ina219.h"
#include "sht35.h"


typedef struct
{
    bme280_data_t bme280;
    sht35_data_t sht35;
    // ina219_data_t ina219;
    // TODO: add wind and others
} cloudia_t;


#endif //_data_h