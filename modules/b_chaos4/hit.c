/* Chaos 4's hit. he bursts into drops that fall into the swamp, and
 * SCATTERED_STEPS steps on, he forms again from a blob as they fly back
 * into him. his last hit clears the stage */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/hit.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/textures.h"
#include "b_chaos4/util.h"
#include "chaos/chaos.h"
#include "chaos/emeralds.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "draw/state.h"
#include "draw/tree.h"
#include "effect/brain.h"
#include "enemy/enemy.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT chaos4_drop_object;

enum hit_mode { HIT_BURST, HIT_SCATTERED, HIT_FORMING, HIT_BEATEN };

/* the drops he bursts into, and the steps until he starts to form again.
 * the burst sets his speed's y to SINK_SPEED. his body comes back once the
 * returning drops have grown him to FORMED_SCALE */
#define BURST_DROPS 25
#define SCATTERED_STEPS 120
#define SINK_SPEED (-2.0F)
#define FORMED_SCALE 0.96F

/* the sound he bursts with, held until he has formed */
#define SE_BURST 0x16d

/* the attack level his shapes take once he has formed again */
#define FORMED_ATTACK 1

/* a drop's speed as it flies out, and how far out from him it starts */
#define DROP_SPEED 1.0F
#define DROP_REACH 10.0F
/* what gravity takes from a drop's speed's y a step */
#define DROP_FALL (9.8F / 360.0F)
/* a drop falls to DROP_BOTTOM and rests there. it flies back DROP_REST + 1
 * steps after its start */
#define DROP_BOTTOM (-30.0F)
#define DROP_REST 120.0F
/* the splashes a drop makes as it meets the surface, within SPLASH_SPREAD
 * of it in x and z */
#define DROP_SPLASHES 5
#define SPLASH_SPREAD 2.5F
/* the share of its gap to him a returning drop closes a step. the drop
 * ends within RETURN_NEAR of him, or once he has grown to FORMED_SCALE, and
 * grows him by RETURN_GROWTH as it ends */
#define RETURN_SHARE 0.01F
#define RETURN_NEAR 10.0F
#define RETURN_GROWTH 0.04F
/* a drop's wobble turns DROP_WOBBLE a step, and its scale wobbles by
 * DROP_SIZE_WOBBLE about DROP_SIZE */
#define DROP_WOBBLE NJM_DEG_ANG(15)
#define DROP_SIZE 2.0F
#define DROP_SIZE_WOBBLE 0.5F

/* a drop's record, at its task's extra */
struct burst_drop {
    NJS_VECTOR speed;
    Angle wobble;
    Bool in_swamp; /* TRUE once it is below the swamp's surface */
};

struct burst_drop_vars {
    Uint8 unk_00[4];
    /* the count of steps to its flight back, which starts once the count
     * passes below 0 */
    Sint32 rest;
};

#define DROP_VARS(work) ((struct burst_drop_vars *)&(work)->vars)

enum burst_drop_mode { DROP_START, DROP_FALLING, DROP_RETURN };

static void burst_drop_create(NJS_POINT3 *pos);
static void burst_drop_step(struct task *task);
static void burst_drop_draw(struct task *task);
static void burst_drop_destroy(struct task *task);

/* his step in BOSS_HIT. he loses a health and bursts into drops. his last
 * hit clears the stage, and any other leads back to BOSS_SWIM once he has
 * formed again */
void chaos4_hit(struct task_work *work, struct task_motion *motion, void *extra)
{
    struct chaos *chaos = extra;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    int i;

    switch (work->submode) {
    case HIT_BURST:
        chaos->health -= 1.0F;
        chaos->blink = 1;
        chaos4_set_shapes(work, SHAPES_OFF);
        /* @hack spelling: the action set through extra, cast, not chaos, which
         * keeps a second copy of extra on the stack (MOV.L R6,@R15 at
         * 0x0c9028f2) and stores the action through it (0x0c902930). through
         * chaos, 97 of 181 words differ */
        ((struct chaos *)extra)->motion.wanted = ACTION_RISE;
        chaos4_emeralds_create();
        brain_shown = 0;
        work->collision->flags &= ~COLLISION_TARGET;
        record->draw &= ~DRAW_BODY;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        for (i = 0; i < BURST_DROPS; i++) {
            burst_drop_create(&work->pos);
        }
        record->speed.y = SINK_SPEED;
        chaos->target.y = 0.0F;
        CHAOS4_VARS(work)->timer = SCATTERED_STEPS;
        if (chaos->health <= 0.0F) {
            swamp_fade(0);
            stage_clear();
            work->submode = HIT_BEATEN;
        } else {
            sound_play_held(SE_BURST, 0, 1, 0);
            work->submode = HIT_SCATTERED;
        }
        break;
    case HIT_SCATTERED:
        if (--CHAOS4_VARS(work)->timer < 0) {
            chaos_emerald_return_all();
            record->draw |= DRAW_BLOB;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            work->submode = HIT_FORMING;
        }
        break;
    case HIT_FORMING:
        if (work->scl.x >= FORMED_SCALE) {
            record->draw |= DRAW_BODY;
            record->draw &= ~DRAW_BLOB;
            sound_stop(SE_BURST);
            brain_shown = 1;
            for (i = 0; i < CHAOS4_SHAPES; i++) {
                work->collision->shapes[i].levels =
                    (work->collision->shapes[i].levels & ~SHAPE_ATTACK) |
                    FORMED_ATTACK;
            }
            work->submode = MODE_DONE;
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_SWIM) {
            boss_next_mode = BOSS_SWIM;
            boss_last_mode = work->mode;
        }
        work->submode = HIT_BURST;
    }
}

