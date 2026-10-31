/* helpers the game shares: random numbers, angles and vectors, Ninja's
 * draw state, model trees and blends, world sweeps and string formatting */
#ifndef SYSTEM_UTIL_H
#define SYSTEM_UTIL_H
#include <ninja.h>
#include <sg_xpt.h>
#include <stdarg.h>
#include <stdlib.h>

struct angles;
struct task_work;

/* the factor that scales rand()'s 0 to RAND_MAX to a Float from 0 up to 1,
 * never 1 itself */
#define RAND_SCALE (1.0F / (RAND_MAX + 1))
/* RANDOM is a random Float from lo up to hi, never hi itself. RANDOM_ANGLE
 * is a random angle in a turn, a Float from 0 to 65534 in steps of 2 */
#define RANDOM(lo, hi) ((rand() * (((hi) - (lo)) * RAND_SCALE)) + (lo))
#define RANDOM_ANGLE() (rand() * 2.0F)

/* Ninja's angle units, 65536 to a turn, per radian and per degree */
#ifdef GCC
/* the two quotients as SHC folds them, which the image's pool words hold:
 * 0x4622F986 at 0x8c058db0, in vector_angle, and 0x43360B60 at 0x8c08cb50,
 * in ring_scatter. under -frounding-math GCC folds neither, so each use
 * would run an FDIV */
#define ANGLE_PER_RADIAN 0x1.45f30cp+13
#define ANGLE_PER_DEGREE 0x1.6c16cp+7F
#else
#define ANGLE_PER_RADIAN (65536.0 / (2 * NJD_PI))
#define ANGLE_PER_DEGREE (65536.0F / 360.0F)
#endif
/* @hack redundant: the unary plus, which keeps SHC from folding the two
 * factors into one: r takes an FMUL by 65536 and one by 1/(2 pi), as at
 * 0x8c0c1682 and 0x8c0c1684. without it, summed over tails_choose_mode and
 * sonic_choose_mode, 361 of 13251 words differ */
#define RADIANS_TO_ANGLE(r)                                                    \
    ((Angle)(+((r) * 65536.0F) * (1.0F / (2.0F * NJD_PI))))

/* reads x twice */
#define SQUARE(x) ((x) * (x))

/* reads x up to three times */
#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

#define VECTOR_COPY(out, v)                                                    \
    ((out)->x = (v)->x, (out)->y = (v)->y, (out)->z = (v)->z)
#define VECTOR_DIFF(out, a, b)                                                 \
    ((out)->x = (a)->x - (b)->x, (out)->y = (a)->y - (b)->y,                   \
     (out)->z = (a)->z - (b)->z)
#define VECTOR_SUM(out, a, b)                                                  \
    ((out)->x = (a)->x + (b)->x, (out)->y = (a)->y + (b)->y,                   \
     (out)->z = (a)->z + (b)->z)
#define VECTOR_SCALE(out, v, s)                                                \
    ((out)->x = (v)->x * (s), (out)->y = (v)->y * (s), (out)->z = (v)->z * (s))
#define VECTOR_ADD(v, d) ((v)->x += (d)->x, (v)->y += (d)->y, (v)->z += (d)->z)
#define VECTOR_SUB(v, d) ((v)->x -= (d)->x, (v)->y -= (d)->y, (v)->z -= (d)->z)
#define VECTOR_CLEAR(v) ((v)->x = 0.0F, (v)->y = 0.0F, (v)->z = 0.0F)
#define VECTOR_SQUARED(v)                                                      \
    (((v)->x * (v)->x) + ((v)->y * (v)->y) + ((v)->z * (v)->z))
#define ACROSS_SQUARED(v) (((v)->x * (v)->x) + ((v)->z * (v)->z))

/* a point or a vector, and its floats as an array: elements[i] reaches
 * each float, where (&v->x)[i] would step outside x */
union point_floats {
    NJS_POINT3 point;
    NJS_VECTOR vector;
    Float elements[3];
};
#define VECTOR_FLOATS(v) (((union point_floats *)(v))->elements)

/* a line, whole and as its point and direction, or as its six floats */
union line_view {
    NJS_LINE line;
    struct {
        NJS_POINT3 point;
        NJS_VECTOR direction;
    } split;
    Float coordinates[6];
};

/* a plane, whole and as its point and normal */
union plane_view {
    NJS_PLANE plane;
    struct {
        NJS_POINT3 point;
        NJS_VECTOR normal;
    } split;
};

/* an angle, whole and as its halves, the low one first */
union angle_halves {
    Angle whole;
    Sint16 half[2];
};

/* rotate by ang unless it's 0: TURN turns Ninja's current matrix, and
 * TURN_MATRIX matrix. each is an expression, and reads ang twice when it
 * isn't 0 */
#define TURN(rotate, ang) ((ang) ? (rotate)(NULL, (ang)) : (void)0)
#define TURN_MATRIX(rotate, matrix, ang)                                       \
    ((ang) ? (rotate)((matrix), (ang)) : (void)0)
