/* the lock the object list names O LOCK: it takes the key of its kind and
 * holds it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/handy_key.h"
#include "adv02/lock.h"
#include "adv02/texlists.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT ice_stone_lock_object;
extern NJS_OBJECT windy_stone_lock_object;
extern NJS_OBJECT gold_statue_lock_object;
extern NJS_OBJECT silver_statue_lock_object;

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the lock only while a player is this near */
#define NEAR 50.0F

#define LOCK_FLAGS WORLD_SMALL

#define HOLD_HEIGHT 10.0F /* how far above the lock the key waits */
/* the key's rise or fall each step, to HOLD_HEIGHT */
#define RISE_SPEED 0.14F
#define WAIT_STEPS 20 /* the steps of each wait */
/* the key's move each step, over to the lock's center */
#define CROSS_SPEED 0.12F
#define SINK_SPEED 0.1F /* the key's fall each step, into the lock */
/* the key's turn each step, from its first wait on. once it has sunk within
 * TURN_HEIGHT of the lock, it turns only until it faces the lock's y angle */
#define TURN_SPEED 0x200
#define TURN_HEIGHT 7.0F

/* the scl.y of a key the lock creates, which is IN_LOCK_SCL_Y or more, so the
 * key runs though its lock holds it */
#define KEY_SCL_Y 1000.0F

#define SE_LOCKED 582

enum lock_mode { LOCK_INIT, LOCK_RUN };

/* the lock's states, in work->submode. LOCK_RISE to LOCK_SINK carry a key in,
 * and LOCK_HOLDS keeps it there */
enum lock_state {
    LOCK_EMPTY,
    LOCK_RISE,
    LOCK_WAIT,
    LOCK_CROSS,
    LOCK_WAIT_AGAIN,
    LOCK_SINK,
    LOCK_HOLDS
};

/* the lock's key_kind, in work->tick. work->timer counts down the steps left
 * in a wait */
#define KIND(work) ((work)->tick)

struct lock_vars {
    NJS_OBJECT *object; /* the lock's copy in the collision world */
};

#define VARS(work) ((struct lock_vars *)&(work)->vars)

struct lock_kind_info {
    Sint32 unk_00;
    NJS_OBJECT *object; /* the tree the lock draws */
    NJS_OBJECT *world_object; /* what lock_add_to_world copies */
};

static void lock_init(struct task *task);
static void lock_reset(struct task *task);
static void lock_add_to_world(struct task *task);
static void lock_update(struct task *task);
static void lock_draw(struct task *task);
static void lock_destroy(struct task *task);
static void lock_remove(struct task *task);

static struct lock_kind_info lock_kinds[KEY_KINDS] = {
    {0, &ice_stone_lock_object, &ice_stone_lock_object},
    {0, &windy_stone_lock_object, &windy_stone_lock_object},
    {0, &gold_statue_lock_object, &gold_statue_lock_object},
    {0, &silver_statue_lock_object, &silver_statue_lock_object},
};

/* the locks whose tasks have started and not yet been destroyed */
static int lock_count = 0;

struct lock_slot lock_slots[KEY_KINDS] = {{NULL}, {NULL}, {NULL}, {NULL}};

int lock_flags[CHARACTER_COUNT][KEY_KINDS] = {
    {FLAG_SONIC_MR_ICE_STONE, FLAG_SONIC_MR_WINDY_STONE, -1, -1},
    {-1, -1, -1, -1},
    {FLAG_TAILS_MR_ICE_STONE, FLAG_TAILS_MR_WINDY_STONE, -1, -1},
    {-1, -1, FLAG_KNUCKLES_MR_RED_CUBE, FLAG_KNUCKLES_MR_BLUE_CUBE},
    {-1, -1, -1, -1},
    {-1, -1, -1, -1},
    {-1, FLAG_E102_MR_WINDY_STONE, -1, -1},
    {FLAG_BIG_MR_ICE_STONE, -1, -1, -1},
};

/* a lock draws and updates only while it holds its kind's slot and no event
 * has control. while the set editor holds a lock, each step frees its slot
 * and reads its kind again. it then draws the lock only while no other lock
 * holds that kind's slot */
