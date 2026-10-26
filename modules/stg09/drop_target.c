/* Dtarget: a row of three drop targets on the tables. Dtarget2: the same row
 * at twice the size */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/model.h"
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/drop_target.h"
#include "stg09/pinball.h"
#include "system/task.h"
#include "system/util.h"

/* the first two are the housing, plain and lit */
extern NJS_MODEL drop_target_model;
extern NJS_MODEL drop_target_lit_model;
extern NJS_MODEL drop_target_piece_model;

struct drop_target_vars {
    Uint8 unk_00[2];
    /* the views the camera has moved to since the last pay, 0 to 3 */
    Uint8 views;
    /* the steps the housing has left to flash before the pieces stand
     * again */
    Uint8 rising;
    Uint8 unk_04[4];
    Float sunk; /* piece 0's sink */
};

#define VARS(work) ((struct drop_target_vars *)&(work)->vars)
/* the sinks of pieces 1 and 2 */
#define SUNK_1(work) ((work)->scl.x)
#define SUNK_2(work) ((work)->scl.y)
/* TRUE for Dtarget2, which draws and places its row at twice the size */
#define DOUBLED(work) ((work)->submode)

struct drop_target_piece_vars {
    Uint8 piece; /* its index in the row, 0 to 2 */
};

#define PIECE(work) ((struct drop_target_piece_vars *)&(work)->vars)

/* the row's work->flags. a piece is sinking from its hit until it reaches
 * DROP_TARGET_DEPTH. it is then down until the pieces stand again */
#define DROP_TARGET_0_SINKING 0x100
#define DROP_TARGET_0_DOWN 0x200
#define DROP_TARGET_1_SINKING 0x400
#define DROP_TARGET_1_DOWN 0x800
#define DROP_TARGET_2_SINKING 0x1000
#define DROP_TARGET_2_DOWN 0x2000
#define DROP_TARGET_ALL_DOWN                                                   \
    (DROP_TARGET_0_DOWN | DROP_TARGET_1_DOWN | DROP_TARGET_2_DOWN)
#define DROP_TARGET_LIT 0x4000

/* in the row's work->tick: the pieces that went down this step */
#define DROP_TARGET_0_DROPPED 0x1
#define DROP_TARGET_1_DROPPED 0x2
#define DROP_TARGET_2_DROPPED 0x4

/* the sink at which a piece is down. a sinking piece sinks a unit a step */
#define DROP_TARGET_DEPTH (-8.0F)
/* the steps the housing flashes before the pieces stand again. it is lit
 * while rising has DROP_TARGET_BLINK set */
#define DROP_TARGET_RISING 80
#define DROP_TARGET_BLINK 8
#define DROP_TARGET_SPACING 12.0F
/* the share of its forward speed a hit sends player 0 off at */
#define DROP_TARGET_KICK 0.5F

#define SE_DROP_TARGET 242

static struct collision_shape drop_target_shapes[3] = {
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {-DROP_TARGET_SPACING, 0.0F, 0.0F},
     5.0F,
     13.0F,
     0.5F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     13.0F,
     0.5F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {DROP_TARGET_SPACING, 0.0F, 0.0F},
     5.0F,
     13.0F,
     0.5F,
     {0},
     0,
     NULL},
};
static struct collision_shape drop_target2_shapes[3] = {
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {-DROP_TARGET_SPACING * 2.0F, 0.0F, 0.0F},
     10.0F,
     13.0F,
     0.5F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     10.0F,
     13.0F,
     0.5F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     ATTACK(3),
     PUSH_FIRM(PUSH_ALL),
     {DROP_TARGET_SPACING * 2.0F, 0.0F, 0.0F},
     10.0F,
     13.0F,
     0.5F,
     {0},
     0,
     NULL},
};

