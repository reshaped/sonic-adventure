/* Continuous contact of a moving sphere with a finite triangle. */
#ifndef COLLISION_IMPACT_H
#define COLLISION_IMPACT_H
#include "collision/planes.h"
#include "collision/capsule.h"

/* The normal component of p contributes zero to the oriented area. */
ALWAYS_INLINE Float sweep_area(shz_vec3_t a, shz_vec3_t p, shz_vec3_t n)
{
    return n.x * (a.y * p.z - a.z * p.y) + n.y * (a.z * p.x - a.x * p.z) +
           n.z * (a.x * p.y - a.y * p.x);
}

/* Reconstructing a hit as start + time * move loses precision in
 * proportion to the distance traveled, even for a small sphere. */
static inline Float impact_tolerance(Float radius, NJS_VECTOR move)
{
    return radius * 0x1p-16F +
           (njAbs(move.x) + njAbs(move.y) + njAbs(move.z)) * 0x1p-22F;
}

/* All vertices and query points are relative to the first vertex. */
struct impact_triangle {
    shz_vec3_t v[3], cross, normal;
    Float cross2, cross_length;
};

/* The plane normal is (v[1] - v[0]) × (v[2] - v[0]). Single-precision
 * products can cancel on thin faces. Widen the original coordinates. */
LINKED_LAST(impact_sliver_normal)
static __attribute__((noinline)) shz_vec3_t
impact_sliver_normal(const NJS_POINT3 *v)
{
#ifdef DREAMCAST
    Uint32 saved, wide;
    Float x, y, z;
    /* The GCC ABI enters with PR and SZ clear. Widen all three vertices
     * under one mode change and restore the caller's FPSCR. */
    __asm__ volatile(
        "sts fpscr,%[saved]\n\t"
        "mov %[saved],%[wide]\n\t"
        "or %[pr],%[wide]\n\t"
        "lds %[wide],fpscr\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr0\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr2\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr4\n\t"
        /* b - a occupies DR6, DR8, DR10 and c - a occupies DR12, DR14, DR0. */
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr6\n\tfsub dr0,dr6\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr8\n\tfsub dr2,dr8\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr10\n\tfsub dr4,dr10\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr12\n\tfsub dr0,dr12\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr14\n\tfsub dr2,dr14\n\t"
        "lds.l @%[v]+,fpul\n\tfcnvsd fpul,dr0\n\tfsub dr4,dr0\n\t"
        /* Preserve by and bz until all three components have used them. */
        "fmov fr8,fr2\n\tfmov fr9,fr3\n\tfmul dr0,dr2\n\t"
        "fmov fr10,fr4\n\tfmov fr11,fr5\n\tfmul dr14,dr4\n\t"
        "fsub dr4,dr2\n\tfcnvds dr2,fpul\n\tsts fpul,%[x]\n\t"
        "fmul dr12,dr10\n\tfmul dr6,dr0\n\tfsub dr0,dr10\n\t"
        "fcnvds dr10,fpul\n\tsts fpul,%[y]\n\t"
        "fmul dr14,dr6\n\tfmul dr12,dr8\n\tfsub dr8,dr6\n\t"
        "fcnvds dr6,fpul\n\tsts fpul,%[z]\n\t"
        "lds %[saved],fpscr"
        : [v] "+&r"(v), [saved] "=&r"(saved), [wide] "=&r"(wide), [x] "=r"(x),
          [y] "=r"(y), [z] "=r"(z)
        : [pr] "r"(1U << 19), "m"(*(const NJS_POINT3(*)[3])v)
        : "fpul", "fr0", "fr1", "fr2", "fr3", "fr4", "fr5", "fr6", "fr7", "fr8",
          "fr9", "fr10", "fr11", "fr12", "fr13", "fr14", "fr15");
    return (shz_vec3_t){.x = x, .y = y, .z = z};
#else
    /* The native game's ABI narrows double to float in this unit. */
#pragma push_macro("double")
#undef double
    double bx = (double)v[1].x - v[0].x, cx = (double)v[2].x - v[0].x;
    double by = (double)v[1].y - v[0].y, cy = (double)v[2].y - v[0].y;
    double bz = (double)v[1].z - v[0].z, cz = (double)v[2].z - v[0].z;
    return (shz_vec3_t){
        .x = by * cz - bz * cy, .y = bz * cx - bx * cz, .z = bx * cy - by * cx};
#pragma pop_macro("double")
#endif
}

