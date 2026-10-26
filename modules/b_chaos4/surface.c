/* Chaos 4 surfaces. he comes up facing player 0, and a column of water
 * bursts up round him. until he is back under, his shapes hurt nothing and
 * their guard stops no attack, and players can aim at him. back under, his
 * shapes hurt again, and their guard stops every attack */
#include <ninja.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/column.h"
#include "b_chaos4/surface.h"
#include "b_chaos4/swamp.h"
#include "chaos/chaos.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "sound/sound.h"
#include "system/task.h"

enum surface_mode { SURFACE_RISE, SURFACE_BURST, SURFACE_UP, SURFACE_SINK };

#define DEPTH_Y 0.0F /* the depth he sinks back to */

/* the phases' countdowns: his rise before the column, his stay up after
 * it, and his sink. each phase ends on the step its count passes below 0 */
#define RISE_TIME 60
#define UP_TIME 377
#define SINK_TIME 60

/* his sound as he starts up, and again as he starts down */
#define SE_SURFACE 0x178

/* the attack level his shapes take once he is back under, and their guard
 * level, which no attack exceeds */
#define UNDER_ATTACK 1
#define UNDER_GUARD GUARD(3)

void chaos4_surface(struct task_work *work, struct task_motion *motion,
                    void *extra)
{
    struct chaos *chaos = extra;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    int i;

    switch (work->submode) {
    case SURFACE_RISE:
        chaos4_set_shapes(work, SHAPES_ON);
        chaos->target.y = SWAMP_Y;
        /* @hack spelling: the action set through extra, cast, not chaos, which
         * keeps a second copy of extra on the stack (MOV.L R6,@R15 at
         * 0x0c905c32) and stores the action through it (0x0c905c72). through
         * chaos, chaos4_surface runs 32 bytes short, and 169 of 172 words
         * differ */
        ((struct chaos *)extra)->motion.wanted = ACTION_RISE;
        CHAOS4_VARS(work)->timer = RISE_TIME;
        record->turn = TURN_PLAYER;
        sound_play(SE_SURFACE, 0, 0, 0);
        work->collision->flags |= COLLISION_TARGET;
        /* his shapes hurt nothing, and their guard stops no attack */
        for (i = 0; i < CHAOS4_SHAPES; i++) {
            work->collision->shapes[i].levels =
                work->collision->shapes[i].levels & ~SHAPE_ATTACK;
            work->collision->shapes[i].levels =
                work->collision->shapes[i].levels & ~SHAPE_GUARD;
        }
        work->submode = SURFACE_BURST;
    case SURFACE_BURST:
        if (--CHAOS4_VARS(work)->timer < 0) {
            swamp_ripple_add(&work->pos);
            column_create(&work->pos, NULL);
            record->turn = TURN_NONE;
            CHAOS4_VARS(work)->timer = UP_TIME;
            work->submode = SURFACE_UP;
        }
        break;
    case SURFACE_UP:
        if (--CHAOS4_VARS(work)->timer < 0) {
            chaos->target.y = DEPTH_Y;
            CHAOS4_VARS(work)->timer = SINK_TIME;
            sound_play(SE_SURFACE, 0, 0, 0);
            work->submode = SURFACE_SINK;
        }
        break;
    case SURFACE_SINK:
        if (--CHAOS4_VARS(work)->timer < 0) {
            work->collision->flags &= ~COLLISION_TARGET;
            for (i = 0; i < CHAOS4_SHAPES; i++) {
                work->collision->shapes[i].levels =
                    (work->collision->shapes[i].levels & ~SHAPE_ATTACK) |
                    UNDER_ATTACK;
                work->collision->shapes[i].levels =
                    (work->collision->shapes[i].levels & ~SHAPE_GUARD) |
                    UNDER_GUARD;
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
        work->submode = SURFACE_RISE;
    }
}
