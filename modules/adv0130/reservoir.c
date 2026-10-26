/* the water of act 4's reservoir, which the object list names O RESERVOIR */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/egg_carrier_inside.h"
#include "adv0130/reservoir.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/common.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

extern NJS_OBJECT reservoir_object;
extern NJS_OBJECT reservoir_solid_object;

#define RESERVOIR_ACT 4

/* the surface cycles through WATER_TEXTURES of ec_water_texlist's textures,
 * each shown for WATER_TEXTURE_STEPS steps */
#define WATER_TEXTURES 14
#define WATER_TEXTURE_STEPS 2

/* a shown surface's collision is tested while player 0 is within TEST_HEIGHT
 * of the solid model's height */
#define TEST_HEIGHT 20.0F

/* in work->flags, beside WORLD_TESTED: the surface isn't drawn. nothing sets
 * it */
#define RESERVOIR_HIDDEN 0x200

/* the surface's texture in ec_water_texlist */
#define WATER_TEXTURE(work) ((work)->tick)

enum reservoir_mode {
    RESERVOIR_INIT,
    RESERVOIR_SHOWN,
    /* the step returns at once. the step that sets it still runs the height
     * test and draws the surface, once. from then on WORLD_TESTED keeps that
     * test's result, and nothing draws the surface */
    RESERVOIR_IDLE
};

static void reservoir_destroy(struct task *task);
static void reservoir_draw(struct task *task);

/* outside act 4, the step returns at once. the first step in it puts the
 * solid model in the world, solid and water. it then shows the surface if
 * FLAG_E102_EC_TRANSFORM is set, E-102's flag whoever plays, and otherwise
 * idles from then on */
void egg_carrier_reservoir_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;

    if ((Uint8)stage_get_stage_act() != RESERVOIR_ACT) {
        return;
    }
    switch (work->mode) {
    case RESERVOIR_INIT:
        WORLD_MODEL(work)->model = &reservoir_solid_object;
        solid = world_object_alloc();
        WORLD_MODEL(work)->world = solid;
        solid->evalflags = NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = 0;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x = WORLD_MODEL(work)->model->pos[0];
        solid->pos[1] = WORLD_MODEL(work)->model->pos[1];
        solid->pos[2] = work->pos.z = WORLD_MODEL(work)->model->pos[2];
        solid->model = WORLD_MODEL(work)->model->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(WORLD_SOLID | WORLD_WATER, task, solid);
        task->destructor = reservoir_destroy;
        if (!event_flag_check(FLAG_E102_EC_TRANSFORM)) {
            work->mode = RESERVOIR_IDLE;
            task->draw = NULL;
        } else {
            work->mode = RESERVOIR_SHOWN;
            task->draw = reservoir_draw;
        }
        break;
    case RESERVOIR_SHOWN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) == WATER_TEXTURE_STEPS) {
            TASK_TIMER(work) = 0;
            WATER_TEXTURE(work)++;
            if (WATER_TEXTURE(work) == WATER_TEXTURES) {
                WATER_TEXTURE(work) = 0;
            }
        }
        break;
    case RESERVOIR_IDLE:
        return;
    }
    work->flags &= ~WORLD_TESTED;
    if (_builtin_fabsf(player_work[0]->pos.y -
                       WORLD_MODEL(work)->model->pos[1]) < TEST_HEIGHT) {
        work->flags |= WORLD_TESTED;
    }
    reservoir_draw(task);
}

static void reservoir_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}

/* draw the surface with its texture, unless RESERVOIR_HIDDEN is set. it
 * translates the current matrix by pos without pushing it, and leaves it
 * there */
static void reservoir_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &reservoir_object;

    object->model->mats->attr_texId = WATER_TEXTURE(work);
    if (!(work->flags & RESERVOIR_HIDDEN)) {
        njSetTexture(&ec_water_texlist);
        draw_set_material(-0.3F, 0.0F, 0.0F, 0.0F);
        njTranslateV(NULL, &work->pos);
        draw_tree(object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}
