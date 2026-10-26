/* the cylinders CYL_0, CYL_1 and CYL_2. a cylinder takes hits through its
 * target, its entry in sky_deck_targets. the first hit sets it puffing dust.
 * the second breaks it into CYLINDER_PIECES pieces. once broken, it hits one
 * TARGET_TALAP_1 platform near it each step. then its pieces fly apart and fall
 * away */
#ifndef STG06_CYLINDER_H
#define STG06_CYLINDER_H
#include <ninja.h>
#include <sg_xpt.h>

#include "stg06/platform.h"
#include "system/task.h"

struct sky_deck_target;

#define CYLINDER_PIECES 19

/* the share of its speed and of its spin's low half a piece keeps each step */
#define CYLINDER_DRAG 0.99F

/* the box a broken cylinder looks for a platform to hit in. it reaches
 * HIT_REACH from the cylinder along x and along z, and HIT_HEIGHT along y */
#define HIT_REACH 100.0F
#define HIT_HEIGHT 40.0F

/* a hit cylinder's dust starts DUST_OUT from its axis and DUST_RISE above its
 * position. a puff turns DUST_SIDE degrees either way, or not at all, at
 * random, and an odd draw turns it DUST_NUDGE degrees more. DUST_SIDE's 1820
 * degrees are five whole turns and 20 degrees, so it turns a puff 20 degrees */
#define DUST_OUT 110.0F
#define DUST_RISE 6.0F
#define DUST_SIDE 1820.0F
#define DUST_NUDGE 2.0F

/* a broken cylinder's pieces, which its target's hit allocates from the heap */
struct cylinder_pieces {
    /* TRUE from the break until the step finds no platform to hit, or a hit
     * returns FALSE. the pieces don't move while it's TRUE */
    Bool hitting;
    struct platform_piece pieces[CYLINDER_PIECES];
};

struct cylinder_vars {
    NJS_OBJECT *world; /* its shape in the world, until its pieces fly */
    struct sky_deck_target *target;
    struct cylinder_pieces *pieces;
};

#define CYLINDER_VARS(work) ((struct cylinder_vars *)&(work)->vars)

/* the bearing the latest hit came from, less the cylinder's yaw. a hit with no
 * speed sets 0. it is an Angle kept in the word of scl.z, a Float */
#define CYLINDER_YAW(work) (*(Angle *)&(work)->scl.z)

/* a cylinder's mode. the first hit sets CYLINDER_HIT, and the step sets
 * CYLINDER_GONE once every piece is gone */
enum cylinder_mode { CYLINDER_WHOLE, CYLINDER_HIT, CYLINDER_GONE };

#endif