LINKED_LAST(impact_triangle_init)
static __attribute__((noinline)) Bool
impact_triangle_init(struct impact_triangle *face, const NJS_POINT3 *v)
{
    face->v[0] = (shz_vec3_t){0};
    face->v[1] = shz_vec3_sub(zam_load_vec3(v + 1), zam_load_vec3(v));
    face->v[2] = shz_vec3_sub(zam_load_vec3(v + 2), zam_load_vec3(v));
    face->cross = plane_cross(face->v[1], face->v[2]);
    face->cross2 = plane_dot(face->cross, face->cross);
    /* Widen when the sine of the angle at v[0] is at most 1/32. */
    shz_vec3_t b = face->v[1], c = face->v[2];
    if (face->cross2 <= 0x1p-10F * plane_dot(b, b) * plane_dot(c, c)) {
        face->cross = impact_sliver_normal(v);
        face->cross2 = plane_dot(face->cross, face->cross);
    }
    if (!(face->cross2 > 0.0F)) {
        return FALSE;
    }
    /* Normalizing a large axial face via a reciprocal can shorten its
     * unit normal and bring an adjacent wall into a landing too early. */
    shz_vec3_t n = face->cross;
    if (n.x == 0.0F && n.z == 0.0F) {
        face->cross_length = njAbs(n.y);
        face->normal = (shz_vec3_t){.y = n.y > 0.0F ? 1.0F : -1.0F};
    } else if (n.y == 0.0F && n.z == 0.0F) {
        face->cross_length = njAbs(n.x);
        face->normal = (shz_vec3_t){.x = n.x > 0.0F ? 1.0F : -1.0F};
    } else if (n.x == 0.0F && n.y == 0.0F) {
        face->cross_length = njAbs(n.z);
        face->normal = (shz_vec3_t){.z = n.z > 0.0F ? 1.0F : -1.0F};
    } else {
        face->cross_length = njSqrt(face->cross2);
        face->normal = shz_vec3_scale(n, 1.0F / face->cross_length);
    }
    return TRUE;
}

/* Measure each edge from its nearer endpoint. At a sharp tip, subtracting
 * two products formed at the far end can round an exterior point onto it. */
LINKED_LAST(impact_outside)
static __attribute__((noinline, optimize("fp-contract=off"))) unsigned
impact_outside(const struct impact_triangle *face, shz_vec3_t p)
{
    shz_vec3_t b = face->v[1], c = face->v[2], n = face->cross;
    shz_vec3_t pb = shz_vec3_sub(p, b), pc = shz_vec3_sub(p, c);
    Float da = plane_dot(p, p), db = plane_dot(pb, pb), dc = plane_dot(pc, pc);
    Float u = sweep_area(b, da <= db ? p : pb, n);
    Float v = sweep_area(da <= dc ? p : pc, c, n);
    Float w = sweep_area(shz_vec3_sub(c, b), db <= dc ? pb : pc, n);
    return (u < 0.0F) | ((w < 0.0F) << 1) | ((v < 0.0F) << 2);
}

/* Only exterior edges can be closest. An interior projection onto one
 * of them is the minimum. Otherwise compare their endpoints. */
