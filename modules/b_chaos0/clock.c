/* O BCLOCK clock that starts at ten o'clock and advances one minute every 361
 * steps */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos0/clock.h"
#include "collision/collision.h"
#include "draw/model.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the face, its child the minute hand, and that child's sibling the hour
 * hand */
extern NJS_OBJECT chaos0_clock_object;

/* the minute moves on as work->timer passes MINUTE_WAIT, every 361 steps */
#define MINUTE_WAIT 360
#define START_HOUR 10

/* a bit of work->tick, which the clock sets as it starts and clears as it
 * is destroyed */
#define CLOCK_LIVE 0x1

/* the clock's work->mode: its first step starts it, and the rest run it */
enum clock_mode { CLOCK_START, CLOCK_RUN };

/* the minute's and the hour's indexes in clock_vars.time */
enum { CLOCK_MINUTE, CLOCK_HOUR };

struct clock_vars {
    NJS_OBJECT *object; /* chaos0_clock_object, unless the work holds one */
    /* @hack type: the minute and hour in an array, whose address SHC takes
     * (ADD #12,R13 at 0x0c90516e) to read both through it in clock_draw. as
     * two Sint16 fields, 94 of the 223 words of clock_draw and
     * chaos0_clock_step differ */
    Sint16 time[2];
};

#define VARS(work) ((struct clock_vars *)&(work)->vars)

/* clang-format off */
static struct collision_shape clock_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {0.0F, 0.0F, 3.75F},
    17.5F, 17.5F, 3.75F, {0}, 0, NULL,
};
/* clang-format on */

static void clock_set_live(struct task_work *work)
{
    if (!(work->tick & CLOCK_LIVE)) {
        work->tick |= CLOCK_LIVE;
    }
}

/* draw the face, then each hand turned clockwise about the minute hand's
 * position */
static void clock_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *hand;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    njTranslate(NULL, VARS(work)->object->pos[0], VARS(work)->object->pos[1],
                VARS(work)->object->pos[2]);
    draw_model(VARS(work)->object->model);
    njPushMatrix(NULL);
    hand = VARS(work)->object->child;
    njTranslate(NULL, hand->pos[0], hand->pos[1], hand->pos[2]);
    TURN_VIA(njRotateZ,
             0x10000 - (Angle)((VARS(work)->time[CLOCK_MINUTE] * 6) *
                               ANGLE_PER_DEGREE),
             ang);
    draw_model(hand->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, hand->pos[0], hand->pos[1], hand->pos[2]);
    hand = VARS(work)->object->child->sibling;
    /* @hack redundant: the hour hand's angle tested a second time inside TURN
     * (TST R13,R13 at 0x0c9051aa and 0x0c9051ae). through TURN_VIA, 63 of
     * 146 words differ */
    (ang = 0x10000 -
           (Angle)((VARS(work)->time[CLOCK_HOUR] * 30) * ANGLE_PER_DEGREE) -
           (Angle)(VARS(work)->time[CLOCK_MINUTE] * (ANGLE_PER_DEGREE / 2.0F)))
        ? TURN(njRotateZ, ang)
        : (void)0;
    draw_model(hand->model);
    njPopMatrix(2);
}

static void clock_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (work->tick & CLOCK_LIVE) {
        work->tick &= ~CLOCK_LIVE;
    }
}

void chaos0_clock_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CLOCK_START:
        work->mode = CLOCK_RUN;
        work->timer = 0;
        VARS(work)->time[CLOCK_MINUTE] = 0;
        VARS(work)->time[CLOCK_HOUR] = START_HOUR;
        if (!VARS(work)->object) {
            VARS(work)->object = &chaos0_clock_object;
        }
        clock_set_live(work);
        collision_init(task, &clock_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = clock_destroy;
        task->draw = clock_draw;
        break;
    case CLOCK_RUN:
        if (++TASK_TIMER(work) > MINUTE_WAIT) {
            /* @bug logic: the minute wraps past 60 and the hour past 12
             * (CMP/GT at 0x0c905298 and 0x0c9052a6). minute 60 points where
             * minute 0 does, so the minute hand stands on twelve for two
             * minutes an hour. hour 12 points where hour 0 does, so the hour
             * hand sweeps from twelve to one twice */
            if (++VARS(work)->time[CLOCK_MINUTE] > 60) {
                if (++VARS(work)->time[CLOCK_HOUR] > 12) {
                    VARS(work)->time[CLOCK_HOUR] = 0;
                }
                VARS(work)->time[CLOCK_MINUTE] = 0;
            }
            work->timer = 0;
        }
        collision_register(work);
        clock_draw(task);
        break;
    }
}
