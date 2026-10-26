/* O C EMERALD, a Chaos Emerald that turns and bobs */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/chaos_emerald.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT chaos_emerald_red_object;
extern NJS_OBJECT chaos_emerald_yellow_object;
extern NJS_OBJECT chaos_emerald_green_object;
extern NJS_OBJECT chaos_emerald_sky_object;
extern NJS_OBJECT chaos_emerald_blue_object;
extern NJS_OBJECT chaos_emerald_purple_object;
extern NJS_OBJECT chaos_emerald_white_object;

#define EMERALDS 7

/* each step but the first turns the emerald TURN_SPEED about y and advances
 * its bob's phase BOB_SPEED */
#define TURN_SPEED 0x80
#define BOB_SPEED 0x100

enum emerald_mode { EMERALD_INIT, EMERALD_RUN };

/* the emerald's index in chaos_emeralds */
#define EMERALD(work) ((work)->submode)

struct emerald_vars {
    Uint8 unk_00[4];
    Sint16 phase; /* the bob's phase, whose sine is bob */
    Uint8 unk_06[2];
    Float bob; /* the emerald's height above pos.y, from -1 to 1 */
};

#define VARS(work) ((struct emerald_vars *)&(work)->vars)

static NJS_OBJECT *chaos_emeralds[EMERALDS] = {
    &chaos_emerald_red_object,   &chaos_emerald_yellow_object,
    &chaos_emerald_green_object, &chaos_emerald_sky_object,
    &chaos_emerald_blue_object,  &chaos_emerald_purple_object,
    &chaos_emerald_white_object,
};

static void past_chaos_emerald_draw(struct task *task);

void past_chaos_emerald_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case EMERALD_INIT:
        EMERALD(work) = work->ang.x;
        if (EMERALD(work) < 0 || EMERALD(work) >= EMERALDS) {
            EMERALD(work) = 0;
        }
        task->draw = past_chaos_emerald_draw;
        work->mode = EMERALD_RUN;
        break;
    case EMERALD_RUN:
        work->ang.y += TURN_SPEED;
        VARS(work)->phase += BOB_SPEED;
        VARS(work)->bob = njSin(VARS(work)->phase);
        break;
    }
    past_chaos_emerald_draw(task);
}

static void past_chaos_emerald_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->bob, work->pos.z);
    TURN(njRotateY, work->ang.y);
    draw_tree(chaos_emeralds[EMERALD(work)]);
    njPopMatrix(1);
}