LINKED_LAST(impact_rim)
static __attribute__((noinline, optimize("fp-contract=off"))) shz_vec3_t
impact_rim(const struct impact_triangle *face, shz_vec3_t p, unsigned edges)
{
    shz_vec3_t nearest = {0};
    Float minimum = __builtin_inff();
    while (edges) {
        int i = edges & 1 ? 0 : edges & 2 ? 1 : 2;
        edges &= edges - 1;
        shz_vec3_t a = face->v[i], b = face->v[(i + 1) % 3];
        shz_vec3_t edge = shz_vec3_sub(b, a), from = shz_vec3_sub(p, a);
        Float along = plane_dot(from, edge);
        Float length2 = plane_dot(edge, edge);
        /* Measure from the nearer endpoint to retain a short displacement. */
        if (along > length2 * 0.5F) {
            from = shz_vec3_sub(p, b);
            along = plane_dot(from, edge);
            if (along < 0.0F) {
                return shz_vec3_sub(from,
                                    shz_vec3_scale(edge, along / length2));
            }
        } else if (along > 0.0F) {
            return shz_vec3_sub(from, shz_vec3_scale(edge, along / length2));
        }

        Float squared = plane_dot(from, from);
        if (squared < minimum) {
            minimum = squared;
            nearest = from;
        }
    }
    return nearest;
}

static inline Bool impact_at_face(shz_vec3_t radial, shz_vec3_t normal,
                                  Float tolerance)
{
    shz_vec3_t offset =
        shz_vec3_sub(radial, shz_vec3_scale(normal, plane_dot(normal, radial)));
    return plane_dot(offset, offset) <= tolerance * tolerance;
}

LINKED_LAST(impact_inside)
static __attribute__((noinline, optimize("fp-contract=off"))) Bool
impact_inside(const struct impact_triangle *face, shz_vec3_t p, Float tolerance)
{
    unsigned edges = impact_outside(face, p);
    if (!edges) {
        return TRUE;
    }
    shz_vec3_t radial = impact_rim(face, p, edges);
    return impact_at_face(radial, face->normal, tolerance);
}

/* FACE and RIM carry a contact at the starting position. LATER carries
 * a time of impact along the segment. */
enum impact_feature { IMPACT_NONE, IMPACT_FACE, IMPACT_RIM, IMPACT_LATER };
LINKED_LAST(impact_contact_height)
static
    __attribute__((noinline, optimize("fp-contract=off"))) enum impact_feature
    impact_contact_height(const struct impact_triangle *face, shz_vec3_t p,
                          Float height, Float radius, Float tolerance,
                          struct contact_plane *contact)
{
    Float reach = radius + tolerance;
    if (njAbs(height) > reach) {
        return IMPACT_NONE;
    }
    unsigned edges = impact_outside(face, p);
    if (edges) {
        shz_vec3_t radial = impact_rim(face, p, edges);
        Float squared = plane_dot(radial, radial);
        if (!(squared <= reach * reach)) {
            return IMPACT_NONE;
        }
        if (!impact_at_face(radial, face->normal, tolerance)) {
            Float distance = njSqrt(squared);
            contact->normal = distance > 0.0F
                                  ? shz_vec3_scale(radial, 1.0F / distance)
                                  : face->normal;
            contact->offset = radius - distance;
            return IMPACT_RIM;
        }
    }
    contact->normal =
        height < 0.0F ? shz_vec3_scale(face->normal, -1.0F) : face->normal;
    contact->offset = radius - njAbs(height);
    return IMPACT_FACE;
}

static inline enum impact_feature
impact_contact(const struct impact_triangle *face, shz_vec3_t p, Float radius,
               Float tolerance, struct contact_plane *contact)
{
    return impact_contact_height(face, p, plane_dot(face->normal, p), radius,
                                 tolerance, contact);
}

/* Evaluate the discriminant at closest approach. The conjugate root
 * retains the small entry time when the sphere starts just outside. */
static inline Bool impact_root(shz_vec3_t from, shz_vec3_t motion, Float radius,
                               Float limit, Float slack, Float *time)
{
    Float squared = plane_dot(motion, motion);
    Float toward = -plane_dot(from, motion);
    if (!(squared > 0.0F && toward > 0.0F)) {
        return FALSE;
    }
    shz_vec3_t near =
        shz_vec3_add(from, shz_vec3_scale(motion, toward / squared));
    Float inside = radius * radius - plane_dot(near, near);
    /* At a tangent, rounding can put the squared distance just inside.
     * A blocking cast needs a resolvable penetration at closest approach. */
    if (!(inside >= slack)) {
        return FALSE;
    }
    Float root = njSqrt(squared * inside);
    if (root < slack) {
        return FALSE;
    }
    Float gap = plane_dot(from, from) - radius * radius;
    Float t = gap / (toward + root);
    if (!(t >= 0.0F && t <= limit)) {
        return FALSE;
    }
    *time = t;
    return TRUE;
}

