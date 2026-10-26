#include "shinobi/sd_work.h"
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* the word whose bytes in memory spell "SEGA" */
static Uint32 fill = 0x41474553;

SDE_ERR sdSndMemClear(void)
{
    sdARM7ResetKeep();
    syG2Write(&fill, (void *)SD_SND_MEM, SYE_G2_DATA_WIDTH_32BIT,
              SD_SND_MEM_SIZE / 4, SYE_G2_ADR_MODE_FIX, SYE_G2_ADR_MODE_ADD);
    return SDE_ERR_NOTHING;
}
