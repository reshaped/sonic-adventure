/* Zero, E-100 alpha, the robot that chases Amy */
#ifndef ENEMY_ZERO_H
#define ENEMY_ZERO_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* EGGROB, Zero's PVM */
extern NJS_TEXLIST zero_texlist;

/* where and how Zero comes in */
struct zero_start {
    Sint32 how; /* ZERO_DASH, ZERO_STAND or ZERO_LEAP */
    NJS_POINT3 pos;
    NJS_POINT3 target; /* ignored by ZERO_STAND */
};

/* ZERO_DASH and ZERO_LEAP start Zero 5 above pos, heading for target.
 * ZERO_DASH covers the level distance to target at 3 a step, then slows to
 * a stop and drops. ZERO_LEAP leaps ahead at 1 a step and up at 4.
 * ZERO_STAND drops from pos itself, facing player 0 */
#define ZERO_DASH 0
#define ZERO_STAND 1
#define ZERO_LEAP 2

/* a bit of Zero's work->tick, which zero_leave sets. on its next step,
 * Zero rises away for 60 steps and ends */
#define ZERO_LEAVING 0x1

/* make Zero as start says, and return its task. returns null when the
 * character isn't Amy, player 0 has no work, or task_create fails */
struct task *zero_create(struct zero_start *start);
/* zero_blind keeps Zero from seeing Amy, and zero_push adds push to each of
 * its moves. each holds until the end of Zero's next step that no event
 * controls */
void zero_blind(void);
void zero_push(NJS_VECTOR *push);

/* the first step of the Zero an event makes with event_create_player. it
 * loads EGGROB and drops any throw zero_event_grab asked for */
void zero_event_init(struct task *task);
/* ask the event's Zero to throw its hand at pos. the throw starts on Zero's
 * second step after the call. returns TRUE, or FALSE when a throw was
 * already asked for since zero_event_init */
Bool zero_event_grab(NJS_POINT3 *pos);
/* show a question mark, or a flash of an idea, at pos. it draws on a
 * frame's first step only, for 59 steps, and its task ends on its 60th step */
void zero_show_hatena(NJS_POINT3 *pos);
void zero_show_hirame(NJS_POINT3 *pos);

#endif
