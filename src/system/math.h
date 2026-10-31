/* vector and point math, angle lookups, color conversion and memory fills.
 * across means in the x-z plane: an _across function reads x and z and
 * ignores y. an angle is in Ninja's units, 65536 to a turn */
#ifndef SYSTEM_MATH_H
#define SYSTEM_MATH_H
#include <ninja.h>
#include <sg_xpt.h>

/* a . b, in 3D or across */
/* NOLINTBEGIN(readability-identifier-length) */
Float vector_inner_product(NJS_VECTOR *a, NJS_VECTOR *b);
Float vector_inner_product_across(NJS_VECTOR *a, NJS_VECTOR *b);
/* write a x b to out, which must be neither a nor b */
void vector_outer_product(NJS_VECTOR *a, NJS_VECTOR *b, NJS_VECTOR *out);
/* NOLINTEND(readability-identifier-length) */
/* vector's length, in 3D or across */
Float vector_magnitude(NJS_VECTOR *vector);
Float vector_magnitude_across(NJS_VECTOR *vector);
/* the square of vector's length, in 3D or across */
Float vector_magnitude_squared(NJS_VECTOR *vector);
Float vector_magnitude_squared_across(NJS_VECTOR *vector);
/* normalize vector into out and return the reciprocal of its length, as
 * FSRRA approximates it. the _across function normalizes x and z across and
 * leaves out->y as it was. neither checks for a zero length */
Float vector_unit(NJS_VECTOR *vector, NJS_VECTOR *out);
Float vector_unit_across(NJS_VECTOR *vector, NJS_VECTOR *out);
/* the cosine of the angle between a and b */
/* NOLINTNEXTLINE(readability-identifier-length) */
Float vector_cosine(NJS_VECTOR *a, NJS_VECTOR *b);

/* the distance between the points, and its square, in 3D or across */
Float point_distance(NJS_POINT3 *from, NJS_POINT3 *to);
Float point_distance_across(NJS_POINT3 *from, NJS_POINT3 *to);
Float point_distance_squared(NJS_POINT3 *from, NJS_POINT3 *to);
Float point_distance_squared_across(NJS_POINT3 *from, NJS_POINT3 *to);
/* project pos, in view space, onto the screen, from the screen's center.
 * out->x and out->y are pos->x and pos->y times the screen's distance and
 * Ninja's aspect for their axis, over pos->z. out->z is pos->z. nothing
 * checks pos->z for 0 */
void point_project_centered(NJS_POINT3 *pos, NJS_POINT3 *out);
/* write to out[0] and out[1] the x and y of (point->x, point->y, 0, 1)
 * times matrix. a NULL matrix takes XMTRX as it stands */
void point_calc_xy(NJS_MATRIX *matrix, NJS_POINT3 *point, Float *out);

/* the angle from the x axis to (x, y), from -0x8000 to 0x8000, or
 * 0x80000000 when both are 0 */
Angle angle_arc_tan2(Float y, Float x);
/* the angle whose sine is sine, from -0x4000 to 0x4000, interpolated in a
 * table, or for a negative sine extrapolated from the entry 256 * sine
 * truncates to and the next one up. sine must be from -1 to 1 */
Angle angle_arc_sin(Float sine);
/* the angle whose cosine is cosine, from 0 to 0x8000, interpolated in a
 * table, or for a negative cosine extrapolated from the entry 256 * cosine
 * truncates to and the next one up, but -0x8000 for -1. cosine must be from
 * -1 to 1 */
Angle angle_arc_cos(Float cosine);
/* sin(ang) / |cos(ang)|, as FSCA and FSRRA approximate them: tan(ang)
 * where cos(ang) is positive, and -tan(ang) where it is negative */
Float angle_sin_over_abs_cos(Angle ang);
/* x - y * trunc(x / y), or 0 when y is 0 */
Float float_mod(Float x, Float y);

/* each channel of color times by's, over 255 and rounded */
Uint32 color_multiply(Uint32 color, Uint32 by);
/* write color's channels to argb as floats from 0 to 255 */
void color_to_argb(Uint32 color, NJS_ARGB *argb);
/* the color with argb's channels, each truncated. a channel outside 0 to
 * 255 sets bits of the channels above it */
Uint32 color_from_argb(NJS_ARGB *argb);
/* write color's channels to argb as floats from 0 to 1 */
void color_to_argb_unit(Uint32 color, NJS_ARGB *argb);
/* the color with argb's channels, from 0 to 1, each times 255 and rounded.
 * a channel that rounds outside 0 to 255 sets bits of the channels above
 * it */
Uint32 color_from_argb_unit(NJS_ARGB *argb);

/* set the count bytes at mem to value's low byte. count must be positive:
 * 0 writes 2^32 bytes below mem */
