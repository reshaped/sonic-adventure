/* the Egg Hornet's wreck. it smokes from when its health runs out. once its
 * last attack is flown, it shudders and bursts apart. the Egg Mobile left
 * bobs where it hung */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm1/debris.h"
#include "b_egm1/egg_hornet.h"
#include "b_egm1/hornet_textures.h"
#include "b_egm1/texlists.h"
#include "b_egm1/wreck.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* the smoke's record, which its task's extra points to */
struct smoke {
    NJS_POINT3 *pos;
    Uint32 timer; /* the steps since the last puff */
    Sint32 mode; /* an enum smoke_mode */
};

/* SMOKE_WAIT steps pass between puffs. each puff appears at random up to
 * SMOKE_SPREAD from *pos on each axis */
#define SMOKE_WAIT 2
#define SMOKE_SPREAD 19.0F

/* wreck_trail's puffs of smoke appear at random up to TRAIL_SPREAD from
 * the piece on each axis */
#define TRAIL_SPREAD 4.0F

/* each step, wreck_bob turns the bob's phase BOB_TURN, a 120th of a cycle.
 * the boss moves up to BOB_HEIGHT above and below hover_y */
#define BOB_TURN (0x10000 / 120)
#define BOB_HEIGHT 10.0F

/* record->count counts the shudders down from SHUDDERS + 1. a step that
 * finds the boss's motion playing entry 0 takes one off. the step then asks
 * for ACTION_SHUDDER while the count stays above 0, and sets WRECK_BURST
 * at 0 */
#define SHUDDERS 2
/* record->timer counts WAIT_TIME down on the steps other entries play. once
 * it runs out, the first step that finds player 0 on the ground sets it to
 * BURST_DELAY. the state turns WRECK_BURST as it runs out again */
#define WAIT_TIME 600
#define BURST_DELAY 180

/* the burst's pieces fly out at BURST_SPEED. each spins up to half of
 * BURST_SPIN a step about each axis */
#define BURST_SPIN NJM_DEG_ANG(8)
#define BURST_SPEED 1.0F
/* each piece's delay after the last, which the debris keeps and never waits
 * out */
#define PIECE_DELAY 12
/* WRECK_DONE comes BOB_TIME steps after the burst's step. record->count
 * counts the bob's steps, the burst's step first */
#define BOB_TIME 180

#define SE_BURST 464

/* the smoke wreck_smoke_create made last. it stays set once the smoke ends */
static struct task *smoke_task;

/* move the boss a step along its bob about hover_y */
static void wreck_bob(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;

    record->bob += BOB_TURN;
    record->bob &= 0xffff;
    work->pos.y = record->hover_y - (njSin(record->bob) * BOB_HEIGHT);
}

static void wreck_smoke_step(struct task *task)
{
    struct smoke *smoke = task->extra;
    NJS_POINT3 pos;

    switch (smoke->mode) {
    case SMOKE_ON:
        if (++smoke->timer > SMOKE_WAIT) {
            pos.x = smoke->pos->x +
                    (rand() * (2.0F * SMOKE_SPREAD * RAND_SCALE)) -
                    SMOKE_SPREAD;
            pos.y = smoke->pos->y +
                    (rand() * (2.0F * SMOKE_SPREAD * RAND_SCALE)) -
                    SMOKE_SPREAD;
            pos.z = smoke->pos->z +
                    (rand() * (2.0F * SMOKE_SPREAD * RAND_SCALE)) -
                    SMOKE_SPREAD;
            smoke->timer = 0;
            bomb_create(&pos, 1.0F);
        }
        break;
    case SMOKE_OFF:
        task_kill(task);
        break;
    }
}

void wreck_smoke_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct smoke *smoke;

    if ((smoke_task = task = task_create(0, 3, wreck_smoke_step)) != NULL) {
        smoke = heap_calloc(1, sizeof(struct smoke));
        task->extra = smoke;
        smoke->pos = pos;
        smoke->timer = 0;
        smoke->mode = SMOKE_ON;
        task->step = wreck_smoke_step;
    }
}

void wreck_smoke_set_mode(enum smoke_mode mode)
{
    struct smoke *smoke = smoke_task->extra;

    smoke->mode = mode;
}

/* a burst piece's trail. steps is the debris's count of its steps, from 1.
 * on each odd step, it makes a still puff of dust at pos. on every eighth,
 * it makes a puff of smoke near pos */
static void wreck_trail(NJS_POINT3 *pos, int steps)
{
    NJS_POINT3 point;

    if (steps & 1) {
        point.x = 0.0F;
        point.y = 0.0F;
        point.z = 0.0F;
        dust_create(pos, &point, 0.7F);
    }
    if (!(steps & 7)) {
        point.x = (rand() * (2.0F * TRAIL_SPREAD * RAND_SCALE)) + pos->x -
                  TRAIL_SPREAD;
        point.y = (rand() * (2.0F * TRAIL_SPREAD * RAND_SCALE)) + pos->y -
                  TRAIL_SPREAD;
        point.z = (rand() * (2.0F * TRAIL_SPREAD * RAND_SCALE)) + pos->z -
                  TRAIL_SPREAD;
        bomb_create(&point, 0.3F);
    }
}

void wreck_reset(struct task *task)
{
    struct egg_hornet *record = task->extra;

    /* @hack redundant: record's address taken and dropped, which keeps record
     * in a 4-byte frame (MOV.L R3,@R15 at 0x0c9010fa); without it, the
     * object runs 32 bytes short and 8 of 8 words differ */
    (void)&record;
    record->state = WRECK_START;
}

void wreck_step(struct task *task, struct task_work *work)
{
    struct egg_hornet *record = task->extra;
    struct debris_config config;

    switch (record->state) {
    case WRECK_START:
        record->count = SHUDDERS + 1;
        record->state = WRECK_SHUDDER;
        record->timer = WAIT_TIME;
        /* the shudders start on this step */
    case WRECK_SHUDDER:
        if (record->motion.current == 0) {
            if (--record->count <= 0) {
                record->state = WRECK_BURST;
            } else {
                record->motion.wanted = ACTION_SHUDDER;
            }
        } else if (--record->timer <= 0) {
            /* @hack type: the test narrowed to a Sint16, which makes it TST
             * #3,R0 (0x0c9011de); without it, SHC tests against a 3 it keeps
             * in R10, and 37 of 149 words differ */
            if ((Sint16)(player_get(0)->last_flags & PLAYER_ON_GROUND)) {
                record->timer = BURST_DELAY;
                record->state = WRECK_DELAY;
            }
        }
        break;
    case WRECK_DELAY:
        if (--record->timer <= 0) {
            record->state = WRECK_BURST;
        }
        break;
    case WRECK_BURST:
        config.texlist = &egg_hornet_texlist;
        config.object = &egg_hornet_object;
        config.pos = &work->pos;
        config.ang = &work->ang;
        config.spin = BURST_SPIN;
        config.speed = BURST_SPEED;
        config.delay = PIECE_DELAY;
        config.trail = wreck_trail;
        egg_hornet_debris_create(&config);
        sound_play(SE_BURST, 0, 0, 0);
        wreck_smoke_set_mode(SMOKE_OFF);
        record->burst = TRUE;
        record->count = 0;
        record->bob = 0;
        record->hover_y = work->pos.y;
        record->state = WRECK_BOB;
        /* the bob starts on this step */
    case WRECK_BOB:
        wreck_bob(task);
        if (++record->count > BOB_TIME) {
            record->state = WRECK_DONE;
        }
        break;
    case WRECK_DONE:
        wreck_bob(task);
        break;
    }
}
