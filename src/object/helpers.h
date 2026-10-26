/* helpers: small routines the objects and the race share, from vector sums
 * to mirrors and jumps */
#ifndef OBJECT_HELPERS_H
#define OBJECT_HELPERS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct collision_shape;

/* what object_touch_moved keeps of a touch between its calls. a touch's
 * first call fills moved and by.shape, and its report replaces both */
struct touch {
    NJS_VECTOR moved; /* the shape's center, then how far it moved since */
    union {
        struct collision_shape *shape; /* the shape, until the report */
        struct task *owner; /* the shape's task, from the report on */
    } by;
};

/* a leap from pos to target in steps steps, whose y speed gravity takes
 * away each step. jump_aim sets its speed */
struct jump {
    NJS_POINT3 pos;
    NJS_POINT3 target;
    Float gravity;
    Sint32 steps; /* the steps left, which jump_landed counts down */
    NJS_VECTOR speed;
};

/* jump_landed's result: steps were left, or none were and pos is within 2
 * of target, or farther */
enum jump_state { JUMP_FLYING, JUMP_ON_TARGET, JUMP_OFF_TARGET };

/* what item_drop leaves: one ring, 5 or 10 scattered, or an item box of
 * random rings or of an extra life */
enum drop_kind {
    DROP_RING = 1,
    DROP_5_RINGS = 2,
    DROP_10_RINGS = 3,
    DROP_RANDOM_RINGS = 4,
    DROP_1UP = 6
};

/* copy, clear, add and subtract three Floats or Angles. a Float or Angle
 * pointer here is an NJS_VECTOR's or a struct angles's first member, and
 * these reach the other two past it. vector_between sets out to to - from */
void vector_copy(const Float *from, Float *to);
void angles_copy(const Angle *from, Angle *to);
void vector_clear(Float *vector);
void vector_add(const NJS_VECTOR *first, const NJS_VECTOR *second,
                NJS_VECTOR *sum);
void vector_sub(const Float *first, const Float *second, Float *difference);
void vector_between(const NJS_VECTOR *from, const NJS_VECTOR *to,
                    NJS_VECTOR *out);

/* steps that call the task's draw. object_step_draw_range then has
 * placement_try_leave end the task once it's out of range */
void object_step_draw_range(struct task *task);
void object_step_draw(struct task *task);
/* draw the object of the work's MODEL vars, with their texlist, at the
 * work's pos and angles. object_draw_model_scaled scales it by work->scl.
 * object_draw_action draws the action of the ACTION vars instead, and moves
 * their frame on by one each draw, back to 0 at the motion's end */
void object_draw_model(struct task *task);
void object_draw_model_scaled(struct task *task);
void object_draw_action(struct task *task);
/* report a shape that hurts the task on two calls running. it holds the
 * touch in a bit of work->tick, so the task must keep no count there.
 * - a call that finds the task hurt, with no touch held, holds the shape
 *   and its center in touch and returns 0.
 * - if the same shape hurts the task on the next call, that call reports
 *   it. it sets touch->moved to how far the shape moved, and touch->by.owner
 *   to its task. it returns 1 << that task's character, read as a
 *   player's.
 * - any other call drops the touch and returns 0. the call after a report
 *   is one, as touch->by then holds an owner, which never matches a shape.
 *   so at most every third call reports */
int object_touch_moved(struct task_work *work, struct touch *touch);

Float vector_dot(NJS_VECTOR *first, NJS_VECTOR *second);
void vector_cross(NJS_VECTOR *first, NJS_VECTOR *second, NJS_VECTOR *out);
Float vector_length(NJS_VECTOR *vector);
/* the angles that turn z, y or x to point along vector */
void vector_aim_z(NJS_VECTOR *vector, Angle *ang_x, Angle *ang_y);
void vector_aim_y(NJS_VECTOR *vector, Angle *ang_x, Angle *ang_z);
void vector_aim_x(NJS_VECTOR *vector, Angle *ang_y, Angle *ang_z);
/* set out to vector reflected off the plane with the unit normal normal.
 * returns how squarely vector meets the plane: the cosine between vector
 * and normal without its sign, 1 head on and 0 along the plane. vector
 * must not be zero */
Float vector_reflect(NJS_VECTOR *vector, NJS_VECTOR *normal, NJS_VECTOR *out);

/* debug draws: an arrow from pos along vector and as long, or
 * marker_object at pos, scaled by a tenth of size. arrow_draw stretches an
 * arrow model that lies along z, and arrow_draw_y one that lies along y.
 * each draws nothing while frame_step is 0 */
