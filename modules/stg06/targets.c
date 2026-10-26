/* the deck's targets and sequences, and the task that runs the sequences */
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

#include "stg06/target.h"
#include "system/task.h"

struct sky_deck_target sky_deck_targets[SKY_DECK_TARGETS];
struct sky_deck_sequence sky_deck_sequences[SKY_DECK_SEQUENCES];

/* the targets' task, or null while there's none */
static struct task *targets_task = NULL;

#define TARGETS_LEVEL 3

static void targets_step(struct task *task)
{
    struct sky_deck_sequence *sequence = sky_deck_sequences;
    int i;
    Bool keep;

    for (i = 0; i < SKY_DECK_SEQUENCES; i++) {
        if (sequence->step) {
            keep = TRUE;
            if (sequence->step(sequence->script, sequence->index, &keep)) {
                sequence->index++;
            }
            if (!keep) {
                sequence->step = NULL;
                sequence->index = 0;
                sequence->script = NULL;
            }
        }
        sequence++;
    }
}

static void targets_destroy(struct task *task)
{
    if (targets_task == task) {
        targets_task = NULL;
    }
}

static void targets_start(struct task *task)
{
    if (!targets_task) {
        targets_task = task;
    }
    task->step = targets_step;
    task->destructor = targets_destroy;
}

static void targets_create(void)
{
    if (!targets_task) {
        memset(sky_deck_targets, 0, sizeof sky_deck_targets);
        memset(sky_deck_sequences, 0, sizeof sky_deck_sequences);
        targets_task = task_create(0, TARGETS_LEVEL, targets_start);
    }
}

void fun_0c9185d2(void)
{
}

struct sky_deck_target *sky_deck_target_add(struct task_work *work)
{
    struct sky_deck_target *target = sky_deck_targets;
    int i;

    targets_create();
    /* @hack spelling: the cursor stepped on as it tests an entry, and back
     * for a free one. the image steps it in the delay slot of the BF/S at
     * 0x0c9185ee, then takes the step back with ADD #-12,R14; stepped once a
     * pass, the function runs 2 bytes short and 22 of 32 words differ */
    for (i = 0; i < SKY_DECK_TARGETS; i++) {
        if (!(target++)->work) {
            memset(--target, 0, sizeof *target);
            target->work = work;
            return target;
        }
    }
    return NULL;
}

Bool sky_deck_target_remove(struct sky_deck_target *target)
{
    if (target->work) {
        memset(target, 0, sizeof *target);
        return TRUE;
    }
    return FALSE;
}

struct sky_deck_target *sky_deck_target_find_stood_on(Sint8 unused)
{
    struct sky_deck_target *target = sky_deck_targets;
    int i;

    for (i = 0; i < SKY_DECK_TARGETS; i++, target++) {
        if (target->work && (target->state & TARGET_STOOD_ON)) {
            return target;
        }
    }
    return NULL;
}

struct sky_deck_target *sky_deck_target_find_stood_on_in(Sint8 group)
{
    struct sky_deck_target *target = sky_deck_targets;
    int i;

    for (i = 0; i < SKY_DECK_TARGETS; i++, target++) {
        if (target->work && target->group == group &&
            (target->state & TARGET_STOOD_ON)) {
            return target;
        }
    }
    return NULL;
}

struct sky_deck_target *sky_deck_target_find_nearest(Sint8 group,
                                                     NJS_POINT3 *pos)
{
    Float x = pos->x;
    Float y = pos->y;
    Float z = pos->z;
    Float nearest = -1.0F;
    struct sky_deck_target *target = sky_deck_targets;
    struct sky_deck_target *found = NULL;
    int i;

    /* @hack redundant: three stores that never run, which weigh target in
     * SHC's register choice; without them, 24 of 133 words differ */
    if (0) {
        target->queued = target->state;
        target->queued = target->state;
        target->queued = target->state;
    }
    for (i = 0; i < SKY_DECK_TARGETS; i++) {
        if (target->work && target->group == group) {
            /* @hack spelling: the position read through a byte cursor, for the
             * image's @Rn+ loads; through the members, the function runs 32
             * bytes long and 120 of 133 words differ */
            char *at = (char *)&target->work->pos;
            Float dx;
            Float dy;
            Float dz;
            Float distance;

            dx = *(Float *)at - x;
            at += sizeof(Float);
            dy = *(Float *)at - y;
            at += sizeof(Float);
            dz = *(Float *)at - z;
            distance = (dx * dx) + (dy * dy) + (dz * dz);
            if (distance < nearest || !found) {
                nearest = distance;
                found = target;
            }
        }
        target++;
    }
    return found;
}

