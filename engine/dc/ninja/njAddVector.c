/* njAddVector on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njAddVector.c */
#include <ninja.h>

/* add vs to vd */
void njAddVector(NJS_VECTOR *vd, NJS_VECTOR *vs)
{
    zam_store_vec3(vd, shz_vec3_add(zam_load_vec3(vs), zam_load_vec3(vd)));
}
