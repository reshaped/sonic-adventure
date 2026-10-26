/* the surface of the water act 0's dam holds back */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "stg12/dam.h"
#include "stg12/dam_water.h"
#include "stg12/texlists.h"
#include "system/task.h"

extern NJS_OBJECT shelter_dam_water_object;

/* the textures the surface cycles through. each shows while work->tick
 * counts from 0 to TEXTURE_LAST_TICK, for three steps */
#define TEXTURES 14
#define TEXTURE_LAST_TICK 2

/* the alpha the surface loses each step it fades */
#define FADE_SPEED 0.02F

/* the surface stays still until the dam's water starts to fall. it falls
 * with that water until the water stops, then fades and is gone */
enum shelter_dam_water_mode { WATER_STILL, WATER_FALL, WATER_FADE, WATER_GONE };

/* work->tick counts the steps the texture has shown, and TASK_TIMER(work)
 * holds the texture's index */
struct shelter_dam_water_vars {
    Uint8 unk_00[8];
    Float alpha; /* constant material alpha, from 0 down to -1 as it fades */
};

#define VARS(work) ((struct shelter_dam_water_vars *)&(work)->vars)

static void shelter_dam_water_step(struct task *task);
static void shelter_dam_water_draw(struct task *task);

void shelter_dam_water_start(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *point;
    Sint32 count;
    int i;

    work->mode = WATER_STILL;
    work->tick = 0;
    TASK_TIMER(work) = 0;
    VARS(work)->alpha = 0.0F;
    work->pos.y = shelter_dam_level;
    count = shelter_dam_water_object.model->nbPoint;
    point = shelter_dam_water_object.model->points;
    for (i = 0; i < count; i++, point++) {
        point->y = 0.0F;
    }
    task->step = shelter_dam_water_step;
    task->draw = shelter_dam_water_draw;
}

static void shelter_dam_water_step(struct task *task)
{
    struct task_work *work = task->work;
    Float level = shelter_dam_level;

    switch (work->mode) {
    case WATER_STILL:
        if (work->pos.y != level) {
            work->pos.y = level;
            work->mode = WATER_FALL;
        }
        break;
    case WATER_FALL:
        if (work->pos.y == level) {
            work->mode = WATER_FADE;
        } else {
            work->pos.y = level;
        }
        break;
    case WATER_FADE:
        VARS(work)->alpha -= FADE_SPEED;
        if (VARS(work)->alpha <= -1.0F) {
            VARS(work)->alpha = -1.0F;
            work->mode = WATER_GONE;
        }
        break;
    case WATER_GONE:
        return;
    }
    work->tick++;
    if (work->tick > TEXTURE_LAST_TICK) {
        work->tick = 0;
        TASK_TIMER(work)++;
        TASK_TIMER(work) %= TEXTURES;
    }
    shelter_dam_water_object.model->mats[0].attr_texId = TASK_TIMER(work);
    shelter_dam_water_draw(task);
}

static void shelter_dam_water_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&shelter_suimen_texlist);
    njPushMatrix(NULL);
    draw_set_material(VARS(work)->alpha, 0.0F, 0.0F, 0.0F);
    njTranslateV(NULL, &work->pos);
    draw_tree(&shelter_dam_water_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}
