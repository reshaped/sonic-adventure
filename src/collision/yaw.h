/* Sphere contact with a triangle under a linear translation and a constant
 * turn about world Y. The triangle is in its published starting pose. */
#ifndef COLLISION_YAW_H
#define COLLISION_YAW_H
#include "system/util.h"
#include "collision/impact.h"

struct yaw_path {
    /* Initial player center relative to the pivot, and velocity relative
     * to the pivot's translation, both in the starting world axes. */
    shz_vec3_t origin, relative, travel;
    Float turn, radians, acceleration, speed_bound, radius, tolerance;
};

struct yaw_sample {
    shz_vec3_t center;
    Float time, sine, cosine;
};

struct yaw_contact {
    shz_vec3_t normal, velocity;
    Float time, safe;
    enum impact_feature feature;
};

enum yaw_result { YAW_CLEAR, YAW_TOUCH, YAW_UNCERTAIN };

struct task;
struct polygon_yaw;
struct collision_mesh;
struct polygon;
struct yaw_hits {
    struct yaw_contact contact;
    const struct polygon *face;
    Float unresolved, time_tolerance;
};

/* The provider owns the next pose and the mesh cast in its overlay. */
struct polygon_yaw_ops {
    void (*pose)(struct task *, struct polygon_yaw *);
    Bool (*cast)(const struct collision_mesh *, shz_vec3_t, struct yaw_path *,
                 Float, Float, struct yaw_hits *);
};

static inline shz_vec3_t yaw_turn(shz_vec3_t v, Float sine, Float cosine)
{
    return (shz_vec3_t){v.x * cosine + v.z * sine, v.y,
                        v.z * cosine - v.x * sine};
}

static inline Float yaw_length(shz_vec3_t v)
{
    return njSqrt(v.x * v.x + v.z * v.z);
}

static inline void yaw_prepare(struct yaw_path *path, Float end)
{
    path->radians = path->turn * (1.0F / ANGLE_PER_RADIAN);
    Float distance = yaw_length(path->origin);
    Float last = yaw_length(
        shz_vec3_add(path->origin, shz_vec3_scale(path->relative, end)));
    if (last > distance) {
        distance = last;
    }
    path->speed_bound = njSqrt(plane_dot(path->relative, path->relative)) +
                        njAbs(path->radians) * distance;
    path->acceleration = path->radians * path->radians * distance +
                         2 * njAbs(path->radians) * yaw_length(path->relative);
}

/* The residual below one Ninja angle unit is under 0.000096 radians.
 * Its cosine differs from one by less than half a float ulp. */
static inline struct yaw_sample yaw_at(const struct yaw_path *path, Float t)
{
    Float angle = path->turn * t;
    Angle whole = (Angle)angle;
    Float fraction = (angle - (Float)whole) * (1.0F / ANGLE_PER_RADIAN);
    Float sine, cosine;
    _builtin_fsca(whole, &sine, &cosine);
    struct yaw_sample at = {.time = t,
                            .sine = sine + fraction * cosine,
                            .cosine = cosine - fraction * sine};
    at.center =
        yaw_turn(shz_vec3_add(path->origin, shz_vec3_scale(path->relative, t)),
                 -at.sine, at.cosine);
    return at;
}

/* Form the small separation before adding the rotation. Rounding the rotated
 * center near a distant blade tip can otherwise erase a grazing clearance. */
static inline shz_vec3_t yaw_from(const struct yaw_path *path,
                                  const struct yaw_sample *at,
                                  shz_vec3_t vertex)
{
    shz_vec3_t travel = shz_vec3_scale(path->relative, at->time);
    shz_vec3_t p = shz_vec3_add(path->origin, travel);
    shz_vec3_t base = shz_vec3_add(shz_vec3_sub(path->origin, vertex), travel);
    Float omc = at->cosine > 0 ? -at->sine * at->sine / (1.0F + at->cosine)
                               : at->cosine - 1.0F;
    base.x += omc * p.x - at->sine * p.z;
    base.z += at->sine * p.x + omc * p.z;
    return base;
}

