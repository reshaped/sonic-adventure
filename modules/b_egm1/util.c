/* the fight's helpers: curves, angles, matrices, trees and splines */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm1/util.h"
#include "system/task.h"
#include "system/util.h"

void curve_point(NJS_POINT3 *out, struct curve *curve, Float time)
{
    NJS_POINT3 *control = &curve->control;
    NJS_POINT3 *end = &curve->end;
    Float rest = 1.0F - time;
    Float middle = time * rest * 2.0F;
    Float last = time * time;
    Float first = rest * rest;

    out->x = (curve->start.x * first) + (control->x * middle) + (end->x * last);
    out->y = (curve->start.y * first) + (control->y * middle) + (end->y * last);
    out->z = (curve->start.z * first) + (control->z * middle) + (end->z * last);
}

Angle vector_rise(NJS_VECTOR *vector)
{
    return (Angle)(atan2f(vector->y, _builtin_fabsf(vector->z)) *
                   ANGLE_PER_RADIAN);
}

Angle vector_rise_unsigned(NJS_VECTOR *vector)
{
    Angle ang = vector_rise(vector);

    return (Uint16)ang;
}

Angle vector_yaw(NJS_VECTOR *vector)
{
    return (Angle)(atan2f(vector->x, vector->z) * ANGLE_PER_RADIAN);
}

Angle vector_yaw_unsigned(NJS_VECTOR *vector)
{
    Angle ang = vector_yaw(vector);

    return (Uint16)ang;
}

Angle vector_pitch(NJS_VECTOR *vector)
{
    return -(Angle)(atan2f(vector->y, _builtin_fabsf(vector->z)) *
                    ANGLE_PER_RADIAN);
}

Angle random_spin(Angle range)
{
    return (Angle)(((Float)rand() * range * RAND_SCALE) - (range * 0.5F));
}

/* @hack spelling: cells 10 and 5 multiplied by -1.0F, which SHC writes as
 * a multiply by 1.0 and an FNEG (for cell 10, FLDI1 at 0x0c90179c and FNEG at
 * 0x0c9017ae); negated with a minus, 37 of 49 words differ */
void fun_0c901780(NJS_MATRIX *matrix, struct angles *ang)
{
    Float *cell = *matrix;

    ang->x = (Angle)(asinf(cell[9]) * ANGLE_PER_RADIAN);
    ang->y = (Angle)(atan2f(cell[8], cell[10] * -1.0F) * ANGLE_PER_RADIAN);
    ang->z = (Angle)(atan2f(cell[1], cell[5] * -1.0F) * ANGLE_PER_RADIAN);
}

/* @hack spelling: cells 10 and 0 multiplied by -1.0F, which SHC writes as
 * a multiply by 1.0 and an FNEG (for cell 10, FLDI1 at 0x0c90182c and FNEG at
 * 0x0c90183e); negated with a minus, 43 of 48 words differ */
void fun_0c901820(NJS_MATRIX *matrix, struct angles *ang)
{
    Float *cell = *matrix;

    ang->x = (Angle)(atan2f(cell[6], cell[10] * -1.0F) * ANGLE_PER_RADIAN);
    ang->y = (Angle)(asinf(cell[2]) * ANGLE_PER_RADIAN);
    ang->z = (Angle)(atan2f(cell[1], cell[0] * -1.0F) * ANGLE_PER_RADIAN);
}

int tree_count_nodes(NJS_OBJECT *object)
{
    int count = 1;

    if (object->child != NULL) {
        count += tree_count_nodes(object->child);
    }
    if (object->sibling != NULL) {
        count += tree_count_nodes(object->sibling);
    }
    return count;
}

void matrix_turn_zxy(NJS_MATRIX *matrix, struct angles *ang)
{
    TURN_MATRIX(njRotateZ, matrix, ang->z);
    TURN_MATRIX(njRotateX, matrix, ang->x);
    TURN_MATRIX(njRotateY, matrix, ang->y);
}

/* @hack type: each array's size as count * (int)sizeof(Float), an int,
 * which SHC shifts once into R12 for all five calls (SHLL2 at 0x0c901908);
 * as sizeof's unsigned, 28 of 36 words differ */
struct spline *spline_create(int count)
{
    struct spline *spline = heap_alloc(sizeof(struct spline));

    spline->param = heap_alloc(count * (int)sizeof(Float));
    spline->x_coefficients = heap_alloc(count * (int)sizeof(Float));
    spline->y_coefficients = heap_alloc(count * (int)sizeof(Float));
    spline->x = heap_alloc(count * (int)sizeof(Float));
    spline->y = heap_alloc(count * (int)sizeof(Float));
    spline->count = count;
    return spline;
}

