/* Roll, the first table's slot machine. a touch holds player 0 and spins
 * its three reels, and presses of A stop them in turn. three of one symbol
 * in a line start that symbol's countdown */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/ring.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/pinball.h"
#include "stg09/roll.h"
#include "stg09/roll_prize.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the three reels, then the nine lamps that light the found line. the
 * lamps run row by row from the upper row, each from left to right */
extern NJS_MODEL roll_left_reel_model;
extern NJS_MODEL roll_middle_reel_model;
extern NJS_MODEL roll_right_reel_model;
extern NJS_MODEL roll_upper_left_lamp_model;
extern NJS_MODEL roll_upper_lamp_model;
extern NJS_MODEL roll_upper_right_lamp_model;
extern NJS_MODEL roll_left_lamp_model;
extern NJS_MODEL roll_center_lamp_model;
extern NJS_MODEL roll_right_lamp_model;
extern NJS_MODEL roll_lower_left_lamp_model;
extern NJS_MODEL roll_lower_lamp_model;
extern NJS_MODEL roll_lower_right_lamp_model;

/* the line roll_read finds */
struct roll_line {
    Sint8 line; /* a ROLL_LINE_ value, or ROLL_NONE */
    Uint8 unk_11;
    Sint8 symbol; /* the line's symbol, or ROLL_NONE */
    Sint8 checks; /* the right reel's checks for a line since its press */
};

/* Roll's work->vars. each reel turns while its byte is nonzero */
struct roll_vars {
    Sint8 left;
    Sint8 middle;
    Sint8 right;
    Uint8 unk_0b[5];
    struct roll_line found;
};

#define VARS(work) ((struct roll_vars *)&(work)->vars)
/* the steps each reel has turned, counted in work's mode, submode and
 * tick */
#define LEFT_STEPS(work) ((work)->mode)
#define MIDDLE_STEPS(work) ((work)->submode)
#define RIGHT_STEPS(work) ((work)->tick)

/* a reel's ROLL_SYMBOLS symbols lie ROLL_SYMBOL_ANGLE apart. a turning reel
 * moves ROLL_STEP each step, a quarter of a symbol */
#define ROLL_SYMBOLS 16
#define ROLL_SYMBOL_ANGLE 0x1000
#define ROLL_STEP 0x400
/* a reel's byte while it turns */
#define ROLL_SPINNING 100
/* found.line and found.symbol until roll_read finds a line */
#define ROLL_NONE 100
/* after its press, the right reel checks for a line at each symbol it comes
 * to. it stops at the first line, or after ROLL_CHECKS checks */
#define ROLL_CHECKS 3

/* found.line's values. the first three are the upper, center and lower
 * rows. the last two are the diagonals down from the upper left and up to
 * the upper right */
#define ROLL_LINE_UPPER 0
#define ROLL_LINE_CENTER 1
#define ROLL_LINE_LOWER 2
#define ROLL_LINE_DOWN 3
#define ROLL_LINE_UP 4

/* the symbol whose line takes rings as it starts its countdown. every other
 * symbol's line gives rings */
#define ROLL_SYMBOL_TAKE 5

/* in work->flags. while a reel's press bit is set, Roll waits for the press
 * of A that stops the reel. the left reel's press comes first */
#define ROLL_LEFT_PRESS 0x100
#define ROLL_MIDDLE_PRESS 0x200
#define ROLL_RIGHT_PRESS 0x400
/* set from the touch that starts a game until Roll lets player 0 go */
#define ROLL_PLAYING 0x800
/* set until roll_pay reads the line */
#define ROLL_UNSCORED 0x1000
/* set from the touch that starts a game until a step without a touch.
 * each touch starts one game */
#define ROLL_TOUCHING 0x8000

/* the bits of the first table's six countdowns */
#define ACT2_COUNTDOWNS                                                        \
    (CASINO_COUNTDOWN_ACT2_0 | CASINO_COUNTDOWN_ACT2_1 |                       \
     CASINO_COUNTDOWN_ACT2_2 | CASINO_COUNTDOWN_ACT2_3 |                       \
     CASINO_COUNTDOWN_ACT2_4 | CASINO_COUNTDOWN_ACT2_5)

/* the steps Roll holds player 0 once the reels stop, while a line's lamps
 * flash. WIN_STEPS follows a line while no countdown of the first table
 * runs. WIN_STEPS_RUNNING follows a line while one runs, and LOSE_STEPS no
 * line */
