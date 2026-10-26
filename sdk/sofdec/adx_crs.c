/* ADX critical sections, which raise the interrupt mask to IMASK_ALL and nest
 * to any depth */
#include <machine.h>
#include <sg_xpt.h>
#include <sofdec.h>

/* the lock nesting depth, and the interrupt mask from before the outermost
 * lock. an unlock too many leaves the depth at -1. from then until
 * ADXCRS_Init, an outermost lock raises it only to 0, and masks nothing */
Sint32 adxcrs_lvl = 0;
Sint32 adxcrs_msk = 0;

void ADXCRS_Init(void)
{
    adxcrs_lvl = 0;
}

void ADXCRS_Lock(void)
{
    if (adxcrs_lvl == 0) {
        adxcrs_msk = get_imask();
        set_imask(IMASK_ALL);
    }
    adxcrs_lvl++;
}

void ADXCRS_Unlock(void)
{
    adxcrs_lvl--;
    if (adxcrs_lvl == 0) {
        set_imask(adxcrs_msk);
    }
}
