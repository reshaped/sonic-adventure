/* Conversions between Ninja points and SH4ZAM vectors. */
#ifndef NINJA_VEC3_H
#define NINJA_VEC3_H
#include <sh4zam/shz_vector.h>

/* p's x, y and z as an SH4ZAM vector. all three must be set. the helper
 * reads them as p's members, so where GCC can't prove one set, its warning
 * names the caller's variable. read through shz_vec3_deref, the warning
 * would name SH4ZAM's unnamed union by a number, which moves whenever a
 * declaration is added above */
ALWAYS_INLINE shz_vec3_t zam_load_vec3(const NJS_POINT3 *p)
{
    return shz_vec3_init(p->x, p->y, p->z);
}

/* store v in p's x, y and z */
ALWAYS_INLINE void zam_store_vec3(NJS_POINT3 *p, shz_vec3_t v)
{
    shz_vec3_deref(p) = v;
}

#endif
