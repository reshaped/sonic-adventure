/* O ZAKO and O ZAKO_B, pieces of rubble, and O BPOLE, a pole. each is
 * solid, in a model its set entry picks. O ZAKO_B blows away once E-102
 * shoots its dynamite */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "object/common.h"
#include "object/placement.h"
#include "object/target.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg05/rubble.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT pole_0_object;
extern NJS_OBJECT pole_1_object;

/* object/placement.c's set files, by stage and act. set_files[0][1] points
 * at SET_AREA_1, where a stage loads its act 1's set file */
extern struct set_file *set_files[STAGE_COUNT][STAGE_ACTS];

/* what a piece adds to each set entry scale it draws at */
#define BASE_SCALE 0.2F

/* the index in rubble_objects or pole_objects of the model drawn. each draw
 * sets it from scale z */
#define MODEL_INDEX(work) ((work)->submode)
/* each step, a flying piece turns 16 times this many angle units. it turns
 * one way about y and the other way about x */
#define SPIN(work) ((work)->tick)

/* a flying piece's speed, over the bytes of WORLD_MODEL(work) */
struct rubble_flight {
    NJS_VECTOR speed;
};

#define FLIGHT(work) ((struct rubble_flight *)&(work)->vars)

enum rubble_mode { RUBBLE_INIT, RUBBLE_STAND, RUBBLE_WAIT, RUBBLE_FLY };

/* the id of O DYNAMITE's set entries, its index in mountain_object_list */
#define DYNAMITE_KIND 21

#define FLY_STEPS 60
/* each step, a flying piece keeps FLY_DRAG of its speed along x and z. it
 * gains FLY_GRAVITY of speed down, and keeps FLY_SHRINK of its scale y */
#define FLY_DRAG 0.95F
#define FLY_GRAVITY 0.2F
#define FLY_SHRINK 0.95F
/* the share of the piece's speed that its flame takes */
#define FLAME_SPEED 0.7F

/* the world tests a copy only while a player is within its model's radius,
 * scaled, plus this margin */
#define NEAR_MARGIN 20.0F
#define DRAW_RADIUS 50.0F

static NJS_OBJECT *rubble_objects[6] = {
    &rubble_0_object, &rubble_1_object, &rubble_2_object,
    &rubble_3_object, &rubble_4_object, &rubble_5_object,
};
/* the flame a flying piece trails, on three steps of every four.
 * rubble_b_step sets its pos and speed */
static struct dust_params rubble_flame = {
    0.8F,
    0.038F,
    0.5F,
    0.06F,
    0.01F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static NJS_OBJECT *pole_objects[2] = {&pole_0_object, &pole_1_object};

static void rubble_draw(struct task *task);
static void rubble_destroy(struct task *task);
static void rubble_add_world(struct task *task);
static void rubble_sync_world(struct task *task);
static void rubble_b_draw(struct task *task);
static void rubble_b_destroy(struct task *task);
static void rubble_b_add_world(struct task *task);
static void rubble_b_sync_world(struct task *task);
static void pole_draw(struct task *task);
static void pole_destroy(struct task *task);
static void pole_add_world(struct task *task);
static void pole_sync_world(struct task *task);

void rubble_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        rubble_draw(task);
    } else {
        switch (work->mode) {
        case RUBBLE_INIT:
            rubble_draw(task);
            rubble_add_world(task);
            task->draw = rubble_draw;
            task->destructor = rubble_destroy;
            work->mode = RUBBLE_STAND;
            break;
        case RUBBLE_STAND:
            rubble_draw(task);
            rubble_sync_world(task);
            break;
        }
    }
}

static void rubble_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Sint8 model;
    Angle ang;

    model = (int)work->scl.z;
    if (model < 0 || model > 5) {
        model = 0;
    }
    MODEL_INDEX(work) = model;
    object = rubble_objects[MODEL_INDEX(work)];
    WORLD_MODEL(work)->model = object;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        njScale(NULL, work->scl.x + BASE_SCALE, work->scl.y + BASE_SCALE,
                work->scl.x + BASE_SCALE);
        draw_divide_light(work->scl.x + BASE_SCALE);
        draw_tree(object);
        draw_reset_light();
        njPopMatrix(1);
    }
}

