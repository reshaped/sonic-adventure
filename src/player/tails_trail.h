/* Tails' trails: the quads his tails leave as he attacks with them */
#ifndef PLAYER_TAILS_TRAIL_H
#define PLAYER_TAILS_TRAIL_H
#include <ninja.h>
#include <sg_xpt.h>

struct collision_work;
struct task;

/* a trail's work, in task_work's layout, since the collision code and
 * placement_try_leave read it as a task_work. from lies where
 * task_work's pos does */
struct tails_trail_work {
    Uint8 unk_00[6];
    Uint16 timer; /* the steps it has lived */
    NJS_POINT3 center; /* the middle of his body, the step before */
    Uint8 unk_14[0xc];
    NJS_POINT3 from; /* where the tail's end was */
    NJS_POINT3 to; /* where the tail's end is */
    struct collision_work *collision;
};

#define TAILS_TRAIL_WORK(task) ((struct tails_trail_work *)(task)->work)
/* the middle of his body now, in the task's extra */
#define TAILS_TRAIL_CENTER(task) ((NJS_POINT3 *)(task)->extra)

/* the steps of a trail's task. each tail's end leaves two trails in Tails'
 * tail attack. tails_trail_step runs the outer one, which strikes enemies.
 * tails_trail_inner_step runs the inner one, which strikes nothing. the
 * code that makes a trail's task gives it TASK_WORK and TASK_EXTRA. it sets
 * the work's from, to and center, and TAILS_TRAIL_CENTER in the extra */
void tails_trail_step(struct task *task);
void tails_trail_inner_step(struct task *task);

#endif
