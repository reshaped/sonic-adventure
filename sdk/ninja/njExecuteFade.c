#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

void njExecuteFade(Float f)
{
    NJS_FOG_TABLE fog;
    Int i;

    for (i = 0; i < FOG_TABLE_ENTRIES; i++) {
        fog[i] = f;
    }
    kmSetFogTable(fog);
}
