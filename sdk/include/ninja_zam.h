/* GCC's inline helpers for Ninja's matrix stack, transforms, vector math and
 * trig. zam_load_vec3, zam_fipr, zam_fipr_self and zam_squared_length serve
 * the tree's other GCC code too. this header also defines _builtin_fsrra and
 * _builtin_fsca, SHC's intrinsics for FSRRA and FSCA. ninja.h includes it
 * under GCC, below its prototypes.
 *
 * each instruction the helpers run that GCC doesn't compile from C comes
 * from SH4ZAM, but for zam_fipr_self's FIPR FV4,FV4, an asm of its own.
 *
 * a function's helper is named zam_ and the function's name. under GCC, the
 * function's own source defines it from the helper. for a function that C
 * calls inline, a function-like macro of its name follows the helper and
 * calls it. that helper keeps the function's type and contract.
 * ZAM_ASSERT_COMPATIBLE checks the type. the macro expands only where a (
 * follows the name. a pointer to the function, or its name in parentheses,
 * still reaches the definition, as a call from assembly does.
 *
 * njPushMatrix, njScale and njScaleV have no macro, so a C call reaches the
 * definition. inlined at the game's calls, each would add over 30 bytes a
 * call on average, to save the call's 5 to 8 instructions. njPopMatrix has
 * its macro only where a source compiles for speed: at -Os GCC leaves the
 * XMTRX load out of line, so there the macro would add bytes and still call.
 *
 * njCalcVector has no helper, so a C call reaches the shipped body. inlined
 * at the game's calls, a helper would add over 30 bytes a call on average.
 * where the caller keeps no value across the call, it would also run more
 * instructions than the call. out of line, it would run 28 instructions with
 * a null m and 47 with any other, where the shipped body runs 18 and 43.
 * these counts take _nj_matrix_flag_ clear, as the game keeps it.
 *
 * njMultiMatrix, njSetMatrix, njGetMatrix and njUnitMatrix have no helper
 * either. a helper of njGetMatrix would inline one of SH4ZAM's XMTRX entries
 * at each call. a helper of each of the other three would inline one at each
 * call that passes NULL as the first argument. inlined at the game's calls,
 * the four would add 55, 47, 34 and 29 bytes a call on average, to save the
 * call. the rotations, njOuterProduct, njSubVector and njAddVector have no
 * helper either.
 *
 * each FIPR takes its operands in the shipped body's order. -ffp-contract=on
 * fuses a product into an FMAC only where one expression sums it. no helper
 * sums a product, so an inlined helper fuses none under on, as under off */
#ifndef NINJA_ZAM_H
#define NINJA_ZAM_H
#include <zam.h>
#include <assert.h>
#include <stdint.h>
#include <sh4zam/shz_matrix.h>
#include <sh4zam/shz_trig.h>
#include <sh4zam/shz_vector.h>
#include <sh4zam/shz_xmtrx.h>

/* the matrix stack's room and depth, in matrices */
extern Sint32 _max_stack_size_;
extern Sint32 _stack_size_;

/* the helpers cast an NJS_MATRIX pointer to shz_mat4x4_t *. they cast a
 * pointer to an NJS_POINT3, or into a matrix, to shz_vec3_t * or
 * shz_vec4_t *. each SH4ZAM type must have the size of what it stands for,
 * and no greater alignment */
_Static_assert(sizeof(shz_mat4x4_t) == sizeof(NJS_MATRIX));
_Static_assert(_Alignof(shz_mat4x4_t) <= _Alignof(NJS_MATRIX));
_Static_assert(sizeof(shz_vec3_t) == sizeof(NJS_POINT3));
_Static_assert(_Alignof(shz_vec3_t) <= _Alignof(Float));
_Static_assert(sizeof(shz_vec4_t) == 4 * sizeof(Float));
_Static_assert(_Alignof(shz_vec4_t) <= _Alignof(Float));

/* p's x, y and z as an SH4ZAM vector. all three must be set. the helper
 * reads them as p's members, so where GCC can't prove one set, its warning
 * names the caller's variable. read through shz_vec3_deref, the warning
 * would name SH4ZAM's unnamed union by a number, which moves whenever a
 * declaration is added above */
ALWAYS_INLINE shz_vec3_t zam_load_vec3(const NJS_POINT3 *p)
{
    return shz_vec3_init(p->x, p->y, p->z);
}

