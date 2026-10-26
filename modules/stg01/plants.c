/* O MKUSA and O BKUSA plants, and O K2, a plant that sends up butterflies as
 * player 0 runs past */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg01/plants.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL beach_mkusa_model;
extern NJS_OBJECT beach_mkusa_leaf_1_object;
extern NJS_OBJECT beach_mkusa_leaf_2_object;
extern NJS_OBJECT beach_mkusa_leaf_3_object;
extern NJS_OBJECT beach_mkusa_leaf_4_object;
extern NJS_OBJECT beach_mkusa_leaf_5_object;
extern NJS_OBJECT beach_mkusa_leaf_6_object;
extern NJS_OBJECT beach_mkusa_leaf_7_object;
extern NJS_OBJECT beach_mkusa_leaf_8_object;
extern NJS_OBJECT beach_bkusa_a_object;
extern NJS_OBJECT beach_bkusa_b_object;
extern NJS_OBJECT beach_bkusa_other_object;
extern NJS_OBJECT beach_k2_object;
/* one quad folded along a diagonal, whose two halves are a butterfly's
 * wings */
extern NJS_OBJECT beach_butterfly_object;

/* the plants' modes. only beach_kusa_touch, which O BKUSA's step calls,
 * enters KUSA_SWAY. nothing here enters KUSA_STOP */
enum beach_kusa_mode { KUSA_INIT, KUSA_STILL, KUSA_SWAY, KUSA_STOP };

/* a butterfly flutters until it rises above its height, then falls until it
 * drops below it */
enum beach_butterfly_mode {
    BUTTERFLY_INIT = 0,
    BUTTERFLY_FLUTTER = 3,
    BUTTERFLY_FALL = 6
};

/* a plant's sway: scl.y holds its amplitude, and scl.x its offset this step.
 * neither draw reads them, so a swaying plant draws as a still one does */
struct beach_kusa_vars {
    Float phase; /* the sway's phase, in degrees */
};

#define PHASE(work) (((struct beach_kusa_vars *)&(work)->vars)->phase)

/* a touch starts the sway at SWAY_AMPLITUDE. each step, the phase moves on
 * SWAY_SPEED degrees and the amplitude loses SWAY_DECAY. the sway ends once
 * the amplitude falls below 0, 223 steps on */
#define SWAY_AMPLITUDE 10.0F
#define SWAY_SPEED 3.5F
#define SWAY_DECAY 0.045F

/* player 0 touches O BKUSA by running on the ground within TOUCH_RANGE of it
 * at TOUCH_SPEED or faster */
#define TOUCH_RANGE 20.0F
#define TOUCH_SPEED 0.2F

/* O BKUSA's scl.z picks its model: beach_bkusa_a_object at 0,
 * beach_bkusa_other_object above OTHER_MODEL, and beach_bkusa_b_object
 * otherwise */
#define OTHER_MODEL 5.0F

/* scl holds a butterfly's speed, tick its scale in 32nds, and
 * TASK_TIMER(work) the steps it has left */
struct beach_butterfly_vars {
    Uint16 spin_x; /* the turn added to ang.x each step */
    Uint16 spin_y; /* the turn added to ang.y each step */
    Uint8 unk_04[4];
    Float height; /* the height the butterfly flutters about */
};

#define BUTTERFLY(work) ((struct beach_butterfly_vars *)&(work)->vars)

#define BUTTERFLY_STEPS 600

/* each spin is random, from -SPIN / 2 up to SPIN / 2 */
#define SPIN (65536.0F / 20.0F)

/* while a butterfly flutters, its x and z speeds gain PUSH a step along
 * ang.y. its y speed gains PUSH times sin(2 * ang.x). while it falls, its y
 * speed loses PUSH a step, down to -FALL_MOST */
#define PUSH 0.1F
#define FALL_MOST 0.67F

/* a step moves a butterfly by its x and z speeds times MOVE_XZ. it moves by
 * its y speed times MOVE_Y */
#define MOVE_XZ (1.0F / 24.0F)
#define MOVE_Y 0.1F

/* O K2 sends up butterflies while player 0 runs on the ground near it. he
 * must be within SCATTER_RANGE, at SCATTER_SPEED or faster.
 * TASK_TIMER(work) counts those steps from 0 up to SCATTER_WAIT and round
 * again. BUTTERFLIES go up on each step it is 0, each to flutter about
 * FLUTTER_HEIGHT above the plant */
#define SCATTER_RANGE 30.0F
#define SCATTER_SPEED 0.7F
#define SCATTER_WAIT 10
#define BUTTERFLIES 2
#define FLUTTER_HEIGHT 10.0F
/* a butterfly starts off the plant by a random offset, from 0 up to
 * SCATTER_REACH. the same offset applies in x and z, and is its first y
 * speed */
