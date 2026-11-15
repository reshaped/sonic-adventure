/* njSetLightAngle: set l's spotlight cone's inner and outer angles, the
 * squares of their cosines in ltcal.cosi and ltcal.cose, and both angles'
 * low 16 bits in ltcal.reserve, iang's in its high half. make pc compiles
 * this in place of sdk/ninja/njSetLightAngle.c, whose body is SH-4
 * assembly */
#include <stddef.h>

#include <ninja.h>

/* the body's offsets */
_Static_assert(offsetof(NJS_LIGHT, ltcal.cosi) == 0x70, "cosi");
_Static_assert(offsetof(NJS_LIGHT, ltcal.cose) == 0x74, "cose");
_Static_assert(offsetof(NJS_LIGHT, ltcal.reserve) == 0x8c, "reserve");
_Static_assert(offsetof(NJS_LIGHT, attr.iang) == 0x140, "iang");
_Static_assert(offsetof(NJS_LIGHT, attr.oang) == 0x144, "oang");

void(njSetLightAngle)(NJS_LIGHT *l, Angle iang, Angle oang)
{
    Float s;
    Float c;

    l->attr.iang = iang;
    l->attr.oang = oang;
    _builtin_fsca(iang, &s, &c);
    l->ltcal.cosi = c * c;
    _builtin_fsca(oang, &s, &c);
    l->ltcal.cose = c * c;
    l->ltcal.reserve = (Int)(((Uint32)l->attr.iang + 0x10000 & 0xffff) << 16 |
                             ((Uint32)l->attr.oang + 0x10000 & 0xffff));
}