/* p's x, y and z, with w as the fourth component */
ALWAYS_INLINE shz_vec4_t zam_load_vec4(const NJS_POINT3 *p, Float w)
{
    return shz_vec4_init(p->x, p->y, p->z, w);
}

/* store v in p's x, y and z */
ALWAYS_INLINE void zam_store_vec3(NJS_POINT3 *p, shz_vec3_t v)
{
    shz_vec3_deref(p) = v;
}

/* the FIPR of a and b, with a as FIPR's first operand. each caller passes
 * first the vector its shipped body's FIPR names first. the helper's
 * shz_vec4_dot2(a, b, b) runs FIPR FV4,FV8 of a and b, and then FIPR FV4,FV0
 * of a and b again. at -O2 or -Os GCC deletes the second, whose result
 * nothing reads. at -O0 the second would run, but no object compiled at -O0
 * calls this helper. at the SH4ZAM commit gcc.mk pins, shz_vec4_dot and
 * shz_vec3_dot are C multiply-adds, whose bits needn't match FIPR's */
ALWAYS_INLINE Float zam_fipr(shz_vec4_t a, shz_vec4_t b)
{
    return shz_vec4_dot2(a, b, b).x;
}

/* FIPR FV4,FV4 of (x, y, z, w) with itself, as the shipped njScalor and the
 * motion sources' shipped squared_length run it. the FIPR writes FR7 alone,
 * so x, y and z stay in FR4 to FR6 past it. through zam_fipr, which also
 * copies the vector to FV8, GCC would call each linear morph loop's blend of
 * a normal. through this, GCC inlines the blend */
ALWAYS_INLINE Float zam_fipr_self(Float x, Float y, Float z, Float w)
{
    register Float fr4 __asm__("fr4") = x;
    register Float fr5 __asm__("fr5") = y;
    register Float fr6 __asm__("fr6") = z;
    register Float square __asm__("fr7") = w;

    __asm__("fipr fv4,fv4" : "+f"(square) : "f"(fr4), "f"(fr5), "f"(fr6));
    return square;
}

/* the square of v's length: the FIPR of (v, 0) with itself */
ALWAYS_INLINE Float zam_squared_length(shz_vec3_t v)
{
    return zam_fipr_self(v.x, v.y, v.z, 0.0F);
}

/* _builtin_fsrra and zam_fsca run FSRRA and FSCA through SH4ZAM's SH-4 back
 * end, whatever the argument. under -frounding-math, SH4ZAM's public entries
 * take another path for a constant. shz_inv_sqrtf_fsrra computes a
 * constant's 1 / sqrt(|n|) by FSQRT and FDIV. shz_sincosu16 converts a
 * constant Angle to radians. GCC folds the sine and cosine of an Angle of 0.
 * for an Angle that is a power of two, GCC calls sinf and cosf, which the
 * tree doesn't define. for any other Angle, shz_sincosu16 converts the radians
 * back by FTRC and runs FSCA. under the game's rounding toward zero, that
 * FTRC can give the Angle less one, a 65536th of a turn short */
ALWAYS_INLINE Float _builtin_fsrra(Float n)
{
    return shz_inv_sqrtf_fsrra_sh4(n);
}

/* FSCA's sine and cosine of ang. FSCA reads only FPUL's low 16 bits, 65536
 * to a full turn, so the conversion to uint16_t changes no bit it reads. GCC
 * still writes an EXTU.W for the conversion ahead of some FSCAs */
ALWAYS_INLINE shz_sincos_t zam_fsca(Angle ang)
{
    return shz_sincosu16_sh4((uint16_t)ang);
}

ALWAYS_INLINE void _builtin_fsca(Angle ang, Float *sin, Float *cos)
{
    shz_sincos_t pair = zam_fsca(ang);

    *sin = pair.sin;
    *cos = pair.cos;
}

/* SH4ZAM's XMTRX entries take a slot of the matrix stack as an
 * shz_mat4x4_t, so each slot must be 8-byte aligned. every slot is:
 * src/main.c aligns matrix_stack to 32, and _nj_current_matrix_ptr_ moves by
 * whole matrices.
 *
 * the push loads m by single floats, where the shipped body loads pairs. so
 * m need only be 4-byte aligned, as a matrix inside a structure may be.
 * shz_xmtrx.h's entry for such a matrix, shz_xmtrx_load_unaligned_4x4, tests
 * m's alignment at run time and loads pairs where m is 8-byte aligned. the
 * push spends no test: it calls the load that entry takes otherwise,
 * shz_xmtrx_load_aligned4_4x4, which shz_xmtrx.h doesn't declare */
