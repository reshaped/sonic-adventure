/* O SHIP, a ship that cruises on, or plies to and fro, while player 0 is
 * near */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "minicart/shareobj_textures.h"
#include "minicart/ship.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT ship_object;

/* the ship's range for placement_try_leave_by, squared */
#define SHIP_RANGE (3010.0F * 3010.0F)

/* the ship's speed a step is its placement's scl.y plus SHIP_SPEED */
#define SHIP_SPEED 2.0F
/* the ship moves while player 0 is within its placement's scl.z. a scl.z of
 * 0 or less, or over SHIP_REACH, becomes SHIP_REACH */
#define SHIP_REACH 3000.0F
/* a cruising ship starts to turn once player 0 is within SHIP_TURN_NEAR of
 * it in x and z */
#define SHIP_TURN_NEAR 1000.0F
/* a turning ship turns SHIP_TURN_SPEED a step toward ACROSS, and holds its
 * course once within SHIP_ON_COURSE of it */
#define SHIP_TURN_SPEED 16
#define SHIP_ON_COURSE 256

/* the ang.y that sends the ship at right angles to the line dx, dz */
#define ACROSS(dx, dz) ((Angle)(atan2f((dx), (dz)) * ANGLE_PER_RADIAN))

/* ang.z, the side parts' rock, moves ROCK_STEP a step, and turns back once
 * past ROCK_LIMIT either way */
#define ROCK_LIMIT 4096
#define ROCK_STEP 128

/* BLINK is the texture of material BLINK_MATERIAL of the hull's first
 * child. every ship draws the one ship_object, so ship_move sets that
 * texture just for the ship's draw. it then puts back M32_TP1_003, the
 * model's own */
#define BLINK_MATERIAL 5
#define BLINK(work)                                                            \
    (VARS(work)->object->child->model->mats[BLINK_MATERIAL].attr_texId)
/* the blinking material's textures, by their names in OBJ_SHAREOBJ */
#define M32_TP1_003 44
#define M32_TP1_004 43
#define M32_TP1_005 48
/* ship_move counts steps in the work's timer, which picks the material's
 * texture:
 *
 *   under BLINK_STEPS        M32_TP1_003
 *   under 2 * BLINK_STEPS    M32_TP1_004
 *   under 3 * BLINK_STEPS    M32_TP1_005
 *   otherwise                M32_TP1_004
 *
 * the timer starts over at 0 once it passes 4 * BLINK_STEPS */
#define BLINK_STEPS 30

/* the ship scrolls the first SCROLL_UVS uvs of the hull's meshset
 * SCROLL_MESHSET in u. the work's tick grows SCROLL_STEP a step. ship_move
 * adds it to those uvs' u for its draw, and takes it off after */
#define SCROLL_MESHSET 6
#define SCROLL_UVS 4
#define SCROLL_STEP 8

#define SE_SHIP 89

struct ship_vars {
    NJS_OBJECT *object;
    /* the sum of scl.y over a plying ship's steps, the ones it stands still
     * on included. the ship moves only while player 0 is within scl.z */
    Float travel;
    Sint16 unk_08[2]; /* random values from -8192 to 8191, set at SHIP_INIT */
};

#define VARS(work) ((struct ship_vars *)&(work)->vars)

/* a ship whose placement's scl.x is above 0 plies to and fro, from
 * SHIP_OUT. it reverses once travel passes scl.x, and again once travel
 * falls below 0. any other ship cruises from SHIP_CRUISE, and turns once
 * player 0 comes near */
enum ship_mode {
    SHIP_INIT,
    SHIP_CRUISE,
    SHIP_TURN,
    SHIP_ON,
    SHIP_OUT,
    SHIP_BACK
};

enum ship_rock { ROCK_UP, ROCK_DOWN };

/* on a frame's first step, draw the hull, then its three children. the
 * first stays still, and the other two rock about x by ang.z, opposite
 * ways */
