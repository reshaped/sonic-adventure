/* TURIBASI 1: a rope bridge that falls apart when an enemy strikes it. it
 * stands again about 200 steps later */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "object/bridge.h"
#include "sound/sound.h"
#include "stg08/bridge.h"
#include "system/task.h"

extern NJS_TEXLIST hasi_texlist;
extern NJS_MODEL icecap_bridge_1_model;
extern NJS_MODEL icecap_bridge_2_model;
extern NJS_MODEL icecap_bridge_post_model;
extern NJS_MODEL icecap_bridge_rope_model;
extern NJS_MODEL icecap_bridge_plank_model;

#define SE_BRIDGE_BREAK 210

/* the bridge's collision reach, far past its shape */
#define BRIDGE_REACH 250000.0F

/* a fallen bridge stands again once it has been down more than
 * BRIDGE_DOWN_LIMIT steps */
#define BRIDGE_DOWN_LIMIT 200

/* in work->submode */
enum icecap_bridge_mode { BRIDGE_WHOLE, BRIDGE_BREAK, BRIDGE_DOWN };

/* the steps the bridge has been down, in work->ang.z, which bridge_init
 * zeroes */
#define STEPS_DOWN(work) ((work)->ang.z)

/* a sphere of radius 100, 100 along the bridge. an enemy that touches it
 * breaks the bridge */
/* clang-format off */
static struct collision_shape icecap_bridge_shape = {
    SHAPE_HURTABLE | SHAPE_SPHERE, SHAPE_HURTS_PLAYERS, PUSH_FIRM(PUSH_ALL),
    {100.0F, 0.0F, 0.0F}, 100.0F, 0.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

/* bridge_init's defaults, nine floats of struct bridge_params' ten. in the
 * image, icecap_bridge_1_parts follows with no room for a tenth */
static Float icecap_bridge_1_params[9] = {
    170.0F, 2.5F, -0.12F, 16.0F, 11.5F, 0.04F, 0.0065F, 17.5F, -1.3F,
};

static struct bridge_parts icecap_bridge_1_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{236, 237, 238, 239}, {240, 241, 242, 243}},
    {0, 1, 2, 3},
    {{0.0F, 347.7488F}, {-30.0F, 72.0F}, {-17.7F, 17.7F}},
    347.7488F,
    1.0F,
    &hasi_texlist,
    &icecap_bridge_1_model,
    &icecap_bridge_post_model,
    &icecap_bridge_rope_model,
    &icecap_bridge_plank_model,
    (struct bridge_params *)icecap_bridge_1_params,
};

/* the same, but for the width */
static Float icecap_bridge_2_params[9] = {
    170.0F, 2.5F, -0.12F, 16.0F, 11.5F, 0.04F, 0.0065F, 25.188F, -1.3F,
};

static struct bridge_parts icecap_bridge_2_parts = {
    {{0, 1, 2, 3}, {4, 5, 6, 7}},
    {{465, 465, 466, 467}, {468, 469, 470, 471}},
    {0, 1, 2, 3},
    {{0.0F, 1095.3979F}, {-10.0F, 280.0F}, {-28.7F, 28.7F}},
    1095.3979F,
    1.4F,
    &hasi_texlist,
    &icecap_bridge_2_model,
    &icecap_bridge_post_model,
    &icecap_bridge_rope_model,
    &icecap_bridge_plank_model,
    (struct bridge_params *)icecap_bridge_2_params,
};

/* whole until an enemy strikes it, then down until it stands again. in the
 * set editor, the bridge draws its model alone */
static void icecap_bridge_run(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode) {
        bridge_edit_draw(work);
    } else {
        switch (work->submode) {
        case BRIDGE_WHOLE:
            bridge_step_whole(task);
            /* @hack spelling: the test of the hit's kind nested inside the test
             * of COLLISION_HIT, which loads work->collision again for it
             * (0x0c905bf6, 0x0c905c00); as one test with &&, 24 of 80 words
             * differ */
            if (work->collision->flags & COLLISION_HIT) {
                if (work->collision->hit->kind == COLLISION_ENEMY) {
                    work->submode++;
                    sound_play(SE_BRIDGE_BREAK, 0, 0, 0);
                }
            }
            collision_register(work);
            break;
        case BRIDGE_BREAK:
            bridge_collapse(work);
            bridge_step_broken(task);
            work->submode++;
            break;
        case BRIDGE_DOWN:
            bridge_step_broken(task);
            if (STEPS_DOWN(work)++ > BRIDGE_DOWN_LIMIT) {
                work->submode = BRIDGE_WHOLE;
                STEPS_DOWN(work) = 0;
            }
            break;
        }
    }
}

void icecap_bridge_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &icecap_bridge_1_parts;
    collision_init(task, &icecap_bridge_shape, COLLISION_OBJECT, 1, 1);
    task->work->collision->radius = BRIDGE_REACH;
    bridge_init(task);
    task->step = icecap_bridge_run;
}

void icecap_bridge_2_step(struct task *task)
{
    BRIDGE_VARS(task->work)->parts = &icecap_bridge_2_parts;
    bridge_init(task);
}
