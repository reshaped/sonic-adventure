/* the deal, which collects the race's items and shuffles them out to the
 * lanes. an item adds itself with deal_add while it waits, and a lane's
 * racer takes its item with deal_take */
#ifndef AL_RACE_DEAL_H
#define AL_RACE_DEAL_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the room for items. after the shuffle, the first RACE_LANES entries are
 * the lanes' */
#define DEAL_ITEMS 20

enum deal_state {
    DEAL_IDLE, /* no deal's task is live */
    DEAL_OPEN, /* deal_add takes items */
    DEAL_DEALT /* deal_take gives them out */
};

/* an item in the deal */
struct deal_item {
    struct task *task; /* null for a lane dealt nothing */
    void (*set)(struct task *task, int lane); /* gives the item its racer */
    Sint8 kind; /* its item kind, as deal_add was given it */
};

/* the deal task's state, in its task's motion */
struct deal_work {
    Sint8 mode; /* 0 until the deal is dealt, then 1 */
};

extern struct deal_item deal_items[DEAL_ITEMS];
extern Sint8 deal_state; /* an enum deal_state */
/* the entries in use: the items added since deal_start, then RACE_LANES or
 * more once dealt */
extern Sint8 deal_count;

/* while deal_state is DEAL_IDLE, store open for deal_add, and add the deal's
 * task to the scenery, whose start opens the deal at once. return the task.
 * return null, changing nothing, while a deal's task is live. without the
 * scenery task, return null, with open stored and the deal still idle */
struct task *deal_create(Sint8 open);
/* add the item in task to the deal, with set to give it its lane's racer.
 * a deal is open when the last deal_create made in DEAL_IDLE had open other
 * than 0, and full once DEAL_ITEMS have been added since deal_start.
 * - an open deal with room adds the item in DEAL_OPEN, and returns TRUE
 * - in DEAL_IDLE, an open deal with room returns FALSE and leaves task be
 * - any other deal returns FALSE and kills task, which task_kill destroys at
 *   its next step
 * the deal keeps task, and deal_take reads its work, so an added task must
 * last through every deal_take */
Bool deal_add(struct task *task, void (*set)(struct task *task, int lane),
              Sint8 kind);
/* in DEAL_DEALT, store in pos the position of the item dealt to lane, and
 * return 1. before it returns, it calls the item's set, if any, with the
 * item's task and lane. return 0, storing nothing, before the deal is dealt
 * or for a null pos. return 0 too for a lane outside 0 to RACE_LANES - 1,
 * or one dealt nothing. the item stays dealt, and each call sets its racer
 * again */
int deal_take(NJS_POINT3 *pos, int lane);
/* the deal's task. deal_start opens the deal, empty. deal_step deals once
 * the deal holds an item and the set editor is closed. it pads the entries
 * with empty ones to RACE_LANES and shuffles them all, so a lane may be
 * dealt nothing. deal_destroy returns deal_state to DEAL_IDLE and leaves the
 * entries and their tasks as they are */
void deal_start(struct task *task, void *arg);
void deal_step(struct task *task);
void deal_destroy(struct task *task);

#endif
