#ifndef _conf_
#define _conf_

#include <inttypes.h>

#define CONF_R1_TEN (1)
#define CONF_R1_HEN (1 << 1)
#define CONF_R1_PEN (1 << 2)
#define CONF_R1_IEN (1 << 3)
#define CONF_R1_WEN (1 << 4)
#define CONF_R1_TRES_POS (5)
#define CONF_R1_TRES_MASK (1 << CONF_R1_TRES_POS)
#define CONF_R1_TRES_0DEC (0)
#define CONF_R1_TRES_1DEC (1 << CONF_R1_TRES_POS)
#define CONF_R1_TRES_2DEC (2 << CONF_R1_TRES_POS)

#define CONF_DIFFS_NEVER (0)
#define CONF_DIFFS_ALWAYS (1)
#define CONF_DIFFS_AUTO (2)

#define CONF_R2_IRES_POS (0)
#define CONF_R2_IRES_MASK (3 << CONF_R2_IRES_POS)
#define CONF_R2_IRES_9BIT (0 << CONF_R2_IRES_POS)
#define CONF_R2_IRES_10BIT (1 << CONF_R2_IRES_POS)
#define CONF_R2_IRES_11BIT (2 << CONF_R2_IRES_POS)
#define CONF_R2_IRES_12BIT (3 << CONF_R2_IRES_POS)

#define CONF_R2_TDIFFS_POS (2)
#define CONF_R2_HDIFFS_POS (4)
#define CONF_R2_PDIFFS_POS (6)
#define CONF_R2_TDIFFS_MASK (3 << CONF_R2_TDIFFS_POS)
#define CONF_R2_HDIFFS_MASK (3 << CONF_R2_HDIFFS_POS)
#define CONF_R2_PDIFFS_MASK (3 << CONF_R2_PDIFFS_POS)

#define CONF_R2_TDIFFS_ALWAYS (CONF_DIFFS_ALWAYS << CONF_R2_TDIFFS_POS)
#define CONF_R2_TDIFFS_AUTO (CONF_DIFFS_AUTO << CONF_R2_TDIFFS_POS)
#define CONF_R2_TDIFFS_NEVER (CONF_DIFFS_NEVER << CONF_R2_TDIFFS_POS)
#define CONF_R2_HDIFFS_ALWAYS (CONF_DIFFS_ALWAYS << CONF_R2_HDIFFS_POS)
#define CONF_R2_HDIFFS_AUTO (CONF_DIFFS_AUTO << CONF_R2_HDIFFS_POS)
#define CONF_R2_HDIFFS_NEVER (CONF_DIFFS_NEVER << CONF_R2_HDIFFS_POS)
#define CONF_R2_PDIFFS_ALWAYS (CONF_DIFFS_ALWAYS << CONF_R2_PDIFFS_POS)
#define CONF_R2_PDIFFS_AUTO (CONF_DIFFS_AUTO << CONF_R2_PDIFFS_POS)
#define CONF_R2_PDIFFS_NEVER (CONF_DIFFS_NEVER << CONF_R2_PDIFFS_POS)

#define CONF_OPT_I_NBITS_MASK (3 << 5)
#define CONF_OPT_I_NBITS_12 (0 << 5)
#define CONF_OPT_I_NBITS_11 (1 << 5)
#define CONF_OPT_I_NBITS_10 (2 << 5)
#define CONF_OPT_I_NBITS_9 (3 << 5)

#define CONF_PERIOD_MINS_MASK (1 << 7)
#define CONF_PERIOD_SECS_HRS_MASK (1 << 6)
#define CONF_PERIOD_MINS_VALUE_MASK (0x7F)
#define CONF_PERIOD_SECS_HRS_VALUE_MASK (0x3F)

//PORTS
#define CONF_PORT 144
typedef struct
{
    uint8_t r1;
    uint8_t r2;
    uint8_t r3; // period
    uint8_t r4; // buffer size
    uint8_t r5;
    // uint32_t thesholds[12];
    // uint16_t join_sched[10];
} conf_v1_t;

typedef conf_v1_t conf_t;

static const conf_t defaultcfg = {
    .r1 = CONF_R1_TEN |
          CONF_R1_HEN |
          CONF_R1_PEN |
          CONF_R1_TRES_1DEC,
    .r2 = CONF_R2_IRES_12BIT |
          CONF_R2_TDIFFS_AUTO |
          CONF_R2_HDIFFS_AUTO |
          CONF_R2_PDIFFS_AUTO,
    .r3 = 5,
    .r4 = 5};
#endif