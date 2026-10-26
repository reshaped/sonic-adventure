/* the upper pool's surface in act 0, which players swim in, and a second
 * piece of its water. only the collision world holds the second piece. the
 * surface follows the pool down as it pours, then fades */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stg12/texlists.h"
#include "stg12/upper_pool.h"
#include "stg12/upper_water.h"
#include "system/frame.h"
#include "system/task.h"

extern NJS_OBJECT shelter_upper_water_object;

#define WATER_WORLD_FLAGS (WORLD_LARGE | WORLD_MOVES | WORLD_WATER)

/* the height of the pool's floor */
#define FLOOR_LEVEL 82.5F
/* the radius of each piece's test cylinder. player 0 inside it makes the
 * collision world test the piece */
#define NEAR 180.0F
/* the surface's test cylinder spans the floor to the surface, reaches
 * TEST_MARGIN past each, and is raised TEST_RAISE. it runs from 10 below
 * the floor to 30 above the surface */
#define TEST_MARGIN 20.0F
#define TEST_RAISE 10.0F

/* the surface shows its TEXTURES textures in turn. work->tick counts a
 * texture's steps up to TEXTURE_LAST_TICK, so each shows for three steps.
 * only the first texture's first showing lasts two */
#define TEXTURES 14
#define TEXTURE_LAST_TICK 2

/* alpha lost each step as the surface fades */
#define FADE_SPEED 0.02F

/* the second piece's place. its water stands at SECOND_TOP until the pool
 * falls below that. SECOND_Y is its test cylinder's first center, and
 * SECOND_FLOOR the floor under it */
#define SECOND_X 750.0F
#define SECOND_Y 108.45F
#define SECOND_Z (-910.0F)
#define SECOND_TOP 126.4F
#define SECOND_FLOOR 80.5F

enum shelter_upper_water_mode {
    WATER_STILL,
    WATER_FALL,
    WATER_FADE,
    WATER_GONE
};

/* the surface's work->tick counts its texture's steps, and TASK_TIMER(work)
 * holds the texture. work->scl.y holds its alpha for draw_set_material, from
 * 0 down to -1 */
struct shelter_upper_water_vars {
    NJS_OBJECT *world;
    /* how far the second piece's test cylinder reaches above and below
     * work->pos */
    Float height;
};

#define VARS(work) ((struct shelter_upper_water_vars *)&(work)->vars)

static void shelter_upper_water_step(struct task *task);
static void shelter_upper_water_draw(struct task *task);
static void shelter_upper_water_destroy(struct task *task);
static void shelter_upper_water_2_start(struct task *task);
static void shelter_upper_water_2_step(struct task *task);
static void shelter_upper_water_2_destroy(struct task *task);

/* from here to shelter_upper_water_2_object, the quad the second piece adds
 * to the collision world */
static NJS_MATERIAL shelter_upper_water_2_materials[1] = {
#include "stg12/upper_water/shelter_upper_water_2_materials.inc"
};

static Sint16 shelter_upper_water_2_meshes[4] = {
#include "stg12/upper_water/shelter_upper_water_2_meshes.inc"
};

static NJS_MESHSET shelter_upper_water_2_meshsets[1] = {
#include "stg12/upper_water/shelter_upper_water_2_meshsets.inc"
};

static NJS_POINT3 shelter_upper_water_2_points[4] = {
#include "stg12/upper_water/shelter_upper_water_2_points.inc"
};

static NJS_VECTOR shelter_upper_water_2_normals[4] = {
#include "stg12/upper_water/shelter_upper_water_2_normals.inc"
};

static NJS_MODEL shelter_upper_water_2_model = {
#include "stg12/upper_water/shelter_upper_water_2_model.inc"
};

/* @hack type: an array of one, not the single NJS_OBJECT it is. SHC pools
 * the object's address at 0x0c91057c and reads the model through it. as a
 * single object, SHC pools the model field's address instead, and 7 of
 * shelter_upper_water_2_start's 88 words and a pool word differ */
static NJS_OBJECT shelter_upper_water_2_object[1] = {
#include "stg12/upper_water/shelter_upper_water_2_object.inc"
};