void arrow_draw(NJS_POINT3 *pos, NJS_VECTOR *vector);
void arrow_draw_y(NJS_POINT3 *pos, NJS_VECTOR *vector);
void marker_draw(NJS_POINT3 *pos, Float size);

/* a kind picked at random from table, pairs of a weight and a kind as
 * Floats that end at a weight of 0. a kind's chance is its weight, out of
 * 1. returns 0 when the pick passes every weight */
int item_pick(Float *table);
/* leave what kind names at work's pos. any other kind leaves nothing.
 * returns TRUE */
Bool item_drop(struct task_work *work, enum drop_kind kind);
/* TRUE when point lies inside the convex polygon of count corners, which
 * run anticlockwise about normal. point must lie in the polygon's plane.
 * count must be 7 at most: the corners' offsets and the first again fill a
 * table of 8 */
Bool polygon_contains(NJS_POINT3 *point, NJS_POINT3 *corners,
                      NJS_VECTOR *normal, int count);
/* copy count words from from to to, the first word first */
void words_copy(Sint32 *from, Sint32 *to, int count);
/* set pos and ang to where the player stood steps back, as
 * player_get_history gives them. then move pos up half the character's
 * height along its own y, to the middle of its body. returns FALSE, moving
 * nothing, where player_get_history gives FALSE, else TRUE */
Bool player_get_middle(Uint8 player, int steps, NJS_POINT3 *pos,
                       struct angles *ang);

/* forget the mirrors, or count mirror_add's calls since */
void mirror_clear(void);
int mirror_count(void);
/* make the quads of model's first meshset a mirror posed as work is, in
 * the next of MIRRORS slots. the meshset must hold MIRROR_QUADS quads at
 * most, as mirror_add copies them all. returns FALSE, filling no slot,
 * when the meshset isn't quads or the slots are taken. every call counts
 * as a slot taken, even one that fails. a slot that a failed call took
 * keeps what it last held, and mirror_draw still draws in it. past MIRRORS
 * calls, mirror_draw reads beyond the slots */
Bool mirror_add(struct task_work *work, NJS_MODEL *model);
/* call draw again, reflected in each mirror quad whose face the task's pos
 * lies in front of, within its edges. markers show the point of the quad
 * nearest the task, and the reflected pos */
void mirror_draw(struct task *task, void (*draw)(struct task *task));

/* lend a world object for model, posed as the task, and add it to the
 * world. returns it, or NULL with the task marked when the pool is out */
NJS_OBJECT *object_add_world_model(struct task *task, NJS_MODEL *model);
/* set WORLD_TESTED while a player is within the model's radius plus 20 of
 * the task, else clear it. while a player is that near, object_world_follow
 * also poses object as the task. a task object_add_world_model marked
 * changes nothing */
void object_world_follow(struct task_work *work, NJS_OBJECT *object);
void object_world_test(struct task_work *work, NJS_OBJECT *object);
/* clear or set SHAPE_OFF on each of the task's shapes, which the collision
 * checks then test or pass over */
void collision_shapes_on(struct task_work *work);
void collision_shapes_off(struct task_work *work);
/* add step to *frame, and wrap it once into the action's motion, from 0
 * to its nbFrame. step must lie within nbFrame of 0, or *frame can end
 * outside */
void action_advance(NJS_ACTION *action, Float *frame, Float step);
/* a bit, 1 << n, for each of players 0 and 1 whose first collision shape
 * is centered nearer pos than range, a squared distance */
int players_near(NJS_POINT3 *pos, Float range);
/* the distance between the points, squared, as system/math.h's
 * point_distance_squared gives it */
Float vector_distance2(NJS_POINT3 *from, NJS_POINT3 *to);
/* set the jump's speed for a leap from pos to target in steps steps. its
 * level speed covers the distance in that time. its y speed, (dy + gravity
 * * steps * steps / 2) / steps, rises dy in that time under a steady pull
 * of gravity */
void jump_aim(struct jump *jump);
/* count a step off the jump, and return JUMP_FLYING while steps were left.
 * then return JUMP_ON_TARGET while pos, as the caller has left it, is
 * within 2 of target, else JUMP_OFF_TARGET */
enum jump_state jump_landed(struct jump *jump);
/* normalize first and second in place, as vector_normalize does, and
 * return the cosine between them */
Float vector_cos(NJS_VECTOR *first, NJS_VECTOR *second);
/* TRUE in a stage numbered below STAGE_CHAOS0: the hedgehog hammer, which
 * is STAGE_PRACTICE, or an action stage. else FALSE */
Bool stage_before_bosses(void);
/* make vector a unit vector, and return its length before. a vector
 * shorter than 0.000001 becomes the unit vector along y */
Float vector_normalize(NJS_VECTOR *vector);
void object_idle(struct task *task);

#endif