ALWAYS_INLINE Bool zam_njPushMatrix(NJS_MATRIX *m)
{
    NJS_MATRIX *top = _nj_current_matrix_ptr_;

    if (_stack_size_ >= _max_stack_size_) {
        return FALSE;
    }
    _stack_size_++;
    shz_xmtrx_store_4x4((shz_mat4x4_t *)top);
    _nj_current_matrix_ptr_ = top + 1;
    if (m == NULL) {
        shz_xmtrx_store_4x4((shz_mat4x4_t *)(top + 1));
    } else {
        shz_xmtrx_load_aligned4_4x4(*m);
    }
    return TRUE;
}

ALWAYS_INLINE Bool zam_njPopMatrix(Uint32 n)
{
    Uint32 depth = (Uint32)_stack_size_ - n;

    if (depth == 0) {
        return FALSE;
    }
    _stack_size_ = (Sint32)depth;
    _nj_current_matrix_ptr_ -= n;
    shz_xmtrx_load_4x4((const shz_mat4x4_t *)_nj_current_matrix_ptr_);
    return TRUE;
}
#ifndef __OPTIMIZE_SIZE__
ZAM_ASSERT_COMPATIBLE(njPopMatrix);
#define njPopMatrix(n) zam_njPopMatrix(n)
#endif

/* with m in memory, the shipped body runs four FIPRs. each takes
 * (x, y, z, 1) first and a column of m second, as SH4ZAM's transform of that
 * vector by m does. the helper calls the SH-4 back end's transform, which is
 * always_inline. the public shz_mat4x4_transform_vec4 forwards to it but is
 * only inline, so GCC may leave a call of it out of line.
 *
 * m must be 8-byte aligned, as an shz_mat4x4_t is. at the SH4ZAM commit
 * gcc.mk pins, the back end loads m by single floats. those take no address
 * error at 4, but a load by pairs would. a matrix outside a structure is
 * 8-byte aligned. of the structures, only NJS_VIEW and NJS_LIGHT keep theirs
 * 8-byte aligned, by njtypes.h's pack(8) */
ALWAYS_INLINE void zam_njTranslate(NJS_MATRIX *m, Float x, Float y, Float z)
{
    shz_vec4_t moved;

    if (m == NULL) {
        shz_xmtrx_translate(x, y, z);
        return;
    }
    assert((uintptr_t)m % 8 == 0);
    moved = shz_mat4x4_transform_vec4_sh4((const shz_mat4x4_t *)m,
                                          shz_vec4_init(x, y, z, 1.0F));
    shz_vec4_deref(&(*m)[12]) = moved;
}
ZAM_ASSERT_COMPATIBLE(njTranslate);
#define njTranslate(m, x, y, z) zam_njTranslate(m, x, y, z)

ALWAYS_INLINE void zam_njTranslateV(NJS_MATRIX *m, NJS_VECTOR *v)
{
    shz_vec3_t by = zam_load_vec3(v);

    zam_njTranslate(m, by.x, by.y, by.z);
}
ZAM_ASSERT_COMPATIBLE(njTranslateV);
#define njTranslateV(m, v) zam_njTranslateV(m, v)

/* m need only be 4-byte aligned: the helper takes its rows as shz_vec4_t,
 * which aligns to 4 */
ALWAYS_INLINE void zam_njScale(NJS_MATRIX *m, Float sx, Float sy, Float sz)
{
    if (m == NULL) {
        shz_xmtrx_scale(sx, sy, sz);
        return;
    }
    shz_vec4_deref(&(*m)[0]) = shz_vec4_scale(shz_vec4_deref(&(*m)[0]), sx);
    shz_vec4_deref(&(*m)[4]) = shz_vec4_scale(shz_vec4_deref(&(*m)[4]), sy);
    shz_vec4_deref(&(*m)[8]) = shz_vec4_scale(shz_vec4_deref(&(*m)[8]), sz);
}

ALWAYS_INLINE void zam_njScaleV(NJS_MATRIX *m, NJS_VECTOR *v)
{
    shz_vec3_t by = zam_load_vec3(v);

    zam_njScale(m, by.x, by.y, by.z);
}

/* as in zam_njTranslate, m must be 8-byte aligned, and the transform is the
 * back end's */
