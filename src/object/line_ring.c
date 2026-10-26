/* the line ring, O LINE RING: once its switch is on, it drops rings one by
 * one along a straight line */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/sparkle.h"
#include "object/line_ring.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/switch.h"
#include "system/task.h"

/* the most rings it drops */
#define RING_COUNT 30

/* the timer counts steps from 0. a ring drops when it passes RING_STEPS, so
 * one drops every RING_STEPS + 1 steps. once all have dropped, the rings
 * left go when it passes LIE_STEPS */
#define RING_STEPS 3
#define LIE_STEPS 600

/* the space between rings is scl.z plus SPACE_BASE, and at least
 * SPACE_LEAST */
#define SPACE_BASE 20.0F
#define SPACE_LEAST 2.0F

/* a line ring's vars. its placement sets its angles and scale. the low byte
 * of ang.x is its switch's id, and ang.y the way the line runs. scl.x and
 * scl.y are the line's length across and up, and scl.z adds to the space
 * between rings */
struct line_ring_vars {
    Uint8 unk_00[4];
    Sint32 count; /* the rings on the line, from 1 to RING_COUNT */
    Sint32 dropped; /* the rings dropped so far */
};

#define VARS(work) ((struct line_ring_vars *)&(work)->vars)

/* the line ring's own record, in its task's motion */
struct line_ring_motion {
    Uint8 unk_00[4];
    NJS_VECTOR step; /* from one ring to the next */
    NJS_POINT3 next; /* where the next ring drops */
};

enum line_ring_mode {
    LINE_RING_WAIT, /* for its switch */
    LINE_RING_DROP,
    LINE_RING_LIE, /* the rings lie until they're taken, or time runs out */
    LINE_RING_RESET
};

static void line_ring_step(struct task *task);
static void line_ring_destroy(struct task *task);
static void line_ring_measure(struct task *task);
static void line_ring_draw(struct task *task);

void line_ring_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.z = 0;
    work->mode = LINE_RING_WAIT;
    TASK_TIMER(work) = 0;
    /* @hack redundant: mode stored again, as the image stores it twice
     * (MOV.B at 0x8c06518e and 0x8c065192); stored once, 10 of 19 words
     * differ */
    work->mode = LINE_RING_WAIT;
    line_ring_measure(task);
    task->step = line_ring_step;
    task->destructor = line_ring_destroy;
}

