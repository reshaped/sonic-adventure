/* GOAL ZONE, PREVIOUS ZONE and NEXT ZONE, boxes that tell the race when a
 * player is in one. they draw only while the set editor is open */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/draw.h"
#include "debug/editor.h"
#include "minicart/race.h"
#include "minicart/zone.h"
#include "object/simple.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"

static void cart_zone_init(struct task *task, void *arg);
static void cart_zone_step(struct task *task);
static void cart_zone_draw(struct task *task);

/* the zones' task kind. its record, in task->motion, is one Sint8: the
 * race_zone the box reports */
static const struct task_desc cart_zone_desc = {sizeof(Sint8),  TASK_WORK,
                                                cart_zone_init, cart_zone_step,
                                                cart_zone_draw, NULL};
static const Sint8 cart_zones[3] = {ZONE_GOAL, ZONE_PREVIOUS, ZONE_NEXT};

/* the box each zone copies once init has set its half sizes to the zone's
 * scale. it turns about y and pushes nothing, and only the shapes of
 * players and attacks meet it */
static struct collision_shape cart_zone_shape = {
    SHAPE_RECTANGLE | SHAPE_IGNORE_ENEMIES | SHAPE_IGNORE_OBJECTS |
        SHAPE_IGNORE_OTHERS,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    0.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL};
static const struct collision_list cart_zone_shapes = {&cart_zone_shape, 1,
                                                       40.0F};
static const struct collision_kind cart_zone_kind = {COLLISION_OBJECT, 1};

void goal_zone_step(struct task *task)
{
    task_apply_desc(task, &cart_zone_desc, (void *)&cart_zones[ZONE_GOAL]);
}

void previous_zone_step(struct task *task)
{
    task_apply_desc(task, &cart_zone_desc, (void *)&cart_zones[ZONE_PREVIOUS]);
}

void next_zone_step(struct task *task)
{
    task_apply_desc(task, &cart_zone_desc, (void *)&cart_zones[ZONE_NEXT]);
}

static void cart_zone_init(struct task *task, void *arg)
{
    struct task_work *work;
    NJS_POINT3 *scl;
    struct collision_shape *shape;
    Sint8 *zone;

    work = task->work;
    scl = &work->scl;
    shape = &cart_zone_shape;
    zone = (Sint8 *)task->motion;
    shape->radius = scl->x;
    shape->size_y = scl->y;
    shape->size_z = scl->z;
    object_collision_init(task, &cart_zone_shapes, &cart_zone_kind);
    *zone = *(Sint8 *)arg;
}

/* draw the box. then report a player's touch to the race, and register the
 * box for this step's checks. while the set editor holds the zone, size the
 * box to its scale instead */
static void cart_zone_step(struct task *task)
{
    struct task_work *work;
    Sint8 *zone;
    struct collision_work *collision;
    struct collision_work *hit;
    NJS_POINT3 *scl;
    struct collision_shape *shape;

    cart_zone_draw(task);
    work = task->work;
    zone = (Sint8 *)task->motion;
    if (placement_editing(task)) {
        shape = work->collision->shapes;
        scl = &work->scl;
        shape->radius = scl->x;
        shape->size_y = scl->y;
        shape->size_z = scl->z;
    } else {
        collision = work->collision;
        if (collision->flags & COLLISION_HIT) {
            hit = collision->hit;
            if (hit != NULL) {
                /* @hack spelling: the kind tested by a switch of one case,
                 * which tests it with CMP/EQ #0,R0 (0x0c90c42e); as an if, SHC
                 * tests it with TST R3,R3, cart_zone_step runs 2 bytes short
                 * and 24 of its 58 words differ */
                switch (hit->kind) {
                case COLLISION_PLAYER:
                    race_enter_zone(*zone);
                }
            }
        }
        collision_register(work);
    }
}

/* while the set editor is open, draw the box on a frame's first step */
static void cart_zone_draw(struct task *task)
{
    if (edit_mode) {
        if (frame_step == 0) {
            collision_draw(task->work->collision);
        }
    }
}