void shelter_upper_water_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->mode = WATER_STILL;
    work->tick = 0;
    TASK_TIMER(work) = 0;
    work->scl.y = 0.0F;
    work->pos.y = shelter_upper_pool_level;
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
    world->model = shelter_upper_water_object.model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WATER_WORLD_FLAGS, task, world);
    work->flags |= WORLD_TESTED;
    task->step = shelter_upper_water_step;
    task->draw = shelter_upper_water_draw;
    task->destructor = shelter_upper_water_destroy;
    task_create_child(TASK_WORK | TASK_FORCE, shelter_upper_water_2_start,
                      task);
}

static void shelter_upper_water_step(struct task *task)
{
    struct task_work *work = task->work;
    Float level = shelter_upper_pool_level;
    Float y;
    Float half;
    Float middle;

    switch (work->mode) {
    case WATER_STILL:
        if (work->pos.y != level) {
            work->pos.y = level;
            VARS(work)->world->pos[1] = level;
            work->mode = WATER_FALL;
        }
        break;
    case WATER_FALL:
        if (work->pos.y == level) {
            work->mode = WATER_FADE;
            work->flags &= ~WORLD_TESTED;
        } else {
            work->pos.y = level;
            VARS(work)->world->pos[1] = level;
        }
        break;
    case WATER_FADE:
        work->scl.y -= FADE_SPEED;
        if (work->scl.y <= -1.0F) {
            work->scl.y = -1.0F;
            work->mode = WATER_GONE;
            work->flags &= ~WORLD_TESTED;
        }
        break;
    case WATER_GONE:
        task_destroy_children(task);
        break;
    }
    y = work->pos.y;
    half = (y - FLOOR_LEVEL) * 0.5F;
    middle = y - half + TEST_RAISE;
    half += TEST_MARGIN;
    work->pos.y = middle;
    /* the test sets or clears WORLD_TESTED every step, so the clears in
     * WATER_FALL and WATER_FADE change nothing */
    if (player_in_cylinder(&work->pos, NEAR, half)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    work->pos.y = y;
    work->tick++;
    if (work->tick > TEXTURE_LAST_TICK) {
        work->tick = 0;
        TASK_TIMER(work)++;
        TASK_TIMER(work) %= TEXTURES;
    }
    shelter_upper_water_object.model->mats[0].attr_texId = TASK_TIMER(work);
    task_step_children(task);
    shelter_upper_water_draw(task);
}

static void shelter_upper_water_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&shelter_suimen_texlist);
        njPushMatrix(NULL);
        draw_set_material(work->scl.y, 0.0F, 0.0F, 0.0F);
        njTranslateV(NULL, &work->pos);
        draw_tree(&shelter_upper_water_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
}

static void shelter_upper_water_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}

/* the second piece's first step. the surface creates the piece as its
 * child, and destroys it once the surface has faded */
static void shelter_upper_water_2_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->mode = WATER_STILL;
    work->tick = 0;
    work->submode = 0;
    work->pos.x = SECOND_X;
    work->pos.y = SECOND_Y;
    work->pos.z = SECOND_Z;
    VARS(work)->height = SECOND_Y - SECOND_FLOOR;
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
    world->pos[1] = SECOND_TOP;
    world->pos[2] = work->pos.z;
    world->model = shelter_upper_water_2_object->model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WATER_WORLD_FLAGS, task, world);
    work->flags &= ~WORLD_TESTED;
    task->step = shelter_upper_water_2_step;
    task->destructor = shelter_upper_water_2_destroy;
}

static void shelter_upper_water_2_step(struct task *task)
{
    struct task_work *work = task->work;
    Float level = shelter_upper_pool_level;

    if (level < SECOND_TOP) {
        work->pos.y = (level + 90.5F) * 0.5F;
        VARS(work)->world->pos[1] = level;
        VARS(work)->height = level - SECOND_FLOOR;
    }
    if (player_in_cylinder(&work->pos, NEAR, VARS(work)->height)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    /* @hack redundant: a store of level that never runs. SHC then keeps
     * level in FR15, which it saves at 0x0c9105a6 and loads at 0x0c9105ac.
     * without it, level takes FR4, the function ends 6 bytes early, and 49
     * of its 50 words differ */
    if (0) {
        work->pos.y = level;
    }
}

static void shelter_upper_water_2_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}