#define SCATTER_REACH 3.0F

static void beach_bkusa_draw(struct task *task);
static void beach_mkusa_draw(struct task *task);
static void beach_kusa_sway(struct task *task);
static void beach_butterfly_step(struct task *task);
static void beach_kusa_touch(struct task *task);
static void beach_k2_draw(struct task *task);
static void beach_k2_scatter(struct task *task);

/* O MKUSA never tests for a touch, so it never enters KUSA_SWAY */
void beach_mkusa_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case KUSA_INIT:
        task->draw = beach_mkusa_draw;
        work->mode = KUSA_STILL;
        break;
    case KUSA_STILL:
        beach_mkusa_draw(task);
        /* @hack redundant: a test of placement_editing's result that does
         * nothing, which keeps the image's TST R0,R0 at 0x0c906b7e. as a
         * bare call, 31 of 54 words differ and the functions after it
         * move */
        if (!placement_editing(task)) {
        }
        break;
    case KUSA_SWAY:
        beach_kusa_sway(task);
        if (work->scl.y < 0.0F) {
            work->scl.y = 0.0F;
            work->mode = KUSA_STILL;
            PHASE(work) = 0.0F;
        }
        beach_mkusa_draw(task);
        break;
    case KUSA_STOP:
        break;
    }
}

/* outside the set editor, an O BKUSA whose scl.z is below OTHER_MODEL tests
 * for player 0's touch */
void beach_bkusa_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case KUSA_INIT:
        task->draw = beach_bkusa_draw;
        work->mode = KUSA_STILL;
        break;
    case KUSA_STILL:
        beach_bkusa_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- OTHER MODEL 5 over");
        }
        if (!placement_editing(task) && work->scl.z < OTHER_MODEL) {
            beach_kusa_touch(task);
        }
        break;
    case KUSA_SWAY:
        beach_kusa_sway(task);
        if (work->scl.y < 0.0F) {
            work->mode = KUSA_STILL;
            PHASE(work) = 0.0F;
        }
        beach_bkusa_draw(task);
        break;
    case KUSA_STOP:
        break;
    }
}

static void beach_bkusa_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->scl.z == 0.0F) {
        draw_tree(&beach_bkusa_a_object);
    } else if (work->scl.z > OTHER_MODEL) {
        draw_tree(&beach_bkusa_other_object);
    } else {
        draw_tree(&beach_bkusa_b_object);
    }
    njPopMatrix(1);
}

/* draw_tree draws each sibling after the node it is given. the leaves are
 * one sibling chain, from leaf 1 to leaf 8, so leaf n draws n times */
static void beach_mkusa_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a vector declared and never used, which keeps the frame
     * at 16 bytes (ADD #-16,R15 at 0x0c906cfe). without it, 2 of 83 words
     * differ */
    NJS_VECTOR unused;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&beach_mkusa_model);
    njPushMatrix(NULL);
    draw_tree(&beach_mkusa_leaf_1_object);
    draw_tree(&beach_mkusa_leaf_2_object);
    draw_tree(&beach_mkusa_leaf_3_object);
    draw_tree(&beach_mkusa_leaf_4_object);
    draw_tree(&beach_mkusa_leaf_5_object);
    draw_tree(&beach_mkusa_leaf_6_object);
    draw_tree(&beach_mkusa_leaf_7_object);
    draw_tree(&beach_mkusa_leaf_8_object);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void beach_kusa_sway(struct task *task)
{
    struct task_work *work = task->work;

    PHASE(work) += SWAY_SPEED;
    work->scl.y -= SWAY_DECAY;
    work->scl.x = work->scl.y * njSin((Angle)(PHASE(work) * ANGLE_PER_DEGREE));
}

/* a butterfly starts with random x and z speeds from -0.5 up to 0.5. its
 * angles and spins start random too, and its scale from 0.5 up to 1.5, in
 * 16ths. it flies and draws for BUTTERFLY_STEPS steps */
