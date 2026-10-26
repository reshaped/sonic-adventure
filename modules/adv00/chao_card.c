/* O CHAOCARD Chao Stadium membership card, which spins until a player picks it
 * up */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/chao_card.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "event/script.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 43
#include "adv00/chao_card/text.inc"

extern NJS_OBJECT chao_card_object;

/* player 0's held kind while carrying the card */
#define HELD_CHAO_CARD 33

#define CARD_SPIN 0x400 /* the card's turn a step while it lies */
/* a dropped card goes once it has lain more than this many steps */
#define CARD_WAIT 1800

enum chao_card_mode {
    CARD_PLACED = 0, /* lying where it was placed */
    CARD_DROPPED = 1, /* lying where it was dropped */
    CARD_HELD = 2,
    CARD_ALREADY_HELD = 3, /* player 0 held the card when the task started */
    CARD_GONE = 6
};

static void chao_card_draw(struct task *task);
static void chao_card_step(struct task *task);

/* clang-format off */
static struct collision_shape chao_card_shapes[] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 3.0F, 0.0F}, 5.5F, 3.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL, {0.0F, 3.0F, 0.0F},
     2.0F, 3.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static char *chao_card_found[] = {
    TEXT_1,
    NULL,
};

/* TRUE while player 0 is one of the six playable characters */
static Bool chao_card_available(struct task *task)
{
    switch (player_get_character(0)) {
    /* @hack spelling: a return for each case, so that the last test falls into
     * the return of TRUE and branches to FALSE (BF at 0x0c90f7de). grouped,
     * the cases take a BRA more and 14 of 23 words differ */
    case CHARACTER_SONIC:
        return TRUE;
    case CHARACTER_TAILS:
        return TRUE;
    case CHARACTER_KNUCKLES:
        return TRUE;
    case CHARACTER_AMY:
        return TRUE;
    case CHARACTER_E102:
        return TRUE;
    case CHARACTER_BIG:
        return TRUE;
    }
    return FALSE;
}

static void chao_card_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&chao_card_object);
    njPopMatrix(1);
}

static void chao_card_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        chao_card_draw(task);
    } else {
        chao_card_draw(task);
        switch (work->mode) {
        case CARD_PLACED:
        case CARD_DROPPED:
            carry_step_small(task);
            work->ang.y += CARD_SPIN;
            if (work->mode == CARD_DROPPED) {
                work->timer++;
                if (TASK_TIMER(work) > CARD_WAIT) {
                    task_kill(task);
                }
            }
            if (work->flags & CARRY_HELD) {
                player_set_held_kind(0, HELD_CHAO_CARD);
                work->mode = CARD_HELD;
                message_show_timed(chao_card_found);
            }
            break;
        case CARD_HELD:
            work->ang.y = -player_work[0]->ang.y;
            carry_step_small(task);
            if (!(work->flags & CARRY_HELD)) {
                player_set_held_kind(0, 0);
                work->timer = 0;
                work->mode = CARD_DROPPED;
            } else {
                placement_hold(task);
            }
            break;
        case CARD_ALREADY_HELD:
            break;
        case CARD_GONE:
            placement_kill(task);
            break;
        }
    }
}

void chao_card_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    work->ang.x = 0;
    work->ang.z = 0;
    work->timer = 0;
    if (!placement_editing(task) && !chao_card_available(task)) {
        placement_kill(task);
        return;
    }
    if (player_get_held_kind(0) == HELD_CHAO_CARD) {
        work->mode = CARD_ALREADY_HELD;
    } else {
        work->mode = CARD_PLACED;
    }
    if (!placement_editing(task)) {
        collision_init(task, chao_card_shapes, COLLISION_OBJECT, 1, 2);
    }
    carry_set_kind(work, motion, CARRY_KIND_SMALL);
    task->step = chao_card_step;
    task->draw = chao_card_draw;
}
