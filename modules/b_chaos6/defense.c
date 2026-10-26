/* Chaos 6's guard flash: c_defens01 to c_defens12, played on a model beside
 * the boss. it fades in on the first texture and out on the last */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos6/defense.h"
#include "b_chaos6/defense_model.h"
#include "b_chaos6/textures.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/util.h"

/* c_defens01's index in chaos6_effect_texlist */
#define DEFENSE_TEXTURE 20
/* c_defens12's index, counted from c_defens01 */
#define DEFENSE_LAST 11
/* the flash holds c_defens01 as it fades in and for DEFENSE_WAIT - 1 steps
 * of DEFENSE_PLAY. it then shows the next texture each step */
#define DEFENSE_WAIT 2
/* how much its alpha changes a step as it fades, and how far it keeps from
 * the boss across */
#define DEFENSE_FADE 0.05F
#define DEFENSE_REACH 38.0F

struct defense_vars {
    Float alpha;
    Sint8 steps; /* the steps of DEFENSE_PLAY so far */
    Sint8 texture; /* the texture it shows, past c_defens01 */
};

#define VARS(work) ((struct defense_vars *)&(work)->vars)

enum defense_mode { DEFENSE_IN, DEFENSE_PLAY, DEFENSE_OUT };

static void defense_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    njSetTexture(&chaos6_effect_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, 0x8000 - work->ang.y, ang);
    njTranslate(NULL, -8.0F, 5.0F, 0.0F);
    njScale(NULL, 1.5F, 1.5F, 1.5F);
    /* @hack spelling: the model read through the object's address, which the
     * pool holds for draw_tree too: the load at 0x0c904066. read as
     * chaos6_defense_object.model, the functions after move, and 12 of 78
     * words differ */
    (&chaos6_defense_object)->model->mats->attr_texId =
        VARS(work)->texture + DEFENSE_TEXTURE;
    draw_set_material(VARS(work)->alpha, 0.0F, 0.0F, 0.0F);
    draw_tree(&chaos6_defense_object);
    njPopMatrix(1);
    draw_control_restore();
}

static void defense_run(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case DEFENSE_IN:
        if ((VARS(work)->alpha += DEFENSE_FADE) >= 1.0F) {
            work->mode = DEFENSE_PLAY;
        }
        break;
    case DEFENSE_PLAY:
        VARS(work)->steps++;
        if (VARS(work)->steps >= DEFENSE_WAIT) {
            VARS(work)->texture++;
            if (VARS(work)->texture >= DEFENSE_LAST) {
                work->mode = DEFENSE_OUT;
            }
        }
        break;
    case DEFENSE_OUT:
        if ((VARS(work)->alpha -= DEFENSE_FADE) <= 0.0F) {
            task_kill(task);
            return;
        }
        break;
    }
    work->pos.x = boss_work->pos.x + (njCos(work->ang.y) * DEFENSE_REACH);
    work->pos.z = boss_work->pos.z + (njSin(work->ang.y) * DEFENSE_REACH);
    defense_draw(task);
}

void chaos6_defense_step(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float z;

    x = work->pos.x - boss_work->pos.x;
    z = work->pos.z - boss_work->pos.z;
    work->ang.y = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    VARS(work)->alpha = 0.0F;
    /* @hack spelling: the model read through the object's address, which the
     * pool holds: the load at 0x0c9041f4. read as chaos6_defense_object.model,
     * 14 of 41 words differ */
    (&chaos6_defense_object)->model->mats[0].attrflags |= NJD_FLAG_USE_ALPHA;
    task->step = defense_run;
    task->draw = defense_draw;
}
