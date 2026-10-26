/* O PIT trapdoor that shakes, then swings open */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg03/pit.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT pit_object; /* one leaf, whose child is the other */

/* the pit shakes for PIT_SHAKE_STEPS steps more than its set entry's scale x,
 * then opens */
#define PIT_SHAKE_STEPS 30.0F
/* the shake raises dust at random over a box DUST_SPREAD_X by DUST_SPREAD_Z.
 * the box runs from the pit toward -x, and is centered on it along z */
#define DUST_SPREAD_X 45.0F
#define DUST_SPREAD_Z 60.0F

#define PIT_NEAR 65.5F
/* how far the pit's collision record reaches from its position */
#define PIT_REACH 45.0F
/* the leaves swing open PIT_OPEN_SPEED a step, to PIT_OPEN */
#define PIT_OPEN_SPEED 0x100
#define PIT_OPEN NJM_DEG_ANG(90)
#define HALF_TURN NJM_DEG_ANG(180)

struct pit_vars {
    NJS_OBJECT *world; /* the leaves' world object, which players stand on */
    NJS_OBJECT *object;
};

#define VARS(work) ((struct pit_vars *)&(work)->vars)
/* the steps left to shake */
#define PIT_SHAKE(work) ((work)->scl.x)
/* the pit's height at rest, which the shake jitters about */
#define PIT_REST_Y(work) ((work)->scl.y)
/* above 0, the pit draws a second copy a half turn about y, its reflection */
#define PIT_MIRRORED(work) ((work)->scl.z)

enum pit_mode { PIT_INIT, PIT_SHUT, PIT_SHAKING, PIT_OPENING };

static struct collision_shape pit_shape = {
    SHAPE_RECTANGLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {-22.5F, 4.0F, 0.0F},
    22.5F,
    2.0F,
    30.0F,
    {0},
    0,
    NULL,
};

static struct dust_params pit_dust = {
    0.7F,
    0.07F,
    0.1F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.32F, 0.8F, 0.8F, 1.0F},
};

static void pit_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
    task_kill(task);
}

/* have the world test the leaves only while a player is within PIT_NEAR of
 * the pit */
static void pit_touch(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, PIT_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* on a frame's first step, draw the first leaf swung about z by work->ang.z.
 * its child swings back as far. with PIT_MIRRORED above 0, draw both again a
 * half turn about y */
static void pit_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *leaf = VARS(work)->object->child;
    Angle ang;

    if (frame_step != 0) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslateV(NULL, (NJS_VECTOR *)VARS(work)->object->pos);
    njPushMatrix(NULL);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    draw_model(VARS(work)->object->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)leaf->pos);
    TURN_VIA(njRotateZ, -work->ang.z, ang);
    draw_model(leaf->model);
    njPopMatrix(1);
    njPopMatrix(1);
    if (PIT_MIRRORED(work) > 0.0F) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y + HALF_TURN, ang);
        njTranslateV(NULL, (NJS_VECTOR *)VARS(work)->object->pos);
        njPushMatrix(NULL);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_model(VARS(work)->object->model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)leaf->pos);
        TURN_VIA(njRotateZ, -work->ang.z, ang);
        draw_model(leaf->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

/* a player standing on the pit makes it shake, raising dust and vibrating
 * pad 0, then swing open. while the set editor holds the pit, the step labels
 * scale x and scale z */
void pit_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    NJS_OBJECT *object;
    Sint8 player;
    NJS_POINT3 pos;

    if (!placement_try_leave(task)) {
        object = &pit_object;
        if (edit_mode != 0) {
            VARS(work)->object = object;
            pit_draw(task);
            if (placement_editing(task)) {
                njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> PERIOD TO OPEN +");
                njPrintD(NJM_LOCATION(36, 35), 30, 2);
                if (PIT_MIRRORED(work) > 0.0F) {
                    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z),
                             "> DISPLAY IN THE MIRROR");
                } else {
                    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z),
                             "> DISAPEAR IN THE MIRROR");
                }
            }
        } else {
            switch (work->mode) {
            case PIT_INIT:
                work->mode = PIT_SHUT;
                work->ang.x = work->ang.z = 0;
                PIT_SHAKE(work) += PIT_SHAKE_STEPS;
                PIT_REST_Y(work) = work->pos.y;
                VARS(work)->object = object;
                task->destructor = pit_destroy;
                task->draw = pit_draw;
                /* @hack redundant: the always-true test around the call, which
                 * keeps the switch's constant 1 in R4 (0x0c906834); as a plain
                 * call, 1 takes R12, a saved register, and 120 of 291 words
                 * differ */
                /* NOLINTNEXTLINE(clang-diagnostic-literal-conversion) */
                (void)(2.0F && (collision_init(task, &pit_shape,
                                               COLLISION_OBJECT, 1, 1),
                                1));
                task->work->collision->radius = PIT_REACH;
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
                world_add(WORLD_MEDIUM, task, world);
                break;
            case PIT_SHUT:
                if ((work->collision->flags & COLLISION_HIT) &&
                    work->collision->hit->kind == COLLISION_PLAYER) {
                    player =
                        PLAYER_VARS(work->collision->hit->owner->work)->player;
                    if (player_work[player]->flags & PLAYER_ON_LAND) {
                        work->mode = PIT_SHAKING;
                    }
                }
                pit_touch(task);
                collision_register(work);
                object_clear_touch(work, NULL);
                pit_draw(task);
                break;
            case PIT_SHAKING:
                PIT_SHAKE(work) -= 1.0F;
                if (PIT_SHAKE(work) < 0.0F) {
                    work->mode = PIT_OPENING;
                    work->flags &= ~WORLD_TESTED;
                } else {
                    work->pos.y =
                        PIT_REST_Y(work) + (rand() * RAND_SCALE) - 0.5F;
                    pos = work->pos;
                    pos.x -= rand() * RAND_SCALE * DUST_SPREAD_X;
                    pos.z += ((rand() * RAND_SCALE) - 0.5F) * DUST_SPREAD_Z;
                    pit_touch(task);
                    pit_dust.pos = pos;
                    dust_create_glow(&pit_dust);
                    vibration_start(0, 1, 48, 0);
                }
                pit_draw(task);
                break;
            case PIT_OPENING:
                work->ang.z =
                    angle_approach(work->ang.z, PIT_OPEN, PIT_OPEN_SPEED);
                pit_draw(task);
                break;
            }
        }
    }
}