/* The plane slab bounds the whole triangle expanded by the radius.
 * An interior hit at slab entry needs no edge or vertex roots. */
LINKED_LAST(impact_cast)
static
    __attribute__((noinline, optimize("fp-contract=off"))) enum impact_feature
    impact_cast(const struct impact_triangle *face, shz_vec3_t p,
                shz_vec3_t motion, Float height, Float radius, Float tolerance,
                Bool blocking, Float limit, Float *time,
                struct contact_plane *initial)
{
    Float delta = plane_dot(face->normal, motion);
    Float distance = njAbs(height);
    Float closing = height < 0.0F ? delta : -delta;
    /* A segment that stays outside the supporting plane cannot enter a
     * neighboring triangle's rim along their shared seam. */
    if (blocking && distance <= radius + tolerance &&
        distance >= radius - tolerance &&
        distance - closing >= radius - tolerance) {
        return IMPACT_NONE;
    }
    if (distance <= radius + tolerance) {
        enum impact_feature feature =
            impact_contact_height(face, p, height, radius, tolerance, initial);
        if (feature != IMPACT_NONE) {
            if (blocking && initial->offset <= tolerance &&
                plane_dot(initial->normal, motion) >= -tolerance) {
                return IMPACT_NONE;
            }
            *time = 0.0F;
            return feature;
        }
    }
    if (distance > radius) {
        if (!(closing > 0.0F) || distance - radius > closing * limit) {
            return IMPACT_NONE;
        }
        Float t = (distance - radius) / closing;
        shz_vec3_t at = shz_vec3_add(p, shz_vec3_scale(motion, t));
        if (impact_inside(face, at, tolerance)) {
            if (blocking && closing <= tolerance) {
                return IMPACT_NONE;
            }
            *time = t;
            return IMPACT_LATER;
        }
    }
    if (!(motion.x || motion.y || motion.z)) {
        return IMPACT_NONE;
    }
    Bool found = FALSE;
    Float inward = blocking ? radius * tolerance : 0.0F;
    for (int i = 0; i < 3; i++) {
        shz_vec3_t from = shz_vec3_sub(p, face->v[i]);
        shz_vec3_t edge = shz_vec3_sub(face->v[(i + 1) % 3], face->v[i]);
        Float inverse = 1.0F / plane_dot(edge, edge);
        Float start = plane_dot(from, edge) * inverse;
        Float along = plane_dot(motion, edge) * inverse;
        shz_vec3_t radial = shz_vec3_sub(from, shz_vec3_scale(edge, start));
        shz_vec3_t across = shz_vec3_sub(motion, shz_vec3_scale(edge, along));
        Float t;
        if (impact_root(radial, across, radius, limit, inward, &t)) {
            Float u = start + t * along;
            if (u >= 0.0F && u <= 1.0F) {
                found = TRUE;
                limit = t;
            }
        }
        if (impact_root(from, motion, radius, limit, inward, &t)) {
            found = TRUE;
            limit = t;
        }
    }
    if (found) {
        *time = limit;
    }
    return found ? IMPACT_LATER : IMPACT_NONE;
}

static inline Bool impact_time_height(const struct impact_triangle *face,
                                      shz_vec3_t p, shz_vec3_t motion,
                                      Float height, Float radius,
                                      Float tolerance, Bool blocking,
                                      Float limit, Float *time)
{
    struct contact_plane initial;
    return impact_cast(face, p, motion, height, radius, tolerance, blocking,
                       limit, time, &initial) != IMPACT_NONE;
}

static inline Bool impact_time(const struct impact_triangle *face, shz_vec3_t p,
                               shz_vec3_t motion, Float radius, Float tolerance,
                               Bool blocking, Float limit, Float *time)
{
    return impact_time_height(face, p, motion, plane_dot(face->normal, p),
                              radius, tolerance, blocking, limit, time);
}
#endif