static void line_ring_step(struct task *task)
{
    struct task_work *work = task->work;
    struct line_ring_motion *motion = (struct line_ring_motion *)task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        line_ring_measure(task);
        line_ring_draw(task);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<- SWITCH ID");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> X LENGTH");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> Y LENGTH");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> RING INTERVAL");
    } else if (edit_mode) {
        line_ring_draw(task);
    }
    switch (work->mode) {
    case LINE_RING_WAIT:
        if (switch_is_on(work->ang.x & 0xff)) {
            Float *next;
            Float *pos;

            work->mode = LINE_RING_DROP;
            VARS(work)->dropped = 0;
            /* @hack spelling: pos copied to next through two Float walks, which
             * load with @R4+ and step the stores with ADD #4,R5; through the
             * fields, each takes MOV #n,R0 and @(R0,Rn), and as byte cursors,
             * 4 of 212 words differ */
            next = &motion->next.x;
            pos = &work->pos.x;
            *next++ = *pos++;
            *next++ = *pos++;
            *next = *pos;
        }
        break;
    case LINE_RING_DROP:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > RING_STEPS) {
            TASK_TIMER(work) = 0;
            /* @hack spelling: dropped stepped by assignment, which keeps the
             * motion in R12 (MOV.L @(36,R13),R12 at 0x8c0651c0); as ++, R11
             * and R12 trade, so do FR14 and FR15, and 37 of 212 words
             * differ */
            VARS(work)->dropped = VARS(work)->dropped + 1;
            if (VARS(work)->dropped > VARS(work)->count) {
                work->mode = LINE_RING_LIE;
                TASK_TIMER(work) = 0;
            } else {
                struct task *child;
                Float dx;
                Float dy;
                Float dz;
                Float x;
                Float y;
                Float z;
                char *step;
                char *next;

                /* @hack spelling: next read, passed on and written back through
                 * a byte cursor; through the members, the function runs 32
                 * bytes long and 87 of 212 words differ */
                next = (char *)&motion->next;
                x = *(Float *)next;
                /* @hack spelling: next stepped to y inside y's read, and to z
                 * by a statement of its own, which gives y FR14 and z FR15;
                 * stepped the same way both times, y and z trade and 12 of
                 * 212 words differ */
                y = *(Float *)(next += sizeof(Float));
                next += sizeof(Float);
                z = *(Float *)next;
                next -= 2 * sizeof(Float);
                /* @hack spelling: step read through a byte cursor; through the
                 * members, the function runs 32 bytes long and 104 of 212
                 * words differ */
                step = (char *)&motion->step;
                dx = *(Float *)step;
                step += sizeof(Float);
                dy = *(Float *)step;
                step += sizeof(Float);
                dz = *(Float *)step;
                /* next moves on by step only when a ring is made */
                child = task_create_child(TASK_WORK, ring_step, task);
                if (child != NULL) {
                    /* @hack spelling: the ring's pos written through a byte
                     * cursor; through the members, the function runs 32
                     * bytes long and 83 of 212 words differ */
                    char *ring = (char *)&child->work->pos;

                    *(Float *)ring = x;
                    ring += sizeof(Float);
                    *(Float *)ring = y;
                    ring += sizeof(Float);
                    *(Float *)ring = z;
                    x += dx;
                    y += dy;
                    z += dz;
                }
                {
                    NJS_VECTOR speed = {0.0F, 0.05F, 0.0F};

                    sparkle_create((NJS_POINT3 *)next, &speed, 0.8F,
                                   NJM_DEG_ANG(5));
                }
                *(Float *)next = x;
                next += sizeof(Float);
                *(Float *)next = y;
                next += sizeof(Float);
                *(Float *)next = z;
            }
        }
        task_step_children(task);
        break;
    case LINE_RING_LIE:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > LIE_STEPS || task->child == NULL) {
            work->mode = LINE_RING_RESET;
            TASK_TIMER(work) = 0;
        }
        task_step_children(task);
        break;
    case LINE_RING_RESET:
        task_destroy_children(task);
        work->mode = LINE_RING_WAIT;
        break;
    }
}

static void line_ring_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* set the step from ring to ring, and the count of rings on the line, from
 * the scale. the count is at most RING_COUNT */
static void line_ring_measure(struct task *task)
{
    struct task_work *work = task->work;
    struct line_ring_motion *motion = (struct line_ring_motion *)task->motion;
    NJS_VECTOR way;
    /* @hack spelling: step written through a byte cursor, ADD #4 between the
     * stores; through the members, the function runs 6 bytes short and 72 of
     * 93 words differ */
    char *step;
    /* @hack spelling: the scale read through a byte cursor, for the image's
     * @Rn+ loads; through the members, the function runs 2 bytes long and 84 of
     * 93 words differ */
    char *scl = (char *)&work->scl;
    Float across;
    Float up;
    Float space;
    Angle ang;
    Float length;

    across = *(Float *)scl;
    scl += sizeof(Float);
    up = *(Float *)scl;
    scl += sizeof(Float);
    space = *(Float *)scl + SPACE_BASE;
    if (space < SPACE_LEAST) {
        space = SPACE_LEAST;
    }
    ang = work->ang.y;
    way.x = across * njCos(ang);
    way.y = up;
    way.z = across * njSin(ang);
    njUnitVector(&way);
    step = (char *)&motion->step;
    *(Float *)step = way.x * space;
    step += sizeof(Float);
    *(Float *)step = way.y * space;
    step += sizeof(Float);
    *(Float *)step = way.z * space;
    length = njSqrt((across * across) + (up * up));
    if (space <= 0.0F) {
        VARS(work)->count = 1;
    } else {
        VARS(work)->count = (Uint16)(length / space) + 1;
        if (VARS(work)->count >= RING_COUNT) {
            VARS(work)->count = RING_COUNT;
        }
    }
}

/* draw a marker where each ring will drop, for the set editor */
static void line_ring_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct line_ring_motion *motion = (struct line_ring_motion *)task->motion;
    int i;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    for (i = 0; i < VARS(work)->count; i++) {
        njPushMatrix(NULL);
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        draw_tree(&marker_object);
        njPopMatrix(1);
        njTranslateV(NULL, &motion->step);
    }
    njPopMatrix(1);
}
