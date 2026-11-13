/* njCalcVector on SH4ZAM. make dc compiles this in place of
 * sdk/ninja/njCalcVector.c */
#include <ninja.h>

ALWAYS_INLINE void unit_store(shz_vec3_t v, NJS_VECTOR *vd)
{
    if (_nj_matrix_flag_) {
        v = shz_vec3_scale(v, _builtin_fsrra(zam_squared_length(v)));
    }
    zam_store_vec3(vd, v);
}

/* transform vs by m in memory with SH4ZAM's three FIPRs, one for each
 * column. each takes (vs, 0) first and the column, with 0 for its fourth, as
 * the shipped body does. SH4ZAM's routine takes an shz_mat4x4_t, so m must
 * be 8-byte aligned, as njtypes.h aligns an NJS_MATRIX. FTRV transforms a
 * vector register in place. keeping this out of line lets the null m's path
 * load vs there and store vd from there */
static void __attribute__((noinline))
transform_by(NJS_MATRIX *m, NJS_VECTOR *vs, NJS_VECTOR *vd)
{
    assert((uintptr_t)m % 8 == 0);
    unit_store(
        shz_mat4x4_transform_vec3((const shz_mat4x4_t *)m, shz_vec3_deref(vs)),
        vd);
}

/* vd = (vs, 0) times m, made unit length, to FSRRA's precision, while
 * _nj_matrix_flag_ is set. a null m selects the current matrix, in XMTRX. vd
 * may be vs */
void njCalcVector(NJS_MATRIX *m, NJS_VECTOR *vs, NJS_VECTOR *vd)
{
    if (m != NULL) {
        transform_by(m, vs, vd);
        return;
    }
    unit_store(shz_xmtrx_transform_vec4(zam_load_vec4(vs, 0.0F)).xyz, vd);
}
