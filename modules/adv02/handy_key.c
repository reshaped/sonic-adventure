/* the key the object list names O HANDY KEY: a stone or statue a player
 * carries to the lock of its kind */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/handy_key.h"
#include "adv02/lock.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "event/script.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 63
#define TEXT_2_LENGTH 51
#define TEXT_3_LENGTH 30
#define TEXT_4_LENGTH 30
#define TEXT_5_LENGTH 28
#define TEXT_6_LENGTH 38
#include "adv02/handy_key/text.inc"

extern NJS_OBJECT ice_stone_object;
extern NJS_ACTION ice_stone_action;
extern NJS_OBJECT windy_stone_object;
extern NJS_ACTION windy_stone_action;
extern NJS_OBJECT gold_statue_object;
extern NJS_OBJECT silver_statue_object;

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 520, unless a player holds it */
#define RANGE (520.0F * 520.0F)

#define FRAME_SPEED 0.2 /* the action frames the key advances each step */

/* a key let go within this squared distance of the nearest live lock is tried
 * in it */
#define LOCK_REACH (40.0F * 40.0F)
/* a squared distance beyond every lock, where the search for the nearest
 * starts */
#define FAR 100000000.0F
/* a loose key, neither held nor in its lock, destroys itself once it has lain
 * this many steps. work->timer counts them */
#define LOOSE_STEPS 1800

#define RADIUS 50.0F /* the collision record's radius */

/* event flags set while a player holds each stone */
#define FLAG_HOLDING_WINDY_STONE 48
#define FLAG_HOLDING_ICE_STONE 49

/* a key that handy_key_init doesn't start stays in HANDY_KEY_IDLE, where its
 * step neither draws nor updates it */
enum handy_key_mode { HANDY_KEY_INIT, HANDY_KEY_RUN, HANDY_KEY_IDLE };

/* the key's kind, in work->tick */
#define KIND(work) ((work)->tick)

struct handy_key_vars {
    Uint8 unk_00[4];
    Sint8 held; /* TRUE if a player held the key at the last step */
    Uint8 unk_05[3];
    Float frame;
};

#define VARS(work) ((struct handy_key_vars *)&(work)->vars)

struct key_kind_info {
    Sint32 unk_00;
    NJS_OBJECT *object;
    NJS_ACTION *action; /* the key's action, or null to draw object still */
    Sint32 held_kind; /* player 0's held kind while a player carries the key */
    char *lines[3]; /* what a pickup in the Mystic Ruins shows */
    /* what a pickup outside the Mystic Ruins shows. every kind's
     * lines_elsewhere holds no line */
    char *lines_elsewhere[3];
};

static void handy_key_init(struct task *task);
static void handy_key_reset(struct task *task);
static void handy_key_update(struct task *task);
static void handy_key_draw(struct task *task);
static void handy_key_destroy(struct task *task);

static struct key_kind_info key_kinds[KEY_KINDS];

void handy_key_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!(work->flags & CARRY_HELD) && placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (player_get_task(0)->step == NULL) {
        task->step = NULL;
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case HANDY_KEY_INIT:
            handy_key_init(task);
            break;
        case HANDY_KEY_RUN:
            if (!event_in_control()) {
                handy_key_draw(task);
                handy_key_update(task);
            }
            break;
        }
    } else {
        switch (work->mode) {
        case HANDY_KEY_INIT:
            handy_key_init(task);
            break;
        case HANDY_KEY_RUN:
            handy_key_reset(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> KIND(0-%d)",
                        KEY_KINDS - 1);
            if (key_slots[KIND(work)].key != NULL) {
                debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y),
                            "DUPLICATED BY ANOTHER!");
            } else {
                handy_key_draw(task);
            }
            break;
        }
    }
}

