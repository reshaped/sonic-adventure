/* O KARTKANBAN Twinkle Circuit sign, whose lights cycle through three
 * textures */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/kart_sign.h"
#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "system/task.h"

extern NJS_OBJECT kart_sign_object;

#define LIGHTS_MATERIAL 2 /* the model's material that shows the lights */
#define TEXTURE(work) ((work)->submode) /* the texture the lights show */
/* the lights show ss_twins_texlist's textures from LIGHTS_FIRST to
 * LIGHTS_LAST, in turn */
#define LIGHTS_FIRST 4
#define LIGHTS_LAST 6
/* the texture changes once tick passes this, every third step */
#define LIGHTS_DELAY 2

static void kart_sign_step(struct task *task);
static void kart_sign_draw(struct task *task);
static void kart_sign_destroy(struct task *task);

void kart_sign_init(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    if ((Uint8)stage_act != STATION_SQUARE_TWINKLE) {
        njPrintC(NJM_LOCATION(10, 8), "ACT TWINKLE ONLY !");
        return;
    }
    work->ang.x = 0;
    work->ang.y = 0;
    work->ang.z = 0;
    work->mode = 0;
    work->tick = 0;
    TEXTURE(work) = LIGHTS_FIRST;
    task->step = kart_sign_step;
    task->draw = kart_sign_draw;
    task->destructor = kart_sign_destroy;
}

static void kart_sign_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    work->tick++;
    if (work->tick > LIGHTS_DELAY) {
        work->tick = 0;
        TEXTURE(work)++;
        if (TEXTURE(work) > LIGHTS_LAST) {
            TEXTURE(work) = LIGHTS_FIRST;
        }
    }
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    kart_sign_object.model->mats[LIGHTS_MATERIAL].attr_texId = TEXTURE(work);
    kart_sign_draw(task);
}

static void kart_sign_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ss_twins_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_tree(&kart_sign_object);
    njPopMatrix(1);
}

static void kart_sign_destroy(struct task *task)
{
}