static void rubble_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}

/* unless the set editor holds the piece, put a copy of the model in the world.
 * the copy scales as the piece draws. the model's radius, unscaled, picks its
 * world size */
static void rubble_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    Uint32 flags;

    if (!placement_editing(task)) {
        WORLD_MODEL(work)->world = world = world_object_alloc();
        world->evalflags = NJD_EVAL_BREAK;
        world->scl[0] = work->scl.x + BASE_SCALE;
        world->scl[1] = work->scl.y + BASE_SCALE;
        world->scl[2] = work->scl.x + BASE_SCALE;
        world->ang[0] = work->ang.x;
        world->ang[1] = work->ang.y;
        world->ang[2] = work->ang.z;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = WORLD_MODEL(work)->model->model;
        world->child = WORLD_MODEL(work)->model->child;
        world->sibling = WORLD_MODEL(work)->model->sibling;
        /* @hack spelling: flags set before the tests, which leaves the first
         * arm empty; set in that arm, flags takes another register and 35 of
         * 100 words differ */
        flags = WORLD_SOLID | WORLD_HUGE;
        if (world->model->r > 20.0F) {
        } else if (world->model->r > 10.0F) {
            flags = WORLD_SOLID | WORLD_LARGE;
        } else if (world->model->r > 3.0F) {
            flags = WORLD_SOLID | WORLD_MEDIUM;
        } else {
            flags = WORLD_SOLID | WORLD_SMALL;
        }
        world_add(flags, task, world);
    }
}

/* move the copy to the rubble's place and angles, and set WORLD_TESTED only
 * while a player is near. the near test scales the model's radius by the
 * larger of scales x and y */