void fill_bytes(void *mem, int value, int count);
/* set the size bytes at mem to value, a long at a time. size must be a
 * positive multiple of 32, and mem a multiple of 4 */
void fill_blocks(void *mem, Uint32 value, int size);

#ifdef GCC
/* under GCC, a C call of vector_inner_product_across,
 * vector_magnitude_across, vector_unit_across or point_calc_xy runs its
 * helper inline. as in ninja_zam.h, the helper is named zam_ and the
 * function's name, and a macro of the name calls it. math.src stays each
 * function's definition, which a function pointer still reaches. each
 * helper runs math.src's arithmetic on the same operands, but for the
 * registers its comment says a caller left. each FIPR takes its two vectors
 * in math.src's order.
 *
 * the other functions have no helper, so a C call reaches math.src. of
 * them, vector_inner_product, vector_magnitude, point_project_centered,
 * point_distance, point_distance_across, point_distance_squared and
 * point_distance_squared_across would each run no fewer instructions
 * inlined than the call. their bodies load a vector by post-increment,
 * where GCC's inlined loads compute each member's address */

/* v's x and z, with 0 for y, as math.src's _across bodies load them. like
 * them, it reads no y, which some callers leave unset */
ALWAYS_INLINE shz_vec3_t zam_load_across(const NJS_VECTOR *v)
{
    return shz_vec3_init(v->x, 0.0F, v->z);
}

/* b is FIPR's first operand, as in math.src's FIPR FV4,FV0. that FIPR also
 * multiplies the zeros in FR1 and FR3 by the FR5 and FR7 its caller left,
 * where this one multiplies 0 by 0. an infinity or a NaN in FR5 or FR7
 * makes math.src's result a NaN, and not the helper's */
ALWAYS_INLINE Float zam_vector_inner_product_across(NJS_VECTOR *a,
                                                    NJS_VECTOR *b)
{
    return zam_fipr(shz_vec3_vec4(zam_load_across(b), 0.0F),
                    shz_vec3_vec4(zam_load_across(a), 0.0F));
}
ZAM_ASSERT_COMPATIBLE(vector_inner_product_across);
#define vector_inner_product_across(a, b) zam_vector_inner_product_across(a, b)

ALWAYS_INLINE Float zam_vector_magnitude_across(NJS_VECTOR *vector)
{
    return __builtin_sqrtf(zam_squared_length(zam_load_across(vector)));
}
ZAM_ASSERT_COMPATIBLE(vector_magnitude_across);
#define vector_magnitude_across(vector) zam_vector_magnitude_across(vector)

ALWAYS_INLINE Float zam_vector_unit_across(NJS_VECTOR *vector, NJS_VECTOR *out)
{
    shz_vec3_t across = zam_load_across(vector);
    Float scale = _builtin_fsrra(zam_squared_length(across));

    out->x = across.x * scale;
    out->z = across.z * scale;
    return scale;
}
ZAM_ASSERT_COMPATIBLE(vector_unit_across);
#define vector_unit_across(vector, out) zam_vector_unit_across(vector, out)

/* with a matrix in memory, the helper runs the two FIPRs
 * shz_mat4x4_transform_point2 runs: the point (x, y, 0, 1) with
 * (*matrix)[0], [4], [8] and [12] for x, and with [1], [5], [9] and [13]
 * for y. it puts 0 in place of [8] and [9], which the point's 0 z
 * multiplies, and loads neither, as math.src doesn't. the point is each
 * FIPR's first operand, as in math.src's FIPR FV0,FV4. that FIPR multiplies
 * the point's 0 z by the FR6 its caller left, where this one multiplies 0
 * by 0. an infinity or a NaN in FR6 makes math.src's result a NaN, and not
 * the helper's */
ALWAYS_INLINE void zam_point_calc_xy(NJS_MATRIX *matrix, NJS_POINT3 *point,
                                     Float *out)
{
    shz_vec2_t xy = shz_vec2_init(point->x, point->y);
    shz_vec4_t at = shz_vec2_vec4(xy, 0.0F, 1.0F);

    if (matrix == NULL) {
        xy = shz_xmtrx_transform_point2(xy);
        out[0] = xy.x;
        out[1] = xy.y;
    } else {
        out[0] = zam_fipr(
            at, shz_vec4_init((*matrix)[0], (*matrix)[4], 0.0F, (*matrix)[12]));
        out[1] = zam_fipr(
            at, shz_vec4_init((*matrix)[1], (*matrix)[5], 0.0F, (*matrix)[13]));
    }
}
ZAM_ASSERT_COMPATIBLE(point_calc_xy);
#define point_calc_xy(matrix, point, out) zam_point_calc_xy(matrix, point, out)

#endif

#endif
