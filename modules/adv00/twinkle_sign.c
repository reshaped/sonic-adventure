/* O TWKANBAN Twinkle Park sign, whose lights cycle through three textures */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "adv00/twinkle_sign.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/task.h"

extern NJS_OBJECT twinkle_sign_object;

/* the lights' texture id, from LIGHTS_FIRST to LIGHTS_LAST and round again.
 * it moves to the next on every third step, when work->tick passes
 * LIGHTS_DELAY */
#define LIGHTS(work) ((work)->submode)
#define LIGHTS_FIRST 7
#define LIGHTS_LAST 9
#define LIGHTS_DELAY 2
/* the sign's material that takes the lights' texture */
#define LIGHTS_MATERIAL 2

static void twinkle_sign_step(struct task *task);
static void twinkle_sign_draw(struct task *task);

void twinkle_sign_init(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    if ((Uint8)stage_act != STATION_SQUARE_TWINKLE) {
        njPrintC(NJM_LOCATION(10, 8), "ACT TWINKLE ONLY !");
        return;
    }
    work->tick = 0;
    LIGHTS(work) = LIGHTS_FIRST;
    work->mode = 0;
    task->step = twinkle_sign_step;
    task->draw = twinkle_sign_draw;
}

static void twinkle_sign_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    work->tick++;
    if (work->tick > LIGHTS_DELAY) {
        work->tick = 0;
        LIGHTS(work)++;
        if (LIGHTS(work) > LIGHTS_LAST) {
            LIGHTS(work) = LIGHTS_FIRST;
        }
    }
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    twinkle_sign_object.model->mats[LIGHTS_MATERIAL].attr_texId = LIGHTS(work);
    twinkle_sign_draw(task);
}

static void twinkle_sign_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ss_twins_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&twinkle_sign_object);
    njPopMatrix(1);
}
