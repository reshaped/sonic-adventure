/* the prize for a line on Roll's reels. the screen dims, and the symbol's
 * word comes in a letter at a time, each with a flash. a model then grows,
 * and the screen turns white and fades */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/bomb.h" /* NOLINT(misc-include-cleaner) */
#include "effect/explosion.h"
#include "effect/hit.h" /* NOLINT(misc-include-cleaner) */
#include "effect/snow.h" /* NOLINT(misc-include-cleaner) */
#include "effect/sparkle.h" /* NOLINT(misc-include-cleaner) */
#include "effect/splash.h" /* NOLINT(misc-include-cleaner) */
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "stg09/roll_prize.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* in work->flags, what the prize draws: the screen's overlay, the letters
 * shown so far, and the final model */
#define PRIZE_OVERLAY 0x100
#define PRIZE_WORD 0x200
#define PRIZE_FINAL 0x400

/* the prize's work->vars: the overlay's alpha, and its gray, from black at 0
 * to white at 1 */
struct roll_prize_vars {
    Float alpha;
    Float shade;
};
#define VARS(work) ((struct roll_prize_vars *)&(work)->vars)

extern NJS_MODEL roll_prize_0_model_0;
extern NJS_MODEL roll_prize_0_model_1;
extern NJS_MODEL roll_prize_0_model_2;
extern NJS_MODEL roll_prize_0_model_3;
extern NJS_MODEL roll_prize_0_model_4;
extern NJS_MODEL roll_prize_0_model_5;
extern NJS_MODEL roll_prize_0_model_6;
extern NJS_MODEL roll_prize_0_model_7;
extern NJS_MODEL roll_prize_0_model_8;
extern NJS_MODEL roll_prize_1_model_0;
extern NJS_MODEL roll_prize_1_model_1;
extern NJS_MODEL roll_prize_1_model_2;
extern NJS_MODEL roll_prize_1_model_3;
extern NJS_MODEL roll_prize_1_model_4;
extern NJS_MODEL roll_prize_1_model_5;
extern NJS_MODEL roll_prize_1_model_6;
extern NJS_MODEL roll_prize_2_model_0;
extern NJS_MODEL roll_prize_2_model_1;
extern NJS_MODEL roll_prize_2_model_2;
extern NJS_MODEL roll_prize_2_model_3;
extern NJS_MODEL roll_prize_2_model_4;
extern NJS_MODEL roll_prize_2_model_5;
extern NJS_MODEL roll_prize_2_model_6;
extern NJS_MODEL roll_prize_2_model_7;
extern NJS_MODEL roll_prize_3_model_0;
extern NJS_MODEL roll_prize_3_model_1;
extern NJS_MODEL roll_prize_3_model_2;
extern NJS_MODEL roll_prize_3_model_3;
extern NJS_MODEL roll_prize_3_model_4;
extern NJS_MODEL roll_prize_3_model_5;
extern NJS_MODEL roll_prize_3_model_6;
extern NJS_MODEL roll_prize_3_model_7;
extern NJS_MODEL roll_prize_3_model_8;
extern NJS_MODEL roll_prize_3_model_9;
extern NJS_MODEL roll_prize_4_model_0;
extern NJS_MODEL roll_prize_4_model_1;
extern NJS_MODEL roll_prize_4_model_2;
extern NJS_MODEL roll_prize_4_model_3;
extern NJS_MODEL roll_prize_4_model_4;
extern NJS_MODEL roll_prize_4_model_5;
extern NJS_MODEL roll_prize_4_model_6;
extern NJS_MODEL roll_prize_4_model_7;
extern NJS_MODEL roll_prize_5_model_0;
extern NJS_MODEL roll_prize_5_model_1;
extern NJS_MODEL roll_prize_5_model_2;
extern NJS_MODEL roll_prize_5_model_3;
extern NJS_MODEL roll_prize_5_model_4;
extern NJS_MODEL roll_prize_5_model_5;
extern NJS_MODEL roll_prize_5_model_6;
extern NJS_MODEL roll_prize_5_model_7;
extern NJS_MODEL roll_prize_5_model_8;
extern NJS_MODEL roll_prize_5_model_9;
extern NJS_MODEL roll_prize_5_model_10;
extern NJS_MODEL roll_prize_final_model;

static NJS_TEXANIM bomb_flash_anim[8] = {
#include "stg09/roll_prize/bomb_flash_anim.inc"
};
static NJS_SPRITE bomb_flash = {
#include "stg09/roll_prize/bomb_flash.inc"
};

static NJS_TEXANIM sparkle_flash_anim[8] = {
#include "stg09/roll_prize/sparkle_flash_anim.inc"
};
static NJS_SPRITE sparkle_flash = {
#include "stg09/roll_prize/sparkle_flash.inc"
};

