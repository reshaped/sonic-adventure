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

#endif