/* The closest point also supplies the contact-point velocity. At an edge
 * its radial normal differs from the triangle's face normal. */
static Bool
yaw_contact_at(const struct yaw_path *path, const struct impact_triangle *face,
               shz_vec3_t vertex, Bool both, const struct yaw_sample *at,
               struct yaw_contact *contact, Float *outward, Float *gap)
{
    shz_vec3_t q = yaw_from(path, at, vertex);
    Float height = plane_dot(q, face->normal);
    unsigned edges = impact_outside(face, q);
    shz_vec3_t radial = edges ? impact_rim(face, q, edges)
                              : shz_vec3_scale(face->normal, height);
    Float distance = edges ? njSqrt(plane_dot(radial, radial)) : njAbs(height);
    shz_vec3_t local =
        distance > 0 ? shz_vec3_scale(radial, 1.0F / distance)
                     : (both && height < 0 ? shz_vec3_scale(face->normal, -1)
                                           : face->normal);
    shz_vec3_t normal = yaw_turn(local, at->sine, at->cosine);
    shz_vec3_t arm =
        yaw_turn(shz_vec3_sub(at->center, radial), at->sine, at->cosine);
    shz_vec3_t spin = {.x = path->radians * arm.z, .z = -path->radians * arm.x};
    Float rate = plane_dot(normal, shz_vec3_sub(path->relative, spin));
    *outward = distance > 0 ? rate : -__builtin_inff();
    *gap = distance - path->radius;
    if ((!both && height < 0) || distance > path->radius || rate >= 0) {
        return FALSE;
    }
    *contact =
        (struct yaw_contact){.time = at->time,
                             .safe = at->time,
                             .normal = normal,
                             .velocity = shz_vec3_add(path->travel, spin),
                             .feature = edges ? IMPACT_RIM : IMPACT_FACE};
    return TRUE;
}

/* In the blade's frame the center follows a curve. Its second derivative
 * bounds the deviation from this chord by acceleration * duration² / 8.
 * Only an actual contact on the curve can supply a normal or a response. */
/* An unresolved node needs subdivision. It does not yet imply contact. */
static enum yaw_result yaw_node(const struct yaw_path *path,
                                const struct impact_triangle *face,
                                shz_vec3_t vertex, Bool both,
                                struct yaw_sample start, struct yaw_sample end,
                                struct yaw_contact *contact, Float *advance)
{
    Float duration = end.time - start.time;
    Float bow = (0.125F * path->acceleration) * duration * duration;
    shz_vec3_t from = yaw_from(path, &start, vertex);
    shz_vec3_t chord = shz_vec3_sub(yaw_from(path, &end, vertex), from);
    Float height = plane_dot(face->normal, from);
    if (!face->normal.x && !face->normal.z) {
        Float end_height = height + chord.y * face->normal.y;
        if ((height > path->radius && end_height > path->radius) ||
            (height < -path->radius && end_height < -path->radius) ||
            (!chord.y && njAbs(height) >= path->radius)) {
            return YAW_CLEAR;
        }
    }
    if (!both && height < -bow - path->tolerance &&
        height + plane_dot(face->normal, chord) < -bow - path->tolerance) {
        return YAW_CLEAR;
    }
    Float roundoff = path->tolerance * 0.125F;
    Float outward, gap;
    if (yaw_contact_at(path, face, vertex, both, &start, contact, &outward,
                       &gap)) {
        return YAW_TOUCH;
    }
    /* The distance to a convex triangle is convex along a straight line.
     * Only the curve's own acceleration can turn an opening gap inward. */
    Float worst = (outward - 0.5F * path->acceleration * duration) * duration;
    if (outward >= path->acceleration * duration ||
        (gap > roundoff * 0.25F && gap + worst > roundoff * 0.25F)) {
        return YAW_CLEAR;
    }
    /* The positive root of gap + outward*t - M*t²/2 bounds the
     * first time the curved path can reach this triangle. */
    Float margin = gap - roundoff * 0.25F;
    Float time_tol = path->tolerance / path->speed_bound;
    if (margin > 0) {
        Float root =
            njSqrt(outward * outward + 2.0F * path->acceleration * margin);
        Float divisor = root - outward;
        if (divisor > 0) {
            Float dt = 2.0F * margin / divisor;
            if (dt > time_tol && dt < duration) {
                *advance = start.time + dt * (1.0F - 0x1p-19F);
                return YAW_UNCERTAIN;
            }
        }
    }
    if (outward < 0 && gap <= path->tolerance) {
        Float dt = 0.875F * time_tol;
        struct yaw_sample close =
            dt < duration ? yaw_at(path, start.time + dt) : end;
        if (yaw_contact_at(path, face, vertex, both, &close, contact, &outward,
                           &gap)) {
            contact->safe = start.time;
            return YAW_TOUCH;
        }
    }
    Float first;
    if (!impact_time_height(face, from, chord, height,
                            path->radius + bow + roundoff, roundoff, FALSE, 1,
                            &first)) {
        return YAW_CLEAR;
    }
    return YAW_UNCERTAIN;
}

