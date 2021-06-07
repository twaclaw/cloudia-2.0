#include "conf.h"

void default_conf(conf_t * conf)
{    conf->r1 = CONF_R1_TEN |
               CONF_R1_HEN |
               CONF_R1_PEN |
               CONF_R1_IEN |
               CONF_R1_TRES_1DEC;

    conf->r2 = CONF_R2_IRES_12BIT |
               CONF_R2_TDIFFS_AUTO |
               CONF_R2_HDIFFS_AUTO |
               CONF_R2_PDIFFS_AUTO;

    conf->r3 = 0;
    conf->r4 = 1;
}