/* Projection onto the intersection of contact half-spaces in three dimensions.
 */
#ifndef COLLISION_PLANES_H
#define COLLISION_PLANES_H

#include <ninja_vec3.h>
#include "system/port.h"

struct contact_plane {
    shz_vec3_t normal;
    float offset; /* normal . displacement >= offset */
};

static inline float plane_dot(shz_vec3_t a, shz_vec3_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline shz_vec3_t plane_cross(shz_vec3_t a, shz_vec3_t b)
{
    return (shz_vec3_t){.x = a.y * b.z - a.z * b.y,
                        .y = a.z * b.x - a.x * b.z,
                        .z = a.x * b.y - a.y * b.x};
}

/* Nearly parallel normals share most of their components. Subtract that
 * direction before crossing them or taking a bound on their intersection. */
static inline struct contact_plane plane_relative(struct contact_plane p,
                                                  struct contact_plane to)
{
    float cosine = plane_dot(p.normal, to.normal);
    if (cosine > 0.99F) {
        p.normal = shz_vec3_sub(p.normal, to.normal);
        p.offset -= to.offset;
    } else if (cosine < -0.99F) {
        p.normal = shz_vec3_add(p.normal, to.normal);
        p.offset += to.offset;
    }
    return p;
}

/* A small positive residual can still move the intersection appreciably.
 * Test it against the scale of this dot product before discarding it. */
static inline int plane_blocks(struct contact_plane p, shz_vec3_t x,
                               float tolerance)
{
    float a = p.normal.x * x.x, b = p.normal.y * x.y, c = p.normal.z * x.z;
    float gap = p.offset - (a + b + c);
    if (gap <= 0.0F) {
        return 0;
    }
    if (gap > tolerance) {
        return 1;
    }
    float error = 0x1p-24F * (__builtin_fabsf(a) + __builtin_fabsf(b) +
                              __builtin_fabsf(c) + __builtin_fabsf(p.offset));
    return gap > error;
}

static inline int cone_accepts(const struct contact_plane *planes, int count,
                               shz_vec3_t v, float tolerance)
{
    for (int i = 0; i < count; i++) {
        if (!(plane_dot(planes[i].normal, v) >= -tolerance)) {
            return 0;
        }
    }
    return 1;
}

/* For planes through the origin, the closest velocity lies inside the cone,
 * on a face, on a crease, or at rest. A candidate on a face or crease is an
 * orthogonal projection, so the longest feasible candidate is the closest. */
LINKED_LAST(planes_project_cone)
static __attribute__((noinline, optimize("fp-contract=off"))) shz_vec3_t
planes_project_cone(const struct contact_plane *planes, int count,
                    shz_vec3_t wanted, float tolerance)
{
    shz_vec3_t best = {0};
    float best_length = 0.0F;
    /* Rounded face normals of an extruded passage need not share an exact
     * axis. Allow the dot-product error at this velocity, within tolerance. */
    float noise =
        0x1p-22F * (1.0F + __builtin_fabsf(wanted.x) +
                    __builtin_fabsf(wanted.y) + __builtin_fabsf(wanted.z));
    if (noise < tolerance) {
        tolerance = noise;
    }
    if (cone_accepts(planes, count, wanted, tolerance)) {
        return wanted;
    }
    for (int i = 0; i < count; i++) {
        shz_vec3_t n = planes[i].normal;
        float across = plane_dot(n, wanted) / plane_dot(n, n);
        shz_vec3_t face = shz_vec3_sub(wanted, shz_vec3_scale(n, across));
        float length = plane_dot(face, face);
        if (length > best_length &&
            cone_accepts(planes, count, face, tolerance)) {
            best = face;
            best_length = length;
        }
        for (int j = 0; j < i; j++) {
            struct contact_plane p = plane_relative(planes[j], planes[i]);
            shz_vec3_t axis = plane_cross(n, p.normal);
            float square = plane_dot(axis, axis);
            if (!(square > 1.0e-12F)) {
                continue;
            }
            shz_vec3_t crease =
                shz_vec3_scale(axis, plane_dot(wanted, axis) / square);
            length = plane_dot(crease, crease);
            if (length > best_length &&
                cone_accepts(planes, count, crease, tolerance)) {
                best = crease;
                best_length = length;
            }
        }
    }
    return best;
}

/* If the closest feasible point violates the next plane, the new closest
 * point lies on its boundary. A second boundary restricts it to a line.
 * Each remaining half-space then gives one end of an interval on that line.
 * Normals have unit length. Failure leaves the caller's output untouched. */
LINKED_LAST(planes_project)
static __attribute__((noinline, optimize("fp-contract=off"))) int
planes_project(const struct contact_plane *planes, int count, shz_vec3_t wanted,
               float tolerance, shz_vec3_t *out)
{
    shz_vec3_t result = wanted;
    float roundoff = tolerance * (1.0F / 64.0F);

    int may_be_cone = count > 2;
    int i;
    for (i = 0; i < count; i++) {
        shz_vec3_t n = planes[i].normal;
        if (!plane_blocks(planes[i], result, roundoff)) {
            continue;
        }
        float normal2 = plane_dot(n, n);
        float distance = (planes[i].offset - plane_dot(n, wanted)) / normal2;
        shz_vec3_t on_plane = shz_vec3_add(wanted, shz_vec3_scale(n, distance));
        result = on_plane;
        for (int j = 0; j < i; j++) {
            if (!plane_blocks(planes[j], result, roundoff)) {
                continue;
            }
            if (may_be_cone) {
                int k = 0;
                while (k < count && planes[k].offset == 0.0F) {
                    k++;
                }
                if (k == count) {
                    result =
                        planes_project_cone(planes, count, wanted, tolerance);
                    goto verify;
                }
                may_be_cone = 0;
            }
            struct contact_plane p = plane_relative(planes[j], planes[i]);
            shz_vec3_t axis = plane_cross(n, p.normal);
            float squared = plane_dot(axis, axis);
            if (!(squared > 1.0e-12F)) {
                if (planes[j].offset - plane_dot(planes[j].normal, result) <=
                    tolerance) {
                    continue;
                }
                return 0;
            }
            /* The intersection of two velocity boundaries goes through the
             * origin. For offset boundaries use the closest point to it. */
            shz_vec3_t line = {0};
            if (planes[i].offset != 0.0F || p.offset != 0.0F) {
                line = shz_vec3_scale(n, planes[i].offset / normal2);
                float across = (p.offset - plane_dot(p.normal, line)) / squared;
                line = shz_vec3_add(
                    line, shz_vec3_scale(plane_cross(axis, n), across));
            }
            float desired = plane_dot(wanted, axis) / squared;
            float low = -__builtin_inff(), high = __builtin_inff();
            for (int k = 0; k < j; k++) {
                struct contact_plane limit =
                    plane_relative(planes[k], planes[i]);
                float slope = plane_dot(limit.normal, axis);
                float gap = limit.offset - plane_dot(limit.normal, line);
                if (slope * slope <= 1.0e-12F * squared &&
                    __builtin_fabsf(gap) <= tolerance) {
                    continue;
                }
                if (slope > 0.0F) {
                    float bound = gap / slope;
                    if (low < bound) {
                        low = bound;
                    }
                } else if (slope < 0.0F) {
                    float bound = gap / slope;
                    if (high > bound) {
                        high = bound;
                    }
                } else if (gap > tolerance) {
                    return 0;
                }
            }
            if (low > high) {
                return 0;
            }
            float along = low > desired ? low : high < desired ? high : desired;
            result = shz_vec3_add(line, shz_vec3_scale(axis, along));
        }
    }
verify:
    for (i = 0; i < count; i++) {
        if (!(planes[i].offset - plane_dot(planes[i].normal, result) <=
              tolerance)) {
            return 0;
        }
    }
    *out = result;
    return 1;
}
#endif
