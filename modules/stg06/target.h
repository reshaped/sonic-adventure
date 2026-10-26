/* the deck's targets and sequences. a target is an entry by which the
 * cannons and the things they hit find one another. a sequence is a script
 * the targets' task steps through by index */
#ifndef STG06_TARGET_H
#define STG06_TARGET_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

#define SKY_DECK_TARGETS 100
#define SKY_DECK_SEQUENCES 5

/* the bits of a target's state */
#define TARGET_STOOD_ON 0x1 /* player 0 stands on it */
#define TARGET_BROKEN 0x2 /* a hit has broken it */
#define TARGET_GONE 0x4 /* its pieces have fallen away */

/* a target's group. the finds compare a target's group with the one they are
 * given. the cannons also test its bits. TARGET_CYLINDER holds the
 * cylinders. TARGET_PLATFORM holds TALAP_0 and CONNECT_0, which the launchers
 * fire at. TARGET_TALAP_1 holds TALAP_1 and EDGE_1, which a breaking
 * cylinder hits. TARGET_LAUNCHER holds the launchers, CANNON_B.
 * TARGET_CANNON_H2 holds CANNON_H2, which nothing hits */
#define TARGET_CYLINDER 1
#define TARGET_PLATFORM 4
#define TARGET_TALAP_1 (TARGET_CYLINDER | TARGET_PLATFORM)
#define TARGET_LAUNCHER 8
#define TARGET_CANNON_H2 16

struct sky_deck_target {
    struct task_work *work; /* null for a free entry */
    /* the target's hit at pos, by something moving at speed: FALSE once it
     * takes no more hits. null for a target nothing hits */
    Bool (*hit)(NJS_POINT3 *pos, NJS_VECTOR *speed, struct task_work *work);
    Sint8 id; /* its owner's, to find it by */
    Sint8 state;
    Sint8 group;
    Sint8 queued; /* a launcher's: 1 once its script is queued */
};

/* a script the targets' task runs. each step of the task calls the
 * sequence's step with its script and index. a TRUE result advances the
 * index. a step that clears keep, which starts TRUE, frees the slot */
struct sky_deck_sequence {
    Bool (*step)(void *script, int index, Bool *keep); /* null for a free one */
    int index;
    void *script;
};

extern struct sky_deck_target sky_deck_targets[SKY_DECK_TARGETS];
extern struct sky_deck_sequence sky_deck_sequences[SKY_DECK_SEQUENCES];

/* the targets' task is gone when an act starts. while it is gone, the next
 * sky_deck_target_add resets the targets. a reset clears every entry and
 * sequence, then creates the task. while task_create fails, each add resets
 * again. after a reset, an add can give new work an entry that an old holder
 * still keeps. the old holder's remove then frees the entry from the new
 * work. so a holder adds in its start, removes in its destructor, and keeps
 * no entry across a reset */

/* clear the first free entry of sky_deck_targets and give it to work. null
 * with every entry taken. the entry stays work's until
 * sky_deck_target_remove or a reset. the game's callers leave the result
 * unchecked */
struct sky_deck_target *sky_deck_target_add(struct task_work *work);
/* free the entry. TRUE when work held it, and FALSE, changing nothing,
 * when it was free */
Bool sky_deck_target_remove(struct sky_deck_target *target);
/* does nothing */
void fun_0c9185d2(void);
/* the finds below look at taken entries alone, in the order of
 * sky_deck_targets. an entry's position is its work's. each find returns
 * null when no entry fits */

/* the first entry with TARGET_STOOD_ON, of any group. unused goes unread */
struct sky_deck_target *sky_deck_target_find_stood_on(Sint8 unused);
/* the first entry in group with TARGET_STOOD_ON */
struct sky_deck_target *sky_deck_target_find_stood_on_in(Sint8 group);
/* the lowest entry in group whose state equals state, below pos. it must lie
 * within radius of pos along x and along z. of entries at one height, the
 * first wins */
struct sky_deck_target *sky_deck_target_find_lowest_below(Sint8 group,
                                                          NJS_POINT3 *pos,
                                                          Sint8 state,
                                                          Float radius);
/* the first entry in group whose state equals state, in the box about pos.
 * the box reaches radius along x and along z, and height along y */
struct sky_deck_target *sky_deck_target_find(Sint8 group, NJS_POINT3 *pos,
                                             Float radius, Sint8 state,
                                             Float height);
/* the entry in group nearest pos. of entries equally near, the first
 * wins */
struct sky_deck_target *sky_deck_target_find_nearest(Sint8 group,
                                                     NJS_POINT3 *pos);
/* the first entry as sky_deck_target_find finds it, but with state
 * compared in mask's bits alone */
struct sky_deck_target *sky_deck_target_find_masked(Sint8 group,
                                                    NJS_POINT3 *pos,
                                                    Float radius, Float height,
                                                    Sint8 state, Sint8 mask);
/* the first entry in group with id, whose state equals state in mask's
 * bits */
struct sky_deck_target *sky_deck_target_find_id_masked(Sint8 group, Sint8 state,
                                                       Sint8 mask, Sint8 id);
/* the first entry in group with id, whose state equals state */
struct sky_deck_target *sky_deck_target_find_id(Sint8 group, Sint8 state,
                                                Sint8 id);

/* run script through step, from index 0, in the first free slot. TRUE once
 * a slot holds it. FALSE, changing nothing, for a null step or with every
 * slot taken. only the targets' task runs a sequence, and the reset that
 * creates the task frees every slot. so add a sequence only after a
 * sky_deck_target_add has created the task. script is borrowed until its
 * slot is freed */
Bool sky_deck_sequence_add(Bool (*step)(void *script, int index, Bool *keep),
                           void *script);

#endif