/* make a drop that flies out of him from pos. it makes none when it can't
 * allocate its task or its record */
static void burst_drop_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;
    struct burst_drop *drop;

    if ((task = task_create(TASK_WORK, 3, burst_drop_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        /* @hack spelling: the allocation tested in the statement that stores
         * it, which copies it to R12 in the slot of the test's branch (BF/S
         * at 0x0c902aca); stored and then tested, 3 of 70 words differ */
        if ((drop = heap_calloc(1, sizeof(struct burst_drop))) == NULL) {
            task_kill(task);
            return;
        }
        task->extra = drop;
        VECTOR_COPY(&work->pos, pos);
        task->step = burst_drop_step;
        task->draw = burst_drop_draw;
        task->destructor = burst_drop_destroy;
        drop->wobble = RANDOM_ANGLE();
        drop->in_swamp = FALSE;
    }
}

/* a drop flies out from him in a random direction, splashes into the
 * swamp and comes to rest at its bottom. DROP_REST + 1 steps after its
 * start, it flies back into him and grows him */
static void burst_drop_step(struct task *task)
{
    struct burst_drop *drop = task->extra;
    struct task_work *work = task->work;
    Float pitch;
    Float yaw;
    Float distance;
    NJS_POINT3 pos;
    int i;

    drop->wobble += DROP_WOBBLE;
    switch (work->mode) {
    case DROP_START:
        pitch = RANDOM_ANGLE();
        yaw = RANDOM_ANGLE();
        drop->speed.y = njSin((Angle)pitch) * DROP_SPEED;
        drop->speed.x = njCos((Angle)pitch) * njSin((Angle)yaw) * DROP_SPEED;
        drop->speed.z = njCos((Angle)pitch) * njCos((Angle)yaw) * DROP_SPEED;
        work->pos.y += njSin((Angle)pitch) * DROP_REACH;
        work->pos.x += njCos((Angle)pitch) * njSin((Angle)yaw) * DROP_REACH;
        work->pos.z += njCos((Angle)pitch) * njCos((Angle)yaw) * DROP_REACH;
        work->scl.x = work->scl.y = work->scl.z = 1.0F;
        /* rand() is scaled by 0.0F, so every drop's count starts at
         * DROP_REST */
        DROP_VARS(work)->rest = DROP_REST - (rand() * 0.0F);
        drop->in_swamp = work->pos.y < SWAMP_Y;
        work->mode = DROP_FALLING;
    case DROP_FALLING:
        if (!drop->in_swamp && work->pos.y <= SWAMP_Y) {
            drop->in_swamp = TRUE;
            sound_play(SE_SPLASH, 0, 0, 0);
            for (i = 0; i < DROP_SPLASHES; i++) {
                pos = work->pos;
                pos.x += RANDOM(-SPLASH_SPREAD, SPLASH_SPREAD);
                pos.z += RANDOM(-SPLASH_SPREAD, SPLASH_SPREAD);
                chaos4_splash_create(&pos);
            }
        }
        VECTOR_SUM(&work->pos, &work->pos, &drop->speed);
        drop->speed.y -= DROP_FALL;
        if (work->pos.y < DROP_BOTTOM) {
            drop->speed.x = drop->speed.y = drop->speed.z = 0.0F;
            work->pos.y = DROP_BOTTOM;
        }
        if (--DROP_VARS(work)->rest < 0) {
            work->mode = DROP_RETURN;
        }
        break;
    case DROP_RETURN:
        VECTOR_DIFF(&drop->speed, chaos4_pos, &work->pos);
        distance = njSqrt(VECTOR_SQUARED(&drop->speed));
        VECTOR_SCALE(&drop->speed, &drop->speed, RETURN_SHARE);
        VECTOR_SUM(&work->pos, &work->pos, &drop->speed);
        if (distance <= RETURN_NEAR || chaos4_scale->x >= FORMED_SCALE) {
            chaos4_scale->x = chaos4_scale->y = chaos4_scale->z +=
                RETURN_GROWTH;
            task_kill(task);
        }
        break;
    }
    burst_drop_draw(task);
}

/* on a frame's first step, draw the drop at half alpha, its scale
 * wobbling */
static void burst_drop_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct burst_drop *drop = task->extra;

    if (frame_step == 0) {
        njSetTexture(&chaos4_common_texlist);
        draw_state_save();
        draw_translucent(0.5F);
        work->scl.x = work->scl.z =
            DROP_SIZE + (DROP_SIZE_WOBBLE * njSin(drop->wobble));
        work->scl.y = DROP_SIZE + (DROP_SIZE_WOBBLE * njCos(drop->wobble));
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        draw_tree(&chaos4_drop_object);
        njPopMatrix(1);
        draw_state_restore();
    }
}

static void burst_drop_destroy(struct task *task)
{
}
