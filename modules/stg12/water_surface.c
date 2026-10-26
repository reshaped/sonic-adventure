/* O SUIMEN, a sheet of water players swim in. the set entry's scale x picks
 * one of four sheets */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg12/texlists.h"
#include "stg12/water_surface.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT shelter_water_surface_0_object;
extern NJS_OBJECT shelter_water_surface_1_object;
/* sheet 2, which is also lower_water.c's fourth piece */
extern NJS_OBJECT shelter_lower_water_4_object;
extern NJS_OBJECT shelter_water_surface_3_object;

#define SHEET_WORLD (WORLD_LARGE | WORLD_WATER)

/* destroyed beyond 1000 units of the camera and player 0 */
#define RANGE (1000.0F * 1000.0F)

/* the sheets the set entry's scale x picks from */
#define SHEETS 4
/* a sheet shows its TEXTURES textures in turn. work->tick counts a
 * texture's steps up to TEXTURE_LAST_TICK, so each shows for three steps.
 * only the first texture's first showing lasts two */
#define TEXTURES 14
#define TEXTURE_LAST_TICK 2

/* the drawn sheet moves BOB_SPEED a step. it turns back once it is more
 * than BOB above or below its place */
#define BOB 1.5F
#define BOB_SPEED 0.005F

enum shelter_water_surface_mode { SURFACE_RISE, SURFACE_FALL };

/* work->submode holds the sheet, the set entry's scale x modulo SHEETS.
 * work->tick counts its texture's steps, and TASK_TIMER(work) holds the
 * texture */
struct shelter_water_surface_vars {
    NJS_OBJECT *world;
    /* player 0 or 1 this near makes the collision world test the sheet */
    Float near;
    Float bob; /* the drawn sheet's height above its place */
};

#define VARS(work) ((struct shelter_water_surface_vars *)&(work)->vars)

static void shelter_water_surface_run(struct task *task);
static void shelter_water_surface_draw(struct task *task);
static void shelter_water_surface_destroy(struct task *task);

static NJS_OBJECT *shelter_water_surface_objects[SHEETS] = {
    &shelter_water_surface_0_object,
    &shelter_water_surface_1_object,
    &shelter_lower_water_4_object,
    &shelter_water_surface_3_object,
};
static Float shelter_water_surface_nears[SHEETS] = {250.0F, 350.0F, 220.0F,
                                                    160.0F};

void shelter_water_surface_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *world;

    work->submode = (Uint8)work->scl.x % SHEETS;
    work->mode = SURFACE_RISE;
    work->tick = 0;
    work->timer = 0;
    VARS(work)->bob = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        VARS(work)->world = world = world_object_alloc();
        world->evalflags =
            NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK | NJD_EVAL_ZXY_ANG;
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = 0;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = shelter_water_surface_objects[work->submode]->model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(SHEET_WORLD, task, world);
        work->flags |= WORLD_TESTED;
        VARS(work)->near = shelter_water_surface_nears[work->submode];
    }
    task->step = shelter_water_surface_run;
    task->draw = shelter_water_surface_draw;
    task->destructor = shelter_water_surface_destroy;
}

/* the bob moves only the drawn sheet; its object in the collision world stays
 * where it was placed */
static void shelter_water_surface_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        work->submode = (Uint8)work->scl.x % SHEETS;
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> KIND OF SUIMEN");
    } else {
        if (player_in_range(&work->pos, VARS(work)->near)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    if (work->mode == SURFACE_RISE) {
        VARS(work)->bob += BOB_SPEED;
        if (VARS(work)->bob > BOB) {
            work->mode = SURFACE_FALL;
        }
    } else {
        VARS(work)->bob -= BOB_SPEED;
        if (VARS(work)->bob < -BOB) {
            work->mode = SURFACE_RISE;
        }
    }
    work->tick++;
    if (work->tick > TEXTURE_LAST_TICK) {
        work->tick = 0;
        TASK_TIMER(work)++;
        TASK_TIMER(work) %= TEXTURES;
    }
    shelter_water_surface_objects[work->submode]->model->mats->attr_texId =
        TASK_TIMER(work);
    shelter_water_surface_draw(task);
}

static void shelter_water_surface_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&shelter_suimen_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njTranslate(NULL, 0.0F, VARS(work)->bob, 0.0F);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_tree(shelter_water_surface_objects[work->submode]);
        njPopMatrix(1);
    }
}

static void shelter_water_surface_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
}
