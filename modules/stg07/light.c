/* the light: O RLIGHT, a model that only draws, and a gravity panel's glow */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "stg07/light.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT ruin_light_object;
extern NJS_MODEL ruin_light_glow_model;

struct glow_vars {
    Float spread;
    Float alpha;
};

#define VARS(work) ((struct glow_vars *)&(work)->vars)

enum glow_mode { GLOW_START, GLOW_SPREAD, GLOW_END };

/* each step, the glow's spread grows by its scl.y plus SPREAD_STEP, and its
 * alpha drops by FADE_STEP */
#define SPREAD_STEP 0.02F
#define FADE_STEP 0.03F
/* the glow's scale across its face is its spread plus GLOW_WIDTH, and its
 * scale through its face GLOW_DEPTH */
#define GLOW_WIDTH 0.55F
#define GLOW_DEPTH 0.05F

void ruin_light_step(struct task *task)
{
    struct task_work *work = task->work;

    MODEL(work)->object = &ruin_light_object;
    model_start_scaled(task);
}

/* the glow fades out from alpha 1, then runs about as long again, until
 * alpha passes -1 */
void ruin_light_glow_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    switch (work->mode) {
    case GLOW_START:
        VARS(work)->alpha = 1.0F;
        work->mode = GLOW_SPREAD;
        break;
    case GLOW_SPREAD:
        VARS(work)->spread += work->scl.y + SPREAD_STEP;
        VARS(work)->alpha -= FADE_STEP;
        texture_set_stage();
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        /* the turns face the model's flat y axis the way ang.y faces */
        TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(90), ang);
        njRotateZ(NULL, NJM_DEG_ANG(270));
        njScale(NULL, VARS(work)->spread + GLOW_WIDTH, GLOW_DEPTH,
                VARS(work)->spread + GLOW_WIDTH);
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        draw_set_material(VARS(work)->alpha, 0.0F, 0.0F, 0.0F);
        draw_model(&ruin_light_glow_model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
        njFogEnable();
        draw_attr_restore();
        if (VARS(work)->alpha < -1.0F) {
            work->mode = GLOW_END;
        }
        break;
    case GLOW_END:
        task_kill(task);
        break;
    }
}