void lock_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case LOCK_INIT:
                lock_init(task);
                break;
            case LOCK_RUN:
                if (event_in_control()) {
                    break;
                }
                if (lock_slots[KIND(work)].lock != task) {
                    break;
                }
                lock_draw(task);
                lock_update(task);
                break;
            }
        } else {
            switch (work->mode) {
            case LOCK_INIT:
                lock_init(task);
                break;
            case LOCK_RUN:
                lock_remove(task);
                lock_reset(task);
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> KIND(0-%d)",
                            KEY_KINDS - 1);
                if (lock_slots[KIND(work)].lock != NULL) {
                    debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y),
                                "DUPLICATED BY ANOTHER!");
                } else {
                    lock_draw(task);
                }
                break;
            }
        }
    }
}

static void lock_init(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    /* @hack redundant: a test of lock_count and an empty loop over the key
     * kinds, which the image keeps (TST R3,R3 at 0x0c90a1ac, the loop from
     * 0x0c90a1b0 to 0x0c90a1b6); without them, the function shrinks, the
     * functions after it move, and 23 of 27 words differ */
    if (lock_count == 0) {
        for (i = 0; i < KEY_KINDS; i++) {
        }
    }
    lock_count++;
    task->draw = lock_draw;
    task->destructor = lock_destroy;
    lock_reset(task);
    work->mode = LOCK_RUN;
}

/* read the lock's kind from scale x, clamped to the key kinds. the lock then
 * takes its kind's slot if no lock holds it. a lock that takes the slot goes
 * into the collision world. if its flag is set, it starts in LOCK_HOLDS and
 * creates the key it holds, unless a key of its kind is live. while the set
 * editor holds the lock, it takes no slot */
static void lock_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct task *key;
    struct task_work *key_work;
    struct task_motion *motion;

    KIND(work) = (int)work->scl.x;
    /* @bug logic: work->tick is a Uint8, so the test below 0, CMP/PZ at
     * 0x0c90a1ee after EXTU.B, never holds. a scale x of -1, whose low byte is
     * 255, gives KEY_SILVER_STATUE rather than KEY_ICE_STONE */
    if (KIND(work) < 0) {
        KIND(work) = 0;
    } else if (KIND(work) >= KEY_KINDS) {
        KIND(work) = KEY_KINDS - 1;
    }
    if (!placement_editing(task)) {
        if (lock_slots[KIND(work)].lock == NULL) {
            lock_slots[KIND(work)].lock = task;
            lock_add_to_world(task);
            if (lock_flag(KIND(work))) {
                if (key_slots[KIND(work)].key == NULL) {
                    key =
                        task_create(TASK_MOTION | TASK_WORK, 3, handy_key_step);
                    key_work = key->work;
                    motion = key->motion;
                    key_work->pos.x = work->pos.x;
                    key_work->pos.y = work->pos.y;
                    key_work->pos.z = work->pos.z;
                    key_work->ang.y = work->ang.y;
                    key_work->scl.x = KIND(work);
                    key_work->scl.y = KEY_SCL_Y;
                    motion->speed.x = 0.0F;
                    motion->speed.y = 0.5F;
                    motion->speed.z = 0.0F;
                }
                work->submode = LOCK_HOLDS;
            } else {
                work->submode = LOCK_EMPTY;
            }
        }
    }
}

/* put a copy of the kind's world_object into the collision world, moved by
 * the lock's position and turned to its y angle. the task's force is zeroed
 * first, so a player standing on the copy isn't moved */
static void lock_add_to_world(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *from;
    NJS_OBJECT *object;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = lock_kinds[KIND(work)].world_object;
    VARS(work)->object = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(LOCK_FLAGS, task, object);
}

/* an empty lock takes its kind's key once the key is live and its key_slot's
 * in_lock is set. once the key has sunk in, the lock sets its flag and plays
 * SE_LOCKED. a lock in LOCK_HOLDS keeps its key at its own position and y
 * angle. no state after LOCK_EMPTY tests the key for null */
