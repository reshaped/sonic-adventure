/* the column of water that falls from a burst water tank, and splashes */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "stg12/texlists.h"
#include "stg12/water_column.h"
#include "system/frame.h"
#include "system/task.h"

/* @hack type: an array, not the single NJS_ACTION it is. SHC pools only the
 * action's address, and reads the motion through it at 0x0c90d068. as a
 * single object, SHC pools the motion field's address too, and 75 of the 200
 * words of water_column_step and water_column_draw differ */
extern NJS_ACTION water_column_action[];

/* the column starts START_HEIGHT above its position. it falls START_SPEED
 * the first step, and DROP_ACCEL faster each step after */
#define START_HEIGHT 40.0F
#define START_SPEED 1.0F
#define DROP_ACCEL 0.8F

/* once the action's frame passes SPREAD_FRAME, the column widens by
 * SPREAD_SPEED each step. once it passes FADE_FRAME, its alpha falls by
 * FADE_SPEED each step */
#define SPREAD_FRAME 10.0F
#define SPREAD_SPEED 0.04F
#define FADE_FRAME 20.0F
#define FADE_SPEED 0.1F

/* the action's sheets, which all show the same texture */
#define SHEETS 4
/* the sheets show TEXTURES textures in turn. work->tick counts a texture's
 * steps up to TEXTURE_LAST_TICK, so each shows for three steps. only the
 * first texture's first showing lasts two */
#define TEXTURES 5
#define TEXTURE_LAST_TICK 2

enum water_column_mode { COLUMN_DROP, COLUMN_SPLASH };

/* work->scl.x scales the column's width. work->scl.z holds the change in
 * its height each step as it falls. work->tick counts its texture's steps,
 * and TASK_TIMER(work) holds the texture */
struct water_column_vars {
    Float frame; /* of water_column_action, which the draw shows */
    Float alpha; /* for draw_set_material, from 0 down */
    Float height; /* above work->pos */
};

#define VARS(work) ((struct water_column_vars *)&(work)->vars)

static void water_column_step(struct task *task);
static void water_column_draw(struct task *task);

void water_column_start(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = 0;
    TASK_TIMER(work) = 0;
    work->mode = COLUMN_DROP;
    work->scl.x = 1.0F;
    VARS(work)->frame = 0.0F;
    VARS(work)->alpha = 0.0F;
    VARS(work)->height = START_HEIGHT;
    work->scl.z = -START_SPEED;
    task->step = water_column_step;
    task->draw = water_column_draw;
}

static void water_column_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *sheet;
    Float last;
    int i;

    switch (work->mode) {
    case COLUMN_DROP:
        VARS(work)->height += work->scl.z;
        work->scl.z -= DROP_ACCEL;
        if (VARS(work)->height <= 0.0F) {
            VARS(work)->height = 0.0F;
            work->mode = COLUMN_SPLASH;
        }
        break;
    case COLUMN_SPLASH:
        VARS(work)->frame += 1.0F;
        last = water_column_action->motion->nbFrame - 1;
        if (VARS(work)->frame >= last) {
            VARS(work)->frame -= last;
            task_kill(task);
        }
        if (VARS(work)->frame > SPREAD_FRAME) {
            work->scl.x += SPREAD_SPEED;
        }
        if (VARS(work)->frame > FADE_FRAME) {
            VARS(work)->alpha -= FADE_SPEED;
        }
        break;
    }
    work->tick++;
    if (work->tick > TEXTURE_LAST_TICK) {
        work->tick = 0;
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) >= TEXTURES) {
            TASK_TIMER(work) = 0;
        }
    }
    sheet = water_column_action->object;
    sheet = sheet->child;
    for (i = 0; i < SHEETS; i++) {
        sheet->model->mats[0].attr_texId = TASK_TIMER(work);
        sheet = sheet->sibling;
    }
    water_column_draw(task);
}

static void water_column_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&shelter_column_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njTranslate(NULL, 0.0F, VARS(work)->height, 0.0F);
        njScale(NULL, work->scl.x, 1.0F, work->scl.x);
        draw_set_material(VARS(work)->alpha, 0.0F, 0.0F, 0.0F);
        draw_action(water_column_action, VARS(work)->frame);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
}
