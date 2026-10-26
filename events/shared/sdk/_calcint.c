/* an integer times 10, plus a digit */
#include <multiprec.h>
#include <string.h>

#include "shared/sdk/scan.h"

void _calcint(unsigned char *integer, int digit, int count)
{
    unsigned char times4[16];
    unsigned char *whole = integer;
    unsigned char *copy;
    int bytes;
    unsigned short sum;
    unsigned short i;

    copy = times4;
    /* the result has at most count + 1 digits, and bytes is enough to hold
     * them. integer and copy address that many low bytes from here on */
    bytes = (((count + 1) / 5) + 1) * 2;
    copy = copy + 8 - bytes;
    integer = integer + 8 - bytes;
    /* @hack redundant: a doubling of bytes that never runs, which SHC still
     * weighs. it puts i in R6 and the 0 that sum and digit take in R7 (MOV
     * #7,R6 at 0x0cb860f0 and MOV #0,R7 at 0x0cb860e8 in EV001E); without
     * it, the two trade and 6 of 74 words differ */
    if (0) {
        bytes += bytes;
    }
    memcpy(copy, integer, bytes);
    /* integer * 10, as (integer * 4 + integer) * 2 */
    _lsfts(copy, bytes, 2);
    _add(integer, copy, bytes);
    _lsft(integer, bytes);
    if (digit != 0) {
        sum = 0;
        /* @bug logic: i is unsigned, so i >= 0 always holds (EXTU.W R6,R3
         * and CMP/PZ R3 at 0x0cb86108 and 0x0cb8610c in EV001E), and only the
         * break ends the loop. a carry out of byte 0 would go on to whole[-1]:
         * the image walks whole with a pointer that steps down a byte a pass
         * (ADD #-1,R4 at 0x0cb86110). none comes, since the result fits in 8
         * bytes */
        for (i = 7; i >= 0; i--) {
            whole[i] = sum += whole[i] + digit;
            if (sum >= 256) {
                sum = 1;
                digit = 0;
            } else {
                break;
            }
        }
    }
}
