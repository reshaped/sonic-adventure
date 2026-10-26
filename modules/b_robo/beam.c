/* Zero's laser. each step, it keeps a segment from its origin to where the
 * ray ends. it draws as a strip through the last few segments, fading with
 * age. where two segments in a row end on the floor, a scorch joins their
 * ends */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_robo/beam.h"
#include "b_robo/erobo.h"
#include "collision/polygon.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* @hack spelling: the copy as three statements, one for each component,
 * which has SHC set a stack address in R2 ahead of beam_cast's stores (MOV
 * R15,R2 at 0x0c903634); as VECTOR_COPY, one comma expression, 35 of the
 * 289 words of beam_start and beam_cast differ */
#define BEAM_COPY(out, v)                                                      \
    {                                                                          \
        (out)->x = (v)->x;                                                     \
        (out)->y = (v)->y;                                                     \
        (out)->z = (v)->z;                                                     \
    }
/* @hack spelling: the sum as three statements, one for each component, which
 * puts the z offset in R0 (MOV #8,R0 at 0x0c90352a); as VECTOR_SUM, one
 * comma expression, 30 of beam_track's 195 words differ */
#define BEAM_SUM(out, a, b)                                                    \
    {                                                                          \
        (out)->x = (a)->x + (b)->x;                                            \
        (out)->y = (a)->y + (b)->y;                                            \
        (out)->z = (a)->z + (b)->z;                                            \
    }
/* @hack spelling: the components zeroed by one chained assignment, which
 * loads the 0 into FR3 after the copy ahead of it (FLDI0 at 0x0c9032fc); as
 * VECTOR_CLEAR's three stores, SHC loads it first, into FR4, and 57 of the
 * 269 words of beam_start and beam_track differ */
#define BEAM_CLEAR(v)                                                          \
    {                                                                          \
        (v)->x = (v)->y = (v)->z = 0.0F;                                       \
    }

/* the segments kept: the newest, and the DRAWN_MAX before it that the
 * strip can reach back to */
#define SEGMENTS 9
#define DRAWN_MAX 8
/* the strip's points: both ends of each segment */
#define SWEEP_POINTS (SEGMENTS * 2)
/* the strip's color, yellow. the alpha at the origin's end stays 0 */
#define SWEEP_COLOR 0x00c0c000

/* a shot starts LENGTH_START long, with its aim AIM_START ahead of the
 * origin. it grows a unit a step to LENGTH_MAX */
#define LENGTH_START 1.0F
#define AIM_START 2.0F
#define LENGTH_MAX 150.0F
/* beam_reaches_player tests points PROBE_STEP apart along the laser. a
 * point within PROBE_NEAR of the player's middle reaches the player */
#define PROBE_STEP 4.0F
#define PROBE_NEAR 4.0F
/* the player's middle, this far above its position */
#define PLAYER_MIDDLE 5.0F
/* the drift turns toward the player every TURN_STEPS to 2 * TURN_STEPS - 1
 * steps. it stops instead when the aim lies within DRIFT_NEAR of the
 * player's middle */
#define TURN_STEPS 17
#define DRIFT_NEAR 0.1F
/* how much each axis of the drift leans toward the player, each step
 * between its turns */
#define DRIFT_LEAN 0.01F
/* how many steps a scorch lasts */
#define SCORCH_STEPS 45

/* in beam_flags */
#define BEAM_REACHED 0x1 /* the laser has reached the player */
#define BEAM_BLOCKED 0x2 /* it can't, until it fires again */

/* the laser as one step left it, from its origin to where the ray ended */
struct beam_segment {
    NJS_POINT3 from;
    NJS_POINT3 to;
    Bool hit; /* it ends on the floor */
};

static NJS_POINT3 beam_points[2];
static NJS_POINT3 sweep_points[SWEEP_POINTS];
static NJS_COLOR sweep_colors[SWEEP_POINTS];
static enum erobo_beam_mode beam_mode;
/* a ring of the last SEGMENTS segments, the newest at beam_newest */
static struct beam_segment beam_segments[SEGMENTS];
/* steps until the drift turns toward the player again */
static int beam_turn_timer;
static int beam_newest;
static Float beam_length;
/* how many segments behind the newest the strip draws, up to DRAWN_MAX */
static int beam_drawn;
static NJS_POINT3 *beam_origin;
static NJS_VECTOR *beam_velocity;
static Angle *beam_angle;
static NJS_POINT3 beam_aim;
/* from the origin toward the aim, a unit vector */
static NJS_VECTOR beam_dir;
static NJS_VECTOR beam_drift;
static int beam_flags;
static NJS_POINT3 scorch_points[2];

