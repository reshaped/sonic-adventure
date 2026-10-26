/* Chao race item messages and zones */
#ifndef AL_RACE_ITEM_H
#define AL_RACE_ITEM_H
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "object/simple.h"
#include "system/task.h"

/* the messages an item and its racer's Chao send each other. ITEM_LET_GO
 * goes either way, and the side that gets it lets go of the other. the
 * rest are what an item asks of the Chao, as racer_tell's modes */
#define ITEM_LET_GO 0
#define ITEM_SWING 1
#define ITEM_GO_TO 2
#define ITEM_EAT 3
#define ITEM_PUSH 4
#define ITEM_TAKE 5

/* the modes of an item a Chao takes and plays */
enum item_mode {
    ITEM_WAIT, /* until deal_add adds it to the deal */
    ITEM_LYING, /* dealt, until its racer's Chao takes it */
    ITEM_HELD, /* taken, until the Chao starts to play it */
    ITEM_IN_USE, /* the Chao plays it */
    ITEM_USED /* played, and let go */
};

/* an item's state, in its task's motion */
struct item {
    Sint8 mode;
    Sint8 kind; /* its index in item_tells */
    Sint8 racer; /* the lane it's dealt to, or -1 */
    Uint8 unk_03;
    NJS_MODEL *model;
    struct collision_list *list;
    struct task *owner; /* the task of the Chao that holds it, or null */
};

#define ITEM(task) ((struct item *)(task)->motion)

/* an item kind's starting values */
struct item_start {
    Sint8 kind;
    NJS_OBJECT *object;
    struct collision_list list;
};

enum zone_size { ZONE_NONE, ZONE_SMALL, ZONE_LARGE };

/* the task of the Chao whose record collision touched this step, or null.
 * a Chao's record is a COLLISION_ENEMY one, and any of that kind counts */
struct task *item_hit_chao(struct collision_work *collision);
/* give chao_task's racer command through racer_tell, with other as its
 * source and kind as the source's kind, and return 1. 0, telling nothing,
 * when chao_task, its work or other is null. 0 too when the task's racer is
 * null or hasn't its Chao */
int item_tell_chao(struct task *chao_task, struct task *other, Sint8 command,
                   Sint8 kind);
/* tell the item in task command, from other, through item_tells[kind], and
 * return the handler's result. 0, telling nothing, when task or other is
 * null. kind must be 0 to 5 */
int item_tell(struct task *task, struct task *other, Sint8 command, Sint8 kind);
/* the square of the distance between task's position and other's, or -1
 * when either is null */
Float item_distance_squared(struct task *task, struct task *other);
/* the work of the zone that owns collision, or null when no zone does.
 * collision may be null */
struct task_work *zone_get_work(struct collision_work *collision);
/* the size of the zone that owns collision, or ZONE_NONE when no zone does.
 * collision may be null */
enum zone_size zone_get_size(struct collision_work *collision);

#endif
