/* the pirates O KAIZA, O KAIZB, O KAIZC and O KAIZS. each walks round an
 * ellipse until an attack hurts it, then tumbles and breaks apart */
#ifndef STG09_PIRATE_H
#define STG09_PIRATE_H
#include <ninja.h>

#include "system/task.h"
#include "system/util.h"

/* where a pirate is and how it moves. the object list's step allocates it,
 * and the destructor frees it. pos and speed are relative to the ellipse:
 * from origin, along axes turned by heading */
struct pirate_state {
    struct angles ang; /* the turn the tumble has given it, 0 while it walks */
    /* the turn the tumble adds to ang each frame. the drag and the bounces
     * scale only each angle's low halfword. a rotation reads an Angle mod
     * 0x10000 */
    struct angles spin;
    union point_floats pos; /* y is its height above the floor */
    union point_floats speed; /* its move each frame */
    /* the ellipse's center, where the placement put the pirate. its y is the
     * floor's height */
    NJS_POINT3 origin;
    Angle heading; /* the ellipse's turn about y: the placement's y angle */
};

/* a pirate's work->vars */
struct pirate_vars {
    /* the walk action's frame. the draw shows the model without the action
     * while it is 0 */
    Float frame;
    struct pirate_state *state;
    Sint16 phase; /* its angle round the ellipse */
    Sint16 facing; /* the y angle it walks toward, relative to heading */
};

#define PIRATE_VARS(work) ((struct pirate_vars *)&(work)->vars)

/* scale a spin angle's low halfword by scale, keeping its high one */
#define PIRATE_SCALE_SPIN(angle, scale)                                        \
    do {                                                                       \
        Sint16 *low = (Sint16 *)&(angle);                                      \
        Float scaled = *low;                                                   \
        scaled *= (scale);                                                     \
        *low = scaled;                                                         \
    } while (0)

/* a pirate's work->mode: walking round the ellipse, or tumbling after a
 * hit */
#define PIRATE_WALKING 0
#define PIRATE_TUMBLING 1

/* the object list's steps. the placement's scl.x is the angle the pirate
 * walks round the ellipse each frame, in degrees. scl.y is the ellipse's
 * radius along z, and scl.z + 1 stretches it along x */
void pirate_a_step(struct task *task);
void pirate_b_step(struct task *task);
void pirate_c_step(struct task *task);
void pirate_s_step(struct task *task);

#endif
