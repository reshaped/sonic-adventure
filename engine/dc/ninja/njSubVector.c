/* njSubVector on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njSubVector.c */
#include <ninja.h>

/* subtract vs from vd */
void njSubVector(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
    zam_store_vec3(vd, shz_vec3_sub(zam_load_vec3(vd), zam_load_vec3(vs)));
}
