/* act 0's water surface below the dam, with a floating island and three more
 * pieces of water. the surface rises with act 0's water, and raises the
 * island with it. the third and fourth pieces start once the water is
 * SHELTER_WATER_FULL */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "draw/tree.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg12/floating_island.h"
#include "stg12/lower_water.h"
#include "stg12/texlists.h"
#include "stg12/water.h"
#include "system/frame.h"
#include "system/task.h"

extern NJS_OBJECT shelter_lower_water_object;
extern NJS_OBJECT shelter_lower_water_4_object;

#define WATER_WORLD_FLAGS (WORLD_LARGE | WORLD_MOVES | WORLD_WATER)

/* the surface's place. its height is act 0's water level */
#define SURFACE_X 750.0F
#define SURFACE_Z (-650.0F)
/* the height of the floor under the surface. act 0's water stands at this
 * level until it rises. until then, the surface's step only steps its
 * children */
#define FLOOR_LEVEL (-80.0F)

/* the radius of the surface's test cylinder. player 0 inside the cylinder
 * makes the collision world test the surface */
#define NEAR 250.0F
/* the surface's test cylinder spans the floor to the surface, reaches
 * TEST_MARGIN past each, and is raised TEST_RAISE. it runs from 10 below the
 * floor to 30 above the surface */
#define TEST_MARGIN 20.0F
#define TEST_RAISE 10.0F

/* the surface shows its TEXTURES textures in turn. work->tick counts a
 * texture's steps up to TEXTURE_LAST_TICK, so each shows for three steps.
 * only the first texture's first showing lasts two */
#define TEXTURES 14
#define TEXTURE_LAST_TICK 2

#define ISLAND_X 865.0F
#define ISLAND_Y (-75.0F)
#define ISLAND_Z (-650.0F)

/* the second and third pieces. SECOND_Y and THIRD_Y are their test cylinders'
 * centers. SECOND_WATER and THIRD_WATER are the heights of their water in the
 * collision world. the pieces store SECOND_HEIGHT and THIRD_HEIGHT, and no
 * code reads them */
#define SECOND_X 750.0F
#define SECOND_Y (-21.3F)
#define SECOND_Z (-910.5F)
#define SECOND_WATER (-2.6F)
#define SECOND_HEIGHT 28.7F
#define THIRD_X 750.0F
#define THIRD_Y 28.45F
#define THIRD_Z (-390.5F)
#define THIRD_WATER 46.9F
#define THIRD_HEIGHT 28.45F
/* the radius of the second and third pieces' test cylinders. each reaches
 * PIECE_HEIGHT above and below its center */
#define PIECE_NEAR 150.0F
#define PIECE_HEIGHT 50.0F

/* the fourth piece. its water stands at FOURTH_Y, and it shows the surface's
 * texture. its test cylinder has its center at FOURTH_MIDDLE and radius
 * FOURTH_NEAR. the cylinder reaches FOURTH_HEIGHT above and below its
 * center */
#define FOURTH_X 750.0F
#define FOURTH_Y 52.0F
#define FOURTH_Z (-272.5F)
#define FOURTH_MIDDLE 31.0F
#define FOURTH_HEIGHT 31.0F
#define FOURTH_NEAR 110.0F

/* the mode the second and third pieces set. no code reads it */
#define PIECE_MODE 1

/* the surface's TASK_TIMER(work) holds the stage and act as it started, as
 * stage_get_stage_act packs them. its step kills it once they change.
 * work->submode is the texture it shows, and work->tick counts that
 * texture's steps */
struct shelter_lower_water_vars {
    NJS_OBJECT *world;
    struct task *island;
    Bool full; /* the third and fourth pieces have started */
};

/* only the fourth piece's step reads height and middle. its test cylinder
 * has its center at middle, and reaches height above and below it */
struct shelter_lower_water_piece_vars {
    NJS_OBJECT *world;
    Float height;
    Float middle;
};

#define VARS(work) ((struct shelter_lower_water_vars *)&(work)->vars)
#define PIECE(work) ((struct shelter_lower_water_piece_vars *)&(work)->vars)

