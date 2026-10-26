/* EV0024.PRS's wreckage: a piece that flies and spins, and can bounce,
 * smoke and cast a shadow */
#ifndef EV0024_WRECKAGE_H
#define EV0024_WRECKAGE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* bits of wreckage_params.flags */
#define WRECKAGE_SMOKES 0x1 /* a puff of smoke every interval + 1 steps */
#define WRECKAGE_BOUNCES 0x2 /* the piece bounces off the ground below it */
#define WRECKAGE_SHADOW 0x4 /* a shadow on the ground below the piece */

struct wreckage_params {
    Uint8 model; /* which of the 8 wreckage models */
    Uint8 flags;
    Sint16 interval;
    Sint16 life; /* the steps a piece moves before it is killed */
    Float bounce; /* the share of its fall speed a bounce keeps */
    Float shadow; /* the shadow's scale */
    /* each step, a piece turns about each axis by an angle picked at random
     * when it is made. the angle lies from 0 down to just above -2 times
     * spin. each bounce halves it */
    struct angles spin;
};

/* make a piece at pos, moving at speed, at scale. it copies *pos, *speed
 * and *params, and keeps none of them. when task_create fails, it makes no
 * piece. when heap_alloc fails, it stores the copy of *params through
 * null */
void wreckage_create(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale,
                     struct wreckage_params *params);

#endif
