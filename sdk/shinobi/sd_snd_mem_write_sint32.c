#include "shinobi/sd_work.h"
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdSndMemWriteSint32(Sint32 address, Sint32 value)
{
    if (address < SD_SND_MEM_SIZE) {
        syG2Write(&value, (void *)(SD_SND_MEM + address),
                  SYE_G2_DATA_WIDTH_32BIT, 1, SYE_G2_ADR_MODE_FIX,
                  SYE_G2_ADR_MODE_FIX);
    }
    return SDE_ERR_NOTHING;
}
