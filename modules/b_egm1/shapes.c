/* the Egg Hornet's collision shapes: which of its seven are on, and how they
 * turn */
#include <sg_xpt.h>

#include "b_egm1/shapes.h"
#include "collision/collision.h"
#include "system/task.h"

#define SHAPES 7

/* the shapes each enum shape_row turns on, by a nonzero byte. the flying
 * boss has sphere 3, at its middle, and its tail's; the landed boss has all
 * but sphere 3 */
static const Sint8 shape_rows[3][SHAPES] = {
    {0, 0, 0, 0, 0, 0, 0}, /* SHAPES_NONE */
    {0, 0, 0, 1, 1, 0, 0}, /* SHAPES_FLYING */
    {1, 1, 1, 0, 1, 1, 1}, /* SHAPES_LANDED */
};

/* the boss's seven spheres. 0 and 3 are at its middle, 1 and 2 at its
 * sides, and 4 at its tail; 5 and 6 lie between. an attack hurts a sphere
 * only when it exceeds the sphere's guard. an attack's two bits reach no
 * higher than 3. so GUARD(3) keeps all but 0 and 3 from harm */
/* clang-format off */
static struct collision_shape egg_hornet_shapes[SHAPES] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 22.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {28.0F, 0.0F, 0.0F}, 16.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-28.0F, 0.0F, 0.0F}, 16.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 24.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -88.0F}, 24.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {28.0F, 4.0F, -58.0F}, 18.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, GUARD(3) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-28.0F, 4.0F, -58.0F}, 18.0F, 0.0F, 0.0F, {0, 0, 0}, 0, NULL},
};
/* clang-format on */

void egg_hornet_shapes_turn_yzx(struct task_work *work, int on)
{
    int i;

    for (i = 0; i < SHAPES; i++) {
        if (!on) {
            work->collision->shapes[i].flags &= ~SHAPE_TURN_YZX;
        } else {
            work->collision->shapes[i].flags |= SHAPE_TURN_YZX;
        }
    }
}

void egg_hornet_shapes_enable(struct task_work *work, enum shape_row row)
{
    int i;

    for (i = 0; i < SHAPES; i++) {
        if (shape_rows[row][i]) {
            work->collision->shapes[i].flags &= ~SHAPE_OFF;
        } else {
            work->collision->shapes[i].flags |= SHAPE_OFF;
        }
    }
}

void egg_hornet_shapes_init(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, egg_hornet_shapes, COLLISION_ITEM, 1, SHAPES);
    egg_hornet_shapes_enable(work, SHAPES_NONE);
}