ALWAYS_INLINE void zam_njCalcPoint(NJS_MATRIX *m, NJS_POINT3 *ps,
                                   NJS_POINT3 *pd)
{
    shz_vec4_t point = zam_load_vec4(ps, 1.0F);

    if (m == NULL) {
        point = shz_xmtrx_transform_vec4(point);
    } else {
        assert((uintptr_t)m % 8 == 0);
        point = shz_mat4x4_transform_vec4_sh4((const shz_mat4x4_t *)m, point);
    }
    zam_store_vec3(pd, point.xyz);
}
ZAM_ASSERT_COMPATIBLE(njCalcPoint);
#define njCalcPoint(m, ps, pd) zam_njCalcPoint(m, ps, pd)

/* m need only be 4-byte aligned: the helper reads m[12] to m[14] as an
 * shz_vec3_t, which aligns to 4 */
ALWAYS_INLINE void zam_njGetTranslation(NJS_MATRIX *m, NJS_POINT3 *p)
{
    if (m == NULL) {
        zam_store_vec3(p, shz_xmtrx_get_translation());
    } else {
        zam_store_vec3(p, shz_vec3_deref(&(*m)[12]));
    }
}
ZAM_ASSERT_COMPATIBLE(njGetTranslation);
#define njGetTranslation(m, p) zam_njGetTranslation(m, p)

/* v2 first, as the shipped FIPR FV4,FV0 takes them */
ALWAYS_INLINE Float zam_njInnerProduct(NJS_VECTOR *v1, NJS_VECTOR *v2)
{
    return zam_fipr(zam_load_vec4(v2, 0.0F), zam_load_vec4(v1, 0.0F));
}
ZAM_ASSERT_COMPATIBLE(njInnerProduct);
#define njInnerProduct(v1, v2) zam_njInnerProduct(v1, v2)

ALWAYS_INLINE Float zam_njScalor(NJS_VECTOR *v)
{
    return __builtin_sqrtf(zam_squared_length(zam_load_vec3(v)));
}
ZAM_ASSERT_COMPATIBLE(njScalor);
#define njScalor(v) zam_njScalor(v)

ALWAYS_INLINE Float zam_njScalor2(NJS_VECTOR *v)
{
    return zam_squared_length(zam_load_vec3(v));
}
ZAM_ASSERT_COMPATIBLE(njScalor2);
#define njScalor2(v) zam_njScalor2(v)

/* the length returned is the squared length times FSRRA's 1 / sqrt of it, as
 * the shipped body computes it */
ALWAYS_INLINE Float zam_njUnitVector(NJS_VECTOR *v)
{
    shz_vec3_t unscaled = zam_load_vec3(v);
    Float squared = zam_squared_length(unscaled);
    Float scale = _builtin_fsrra(squared);

    zam_store_vec3(v, shz_vec3_scale(unscaled, scale));
    return squared * scale;
}
ZAM_ASSERT_COMPATIBLE(njUnitVector);
#define njUnitVector(v) zam_njUnitVector(v)

ALWAYS_INLINE Float zam_njSqrt(Float n)
{
    return n < 0.0F ? -__builtin_sqrtf(-n) : __builtin_sqrtf(n);
}
ZAM_ASSERT_COMPATIBLE(njSqrt);
#define njSqrt(n) zam_njSqrt(n)

ALWAYS_INLINE Float zam_njInvertSqrt(Float n)
{
    return n < 0.0F ? -_builtin_fsrra(-n) : _builtin_fsrra(n);
}
ZAM_ASSERT_COMPATIBLE(njInvertSqrt);
#define njInvertSqrt(n) zam_njInvertSqrt(n)

/* the shipped njSin and njCos run FSCA too, so a C call gets their bits */
ALWAYS_INLINE Float zam_njSin(Angle n)
{
    return zam_fsca(n).sin;
}
ZAM_ASSERT_COMPATIBLE(njSin);
#define njSin(n) zam_njSin(n)

ALWAYS_INLINE Float zam_njCos(Angle n)
{
    return zam_fsca(n).cos;
}
ZAM_ASSERT_COMPATIBLE(njCos);
#define njCos(n) zam_njCos(n)

ALWAYS_INLINE Float zam_njTan(Angle n)
{
    shz_sincos_t pair = zam_fsca(n);

    return pair.sin / pair.cos;
}
ZAM_ASSERT_COMPATIBLE(njTan);
#define njTan(n) zam_njTan(n)

#endif
