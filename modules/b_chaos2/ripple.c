/* the ripple where Chaos 2's ball lands. it is ripple_object, which its
 * action spreads while its texture cycles, and it darkens away */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/ripple.h"
#include "b_chaos2/textures.h"
#include "draw/draw.h"
#include "system/task.h"
#include "system/util.h"

/* the ripple plays its action's RIPPLE_FRAMES frames over RIPPLE_STEPS
 * steps, and ends once its frame reaches RIPPLE_FRAMES */
#define RIPPLE_FRAMES 7.0F
#define RIPPLE_STEPS 60.0F
#define RIPPLE_PACE (RIPPLE_FRAMES / RIPPLE_STEPS)
/* how far above its maker's pos the ripple lies */
#define RIPPLE_HEIGHT 1.0F
/* the ripple shows TEXTURES textures in turn, moving on every
 * TEXTURE_STEPS steps, and then starts again. the first is
 * chaos2_effect_texlist's FIRST_TEXTURE, c2_bb01 */
#define TEXTURE_STEPS 2
#define TEXTURES 8
#define FIRST_TEXTURE 41
/* past DARKEN_AT of its frames, the ripple darkens, as its offset color
 * falls from 0 to -1 over the DARKEN_SPAN left. DARKEN_SPAN is written
 * 0.3F: as 1.0F - DARKEN_AT, its reciprocal folds to 0x40555554, where the
 * image has 0x40555556 (0x0c906acc) */
#define DARKEN_AT 0.7F
#define DARKEN_SPAN 0.3F

/* the ripple's work->mode */
enum ripple_mode { RIPPLE_START, RIPPLE_RUN };

struct ripple_vars {
    Float frame; /* the action's frame */
    /* @hack type: the steps on the texture and the texture shown, as an array
     * that SHC indexes from its base (ADD R2,R3 at 0x0c906a56). a union clears
     * both in one store (MOV.L R6,@(12,R5) at 0x0c906a32). as two Sint16 fields
     * cleared one by one, summed over ripple_draw and chaos2_ripple_step, 54 of
     * 137 words differ */
    union {
        Sint16 count[2];
        Sint32 all;
    } texture;
};

#define VARS(work) ((struct ripple_vars *)&(work)->vars)

static NJS_MATERIAL ripple_materials[1] = {
#include "b_chaos2/ripple/ripple_materials.inc"
};
static Sint16 ripple_meshes0[37] = {
#include "b_chaos2/ripple/ripple_meshes0.inc"
};
static NJS_COLOR ripple_uvs0[30] = {
#include "b_chaos2/ripple/ripple_uvs0.inc"
};
static NJS_MESHSET ripple_meshsets[1] = {
#include "b_chaos2/ripple/ripple_meshsets.inc"
};
static NJS_POINT3 ripple_points[17] = {
#include "b_chaos2/ripple/ripple_points.inc"
};
static NJS_VECTOR ripple_normals[17] = {
#include "b_chaos2/ripple/ripple_normals.inc"
};
static NJS_MODEL ripple_model = {
#include "b_chaos2/ripple/ripple_model.inc"
};
static NJS_OBJECT ripple_object = {
#include "b_chaos2/ripple/ripple_object.inc"
};
static NJS_MKEY_F ripple_scl[8] = {
#include "b_chaos2/ripple/ripple_scl.inc"
};
static NJS_MDATA3 ripple_mdata[1] = {
#include "b_chaos2/ripple/ripple_mdata.inc"
};
static NJS_MOTION ripple_motion = {
#include "b_chaos2/ripple/ripple_motion.inc"
};
static NJS_ACTION ripple_action = {
#include "b_chaos2/ripple/ripple_action.inc"
};

/* draw ripple_object at the work's pos, posed by its action at the step's
 * frame, with the texture the step shows. it draws through the offset
 * material, so a color below 0 darkens it */
static void ripple_draw(struct task *task)
{
    static NJS_ARGB ripple_color = {1.0F, 0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;
    Float done;

    ripple_materials[0].attr_texId =
        VARS(work)->texture.count[1] + FIRST_TEXTURE;
    njSetTexture(&chaos2_effect_texlist);
    njPushMatrix(NULL);
    done = VARS(work)->frame * (1.0F / RIPPLE_FRAMES);
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_OFFSET_MATERIAL);
    if (done < DARKEN_AT) {
        ripple_color.r = ripple_color.g = ripple_color.b = 0.0F;
    } else {
        Float dark = (DARKEN_AT - done) * (1.0F / DARKEN_SPAN);

        ripple_color.r = ripple_color.g = ripple_color.b = dark;
    }
    njSetConstantMaterial(&ripple_color);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, 1.0F, 1.0F, 1.0F);
    draw_action(&ripple_action, VARS(work)->frame);
    draw_control_restore();
    njPopMatrix(1);
}

void chaos2_ripple_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RIPPLE_START:
        VARS(work)->texture.all = 0;
        VARS(work)->frame = 0.0F;
        work->pos.y += RIPPLE_HEIGHT;
        task->draw = ripple_draw;
        work->mode = RIPPLE_RUN;
        /* fall through */
    case RIPPLE_RUN:
        VARS(work)->texture.count[0]++;
        if (VARS(work)->texture.count[0] >= TEXTURE_STEPS) {
            VARS(work)->texture.count[0] = 0;
            VARS(work)->texture.count[1]++;
            if (VARS(work)->texture.count[1] >= TEXTURES) {
                VARS(work)->texture.count[1] = 0;
            }
        }
        VARS(work)->frame += RIPPLE_PACE;
        if (VARS(work)->frame >= RIPPLE_FRAMES) {
            task_destroy(task);
            return;
        }
        break;
    }
    ripple_draw(task);
}