void spline_free(struct spline *spline)
{
    heap_free(spline->param);
    heap_free(spline->x_coefficients);
    heap_free(spline->y_coefficients);
    heap_free(spline->x);
    heap_free(spline->y);
    heap_free(spline);
}

/* @hack type: each array's size as count * (int)sizeof(Float), an int,
 * which SHC shifts once and keeps on the stack for both calls (SHLL2 at
 * 0x0c9019b4); as sizeof's unsigned, 43 of 161 words differ */
void spline_solve(int count, const Float *knots, const Float *values,
                  Float *coefficients)
{
    Float *diagonal = heap_alloc(count * (int)sizeof(Float));
    Float *intervals = heap_alloc(count * (int)sizeof(Float));
    Float ratio;
    int i;
    int last;

    last = count - 1;
    coefficients[0] = coefficients[last] = 0.0F;
    for (i = 0; i < last; i++) {
        intervals[i] = knots[i + 1] - knots[i];
        diagonal[i + 1] = (values[i + 1] - values[i]) / intervals[i];
    }
    coefficients[1] = diagonal[2] - diagonal[1];
    diagonal[1] = 2.0F * (knots[2] - knots[0]);
    last = count - 2;
    for (i = 1; i < last; i++) {
        ratio = intervals[i] / diagonal[i];
        coefficients[i + 1] =
            diagonal[i + 2] - diagonal[i + 1] - (ratio * coefficients[i]);
        diagonal[i + 1] =
            (2.0F * (knots[i + 2] - knots[i])) - (ratio * intervals[i]);
    }
    coefficients[last] -= coefficients[last + 1] * intervals[last];
    for (i = last; i > 0; i--) {
        coefficients[i] =
            (coefficients[i] - (coefficients[i + 1] * intervals[i])) /
            diagonal[i];
    }
    heap_free(diagonal);
    heap_free(intervals);
}

void spline_set_values(struct spline *spline, const Float *knots,
                       const Float *values)
{
    int i;

    for (i = 0; i < spline->count; i++) {
        spline->x[i] = knots[i];
        spline->y[i] = values[i];
    }
    spline_solve(spline->count, spline->x, spline->y, spline->param);
}

void spline_set_path(struct spline *spline, const Float *x, const Float *y,
                     Float start, Float end)
{
    Float *distance = spline->param;
    Float *path_x = spline->x;
    Float *path_y = spline->y;
    Float dx;
    Float dy;
    int i;

    for (i = 0; i < spline->count; i++) {
        path_x[i] = x[i];
        path_y[i] = y[i];
    }
    distance[0] = 0.0F;
    for (i = 1; i < spline->count; i++) {
        dx = path_x[i] - path_x[i - 1];
        dy = path_y[i] - path_y[i - 1];
        distance[i] = distance[i - 1] + njSqrt((dx * dx) + (dy * dy));
    }
    for (i = 1; i < spline->count; i++) {
        distance[i] /= distance[spline->count - 1];
    }
    spline_solve(spline->count, distance, path_x, spline->x_coefficients);
    spline_solve(spline->count, distance, path_y, spline->y_coefficients);
    spline->start = start;
    spline->end = end;
}

Float spline_eval(int count, const Float *knots, const Float *values,
                  Float time, const Float *coefficients)
{
    int low = 0;
    int high = count - 1;
    int middle;
    Float width;

    while (low < high) {
        middle = (low + high) / 2;
        if (knots[middle] < time) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    if (low > 0) {
        low--;
    }
    width = knots[low + 1] - knots[low];
    time -= knots[low];
    return (((((((coefficients[low + 1] - coefficients[low]) * time) / width) +
               (3.0F * coefficients[low])) *
              time) -
             (((2.0F * coefficients[low]) + coefficients[low + 1]) * width) +
             ((values[low + 1] - values[low]) / width)) *
            time) +
           values[low];
}

Float spline_value(struct spline *spline, Float time)
{
    return spline_eval(spline->count, spline->x, spline->y, time,
                       spline->param);
}

void spline_point(struct spline *spline, Float time, Float *x, Float *y,
                  Float *z)
{
    *x = spline_eval(spline->count, spline->param, spline->x, time,
                     spline->x_coefficients);
    *y = spline_eval(spline->count, spline->param, spline->y, time,
                     spline->y_coefficients);
    *z = spline->start + ((spline->end - spline->start) * time);
}

void matrix_clear_translation(NJS_MATRIX *matrix)
{
    Float *cell = *matrix;

    cell[12] = 0.0F;
    cell[13] = 0.0F;
    cell[14] = 0.0F;
}