/* the alpha of each drawn segment's far end. the oldest segment drawn
 * takes the last, and each newer one the one before */
static Uint8 sweep_alpha[SEGMENTS] = {175, 150, 125, 100, 75, 50, 25, 0, 0};
static NJS_COLOR beam_colors[2] = {
#include "b_robo/beam/beam_colors.inc"
};
static NJS_POINT3COL beam_line = {
#include "b_robo/beam/beam_line.inc"
};
static NJS_POINT3COL sweep_strip = {
#include "b_robo/beam/sweep_strip.inc"
};
static NJS_COLOR scorch_colors[2] = {
#include "b_robo/beam/scorch_colors.inc"
};
static NJS_POINT3COL scorch_line = {
#include "b_robo/beam/scorch_line.inc"
};

static void scorch_create(NJS_POINT3 *from, NJS_POINT3 *to);

void erobo_beam_init(NJS_POINT3 *origin, NJS_VECTOR *velocity, Angle *angle)
{
    int i;

    beam_mode = EROBO_BEAM_RESET;
    beam_newest = 0;
    beam_length = 0.0F;
    beam_drawn = 0;
    beam_origin = origin;
    beam_velocity = velocity;
    beam_angle = angle;
    for (i = 0; i < SWEEP_POINTS; i++) {
        sweep_colors[i].color = SWEEP_COLOR;
    }
}

void erobo_beam_detach(void)
{
    beam_origin = NULL;
}

void erobo_beam_set_mode(enum erobo_beam_mode mode)
{
    beam_mode = mode;
}

/* whether a point between the laser's origin and its aim lies within
 * PROBE_NEAR of the player's middle */
static Bool beam_reaches_player(void)
{
    NJS_POINT3 point;
    NJS_VECTOR step;
    NJS_POINT3 player;
    Float left;

    left = njDistanceP2P(beam_origin, &beam_aim);
    VECTOR_COPY(&point, beam_origin);
    VECTOR_COPY(&step, &beam_dir);
    step.x *= PROBE_STEP;
    step.y *= PROBE_STEP;
    step.z *= PROBE_STEP;
    VECTOR_COPY(&player, &player_work[0]->pos);
    player.y += PLAYER_MIDDLE;
    /* @hack spelling: a do-while inside an if in place of a while, which loads
     * point_distance_squared's address into R14 (0x0c903186) after the test at
     * 0x0c903180; as a while, SHC hoists the load above the test and 15 of
     * 136 words differ */
    if (left > 0.0F) {
        do {
            if (point_distance_squared(&player, &point) <
                PROBE_NEAR * PROBE_NEAR) {
                return TRUE;
            }
            left -= PROBE_STEP;
            point.x += step.x;
            point.y += step.y;
            point.z += step.z;
        } while (left > 0.0F);
    }
    return FALSE;
}

Bool erobo_beam_reached(void)
{
    int flags = beam_flags;

    if (flags & BEAM_REACHED) {
        return TRUE;
    }
    if (!(flags & BEAM_BLOCKED)) {
        if (beam_reaches_player()) {
            beam_flags |= BEAM_REACHED;
            return TRUE;
        }
    }
    return FALSE;
}

void erobo_beam_block(void)
{
    /* @hack spelling: a store of 0 ahead of the OR, which SHC drops, but
     * keeps the MOV #0,R3 it loads for it (0x0c903286); stored as
     * BEAM_BLOCKED alone, 5 of 5 words differ */
    beam_flags = 0;
    beam_flags |= BEAM_BLOCKED;
}

/* start a new shot from the origin. no segment kept counts as ending on
 * the floor */
static void beam_start(void)
{
    Float sine;
    Float cosine;
    struct beam_segment *segment;
    int count;

    beam_newest = 0;
    beam_drawn = 0;
    beam_length = LENGTH_START;
    beam_turn_timer = -1;
    beam_flags = 0;
    BEAM_COPY(&beam_aim, beam_origin);
    BEAM_CLEAR(&beam_drift);
    _builtin_fsca(-0x4000 - *beam_angle, &sine, &cosine);
    beam_aim.x += sine * AIM_START;
    beam_aim.z += cosine * AIM_START;
    segment = beam_segments;
    count = SEGMENTS;
    while (count--) {
        segment->hit = FALSE;
        segment++;
    }
}

