/* O ROAD 0 to O ROAD 4: the Perfect Chaos fight's roads above the sea */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/road.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT road_0_object;
extern NJS_OBJECT road_1_object;
extern NJS_OBJECT road_2_object;
extern NJS_OBJECT road_3_object;
extern NJS_OBJECT road_4_object;
extern NJS_OBJECT road_1_ground_object;
extern NJS_OBJECT road_2_ground_object;
extern NJS_OBJECT road_3_ground_object;
extern NJS_OBJECT road_4_ground_object;

/* a road destroys itself once neither the camera nor player 0 is within 4010.
 * the range is squared, as placement_try_leave_by takes it */
#define ROAD_RANGE (4010.0F * 4010.0F)

/* the road's row of road_kinds, which each road's step sets before road_step
 * runs */
#define ROAD_KIND(work) ((work)->tick)

/* nothing sets ROAD_IDLE */
enum road_mode { ROAD_INIT, ROAD_STANDING, ROAD_IDLE };

struct road_kind {
    NJS_OBJECT *object;
    /* the model the collision world gets a copy of, or NULL for none */
    NJS_OBJECT *ground;
    Float radius; /* how near a player must be for the world to test the copy */
};

/* a row for each road, by the number its step sets. road 0 has no ground */
static struct road_kind road_kinds[5] = {
    {&road_0_object, NULL, 171.8F},
    {&road_1_object, &road_1_ground_object, 157.2F},
    {&road_2_object, &road_2_ground_object, 196.3F},
    {&road_3_object, &road_3_ground_object, 221.2F},
    {&road_4_object, &road_4_ground_object, 158.4F},
};

static void road_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(WORLD_MODEL(work)->model);
    njPopMatrix(1);
}

/* take the ground's copy out of the collision world, and give it back to the
 * world's pool. while the set editor holds the road, the copy stays in the
 * world. the pool doesn't get it back */
static void road_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
    task_destroy_children(task);
    task_kill(task);
}

/* in edit mode, draw the road, and label its set fields while the set editor
 * holds it. otherwise, put a copy of its ground in the collision world, posed
 * as the road. from the next step on, draw the road. the world tests the copy
 * only while a player is within the kind's radius */
static void road_step(struct task *task)
{
    struct task_work *work = task->work;
    Sint8 kind = ROAD_KIND(work);
    NJS_OBJECT *world;
    NJS_OBJECT *ground;

    if (placement_try_leave_by(task, ROAD_RANGE)) {
        return;
    }
    if (edit_mode) {
        WORLD_MODEL(work)->model = road_kinds[kind].object;
        road_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
        }
    } else {
        switch (work->mode) {
        case ROAD_INIT:
            work->mode = ROAD_STANDING;
            WORLD_MODEL(work)->model = road_kinds[kind].object;
            task->destructor = road_destroy;
            task->draw = road_draw;
            /* a road with no ground still takes an object from the collision
             * world's pool, and never returns it */
            world = world_object_alloc();
            ground = road_kinds[kind].ground;
            if (ground == NULL) {
                return;
            }
            WORLD_MODEL(work)->world = world;
            world->evalflags =
                ground->evalflags & ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = ground->model;
            world->child = ground->child;
            world->sibling = ground->sibling;
            world_add(WORLD_LARGE, task, world);
            break;
        case ROAD_STANDING:
            if (player_in_range(&work->pos, road_kinds[kind].radius)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            road_draw(task);
            break;
        case ROAD_IDLE:
            break;
        }
    }
}

void chaos7_road_0_step(struct task *task)
{
    ROAD_KIND(task->work) = 0;
    road_step(task);
}

void chaos7_road_1_step(struct task *task)
{
    ROAD_KIND(task->work) = 1;
    road_step(task);
}

void chaos7_road_2_step(struct task *task)
{
    ROAD_KIND(task->work) = 2;
    road_step(task);
}

void chaos7_road_3_step(struct task *task)
{
    ROAD_KIND(task->work) = 3;
    road_step(task);
}

void chaos7_road_4_step(struct task *task)
{
    ROAD_KIND(task->work) = 4;
    road_step(task);
}
