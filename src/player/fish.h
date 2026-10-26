/* fish: what the fish of Big's stages share, and the frog he fishes for */
#ifndef PLAYER_FISH_H
#define PLAYER_FISH_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a fish task's work->vars, which FISH_VARS casts in place */
struct fish_vars {
    NJS_ACTION *action;
    Float frame;
    Sint16 turn_steps; /* the steps left to its next turn */
    Sint16 kind;
};

#define FISH_VARS(work) ((struct fish_vars *)&(work)->vars)

/* the radius of the ball a fish sweeps through the world */
#define FISH_RADIUS 5.5F

/* the frog's kind. the fish of the stages have kinds from 1 */
#define FISH_FROG 0

/* a fish task's motion, which FISH_MOTION casts. its scale is in
 * work->scl.x, and its size class, 0 to 2, in work->scl.y */
struct fish_motion {
    /* the water's height, taken as it makes for the lure and as it bites */
    Float surface;
    NJS_VECTOR speed;
    Uint8 unk_10[12];
    struct angles aim; /* the heading it turns toward. only y is used */
    Uint8 unk_28[16];
    Float interest; /* how much the lure's twitches have drawn it */
    Float weight;
};

#define FISH_MOTION(task) ((struct fish_motion *)(task)->motion)

/* TRUE while point lies within 300 of player 0, at a bearing from him
 * within 30 degrees of his heading or exactly opposite it. the bearing is
 * in his heading's terms. otherwise the result is FALSE, and the fish's
 * aim is set. where the bearing less his heading is 30 to 180 degrees, the
 * aim is the bearing plus 100 degrees. where it is 180 to 330 degrees, the
 * aim is the bearing less 100 degrees. a point that is only too far away
 * sets the aim to his heading plus 180 degrees */
Bool fish_keep_ahead_of_player(struct task *task, NJS_POINT3 *point);
/* turn the fish a degree every third step, as turn_steps counts them
 * down */
void fish_wander(struct task *task);
/* aim the fish at the lure, and turn it a little under a degree toward
 * that aim. nothing while big_lure is null */
void fish_turn_to_lure(struct task *task);
/* swim the fish along its heading. it swims 0.2 a step while its action's
 * frame is between 25 and 45, its kick. otherwise it slows by 0.004 a step
 * while faster than 0.05, and swims 0.1 a step once it isn't. a sweep that
 * hits stops it where it is */
void fish_swim(struct task *task);
/* grow the fish's interest on each step the lure twitches within its
 * sight. a lure within 90 degrees of its front adds 2, and a lure anywhere
 * else adds 1. the sight is 100, 60 or 30 by the size class. TRUE on such
 * a step once interest is 3 or more. FALSE while big_lure is null or
 * FISHING_CAMERA is clear */
Bool fish_watch_lure(struct task *task);
/* TRUE once the fish is within 1 of the lure, with surface set to the
 * water's height. FALSE while big_lure is null */
Bool fish_try_bite(struct task *task);
/* TRUE while the fish holds the lure, as big_catch. with neither
 * FISHING_CAMERA nor FISHING_HOOKED set, no fish holds it. with no holder,
 * a fish within 100 of the lure takes it. a fish nearer the lure than its
 * holder takes it from the holder, unless a fish is hooked, landed or
 * lost. the holder keeps the lure while a fish is hooked, landed or lost,
 * or while it lies within 100, and otherwise lets it go. FALSE, and no
 * change, while big_lure is null */
Bool fish_claim_lure(struct task *task);
/* the frog's step, which the object lists of Big's four stages name */
void frog_step(struct task *task);
/* sweep a ball of radius from pos along speed, through world_objects as
 * the last collect or list found them. its up is the y axis, turned by ang
 * unless ang is null. TRUE at the first object it hits, with speed zeroed.
 * pos stays as it is */
Bool fish_sweep(NJS_POINT3 *pos, NJS_VECTOR *speed, struct angles *ang,
                Float radius);

#endif
