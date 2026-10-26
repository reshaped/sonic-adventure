/* act 0's deep water beside the upper pool: a surface the collision world
 * holds as water. once the water below has risen, the surface rides above
 * player 0, no higher than the pool. once that water is full, the surface
 * settles at that water's level and draws */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stg12/deep_water.h"
#include "stg12/texlists.h"
#include "stg12/upper_pool.h"
#include "stg12/water.h"
#include "system/frame.h"
#include "system/task.h"

extern NJS_OBJECT shelter_lower_water_4_object;

/* @hack type: int, not stage.h's Uint16, so the step compares the result at
 * 0x0c910750 without extending it; as stage.h declares it, 7 of
 * shelter_deep_water_step's 209 words and 1 of the 109 words of its pool and
 * pad differ */
extern int stage_get_stage_act(void);

#define WATER_WORLD_FLAGS (WORLD_LARGE | WORLD_MOVES | WORLD_WATER)

#define DEEP_X 750.0F
#define DEEP_Z (-1026.5F)

/* how far below 0 the water's floor lies */
#define FLOOR_DEPTH 49.0F
/* the collision world tests the water while player 0 is within a cylinder of
 * this radius. the cylinder runs from 10 below the floor to 30 above the
 * surface */
#define NEAR 135.0F
/* how far above player 0 the surface rides until the water below is full */
#define ABOVE 80.0F

/* the textures the surface cycles through once the water below is full. each
 * shows while work->tick counts from 0 to TEXTURE_LAST_TICK, for three
 * steps */
#define TEXTURES 14
#define TEXTURE_LAST_TICK 2

/* DEEP_WAIT waits for the water below to rise, and DEEP_RISEN for it to be
 * full. the deep water draws only in DEEP_FULL */
enum shelter_deep_water_mode { DEEP_WAIT, DEEP_RISEN, DEEP_FULL };

/* TASK_TIMER(work) holds the stage and act the water was made for.
 * work->submode holds the texture's index, and work->tick the steps it has
 * shown */
struct shelter_deep_water_vars {
    NJS_OBJECT *world;
    /* the middle of the cylinder player 0 must be in, once full, and its
     * reach above and below the middle */
    Float middle;
    Float height;
};

#define VARS(work) ((struct shelter_deep_water_vars *)&(work)->vars)

static void shelter_deep_water_step(struct task *task);
static void shelter_deep_water_draw(struct task *task);
static void shelter_deep_water_destroy(struct task *task);

/* the quad the water adds to the collision world */
static NJS_MATERIAL shelter_deep_water_materials[1] = {
#include "stg12/deep_water/shelter_deep_water_materials.inc"
};

static Sint16 shelter_deep_water_meshes[4] = {
#include "stg12/deep_water/shelter_deep_water_meshes.inc"
};

static NJS_MESHSET shelter_deep_water_meshsets[1] = {
#include "stg12/deep_water/shelter_deep_water_meshsets.inc"
};

static NJS_POINT3 shelter_deep_water_points[4] = {
#include "stg12/deep_water/shelter_deep_water_points.inc"
};

static NJS_VECTOR shelter_deep_water_normals[4] = {
#include "stg12/deep_water/shelter_deep_water_normals.inc"
};

static NJS_MODEL shelter_deep_water_model = {
#include "stg12/deep_water/shelter_deep_water_model.inc"
};

/* @hack type: the object declared as an array, so SHC pools its address, at
 * 0x0c910778, and the start reads its model through it at 0x0c91070c.
 * declared as one, SHC pools the model field's address in its place. then 7
 * of shelter_deep_water_start's 91 words differ, as does 1 of the 109 words
 * of the pool and pad inside shelter_deep_water_step */
static NJS_OBJECT shelter_deep_water_object[1] = {
#include "stg12/deep_water/shelter_deep_water_object.inc"
};

void shelter_deep_water_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->mode = DEEP_WAIT;
    TASK_TIMER(work) = stage_get_stage_act();
    work->tick = 0;
    work->submode = 0;
    work->scl.y = 0.0F;
    work->pos.x = DEEP_X;
    work->pos.y = shelter_upper_pool_level;
    work->pos.z = DEEP_Z;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    VARS(work)->world = world = world_object_alloc();
    world->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = 0;
    world->ang[1] = 0;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = shelter_deep_water_object->model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WATER_WORLD_FLAGS, task, world);
    work->flags &= ~WORLD_TESTED;
    task->step = shelter_deep_water_step;
    task->destructor = shelter_deep_water_destroy;
}

/* until the water below has risen, the collision world leaves the water
 * untested. then the surface rides ABOVE over player 0, no higher than the
 * upper pool. once the water below is full, the surface stays at that water's
 * level, and the water draws */
static void shelter_deep_water_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 player;

    if (TASK_TIMER(work) != stage_get_stage_act()) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case DEEP_WAIT:
        if (shelter_water_state == SHELTER_WATER_RISEN) {
            Float y = shelter_upper_pool_level;

            work->mode = DEEP_RISEN;
            work->pos.y = y;
            VARS(work)->world->pos[1] = y;
        }
        break;
    case DEEP_RISEN:
        if (shelter_water_state == SHELTER_WATER_FULL) {
            Float y = shelter_water_level;
            Float half;

            work->pos.y = y;
            VARS(work)->world->pos[1] = y;
            half = (y + FLOOR_DEPTH) * 0.5F;
            y = y - half + 10.0F;
            half += 20.0F;
            VARS(work)->height = half;
            VARS(work)->middle = y;
            task->draw = shelter_deep_water_draw;
            work->mode = DEEP_FULL;
            work->tick = 0;
            work->submode = 0;
        } else {
            Float y;
            Float saved;
            Float half;

            /* player 0 must exist, as player_in_cylinder requires. without
             * it, this would leave player unset */
            player_get_history(0, 0, &player, NULL);
            y = player.y + ABOVE;
            if (y > shelter_upper_pool_level) {
                y = shelter_upper_pool_level;
            }
            VARS(work)->world->pos[1] = y;
            saved = work->pos.y;
            half = (y + FLOOR_DEPTH) * 0.5F;
            y = y - half + 10.0F;
            half += 20.0F;
            work->pos.y = y;
            if (player_in_cylinder(&work->pos, NEAR, half)) {
                work->flags |= WORLD_TESTED;
            } else {
                work->flags &= ~WORLD_TESTED;
            }
            work->pos.y = saved;
        }
        break;
    case DEEP_FULL: {
        Float saved = work->pos.y;

        work->pos.y = VARS(work)->middle;
        if (player_in_cylinder(&work->pos, NEAR, VARS(work)->height)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
        work->pos.y = saved;
        ++work->tick;
        if (work->tick > TEXTURE_LAST_TICK) {
            work->tick = 0;
            ++work->submode;
            if (work->submode >= TEXTURES) {
                work->submode = 0;
            }
        }
        /* NOLINTBEGIN(bugprone-signed-char-misuse) */
        /* the texture's index runs from 0 to TEXTURES - 1, so its sign never
         * shows */
        shelter_lower_water_4_object.model->mats->attr_texId = work->submode;
        /* NOLINTEND(bugprone-signed-char-misuse) */
        shelter_deep_water_draw(task);
    } break;
    }
}

static void shelter_deep_water_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&shelter_suimen_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&shelter_lower_water_4_object);
        njPopMatrix(1);
    }
}

static void shelter_deep_water_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}
