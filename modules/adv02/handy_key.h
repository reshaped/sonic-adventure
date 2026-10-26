/* the key the object list names O HANDY KEY: a stone or statue a player
 * carries to the lock of its kind */
#ifndef ADV02_HANDY_KEY_H
#define ADV02_HANDY_KEY_H
#include <sg_xpt.h>

#include "system/task.h"

/* the kinds of key, each with a lock of its own. lock_flags gives Knuckles'
 * locks for the gold and silver statues the flags FLAG_KNUCKLES_MR_RED_CUBE
 * and FLAG_KNUCKLES_MR_BLUE_CUBE */
enum key_kind {
    KEY_ICE_STONE,
    KEY_WINDY_STONE,
    KEY_GOLD_STATUE,
    KEY_SILVER_STATUE,
    KEY_KINDS
};

/* the held kind player_set_held_kind gives player 0 while a player carries
 * each kind of key */
enum {
    HELD_ICE_STONE = 1,
    HELD_WINDY_STONE = 2,
    HELD_GOLD_STATUE = 7,
    HELD_SILVER_STATUE = 8
};

struct key_slot {
    struct task *key; /* the kind's live key, or null */
    Sint8 in_lock; /* TRUE while it sits in its lock */
};

extern struct key_slot key_slots[KEY_KINDS];

/* the least scl.y a key needs to run while its kind's lock holds its key */
#define IN_LOCK_SCL_Y 10.0F

/* the step the object list names for O HANDY KEY. the lock, O DIG PLACE1 and
 * the Mystic Ruins field task also create keys with it, with the kind in
 * scl.x. a key meant to run while its lock holds it needs scl.y
 * IN_LOCK_SCL_Y or more */
void handy_key_step(struct task *task);

#endif
