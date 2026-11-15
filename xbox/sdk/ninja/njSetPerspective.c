/* njSetPerspective: set _nj_screen_.dist to _nj_screen_.w / (2 tan(ang / 2)).
 * make pc compiles this in place of sdk/ninja/njSetPerspective.c, whose
 * body is SH-4 assembly: a call of njTan, in parentheses so that it reaches
 * the function as the body's does and not a macro of its name, of ang
 * halved by SHAR, then the width over
 * the tangent's sum with itself */
#include <ninja.h>

void(njSetPerspective)(Angle ang)
{
    Float tangent = (njTan)(ang >> 1);

    _nj_screen_.dist = _nj_screen_.w / (tangent + tangent);
}
