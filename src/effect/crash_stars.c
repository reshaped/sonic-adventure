/* crash stars: the stars that burst from a player as it crashes into a
 * wall */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "effect/crash_stars.h"
#include "object/models.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* how many stars burst, the steps each flies, and how far it flies a step */
#define STAR_COUNT 6
#define STAR_STEPS 10
#define STAR_SPEED 2.0F
/* the headings a star can take, evenly round, and the bound of its tilt
 * about x, 67.5 degrees */
#define STAR_HEADINGS 32.0F
#define STAR_TILT 12288.0F

enum crash_stars_mode { CRASH_STARS_START, CRASH_STARS_PLACED };
enum star_mode { STAR_START, STAR_FLY, STAR_END };

static void crash_stars_place(struct task_work *work);
static void crash_stars_burst(struct task_work *from);
static void crash_star_step(struct task *task);
static void crash_star_fly(struct task_work *work);

void crash_stars_step(struct task *task)
{
    struct task_work *work = task->work;

    crash_stars_place(work);
    crash_stars_burst(work);
    /* @hack redundant: the call in a loop of one pass, which keeps it a JSR
     * from a frame of its own (ADD #-4,R15 at 0x8c0473c4); alone, it becomes
     * a tail jump, the function runs 4 bytes short and 16 of 17 words differ */
    do {
        task_kill(task);
    } while (0);
}

static void crash_stars_place(struct task_work *work)
{
    struct task_work *player;
    NJS_POINT3 offset;
    NJS_POINT3 point;

    work->mode = CRASH_STARS_PLACED;
    player = player_work[CRASH_STARS_VARS(work)->player];
    if (player != NULL) {
        offset.x = offset.z = 0.0F;
        offset.y = 5.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, -player->ang.y);
        njCalcVector(NULL, &offset, &point);
        njPopMatrix(1);
        work->pos.x = player->pos.x + point.x;
        work->pos.y = player->pos.y + point.y;
        work->pos.z = player->pos.z + point.z;
    }
}

/* start STAR_COUNT stars at from's place. one random draw picks a star's
 * heading, its tilt and its color, yellow or orange */
static void crash_stars_burst(struct task_work *from)
{
    int i;
    struct task *task;
    struct task_work *work;
    Float roll;

    for (i = 0; i < STAR_COUNT; i++) {
        task = task_create(TASK_WORK, 5, crash_star_step);
        if (task == NULL) {
            break;
        }
        roll = rand() * RAND_SCALE;
        work = task->work;
        work->ang.y = (Uint32)(roll * STAR_HEADINGS) << 11;
        /* @hack spelling: ang.x taken from the field as it stands, in two
         * statements, which loads the loop's 0 into R13 ahead of its 6 in R10
         * (0x8c047500, 0x8c047504); in one expression, 15 of 102 words
         * differ */
        work->ang.x = (Uint32)(roll * STAR_TILT) & 0x3fff;
        work->ang.x = 0x10000 - work->ang.x;
        work->tick = (Uint8)(roll * 64.0F) & 1;
        work->pos = from->pos;
    }
}

static void crash_star_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STAR_START:
        work->mode = STAR_FLY;
        break;
    case STAR_FLY:
        crash_star_fly(work);
        break;
    case STAR_END:
        /* @hack redundant: the call in a loop of one pass, as in
         * crash_stars_step, which keeps it a JSR (STS.L PR,@-R15 at
         * 0x8c047602); alone, it becomes a tail jump, the function runs 6
         * bytes short and 24 of 25 words differ */
        do {
            task_kill(task);
        } while (0);
        break;
    }
}

static void crash_star_fly(struct task_work *work)
{
    NJS_VECTOR ahead;
    NJS_VECTOR move;

    if (TASK_TIMER(work)++ >= STAR_STEPS) {
        work->mode = STAR_END;
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateZ, work->ang.z);
    if (work->tick & 1) {
        njSimpleDrawObject(&star_yellow_object);
    } else {
        njSimpleDrawObject(&star_orange_object);
    }
    njPopMatrix(1);
    ahead.x = STAR_SPEED;
    ahead.z = 0.0F;
    ahead.y = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njCalcVector(NULL, &ahead, &move);
    njPopMatrix(1);
    /* @hack spelling: the sums spelled out, which read move.y from the frame
     * with FMOV.S @(R0,R15),FR3 (0x8c04771a to 0x8c04771e); with +=, SHC
     * forms the address first, the function runs 6 bytes long and 25 of 122
     * words differ */
    work->pos.x = work->pos.x + move.x;
    work->pos.y = work->pos.y + move.y;
    work->pos.z = work->pos.z + move.z;
    work->ang.z += NJM_DEG_ANG(22.5);
}
