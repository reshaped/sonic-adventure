/* helpers for the modules' simple objects: collision records, trees drawn
 * at a work's place, their copies in the collision world, and bounces on
 * the ground */
#ifndef OBJECT_SIMPLE_H
#define OBJECT_SIMPLE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

struct collision_shape;

/* a task's shapes and their count, for object_collision_init */
struct collision_list {
    const struct collision_shape *shapes;
    Sint32 count;
    /* 40 or 160 in each list that sets it. object_collision_init doesn't
     * read it */
    Float unk_08;
};

/* the record's kind for object_collision_init, and the argument it passes
 * collision_init as unused */
struct collision_kind {
    Uint8 kind; /* a COLLISION_ kind */
    Sint8 unused;
};

/* give the task its collision record: collision_init with list's shapes
 * and count, and kind's kind */
void object_collision_init(struct task *task, const struct collision_list *list,
                           const struct collision_kind *kind);
/* draw object's tree at work->pos, turned by work->ang about z, x and then
 * y. it draws with texlist, or with the stage's textures for NULL.
 * object_draw_tree_scaled also scales the tree by scale.
 * object_draw_tree_zyx scales it too, and turns it about z, y and then x */
void object_draw_tree(NJS_OBJECT *object, struct task_work *work,
                      NJS_TEXLIST *texlist);
void object_draw_tree_scaled(NJS_OBJECT *object, struct task_work *work,
                             Float scale, NJS_TEXLIST *texlist);
void object_draw_tree_zyx(NJS_OBJECT *object, struct task_work *work,
                          Float scale, NJS_TEXLIST *texlist);
/* a world object with source's model, child and sibling at a scale of 1, or
 * NULL when the pool has none to lend. its evalflags are source's without
 * NJD_EVAL_UNIT_POS and NJD_EVAL_UNIT_ANG. its pos and ang stay as the pool
 * lends them, for object_world_place to set */
NJS_OBJECT *object_world_copy(NJS_OBJECT *source);
/* set the world object's pos and ang to work's, and its scale to scale on
 * each axis */
void object_world_place(NJS_OBJECT *object, struct task_work *work,
                        Float scale);
/* put the world object in the world under flags and WORLD_SOLID. its size
 * goes by its model's radius times scale: WORLD_HUGE past 200, WORLD_LARGE
 * past 100, WORLD_MEDIUM past 30 and WORLD_SMALL otherwise.
 * object_world_free passes a non-null object to world_remove, then gives
 * it back to the pool */
void object_world_add_sized(struct task *task, NJS_OBJECT *object, Sint32 flags,
                            Float scale);
void object_world_free(struct task *task, NJS_OBJECT *object);
/* set WORLD_TESTED in work->flags while player 0 or 1 is within radius of
 * work->pos, and clear it otherwise. TRUE while one is, else FALSE */
Bool object_world_test_radius(struct task_work *work, Float radius);
/* zero motion->speed and motion->accel. each caller passes a task's force
 * instead, whose turn and speed lie where speed and accel do. the call
 * zeroes the force's turn and speed */
void object_force_clear(struct task_motion *motion);
/* move *pos a step at *speed, falling, and keep it offset above the ground.
 * a step that touches the ground scales speed's x and z by bounce's, and a
 * landing also reverses y, scaled by bounce->y. FALSE once *pos rests on
 * the ground with no speed across, else TRUE */
Bool object_bounce(NJS_POINT3 *pos, NJS_VECTOR *speed, NJS_VECTOR *bounce,
                   Float offset);
/* the points of the meshset's nbMesh strips, summed. each strip's count is
 * the low byte of its head, which its points' indices follow */
int meshset_count_points(NJS_MESHSET *meshset);

#endif
