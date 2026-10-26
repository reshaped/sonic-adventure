/* Perfect Chaos's ray along node 15, with its ripples, sparks and flash */
#ifndef B_CHAOS7_RAY_H
#define B_CHAOS7_RAY_H

#include <ninja.h>

#include "system/task.h"

/* the ray's sound. the boss plays it once as a ray starts. the ray plays it
 * held, and stops it as it bursts */
#define SE_RAY 0x1b7

/* the textures of CHAOS7_0BREATH, which the ray and the breath draw with. the
 * fight's second part loads CHAOS7_0BREATH2ND in their place */
extern NJS_TEXLIST chaos7_breath_texlist;

struct ray_vars {
    Float length;
    Sint16 scroll;
    Uint8 unk_06[2];
    /* @hack type: the body's texture tick and frame, then the tip's, as an
     * array. SHC reaches the first through the array's address (ADD #16,R3 at
     * 0x0c919556). as four Sint8 fields, 139 of 252 words of ray_step
     * differ */
    Sint8 anim[4];
};

#define RAY_VARS(work) ((struct ray_vars *)&(work)->vars)

/* a ray's ang.z is a quarter turn further about z than its draw turns it. the
 * turn lays the y axis of its collision cylinder along the ray */
#define RAY_SHAPE_TURN NJM_DEG_ANG(90)

enum ray_mode {
    RAY_GROWING,
    RAY_HOLDING,
    RAY_BURSTING,
    RAY_THINNING,
    RAY_GONE
};

/* a ray's first step, for a task with a work and a motion. the ray runs along
 * the x axis of Chaos's node 15. it reaches as far as the length the boss sets
 * in RAY_VARS each step. its cylinder hurts players and objects. while the ray
 * holds, the cylinder spans 0.45 to 1.35 of that length out along the axis.
 * once the boss sets its mode to RAY_BURSTING, it scatters sparks along that
 * length and thins away. it then idles */
void chaos7_ray_start(struct task *task);
/* a ripple's first step, for a task with a work and a motion. the ripple
 * plays once, out along node 15's axis. its creator sets vars.b[4] to the
 * ripples that are to follow it. if that is above 0, the ripple starts the
 * next 21 steps after this one. the next has one fewer to follow */
void chaos7_ray_ripple_start(struct task *task);
/* the flash's first step, for a task with a work. the flash plays once on
 * Chaos's node 13 */
void chaos7_ray_flash_start(struct task *task);

#endif