#define WIN_STEPS 200
#define WIN_STEPS_RUNNING 90
#define LOSE_STEPS 30
/* roll_pay starts the prize when the hold has this many steps left */
#define PRIZE_STEPS_LEFT 150
/* the found line's lamps light while the timer has this bit set. they flash
 * 8 steps on and 8 off */
#define ROLL_LAMPS_LIT 0x8

#define SE_WIN 255
#define SE_TAKE 256

static struct collision_shape roll_shape = {
    SHAPE_SPHERE,
    ATTACK(3),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};
NJS_POINT3 pinball_ring_places[PINBALL_RING_PLACES] = {
#include "stg09/roll/pinball_ring_places.inc"
};
/* the symbols round each reel, 0 to 5. SYMBOL_AT is the symbol a reel at
 * angle ang shows in row. row is 1 for the upper row, 0 for the center and
 * -1 for the lower */
static Uint8 roll_left_symbols[ROLL_SYMBOLS] = {0, 1, 3, 1, 2, 3, 5, 0,
                                                3, 2, 3, 4, 0, 1, 2, 5};
static Uint8 roll_middle_symbols[ROLL_SYMBOLS] = {0, 5, 3, 5, 1, 2, 1, 4,
                                                  5, 3, 5, 1, 2, 1, 5, 3};
static Uint8 roll_right_symbols[ROLL_SYMBOLS] = {0, 5, 1, 3, 2, 1, 3, 2,
                                                 5, 3, 1, 0, 4, 2, 1, 3};
#define SYMBOL_AT(symbols, ang, row)                                           \
    ((symbols)[(((ang) / ROLL_SYMBOL_ANGLE) + (row)) & (ROLL_SYMBOLS - 1)])

static void roll_draw(struct task *task);
static void roll_read(struct task_work *work);
static void roll_pay(struct task_work *work);
static void roll_run(struct task *task);

/* draw the reels, and the found line's lamps. both draw at their own places
 * on the table, not at the task's */
static void roll_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    if (TASK_TIMER(work) & ROLL_LAMPS_LIT) {
        /* @hack spelling: the line read through its address, which SHC forms
         * before the load (ADD #16,R0 at 0x0c903cca). read as found.line,
         * 136 of 155 words differ */
        switch (*(const Sint8 *const)&VARS(work)->found.line) {
        case ROLL_LINE_UPPER:
            draw_model(&roll_upper_left_lamp_model);
            draw_model(&roll_upper_lamp_model);
            draw_model(&roll_upper_right_lamp_model);
            break;
        case ROLL_LINE_CENTER:
            draw_model(&roll_left_lamp_model);
            draw_model(&roll_center_lamp_model);
            draw_model(&roll_right_lamp_model);
            break;
        case ROLL_LINE_LOWER:
            draw_model(&roll_lower_left_lamp_model);
            draw_model(&roll_lower_lamp_model);
            draw_model(&roll_lower_right_lamp_model);
            break;
        case ROLL_LINE_DOWN:
            draw_model(&roll_upper_left_lamp_model);
            draw_model(&roll_center_lamp_model);
            draw_model(&roll_lower_right_lamp_model);
            break;
        case ROLL_LINE_UP:
            draw_model(&roll_upper_right_lamp_model);
            draw_model(&roll_center_lamp_model);
            draw_model(&roll_lower_left_lamp_model);
            break;
        }
    }
    njTranslate(NULL, -21.0F, -55.0F, 79.0F);
    njPushMatrix(NULL);
    TURN_VIA(njRotateX, work->ang.x, ang);
    draw_model(&roll_left_reel_model);
    njPopMatrix(1);
    njTranslate(NULL, 30.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    TURN_VIA(njRotateX, work->ang.y, ang);
    draw_model(&roll_middle_reel_model);
    njPopMatrix(1);
    njTranslate(NULL, 30.0F, 0.0F, 0.0F);
    TURN_VIA(njRotateX, work->ang.z, ang);
    draw_model(&roll_right_reel_model);
    njPopMatrix(1);
}

/* stop the left or the middle reel once it comes to a symbol. a turning
 * reel comes to one every fourth step */
static void roll_stop_left(struct task_work *work)
{
    if (!(LEFT_STEPS(work) & 3)) {
        VARS(work)->left = 0;
    }
}