static enum yaw_result yaw_scan(const struct yaw_path *path,
                                const struct impact_triangle *face,
                                shz_vec3_t vertex, Bool both,
                                struct yaw_sample start, struct yaw_sample end,
                                int depth, int *left_visits,
                                struct yaw_contact *contact)
{
    /* Visit intervals in time order. Only the right endpoints of deferred
     * siblings stay on the stack, including for near-tangent passes. */
    enum { max_depth = 20 };
    struct yaw_sample pending[max_depth];
    int count = 0;
    Float minimum_span = (end.time - start.time) * 0x1p-20F;
    Float earliest = __builtin_inff();
    for (;;) {
        Float advance = start.time;
        enum yaw_result kind =
            yaw_node(path, face, vertex, both, start, end, contact, &advance);
        if (kind == YAW_UNCERTAIN) {
            if (!*left_visits) {
                if (start.time < earliest) {
                    earliest = start.time;
                }
                break;
            }
            --*left_visits;
            if (advance > start.time && advance < end.time) {
                start = yaw_at(path, advance);
                continue;
            }
            Float duration = end.time - start.time;
            struct yaw_sample middle =
                yaw_at(path, start.time + 0.5F * duration);
            if (count + depth < max_depth && duration > minimum_span &&
                duration * path->speed_bound >= path->tolerance * 0.25F &&
                start.time < middle.time && middle.time < end.time) {
                pending[count++] = end;
                end = middle;
                continue;
            }
            if (start.time < earliest) {
                earliest = start.time;
            }
            Float outward, gap;
            if (yaw_contact_at(path, face, vertex, both, &middle, contact,
                               &outward, &gap) ||
                yaw_contact_at(path, face, vertex, both, &end, contact,
                               &outward, &gap)) {
                kind = YAW_TOUCH;
            }
        }
        if (kind == YAW_TOUCH) {
            if (earliest < contact->safe) {
                contact->safe = earliest;
            }
            if ((contact->time - contact->safe) * path->speed_bound <=
                path->tolerance) {
                return YAW_TOUCH;
            }
            break;
        }
        if (!count ||
            (end.time - earliest) * path->speed_bound > path->tolerance) {
            break;
        }
        start = end;
        end = pending[--count];
    }
    if (earliest > end.time) {
        return YAW_CLEAR;
    }
    contact->safe = contact->time = earliest;
    return YAW_UNCERTAIN;
}
static inline enum yaw_result
yaw_interval(const struct yaw_path *path, const struct impact_triangle *face,
             shz_vec3_t vertex, Bool both, struct yaw_sample start,
             struct yaw_sample end, struct yaw_contact *contact)
{
    int visits = 128;
    return yaw_scan(path, face, vertex, both, start, end, 0, &visits, contact);
}
#endif
