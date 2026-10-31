/* rockets: one rises out of its launcher, and a player who takes its grip
 * rides it toward a target, along or up, until the player lets go */
#ifndef OBJECT_ROCKET_H
#define OBJECT_ROCKET_H

#include <ninja.h>
#include <sg_xpt.h>

#include "effect/pools.h"
#include "system/task.h"

#define ROCKET_COUNT 10

/* bits of a rocket's state */
#define ROCKET_LET_GO 0x4 /* its rider may let go */
/* set by another object, to make the rider let go */
#define ROCKET_RELEASE 0x8
#define ROCKET_UP 0x10 /* it flies up, not along */
#define ROCKET_FLYING 0x20 /* it has taken off */

/* a rocket's own layout of task_work */
struct rocket_work {
    Sint8 mode;
    Sint8 slot; /* in rockets */
    Uint8 unk_02;
    Uint8 state;
    Sint16 flags;
    Uint16 timer;
    NJS_ACTION *action;
    Float frame;
    struct task *rider; /* the player's task */
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 target;
    struct collision_work *collision;
    struct event_work *event;
};

/* the rockets that fly along, each in its slot, and NULL in a free slot. a
 * rocket that flies up takes no slot, and as it ends, it clears slot 0 */
extern struct rocket_work *rockets[ROCKET_COUNT];

/* the smoke behind a flying rocket */
extern struct particle_pool_spec rocket_thrust_pool;

/* the steps of the launchers ROCKET H, HS, V and VS, which the stages
 * place */
void rocket_h_step(struct task *task);
void rocket_hs_step(struct task *task);
void rocket_v_step(struct task *task);
void rocket_vs_step(struct task *task);
/* set every slot of rockets to NULL, for a new stage */
void rocket_reset(void);
/* the texlist of a rocket's smoke, EFF_REGULAR's roketthrust01 to 08. a
 * particle animation in ADV03's generated data names it too */
extern NJS_TEXLIST rocket_thrust_texlist;

#endif
