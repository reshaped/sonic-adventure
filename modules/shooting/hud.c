/* the sky chase's hud: the score, the share of targets hit and the carrier
 * fight's time bonus. a combo multiplies the points of each shot's hit, and
 * lapses COMBO_STEPS after the last */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "hud/hud.h"
#include "shooting/gauge.h"
#include "shooting/hud.h"
#include "shooting/shooting.h"
#include "shooting/shot.h"
#include "stage/stage.h"
#include "system/task.h"

#define TEXT_1_LENGTH 13
#include "shooting/hud/text.inc"

enum hud_mode { HUD_INIT, HUD_RUN, HUD_GAME_OVER };

#define SCORE_MAX 99999
#define COMBO_MAX 10
#define COMBO_STEPS 60 /* a second, at 60 steps a second */

/* each act's count of targets, of which shoot_hit_rate gives the share hit */
#define TARGETS_ACT1 91.0F
#define TARGETS_ACT2 176.0F

/* the lives the tornado's gauge follows, which gauge.c defines after its
 * statics. a declaration in gauge.h would move it before them */
extern Float gauge_health;

/* a shot's lock read as a Uint8, as shooting_score_hit reads it */
#define SHOT_LOCK(work) (*(Uint8 *)&((struct shot_vars *)&(work)->vars)->lock)

static void shooting_hud_init(void);
static void shooting_hud_draw(struct task *task);
static void shooting_hud_destroy(struct task *task);

static struct hud_number score_digits = {NUMBER_SCALED | NUMBER_CLAMPED |
                                             NUMBER_ZEROS,
                                         NUMBER_TOP | NUMBER_LEFT,
                                         0,
                                         SCORE_MAX,
                                         0,
                                         {104.0F, 48.0F, 0.0F},
                                         1.0F,
                                         0xffffffff};
static NJS_TEXNAME score_textures[1] = {{TEXT_1, 0, 0}};
NJS_TEXLIST score_texlist = {
#include "shooting/hud/score_texlist.inc"
};
static NJS_TEXANIM score_label_frame[1] = {
#include "shooting/hud/score_label_frame.inc"
};
static NJS_SPRITE score_label = {
#include "shooting/hud/score_label.inc"
};

static int combo_steps;
static int combo_multiple;
static Float targets_hit;
static Float targets;

void shooting_hud_step(struct task *task)
{
    switch (task->work->mode) {
    case HUD_INIT:
        task->destructor = shooting_hud_destroy;
        task->draw = shooting_hud_draw;
        shooting_hud_init();
        task->work->mode = HUD_RUN;
        break;
    case HUD_RUN:
        if (--combo_steps <= 0) {
            combo_steps = 0;
            combo_multiple = 1;
        }
        score_digits.value = shooting_score;
        gauge_health = shooting_lives;
        shooting_hud_draw(task);
        shoot_hit_rate = targets_hit / targets * 100.0F;
        break;
    case HUD_GAME_OVER:
        /* @hack spelling: the two calls in one statement, which keeps
         * njPrintC a JSR ahead of the return the cases share (0x0c901436);
         * as two, it becomes a tail jump with an exit of its own, and 26 of
         * 64 words differ */
        shooting_hud_draw(task), njPrintC(NJM_LOCATION(16, 15), "GAME OVER");
        break;
    }
}

/* 5 lives on a new gauge, no score, no combo, and none of the act's
 * targets hit */
static void shooting_hud_init(void)
{
    shooting_score = 0;
    shooting_lives = 5.0F;
    gauge_create(288, 420, 5);
    combo_steps = 0;
    combo_multiple = 1;
    if (IN_STAGE(STAGE_SHOOTING1)) {
        targets = TARGETS_ACT1;
    } else {
        targets = TARGETS_ACT2;
    }
    targets_hit = 0.0F;
    score = 0;
    shoot_hit_rate = 0.0F;
}

/* draw the score's label and the score, and hide the rings' count */
static void shooting_hud_draw(struct task *task)
{
    rings_shown = -1;
#ifdef GCC
    /* the port's 16:9 squeezes the score about the screen's left edge */
    port_wide_anchor = WIDE_LEFT;
#endif
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njDrawSprite2D(&score_label, 0, -100.0F,
                   NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    hud_draw_number(&score_digits);
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}

static void shooting_hud_destroy(struct task *task)
{
}

void shooting_score_hit(int points, struct task_work *work)
{
    struct task *by = work->collision->hit->owner;

    if (by->step == shooting_shot_step) {
        /* @bug logic: SHOT_LOCK reads the lock as unsigned (EXTU.B at
         * 0x0c901598), so the test for a bullet's lock below 0 never holds
         * (CMP/PZ at 0x0c90159a), and a bullet's hit never scores double */
        if (SHOT_LOCK(by->work) < 0) {
            points *= 2;
        }
        shooting_score += combo_multiple * points;
        if (++combo_multiple > COMBO_MAX) {
            combo_multiple = COMBO_MAX;
        }
        combo_steps = COMBO_STEPS;
    }
    if (shooting_score >= SCORE_MAX) {
        shooting_score = SCORE_MAX;
    }
    score = shooting_score;
}

void shooting_count_hit(void)
{
    targets_hit += 1.0F;
}

void shooting_time_bonus(int steps)
{
    if (steps < 30 * 60) {
        shooting_score += 2000;
        score = shooting_score;
        return;
    }
    if (steps < 60 * 60) {
        shooting_score += 1000;
        score = shooting_score;
        return;
    }
    if (steps < 90 * 60) {
        shooting_score += 500;
        score = shooting_score;
    }
}