static void beach_butterfly_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    switch (work->mode) {
    case BUTTERFLY_INIT:
        TASK_TIMER(work) = BUTTERFLY_STEPS;
        work->scl.x = RANDOM(-0.5F, 0.5F);
        work->scl.z = RANDOM(-0.5F, 0.5F);
        work->ang.y = RANDOM_ANGLE();
        work->ang.x = RANDOM_ANGLE();
        BUTTERFLY(work)->spin_x = RANDOM(-0.5F, 0.5F) * SPIN;
        BUTTERFLY(work)->spin_y = RANDOM(-0.5F, 0.5F) * SPIN;
        work->tick = RANDOM(1.0F, 3.0F) * 16.0F;
        work->mode = BUTTERFLY_FLUTTER;
        break;
    case BUTTERFLY_FLUTTER:
        work->scl.x += PUSH * njSin(work->ang.y);
        work->scl.z += PUSH * njCos(work->ang.y);
        work->scl.y += PUSH * njSin(work->ang.x * 2);
        if (work->pos.y > BUTTERFLY(work)->height) {
            work->mode = BUTTERFLY_FALL;
        }
        break;
    case BUTTERFLY_FALL:
        if ((work->scl.y -= PUSH) < -FALL_MOST) {
            work->scl.y = -FALL_MOST;
        }
        if (work->pos.y < BUTTERFLY(work)->height) {
            work->mode = BUTTERFLY_FLUTTER;
        }
        break;
    }
    work->ang.x += BUTTERFLY(work)->spin_x;
    work->ang.y += BUTTERFLY(work)->spin_y;
    work->pos.x += work->scl.x * MOVE_XZ;
    work->pos.y += work->scl.y * MOVE_Y;
    work->pos.z += work->scl.z * MOVE_XZ;
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    /* halved as an integer, an odd tick loses its last 32nd */
    /* NOLINTNEXTLINE(bugprone-integer-division) */
    scale = (work->tick / 2) * (1.0F / 16.0F);
    njScale(NULL, scale, scale, scale);
    njEasyDrawObject(&beach_butterfly_object);
    njPopMatrix(1);
    /* a butterfly has no placement, so its range is 410 units. out of range,
     * it still counts down this step, and destroys itself on its next */
    placement_try_leave(task);
    if (--TASK_TIMER(work) == 0) {
        task_kill(task);
    }
}

/* at player 0's touch, start the sway. keep a unit vector in the task's
 * extra, where nothing reads it */
static void beach_kusa_touch(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR *way = task->extra;
    Float speed;
    NJS_VECTOR unit;

    if (njDistanceP2P(&player_work[0]->pos, &work->pos) < TOUCH_RANGE) {
        if (player_ground_speed(0, &speed) && speed >= TOUCH_SPEED) {
            /* @bug logic: the vector's y and z take player 0's y and z
             * from the plant's x, not from its y and z (FMOV.S @(R0,R13),FR2
             * with R0 32, pos.x, at 0x0c907168 and 0x0c907178) */
            way->x = work->pos.x - player_work[0]->pos.x;
            way->y = work->pos.x - player_work[0]->pos.y;
            way->z = work->pos.x - player_work[0]->pos.z;
            unit.x = way->x;
            unit.y = way->y;
            unit.z = way->z;
            njUnitVector(&unit);
            way->x = unit.x;
            way->y = unit.y;
            way->z = unit.z;
            work->scl.y = SWAY_AMPLITUDE;
            work->mode = KUSA_SWAY;
        }
    }
}

void beach_k2_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case KUSA_INIT:
        task->draw = beach_k2_draw;
        work->mode = KUSA_STILL;
        break;
    case KUSA_STILL:
        beach_k2_draw(task);
        if (!placement_editing(task)) {
            beach_k2_scatter(task);
        }
        break;
    case KUSA_STOP:
        break;
    }
}

static void beach_k2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&beach_k2_object);
    njPopMatrix(1);
}

static void beach_k2_scatter(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a vector declared and never used, which keeps the frame
     * at 16 bytes (ADD #-16,R15 at 0x0c907304). without it, 2 of 103 words
     * differ */
    NJS_VECTOR unused;
    Float speed;

    if (njDistanceP2P(&player_work[0]->pos, &work->pos) < SCATTER_RANGE) {
        if (player_ground_speed(0, &speed) && speed >= SCATTER_SPEED) {
            if (TASK_TIMER(work) == 0) {
                int i;

                for (i = BUTTERFLIES; i > 0; --i) {
                    struct task *made;

                    if ((made = task_create(TASK_WORK, 5,
                                            beach_butterfly_step)) != NULL) {
                        struct task_work *butterfly = made->work;
                        Float offset = rand() * RAND_SCALE;

                        butterfly->pos.x =
                            work->pos.x + (offset * SCATTER_REACH);
                        butterfly->pos.y = work->pos.y;
                        butterfly->pos.z =
                            work->pos.z + (offset * SCATTER_REACH);
                        BUTTERFLY(butterfly)->height =
                            work->pos.y + FLUTTER_HEIGHT;
                        butterfly->scl.y = njAbs(offset) * SCATTER_REACH;
                    }
                }
            }
            TASK_TIMER(work)++;
            if (TASK_TIMER(work) > SCATTER_WAIT) {
                TASK_TIMER(work) = 0;
            }
        }
    }
}