static struct key_kind_info key_kinds[KEY_KINDS] = {
    {0,
     &ice_stone_object,
     &ice_stone_action,
     HELD_ICE_STONE,
     {TEXT_1, NULL, NULL},
     {NULL, NULL, NULL}},
    {0,
     &windy_stone_object,
     &windy_stone_action,
     HELD_WINDY_STONE,
     {TEXT_2, NULL, NULL},
     {NULL, NULL, NULL}},
    {0,
     &gold_statue_object,
     NULL,
     HELD_GOLD_STATUE,
     {TEXT_3, NULL, NULL},
     {NULL, NULL, NULL}},
    {0,
     &silver_statue_object,
     NULL,
     HELD_SILVER_STATUE,
     {TEXT_4, NULL, NULL},
     {NULL, NULL, NULL}},
};

/* a zero word between the kinds and the shapes, which no code reads */
static int dat_0cb209c0 = 0;

static struct collision_shape handy_key_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 4.0F, 0.0F},
     3.6F,
     6.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {0.0F, 2.0F, 0.0F},
     0.1F,
     2.5F,
     0.0F,
     {0},
     0,
     NULL},
};

struct key_slot key_slots[KEY_KINDS] = {
    {NULL, FALSE},
    {NULL, FALSE},
    {NULL, FALSE},
    {NULL, FALSE},
};

static char *key_fits_lines[2] = {
    TEXT_5,
    NULL,
};
static char *key_wrong_lines[2] = {
    TEXT_6,
    NULL,
};

/* a key runs only while no other key of its kind is live, and only where the
 * character has a lock of its kind */
static void handy_key_init(struct task *task)
{
    struct task_work *work = task->work;

    work->mode = HANDY_KEY_IDLE;
    handy_key_reset(task);
    if (key_slots[KIND(work)].key != NULL) {
        return;
    }
    if (lock_flags[player_get_character(0)][KIND(work)] < 0) {
        return;
    }
    if (lock_flag(KIND(work)) && work->scl.y < IN_LOCK_SCL_Y) {
        return;
    }
    work->mode = HANDY_KEY_RUN;
    collision_init(task, handy_key_shapes, COLLISION_OBJECT, 1, 2);
    work->collision->radius = RADIUS;
    carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
    VARS(work)->frame = 0.0F;
    task->destructor = handy_key_destroy;
    if (!placement_editing(task)) {
        if (lock_flag(KIND(work))) {
            key_slots[KIND(work)].in_lock = TRUE;
        } else {
            key_slots[KIND(work)].in_lock = FALSE;
        }
        key_slots[KIND(work)].key = task;
        task->draw = handy_key_draw;
    }
}

static void handy_key_reset(struct task *task)
{
    struct task_work *work = task->work;

    work->timer = 0;
    VARS(work)->held = FALSE;
    KIND(work) = (int)work->scl.x;
    /* @bug logic: work->tick is a Uint8, so the test below 0, CMP/PZ at
     * 0x0c909b44 after EXTU.B, never holds. a scale x of -1, whose low byte is
     * 255, gives KEY_SILVER_STATUE rather than KEY_ICE_STONE */
    if (KIND(work) < 0) {
        KIND(work) = 0;
    } else if (KIND(work) >= KEY_KINDS) {
        KIND(work) = KEY_KINDS - 1;
    }
}