/* TURN and TURN_MATRIX through local: local takes ang, in local's type,
 * and the test and the call read local. a caller passes a local so that its
 * reads count when SHC weighs which values get registers */
#define TURN_VIA(rotate, ang, local)                                           \
    (((local) = (ang)) ? (rotate)(NULL, (local)) : (void)0)
#define TURN_MATRIX_VIA(rotate, matrix, ang, local)                            \
    (((local) = (ang)) ? (rotate)((matrix), (local)) : (void)0)

/* points of one node's model that model_weld moves onto points of
 * another's */
struct model_weld {
    Sint16 count; /* pairs. a table of welds ends at a count of 0 */
    Sint8 node_a; /* node indices in tree_list_nodes order */
    Sint8 node_b;
    Sint16 *pairs; /* a point index in node_a's model, then one in node_b's */
};

/* from, turned by step toward to the short way round, or to when to lies
 * within step of it. the result is from 0 to 0xffff */
Angle angle_approach(Angle from, Angle to, Angle step);
/* the short turn from from to to, -0x8000 to 0x7fff. angle_distance gives
 * its size, 0 to 0x8000 */
Angle angle_difference(Angle from, Angle to);
Angle angle_distance(Angle from, Angle to);
/* to, moved by whole turns to the side of from that the short turn from
 * from to to goes. that is at or above from for a positive turn, and at or
 * below it for a negative one. from when the two are the same angle */
Angle angle_unwrap(Angle from, Angle to);
/* the angle between unit vectors v1 and v2, 0 to 0x8000. when a non-null
 * normal points against v1 x v2, the result is a turn less that angle,
 * 0x8000 to 0x10000 */
int vector_angle(NJS_VECTOR *v1, NJS_VECTOR *v2, NJS_VECTOR *normal);
/* set angles[1] and angles[2] to the angles about y and z that dir points
 * at, less those that a non-null from points at. angles[0] keeps its
 * value */
void vector_get_angles(const NJS_VECTOR *dir, const NJS_VECTOR *from,
                       Angle angles[3]);
/* value, wrapped into 0 to period, or 0 for a period of 0 */
Float float_wrap(Float value, Float period);
/* from, moved toward to by step, and no further */
Float float_approach(Float from, Float to, Float step);

/* TRUE when work->pos projects inside the screen, widened on every side by
 * a margin. the margin is 3000 over pos's distance from camera_work, which
 * must exist, and is unset at distance 0 */
Bool draw_on_screen(struct task_work *work);
/* draw_attr_save keeps _nj_constant_attr_and_ and _nj_constant_attr_or_ in
 * one slot, which draw_attr_restore puts back. saves don't nest, and
 * draw_state_save takes the same slot. draw_attr_set sets and_attr's bits
 * in _nj_constant_attr_and_, and or_attr's in _nj_constant_attr_or_.
 * draw_attr_clear clears them */
void draw_attr_save(void);
void draw_attr_restore(void);
void draw_attr_set(Uint32 and_attr, Uint32 or_attr);
void draw_attr_clear(Uint32 and_attr, Uint32 or_attr);
/* the same for _nj_control_3d_flag_, in a slot draw_state_save takes too */
void draw_control_save(void);
void draw_control_restore(void);
void draw_control_set(Uint32 flags);
void draw_control_clear(Uint32 flags);

/* blend from's points toward to's into out, as points_blend_sine does, then
 * recompute out's normals */
void model_blend(NJS_MODEL *from, NJS_MODEL *to, NJS_MODEL *out, Float blend);
/* out[i] = from[i] * (1 - weight) + to[i] * weight, for count points.
 * weight is blend, or the sine of blend quarter turns for
 * points_blend_sine, or blend squared for points_blend_square */
void points_blend(NJS_POINT3 *from, NJS_POINT3 *to, NJS_POINT3 *out,
                  Float blend, int count);
void points_blend_sine(NJS_POINT3 *from, NJS_POINT3 *to, NJS_POINT3 *out,
                       Float blend, int count);
void points_blend_square(NJS_POINT3 *from, NJS_POINT3 *to, NJS_POINT3 *out,
                         Float blend, int count);
/* store each polygon's normal in its meshset's normals. the normal is the
 * cross product of the unit edges from the polygon's first point to its
 * second and to its last. it isn't made unit. in a triangle strip, the two
 * edges swap places in the product on every other triangle. the swap starts
 * at the first triangle in a strip whose length word has NJD_TRIMESH_END.
 * every strip stores its normals from its meshset's first, over the strips
 * before it. a meshset of any other type stops the walk */
void model_calc_face_normals(NJS_MODEL *model);
/* set each point's normal to the mean of the normals of the polygons that
 * use it. a strip's normals are read from its meshset's first, as
 * model_calc_face_normals stores them. a point in no polygon divides 0 by 0.
 * a meshset of any other type stops the walk, and leaves that point's sum
 * undivided */
