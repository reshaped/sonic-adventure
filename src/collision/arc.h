/* A linearly moving sphere against a sphere rotating about a fixed pivot. */
#ifndef COLLISION_ARC_H
#define COLLISION_ARC_H

#include "collision/collision.h"
#include "collision/polygon.h"
#include "system/util.h"

/* The center of a sphere sweeps the minor arc from start to its current
 * position about pivot, through at most a quarter turn. The pivot lies
 * in the plane of the circle. */
struct collision_arc {
    NJS_POINT3 pivot, start;
};

/* The single-precision log and exponential used by the game's powf. */
extern float _in_logs(float x);
extern float _in_exps(float x);

static inline Float arc_dot(NJS_VECTOR a, NJS_VECTOR b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline NJS_POINT3 arc_midpoint(NJS_POINT3 a, NJS_POINT3 b)
{
    return (NJS_POINT3){(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F,
                        (a.z + b.z) * 0.5F};
}

/* Slide over one linear interval in the head's frame. The requested
 * velocity stays constant. While touching, only its tangent moves the
 * body. Once it points out of the surface, the body travels freely. */
static void arc_push(NJS_VECTOR start, NJS_VECTOR end, NJS_VECTOR motion,
                     NJS_VECTOR nearest, Float distance2, Float travel2,
                     Float reach, NJS_VECTOR *push)
{
    Float radius2 = reach * reach;
    NJS_VECTOR at = start;
    if (arc_dot(start, start) > radius2) {
        /* Back up from the nearest point to entry. The nearest point may
         * be the end of a still-approaching segment. */
        Float along = arc_dot(nearest, motion);
        Float depth = radius2 - distance2;
        Float back = depth / (njSqrt(along * along + travel2 * depth) - along);
        at.x = nearest.x - back * motion.x;
        at.y = nearest.y - back * motion.y;
        at.z = nearest.z - back * motion.z;
    }
    NJS_VECTOR rest;
    VECTOR_DIFF(&rest, &end, &at);
    Float length = njSqrt(arc_dot(at, at));
    NJS_VECTOR normal = at;
    if (length > 0) {
        VECTOR_SCALE(&normal, &normal, 1.0F / length);
    } else {
        /* A coincident body can exit in its requested direction. */
        Float travel = njSqrt(travel2);
        if (travel > 0) {
            VECTOR_SCALE(&normal, &motion, 1.0F / travel);
        } else {
            normal.y = 1;
        }
    }
    if (length < reach) {
        NJS_VECTOR clear;
        VECTOR_SCALE(&clear, &normal, reach - length);
        VECTOR_ADD(push, &clear);
    }
    Float inward = arc_dot(normal, motion);
    if (inward >= 0) {
        return;
    }
    Float speed = njSqrt(travel2);
    NJS_VECTOR forward;
    VECTOR_SCALE(&forward, &motion, 1.0F / speed);
    Float cosine = inward / speed;
    NJS_VECTOR side;
    VECTOR_SCALE(&side, &forward, cosine);
    VECTOR_DIFF(&side, &normal, &side);
    Float sine = njSqrt(arc_dot(side, side));
    NJS_VECTOR finish = normal;
    Float free_travel = 0;
    if (sine > 0x1p-22F) {
        VECTOR_SCALE(&side, &side, 1.0F / sine);
        /* Let theta be the angle between the normal and -forward. Sliding
         * gives d(theta)/ds = sin(theta)/reach per unit of requested travel.
         * Thus tan(theta/2) grows exponentially until it reaches 1 at release.
         */
        Float k = sine / (1.0F - cosine);
        Float remaining = arc_dot(rest, forward);
        Float decay = _in_exps(-remaining / reach);
        if (k >= decay) {
            finish = side;
            free_travel = remaining + reach * _in_logs(k);
        } else {
            k /= decay;
            Float scale = 1.0F / (1.0F + k * k);
            VECTOR_SCALE(&finish, &forward, (k * k - 1.0F) * scale);
            VECTOR_SCALE(&side, &side, 2.0F * k * scale);
            VECTOR_ADD(&finish, &side);
        }
    }
    NJS_VECTOR shift;
    VECTOR_DIFF(&shift, &finish, &normal);
    VECTOR_SCALE(&shift, &shift, reach);
    VECTOR_SCALE(&forward, &forward, free_travel);
    VECTOR_ADD(&shift, &forward);
    VECTOR_SUB(&shift, &rest);
    VECTOR_ADD(push, &shift);
}

/* a and b bound the head's arc, p and q the player's line over the same
 * interval. All four are relative to the pivot. At equal times the arc
 * and chord differ by at most R * angle^2 / 8. For at most a quarter turn
 * this is less than chord^2 / (4 * R). Linear player motion cancels
 * from that bound. For a push, resolve the touched intervals in time order. */
static Bool arc_interval(NJS_POINT3 a, NJS_POINT3 b, NJS_POINT3 p, NJS_POINT3 q,
                         Float contact, Float orbit, Float error_scale,
                         NJS_VECTOR *push)
{
    NJS_VECTOR chord, start, end, motion;
    VECTOR_DIFF(&chord, &b, &a);
    VECTOR_DIFF(&start, &p, &a);
    VECTOR_DIFF(&end, &q, &b);
    VECTOR_DIFF(&motion, &end, &start);
    if (push) {
        VECTOR_ADD(&start, push);
        VECTOR_ADD(&end, push);
    }
    NJS_VECTOR nearest = start;
    Float travel2 = arc_dot(motion, motion);
    Float along = -arc_dot(start, motion);
    if (along >= travel2) {
        nearest = end;
    } else if (along > 0) {
        Float t = along / travel2;
        nearest.x += t * motion.x;
        nearest.y += t * motion.y;
        nearest.z += t * motion.z;
    }
    Float distance2 = arc_dot(nearest, nearest);
    Float error = arc_dot(chord, chord) * error_scale;
    Float outside = contact + error;
    if (!(distance2 < outside * outside)) {
        return FALSE;
    }
    Float inside = contact - error;
    Bool certain = inside > 0 && distance2 < inside * inside;
    if (!push && certain) {
        return TRUE;
    }
    /* A confirmed push needs a chord accurate to 1/4096 of the clearance.
     * Grazes use the finer bound, at most ten bisections from a quarter turn.
     */
    if ((certain && error <= contact * 0x1p-12F) ||
        error <= contact * 0x1p-16F + orbit * 0x1p-20F) {
        if (!(distance2 < contact * contact)) {
            return FALSE;
        }
        if (push) {
            arc_push(start, end, motion, nearest, distance2, travel2, contact,
                     push);
        }
        return TRUE;
    }
    NJS_POINT3 mid = arc_midpoint(a, b);
    Float scale = orbit / njSqrt(arc_dot(mid, mid));
    VECTOR_SCALE(&mid, &mid, scale);
    NJS_POINT3 player_mid = arc_midpoint(p, q);
    Bool hit =
        arc_interval(a, mid, p, player_mid, contact, orbit, error_scale, push);
    if (hit && !push) {
        return TRUE;
    }
    return arc_interval(mid, b, player_mid, q, contact, orbit, error_scale,
                        push) ||
           hit;
}

static inline Bool collision_arc_sweep(const struct collision_arc *arc,
                                       const NJS_POINT3 *center,
                                       const NJS_POINT3 *start,
                                       const NJS_POINT3 *end, Float reach,
                                       NJS_VECTOR *push)
{
    NJS_POINT3 a, b, p, q;
    VECTOR_DIFF(&a, &arc->start, &arc->pivot);
    VECTOR_DIFF(&b, center, &arc->pivot);
    VECTOR_DIFF(&p, start, &arc->pivot);
    VECTOR_DIFF(&q, end, &arc->pivot);
    Float orbit = njSqrt(arc_dot(a, a));
    return orbit > 0 &&
           arc_interval(a, b, p, q, reach, orbit, 0.25F / orbit, push);
}

/* A solid object resists the player and can push the body without changing
 * its momentum. A damaging head only reports contact. */
static inline Bool collision_arc_player(const struct collision_arc *arc,
                                        struct task *mover,
                                        struct task_work *player,
                                        const NJS_POINT3 *from,
                                        const NJS_POINT3 *to)
{
    struct collision_shape *head = mover->work->collision->shapes;
    struct collision_work *body = player->collision;
    struct collision_shape *sphere = body->shapes;
    Bool solid = (head->push & PUSH_PLAYERS) != 0;
    if (!solid && (!(head->levels & SHAPE_ATTACK) ||
                   !(head->levels & SHAPE_HURTS_PLAYERS))) {
        return FALSE;
    }
    Bool pushes = solid && !(sphere->push & PUSH_FIRM(PUSH_OBJECTS));
    NJS_VECTOR push = {0};
    if (!collision_arc_sweep(arc, &mover->work->pos, from, to,
                             sphere->radius + head->radius,
                             pushes ? &push : NULL)) {
        return FALSE;
    }
    if (pushes) {
        body->flags |= COLLISION_PUSHED;
        if (push.x || push.y || push.z) {
            polygon_limit_carry(to, sphere->radius, &push);
            VECTOR_ADD(&player->pos, &push);
            if (sphere->flags & SHAPE_IN_WORLD) {
                VECTOR_ADD(&sphere->center, &push);
            }
        }
    }
    return TRUE;
}

#endif