static void lock_update(struct task *task)
{
    struct task_work *work = task->work;
    struct key_slot *slot = &key_slots[KIND(work)];
    struct task_work *key;
    Float height;
    Float x;
    Float z;
    Float distance;
    Float scale;
    Sint16 turn;

    if (slot->key == NULL) {
        key = NULL;
    } else {
        key = slot->key->work;
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (work->submode == LOCK_EMPTY) {
        if (slot->in_lock && key != NULL) {
            work->submode = LOCK_RISE;
        }
    } else if (work->submode == LOCK_RISE) {
        height = key->pos.y - work->pos.y - HOLD_HEIGHT;
        if (height < -RISE_SPEED) {
            key->pos.y += RISE_SPEED;
        } else if (height > RISE_SPEED) {
            key->pos.y -= RISE_SPEED;
        } else {
            key->pos.y -= height;
            work->timer = WAIT_STEPS;
            work->submode = LOCK_WAIT;
        }
    } else if (work->submode == LOCK_WAIT) {
        key->ang.y += TURN_SPEED;
        work->timer--;
        if (TASK_TIMER(work) <= 0) {
            work->submode = LOCK_CROSS;
        }
    } else if (work->submode == LOCK_CROSS) {
        key->ang.y += TURN_SPEED;
        x = work->pos.x - key->pos.x;
        z = work->pos.z - key->pos.z;
        distance = njSqrt((x * x) + (z * z));
        if (distance > CROSS_SPEED) {
            scale = CROSS_SPEED / distance;
            key->pos.x += scale * x;
            key->pos.z += scale * z;
        } else {
            key->pos.x = work->pos.x;
            key->pos.z = work->pos.z;
            work->timer = WAIT_STEPS;
            work->submode = LOCK_WAIT_AGAIN;
        }
    } else if (work->submode == LOCK_WAIT_AGAIN) {
        key->ang.y += TURN_SPEED;
        work->timer--;
        if (TASK_TIMER(work) <= 0) {
            work->submode = LOCK_SINK;
        }
    } else if (work->submode == LOCK_SINK) {
        height = key->pos.y - work->pos.y;
        if (height > TURN_HEIGHT) {
            key->ang.y += TURN_SPEED;
        } else {
            turn = (Sint16)(work->ang.y - key->ang.y);
            if (turn != 0) {
                if (turn < 0 || turn > TURN_SPEED) {
                    key->ang.y += TURN_SPEED;
                } else {
                    key->ang.y = work->ang.y;
                }
            }
        }
        if (height < -SINK_SPEED) {
            key->pos.y += SINK_SPEED;
        } else if (height > SINK_SPEED) {
            key->pos.y -= SINK_SPEED;
        } else {
            key->pos.y = work->pos.y;
            lock_set_flag(KIND(work), TRUE);
            sound_play(SE_LOCKED, 0, 0, 0);
            work->submode = LOCK_HOLDS;
        }
    } else {
        key->pos.x = work->pos.x;
        key->pos.y = work->pos.y;
        key->pos.z = work->pos.z;
        key->ang.y = work->ang.y;
    }
}

static void lock_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(lock_kinds[KIND(work)].object);
    njPopMatrix(1);
}

static void lock_destroy(struct task *task)
{
    int i;

    lock_count--;
    /* @hack redundant: a test of lock_count and an empty loop over the key
     * kinds, which the image keeps (TST R2,R2 at 0x0c90a6bc, the loop from
     * 0x0c90a6c2 to 0x0c90a6c8); without them, the function shrinks, and
     * lock.c's P ends 32 bytes short of its row, count unmeasured */
    if (lock_count == 0) {
        for (i = 0; i < KEY_KINDS; i++) {
        }
    }
    lock_remove(task);
}

/* if the lock holds its kind's slot, free the slot and take the lock out of
 * the collision world */
static void lock_remove(struct task *task)
{
    struct task_work *work = task->work;

    if (lock_slots[KIND(work)].lock == task) {
        lock_slots[KIND(work)].lock = NULL;
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

void lock_set_flag(int kind, Bool on)
{
    int flag = lock_flags[player_get_character(0)][kind];

    if (flag >= 0) {
        event_flags[flag] = on;
    }
}

Bool lock_flag(int kind)
{
    int flag = lock_flags[player_get_character(0)][kind];

    if (flag >= 0) {
        return event_flags[flag];
    }
    return FALSE;
}
