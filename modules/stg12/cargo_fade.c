/* screen fade to black and back as the cargo ride starts and ends */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "stg12/cargo_fade.h"
/* the sprite's generated data names hotshelter4_texlist */
#include "stg12/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/frame.h"
#include "system/task.h"

#define FADE_SPEED 0.01F /* the alpha the fade gains or loses each step */
/* the screen holds black until its count of steps passes HOLD_STEPS, so for
 * 31 steps */
#define HOLD_STEPS 30

/* the scales that stretch the 64-pixel sprite over the 640 by 480 screen,
 * and the screen's center */
#define COVER_X 10.0F
#define COVER_Y 7.5F
#define MIDDLE_X 320.0F
#define MIDDLE_Y 240.0F

enum cargo_fade_mode { FADE_TO_BLACK, FADE_HOLD, FADE_BACK };

/* TASK_TIMER(work) counts the steps the screen is held black */
struct cargo_fade_vars {
    Float alpha; /* the sprite's: it rises from -0.5 to 1, then falls to 0 */
};

#define VARS(work) ((struct cargo_fade_vars *)&(work)->vars)

static void cargo_fade_step(struct task *task);
static void cargo_fade_draw(struct task *task);

static NJS_TEXANIM cargo_fade_frame = {
#include "stg12/cargo_fade/cargo_fade_frame.inc"
};
static NJS_SPRITE cargo_fade_sprite = {
#include "stg12/cargo_fade/cargo_fade_sprite.inc"
};

void cargo_fade_start(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = FADE_TO_BLACK;
    VARS(work)->alpha = -0.5F;
    task->step = cargo_fade_step;
    task->draw = cargo_fade_draw;
}

static void cargo_fade_step(struct task *task)
{
    struct task_work *work = task->work;

    cargo_fade_draw(task);
    switch (work->mode) {
    case FADE_TO_BLACK:
        VARS(work)->alpha += FADE_SPEED;
        if (VARS(work)->alpha >= 1.0F) {
            VARS(work)->alpha = 1.0F;
            work->timer = 0;
            work->mode = FADE_HOLD;
        }
        break;
    case FADE_HOLD:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > HOLD_STEPS) {
            work->timer = 0;
            work->mode = FADE_BACK;
        }
        break;
    case FADE_BACK:
        VARS(work)->alpha -= FADE_SPEED;
        if (VARS(work)->alpha <= 0.0F) {
            VARS(work)->alpha = 0.0F;
            task_kill(task);
        }
        break;
    }
}

/* a frame runs frame_step_count steps, and the stage's objects draw in the
 * first. the fade draws in each of the others */
static void cargo_fade_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material(VARS(work)->alpha, 0.0F, 0.0F, 0.0F);
        cargo_fade_sprite.sx = COVER_X;
        cargo_fade_sprite.sy = COVER_Y;
        cargo_fade_sprite.p.x = MIDDLE_X;
        cargo_fade_sprite.p.y = MIDDLE_Y;
        njDrawSprite2D(&cargo_fade_sprite, 0, -0.9F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}
