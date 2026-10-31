/* the marker Zero's laser leaves on player 0. it keeps the lock, the point
 * Zero aims its throw at */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_robo/beam.h"
#include "b_robo/marker.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the marker's modes. the laser's loss of the player, or a request, moves
 * it on from MARKER_LOCK and MARKER_WIND_UP */
enum marker_mode {
    MARKER_START, /* the marker's first step, at SIZE_START, in YELLOW */
    /* the lock follows player 0, and the marker shrinks onto it */
    MARKER_LOCK,
    MARKER_WIND_UP, /* the lock holds still, and the marker spins faster */
    MARKER_LOST, /* the marker waits LOST_STEPS */
    MARKER_SWELL /* the marker swells, then shrinks away */
};

/* what Zero asks of the marker. a request stands until the next
 * erobo_marker_create */
enum marker_request { REQUEST_NONE, REQUEST_LOSE, REQUEST_WIND_UP };

/* the marker's work->vars. work->scl.x is the marker's size, and
 * work->ang.x and work->ang.y are its turn */
struct marker_vars {
    Uint8 unk_00[8];
    Float grow; /* the size MARKER_SWELL adds in a step */
};

#define MARKER_VARS(work) ((struct marker_vars *)&(work)->vars)

/* the marker's colors, as NJS_COLOR's color, in ARGB */
#define YELLOW 0xffffff00
#define RED 0xa0ff0000
#define BLUE 0xa04040ff

/* the marker starts at SIZE_START. each step of MARKER_LOCK, it keeps
 * SHRINK of its size, down to SIZE_LOCKED. a wind-up request sets its size
 * to SIZE_WIND_UP as it moves to MARKER_WIND_UP */
#define SIZE_START 100.0F
#define SHRINK 0.8F
#define SIZE_LOCKED 8.0F
#define SIZE_WIND_UP 10.0F
/* the marker's turn a step about x and about y, and its faster turn in
 * MARKER_WIND_UP */
#define SPIN 0x200
#define SPIN_WIND_UP 0x600

#define LOST_STEPS 30

/* MARKER_SWELL's grow starts at SWELL_GROW, and changes by GROW_LOSS each
 * step. the marker's task ends once its size falls below SIZE_GONE */
#define SWELL_GROW 1.0F
#define GROW_LOSS (-0.1F)
#define SIZE_GONE 0.01F

/* the lock's height above player 0's position */
#define LOCK_HEIGHT 5.0F

#define MARKER_POINTS 5

static NJS_POINT3 marker_lock;
static int marker_request;

/* a tetrahedron about the lock. a strip of three triangles through these
 * points draws its base and two of its sides */
static NJS_POINT3 marker_points[MARKER_POINTS] = {
#include "b_robo/marker/marker_points.inc"
};
static NJS_COLOR marker_colors[MARKER_POINTS] = {
#include "b_robo/marker/marker_colors.inc"
};
static NJS_POINT3COL marker_faces = {
#include "b_robo/marker/marker_faces.inc"
};

static void marker_step(struct task *task);
static void marker_draw(struct task *task);

void erobo_marker_get_lock(NJS_POINT3 *lock)
{
    VECTOR_COPY(lock, &marker_lock);
}

void erobo_marker_wind_up(void)
{
    marker_request = REQUEST_WIND_UP;
}

void erobo_marker_lose(void)
{
    marker_request = REQUEST_LOSE;
}

void erobo_marker_create(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 4, marker_step)) != NULL) {
        NJS_POINT3 *pos = &player_work[0]->pos;

        VECTOR_COPY(&marker_lock, pos);
        marker_request = REQUEST_NONE;
        task->draw = marker_draw;
    }
}

static void marker_step(struct task *task)
{
    NJS_POINT3 *pos = &player_work[0]->pos;
    struct task_work *work = task->work;
    NJS_COLOR color;
    NJS_COLOR *entry;
    NJS_COLOR *end;

#ifdef GCC
    /* GCC can't see that every path sets color: the marker's mode holds only
     * the five the switch has cases for */
    color.color = YELLOW;
#endif
    switch (work->mode) {
    case MARKER_START:
        work->scl.x = SIZE_START;
        work->ang.x = 0;
        work->ang.y = NJM_DEG_ANG(120);
        color.color = YELLOW;
        work->mode = MARKER_LOCK;
        break;
    case MARKER_LOCK:
        /* @hack spelling: the lock written through its address, in a statement
         * for each member, which has SHC build its y's address in R1 ahead
         * of the copy of z (ADD #4,R1 at 0x0c905268); as VECTOR_COPY, 8 of
         * 194 words differ */
        (&marker_lock)->x = pos->x;
        (&marker_lock)->y = pos->y;
        (&marker_lock)->z = pos->z;
        marker_lock.y += LOCK_HEIGHT;
        if ((work->scl.x *= SHRINK) < SIZE_LOCKED) {
            work->scl.x = SIZE_LOCKED;
        }
        work->ang.x += SPIN;
        work->ang.y += SPIN;
        color.color = RED;
        /* the green is the turn about x shifted right 6. it rises 8 a step,
         * and wraps every 32 steps. over those 32 steps, the marker goes
         * from red toward yellow, then turns red again */
        color.argb.g = work->ang.x >> 6;
        if (marker_request == REQUEST_WIND_UP) {
            work->scl.x = SIZE_WIND_UP;
            work->mode = MARKER_WIND_UP;
        }
        if (marker_request == REQUEST_LOSE || !erobo_beam_reached()) {
            work->scl.x = SIZE_LOCKED;
            TASK_TIMER(work) = LOST_STEPS;
            work->mode = MARKER_LOST;
        }
        break;
    case MARKER_WIND_UP:
        work->ang.x += SPIN_WIND_UP;
        work->ang.y += SPIN_WIND_UP;
        color.color = RED;
        if (marker_request == REQUEST_LOSE || !erobo_beam_reached()) {
            TASK_TIMER(work) = LOST_STEPS;
            work->mode = MARKER_LOST;
        }
        break;
    case MARKER_LOST:
        if (--TASK_TIMER(work) <= 0) {
            MARKER_VARS(work)->grow = SWELL_GROW;
            work->mode = MARKER_SWELL;
        }
        color.color = BLUE;
        break;
    case MARKER_SWELL:
        MARKER_VARS(work)->grow += GROW_LOSS;
        color.color = BLUE;
        if ((work->scl.x += MARKER_VARS(work)->grow) < SIZE_GONE) {
            task_kill(task);
            return;
        }
        break;
    }
    /* @hack spelling: end set before entry, and entry from it, which loads
     * marker_colors' address once and copies it (MOV R5,R4 at 0x0c9053e2);
     * as a for loop from marker_colors to marker_colors + MARKER_POINTS, 20
     * of 194 words differ */
    end = marker_colors;
    entry = end;
    end += MARKER_POINTS;
    for (; entry < end; entry++) {
        entry->color = color.color;
    }
    marker_draw(task);
}

static void marker_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njPushMatrix(NULL);
        njTranslateV(NULL, &marker_lock);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        scale = work->scl.x;
        njScale(NULL, scale, scale, scale);
        njDrawTriangle3D(&marker_faces, 3,
                         NJD_TRANSPARENT | NJD_DRAW_CONNECTED);
        njPopMatrix(1);
    }
}