struct sky_deck_target *sky_deck_target_find_lowest_below(Sint8 group,
                                                          NJS_POINT3 *pos,
                                                          Sint8 state,
                                                          Float radius)
{
    Float x = pos->x;
    Float y = pos->y;
    Float z = pos->z;
    struct sky_deck_target *target = sky_deck_targets;
    struct sky_deck_target *found = NULL;
    int i;

    for (i = 0; i < SKY_DECK_TARGETS; i++) {
        if (target->work && target->group == group && target->state == state &&
            target->work->pos.y < y &&
            njAbs(target->work->pos.x - x) < radius &&
            njAbs(target->work->pos.z - z) < radius) {
            found = target;
            y = target->work->pos.y;
        }
        target++;
    }
    return found;
}

struct sky_deck_target *sky_deck_target_find(Sint8 group, NJS_POINT3 *pos,
                                             Float radius, Sint8 state,
                                             Float height)
{
    Float x = pos->x;
    Float y = pos->y;
    Float z = pos->z;
    struct sky_deck_target *target = sky_deck_targets;
    int i;

    for (i = 0; i < SKY_DECK_TARGETS; i++, target++) {
        if (target->work && target->group == group && target->state == state &&
            njAbs(target->work->pos.x - x) < radius &&
            njAbs(target->work->pos.z - z) < radius &&
            njAbs(target->work->pos.y - y) < height) {
            return target;
        }
    }
    return NULL;
}

struct sky_deck_target *sky_deck_target_find_masked(Sint8 group,
                                                    NJS_POINT3 *pos,
                                                    Float radius, Float height,
                                                    Sint8 state, Sint8 mask)
{
    Float x = pos->x;
    Float y = pos->y;
    Float z = pos->z;
    struct sky_deck_target *target = sky_deck_targets;
    int wanted;
    int i;

    /* @hack spelling: i cleared ahead of wanted, which gives i R14 and mask R1
     * (0x0c9189f8, 0x0c9189ee); with i = 0 in the for, 17 of 54 words
     * differ */
    i = 0;
    wanted = state & mask;
    for (; i < SKY_DECK_TARGETS; i++, target++) {
        if (target->work && target->group == group &&
            (target->state & mask) == wanted &&
            njAbs(target->work->pos.x - x) < radius &&
            njAbs(target->work->pos.z - z) < radius &&
            njAbs(target->work->pos.y - y) < height) {
            return target;
        }
    }
    return NULL;
}

struct sky_deck_target *sky_deck_target_find_id_masked(Sint8 group, Sint8 state,
                                                       Sint8 mask, Sint8 id)
{
    struct sky_deck_target *target = sky_deck_targets;
    int wanted;
    int i;

    /* @hack spelling: i cleared ahead of wanted, which clears i's R13 at
     * 0x0c918a52, between the loads of mask and state; with i = 0 in the
     * for, 32 of 35 words differ and the object's code ends 14 bytes early */
    i = 0;
    /* @hack spelling: wanted masked in a statement of its own, which gives
     * wanted R12 and mask R1 (0x0c918a56, 0x0c918a4e); as
     * wanted = state & mask, 9 of 35 words differ */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    wanted = state;
    wanted &= mask;
    for (; i < SKY_DECK_TARGETS; i++, target++) {
        if (target->work && target->group == group &&
            (target->state & mask) == wanted && target->id == id) {
            return target;
        }
    }
    return NULL;
}

struct sky_deck_target *sky_deck_target_find_id(Sint8 group, Sint8 state,
                                                Sint8 id)
{
    struct sky_deck_target *target = sky_deck_targets;
    int i;

    for (i = 0; i < SKY_DECK_TARGETS; i++, target++) {
        if (target->work && target->group == group && target->state == state &&
            target->id == id) {
            return target;
        }
    }
    return NULL;
}

Bool sky_deck_sequence_add(Bool (*step)(void *script, int index, Bool *keep),
                           void *script)
{
    struct sky_deck_sequence *sequence;
    int i;

    if (step) {
        sequence = sky_deck_sequences;
        for (i = 0; i < SKY_DECK_SEQUENCES; i++, sequence++) {
            if (!sequence->step) {
                sequence->step = step;
                sequence->index = 0;
                sequence->script = script;
                return TRUE;
            }
        }
    }
    return FALSE;
}