static void rubble_sync_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world = WORLD_MODEL(work)->world;
    Float scale;

    if (world != NULL) {
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->ang[0] = work->ang.x;
        world->ang[1] = work->ang.y;
        world->ang[2] = work->ang.z;
        if (work->scl.x > work->scl.y) {
            scale = work->scl.x;
        } else {
            scale = work->scl.y;
        }
        if (player_in_range(&work->pos,
                            (world->model->r * scale) + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

/* the first O DYNAMITE in act 1's set file whose scale x truncates to id.
 * returns its index, and puts its position in *x, *y and *z. returns -1
 * when there is none, and leaves them unwritten */
static int rubble_find_dynamite(int id, Float *x, Float *y, Float *z)
{
    struct set_file *file = set_files[0][1];
    struct set_entry *entry = (struct set_entry *)(file + 1);
    Uint32 i;
    int found = -1;

    for (i = 0; i < file->count; i++, entry++) {
        if (entry->id == DYNAMITE_KIND && (int)entry->scl.x == id) {
            found = i;
            *x = entry->pos.x;
            *y = entry->pos.y;
            *z = entry->pos.z;
            break;
        }
    }
    return found;
}

/* for E-102, the piece blows away once every target with its scale x as id is
 * shot down. it leaves the world and waits 4 to 7 steps. it then flies away
 * from that id's O DYNAMITE in act 1's set file. it flies FLY_STEPS - 1 steps.
 * its placement then makes no more */
void rubble_b_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR away;
    Float speed;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        rubble_b_draw(task);
    } else {
        switch (work->mode) {
        case RUBBLE_INIT:
            rubble_b_draw(task);
            rubble_b_add_world(task);
            task->draw = rubble_b_draw;
            task->destructor = rubble_b_destroy;
            work->mode = RUBBLE_STAND;
            break;
        case RUBBLE_STAND:
            if (get_character() == CHARACTER_E102 &&
                target_cleared((int)work->scl.x)) {
                work->mode = RUBBLE_WAIT;
                TASK_TIMER(work) = (rand() * (1.0F / 8192.0F)) + 4.0F;
                /* @bug memory: this gives the copy back to the pool (BSR at
                 * 0x0c910d0a), and WORLD_MODEL(work)->world still points at
                 * it. rubble_b_sync_world below stores the piece's place and
                 * angles into it (from 0x0c911198). the destructor gives it
                 * back again (0x0c910fd6), though the pool may have lent it to
                 * another task by then. once the piece flies, that word is
                 * speed.x, whose bits the destructor passes as the copy */
                rubble_b_destroy(task);
            }
            rubble_b_draw(task);
            rubble_b_sync_world(task);
            break;
        case RUBBLE_WAIT:
            TASK_TIMER(work)--;
            if (TASK_TIMER(work) == 0) {
                work->mode = RUBBLE_FLY;
                TASK_TIMER(work) = FLY_STEPS;
                /* @bug uninitialized: with no such O DYNAMITE, away keeps the
                 * stack's old bytes (FMOV.S at 0x0c910d46), and the piece flies
                 * away from the point they make */
#ifdef GCC
                /* repairs the unset away the tag names: with no such
                 * O DYNAMITE, the piece flies straight up, as from a blast
                 * just below it. the find never fails in play: only E-102's
                 * pieces fly, and his act's set file, SET0501E.BIN, holds an
                 * O DYNAMITE for each id they take */
                away.x = work->pos.x;
                away.y = work->pos.y - 1.0F;
                away.z = work->pos.z;
#endif
                rubble_find_dynamite((int)work->scl.x, &away.x, &away.y,
                                     &away.z);
                away.x = work->pos.x - away.x;
                away.y = work->pos.y - away.y;
                away.z = work->pos.z - away.z;
                njUnitVector(&away);
                speed = (rand() * (1.0F / 16384.0F)) + 6.0F;
                FLIGHT(work)->speed.x = away.x * speed;
                FLIGHT(work)->speed.y = away.y * speed;
                FLIGHT(work)->speed.z = away.z * speed;
                SPIN(work) = (rand() * (1.0F / 2048.0F)) + 2.0F;
                sound_play_at(SE_RUBBLE_BLAST, 0, 0, 64, work->pos.x,
                              work->pos.y, work->pos.z);
            }
            rubble_b_draw(task);
            break;
        case RUBBLE_FLY:
            TASK_TIMER(work)--;
            if (TASK_TIMER(work) == 0) {
                placement_kill(task);
                break;
            }
            FLIGHT(work)->speed.x *= FLY_DRAG;
            FLIGHT(work)->speed.z *= FLY_DRAG;
            FLIGHT(work)->speed.y -= FLY_GRAVITY;
            work->pos.x += FLIGHT(work)->speed.x;
            work->pos.y += FLIGHT(work)->speed.y;
            work->pos.z += FLIGHT(work)->speed.z;
            work->ang.y += SPIN(work) << 4;
            work->ang.x -= SPIN(work) << 4;
            work->scl.y *= FLY_SHRINK;
            if (TASK_TIMER(work) & 3) {
                rubble_flame.pos = work->pos;
                rubble_flame.speed.x = FLIGHT(work)->speed.x * FLAME_SPEED;
                rubble_flame.speed.y = FLIGHT(work)->speed.y * FLAME_SPEED;
                rubble_flame.speed.z = FLIGHT(work)->speed.z * FLAME_SPEED;
                flame_create_from(&rubble_flame);
            }
            rubble_b_draw(task);
            break;
        }
    }
}

static void rubble_b_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Sint8 model;
    Angle ang;
    Float scale;

    model = (int)work->scl.z;
    if (model < 0 || model > 5) {
        model = 0;
    }
    MODEL_INDEX(work) = model;
    object = rubble_objects[MODEL_INDEX(work)];
    /* @bug logic: once the piece flies, FLIGHT(work)->speed.y shares
     * these bytes, and each draw stores the model's address over it
     * (0x0c910f26). the address reads as a float near 0, so a flying piece
     * sinks FLY_GRAVITY a step, whatever speed.y the blow gave it */
    WORLD_MODEL(work)->model = object;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        scale = work->scl.y + BASE_SCALE;
        njScale(NULL, scale, scale, scale);
        draw_divide_light(work->scl.y + BASE_SCALE);
        draw_tree(object);
        draw_reset_light();
        njPopMatrix(1);
    }
}