static void ship_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = VARS(work)->object;
    NJS_OBJECT *child;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&obj_shareobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(object->model);
        njPushMatrix(NULL);
        child = object->child;
        njTranslateV(NULL, (NJS_VECTOR *)child->pos);
        draw_model(child->model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        child = child->sibling;
        njTranslateV(NULL, (NJS_VECTOR *)child->pos);
        TURN_VIA(njRotateX, work->ang.z, ang);
        draw_model(child->model);
        njPopMatrix(1);
        njPushMatrix(NULL);
        child = child->sibling;
        njTranslateV(NULL, (NJS_VECTOR *)child->pos);
        TURN_VIA(njRotateX, -work->ang.z, ang);
        draw_model(child->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void ship_destroy(struct task *task)
{
    task_kill(task);
}

/* move the ship scl.y along ang.y while player 0 is within scl.z, and rock
 * the side parts. then draw with the blinking material's texture and the
 * hull's scrolled uvs. in Twinkle Park, the ship also plays SE_SHIP */
static void ship_move(struct task *task)
{
    struct task_work *work = task->work;
    Float speed;
    Uint16 i;

    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < work->scl.z) {
        speed = work->scl.y;
        work->pos.x -= speed * njCos(work->ang.y);
        work->pos.z += speed * njSin(work->ang.y);
    }
    switch (work->submode) {
    case ROCK_UP:
        if ((work->ang.z += ROCK_STEP) > ROCK_LIMIT) {
            work->submode = ROCK_DOWN;
        }
        break;
    case ROCK_DOWN:
        if ((work->ang.z -= ROCK_STEP) < -ROCK_LIMIT) {
            work->submode = ROCK_UP;
        }
        break;
    }
    ++work->timer;
    /* @hack spelling: a goto past the tests below after each of the first three
     * textures; as an else-if chain, the tests take other registers
     * (0x0c9167e2 to 0x0c91680c) and 17 of 239 words differ */
    if ((Uint16)work->timer < BLINK_STEPS) {
        BLINK(work) = M32_TP1_003;
        goto scroll;
    }
    if ((Uint16)work->timer < 2 * BLINK_STEPS) {
        BLINK(work) = M32_TP1_004;
        goto scroll;
    }
    if ((Uint16)work->timer < 3 * BLINK_STEPS) {
        BLINK(work) = M32_TP1_005;
        goto scroll;
    }
    if ((Uint16)work->timer > 4 * BLINK_STEPS) {
        work->timer = 0;
    }
    BLINK(work) = M32_TP1_004;
scroll:
    /* @bug logic: tick is a Uint8, so the test against 256 never holds
     * (EXTU.B and CMP/GE at 0x0c916834 in MINICART, 0x0c915174 in STG03).
     * tick wraps from 248 to 0 on its own, so the scroll starts over all
     * the same */
    /* NOLINTNEXTLINE(clang-diagnostic-tautological-*) */
    if ((work->tick += SCROLL_STEP) >= 256) {
        work->tick = 0;
    }
    for (i = 0; i < SCROLL_UVS; ++i) {
        VARS(work)->object->model->meshsets[SCROLL_MESHSET].vertuv[i].tex.u +=
            work->tick;
    }
    ship_draw(task);
    for (i = 0; i < SCROLL_UVS; ++i) {
        VARS(work)->object->model->meshsets[SCROLL_MESHSET].vertuv[i].tex.u -=
            work->tick;
    }
    BLINK(work) = M32_TP1_003;
    if (IN_STAGE(STAGE_TWINKLE)) {
        sound_play_at_for(SE_SHIP, (int)work, 1, 0, work->pos.x, work->pos.y,
                          work->pos.z, 2);
    }
}

void ship_step(struct task *task)
{
    struct task_work *work = task->work;
    Float dx;
    Float dz;

    if (placement_try_leave_by(task, SHIP_RANGE)) {
        return;
    }
    if (stage_get_stage_act() != STAGE_ACT(STAGE_TWINKLE, 0) &&
        !IN_STAGE(STAGE_CART)) {
        return;
    }
    if (edit_mode) {
        VARS(work)->object = &ship_object;
        ship_draw(task);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), ">");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), work->scl.x > 0.0F
                                                         ? "> ROUND DISTANCE"
                                                         : "> SIGHT SEEING");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> SPEED +");
            njPrintF(NJM_LOCATION(27, EDITOR_ROW_SCL_Y), SHIP_SPEED, 2);
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> START CUE");
            njPrintC(NJM_LOCATION(1, 24), "DISTANCE FROM PLAYER 1");
            njPrintF(NJM_LOCATION(1, 25),
                     njDistanceP2P(&work->pos, &player_work[0]->pos), 8);
        }
    } else {
        switch (work->mode) {
        case SHIP_INIT:
            if (work->scl.x > 0.0F) {
                work->mode = SHIP_OUT;
            } else {
                work->mode = SHIP_CRUISE;
            }
            work->scl.y += SHIP_SPEED;
            if (work->scl.z <= 0.0F || work->scl.z > SHIP_REACH) {
                work->scl.z = SHIP_REACH;
            }
            /* util.h's RAND_SCALE is exactly 1/32768, and 0.000030517578F
             * is just under it. times 16384, the literal folds to
             * 0x3effffff, just under a half (0x0c916bb0 in MINICART,
             * 0x0c9154f0 in STG03). RAND_SCALE would fold to 0.5 */
            VARS(work)->unk_08[0] =
                (int)(rand() * (0.000030517578F * 16384.0F)) - 8192;
            VARS(work)->unk_08[1] =
                (int)(rand() * (0.000030517578F * 16384.0F)) - 8192;
            VARS(work)->object = &ship_object;
            task->draw = ship_draw;
            task->destructor = ship_destroy;
            break;
        case SHIP_CRUISE:
            dx = work->pos.x - player_work[0]->pos.x;
            dz = work->pos.z - player_work[0]->pos.z;
            if (njSqrt((dx * dx) + (dz * dz)) < SHIP_TURN_NEAR) {
                work->mode = SHIP_TURN;
            }
            ship_move(task);
            break;
        case SHIP_TURN:
            dx = work->pos.x - player_work[0]->pos.x;
            dz = work->pos.z - player_work[0]->pos.z;
            if (angle_distance(work->ang.y, ACROSS(dx, dz)) < SHIP_ON_COURSE) {
                work->mode = SHIP_ON;
            } else {
                work->ang.y = angle_approach(work->ang.y, ACROSS(dx, dz),
                                             SHIP_TURN_SPEED);
            }
            ship_move(task);
            break;
        case SHIP_ON:
            ship_move(task);
            break;
        case SHIP_OUT:
            VARS(work)->travel += work->scl.y;
            if (VARS(work)->travel > work->scl.x) {
                work->mode = SHIP_BACK;
                work->scl.y *= -1.0F;
            }
            ship_move(task);
            break;
        case SHIP_BACK:
            if ((VARS(work)->travel += work->scl.y) < 0.0F) {
                work->mode = SHIP_OUT;
                work->scl.y *= -1.0F;
            }
            ship_move(task);
            break;
        }
    }
}
