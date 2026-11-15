/* njFraction: n's fractional part, by modff, which leaves the whole part in
 * FracInt. make pc compiles this in place of sdk/ninja/njFraction.c, whose
 * body is a tail jump to modff in SH-4 assembly */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

Float FracInt;

Float(njFraction)(Float n)
{
    return (modff)(n, &FracInt);
}
