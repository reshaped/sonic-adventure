/* a float from the decimal digits _fmtin reads */
#include <string.h>

#include "shared/sdk/scan.h"

float _stf(int sign, const char *digits, int exponent, char range_errno)
{
    unsigned char integer[8];
    int last;
    int i;
    float value;

    memset(integer, 0, sizeof(integer));
    exponent -= SCAN_DIGITS - 1;
    /* trailing zeros only raise the exponent */
    for (last = SCAN_DIGITS - 1; digits[last] == '0'; last--, exponent++) {
    }
    for (i = 0; i <= last; i++) {
        _calcint(integer, digits[i] - '0', i);
    }
    value = _calcfpow(integer, exponent, range_errno);
    /* @hack spelling: the product through value, which SHC forms in FR4 and
     * moves to FR0 (FMOV FR0,FR4 and FMOV FR4,FR0 at 0x0cb85ea2 and
     * 0x0cb85eaa in EV001E); returned as value * sign, 54 of 63 words differ */
    value *= sign;
    return value;
}
