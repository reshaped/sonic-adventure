/* O Kuguri: snowboard gates, and the combo they score. they stand only in a
 * mini game's trial */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "hud/hud.h"
#include "main.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/e102_bonus.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg08/gates.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL icecap_gate_model;

/* a gate passed while the combo runs adds one to its count, up to
 * COMBO_MAX. a gate passed once it has run out starts it again at 1. each
 * pass leaves the combo COMBO_STEPS steps to run, and scores COMBO_POINTS
 * times its count */
#define COMBO_MAX 50
#define COMBO_STEPS 180
#define COMBO_POINTS 10

/* the combo's screen x, the spacing of its glyphs, and their priority */
#define COMBO_X 528.0F
#define COMBO_SPACING 32.0F
#define COMBO_PRI (-3.0F)
/* the sprite's frame for the times sign, after the ten digits' */
#define COMBO_TIMES 10

#define SE_GATE 21

/* in work->flags: the gate has been passed */
#define GATE_PASSED 0x200

/* the combo's count, and the steps it has left to run */
static Uint8 icecap_gate_combo_count = 1;
static Uint8 icecap_gate_combo_steps = COMBO_STEPS;
/* a capsule across the gate, turned a quarter turn about z, whose first hit
 * counts the gate passed */
static struct collision_shape icecap_gate_shape = {
    SHAPE_CAPSULE,
    SHAPE_ATTACK,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 23.0F, 0.0F},
    20.0F,
    17.0F,
    0.0F,
    {0, 0, 0x4000},
    0,
    NULL,
};
/* the sprite's frames over e102time_texlist. frames 0 to 9 are the digits,
 * on its textures 0 to 9. frame 10 is the times sign, on its texture 12 */
static NJS_TEXANIM icecap_gate_combo_texanims[11] = {
#include "stg08/gates/icecap_gate_combo_texanims.inc"
};
static NJS_SPRITE icecap_gate_combo_sprite = {
#include "stg08/gates/icecap_gate_combo_sprite.inc"
};
/* the digits draw with a white constant material, and the draw then sets
 * it back to all 0 */
static const NJS_ARGB icecap_gate_combo_white = {1.0F, 1.0F, 1.0F, 1.0F};
static const NJS_ARGB icecap_gate_combo_none = {0.0F, 0.0F, 0.0F, 0.0F};

static void icecap_gate_destroy(struct task *task)
{
}

/* the gate's step and draw */
static void icecap_gate_run(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(&icecap_gate_model);
    njPopMatrix(1);
    if ((work->collision->flags & COLLISION_HIT) &&
        !(work->flags & GATE_PASSED)) {
        work->flags |= GATE_PASSED;
        if (icecap_gate_combo_steps == 0) {
            icecap_gate_combo_count = 1;
        } else if (++icecap_gate_combo_count > COMBO_MAX) {
            icecap_gate_combo_count = COMBO_MAX;
        }
        icecap_gate_combo_steps = COMBO_STEPS;
        score_add(icecap_gate_combo_count * COMBO_POINTS);
        sound_play(SE_GATE, 0, 0, 0);
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
}

void icecap_gate_step(struct task *task)
{
    struct task_work *work = task->work;

    if (game_mode != GAME_MODE_TRIAL ||
        menu_state.trial[0] != MENU_TRIAL_MINI_GAME) {
        placement_kill(task);
        return;
    }
    work->flags &= ~GATE_PASSED;
    collision_init(task, &icecap_gate_shape, COLLISION_OBJECT, 1, 1);
    task->destructor = icecap_gate_destroy;
    task->step = icecap_gate_run;
    task->draw = icecap_gate_run;
}

static void icecap_gate_combo_draw(struct task *task)
{
    if (icecap_gate_combo_steps == 0) {
        return;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    njSetConstantMaterial((NJS_ARGB *)&icecap_gate_combo_white);
#ifdef GCC
    /* the port's 16:9 squeezes the count about the screen's right edge */
    port_wide_anchor = WIDE_RIGHT;
#endif
    icecap_gate_combo_sprite.p.x = COMBO_X;
    njDrawSprite2D(&icecap_gate_combo_sprite, COMBO_TIMES, COMBO_PRI,
                   NJD_SPRITE_ALPHA);
    icecap_gate_combo_sprite.p.x += COMBO_SPACING;
    if (icecap_gate_combo_count >= 10) {
        njDrawSprite2D(&icecap_gate_combo_sprite, icecap_gate_combo_count / 10,
                       COMBO_PRI, NJD_SPRITE_ALPHA);
        icecap_gate_combo_sprite.p.x += COMBO_SPACING;
        njDrawSprite2D(&icecap_gate_combo_sprite, icecap_gate_combo_count % 10,
                       COMBO_PRI, NJD_SPRITE_ALPHA);
    } else {
        njDrawSprite2D(&icecap_gate_combo_sprite, icecap_gate_combo_count,
                       COMBO_PRI, NJD_SPRITE_ALPHA);
    }
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
    njSetConstantMaterial((NJS_ARGB *)&icecap_gate_combo_none);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
}

/* the combo's step ends its task outside act 2, the board run. in act 2, it
 * counts down the combo's steps */
static void icecap_gate_combo_run(struct task *task)
{
    if (stage_get_stage_act() != STAGE_ACT(STAGE_ICECAP, 2)) {
        task_kill(task);
        return;
    }
    if (icecap_gate_combo_steps != 0) {
        icecap_gate_combo_steps--;
    }
    icecap_gate_combo_draw(task);
}

void icecap_gate_combo_step(struct task *task)
{
    icecap_gate_combo_count = 1;
    icecap_gate_combo_steps = 0;
    task->step = icecap_gate_combo_run;
    task->draw = icecap_gate_combo_draw;
}
