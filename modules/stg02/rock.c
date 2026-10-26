/* O_VCROK: a rock that shakes once a player comes near, then flies at a
 * mark and breaks */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/helpers.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg02/bridge.h"
#include "stg02/rock.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL windy_rock_model;
extern NJS_MODEL windy_rock_model_1;
extern NJS_OBJECT windy_rock_piece_0_object;
extern NJS_OBJECT windy_rock_piece_1_object;
extern NJS_OBJECT windy_rock_piece_2_object;
extern NJS_OBJECT windy_rock_piece_3_object;
extern NJS_OBJECT windy_rock_piece_4_object;

/* the rock destroys itself once the camera and player 0 are farther than
 * this */
#define ROCK_RANGE 1000.0F

#define ROCK_REACH 100.0F /* a player this near sets the rock shaking */
#define SHAKE_STEPS 80 /* it flies after shaking for more steps than this */
#define BREAK_REACH 150.0F /* it breaks this near the point it aims at */

/* the shaking rock swings about x, SHAKE_SWING angle units each way. its
 * swing advances SHAKE_RATE degrees of a sine each step */
#define SHAKE_SWING 180.0F
#define SHAKE_RATE 45

#define TUMBLE 255 /* the flying rock's turn about x and y each step */

/* the flying rock aims at scl.y below its own height. its first step sets
 * scl.y to rand() times AIM_DROP over 65536: from 0 to under 150 */
#define AIM_DROP 300.0F
/* each step, the flying rock moves twice FLY_SPEED toward its mark. it moves
 * twice FLY_GAIN more for each step it has flown. it then rises FLY_LIFT */
#define FLY_SPEED 0.2F
#define FLY_GAIN 0.012F
#define FLY_LIFT 1.0F

#define PIECES 5
#define BROKEN_STEPS 300.0F /* the steps a broken rock runs its pieces */

enum rock_mode {
    ROCK_START,
    ROCK_WAIT,
    ROCK_SHAKE,
    ROCK_FLY,
    ROCK_BROKEN,
    ROCK_DONE
};

static NJS_OBJECT *windy_rock_pieces[] = {
    &windy_rock_piece_0_object, &windy_rock_piece_1_object,
    &windy_rock_piece_2_object, &windy_rock_piece_3_object,
    &windy_rock_piece_4_object, NULL,
};

static void windy_rock_draw(struct task *task);

/* a player within ROCK_REACH sets the rock shaking. after more than
 * SHAKE_STEPS steps of it, the rock flies at windy_rock_mark, tumbling. it
 * breaks into PIECES pieces within BREAK_REACH of the point it aims at. a
 * broken rock runs its pieces for BROKEN_STEPS steps, then destroys them and
 * itself. a rock whose scl.x isn't 0 destroys itself on the step after it
 * breaks instead. either way, its placement makes no other rock */
void windy_rock_step(struct task *task)
{
    static const NJS_VECTOR windy_rock_dust_speed = {0.0F, 0.0F, 0.0F};
    /* only x and z count: the rock aims at the height scl.y below its own */
    static const NJS_POINT3 windy_rock_mark = {3859.0F, -400.0F, -1965.0F};
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    NJS_POINT3 mark;
    Float distance;
    Float x;
    Float y;
    Float z;
    Angle ang;
    struct task_work *piece;
    struct task *child;
    NJS_OBJECT *node;
    int i;

    speed = windy_rock_dust_speed;
    mark = windy_rock_mark;
    if (placement_try_leave_by(task, SQUARE(ROCK_RANGE))) {
        return;
    }
    switch (work->mode) {
    case ROCK_START:
        work->mode = ROCK_WAIT;
        work->scl.y = _builtin_fabsf(rand() * (AIM_DROP / 65536.0F));
        break;
    case ROCK_WAIT:
        windy_rock_draw(task);
        if (player_in_range(&work->pos, ROCK_REACH)) {
            work->mode = ROCK_SHAKE;
        }
        break;
    case ROCK_SHAKE:
        windy_rock_draw(task);
        TASK_TIMER(work)++;
        work->ang.x =
            njSin((Angle)(ANGLE_PER_DEGREE * (SHAKE_RATE * TASK_TIMER(work)))) *
            SHAKE_SWING;
        if (TASK_TIMER(work) > SHAKE_STEPS) {
            work->mode = ROCK_FLY;
            dust_create(&work->pos, &speed, 10.0F);
            work->timer = 0;
        }
        break;
    case ROCK_FLY:
        TASK_TIMER(work)++;
        work->ang.x += TUMBLE;
        work->ang.y += TUMBLE;
        distance = FLY_SPEED + (TASK_TIMER(work) * FLY_GAIN);
        mark.y = work->pos.y - work->scl.y;
        point_toward(&work->pos, &mark, distance * 2.0F, &work->pos);
        work->pos.y += FLY_LIFT;
        if (njDistanceP2P(&work->pos, &mark) < BREAK_REACH) {
            /* @bug uninitialized: x is first set at 0x0c906054, inside this
             * test. so the FCMP/GT at 0x0c906038 compares 320 with FR15 as
             * the task's caller left it: the 1900.0 limit stage_run_action
             * keeps there (FMOV.S at 0x8c051b1a). the test never holds, and
             * the rock never breaks */
            if (x < 320.0F) { /* NOLINT(clang-diagnostic-uninitialized) */
                if (work->scl.x != 0.0F) {
                    placement_kill(task);
                }
                x = work->pos.x;
                y = work->pos.y;
                z = work->pos.z;
                ang = work->ang.y;
                for (i = 0; i < PIECES; i++) {
                    child = task_create_child(TASK_WORK | TASK_MOTION,
                                              windy_bridge_piece_start, task);
                    piece = child->work;
                    node = windy_rock_pieces[i];
                    PIECE(piece)->node = node;
                    piece->ang.y = ang;
                    piece->pos.x = x + node->pos[0];
                    piece->pos.y = y + node->pos[1];
                    piece->pos.z = z + node->pos[2];
                    PIECE(piece)->thrown[0] = FALSE;
                }
                work->mode = ROCK_BROKEN;
                work->timer = 0;
            }
        }
        windy_rock_draw(task);
        break;
    case ROCK_BROKEN:
        if (TASK_TIMER(work)++ < BROKEN_STEPS) {
            task_step_children(task);
        } else {
            work->mode = ROCK_DONE;
        }
        break;
    case ROCK_DONE:
        task_destroy_children(task);
        placement_kill(task);
        break;
    }
}

/* scl.x picks the model, 0 or 1. any other scl.x draws nothing */
static void windy_rock_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch ((int)work->scl.x) {
    case 0:
        draw_model(&windy_rock_model);
        break;
    case 1:
        draw_model(&windy_rock_model_1);
    }
    njPopMatrix(1);
}