void model_calc_point_normals(NJS_MODEL *model);
/* pose object at frame of motion. each welded point of node_b's model, and
 * its normal, then becomes its partner's in node_a's model, carried into
 * node_b's space. object's tree must have at most 64 nodes, unchecked */
void model_weld(NJS_OBJECT *object, NJS_MOTION *motion, Float frame,
                struct model_weld *welds);

/* a tree's walk takes each node, then its child's tree, then its next
 * sibling's. tree_node_index gives object's place in the walk from root, or
 * -1. tree_list_nodes lists the walk from root into nodes and returns the
 * count. nodes can't be null: the walk reads each node back from it */
int tree_node_index(NJS_OBJECT *root, NJS_OBJECT *object);
int tree_list_nodes(NJS_OBJECT *root, NJS_OBJECT **nodes);
/* compute the matrices of object and its descendants, in walk order, from
 * base, or from Ninja's current matrix when base is null. they go to a
 * static table of 32, which nothing bounds and the next call overwrites.
 * the result is the table's entry at index, and tree_get_matrix copies one
 * out */
NJS_MATRIX *tree_calc_matrices(NJS_MATRIX *base, NJS_OBJECT *object, int index);
void tree_get_matrix(NJS_MATRIX *out, int index);
/* set or clear evalflags of each object of a null-ended list */
void evalflags_set_all(NJS_OBJECT **objects, Uint32 flags);
void evalflags_clear_all(NJS_OBJECT **objects, Uint32 flags);
/* a sprite of the texture, at the origin, unscaled and unturned */
void sprite_init(NJS_SPRITE *sprite, NJS_TEXLIST *texlist,
                 NJS_TEXANIM *texanim);

/* what world_sweep_ball hits */
#define HIT_GROUND 0x1
#define HIT_WALL 0x2 /* or a ceiling */

/* sweep a ball of radius from pos along speed, past the objects that have
 * any of skip's flags. the result has HIT_GROUND or HIT_WALL for every
 * contact, and is 0 for none. pos and speed then take the nearest
 * contact's. an object's force moves its contact on by the force's speed.
 *
 * ang may be null. otherwise the ball's up is the y axis turned by ang, and
 * ang changes with the nearest contact too. ang->x and ang->z take the
 * surface's slope angles, and ang->y keeps its own. each then adds the turn
 * of the object's force. on a surface facing down, ang->x or ang->z is a
 * half turn less its slope: whichever of the two that puts nearer ang's
 * own. when the nearest contact's object has an owner but no force, ang
 * adds an unset turn */
int world_sweep_ball(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                     int skip, Float radius);
/* world_sweep_all is world_sweep_ball skipping nothing. world_sweep_all_hit
 * and world_sweep_ball_hit sweep as world_sweep_ball does, skipping
 * nothing or skip's flags, and move pos, speed and ang alike. they return
 * TRUE for any contact */
int world_sweep_all(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                    Float radius);
Bool world_sweep_all_hit(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                         Float radius);
Bool world_sweep_ball_hit(NJS_POINT3 *pos, NJS_VECTOR *speed,
                          struct angles *ang, int skip, Float radius);

/* printf's formatting, with the flags -, + and 0 and a field width. its
 * conversions are %s, %c, %u, %d, %x and %X. any other, %% too, takes an
 * int and writes only the width's padding. buffer must hold the result and
 * its null, which nothing checks. a format that ends inside a conversion
 * reads past its null. a zero-padded number wider than 63 writes before
 * the formatter's digit buffer. the result is the length written, less the
 * null */
int string_vformat(char *buffer, const char *format, va_list args);
int string_format(char *buffer, const char *format, ...);

#ifdef GCC
/* under GCC, a C call of angle_difference or angle_distance outside util.c
 * runs its helper inline. as in ninja_zam.h, the helper is named zam_ and
 * the function's name, and a macro of the name calls it. each helper's body
 * is util.c's. util.c undefines both macros above its definitions, which its
 * own calls then reach. angle_approach, angle_unwrap and float_wrap have no
 * helper: inlined, each would add 24 to 58 bytes at a call site, to save the
 * call's 4 to 7 instructions */
ALWAYS_INLINE Angle zam_angle_difference(Angle from, Angle to)
{
    Sint16 turn;

    from = (Uint16)from;
    to = (Uint16)to;
    turn = to - from;
    return turn;
}
ZAM_ASSERT_COMPATIBLE(angle_difference);
#define angle_difference(from, to) zam_angle_difference(from, to)

ALWAYS_INLINE Angle zam_angle_distance(Angle from, Angle to)
{
    Uint16 distance;

    from = (Uint16)from;
    to = (Uint16)to;
    distance = to - from;
    if ((Sint16)distance < 0) {
        distance = -distance;
    }
    return distance;
}
ZAM_ASSERT_COMPATIBLE(angle_distance);
#define angle_distance(from, to) zam_angle_distance(from, to)
#endif

#endif