static NJS_TEXANIM explosion_flash_anim[8] = {
#include "stg09/roll_prize/explosion_flash_anim.inc"
};
static NJS_SPRITE explosion_flash = {
#include "stg09/roll_prize/explosion_flash.inc"
};

static NJS_TEXANIM hit_flash_anim[8] = {
#include "stg09/roll_prize/hit_flash_anim.inc"
};
static NJS_SPRITE hit_flash = {
#include "stg09/roll_prize/hit_flash.inc"
};

static NJS_TEXANIM splash_flash_anim[8] = {
#include "stg09/roll_prize/splash_flash_anim.inc"
};
static NJS_SPRITE splash_flash = {
#include "stg09/roll_prize/splash_flash.inc"
};

static NJS_TEXANIM snow_flash_anim[8] = {
#include "stg09/roll_prize/snow_flash_anim.inc"
};
static NJS_SPRITE snow_flash = {
#include "stg09/roll_prize/snow_flash.inc"
};

/* the sprite that flashes each letter in, by symbol */
static NJS_SPRITE *roll_prize_flashes[6] = {&snow_flash,      &sparkle_flash,
                                            &explosion_flash, &hit_flash,
                                            &splash_flash,    &bomb_flash};

/* each symbol's letters: x and y place a letter, and z scales its flash */
static NJS_POINT3 roll_prize_places[6][11] = {
#include "stg09/roll_prize/roll_prize_places.inc"
};
/* the letters in each symbol's word */
static Uint8 roll_prize_counts[] = {9, 7, 8, 10, 8, 11};

/* each letter's model, each row padded with its last */
static NJS_MODEL *roll_prize_models[6][11] = {
    {&roll_prize_0_model_0, &roll_prize_0_model_1, &roll_prize_0_model_2,
     &roll_prize_0_model_3, &roll_prize_0_model_4, &roll_prize_0_model_5,
     &roll_prize_0_model_6, &roll_prize_0_model_7, &roll_prize_0_model_8,
     &roll_prize_0_model_8, &roll_prize_0_model_8},
    {&roll_prize_1_model_0, &roll_prize_1_model_1, &roll_prize_1_model_2,
     &roll_prize_1_model_3, &roll_prize_1_model_4, &roll_prize_1_model_5,
     &roll_prize_1_model_6, &roll_prize_1_model_6, &roll_prize_1_model_6,
     &roll_prize_1_model_6, &roll_prize_1_model_6},
    {&roll_prize_2_model_0, &roll_prize_2_model_1, &roll_prize_2_model_2,
     &roll_prize_2_model_3, &roll_prize_2_model_4, &roll_prize_2_model_5,
     &roll_prize_2_model_6, &roll_prize_2_model_7, &roll_prize_2_model_7,
     &roll_prize_2_model_7, &roll_prize_2_model_7},
    {&roll_prize_3_model_0, &roll_prize_3_model_1, &roll_prize_3_model_2,
     &roll_prize_3_model_3, &roll_prize_3_model_4, &roll_prize_3_model_5,
     &roll_prize_3_model_6, &roll_prize_3_model_7, &roll_prize_3_model_8,
     &roll_prize_3_model_9, &roll_prize_3_model_9},
    {&roll_prize_4_model_0, &roll_prize_4_model_1, &roll_prize_4_model_2,
     &roll_prize_4_model_3, &roll_prize_4_model_4, &roll_prize_4_model_5,
     &roll_prize_4_model_6, &roll_prize_4_model_7, &roll_prize_4_model_7,
     &roll_prize_4_model_7, &roll_prize_4_model_7},
    {&roll_prize_5_model_0, &roll_prize_5_model_1, &roll_prize_5_model_2,
     &roll_prize_5_model_3, &roll_prize_5_model_4, &roll_prize_5_model_5,
     &roll_prize_5_model_6, &roll_prize_5_model_7, &roll_prize_5_model_8,
     &roll_prize_5_model_9, &roll_prize_5_model_10},
};

static void roll_prize_destroy(struct task *task)
{
}