static void roll_stop_middle(struct task_work *work)
{
    if (!(MIDDLE_STEPS(work) & 3)) {
        VARS(work)->middle = 0;
    }
}

/* find the first line of three like symbols, trying the upper row first.
 * once a line is found, stop the right reel */
static void roll_read(struct task_work *work)
{
    Uint8 left_upper;
    Uint8 middle_upper;
    Uint8 right_upper;
    Uint8 left_center;
    Uint8 middle_center;
    Uint8 right_center;
    Uint8 left_lower;
    Uint8 middle_lower;
    Uint8 right_lower;

    left_upper = SYMBOL_AT(roll_left_symbols, work->ang.x, 1);
    middle_upper = SYMBOL_AT(roll_middle_symbols, work->ang.y, 1);
    right_upper = SYMBOL_AT(roll_right_symbols, work->ang.z, 1);
    left_center = SYMBOL_AT(roll_left_symbols, work->ang.x, 0);
    middle_center = SYMBOL_AT(roll_middle_symbols, work->ang.y, 0);
    right_center = SYMBOL_AT(roll_right_symbols, work->ang.z, 0);
    left_lower = SYMBOL_AT(roll_left_symbols, work->ang.x, -1);
    middle_lower = SYMBOL_AT(roll_middle_symbols, work->ang.y, -1);
    right_lower = SYMBOL_AT(roll_right_symbols, work->ang.z, -1);
    /* @hack spelling: each line stored through its address, which SHC forms
     * before the store (ADD #16,R3 at 0x0c903eee). stored as found.line, the
     * function runs 8 bytes short and 68 of 147 words differ */
    if (left_upper == middle_upper && middle_upper == right_upper) {
        *(Sint8 *const)&VARS(work)->found.line = ROLL_LINE_UPPER;
        VARS(work)->found.symbol = left_upper;
    } else if (left_center == middle_center && middle_center == right_center) {
        *(Sint8 *const)&VARS(work)->found.line = ROLL_LINE_CENTER;
        VARS(work)->found.symbol = left_center;
    } else if (left_lower == middle_lower && middle_lower == right_lower) {
        *(Sint8 *const)&VARS(work)->found.line = ROLL_LINE_LOWER;
        VARS(work)->found.symbol = left_lower;
    } else if (left_upper == middle_center && middle_center == right_lower) {
        *(Sint8 *const)&VARS(work)->found.line = ROLL_LINE_DOWN;
        VARS(work)->found.symbol = left_upper;
    } else if (right_upper == middle_center && middle_center == left_lower) {
        *(Sint8 *const)&VARS(work)->found.line = ROLL_LINE_UP;
        VARS(work)->found.symbol = right_upper;
    }
    if (VARS(work)->found.symbol != ROLL_NONE) {
        VARS(work)->right = 0;
    }
}

/* each time the turning right reel comes to a symbol, count the check and
 * check for a line */
static void roll_stop_right(struct task_work *work)
{
    if (!(RIGHT_STEPS(work) & 3) && VARS(work)->right) {
        /* @hack spelling: the count's increment written out (ADD #1,R0 at
         * 0x0c903fc2). as ++found.checks, the function runs 4 bytes short and
         * 15 of 22 words differ */
        if ((VARS(work)->found.checks = VARS(work)->found.checks + 1) ==
            ROLL_CHECKS) {
            VARS(work)->right = 0;
        }
        roll_read(work);
    }
}

static void roll_press_right(struct task_work *work)
{
    if (per_info[0]->press & PDD_DGT_TA) {
        work->flags &= ~ROLL_RIGHT_PRESS;
    }
    roll_stop_left(work);
    roll_stop_middle(work);
}

static void roll_press_middle(struct task_work *work)
{
    if (per_info[0]->press & PDD_DGT_TA) {
        work->flags &= ~ROLL_MIDDLE_PRESS;
    }
    roll_stop_left(work);
}

static void roll_press_left(struct task_work *work)
{
    if (per_info[0]->press & PDD_DGT_TA) {
        VARS(work)->found.checks = 0;
        work->flags &= ~ROLL_LEFT_PRESS;
    }
}

/* once the reels stop, score the line. hold player 0 while the lamps flash,
 * then send him flying. a line starts its symbol's countdown unless a
 * countdown of the first table runs. while the take symbol's countdown runs
 * alone, a line of it fires both Houdai and gives 100 rings */