/* move the aim by the velocity and the drift. when beam_turn_timer runs
 * out, the drift turns to a unit vector toward the player's middle. in the
 * steps between, the drift leans toward the player */
static void beam_track(void)
{
    NJS_POINT3 *pos;
    NJS_VECTOR toward;

    pos = &player_work[0]->pos;
    if (--beam_turn_timer <= 0) {
        beam_turn_timer =
            (int)(rand() * (TURN_STEPS * RAND_SCALE)) + TURN_STEPS;
        toward.x = pos->x - beam_aim.x;
        toward.y = pos->y - beam_aim.y + PLAYER_MIDDLE;
        toward.z = pos->z - beam_aim.z;
        /* vector_unit returns the reciprocal of the length */
        if (vector_unit(&toward, &beam_drift) > 1.0F / DRIFT_NEAR) {
            BEAM_CLEAR(&beam_drift);
        }
    } else {
        beam_drift.x += beam_aim.x < pos->x ? DRIFT_LEAN : -DRIFT_LEAN;
        beam_drift.x = CLAMP(beam_drift.x, -1.0F, 1.0F);
        beam_drift.y += beam_aim.y < pos->y ? DRIFT_LEAN : -DRIFT_LEAN;
        beam_drift.y = CLAMP(beam_drift.y, -1.0F, 1.0F);
        beam_drift.z += beam_aim.z < pos->z ? DRIFT_LEAN : -DRIFT_LEAN;
        beam_drift.z = CLAMP(beam_drift.z, -1.0F, 1.0F);
    }
    BEAM_SUM(&beam_aim, &beam_aim, beam_velocity);
    BEAM_SUM(&beam_aim, &beam_aim, &beam_drift);
}

/* move beam_newest on round the ring, and draw one segment more, up to
 * DRAWN_MAX */
static void beam_advance(void)
{
    if (++beam_newest >= SEGMENTS) {
        beam_newest = 0;
    }
    if (++beam_drawn >= DRAWN_MAX) {
        beam_drawn = DRAWN_MAX;
    }
}

/* cast the newest segment from the origin toward the aim, and set beam_dir.
 * the segment ends where the ray meets the floor, unless it is a shot's
 * first. otherwise it runs the laser's whole length, through any wall. a
 * segment that ends on the floor right after another scorches the floor
 * between their ends */
static void beam_cast(void)
{
    NJS_VECTOR dir;
    struct ray_query query;
    struct beam_segment *segment;
    struct beam_segment *previous;

    segment = &beam_segments[beam_newest];
    VECTOR_DIFF(&dir, &beam_aim, beam_origin);
    vector_unit(&dir, &dir);
    BEAM_COPY(&beam_dir, &dir);
    VECTOR_COPY(&segment->from, beam_origin);
    query.length = beam_length;
    VECTOR_COPY(&query.origin, beam_origin);
    VECTOR_COPY(&query.dir, &dir);
    if (polygon_cast_ray(&query) == TRUE) {
        VECTOR_COPY(&segment->to, &query.hit);
        segment->hit = TRUE;
        if (query.hit.y <= DECK_FLOOR_Y && beam_drawn > 0) {
            previous = segment - 1;
            if (previous < beam_segments) {
                previous = &beam_segments[SEGMENTS - 1];
            }
            if (previous->hit == TRUE) {
                scorch_create(&previous->to, &segment->to);
            }
            return;
        }
        /* @hack redundant: a goto to the next statement, which SHC drops, but
         * which still has the code below put a stack address in R2 first
         * (MOV R15,R2 at 0x0c903740); without it, 13 of 215 words differ */
        goto miss;
    }
miss:
    VECTOR_SCALE(&dir, &dir, beam_length);
    VECTOR_SUM(&segment->to, beam_origin, &dir);
    segment->hit = FALSE;
}

/* draw the strip back by a segment. returns TRUE once none is left */
static Bool beam_retract(void)
{
    if (--beam_drawn < 0) {
        beam_drawn = 0;
        return TRUE;
    }
    return FALSE;
}

