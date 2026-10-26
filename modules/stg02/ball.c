/* O BALL ball that bounces away once a player comes near */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg02/ball.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL windy_ball_model;

/* the ball's move a step, in work->vars */
struct ball_vars {
    Float speed_y;
    Float speed_x;
    Float speed_z;
};

struct ball_extra {
    /* the speed across is BOUNCE_SPEED plus this times the timer. it grows by
     * 0.0002 a landing */
    Float accel;
    Float bounces; /* the landings so far, plus 1 */
    Float roll; /* added to the timer to give the ball's turn, in degrees */
    Float growth; /* grows by 0.22 a landing, and nothing reads it */
};

#define VARS(work) ((struct ball_vars *)&(work)->vars)
#define EXTRA(task) ((struct ball_extra *)(task)->extra)

enum ball_mode {
    BALL_START = 0,
    BALL_WAIT = 1,
    BALL_BOUNCE = 3, /* the ball bounces until it lands after scl.x rebounds */
    BALL_FALL = 4, /* the ball falls until its tick passes FALL_STEPS */
    BALL_GONE = 5
};

/* the ball starts to bounce once player 0 or 1 is this near */
#define BALL_REACH 70.0F

#define FALL_STEPS 100

/* each bounce starts up at BOUNCE_SPEED times the sine of 45 degrees. its
 * upward speed then falls by BOUNCE_GRAVITY a step. the speed across starts at
 * BOUNCE_SPEED too */
#define BOUNCE_SPEED 0.21F
#define BOUNCE_GRAVITY 0.01F

static void windy_ball_draw(struct task *task);
static void windy_ball_bounce(struct task *task);

/* the ball moves along its heading only on the first step of its first
 * bounce. after that, windy_ball_bounce sets its x and z speeds alike, whatever
 * its heading */
void windy_ball_step(struct task *task)
{
    struct task_work *work = task->work;
    struct ball_extra *extra = EXTRA(task);
    NJS_VECTOR speed = {0.0F, 0.0F, 1.0F};

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BALL_START:
        work->mode = BALL_WAIT;
        task->draw = windy_ball_draw;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, work->ang.y);
        njCalcPoint(NULL, &speed, &speed);
        njPopMatrix(1);
        VARS(work)->speed_x = speed.x;
        VARS(work)->speed_y = speed.y;
        VARS(work)->speed_z = speed.z;
        work->scl.y = work->pos.y;
        extra->accel = 0.0012F;
        extra->bounces = 1.0F;
        extra->roll = 1.0F;
        break;
    case BALL_WAIT:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- NUM_CAISUU");
        }
        if (player_in_range(&work->pos, BALL_REACH)) {
            work->mode = BALL_BOUNCE;
        }
        windy_ball_draw(task);
        break;
    case BALL_BOUNCE:
        work->pos.x += VARS(work)->speed_x;
        work->pos.y += VARS(work)->speed_y;
        work->pos.z += VARS(work)->speed_z;
        windy_ball_bounce(task);
        windy_ball_draw(task);
        break;
    case BALL_FALL:
        work->pos.x += VARS(work)->speed_x;
        work->pos.y += VARS(work)->speed_y;
        work->pos.z += VARS(work)->speed_z;
        work->tick++;
        work->timer++;
        windy_ball_draw(task);
        if (work->tick > FALL_STEPS) {
            work->mode = BALL_GONE;
        }
        break;
    case BALL_GONE:
        task_kill(task);
        break;
    }
}

/* draw the ball turned about x by its timer plus the roll, in degrees */
static void windy_ball_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct ball_extra *extra = EXTRA(task);

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateX,
         (Angle)((TASK_TIMER(work) + extra->roll) * ANGLE_PER_DEGREE));
    draw_model(&windy_ball_model);
    njPopMatrix(1);
}

/* set the ball's speed for its next step. the ball rebounds each time it
 * falls below scl.y, the height it started at. at the landing after scl.x
 * rebounds, the ball falls away instead. work->tick counts the steps of this
 * bounce, and the timer those of every bounce */
static void windy_ball_bounce(struct task *task)
{
    struct task_work *work = task->work;
    struct ball_extra *extra = EXTRA(task);
    Float speed;

    TASK_TIMER(work)++;
    work->tick++;
    speed = (extra->accel * TASK_TIMER(work)) + BOUNCE_SPEED;
    VARS(work)->speed_y =
        (njSin(NJM_DEG_ANG(45)) * BOUNCE_SPEED) - (work->tick * BOUNCE_GRAVITY);
    VARS(work)->speed_x = speed;
    VARS(work)->speed_z = speed;
    if (work->scl.y > work->pos.y) {
        extra->accel += 0.0002F;
        /* @hack spelling: the sum spelled out, which loads 0.22F with MOVA
         * (0x0c90c2e2). with +=, SHC loads it through R1 and FPUL, the
         * function runs 2 bytes longer and 36 of 85 words differ */
        extra->growth = extra->growth + 0.22F;
        work->tick = 0;
        if (extra->bounces++ > work->scl.x) {
            work->mode = BALL_FALL;
            VARS(work)->speed_y = -(njSin(NJM_DEG_ANG(45)) * BOUNCE_SPEED);
            work->timer = 0;
        }
    }
}