static void roll_pay(struct task_work *work)
{
    struct task *countdown;
    struct task_work *ring;
    NJS_VECTOR speed;
    Angle ang[3];
    int i;

    if (work->flags & ROLL_UNSCORED) {
        work->flags &= ~ROLL_UNSCORED;
        /* @hack spelling: the line read through its address, as in roll_draw
         * (ADD #16,R0 at 0x0c90408a). read as found.line, 36 of 348 words
         * differ */
        if (*(const Sint8 *const)&VARS(work)->found.line != ROLL_NONE) {
            if (!(casino_flags & ACT2_COUNTDOWNS)) {
                work->timer = WIN_STEPS;
            } else {
                work->timer = WIN_STEPS_RUNNING;
            }
            if (VARS(work)->found.symbol == ROLL_SYMBOL_TAKE &&
                !(casino_flags & ACT2_COUNTDOWNS)) {
                sound_play(SE_TAKE, 0, 0, 0);
            } else {
                sound_play(SE_WIN, 0, 0, 0);
            }
        } else {
            work->timer = LOSE_STEPS;
        }
    } else if (--TASK_TIMER(work) == 0) {
        player_set_mode(0, PLAYER_MODE_SLOPE_ROLL);
        ang[0] = 0;
        ang[1] = 0xf800;
        ang[2] = 0;
        speed.x = 2.0F + ((Float)rand() * (1.0F / 65536.0F));
        speed.y = 0.5F;
        speed.z = 0.6F + ((Float)rand() * (0.2F * RAND_SCALE));
        player_send_flying(0, &speed, ang, 0);
        pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TABLE;
        work->flags &= ~ROLL_PLAYING;
        if (!(casino_flags & (ACT2_COUNTDOWNS & ~CASINO_COUNTDOWN_ACT2_5))) {
            if (!(casino_flags & CASINO_COUNTDOWN_ACT2_5)) {
                /* the line's symbol picks the countdown to start, and the
                 * rings it gives or takes. found.symbol is ROLL_NONE without
                 * a line, and starts none */
                switch (VARS(work)->found.symbol) {
                case 0:
                    casino_flags |= CASINO_COUNTDOWN_ACT2_0;
                    casino_flags &=
                        ~(CASINO_COUNTDOWN0_HIDE_A | CASINO_COUNTDOWN0_HIDE_B);
                    task_create(TASK_WORK, 3, pinball_act2_countdown0_step)
                        ->work->timer = 5940;
                    pinball_give_rings(50);
                    break;
                case 1:
                    casino_flags |= CASINO_COUNTDOWN_ACT2_1;
                    countdown =
                        task_create(TASK_WORK, 3, pinball_act2_countdown1_step);
                    countdown->work->timer = 1800;
                    for (i = 0; i < PINBALL_RING_PLACES; i++) {
                        ring =
                            task_create_child(TASK_WORK, ring_step, countdown)
                                ->work;
                        ring->pos.x = pinball_ring_places[i].x;
                        ring->pos.y = pinball_ring_places[i].y;
                        ring->pos.z = pinball_ring_places[i].z;
                    }
                    pinball_give_rings(15);
                    break;
                case 2:
                    casino_flags |= CASINO_COUNTDOWN_ACT2_2;
                    pinball_countdown2_place = 0;
                    task_create(TASK_WORK, 3, pinball_act2_countdown2_step)
                        ->work->timer = 4800;
                    pinball_give_rings(30);
                    break;
                case 3:
                    casino_flags |=
                        CASINO_COUNTDOWN_ACT2_3 | CASINO_STOPPER_C_UP;
                    task_create(TASK_WORK, 3, pinball_act2_countdown3_step)
                        ->work->timer = 2700;
                    pinball_give_rings(15);
                    break;
                case 4:
                    casino_flags |= CASINO_COUNTDOWN_ACT2_4;
                    pinball_target_bonuses = 0;
                    task_create(TASK_WORK, 3, pinball_act2_countdown4_step)
                        ->work->timer = 3600;
                    pinball_give_rings(50);
                    break;
                case ROLL_SYMBOL_TAKE:
                    casino_flags |= CASINO_COUNTDOWN_ACT2_5;
                    task_create(TASK_WORK, 3, pinball_act2_countdown5_step)
                        ->work->timer = 3600;
                    pinball_take_rings(50);
                    break;
                }
            } else if (VARS(work)->found.symbol == ROLL_SYMBOL_TAKE) {
                casino_flags |= CASINO_HOUDAI_L_FIRES;
                casino_flags |= CASINO_HOUDAI_R_FIRES;
                pinball_give_rings(100);
            }
        }
    } else if (TASK_TIMER(work) == PRIZE_STEPS_LEFT) {
        if (!(casino_flags & ACT2_COUNTDOWNS) &&
            VARS(work)->found.symbol != ROLL_NONE) {
            task_create(TASK_WORK, 1, roll_prize_step)->work->mode =
                VARS(work)->found.symbol;
        }
    }
}

