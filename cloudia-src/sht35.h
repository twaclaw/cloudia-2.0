#ifndef _sht35_h_
#define _sht35_h_

#define SHT35_OK	0
#define SHT35_ERROR	-1

#define SHT35_SINGLE_HIGH_REP_CLOCK_STRETCH 0x2C06
#define SHT35_SINGLE_MEDIUM_REP_CLOCK_STRETCH 0x2C0D
#define SHT35_SINGLE_LOW_REP_CLOCK_STRETCH 0x2C10
#define SHT35_SINGLE_HIGH_REP_NO_CLOCK_STRETCH 0x2400
#define SHT35_SINGLE_MEDIUM_REP_NO_CLOCK_STRETCH 0x240B
#define SHT35_SINGLE_LOW_REP_NO_CLOCK_STRETCH 0x2416


typedef struct {
    int16_t T;
    uint8_t H;
} sht35_data_t;

void sht35_read (osjob_t* job, osjobcb_t cb, int* pstatus, sht35_data_t* pdata);

#endif
