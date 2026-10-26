/* vector sums, differences and a dot product, and a push of Ninja's matrix
 * stack */
#ifndef SYSTEM_VECTOR_H
#define SYSTEM_VECTOR_H
#include <ninja.h>
#include <sg_xpt.h>

/* out = a + b, or a - b for vector_diff. out may be a or b: both are read
 * whole before out is written */
/* NOLINTBEGIN(readability-identifier-length) */
void vector_sum(NJS_VECTOR *a, NJS_VECTOR *b, NJS_VECTOR *out);
void vector_diff(NJS_VECTOR *a, NJS_VECTOR *b, NJS_VECTOR *out);
/* dot(a - from, b - from) */
Float vector_dot_from(NJS_POINT3 *from, NJS_POINT3 *a, NJS_POINT3 *b);
/* NOLINTEND(readability-identifier-length) */
/* push Ninja's matrix stack, and make the new current matrix a unit
 * matrix. FALSE, with nothing pushed, when the stack is full, and TRUE
 * otherwise */
Bool matrix_push_unit(void);

#endif
