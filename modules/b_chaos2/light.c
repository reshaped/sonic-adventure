/* a light where Chaos 2 bursts into his drops. its action turns
 * light_object and scales it from wide and flat to narrow and tall, while
 * its texture cycles, and the light darkens away */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/light.h"
#include "b_chaos2/textures.h"
#include "draw/draw.h"
#include "system/task.h"
#include "system/util.h"

/* the light plays its action's LIGHT_FRAMES frames, LIGHT_PACE a step,
 * and ends once its frame reaches LIGHT_FRAMES */
#define LIGHT_FRAMES 15.0F
#define LIGHT_PACE (1.0F / 6.0F)
/* the light shows TEXTURES textures in turn, moving on every TEXTURE_STEPS
 * steps, and then starts again. the first is chaos2_effect_texlist's
 * FIRST_TEXTURE, c2_light01 */
#define TEXTURE_STEPS 2
#define TEXTURES 8
#define FIRST_TEXTURE 25
/* past DARKEN_AT of its frames, the light darkens, as its offset color
 * falls from 0 to -1 over the DARKEN_SPAN left. DARKEN_SPAN is written
 * 0.3F: as 1.0F - DARKEN_AT, its reciprocal folds to 0x40555554, where the
 * image has 0x40555556 (0x0c9067a0) */
#define DARKEN_AT 0.7F
#define DARKEN_SPAN 0.3F

/* the light's work->mode */
enum light_mode { LIGHT_START, LIGHT_RUN };

struct light_vars {
    Float frame; /* the action's frame */
    /* @hack type: the steps on the texture and the texture shown, as an array
     * that SHC indexes from its base (ADD R2,R3 at 0x0c906734). a union clears
     * both in one store (MOV.L R6,@(12,R5) at 0x0c906718). as two Sint16 fields
     * cleared one by one, summed over light_draw and chaos2_light_step, 49 of
     * 136 words differ */
    union {
        Sint16 count[2];
        Sint32 all;
    } texture;
};

#define VARS(work) ((struct light_vars *)&(work)->vars)

static NJS_MATERIAL light_materials[1] = {
#include "b_chaos2/light/light_materials.inc"
};
static Sint16 light_meshes0[64] = {
#include "b_chaos2/light/light_meshes0.inc"
};
static NJS_COLOR light_uvs0[64] = {
#include "b_chaos2/light/light_uvs0.inc"
};
static NJS_MESHSET light_meshsets[1] = {
#include "b_chaos2/light/light_meshsets.inc"
};
static NJS_POINT3 light_points[32] = {
#include "b_chaos2/light/light_points.inc"
};
static NJS_VECTOR light_normals[32] = {
#include "b_chaos2/light/light_normals.inc"
};
static NJS_MODEL light_model = {
#include "b_chaos2/light/light_model.inc"
};
static NJS_OBJECT light_object = {
#include "b_chaos2/light/light_object.inc"
};
static NJS_MKEY_A light_ang[16] = {
#include "b_chaos2/light/light_ang.inc"
};
static NJS_MKEY_F light_scl[16] = {
#include "b_chaos2/light/light_scl.inc"
};
static NJS_MDATA3 light_mdata[1] = {
#include "b_chaos2/light/light_mdata.inc"
};
static NJS_MOTION light_motion = {
#include "b_chaos2/light/light_motion.inc"
};
static NJS_ACTION light_action = {
#include "b_chaos2/light/light_action.inc"
};

/* draw light_object at the work's pos, posed by its action at the step's
 * frame, with the texture the step shows. it draws through the offset
 * material, so a color below 0 darkens it */
static void light_draw(struct task *task)
{
    static NJS_ARGB light_color = {1.0F, 0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;
    Float done = VARS(work)->frame * (1.0F / LIGHT_FRAMES);

    light_materials[0].attr_texId =
        VARS(work)->texture.count[1] + FIRST_TEXTURE;
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_OFFSET_MATERIAL);
    if (done < DARKEN_AT) {
        light_color.r = light_color.g = light_color.b = 0.0F;
    } else {
        Float dark = (DARKEN_AT - done) * (1.0F / DARKEN_SPAN);

        light_color.r = light_color.g = light_color.b = dark;
    }
    njSetConstantMaterial(&light_color);
    njSetTexture(&chaos2_effect_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_action(&light_action, VARS(work)->frame);
    njPopMatrix(1);
    light_color.r = light_color.g = light_color.b = 0.0F;
    njSetConstantMaterial(&light_color);
    draw_control_restore();
}

void chaos2_light_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case LIGHT_START:
        VARS(work)->texture.all = 0;
        VARS(work)->frame = 0.0F;
        task->draw = light_draw;
        work->mode = LIGHT_RUN;
        break;
    case LIGHT_RUN:
        VARS(work)->texture.count[0]++;
        if (VARS(work)->texture.count[0] >= TEXTURE_STEPS) {
            VARS(work)->texture.count[0] = 0;
            VARS(work)->texture.count[1]++;
            if (VARS(work)->texture.count[1] >= TEXTURES) {
                VARS(work)->texture.count[1] = 0;
            }
        }
        VARS(work)->frame += LIGHT_PACE;
        if (VARS(work)->frame >= LIGHT_FRAMES) {
            task_destroy(task);
            return;
        }
        break;
    }
    light_draw(task);
}
