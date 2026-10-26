/* placement: the objects an act's set file lists, and the tasks that run
 * them. an object gets a task as a player or the camera comes near it */
#ifndef OBJECT_PLACEMENT_H
#define OBJECT_PLACEMENT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the size of placements, the most objects an act places, whatever its
 * set file holds */
#define PLACEMENT_COUNT 1023

/* a placement's flags */
#define PLACEMENT_LIVE 0x1 /* its task lives */
#define PLACEMENT_SAVED 0x2 /* leave.pose holds how its last task left */
/* placement_set_used sets it, for the placement's next task to read */
#define PLACEMENT_USED 0x4
#define PLACEMENT_IN_RANGE 0x8 /* the range checks always keep its task */
#define PLACEMENT_KEEP 0x10 /* placement_reset leaves it as it is */
#define PLACEMENT_HELD 0x4000 /* its task lives on when the act changes */
/* the manager gives it a task when it is near and has no live one */
#define PLACEMENT_ACTIVE 0x8000

/* the placement range, squared, of a kind without KIND_RANGE */
#define PLACE_RANGE (400.0F * 400.0F)
/* what the manager adds to a squared placement range for the task's leave
 * range. it adds to the square, so the task leaves only a little farther
 * out than it was placed */
#define LEAVE_MARGIN 100.0F

/* a set entry's id: its kind, and whether it's in the player-dependent
 * set rather than the common one */
#define SET_KIND 0x7fff
#define SET_PLAYER 0x8000

/* one object in a set file: its kind, as its place in the object list,
 * and how it stands */
struct set_entry {
    Uint16 id;
    Sint16 ang_x;
    Sint16 ang_y;
    Sint16 ang_z;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
};

/* a set file's 32-byte header: the count of its entries, which follow it */
struct set_file {
    Uint32 count;
    Uint8 unk_04[28];
};

/* an object kind's attr */
#define KIND_RANGE 0x1 /* its range is its own, not PLACE_RANGE */
/* without KIND_RANGE, placed at any distance, and never leaves */
#define KIND_ALWAYS 0x2
/* placed at any distance in the manager's first pass */
#define KIND_FIRST 0x4
#define KIND_FIXED 0x8 /* the set editor can't take its placements */

/* one kind of object an act can place, and the step its task runs */
struct object_kind {
    Uint8 create; /* task_create's flags */
    Uint8 level;
    Sint16 attr;
    Float range; /* with KIND_RANGE, its placement range, squared */
    void *param; /* stored in its task's work->vars.p[0] */
    void (*step)(struct task *task);
    char *name; /* the set editor's name for it */
};

/* the kinds of object an act can place */
struct object_list {
    Sint16 count;
    struct object_kind *kinds;
};

/* how a task stood when it left range */
struct placement_pose {
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
};

/* one object from the set file, and the task that runs it */
struct placement {
    Sint8 tasks; /* how many it has made */
    Sint8 state; /* a byte its tasks keep from one to the next */
    Sint16 flags;
    struct task *task;
    struct set_entry *entry;
    union {
        Float range; /* the leave range, squared, while its task lives */
        struct placement_pose *pose; /* with PLACEMENT_SAVED */
    } leave;
};

/* the act's object list and set file, the file's count and entries, and
 * the placements, one for each entry. nothing outside placement.c reads
 * placement_file. it is extern for its place in bss, ahead of placements:
 * as a static, it would follow them */
extern struct object_list *placement_list;
extern struct set_file *placement_file;
extern Uint32 *placement_file_count;
extern struct set_entry *placement_entries;
extern Sint16 placement_count;
extern struct placement placements[PLACEMENT_COUNT];

/* the manager's step: it places the act's objects as the players or the
 * camera come near them, and changes act when placement_change_act asks */
void placement_manager_step(struct task *task);
/* make each placement without PLACEMENT_KEEP wait to be placed again: its
 * flags become PLACEMENT_ACTIVE alone, and its state 0. the walk runs to
 * placement_count plus the set file's count, up to PLACEMENT_COUNT. as
 * placement_count already counts the file's entries, the walk runs past
 * them and makes the unused placements there active too. without a set
 * file, nothing changes */
void placement_reset(void);
/* step the act by change, and ask the manager to take that act's set file */
void placement_change_act(Sint16 change);

/* the range checks. each compares the squared distance from the task's
 * position with a squared range. a _by function takes the range as its
 * argument. the others take the placement's leave.range, or 410 squared
 * for a task without a placement. a task whose placement has
 * PLACEMENT_IN_RANGE, or whose range is 0, is always in range. each reads
 * the task's work */

/* TRUE when the camera is out of range of the task, else FALSE.
 * camera_work must not be null */
Bool placement_far_from_camera(struct task *task);
Bool placement_far_from_camera_by(struct task *task, Float range);
/* FALSE while the camera or player 0 is within range of the task, or
 * player 1 is Sonic and within 40 of it. otherwise TRUE, and the task
 * destroys itself on its next step */
Bool placement_try_leave(struct task *task);
Bool placement_try_leave_by(struct task *task, Float range);
/* as placement_try_leave. a task that leaves first saves its pose for
 * the placement's next task, when the heap has room for it. a task without
 * a placement saves the pose through null */
Bool placement_try_leave_saving(struct task *task);
Bool placement_try_leave_saving_by(struct task *task, Float range);

/* the placement's flags, kind and state. each function but
 * placement_set_state and placement_get_state needs task->placement */
void placement_set_used(struct task *task);
void placement_keep(struct task *task);
/* PLACEMENT_USED when it is set, else 0 */
int placement_used(struct task *task);
void placement_hold(struct task *task);
/* PLACEMENT_HELD when it is set, else 0 */
int placement_held(struct task *task);
/* clear PLACEMENT_ACTIVE and PLACEMENT_LIVE, so the placement makes no
 * more tasks until placement_reset. the task runs on */
void placement_stop(struct task *task);
/* as placement_stop, and the task destroys itself on its next step */
void placement_kill(struct task *task);
/* the kind in the placement's set entry, without SET_PLAYER */
Uint16 placement_get_kind(struct task *task);
/* the state byte a placement keeps from one of its tasks to the next.
 * without a placement, placement_set_state does nothing, and
 * placement_get_state's result is undefined */
void placement_set_state(struct task *task, Sint8 state);
Sint8 placement_get_state(struct task *task);

#endif
