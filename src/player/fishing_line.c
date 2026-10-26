/* the fishing line: a rope of points from the rod's tip to the lure */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/ground.h"
#include "player/fishing_line.h"
#include "system/task.h"
#include "system/util.h"

/* the distances from a neighbor past which a point is pulled toward it,
 * gently or hard */
struct line_limits {
    Float gentle;
    Float hard;
};

/* the share of the gap to a neighbor that a pull adds to a point's
 * acceleration */
#define PULL_GENTLE 0.08F
#define PULL_HARD 0.24F

/* what each step adds to a point's y acceleration */
#define GRAVITY (-0.05F)

/* the share of its speed a point loses a step. a point whose bit in
 * double_drag is set would lose twice that, but nothing sets one */
#define DRAG 0.1F

/* a point that a move leaves on the ground, or at most this far under it,
 * rises to this far above it */
#define LINE_ABOVE_GROUND 2.0F

/* a point's alpha is ALPHA_NEAR within about 11 of the camera, falls with
 * the squared distance past that, and is ALPHA_FAR from about 28 on. it is
 * ALPHA_NO_CAMERA while camera_work is null */
#define ALPHA_NEAR 0xe0
#define ALPHA_FAR 0x40
#define ALPHA_NO_CAMERA 0x80

static void fishing_line_pull(struct fishing_line *line,
                              struct line_limits *limits, int i);
static void fishing_line_move(struct fishing_line *line, int i);
static int fishing_line_alpha(struct fishing_line *line, int i);

void fishing_line_reset_ends(struct fishing_line *line, NJS_POINT3 *ends)
{
    fishing_line_reset(line, &ends[0], &ends[1]);
}

void fishing_line_reset(struct fishing_line *line, NJS_POINT3 *from,
                        NJS_POINT3 *to)
{
    NJS_VECTOR step;
    NJS_POINT3 point;
    int i;

    VECTOR_DIFF(&step, to, from);
    /* @bug arithmetic: 1 / (FISHING_LINE_POINTS - 1) divides integers and
     * comes to 0, so step is 0 and every point starts at from (FLDI0 FR4 at
     * 0x8c0f6a7a, FMUL FR4,FR3 at 0x8c0f6aa2) */
    /* NOLINTNEXTLINE(bugprone-integer-division) */
    VECTOR_SCALE(&step, &step, 1 / (FISHING_LINE_POINTS - 1));
    VECTOR_COPY(&point, from);
    for (i = 0; i < FISHING_LINE_POINTS; i++) {
        VECTOR_CLEAR(&line->accel[i]);
        VECTOR_CLEAR(&line->speed[i]);
        /* @hack redundant: the copy in a loop of one pass, which keeps SHC
         * from unrolling the point loop, whose test is CMP/GE R6,R5 at
         * 0x8c0f6b80; as util.h's VECTOR_COPY alone, the point loop unrolls
         * and 25 of 144 words differ */
        do {
            VECTOR_COPY(&line->pos[i], &point);
        } while (0);
        if (i != FISHING_LINE_POINTS - 1) {
            VECTOR_ADD(&point, &step);
        }
    }
}

void fishing_line_step(struct fishing_line *line, NJS_POINT3 *from,
                       NJS_POINT3 *to, enum fishing_line_mode mode)
{
    struct line_limits limits;
    int pass;
    int i;

    switch (mode) {
    case FISHING_LINE_SLACK:
        limits.gentle = 10.0F;
        limits.hard = 20.0F;
        break;
    case FISHING_LINE_TAUT:
    default:
        limits.gentle = 0.0F;
        limits.hard = 0.0F;
        break;
    }
    line->pos[0].x = from->x;
    line->pos[0].y = from->y;
    line->pos[0].z = from->z;
    line->pos[FISHING_LINE_POINTS - 1].x = to->x;
    line->pos[FISHING_LINE_POINTS - 1].y = to->y;
    line->pos[FISHING_LINE_POINTS - 1].z = to->z;
    /* @hack redundant: a loop of one pass, which SHC drops but for a test of
     * 1, the point loops' first index, ahead of each (MOV #1,R11 at
     * 0x8c0f6bfe, TST R11,R11 at 0x8c0f6c04 and 0x8c0f6c44); without it, 83
     * of 86 words differ */
    for (pass = 0; pass < 1; pass++) {
        for (i = 1; i < FISHING_LINE_POINTS - 1; i++) {
            fishing_line_pull(line, &limits, i);
        }
        for (i = 1; i < FISHING_LINE_POINTS - 1; i++) {
            fishing_line_move(line, i);
        }
    }
}

void fishing_line_draw(struct fishing_line *line)
{
    NJS_POINT3COL strip;
    NJS_COLOR colors[FISHING_LINE_POINTS];
    int i;

    strip.p = line->pos;
    strip.col = colors;
    strip.tex = NULL;
    strip.num = FISHING_LINE_POINTS;
    for (i = 0; i < FISHING_LINE_POINTS; i++) {
        colors[i].argb.a = fishing_line_alpha(line, i);
        colors[i].argb.r = 0xff;
        colors[i].argb.g = 0xff;
        colors[i].argb.b = 0xff;
    }
    njDrawLine3D(&strip, FISHING_LINE_POINTS - 1,
                 NJD_TRANSPARENT | NJD_DRAW_CONNECTED);
}

