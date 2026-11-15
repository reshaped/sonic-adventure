/* njRoundOff: n rounded toward 0, by the runtime's floorf of |n|, negated
 * where n is below 0. make pc compiles this in place of
 * sdk/ninja/njRoundOff.c, whose body is SH-4 assembly. the call names
 * floorf in parentheses, so it reaches the runtime's, as the body's does,
 * and not mathf.h's GCC helper */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

Float(njRoundOff)(Float n)
{
    if (0.0F > n) {
        return -(floorf)(__builtin_fabsf(n));
    }
    return (floorf)(__builtin_fabsf(n));
}