static void handy_key_update(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *lock;
    NJS_VECTOR to;
    Float distance;
    Float nearest_distance;
    int nearest;
    int i;
    NJS_ACTION *action;
    Float last;

    /* @hack redundant: a never-run test of work, which compiles to nothing but
     * keeps the image's register allocation: without it, the two squared
     * lengths of the vector on the stack load its parts into other registers
     * and in another order (0x0c909d6c, 0x0c909db4) and 15 of 403 words
     * differ */
    if (0) {
        if (work) {
        }
    }
    VARS(work)->frame += FRAME_SPEED;
    last = -1.0F;
    /* every kind's frame wraps at the Ice Stone action's last frame */
    action = &ice_stone_action;
    /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
    /* @hack redundant: a ?: on the constant FRAME_SPEED >= 0.0F, which SHC
     * keeps as a test, MOV #1,R8 at 0x0c909b7a and TST R8,R8 at 0x0c909b7e,
     * with the arm for a backward speed at 0x0c909c00; as an if of the forward
     * arm alone, 381 of handy_key_update's 403 words differ */
    (FRAME_SPEED >= 0.0F)
        ? ((VARS(work)->frame >= action->motion->nbFrame + last)
               ? (VARS(work)->frame -= action->motion->nbFrame + last)
               : 0.0F)
        : ((VARS(work)->frame < 0.0F)
               ? (VARS(work)->frame += action->motion->nbFrame + last)
               : 0.0F);
    /* NOLINTEND(readability-avoid-nested-conditional-operator) */
    if (!key_slots[KIND(work)].in_lock) {
        work->flags |= CARRY_WALLS;
        carry_step_small(task);
        if (work->flags & CARRY_HELD) {
            if (VARS(work)->held == FALSE) {
                /* @bug logic: the second test, TST R2,R2 at 0x0c909c78, is
                 * of lines_elsewhere's address, which is never null, so it
                 * never holds. outside the Mystic Ruins a pickup then shows
                 * lines_elsewhere, which holds no line */
                if ((stage_get_stage_act() >> 8) == STAGE_MYSTIC_RUINS ||
                    /* NOLINTNEXTLINE */
                    !key_kinds[KIND(work)].lines_elsewhere) {
                    message_show_timed(key_kinds[KIND(work)].lines);
                } else {
                    message_show_timed(key_kinds[KIND(work)].lines_elsewhere);
                }
                if (KIND(work) == KEY_ICE_STONE) {
                    event_flag_set(FLAG_HOLDING_ICE_STONE);
                } else if (KIND(work) == KEY_WINDY_STONE) {
                    event_flag_set(FLAG_HOLDING_WINDY_STONE);
                }
                player_set_held_kind(0, key_kinds[KIND(work)].held_kind);
            }
            VARS(work)->held = TRUE;
        } else if (VARS(work)->held == TRUE) {
#ifdef GCC
            /* GCC can't see that nearest is set wherever it is read:
             * nearest_distance falls from FAR to LOCK_REACH or below only where
             * the loop sets nearest too */
            nearest = -1;
#endif
            nearest_distance = FAR;
            for (i = 0; i < KEY_KINDS; i++) {
                if (lock_slots[i].lock != NULL) {
                    lock = lock_slots[i].lock->work;
                    to = lock->pos;
                    njSubVector(&to, &work->pos);
                    distance = (to.x * to.x) + (to.y * to.y) + (to.z * to.z);
                    if (distance < nearest_distance) {
                        nearest_distance = distance;
                        nearest = i;
                    }
                }
            }
            if (nearest_distance <= LOCK_REACH) {
                if (nearest == KIND(work)) {
                    message_show_timed(key_fits_lines);
                    key_slots[KIND(work)].in_lock = TRUE;
                } else {
                    message_show_timed(key_wrong_lines);
                }
            }
            VARS(work)->held = work->timer = 0;
            if (KIND(work) == KEY_ICE_STONE) {
                event_flag_clear(FLAG_HOLDING_ICE_STONE);
            } else if (KIND(work) == KEY_WINDY_STONE) {
                event_flag_clear(FLAG_HOLDING_WINDY_STONE);
            }
            player_set_held_kind(0, 0);
        } else {
            /* @hack spelling: work->timer = work->timer + 1, which steers the
             * same registers as the never-run test above; as work->timer++,
             * the same 15 of handy_key_update's 403 words differ */
            work->timer = work->timer + 1;
            if (TASK_TIMER(work) >= LOOSE_STEPS) {
                task_destroy(task);
            }
        }
    } else {
        /* @hack redundant: an if on lock_flag's result with nothing in it,
         * which keeps the call and its test, JSR @R3 at 0x0c909f3e and TST
         * R0,R0 at 0x0c909f42; without it, 39 of handy_key_update's 403 words
         * differ */
        if (lock_flag(KIND(work))) {
        }
    }
}

static void handy_key_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (key_kinds[KIND(work)].action == NULL) {
        draw_tree(key_kinds[KIND(work)].object);
    } else {
        draw_action(key_kinds[KIND(work)].action, VARS(work)->frame);
    }
    njPopMatrix(1);
}

static void handy_key_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (key_slots[KIND(work)].key == task) {
        key_slots[KIND(work)].key = NULL;
    }
}
