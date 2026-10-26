#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

/* set the fog table color to c, as njSetFogColor does: a fade is table fog, so
 * the two share one color */
void njSetFadeColor(Uint32 c)
{
    KMPACKEDARGB color;

    color.dwPacked = c;
    kmSetFogTableColor(color);
}
