#ifndef _conf_
#define _conf_

#include <inttypes.h>

#define CONF_OPT_T_ENABLED_MASK (1)
#define CONF_OPT_H_ENABLED_MASK (1 << 1)
#define CONF_OPT_P_ENABLED_MASK (1 << 2)
#define CONF_OPT_I_ENABLED_MASK (1 << 3)
#define CONF_OPT_T_HIGH_RES_MASK (1 << 4)

#define CONF_OPT_I_NBITS_MASK (3 << 5)
#define CONF_OPT_I_NBITS_12 (0 << 5)
#define CONF_OPT_I_NBITS_11 (1 << 5)
#define CONF_OPT_I_NBITS_10 (2 << 5)
#define CONF_OPT_I_NBITS_9 (3 << 5)

#define CONF_PERIOD_MINS_MASK (1 << 7)
#define CONF_PERIOD_SECS_HRS_MASK (1 << 6)
#define CONF_PERIOD_MINS_VALUE_MASK (0x7F)
#define CONF_PERIOD_SECS_HRS_VALUE_MASK (0x3F)

typedef struct
{
    uint8_t options;
    uint8_t period;
    uint8_t buffer_size;
} conf_t;

#endif