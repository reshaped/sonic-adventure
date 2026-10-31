/* holding the AICA's ARM7 in reset */
#include "shinobi/sd_work.h"
#include <machine.h>
#include <sg_syg2.h>
#include <sg_xpt.h>
#include <shinobi.h>

SDE_ERR sdARM7ResetKeep(void)
{
    /* @hack spelling: control, static in the function, whose address SHC loads
     * into R3 and copies to syG2Write's first argument (MOV R3,R4 at
     * 0x8c65da0e). at file scope, SHC loads it into R4 directly and 23 of 52
     * words differ */
    static Uint32 control;
    Sint32 mask;
    Sint32 delay;

    mask = get_imask();
    set_imask(IMASK_ALL);
    syG2Read(ARM_CONTROL, &control, SYE_G2_DATA_WIDTH_32BIT, 1,
             SYE_G2_ADR_MODE_ADD, SYE_G2_ADR_MODE_ADD);
    control |= ARM_RESET;
    syG2Write(&control, ARM_CONTROL, SYE_G2_DATA_WIDTH_32BIT, 1,
              SYE_G2_ADR_MODE_ADD, SYE_G2_ADR_MODE_ADD);
    set_imask(mask);
    for (delay = ARM_RESET_DELAY; delay != 0; delay--) {
#ifdef GCC
        /* GCC deletes an empty loop. the asm keeps each pass, so the wait
         * takes as long as the shipped loop */
        __asm__ volatile("");
#endif
    }
    return SDE_ERR_NOTHING;
}