static void shelter_lower_water_step(struct task *task);
static void shelter_lower_water_draw(struct task *task);
static void shelter_lower_water_destroy(struct task *task);
static void shelter_lower_water_3_start(struct task *task);
static void shelter_lower_water_2_start(struct task *task);
static void shelter_lower_water_2_step(struct task *task);
static void shelter_lower_water_2_destroy(struct task *task);
static void shelter_lower_water_4_start(struct task *task);
static void shelter_lower_water_4_step(struct task *task);
static void shelter_lower_water_4_draw(struct task *task);
static void shelter_lower_water_4_destroy(struct task *task);

/* from here to shelter_lower_water_piece_object, the quad the second and
 * third pieces add to the collision world */
static NJS_MATERIAL shelter_lower_water_piece_materials[1] = {
#include "stg12/lower_water/shelter_lower_water_piece_materials.inc"
};

static Sint16 shelter_lower_water_piece_meshes[4] = {
#include "stg12/lower_water/shelter_lower_water_piece_meshes.inc"
};

static NJS_MESHSET shelter_lower_water_piece_meshsets[1] = {
#include "stg12/lower_water/shelter_lower_water_piece_meshsets.inc"
};

static NJS_POINT3 shelter_lower_water_piece_points[4] = {
#include "stg12/lower_water/shelter_lower_water_piece_points.inc"
};

static NJS_VECTOR shelter_lower_water_piece_normals[4] = {
#include "stg12/lower_water/shelter_lower_water_piece_normals.inc"
};

static NJS_MODEL shelter_lower_water_piece_model = {
#include "stg12/lower_water/shelter_lower_water_piece_model.inc"
};

/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC
 * pools its address at 0x0c90f6b8 and 0x0c90f7dc and reads model through
 * it; declared as one, 14 of the 176 words of shelter_lower_water_3_start
 * and shelter_lower_water_2_start and two pool words differ */
static NJS_OBJECT shelter_lower_water_piece_object[1] = {
#include "stg12/lower_water/shelter_lower_water_piece_object.inc"
};

void shelter_lower_water_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;
    struct task *child;
    struct task_work *island;

    work->mode = 0;
    work->tick = 0;
    work->submode = 0;
    TASK_TIMER(work) = stage_get_stage_act();
    VARS(work)->full = FALSE;
    work->pos.x = SURFACE_X;
    work->pos.y = shelter_water_level;
    work->pos.z = SURFACE_Z;
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
    world->model = shelter_lower_water_object.model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WATER_WORLD_FLAGS, task, world);
    work->flags &= ~WORLD_TESTED;
    child = task_create_child(TASK_MOTION | TASK_WORK | TASK_FORCE,
                              floating_island_carried_step, task);
    island = child->work;
    island->pos.x = ISLAND_X;
    island->pos.y = ISLAND_Y;
    island->pos.z = ISLAND_Z;
    VARS(work)->island = child;
    task_create_child(TASK_WORK | TASK_FORCE, shelter_lower_water_2_start,
                      task);
    task->step = shelter_lower_water_step;
    task->draw = shelter_lower_water_draw;
    task->destructor = shelter_lower_water_destroy;
}

static void shelter_lower_water_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *island;
    Float y;
    Float half;
    Float middle;
    Float level;

    if (TASK_TIMER(work) != stage_get_stage_act()) {
        task_kill(task);
        return;
    }
    if (shelter_water_level == FLOOR_LEVEL) {
        task_step_children(task);
        return;
    }
    y = work->pos.y;
    half = (work->pos.y - FLOOR_LEVEL) * 0.5F;
    middle = y - half + TEST_RAISE;
    half += TEST_MARGIN;
    work->pos.y = middle;
    if (player_in_cylinder(&work->pos, NEAR, half)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    work->pos.y = y;
    island = VARS(work)->island->work;
    level = shelter_water_level;
    if (FLOATING_ISLAND(island)->rest < level) {
        FLOATING_ISLAND(island)->rest = level;
        island->pos.y = level;
    }
    if (!VARS(work)->full && shelter_water_state == SHELTER_WATER_FULL) {
        task_create_child(TASK_WORK | TASK_FORCE, shelter_lower_water_3_start,
                          task);
        task_create_child(TASK_WORK | TASK_FORCE, shelter_lower_water_4_start,
                          task);
        VARS(work)->full = TRUE;
    }
    task_step_children(task);
    work->tick++;
    if (work->tick > TEXTURE_LAST_TICK) {
        work->tick = 0;
        work->submode++;
        work->submode %= TEXTURES;
    }
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    shelter_lower_water_object.model->mats->attr_texId = work->submode;
    level = shelter_water_level;
    work->pos.y = level;
    VARS(work)->world->pos[1] = level;
    shelter_lower_water_draw(task);
}