void erobo_beam_step(void)
{
    switch (beam_mode) {
    case EROBO_BEAM_RESET:
        beam_mode = EROBO_BEAM_IDLE;
        break;
    case EROBO_BEAM_FIRE:
        beam_start();
        beam_cast();
        beam_mode = EROBO_BEAM_ON;
        break;
    case EROBO_BEAM_STOP:
        if (beam_retract() == TRUE) {
            beam_mode = EROBO_BEAM_IDLE;
        }
        break;
    case EROBO_BEAM_IDLE:
        break;
    case EROBO_BEAM_ON:
        /* @hack spelling: the sum tested as it is stored, which puts the
         * switch's 3 in R5 (MOV #3,R5 at 0x0c9037c4); as a statement and
         * then a test, R6 takes it and 13 of 62 words differ */
        if ((beam_length += 1.0F) > LENGTH_MAX) {
            beam_length = LENGTH_MAX;
        }
        beam_track();
        beam_advance();
        beam_cast();
        break;
    }
}

/* on a frame's first step, draw the newest segment as a line while the
 * laser is on. then draw the strip from it back through beam_drawn
 * segments, each fainter than the one before */
void erobo_beam_draw(void)
{
    struct beam_segment *segment;
    Uint8 *alpha;
    NJS_POINT3 *point;
    NJS_COLOR *color;
    Uint8 fade;
    int count;
    int i;

    if (!frame_step) {
        segment = &beam_segments[beam_newest];
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        if (beam_mode == EROBO_BEAM_ON) {
            VECTOR_COPY(&beam_points[0], &segment->from);
            VECTOR_COPY(&beam_points[1], &segment->to);
            njDrawLine3D(&beam_line, 1, NJD_TRANSPARENT);
        }
        /* @hack spelling: count and i both set in the test, which has the
         * function save R12 and R10 (MOV.L R12,@-R15 at 0x0c903864); as
         * statements ahead of the test, it saves R12 and R11, and 124 of 158
         * words differ */
        if ((i = count = beam_drawn) > 0) {
            alpha = &sweep_alpha[DRAWN_MAX - count];
            point = sweep_points;
            color = sweep_colors;
            fade = *alpha++;
            color++;
            color->argb.a = fade;
            color++;
            VECTOR_COPY(point, &segment->from);
            point++;
            VECTOR_COPY(point, &segment->to);
            point++;
            while (i--) {
                segment--;
                if (segment < beam_segments) {
                    segment = &beam_segments[SEGMENTS - 1];
                }
                fade = *alpha++;
                color++;
                color->argb.a = fade;
                color++;
                VECTOR_COPY(point, &segment->from);
                point++;
                VECTOR_COPY(point, &segment->to);
                point++;
            }
            sweep_strip.num = (beam_drawn + 1) * 2;
            njDrawTriangle3D(&sweep_strip, beam_drawn * 2,
                             NJD_TRANSPARENT | NJD_FILL | NJD_DRAW_CONNECTED);
        }
    }
}

/* on a frame's first step, draw the scorch from pos to scl. it fades out
 * as its timer runs to SCORCH_STEPS */
static void scorch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float fade;

    if (frame_step) {
        return;
    }
    fade = (SCORCH_STEPS - TASK_TIMER(work)) * (1.0F / SCORCH_STEPS);
    scorch_colors[0].argb.a = fade * 255.0F;
    fade -= 1.0F / SCORCH_STEPS;
    scorch_colors[1].argb.a = fade * 255.0F;
    VECTOR_COPY(&scorch_points[0], &work->pos);
    VECTOR_COPY(&scorch_points[1], &work->scl);
    njDrawLine3D(&scorch_line, 1, NJD_TRANSPARENT);
    placement_try_leave(task);
}

static void scorch_step(struct task *task)
{
    struct task_work *work = task->work;

    if (++TASK_TIMER(work) >= SCORCH_STEPS) {
        task_kill(task);
        return;
    }
    scorch_draw(task);
    placement_try_leave(task);
}

static void scorch_create(NJS_POINT3 *from, NJS_POINT3 *to)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 4, scorch_step)) != NULL) {
        work = task->work;
        VECTOR_COPY(&work->pos, from);
        VECTOR_COPY(&work->scl, to);
        work->timer = 0;
        task->draw = scorch_draw;
    }
}
