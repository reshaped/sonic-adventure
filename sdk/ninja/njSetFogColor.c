#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

void njSetFogColor(Uint32 c)
{
    KMPACKEDARGB color;

    color.dwPacked = c;
    kmSetFogTableColor(color);
}