static void drop_target_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void drop_target_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float x;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njPushMatrix(NULL);
    if (DOUBLED(work)) {
        njScale(NULL, 2.0F, 2.0F, 2.0F);
    }
    if (!(work->flags & DROP_TARGET_LIT)) {
        draw_model(&drop_target_model);
    } else {
        draw_model(&drop_target_lit_model);
    }
    njPopMatrix(1);
    njPushMatrix(NULL);
    if (DOUBLED(work)) {
        njTranslate(NULL, -DROP_TARGET_SPACING * 2.0F, VARS(work)->sunk * 2.0F,
                    0.0F);
        njScale(NULL, 2.0F, 2.0F, 2.0F);
    } else {
        njTranslate(NULL, -DROP_TARGET_SPACING, VARS(work)->sunk, 0.0F);
    }
    draw_model(&drop_target_piece_model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    if (DOUBLED(work)) {
        njTranslate(NULL, 0.0F, SUNK_1(work) * 2.0F, 0.0F);
        njScale(NULL, 2.0F, 2.0F, 2.0F);
    } else {
        njTranslate(NULL, 0.0F, SUNK_1(work), 0.0F);
    }
    draw_model(&drop_target_piece_model);
    njPopMatrix(1);
    x = DROP_TARGET_SPACING;
    /* @hack redundant: a store to x that never runs, which keeps x a variable
     * in FR13. SHC then doubles piece 2's x at run time (FMUL FR15,FR4 at
     * 0x0c905004) rather than fold it. without it, the function runs 4 bytes
     * short and 98 of 165 words differ */
    if (0) {
        x = 0.0F;
    }
    if (DOUBLED(work)) {
        njTranslate(NULL, x * 2.0F, SUNK_2(work) * 2.0F, 0.0F);
        njScale(NULL, 2.0F, 2.0F, 2.0F);
    } else {
        njTranslate(NULL, x, SUNK_2(work), 0.0F);
    }
    draw_model(&drop_target_piece_model);
    njPopMatrix(1);
}

