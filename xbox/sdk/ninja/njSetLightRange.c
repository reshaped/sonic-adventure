/* njSetLightRange: set l's near and far ranges, their squares in ltcal.nrr
 * and ltcal.frr, ltcal.idev to 1 / (frang - nrang) and ltcal.odev to frang
 * times that. make pc compiles this in place of
 * sdk/ninja/njSetLightRange.c, whose body is SH-4 assembly */
#include <stddef.h>

#include <ninja.h>

_Static_assert(offsetof(NJS_LIGHT, ltcal.nrr) == 0x68, "nrr");
_Static_assert(offsetof(NJS_LIGHT, ltcal.idev) == 0x78, "idev");
_Static_assert(offsetof(NJS_LIGHT, attr.nrang) == 0x134, "nrang");

void(njSetLightRange)(NJS_LIGHT *l, Float nrang, Float frang)
{
    Float inverse;

    l->attr.frang = frang;
    l->attr.nrang = nrang;
    l->ltcal.frr = frang * frang;
    l->ltcal.nrr = nrang * nrang;
    inverse = 1.0F / (frang - nrang);
    l->ltcal.idev = inverse;
    l->ltcal.odev = frang * inverse;
}
