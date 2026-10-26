#include <ninja.h>

/* copy s's distance and center into _nj_screen_. the screen keeps its size:
 * s->w and s->h are not read */
void njSetScreen(NJS_SCREEN *s)
{
    _nj_screen_.dist = s->dist;
    _nj_screen_.cx = s->cx;
    _nj_screen_.cy = s->cy;
}
