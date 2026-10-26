/* O BZ, Tails beside a smoking wreck. player 0 clears the stage by touching
 * Tails */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/capsule.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "stg01/crash.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_ACTION it is, so SHC pools
 * its address and reads motion through it, MOV.L @(4,R2),R3 at 0x0c90de16;
 * declared as one, 19 of beach_crash_draw's 210 words differ */
extern NJS_ACTION beach_crash_tails_action[];
extern NJS_OBJECT beach_crash_wreck_object;

enum beach_crash_mode {
    CRASH_INIT = 0,
    CRASH_WAIT = 2, /* until player 0 touches Tails */
    CRASH_CAPSULE = 3, /* one step, which starts the capsule */
    CRASH_GONE = 4,
    CRASH_CLEARED = 5
};

struct beach_crash_vars {
    Float frame; /* Tails' frame in his motion */
};

#define VARS(work) ((struct beach_crash_vars *)&(work)->vars)

/* player 0 touches Tails within REACH times one more than the set entry's
 * scale x */
#define REACH 14.0F

/* Tails stands TAILS_RISE above the task's pos, turned TAILS_TURN more
 * about y than the task. until the touch, his motion plays TAILS_RATE frames
 * a step */
#define TAILS_RISE 5.0F
#define TAILS_TURN NJM_DEG_ANG(270)
#define TAILS_RATE 0.5F
/* the wreck stands WRECK_AHEAD along x from the task's pos, as ang.y turns
 * it */
#define WRECK_AHEAD 50.0F
/* the draw counts up TASK_TIMER. a draw that takes it past SMOKE_WAIT makes
 * a puff of smoke, and sets it back to 0 */
#define SMOKE_WAIT 10

/* the smoke that rises from the wreck. the draw sets pos for each puff */
static struct dust_params beach_crash_smoke = {
    2.0F,
    0.23F,
    0.17F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 2.0F, 0.8F},
    {1.0F, 0.0F, 0.0F, 0.0F},
};

static void beach_crash_draw(struct task *task);
static void beach_crash_draw_reach(struct task *task);
static void beach_crash_touch(struct task *task);

void beach_crash_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *made;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case CRASH_INIT:
        work->mode = CRASH_WAIT;
        break;
    case CRASH_WAIT:
        if (event_flag_check(FLAG_SONIC_CLEAR_BEACH)) {
            work->mode = CRASH_CAPSULE;
            break;
        }
        task->draw = beach_crash_draw;
        if (!placement_editing(task)) {
            beach_crash_touch(task);
        }
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- SCALE");
            beach_crash_draw_reach(task);
        }
        VARS(work)->frame += TAILS_RATE;
        beach_crash_draw(task);
        break;
    case CRASH_CAPSULE:
        made = task_create(TASK_WORK, 2, capsule_start);
        made->work->pos.x = work->pos.x;
        made->work->pos.y = work->pos.y;
        made->work->pos.z = work->pos.z;
        work->mode = CRASH_GONE;
        break;
    case CRASH_GONE:
        break;
    case CRASH_CLEARED:
        beach_crash_draw(task);
        break;
    }
}

/* Tails, then the wreck and its smoke */
static void beach_crash_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack redundant: a vector nothing reads, which SHC still copies from
     * the constants to the frame (0x0c90ddec to 0x0c90de02); without it, 201
     * of 210 words differ */
    NJS_VECTOR unused = {0.3F, 0.5F, 0.0F};
    /* @hack redundant: ahead's initializer, which the draw replaces before
     * it reads ahead, copied from the constants (0x0c90de04 to 0x0c90de12);
     * without it, 199 of 210 words differ */
    NJS_POINT3 ahead = {3.0F, 0.0F, 0.0F};
    Angle ang;

    if (VARS(work)->frame >= beach_crash_tails_action->motion->nbFrame - 1) {
        VARS(work)->frame = 0.0F;
    }
    TASK_TIMER(work)++;
    njSetTexture(&tails_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + TAILS_RISE, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y + TAILS_TURN, ang);
    draw_action(beach_crash_tails_action, VARS(work)->frame);
    njPopMatrix(1);
    ahead.x = WRECK_AHEAD;
    ahead.y = 0.0F;
    ahead.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &ahead, &ahead);
    njPopMatrix(1);
    if (TASK_TIMER(work) > SMOKE_WAIT) {
        beach_crash_smoke.pos.x = work->pos.x + ahead.x;
        beach_crash_smoke.pos.y = work->pos.y;
        beach_crash_smoke.pos.z = work->pos.z + ahead.z;
        TASK_TIMER(work) = 0;
        dust_create_from(&beach_crash_smoke);
    }
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + ahead.x, work->pos.y,
                work->pos.z + ahead.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&beach_crash_wreck_object);
    njPopMatrix(1);
}

/* in the set editor, draw marker_object scaled by one more than scale x, as
 * the touch's reach scales */
static void beach_crash_draw_reach(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: the scale in a vector's x, which SHC keeps on the frame
     * across the calls, FMOV.S FR2,@R15 at 0x0c90df98 and FMOV.S @R15,FR15 at
     * 0x0c90dfb0; as a Float, 40 of 41 words differ */
    NJS_VECTOR scale;

    scale.x = work->scl.x + 1.0F;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, scale.x, scale.x, scale.x);
    draw_tree(&marker_object);
    njPopMatrix(1);
}

static void beach_crash_touch(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, (work->scl.x + 1.0F) * REACH) == 1) {
        stage_clear();
        work->mode = CRASH_CLEARED;
    }
}
