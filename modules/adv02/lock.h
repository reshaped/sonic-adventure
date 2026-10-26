/* the lock the object list names O LOCK: it takes the key of its kind and
 * holds it */
#ifndef ADV02_LOCK_H
#define ADV02_LOCK_H

#include "adv02/handy_key.h"
#include "player/player.h"
#include "system/task.h"

/* a key kind's slot: the one lock of the kind that runs, or null */
struct lock_slot {
    struct task *lock;
};

extern struct lock_slot lock_slots[KEY_KINDS];
/* each character's event flag for each kind of lock, set while the lock holds
 * its key, or -1 where the character has no such lock */
extern int lock_flags[CHARACTER_COUNT][KEY_KINDS];

/* the step the object list names for O LOCK */
void lock_step(struct task *task);
/* set or clear the flag lock_flags gives player 0's character for a kind of
 * lock, and read it. a set flag means the lock holds its key. where the
 * character has no flag, lock_set_flag does nothing and lock_flag returns
 * FALSE */
void lock_set_flag(int kind, Bool on);
Bool lock_flag(int kind);

#endif
