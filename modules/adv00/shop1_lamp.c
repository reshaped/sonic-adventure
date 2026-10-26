/* O S1DNTO lamp of the first shop, lit in the evening and at night */
#include <ninja.h>

#include "adv00/shop1_lamp.h"
#include "adv00/texlists.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/sprite.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

extern NJS_OBJECT shop1_lamp_object;

/* the lamp's material, beside its shade's. by day it shows dentou_d and
 * ignores the lights. otherwise it shows dentou_n, and the lights reach it */
#define LAMP_MATERIAL 1
#define LAMP_DARK 1
#define LAMP_DARK_ATTR (LAMP_LIT_ATTR | NJD_FLAG_IGNORE_LIGHT)
#define LAMP_LIT 2
#define LAMP_LIT_ATTR                                                          \
    (NJD_SA_SRC | NJD_DA_INV_SRC | NJD_FLAG_USE_FLAT | NJD_FLAG_USE_TEXTURE |  \
     NJD_FLAG_CLAMP_U | NJD_FLAG_CLAMP_V | NJD_D_100)

/* the glow hangs GLOW_DROP below the lamp's position, under its shade */
#define GLOW_DROP 2.2F

enum shop1_lamp_mode { LAMP_INIT, LAMP_OFF, LAMP_ON };

static void shop1_lamp_draw(struct task *task);
static void shop1_lamp_glow_draw(struct task *task);
static void shop1_lamp_destroy(struct task *task);

/* the glow sprite, EFF_REGULAR's panlight */
static NJS_TEXANIM shop1_lamp_glow_texanim = {
#include "adv00/shop1_lamp/shop1_lamp_glow_texanim.inc"
};
static NJS_SPRITE shop1_lamp_glow = {
#include "adv00/shop1_lamp/shop1_lamp_glow.inc"
};

static void shop1_lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(MODEL(work)->texlist);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

/* draw the glow, blended additively, then the lamp, on a frame's first step
 * only */
static void shop1_lamp_glow_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!frame_step) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        draw_set_material(0.7F, 0.8F, 0.65F, 0.3F);
        shop1_lamp_glow.p.x = work->pos.x;
        shop1_lamp_glow.p.y = work->pos.y - GLOW_DROP;
        shop1_lamp_glow.p.z = work->pos.z;
        sprite_set_depth(-3.0F);
        sprite_draw_3d(&shop1_lamp_glow, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        shop1_lamp_draw(task);
    }
}

static void shop1_lamp_destroy(struct task *task)
{
    task_kill(task);
}

/* while the set editor is open, draw the lamp as its model stands. with the
 * editor closed, the lamp's first step sets its material by the time of day.
 * by day it is dark, and in the evening and at night it is lit and draws its
 * glow */
void shop1_lamp_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        NJS_TEXLIST *texlist = &ss_dentou_texlist;
        NJS_OBJECT *object = &shop1_lamp_object;

        if (edit_mode) {
            if (!MODEL(work)->object) {
                MODEL(work)->object = object;
            }
            if (!MODEL(work)->texlist) {
                MODEL(work)->texlist = texlist;
            }
            shop1_lamp_draw(task);
        } else {
            switch (work->mode) {
            case LAMP_INIT:
                MODEL(work)->object = object;
                MODEL(work)->texlist = texlist;
                if (story_get_time() == TIME_DAY) {
                    MODEL(work)->object->model->mats[LAMP_MATERIAL].attr_texId =
                        LAMP_DARK;
                    MODEL(work)->object->model->mats[LAMP_MATERIAL].attrflags =
                        LAMP_DARK_ATTR;
                    work->mode = LAMP_OFF;
                    task->draw = shop1_lamp_draw;
                } else {
                    MODEL(work)->object->model->mats[LAMP_MATERIAL].attr_texId =
                        LAMP_LIT;
                    MODEL(work)->object->model->mats[LAMP_MATERIAL].attrflags =
                        LAMP_LIT_ATTR;
                    work->mode = LAMP_ON;
                    work->timer = 0;
                    work->scl.x = 0.7F;
                    work->scl.y = 0.0F;
                    task->draw = shop1_lamp_glow_draw;
                }
                task->destructor = shop1_lamp_destroy;
                break;
            case LAMP_OFF:
                shop1_lamp_draw(task);
                break;
            case LAMP_ON:
                shop1_lamp_glow_draw(task);
                break;
            }
        }
    }
}
