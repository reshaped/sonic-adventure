/* O ROOF turning roof that carries a player around */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg03/roof.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT roof_object;

/* the roof's range for placement_try_leave_by, squared */
#define ROOF_RANGE (1010.0F * 1010.0F)

/* the reach of the roof's collision record. the world tests the roof only
 * while player 0 or 1 is within this distance of it */
#define ROOF_REACH 186.6F

#define ROOF_WORLD_FLAGS (WORLD_LARGE | WORLD_TURNED | WORLD_MOVES)

struct roof_vars {
    NJS_OBJECT *world; /* the roof's copy in the collision world */
    NJS_OBJECT *object;
};

#define VARS(work) ((struct roof_vars *)&(work)->vars)
/* the roof's turn a step about y, in degrees */
#define ROOF_SPEED(work) ((work)->scl.x)

enum roof_mode {
    ROOF_INIT,
    ROOF_TURN,
    ROOF_IDLE /* this file never sets it */
};

static struct collision_shape roof_shape[1] = {
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 76.5F, 0.0F},
     50.0F,
     76.5F,
     0.0F,
     {0},
     0,
     NULL},
};

static void roof_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        Angle ang;

        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

static void roof_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
    task_kill(task);
}

/* turn the roof by ROOF_SPEED a step, and keep its copy in the collision world
 * at its pose. the roof's force carries a touching player round its axis by
 * the same turn. the player must have PLAYER_ON_LAND set */
void roof_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;
    int turn;
    Sint8 player;

    if (placement_try_leave_by(task, ROOF_RANGE)) {
        return;
    }
    if (edit_mode) {
        Angle ang;

        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&roof_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case ROOF_INIT:
            work->mode = ROOF_TURN;
            VARS(work)->object = &roof_object;
            collision_init(task, roof_shape, COLLISION_OBJECT, 1, 1);
            task->work->collision->radius = ROOF_REACH;
            task->destructor = roof_destroy;
            task->draw = roof_draw;
            VARS(work)->world = world = world_object_alloc();
            world->evalflags = VARS(work)->object->evalflags &
                               ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->model = VARS(work)->object->model;
            world->child = VARS(work)->object->child;
            world->sibling = VARS(work)->object->sibling;
            world_add(ROOF_WORLD_FLAGS, task, world);
            break;
        case ROOF_TURN:
            world = VARS(work)->world;
            world->pos[0] = work->pos.x;
            world->pos[1] = work->pos.y;
            world->pos[2] = work->pos.z;
            world->ang[0] = work->ang.x;
            world->ang[1] = work->ang.y;
            world->ang[2] = work->ang.z;
            if (player_in_range(&work->pos, ROOF_REACH)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            turn = ROOF_SPEED(work) * ANGLE_PER_DEGREE;
            work->ang.y += turn;
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                player = PLAYER_VARS(work->collision->hit->owner->work)->player;
                if (player_work[player]->flags & PLAYER_ON_LAND) {
                    NJS_VECTOR from;
                    NJS_VECTOR to;

                    from.x = work->pos.x - player_work[player]->pos.x;
                    from.y = 0.0F;
                    from.z = work->pos.z - player_work[player]->pos.z;
                    njPushMatrix(&_unit_matrix_);
                    TURN(njRotateY, turn);
                    njCalcVector(NULL, &from, &to);
                    njPopMatrix(1);
                    force->turn.y = -turn;
                    force->speed.x = from.x - to.x;
                    force->speed.z = from.z - to.z;
                }
            }
            collision_register(work);
            roof_draw(task);
            break;
        case ROOF_IDLE:
            break;
        }
    }
}
