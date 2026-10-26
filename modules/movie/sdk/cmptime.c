/* Sofdec time comparison, each time in its own unit */
#include <sg_xpt.h>

#include "movie/sdk/sfd.h"

/* for positive units, time1 / tunit1 <= time2 / tunit2 where time1 * tunit2
 * <= time2 * tunit1, which takes no division. UTY_CmpLeSint64 compares the
 * 64-bit products, so where both are negative and share a high word, their
 * low words compare in reverse */
Bool UTY_CmpTime(Sint32 time1, Sint32 tunit1, Sint32 time2, Sint32 tunit2)
{
    Bool result;
    struct uty_sint64 prod1;
    struct uty_sint64 prod2;

    UTY_Dmuls(time1, tunit2, &prod1);
    UTY_Dmuls(time2, tunit1, &prod2);
    /* @hack spelling: UTY_CmpLeSint64's result through result, which makes
     * the call a JSR (0x8ced51a0); returned straight, SHC jumps to it and 4
     * of 26 words differ */
    result = UTY_CmpLeSint64(&prod1, &prod2);
    return result;
}