/* Roll's step after the first. a touch curls player 0 up in the Roll and
 * spins the reels. it has CC3 ease the camera to view 1, and roll_pay has
 * it ease back to view 0 */
static void roll_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos;

    if (work->flags & ROLL_PLAYING) {
        if (work->flags & ROLL_LEFT_PRESS) {
            roll_press_left(work);
        } else if (work->flags & ROLL_MIDDLE_PRESS) {
            roll_press_middle(work);
        } else if (work->flags & ROLL_RIGHT_PRESS) {
            roll_press_right(work);
        } else {
            /* @hack spelling: the reels tested through a pointer of their own
             * (ADD #8,R4 at 0x0c9044b0). tested through VARS(work), the
             * function runs 32 bytes short and 124 of 157 words differ */
            struct roll_vars *reels = VARS(work);

            if (reels->left || reels->middle || reels->right) {
                roll_stop_left(work);
                roll_stop_middle(work);
                roll_stop_right(work);
            } else {
                roll_pay(work);
            }
        }
        /* @hack spelling: the left reel's test breaks out of a loop of one
         * pass. as a plain if, 9 of 157 words differ */
        do {
            if (!VARS(work)->left) {
                break;
            }
            work->ang.x += ROLL_STEP;
            LEFT_STEPS(work)++;
        } while (0);
        if (VARS(work)->middle) {
            work->ang.y += ROLL_STEP;
            MIDDLE_STEPS(work)++;
        }
        /* @hack spelling: the right reel's test breaks out of a loop of one
         * pass. as a plain if, 3 of 157 words differ */
        do {
            if (!VARS(work)->right) {
                break;
            }
            work->ang.z += ROLL_STEP;
            RIGHT_STEPS(work)++;
        } while (0);
    } else if (work->collision->flags & COLLISION_HIT) {
        if (!(work->flags & ROLL_TOUCHING)) {
            struct roll_vars *reels;
            struct roll_line *found;

            pos = &player_work[0]->pos;
            work->flags |= ROLL_LEFT_PRESS | ROLL_MIDDLE_PRESS |
                           ROLL_RIGHT_PRESS | ROLL_PLAYING | ROLL_UNSCORED;
            /* @hack spelling: the reels set through a pointer of their own
             * (ADD #8,R4 at 0x0c904528). set through VARS(work), the
             * function runs 32 bytes short and 68 of 157 words differ */
            reels = VARS(work);
            reels->left = reels->middle = reels->right = ROLL_SPINNING;
            pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_ROLL;
            player_set_mode(0, PLAYER_MODE_BALL);
            pos->x = work->pos.x;
            pos->y = work->pos.y - 10.0F;
            pos->z = work->pos.z;
            pinball_give_rings(5);
            work->flags |= ROLL_TOUCHING;
            /* @hack spelling: the line and symbol set through a pointer of
             * their own. set through VARS(work), 7 of 157 words differ */
            found = &VARS(work)->found;
            found->line = found->symbol = ROLL_NONE;
        }
    } else {
        work->flags &= ~ROLL_TOUCHING;
    }
    roll_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

void roll_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &roll_shape, COLLISION_OBJECT, 1, 1);
    work->flags &= ~(ROLL_LEFT_PRESS | ROLL_MIDDLE_PRESS | ROLL_RIGHT_PRESS |
                     ROLL_PLAYING | ROLL_UNSCORED);
    work->mode = work->submode = work->tick = 0;
    casino_flags &= ~ACT2_COUNTDOWNS;
    task->step = roll_run;
    task->draw = roll_draw;
}