static void roll_prize_draw(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    Angle angle;
    /* @hack spelling: the flags read through a local, which the overlay's
     * test alone reads (MOV R0,R4 and TST R3,R4 at 0x0c909760). tested as
     * work->flags there, 126 of 152 words differ */
    int flags = work->flags;

    if (flags & PRIZE_OVERLAY) {
        draw_set_material(VARS(work)->alpha, VARS(work)->shade,
                          VARS(work)->shade, VARS(work)->shade);
        njDrawSprite2D(&card_white_sprite, 0, -5.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);

    TURN_VIA(njRotateX, work->ang.x, angle);
    njScale(NULL, 0.03F, 0.03F, 0.03F);
    if (work->flags & PRIZE_WORD) {
        for (i = 0; i < work->submode; i++) {
            njPushMatrix(NULL);
            njTranslate(NULL, roll_prize_places[work->mode][i].x,
                        roll_prize_places[work->mode][i].y, 0.0F);
            draw_model(roll_prize_models[work->mode][i]);
            njPopMatrix(1);
        }
    }
    if (work->flags & PRIZE_FINAL) {
        njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
        draw_model(&roll_prize_final_model);
    }
    njPopMatrix(1);
}

/* a letter's flash. it waits two frames for each letter before it, then
 * flashes for eight. after the third frame of its flash, the prize shows one
 * more letter */
static void roll_prize_flash_run(struct task *task)
{
    struct task_work *work = task->work;
    int symbol;

    if (work->timer == 0) {
        /* the prize's work->mode is its symbol, a Sint8 from 0 to 5, so
         * widening it keeps its value */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        symbol = task->parent->work->mode;
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        roll_prize_flashes[symbol]->p.x =
            320.0F + (roll_prize_places[symbol][work->mode].x * 2.8F);
        roll_prize_flashes[symbol]->p.y =
            240.0F - (roll_prize_places[symbol][work->mode].y * 2.8F);
        roll_prize_flashes[symbol]->sx = roll_prize_flashes[symbol]->sy =
            roll_prize_places[symbol][work->mode].z;
        njDrawSprite2D(roll_prize_flashes[symbol], work->tick, -3.0F,
                       NJD_SPRITE_ALPHA);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        if (++work->tick == 8) {
            task_kill(task);
        }
        if (work->tick == 3) {
            task->parent->work->submode++;
        }
        /* @hack redundant: a return where control returns anyway. it gives the
         * image's BRA to the epilogue, with the parent's store in its delay
         * slot (0x0c9099c2). without it, 13 of 124 words differ */
        return;
    } else {
        --work->timer;
    }
}

/* the prize's step after the first. it ends the prize once the act is no
 * longer the first table. work->tick counts frames down from 75, and the
 * prize ends at 0 */
static void roll_prize_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *child;
    int i;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 2)) {
        task_kill(task);
        return;
    }
    if (frame_step) {
        return;
    }
    switch (work->tick--) {
    case 0:
        task_kill(task);
        return;
    /* from 4 to 1, the overlay fades */
    case 1:
    case 2:
    case 3:
    case 4:
        VARS(work)->alpha -= 0.2F;
        break;
    /* from 8 to 5, the overlay turns white and the final model stretches.
     * at 5, the word and the model go */
    case 5:
        work->flags &= ~(PRIZE_WORD | PRIZE_FINAL);
    case 6:
        VARS(work)->alpha += 0.25F;
    case 7:
    case 8:
        VARS(work)->shade += 0.25F;
        work->scl.x += 0.05F;
        work->scl.y += 0.8F;
        break;
    /* from 12 to 9, the final model shows and grows */
    case 12:
        work->flags |= PRIZE_FINAL;
    case 9:
    case 10:
    case 11:
        work->scl.x += 0.25F;
        work->scl.y += 0.025F;
        break;
    /* from 75 to 70, the overlay darkens. at 70, the letters start to flash
     * in */
    case 70:
        work->flags |= PRIZE_WORD;
        for (i = 0; i < roll_prize_counts[work->mode]; i++) {
            child =
                task_create_child(TASK_WORK, roll_prize_flash_run, task)->work;
            child->timer = i * 2;
            child->mode = i;
            child->submode = roll_prize_counts[work->mode];
            child->tick = 0;
        }
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
        VARS(work)->alpha += 0.1F;
        break;
    }
    work->ang.x =
        -(Angle)(atan2f(CAMERA_VARS(camera_work)->target.y - camera_work->scl.y,
                        CAMERA_VARS(camera_work)->target.z -
                            camera_work->scl.z) *
                 ANGLE_PER_RADIAN) +
        0x8000;
    work->pos.x = camera_work->scl.x;
    work->pos.y = camera_work->scl.y + (njSin(work->ang.x) * 4.0F);
    work->pos.z = camera_work->scl.z - (njCos(work->ang.x) * 4.0F);
    task_step_children(task);
    roll_prize_draw(task);
}

void roll_prize_step(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = 75;
    work->pos.y += 50.0F;
    work->submode = 0;
    work->flags &= ~(PRIZE_WORD | PRIZE_FINAL);
    work->flags |= PRIZE_OVERLAY;
    VARS(work)->alpha = VARS(work)->shade = 0.0F;
    work->scl.x = work->scl.y = 0.0F;
    card_white_sprite.sx = 80.0F;
    card_white_sprite.sy = 60.0F;
    card_white_sprite.p.x = card_white_sprite.p.y = 0.0F;
    task->step = roll_prize_run;
    task->draw = roll_prize_draw;
    task->destructor = roll_prize_destroy;
}