static void drop_target_piece_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *row = task->parent->work;
    NJS_VECTOR speed;
    Angle ang[3];
    int down;
    int sinking;
    struct task_work *player;

    switch (PIECE(work)->piece) {
    case 0:
        sinking = DROP_TARGET_0_SINKING;
        down = DROP_TARGET_0_DOWN;
        break;
    case 1:
        sinking = DROP_TARGET_1_SINKING;
        down = DROP_TARGET_1_DOWN;
        break;
    case 2:
        sinking = DROP_TARGET_2_SINKING;
        down = DROP_TARGET_2_DOWN;
        break;
    }
    if (!(row->flags & down)) {
        work->collision->shapes->flags &= ~SHAPE_OFF;
    } else {
        work->collision->shapes->flags |= SHAPE_OFF;
    }
    /* @hack spelling: the two bits added, ADD at 0x0c905116. or'd, they take
     * an OR, and 1 of 109 words differ */
    if ((work->collision->flags & COLLISION_HIT) &&
        !(row->flags & (down + sinking))) {
        player = player_work[0];
        row->flags |= sinking;
        ang[0] = 0;
        ang[1] = -player->ang.y;
        ang[2] = 0;
        speed.x = pinball_player_speed() * DROP_TARGET_KICK;
        speed.y = 0.0F;
        speed.z = 0.0F;
        player_send_flying(0, &speed, ang, 0);
        sound_play(SE_DROP_TARGET, 0, 0, 0);
        if (VARS(row)->views == 0) {
            pinball_give_rings(1);
        }
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

/* once all three pieces are down, the row pays rings and moves the tables'
 * camera to view 0. the pay is 15 rings while neither
 * CASINO_COUNTDOWN_ACT2_4 nor CASINO_COUNTDOWN_ACT3_2 runs. otherwise both
 * cannons fire, and the nth pay since that countdown started is (30n) mod
 * 256 rings. pinball_target_bonuses counts those pays */
static void drop_target_run(struct task *task)
{
    struct task_work *work = task->work;

    task_step_children(task);
    work->tick &= ~(DROP_TARGET_0_DROPPED | DROP_TARGET_1_DROPPED |
                    DROP_TARGET_2_DROPPED);
    if (work->flags & DROP_TARGET_0_SINKING) {
        VARS(work)->sunk -= 1.0F;
        if (VARS(work)->sunk < DROP_TARGET_DEPTH) {
            VARS(work)->sunk = DROP_TARGET_DEPTH;
            work->flags &= ~DROP_TARGET_0_SINKING;
            work->flags |= DROP_TARGET_0_DOWN;
            casino_flags &= ~CASINO_DROP_TARGET_0_UP;
            work->tick |= DROP_TARGET_0_DROPPED;
        }
    }
    if (work->flags & DROP_TARGET_1_SINKING) {
        SUNK_1(work) -= 1.0F;
        if (SUNK_1(work) < DROP_TARGET_DEPTH) {
            SUNK_1(work) = DROP_TARGET_DEPTH;
            work->flags &= ~DROP_TARGET_1_SINKING;
            work->flags |= DROP_TARGET_1_DOWN;
            casino_flags &= ~CASINO_DROP_TARGET_1_UP;
            work->tick |= DROP_TARGET_1_DROPPED;
        }
    }
    if (work->flags & DROP_TARGET_2_SINKING) {
        SUNK_2(work) -= 1.0F;
        if (SUNK_2(work) < DROP_TARGET_DEPTH) {
            SUNK_2(work) = DROP_TARGET_DEPTH;
            work->flags &= ~DROP_TARGET_2_SINKING;
            work->flags |= DROP_TARGET_2_DOWN;
            casino_flags &= ~CASINO_DROP_TARGET_2_UP;
            work->tick |= DROP_TARGET_2_DROPPED;
        }
    }
    if (VARS(work)->rising) {
        if (!(VARS(work)->rising & DROP_TARGET_BLINK)) {
            work->flags &= ~DROP_TARGET_LIT;
        } else {
            work->flags |= DROP_TARGET_LIT;
        }
        if (!--VARS(work)->rising) {
            VARS(work)->sunk = SUNK_1(work) = SUNK_2(work) = 0.0F;
            work->flags &= ~DROP_TARGET_ALL_DOWN;
            casino_flags |= CASINO_DROP_TARGET_0_UP | CASINO_DROP_TARGET_1_UP |
                            CASINO_DROP_TARGET_2_UP;
        }
    } else if ((work->flags & DROP_TARGET_ALL_DOWN) == DROP_TARGET_ALL_DOWN) {
        /* @bug logic: act 2's countdown runs only in act 2, and ends in
         * the first step outside it. act 3's starts from a hand shown in act
         * 3. the two never run together, so this test always holds (TST at
         * 0x0c90532a and 0x0c905332), and the row never takes views 4 to 6 */
        if (!(casino_flags & CASINO_COUNTDOWN_ACT2_4) ||
            !(casino_flags & CASINO_COUNTDOWN_ACT3_2) ||
            VARS(work)->views++ == 3) {
            /* the camera jumps to view 0's place and target, as the tables'
             * camera holds them */
            camera_work->scl.x = 9.0F;
            camera_work->scl.y = 120.0F;
            camera_work->scl.z = 204.0F;
            CAMERA_VARS(camera_work)->target.x = 9.0F;
            CAMERA_VARS(camera_work)->target.y = -33.0F;
            CAMERA_VARS(camera_work)->target.z = 26.0F;
            pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TABLE;
            VARS(work)->rising = DROP_TARGET_RISING;
            VARS(work)->views = 0;
            if (!(casino_flags & CASINO_COUNTDOWN_ACT2_4) &&
                !(casino_flags & CASINO_COUNTDOWN_ACT3_2)) {
                pinball_give_rings(15);
            } else {
                casino_flags |= CASINO_HOUDAI_L_FIRES;
                casino_flags |= CASINO_HOUDAI_R_FIRES;
                /* @bug arithmetic: pinball_give_rings keeps the count's low
                 * byte (0x0c9011ce). from the 9th pay, 30n passes 255, so the
                 * 9th pays 14 rings (MULU.W at 0x0c905416) */
                pinball_give_rings(++pinball_target_bonuses * 30);
            }
            casino_flags |= CASINO_HISTORY_RECORDING;
            casino_flags &= ~CASINO_COUNTDOWNS_HELD;
            if ((Uint8)stage_get_stage_act() == 3) {
                casino_flags |= CASINO_STOPPER_L_UP;
            }
        } else {
            /* while both countdowns run, the row pays nothing the first three
             * times its pieces are all down. it moves the camera to
             * PINBALL_VIEW_TARGETS_1, _2 and _3 in turn instead. the pieces
             * that went down this step stand again */
            if (work->tick & DROP_TARGET_0_DROPPED) {
                VARS(work)->sunk = 0.0F;
                work->flags &= ~DROP_TARGET_0_DOWN;
            }
            if (work->tick & DROP_TARGET_1_DROPPED) {
                SUNK_1(work) = 0.0F;
                work->flags &= ~DROP_TARGET_1_DOWN;
            }
            if (work->tick & DROP_TARGET_2_DROPPED) {
                SUNK_2(work) = 0.0F;
                work->flags &= ~DROP_TARGET_2_DOWN;
            }
            switch (VARS(work)->views) {
            case 1:
                casino_flags |= CASINO_COUNTDOWNS_HELD;
                camera_work->scl.x = 50.41F;
                camera_work->scl.y = 18.05F;
                camera_work->scl.z = 22.02F;
                CAMERA_VARS(camera_work)->target.x = 96.47F;
                CAMERA_VARS(camera_work)->target.y = 3.09F;
                CAMERA_VARS(camera_work)->target.z = -4.4F;
                pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TARGETS_1;
                break;
            case 2:
                camera_work->scl.x = 60.12F;
                camera_work->scl.y = 14.49F;
                camera_work->scl.z = -36.45F;
                CAMERA_VARS(camera_work)->target.x = 97.61F;
                CAMERA_VARS(camera_work)->target.y = 2.2F;
                CAMERA_VARS(camera_work)->target.z = -6.18F;
                pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TARGETS_2;
                break;
            case 3:
                camera_work->scl.x = 84.93F;
                camera_work->scl.y = 7.82F;
                camera_work->scl.z = 27.52F;
                CAMERA_VARS(camera_work)->target.x = 97.61F;
                CAMERA_VARS(camera_work)->target.y = 2.2F;
                CAMERA_VARS(camera_work)->target.z = -6.18F;
                pinball_view = PINBALL_VIEW_MOVING | PINBALL_VIEW_TARGETS_3;
                break;
            }
            /* nothing in this image records the history, so this sets player
             * 0's position, speed and angles to 0 */
            pinball_history_restore();
        }
    }
    drop_target_draw(task);
}

static void drop_target_piece_init(struct task *task, int piece)
{
    if (DOUBLED(task->parent->work) == 0) {
        collision_init(task, &drop_target_shapes[piece], COLLISION_OBJECT, 1,
                       1);
    } else {
        collision_init(task, &drop_target2_shapes[piece], COLLISION_OBJECT, 1,
                       1);
    }
    /* @bug memory: the record holds a copy of one shape, the
     * piece's. pieces 1 and 2 clear SHAPE_OFF in shapes[1] and shapes[2],
     * which start at the copy's end and 52 bytes past it (MUL.L R2,R13 at
     * 0x0c90566a, MOV.L R1,@R2 at 0x0c90567c) */
    task->work->collision->shapes[piece].flags &= ~SHAPE_OFF;
    PIECE(task->work)->piece = piece;
}

static void drop_target_init(struct task *task)
{
    struct task_work *work = task->work;

    drop_target_piece_init(
        task_create_child(TASK_WORK, drop_target_piece_run, task), 0);
    drop_target_piece_init(
        task_create_child(TASK_WORK, drop_target_piece_run, task), 1);
    drop_target_piece_init(
        task_create_child(TASK_WORK, drop_target_piece_run, task), 2);
    VARS(work)->sunk = SUNK_1(work) = SUNK_2(work) = 0.0F;
    work->flags &= ~DROP_TARGET_ALL_DOWN;
    casino_flags |= CASINO_DROP_TARGET_0_UP | CASINO_DROP_TARGET_1_UP |
                    CASINO_DROP_TARGET_2_UP;
    casino_flags |= CASINO_HISTORY_RECORDING;
    casino_flags &= ~CASINO_COUNTDOWNS_HELD;
    VARS(work)->views = 0;
    task->step = drop_target_run;
    task->draw = drop_target_draw;
    task->destructor = drop_target_destroy;
}

void drop_target_step(struct task *task)
{
    DOUBLED(task->work) = 0;
    drop_target_init(task);
}

void drop_target2_step(struct task *task)
{
    DOUBLED(task->work) = 1;
    drop_target_init(task);
}