static void shelter_lower_water_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&shelter_suimen_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_tree(&shelter_lower_water_object);
        njPopMatrix(1);
    }
}

static void shelter_lower_water_destroy(struct task *task)
{
    struct task_work *work = task->work;

    task_destroy_children(task);
    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}

/* the third piece's first step. the surface creates the piece once the water
 * is SHELTER_WATER_FULL. the piece then runs the second piece's step and
 * destructor */
static void shelter_lower_water_3_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->mode = PIECE_MODE;
    work->tick = 0;
    work->submode = 0;
    work->pos.x = THIRD_X;
    work->pos.z = THIRD_Z;
    work->pos.y = THIRD_Y;
    PIECE(work)->height = THIRD_HEIGHT;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    PIECE(work)->world = world = world_object_alloc();
    world->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = 0;
    world->ang[1] = 0;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x;
    world->pos[1] = THIRD_WATER;
    world->pos[2] = work->pos.z;
    world->model = shelter_lower_water_piece_object->model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WATER_WORLD_FLAGS, task, world);
    work->flags &= ~WORLD_TESTED;
    task->step = shelter_lower_water_2_step;
    task->destructor = shelter_lower_water_2_destroy;
}

/* the second piece's first step. shelter_lower_water_start creates the piece.
 * only the collision world holds the second and third pieces: no code draws
 * them */
static void shelter_lower_water_2_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->mode = PIECE_MODE;
    work->tick = 0;
    work->submode = 0;
    work->pos.x = SECOND_X;
    work->pos.z = SECOND_Z;
    work->pos.y = SECOND_Y;
    PIECE(work)->height = SECOND_HEIGHT;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    PIECE(work)->world = world = world_object_alloc();
    world->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = 0;
    world->ang[1] = 0;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x;
    world->pos[1] = SECOND_WATER;
    world->pos[2] = work->pos.z;
    world->model = shelter_lower_water_piece_object->model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WATER_WORLD_FLAGS, task, world);
    work->flags &= ~WORLD_TESTED;
    task->step = shelter_lower_water_2_step;
    task->destructor = shelter_lower_water_2_destroy;
}

static void shelter_lower_water_2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_cylinder(&work->pos, PIECE_NEAR, PIECE_HEIGHT)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void shelter_lower_water_2_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (PIECE(work)->world != NULL) {
        world_remove(task, PIECE(work)->world);
        world_object_free(PIECE(work)->world);
    }
}

static void shelter_lower_water_4_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->tick = 0;
    work->submode = 0;
    work->pos.x = FOURTH_X;
    work->pos.y = FOURTH_Y;
    work->pos.z = FOURTH_Z;
    PIECE(work)->middle = FOURTH_MIDDLE;
    PIECE(work)->height = FOURTH_HEIGHT;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    PIECE(work)->world = world = world_object_alloc();
    world->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    world->ang[0] = 0;
    world->ang[1] = 0;
    world->ang[2] = 0;
    world->pos[0] = work->pos.x;
    world->pos[1] = FOURTH_Y;
    world->pos[2] = work->pos.z;
    world->model = shelter_lower_water_4_object.model;
    world->child = NULL;
    world->sibling = NULL;
    world_add(WATER_WORLD_FLAGS, task, world);
    work->flags &= ~WORLD_TESTED;
    task->step = shelter_lower_water_4_step;
    task->draw = shelter_lower_water_4_draw;
    task->destructor = shelter_lower_water_4_destroy;
}

static void shelter_lower_water_4_step(struct task *task)
{
    struct task_work *work = task->work;
    Float y = work->pos.y;

    work->pos.y = PIECE(work)->middle;
    if (player_in_cylinder(&work->pos, FOURTH_NEAR, PIECE(work)->height)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    work->pos.y = y;
    /* NOLINTBEGIN(bugprone-signed-char-misuse) */
    shelter_lower_water_4_object.model->mats->attr_texId =
        task->parent->work->submode;
    /* NOLINTEND(bugprone-signed-char-misuse) */
    shelter_lower_water_4_draw(task);
}

static void shelter_lower_water_4_draw(struct task *task)
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

static void shelter_lower_water_4_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (PIECE(work)->world != NULL) {
        world_remove(task, PIECE(work)->world);
        world_object_free(PIECE(work)->world);
    }
}
