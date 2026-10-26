/* the fight's helpers: curves, angles, matrices, trees and splines */
#ifndef B_EGM1_UTIL_H
#define B_EGM1_UTIL_H
#include <ninja.h>
#include <sg_xpt.h>

struct angles;

/* a quadratic Bezier curve from start to end, pulled toward control */
struct curve {
    NJS_POINT3 start;
    NJS_POINT3 control;
    NJS_POINT3 end;
};

/* a natural cubic spline through count points, filled one of two ways.
 * spline_set_values makes a curve of values y over knots x, and param holds
 * its coefficients. spline_set_path makes a path through points x and y.
 * param then holds how far along the path each point lies, from 0 to 1.
 * x_coefficients and y_coefficients hold the coefficients for x and for y.
 * the path's z runs straight from start to end. nothing in B_EGM1 makes a
 * spline */
struct spline {
    Float *param;
    Float *x_coefficients;
    Float *y_coefficients;
    Float *x;
    Float *y;
    Float start;
    Float end;
    int count;
};

/* *out gets the curve's point at time, from 0 to 1 */
void curve_point(NJS_POINT3 *out, struct curve *curve, Float time);
/* the angle a vector rises above level, from atan2f of y and |z|, with x
 * unread. vector_rise returns it from -0x4000 to 0x4000, and
 * vector_rise_unsigned returns it mod 0x10000 */
Angle vector_rise(NJS_VECTOR *vector);
Angle vector_rise_unsigned(NJS_VECTOR *vector);
/* a vector's heading, from atan2f of x and z: 0 along +z, rising toward +x.
 * vector_yaw returns it signed, and vector_yaw_unsigned mod 0x10000 */
Angle vector_yaw(NJS_VECTOR *vector);
Angle vector_yaw_unsigned(NJS_VECTOR *vector);
/* vector_rise negated, as an x angle for njRotateX */
Angle vector_pitch(NJS_VECTOR *vector);
/* a random angle from -range / 2 up to range / 2 */
Angle random_spin(Angle range);
/* *ang gets matrix's turn as three angles. fun_0c901780 takes x from asinf
 * and fun_0c901820 takes y; in each, atan2f gives the other two. nothing in
 * B_EGM1 calls either */
void fun_0c901780(NJS_MATRIX *matrix, struct angles *ang);
void fun_0c901820(NJS_MATRIX *matrix, struct angles *ang);
/* the count of nodes in object's tree, its later siblings' trees included */
int tree_count_nodes(NJS_OBJECT *object);
void matrix_turn_zxy(NJS_MATRIX *matrix, struct angles *ang);
/* a spline of count points from the heap, which spline_free frees. no
 * allocation is checked */
struct spline *spline_create(int count);
void spline_free(struct spline *spline);
/* fill coefficients for a natural cubic spline of count values over knots.
 * both end coefficients are 0. count must be at least 3, and the knots must
 * rise */
void spline_solve(int count, const Float *knots, const Float *values,
                  Float *coefficients);
/* copy the spline's count knots and values into it, and solve its
 * coefficients into param */
void spline_set_values(struct spline *spline, const Float *knots,
                       const Float *values);
/* copy the path's count points into the spline. set param to how far along
 * the path each lies, and solve x's and y's coefficients. z runs straight
 * from start to end */
void spline_set_path(struct spline *spline, const Float *x, const Float *y,
                     Float start, Float end);
/* the value at time of the curve through values over knots, with
 * coefficients from spline_solve. a time outside the knots extends the
 * nearest end's segment */
Float spline_eval(int count, const Float *knots, const Float *values,
                  Float time, const Float *coefficients);
/* the value of a spline_set_values spline at time, along its knots */
Float spline_value(struct spline *spline, Float time);
/* *x, *y and *z get a spline_set_path spline's point at time, from 0 to 1 */
void spline_point(struct spline *spline, Float time, Float *x, Float *y,
                  Float *z);
void matrix_clear_translation(NJS_MATRIX *matrix);

#endif
