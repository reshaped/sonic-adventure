/* the platforms that break into pieces as their targets are hit. TALAP_0
 * breaks into 34, TALAP_1 and CONNECT_0 into nine, and EDGE_1 into ten. the
 * cylinders' pieces are struct platform_piece too */
#ifndef STG06_PLATFORM_H
#define STG06_PLATFORM_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"
#include "system/util.h"

struct sky_deck_target;

/* the square of the distance from the camera or player 0 within which a
 * platform stays. a cylinder takes it too */
#define PLATFORM_RANGE ((2100.0F * 2100.0F) + 100.0F)

/* each step, a flying piece's speed gains PIECE_GRAVITY along y. a
 * platform's piece also keeps PIECE_DRAG of its speed and of its spin's low
 * half. a piece is gone once it is farther than PIECE_GONE from its platform
 * or cylinder */
#define PIECE_GRAVITY (-0.07F)
#define PIECE_DRAG 0.98F
#define PIECE_GONE 300.0F

/* a break flings each piece at PIECE_SPEED, plus a random part up to
 * PIECE_SPEED_SPREAD. each piece also takes PIECE_PUSH of the hitter's speed,
 * turned into the platform's or cylinder's frame. each of a piece's spins
 * starts within PIECE_SPIN of 0. the break then throws BREAK_SHARDS shards */
#define PIECE_SPEED 1.5F
#define PIECE_SPEED_SPREAD 0.3F
#define PIECE_PUSH 0.2F
#define PIECE_SPIN 1024
#define BREAK_SHARDS 10

/* a piece of a broken platform or cylinder. pos and speed are in the platform's
 * frame. pos starts where the piece sits in the whole platform. a cylinder's
 * piece starts there turned by CYLINDER_YAW */
struct platform_piece {
    Bool gone; /* TRUE once the piece is past PIECE_GONE */
    union point_floats speed;
    union point_floats pos;
    /* the turns added to ang_x and ang_z each step. the step scales each spin's
     * low half, half[0], by the drag, and leaves its high half. a break starts
     * a spin above -0x10000, so a spin below 0 has a high half of -1. such a
     * spin settles at -0x10000, a whole turn, and one above 0 at 0 */
    union angle_halves spin_x;
    union angle_halves spin_z;
    Angle ang_x;
    Angle ang_z;
};

struct platform_vars {
    NJS_OBJECT *world; /* its shape in the world, until its pieces fly */
    struct sky_deck_target *target;
    /* from the heap, from the hit until every piece is gone. null outside
     * that */
    struct platform_piece *pieces;
};

#define PLATFORM_VARS(work) ((struct platform_vars *)&(work)->vars)

/* &piece[i], with the byte offset cut to a Sint16 as the image adds it. no
 * platform's offset needs more than 16 bits */
#define PLATFORM_PIECE(piece, i)                                               \
    ((struct platform_piece *)((char *)(piece) +                               \
                               (Sint16)((i) * sizeof(struct platform_piece))))

/* a platform's work->mode. the step sets PLATFORM_GONE once every piece is
 * gone */
enum platform_mode { PLATFORM_WHOLE, PLATFORM_GONE };

#endif