static void rubble_b_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}

/* rubble_add_world for O ZAKO_B, whose copy scales by scale y plus
 * BASE_SCALE on every axis */
static void rubble_b_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    Uint32 flags;

    if (!placement_editing(task)) {
        WORLD_MODEL(work)->world = world = world_object_alloc();
        world->evalflags = NJD_EVAL_BREAK;
        world->scl[0] = work->scl.y + BASE_SCALE;
        world->scl[1] = work->scl.y + BASE_SCALE;
        world->scl[2] = work->scl.y + BASE_SCALE;
        world->ang[0] = work->ang.x;
        world->ang[1] = work->ang.y;
        world->ang[2] = work->ang.z;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = WORLD_MODEL(work)->model->model;
        world->child = WORLD_MODEL(work)->model->child;
        world->sibling = WORLD_MODEL(work)->model->sibling;
        /* @hack spelling: flags set before the tests, which leaves the first
         * arm empty; set in that arm, flags takes another register and 35 of
         * 100 words differ */
        flags = WORLD_SOLID | WORLD_HUGE;
        if (world->model->r > 20.0F) {
        } else if (world->model->r > 10.0F) {
            flags = WORLD_SOLID | WORLD_LARGE;
        } else if (world->model->r > 3.0F) {
            flags = WORLD_SOLID | WORLD_MEDIUM;
        } else {
            flags = WORLD_SOLID | WORLD_SMALL;
        }
        world_add(flags, task, world);
    }
}

/* rubble_sync_world for O ZAKO_B, whose near test scales the radius by
 * scale y */
static void rubble_b_sync_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world = WORLD_MODEL(work)->world;
    Float scale;

    if (world != NULL) {
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->ang[0] = work->ang.x;
        world->ang[1] = work->ang.y;
        world->ang[2] = work->ang.z;
        scale = work->scl.y;
        if (player_in_range(&work->pos,
                            (world->model->r * scale) + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

void pole_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        pole_draw(task);
    } else {
        switch (work->mode) {
        case RUBBLE_INIT:
            pole_draw(task);
            pole_add_world(task);
            task->draw = pole_draw;
            task->destructor = pole_destroy;
            work->mode = RUBBLE_STAND;
            break;
        case RUBBLE_STAND:
            pole_draw(task);
            pole_sync_world(task);
            break;
        }
    }
}

static void pole_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Sint8 model;
    Angle ang;

    model = (int)work->scl.z;
    if (model < 0 || model > 1) {
        model = 0;
    }
    MODEL_INDEX(work) = model;
    object = pole_objects[MODEL_INDEX(work)];
    WORLD_MODEL(work)->model = object;
    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_tree(object);
        njPopMatrix(1);
    }
}

static void pole_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}

static void pole_add_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    WORLD_MODEL(work)->world = world = world_object_alloc();
    world->evalflags = NJD_EVAL_BREAK;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = WORLD_MODEL(work)->model->model;
    world->child = WORLD_MODEL(work)->model->child;
    world->sibling = WORLD_MODEL(work)->model->sibling;
    world_add(WORLD_SOLID | WORLD_HUGE, task, world);
}

/* move the copy to the pole's place and angles, and set WORLD_TESTED only
 * while a player is near */
static void pole_sync_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world = WORLD_MODEL(work)->world;

    if (world != NULL) {
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->ang[0] = work->ang.x;
        world->ang[1] = work->ang.y;
        world->ang[2] = work->ang.z;
        if (player_in_range(&work->pos, world->model->r + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}
