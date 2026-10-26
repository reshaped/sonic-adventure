#include <ninja.h>
#include <sg_xpt.h>

/* the signature ninja.h declares, with attr unread and idata not const */
/* NOLINTNEXTLINE(misc-unused-parameters,readability-non-const-parameter) */
void njLinear(Float *idata, Float *odata, NJS_SPLINE *attr, Float frame)
{
    Float rest = 1.0F - frame;

    odata[0] = (idata[0] * rest) + (idata[3] * frame);
    odata[1] = (idata[1] * rest) + (idata[4] * frame);
    odata[2] = (idata[2] * rest) + (idata[5] * frame);
}
