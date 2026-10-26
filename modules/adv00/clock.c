/* O BCLOCK clock that starts at ten o'clock and advances one minute every 3601
 * steps */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/clock.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the face, its child the minute hand, and that child's sibling the hour
 * hand */
extern NJS_OBJECT clock_object;

/* the clock is destroyed once the camera and player 0 are both beyond 3100
 * units of it. CLOCK_RANGE is that distance squared */
#define CLOCK_RANGE 9610000.0F

/* the minute moves on as work->timer passes MINUTE_STEPS, every 3601 steps */
#define MINUTE_STEPS 3600
#define START_HOUR 10

/* the clock's work->mode: its first step starts it, and the rest run it */
enum clock_mode { CLOCK_START, CLOCK_RUN };

/* the minute's and the hour's indexes in clock_vars.time */
enum { CLOCK_MINUTE, CLOCK_HOUR };

struct clock_vars {
    NJS_OBJECT *object;
    /* @hack type: the minute and hour in an array, whose address SHC takes
     * (ADD #12,R13 at 0x0c900b1c) to read both through it in clock_draw. as
     * two Sint16 fields, 100 of the 230 words of clock_draw and clock_step
     * differ */
    Sint16 time[2];
};

#define VARS(work) ((struct clock_vars *)&(work)->vars)

/* each hand's angle from twelve */
#define MINUTE_ANG(work)                                                       \
    ((Angle)((VARS(work)->time[CLOCK_MINUTE] * 6) * ANGLE_PER_DEGREE))
#define HOUR_ANG(work)                                                         \
    ((Angle)((VARS(work)->time[CLOCK_HOUR] * 30) * ANGLE_PER_DEGREE) +         \
     (Angle)(VARS(work)->time[CLOCK_MINUTE] * (ANGLE_PER_DEGREE / 2.0F)))

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
    njTranslate(NULL, VARS(work)->object->pos[0], VARS(work)->object->pos[1],
                VARS(work)->object->pos[2]);
    texture_set_stage();
    draw_model(VARS(work)->object->model);
    njPushMatrix(NULL);
    hand = VARS(work)->object->child;
    njTranslate(NULL, hand->pos[0], hand->pos[1], hand->pos[2]);
    TURN_VIA(njRotateZ, 0x10000 - MINUTE_ANG(work), ang);
    draw_model(hand->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, hand->pos[0], hand->pos[1], hand->pos[2]);
    hand = VARS(work)->object->child->sibling;
    /* @hack redundant: the hour hand's angle tested a second time inside TURN
     * (TST R13,R13 at 0x0c900b58 and 0x0c900b5c). through TURN_VIA, 63 of
     * 146 words differ */
    (ang = 0x10000 - HOUR_ANG(work)) ? TURN(njRotateZ, ang) : (void)0;
    draw_model(hand->model);
    njPopMatrix(2);
}

static void clock_destroy(struct task *task)
{
    task_kill(task);
}

void clock_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, CLOCK_RANGE)) {
        return;
    }
    object = &clock_object;
    if (edit_mode) {
        if (!VARS(work)->object) {
            VARS(work)->object = object;
        }
        clock_draw(task);
    } else {
        switch (work->mode) {
        case CLOCK_START:
            work->mode = CLOCK_RUN;
            TASK_TIMER(work) = 0;
            VARS(work)->time[CLOCK_MINUTE] = 0;
            VARS(work)->time[CLOCK_HOUR] = START_HOUR;
            VARS(work)->object = object;
            task->destructor = clock_destroy;
            task->draw = clock_draw;
            break;
        case CLOCK_RUN:
            if (++TASK_TIMER(work) > MINUTE_STEPS) {
                /* @bug logic: the minute wraps past 60 and the hour past
                 * 12 (CMP/GT at 0x0c900c78 and 0x0c900c86). minute 60 points
                 * where minute 0 does, so the minute hand stands on twelve
                 * for two minutes an hour. hour 12 points where hour 0 does,
                 * so the hour hand sweeps from twelve to one twice */
                if (++VARS(work)->time[CLOCK_MINUTE] > 60) {
                    if (++VARS(work)->time[CLOCK_HOUR] > 12) {
                        VARS(work)->time[CLOCK_HOUR] = 0;
                    }
                    VARS(work)->time[CLOCK_MINUTE] = 0;
                }
                TASK_TIMER(work) = 0;
            }
            clock_draw(task);
            break;
        }
    }
}
