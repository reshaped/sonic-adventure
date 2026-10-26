/* O BATTEN, a board meant to show for E-102 around two events of his story.
 * its test is never TRUE, so it is never drawn */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/batten.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT batten_object;

/* in work->flags: the board is drawn. the step sets it while batten_shown
 * is TRUE */
#define BATTEN_SHOWN 0x200

#define EVENT_WRONG_ROOM 190 /* EV00BE: E-102 finds the wrong room */
/* EV00C5: E-102 suspects E-105 and E-101 are still aboard */
#define EVENT_E105_ABOARD 197

/* the board's kind, in work->submode, which the set entry's z angle gives.
 * a BATTEN_AFTER board is meant to show once EVENT_WRONG_ROOM has played,
 * and a BATTEN_UNTIL board until EVENT_E105_ABOARD has */
#define BATTEN_KIND(work) ((work)->submode)
enum batten_kind { BATTEN_AFTER, BATTEN_UNTIL };

enum batten_mode { BATTEN_INIT, BATTEN_RUN };

/* the step never calls collision_register, so the box takes part in no
 * collision checks */
static struct collision_shape batten_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 20.0F, 5.0F},
     16.0F,
     10.0F,
     2.0F,
     {0, 0, 0},
     0,
     NULL},
};

static void batten_draw(struct task *task);
static void batten_destroy(struct task *task);
static void batten_draw_work(struct task_work *work);
static Bool batten_shown(struct task_work *work);

void egg_carrier_batten_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BATTEN_INIT:
        BATTEN_KIND(work) = work->ang.z;
        work->ang.z = 0;
        collision_init(task, batten_shapes, COLLISION_OBJECT, 1, 1);
        task->destructor = batten_destroy;
        task->draw = batten_draw;
        work->mode = BATTEN_RUN;
        break;
    case BATTEN_RUN:
        if (batten_shown(work) == TRUE) {
            work->flags |= BATTEN_SHOWN;
        } else {
            work->flags &= ~BATTEN_SHOWN;
        }
        break;
    }
    if (work->flags & BATTEN_SHOWN) {
        batten_draw_work(work);
    }
}

static void batten_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (work->flags & BATTEN_SHOWN) {
        batten_draw_work(work);
    }
}

static void batten_destroy(struct task *task)
{
}

static void batten_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&batten_object);
    njPopMatrix(1);
}

/* meant to be TRUE while the nearest player is E-102 and the board's kind
 * allows it. it is always FALSE */
static Bool batten_shown(struct task_work *work)
{
    struct task_work *held;
    int player;

    /* @hack spelling: held, a copy of work in the stack frame's first slot
     * (MOV.L R4,@R15 at 0x0c9256e4), whose address the test below compares;
     * through work's own address, 23 of 44 words differ */
    held = work;
    player = player_nearest(&held->pos);
    /* @bug logic: the test compares held's address, R15, with
     * CHARACTER_E102 (MOV R15,R0 and CMP/EQ #6,R0 at 0x0c9256ec), not the
     * character player_get_character returns after it. no stack address is
     * CHARACTER_E102, so batten_shown returns FALSE and the board never
     * shows */
    if ((int)&held != CHARACTER_E102) {
        return FALSE;
    }
    player_get_character(player);
    switch (BATTEN_KIND(held)) {
    case BATTEN_AFTER:
        if (!event_was_played(EVENT_WRONG_ROOM)) {
            return FALSE;
        }
        break;
    case BATTEN_UNTIL:
        if (event_was_played(EVENT_E105_ABOARD) == TRUE) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}
