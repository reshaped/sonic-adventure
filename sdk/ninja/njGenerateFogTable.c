#include <ninja.h>
#include <sg_xpt.h>

/* fill fog so that the fog is in proportion to depth, 1 at entry 0's depth.
 * entry i stands for that depth divided by s = 2^(i / 16) * (16 + i % 16) / 16,
 * and holds 1 / s */
void njGenerateFogTable(NJS_FOG_TABLE fog)
{
    Int i;

    for (i = 0; i < FOG_TABLE_ENTRIES; i++) {
        fog[i] = 1.0F / ((Float)((1 << (i >> 4)) * ((i & 15) + 16)) / 16.0F);
    }
}
