/* Chaos 6's inhale: two cones of c6_suikomi01 and c6_suikomi02 at his
 * mouth, boss_points[0], turned the way he faces. each cone's texture
 * scrolls at its own pace */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos6/boss.h"
#include "b_chaos6/inhale.h"
#include "b_chaos6/inhale_model.h"
#include "b_chaos6/textures.h"
#include "chaos/chaos.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/util.h"

/* the outer cone. the swirl's task scrolls its uvs in place */
extern NJS_OBJECT chaos6_inhale_object;
extern NJS_COLOR chaos6_inhale_uvs[];

/* the swirl's alpha once it has faded in, and how much its alpha changes a
 * step as it fades in or out */
#define INHALE_ALPHA 0.75F
#define INHALE_FADE 0.025F
/* how far the outer cone's and the inner cone's v coordinates scroll a
 * step */
#define INHALE_SCROLL 25
#define INHALE_INNER_SCROLL 15
/* the uvs of each cone's model */
#define INHALE_UVS 64

struct inhale_vars {
    Float alpha;
    /* how far the outer cone's and the inner cone's v coordinates have
     * scrolled */
    union {
        Sint32 both;
        Sint16 each[2];
    } scroll;
};

#define VARS(work) ((struct inhale_vars *)&(work)->vars)

enum inhale_mode { INHALE_START, INHALE_IN, INHALE_HOLD, INHALE_OUT };

/* TRUE once chaos6_inhale_reset has kept the cones' v coordinates */
static Sint8 inhale_kept = FALSE;

/* the outer cone's and the inner cone's v coordinates, as
 * chaos6_inhale_reset first finds them */
static Sint16 inhale_v[INHALE_UVS];
static Sint16 inhale_inner_v[INHALE_UVS];
/* the pos of his mouth, boss_points[0], which each step sets for the draw */
static NJS_POINT3 *inhale_pos;
/* chaos6_inhale_reset clears it, and nothing reads it */
static Sint32 dat_0ca91ba4;

/* draw the cones at his mouth, turned the way he faces and tipped 5
 * degrees, in the swirl's alpha */
static void inhale_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    draw_control_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    njSetTexture(&chaos6_texlist);
    njPushMatrix(NULL);
    draw_set_material(VARS(work)->alpha, 0.0F, 0.0F, 0.0F);
    njTranslateV(NULL, inhale_pos);
    TURN_VIA(njRotateY, 0x4000 - boss_work->ang.y, ang);
    njRotateX(NULL, NJM_DEG_ANG(5));
    njScale(NULL, 5.0F / 3.0F, 5.0F / 3.0F, 5.0F / 3.0F);
    njTranslate(NULL, 0.0F, -5.0F, 0.0F);
    draw_tree(&chaos6_inhale_object);
    njTranslate(NULL, 0.0F, -10.0F, 0.0F);
    draw_tree(&chaos6_inhale_inner_object);
    njPopMatrix(1);
    draw_control_restore();
}

void chaos6_inhale_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    inhale_pos = &boss_points[0].pos;
    switch (work->mode) {
    case INHALE_START:
        task->draw = inhale_draw;
        /* @hack type: both scrolls cleared through the union's Sint32, in one
         * MOV.L (0x0c90367a); without it, as a store to each, 114 of 138 words
         * differ */
        VARS(work)->scroll.both = 0;
        VARS(work)->alpha = 0.0F;
        work->mode = INHALE_IN;
        /* fall through */
    case INHALE_IN:
        VARS(work)->alpha += INHALE_FADE;
        if (VARS(work)->alpha >= INHALE_ALPHA) {
            VARS(work)->alpha = INHALE_ALPHA;
            work->mode = INHALE_HOLD;
        }
        break;
    case INHALE_HOLD:
        if (chaos_record->motion.current != BODY_INHALING) {
            work->mode = INHALE_OUT;
        }
        break;
    case INHALE_OUT:
        VARS(work)->alpha -= INHALE_FADE;
        if (VARS(work)->alpha <= 0.0F) {
            task_kill(task);
            return;
        }
        break;
    }
    for (i = 0; i < INHALE_UVS; i++) {
        chaos6_inhale_uvs[i].tex.v = inhale_v[i] + VARS(work)->scroll.each[0];
        chaos6_inhale_inner_uvs[i].tex.v =
            inhale_inner_v[i] + VARS(work)->scroll.each[1];
    }
    VARS(work)->scroll.each[0] += INHALE_SCROLL;
    VARS(work)->scroll.each[1] += INHALE_INNER_SCROLL;
    inhale_draw(task);
}

void chaos6_inhale_reset(void)
{
    int i;

    dat_0ca91ba4 = 0;
    if (!inhale_kept) {
        for (i = 0; i < INHALE_UVS; i++) {
            inhale_v[i] = chaos6_inhale_uvs[i].tex.v;
            inhale_inner_v[i] = chaos6_inhale_inner_uvs[i].tex.v;
        }
        inhale_kept = TRUE;
    }
}