/* add to point i's acceleration a pull toward each neighbor further from it
 * than limits->gentle, and GRAVITY */
static void fishing_line_pull(struct fishing_line *line,
                              struct line_limits *limits, int i)
{
    NJS_VECTOR apart;
    NJS_VECTOR pull;
    NJS_POINT3 *pos = line->pos;
    NJS_VECTOR *accel = line->accel;
    Float squared;

    VECTOR_DIFF(&apart, &pos[i], &pos[i - 1]);
    squared = VECTOR_SQUARED(&apart);
    if (squared > limits->hard * limits->hard) {
        VECTOR_DIFF(&pull, &pos[i - 1], &pos[i]);
        VECTOR_SCALE(&pull, &pull, PULL_HARD);
        VECTOR_ADD(&accel[i], &pull);
    } else if (squared > limits->gentle * limits->gentle) {
        VECTOR_DIFF(&pull, &pos[i - 1], &pos[i]);
        VECTOR_SCALE(&pull, &pull, PULL_GENTLE);
        VECTOR_ADD(&accel[i], &pull);
    }
    VECTOR_DIFF(&apart, &pos[i], &pos[i + 1]);
    squared = VECTOR_SQUARED(&apart);
    if (squared > limits->hard * limits->hard) {
        VECTOR_DIFF(&pull, &pos[i + 1], &pos[i]);
        VECTOR_SCALE(&pull, &pull, PULL_HARD);
        VECTOR_ADD(&accel[i], &pull);
    } else if (squared > limits->gentle * limits->gentle) {
        VECTOR_DIFF(&pull, &pos[i + 1], &pos[i]);
        VECTOR_SCALE(&pull, &pull, PULL_GENTLE);
        VECTOR_ADD(&accel[i], &pull);
    }
    accel[i].y += GRAVITY;
}

/* add point i's acceleration to its speed, take off its drag, and move the
 * point by its speed. its acceleration then starts over from 0. a point the
 * move leaves on the ground, or at most LINE_ABOVE_GROUND under it, rises to
 * LINE_ABOVE_GROUND above it */
static void fishing_line_move(struct fishing_line *line, int i)
{
    struct angles slope;
    NJS_VECTOR drag;
    NJS_POINT3 *pos = &line->pos[i];
    NJS_VECTOR *speed = &line->speed[i];
    NJS_VECTOR *accel = &line->accel[i];
    Uint32 bit = 1 << i;
    Float ground;
    /* @hack spelling: 0 through a local, which loads it into FR4 in the
     * slot of the drag bit's test (FLDI0 FR4 at 0x8c0f7110); as the
     * literal, 106 of 181 words differ */
    Float zero = 0.0F;

    if (line->double_drag & bit) {
        VECTOR_ADD(speed, accel);
        VECTOR_SCALE(&drag, speed, 2 * DRAG);
        VECTOR_SUB(speed, &drag);
    } else {
        VECTOR_ADD(speed, accel);
        VECTOR_SCALE(&drag, speed, DRAG);
        VECTOR_SUB(speed, &drag);
    }
    VECTOR_ADD(pos, speed);
    accel->x = zero;
    accel->y = zero;
    accel->z = zero;
    ground = ground_below(pos->x, pos->y + LINE_ABOVE_GROUND, pos->z, &slope);
    if (pos->y <= ground) {
        pos->y = ground + LINE_ABOVE_GROUND;
    }
}

static int fishing_line_alpha(struct fishing_line *line, int i)
{
    NJS_POINT3 *points = line->pos;
    NJS_VECTOR apart;
    Float squared;
    int alpha;

    if (camera_work != NULL) {
        /* @hack spelling: apart written through its address, which SHC forms
         * in R2 and R3 (ADD #4,R2 at 0x8c0f72cc, ADD #4,R3 at 0x8c0f72d2);
         * written as apart.x, apart.y and apart.z, 76 of 94 words differ,
         * and as util.h's VECTOR_DIFF, 90 */
        (&apart)->x = points[i].x - camera_work->pos.x;
        (&apart)->y = points[i].y - camera_work->pos.y;
        (&apart)->z = points[i].z - camera_work->pos.z;
        squared = VECTOR_SQUARED(&apart);
        /* @hack spelling: the alpha taken from 0xff in a second statement,
         * which subtracts in R3 and copies to R4 (SUB R4,R3 and MOV R3,R4 at
         * 0x8c0f733a and 0x8c0f733c); in one statement, the difference lands
         * in R4 and 24 of 94 words differ */
        alpha = (int)(squared * 0.25F);
        alpha = 0xff - alpha;
        if (alpha > ALPHA_NEAR) {
            alpha = ALPHA_NEAR;
        }
        if (alpha < ALPHA_FAR) {
            alpha = ALPHA_FAR;
        }
        return alpha;
    }
    return ALPHA_NO_CAMERA;
}
