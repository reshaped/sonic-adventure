/* the trail a thrown drop of Chaos 2's leaves. the drop makes a sprite
 * each step it flies, and each sprite shrinks away */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/textures.h" /* NOLINT(misc-include-cleaner) */
#include "b_chaos2/trail.h"
#include "system/task.h"
#include "system/util.h"

/* the sprite's scale starts at 1 and falls by TRAIL_SHRINK a step. the
 * sprite ends once its scale falls to 0 or below */
#define TRAIL_SHRINK (1.0F / 15.0F)

/* the sprite's work->mode */
enum trail_mode { TRAIL_START, TRAIL_RUN };

struct trail_vars {
    Float scale;
};

#define VARS(work) ((struct trail_vars *)&(work)->vars)

/* the sprite's one frame, c2_kiseki of chaos2_effect_texlist */
static NJS_TEXANIM trail_frame = {
#include "b_chaos2/trail/trail_frame.inc"
};
static NJS_SPRITE trail_sprite = {
#include "b_chaos2/trail/trail_sprite.inc"
};

/* draw the sprite at the work's pos and scale, added to what lies behind
 * it. the offset color the draw sets doesn't reach the sprite:
 * njDrawSprite3D colors a sprite white unless NJD_SPRITE_COLOR is set (TST
 * at 0x8c6237d4 in 1ST_READ.BIN) */
static void trail_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    draw_control_save();
    njControl3D(NJD_CONTROL_3D_OFFSET_MATERIAL);
    color.r = color.g = color.b = VARS(work)->scale - 1.0F;
    /* @bug uninitialized: color.a has no store before njSetConstantMaterial
     * copies all four of color's members. color lies at R15, which R4 passes
     * at 0x0c906b2c, and njSetConstantMaterial reads color.a through R4 at
     * 0x8c629894 in 1ST_READ.BIN */
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    trail_sprite.sx = trail_sprite.sy = VARS(work)->scale;
    njTranslateV(NULL, &work->pos);
    njDrawSprite3D(&trail_sprite, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
    njPopMatrix(1);
    color.a = 1.0F;
    njSetConstantMaterial(&color);
    draw_control_restore();
}

void chaos2_trail_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case TRAIL_START:
        VARS(work)->scale = 1.0F;
        task->draw = trail_draw;
        work->mode = TRAIL_RUN;
        /* fall through */
    case TRAIL_RUN:
        VARS(work)->scale -= TRAIL_SHRINK;
        if (VARS(work)->scale <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    trail_draw(task);
}
