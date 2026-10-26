/* a light at Chaos 2's feet as he spreads his puddle over the floor, or as
 * he bursts. it is blight_object, which changes shape through its motions
 * while its texture cycles, and darkens away */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/blight.h"
#include "b_chaos2/blight_model.h"
#include "b_chaos2/textures.h"
#include "draw/draw.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MOTION blight_motion;
extern NJS_MOTION blight_shape;

/* the light plays its motions' BLIGHT_FRAMES frames, BLIGHT_PACE a step,
 * and ends once its frame reaches BLIGHT_FRAMES */
#define BLIGHT_FRAMES 15.0F
#define BLIGHT_PACE (1.0F / 3.0F)
/* the light shows TEXTURES textures in turn, moving on every TEXTURE_STEPS
 * steps, and then starts again. the first is chaos2_effect_texlist's
 * FIRST_TEXTURE, c2_blight01 */
#define TEXTURE_STEPS 2
#define TEXTURES 8
#define FIRST_TEXTURE 33
/* past DARKEN_AT of its frames, the light darkens, as its offset color
 * falls from 0 to -1 over the DARKEN_SPAN left. DARKEN_SPAN is written
 * 0.3F: as 1.0F - DARKEN_AT, its reciprocal folds to 0x40555554, where the
 * image has 0x40555556 (0x0c906920) */
#define DARKEN_AT 0.7F
#define DARKEN_SPAN 0.3F

/* the light's work->mode */
enum blight_mode { BLIGHT_START, BLIGHT_RUN };

struct blight_vars {
    Float frame; /* the motions' frame */
    /* @hack type: the steps on the texture and the texture shown, as an array
     * that SHC indexes from its base (ADD R2,R3 at 0x0c9068c4). a union clears
     * both in one store (MOV.L R6,@(12,R5) at 0x0c9068aa). as two Sint16 fields
     * cleared one by one, summed over blight_draw and chaos2_blight_step, 50 of
     * 144 words differ */
    union {
        Sint16 count[2];
        Sint32 all;
    } texture;
};

#define VARS(work) ((struct blight_vars *)&(work)->vars)

/* draw blight_object at the work's pos and scale, posed by its motions at
 * frame, with the texture the step shows. it draws through the offset
 * material, so a color below 0 darkens it */
static void blight_draw(struct task *task)
{
    static NJS_ARGB blight_color = {1.0F, 0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;
    Float done = VARS(work)->frame * (1.0F / BLIGHT_FRAMES);

    blight_materials[0].attr_texId =
        VARS(work)->texture.count[1] + FIRST_TEXTURE;
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_OFFSET_MATERIAL);
    if (done < DARKEN_AT) {
        blight_color.r = blight_color.g = blight_color.b = 0.0F;
    } else {
        Float dark = (DARKEN_AT - done) * (1.0F / DARKEN_SPAN);

        blight_color.r = blight_color.g = blight_color.b = dark;
    }
    njSetConstantMaterial(&blight_color);
    njSetTexture(&chaos2_effect_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScaleV(NULL, &work->scl);
    draw_shape_motion(&blight_object, &blight_motion, &blight_shape,
                      VARS(work)->frame);
    njPopMatrix(1);
    blight_color.r = blight_color.g = blight_color.b = 0.0F;
    njSetConstantMaterial(&blight_color);
    draw_control_restore();
}

void chaos2_blight_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BLIGHT_START:
        VARS(work)->texture.all = 0;
        VARS(work)->frame = 0.0F;
        task->draw = blight_draw;
        work->mode = BLIGHT_RUN;
        /* fall through */
    case BLIGHT_RUN:
        VARS(work)->texture.count[0]++;
        if (VARS(work)->texture.count[0] >= TEXTURE_STEPS) {
            VARS(work)->texture.count[0] = 0;
            VARS(work)->texture.count[1]++;
            if (VARS(work)->texture.count[1] >= TEXTURES) {
                VARS(work)->texture.count[1] = 0;
            }
        }
        VARS(work)->frame += BLIGHT_PACE;
        if (VARS(work)->frame >= BLIGHT_FRAMES) {
            task_destroy(task);
            return;
        }
        break;
    /* @hack redundant: case 2, which no code sets, keeps the image's CMP/EQ #2
     * at 0x0c90689a; without it, 52 of 58 words differ */
    case 2:
        break;
    }
    blight_draw(task);
}
