/* line ring v, O LINE RING V: once its switch is on, it drops rings one by
 * one. they rise along a quarter circle from the ground, then straight
 * up */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/sparkle.h"
#include "object/line_ring_v.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/switch.h"
#include "system/task.h"
#include "system/util.h"

/* the most rings it drops */
#define RING_COUNT 30

/* the timer counts steps from 0. a ring drops when it passes RING_STEPS, so
 * one drops every RING_STEPS + 1 steps. once all have dropped, the rings
 * left go when it passes LIE_STEPS */
#define RING_STEPS 3
#define LIE_STEPS 600

/* the arc's radius is scl.x plus RADIUS_BASE, and at least RADIUS_LEAST.
 * the space between rings is scl.z plus SPACE_BASE, and at least
 * SPACE_LEAST */
#define RADIUS_BASE 20.0F
#define RADIUS_LEAST 10.0F
#define SPACE_BASE 24.0F
#define SPACE_LEAST 10.0F

/* line ring v's vars. its placement sets its angles and scale. the low
 * byte of ang.x is its switch's id, and ang.y the way the arc faces. scl.x,
 * scl.y and scl.z set the arc's radius, the line's height and the space
 * between rings, as line_ring_v_measure reads them */
struct line_ring_v_vars {
    Float arc; /* the arc's length */
    Uint8 unk_04[4];
    Float distance; /* the next ring's place along the arc, then the line */
};

#define VARS(work) ((struct line_ring_v_vars *)&(work)->vars)

/* line ring v's own record, in its task's motion */
struct line_ring_v_motion {
    Uint8 unk_00[0x10];
    NJS_POINT3 next; /* where the next ring on the line drops */
    Uint8 unk_1c[8];
    Angle step; /* round the arc from one ring to the next */
    Uint8 unk_28[8];
    Angle ang; /* round the arc to the next ring */
    Uint8 unk_34[4];
    Float radius;
    Float space;
};

enum line_ring_v_mode {
    LINE_RING_V_WAIT, /* for its switch */
    LINE_RING_V_ARC,
    LINE_RING_V_LINE,
    LINE_RING_V_LIE, /* the rings lie until they're taken, or time runs out */
    LINE_RING_V_RESET
};

static void line_ring_v_step(struct task *task);
static void line_ring_v_destroy(struct task *task);
static void line_ring_v_measure(struct task *task);
static void line_ring_v_draw(struct task *task);

void line_ring_v_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.z = 0;
    work->mode = LINE_RING_V_WAIT;
    TASK_TIMER(work) = 0;
    /* @hack redundant: mode stored again, as the image stores it twice
     * (MOV.B at 0x8c06554e and 0x8c065552); stored once, 10 of 19 words
     * differ */
    work->mode = LINE_RING_V_WAIT;
    line_ring_v_measure(task);
    task->step = line_ring_v_step;
    task->destructor = line_ring_v_destroy;
}

static void line_ring_v_step(struct task *task)
{
    struct task_work *work = task->work;
    struct line_ring_v_motion *motion =
        (struct line_ring_v_motion *)task->motion;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        line_ring_v_measure(task);
        line_ring_v_draw(task);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<- Switch ID");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> R LENGTH");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> Y LENGTH");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> RING INTERVAL");
    } else if (edit_mode) {
        line_ring_v_draw(task);
    }
    switch (work->mode) {
    case LINE_RING_V_WAIT:
        if (switch_is_on(work->ang.x & 0xff)) {
            work->mode = LINE_RING_V_ARC;
            VARS(work)->distance = 0.0F;
            motion->ang = NJM_DEG_ANG(-90);
        }
        break;
    case LINE_RING_V_ARC:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > RING_STEPS) {
            Float *pos;
            char *ring;
            Float x;
            Float y;
            Float z;
            struct task *child;

            TASK_TIMER(work) = 0;
            /* @hack spelling: a Float cursor stepped across pos's members, for
             * the image's @Rn+ loads; as work->pos.x, .y and .z, 127 of 348
             * words differ */
            pos = &work->pos.x;
            x = *pos++;
            y = *pos++ + motion->radius;
            z = *pos;
            child = task_create_child(TASK_WORK, ring_step, task);
            /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
            if (child != NULL) {
                Angle ang = motion->ang;
                Angle heading = work->ang.y;
                Float radius = motion->radius;
                Float dx = njCos(ang) * radius;
                Float dy = njSin(ang) * radius;
                Float dz = -(njSin(heading) * dx);

                dx = njCos(heading) * dx;
                /* @hack spelling: the ring's pos written through a byte cursor,
                 * ADD #4 between stores, which stays at z for the sparkle;
                 * through the members, with ring set after, the function
                 * runs 32 bytes long and 218 of 348 words differ */
                ring = (char *)&child->work->pos;
                *(Float *)ring = x + dx;
                ring += sizeof(Float);
                *(Float *)ring = y + dy;
                ring += sizeof(Float);
                *(Float *)ring = z + dz;
            }
            motion->ang += motion->step;
            /* @bug memory: with no ring made (BT at 0x8c0656d6), no path
             * sets ring. the sparkle then reads its position from 0xfffffff8,
             * 8 bytes before the null that task_create_child leaves in R4
             * (MOV R0,R4 at 0x8c09e754; ADD #-8,R4 at 0x8c065740) */
            ring -= 2 * sizeof(Float);
            {
                NJS_VECTOR speed = {0.0F, 0.05F, 0.0F};

                sparkle_create((NJS_POINT3 *)ring, &speed, 0.8F,
                               NJM_DEG_ANG(5));
            }
            VARS(work)->distance += motion->space;
            if (VARS(work)->distance > VARS(work)->arc) {
                VARS(work)->distance -= VARS(work)->arc;
                work->mode = LINE_RING_V_LINE;
                motion->next.x = x + (njCos(work->ang.y) * motion->radius);
                motion->next.y = y + VARS(work)->distance;
                motion->next.z = z - (njSin(work->ang.y) * motion->radius);
            }
        }
        task_step_children(task);
        break;
    case LINE_RING_V_LINE:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > RING_STEPS) {
            /* @hack spelling: a Float cursor stepped across next's members
             * and back to y, for the image's @Rn+ loads; as motion->next.x, .y
             * and .z, 152 of 348 words differ */
            Float *pos = &motion->next.x;
            Float *ring;
            Float x;
            Float y;
            Float z;
            struct task *child;

            TASK_TIMER(work) = 0;
            x = *pos++;
            y = *pos++;
            z = *pos++;
            pos -= 2;
            *pos = y + motion->space;
            child = task_create_child(TASK_WORK, ring_step, task);
            /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
            if (child != NULL) {
                /* @hack spelling: a Float cursor stepped across the members
                 * of the ring's pos, which stops at z for the sparkle; as the
                 * members, with ring set to pos.z after them, the function
                 * runs 32 bytes long and 151 of 348 words differ */
                ring = &child->work->pos.x;
                *ring++ = x;
                *ring++ = y;
                *ring = z;
            }
            /* @bug memory: with no ring made, the sparkle reads its position
             * from 0xfffffff8, through a ring no path set (BT at 0x8c06581e).
             * R4, ring's register, then holds the null child (0x8c06581a),
             * and ADD #-8,R4 at 0x8c065844 steps back from it */
            ring -= 2;
            {
                NJS_VECTOR speed = {0.0F, 0.05F, 0.0F};

                sparkle_create((NJS_POINT3 *)ring, &speed, 0.8F,
                               NJM_DEG_ANG(5));
            }
            VARS(work)->distance += motion->space;
            if (VARS(work)->distance > work->scl.y) {
                work->mode = LINE_RING_V_LIE;
            }
        }
        task_step_children(task);
        break;
    case LINE_RING_V_LIE:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > LIE_STEPS || task->child == NULL) {
            work->mode = LINE_RING_V_RESET;
            TASK_TIMER(work) = 0;
        }
        task_step_children(task);
        break;
    case LINE_RING_V_RESET:
        task_destroy_children(task);
        work->mode = LINE_RING_V_WAIT;
        break;
    }
}

static void line_ring_v_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* set the arc's radius, the line's height and the space between rings
 * from the scale. the height is scl.y, and at least 0. with more than
 * RING_COUNT rings on the arc, the space widens to the arc's length over
 * RING_COUNT, and the height becomes 0. otherwise, when the arc's rings and
 * the line's would pass RING_COUNT, the line shortens to hold the rest of
 * RING_COUNT. the height is written back to scl.y */
static void line_ring_v_measure(struct task *task)
{
    struct task_work *work = task->work;
    struct line_ring_v_motion *motion =
        (struct line_ring_v_motion *)task->motion;
    /* @hack spelling: a byte cursor over the scale, for the image's @Rn+ loads;
     * field by field, SHC reaches each through R0 and 100 of 110 words
     * differ */
    const char *scl = (const char *)&work->scl;
    Float radius;
    Float height;
    Float space;
    Float count;
    Float rest;

    radius = *(const Float *)scl + RADIUS_BASE;
    scl += sizeof(Float);
    height = *(const Float *)scl;
    scl += sizeof(Float);
    space = *(const Float *)scl + SPACE_BASE;
    if (radius < RADIUS_LEAST) {
        radius = RADIUS_LEAST;
    }
    if (height < 0.0F) {
        height = 0.0F;
    }
    if (space < SPACE_LEAST) {
        space = SPACE_LEAST;
    }
    VARS(work)->arc = radius * (NJD_PI / 2);
    motion->step = space * NJM_DEG_ANG(90) / VARS(work)->arc;
    count = njRoundOff(VARS(work)->arc / space) + 1.0F;
    rest = VARS(work)->arc - (space * count);
    if ((int)count > RING_COUNT) {
        space = VARS(work)->arc * (1.0F / RING_COUNT);
        height = 0.0F;
    } else {
        Float up = njRoundOff((height + rest) / space);

        if ((int)(count + up) > RING_COUNT) {
            height = ((RING_COUNT - count) * space) - rest;
        }
    }
    motion->radius = radius;
    work->scl.y = height;
    motion->space = space;
}

/* draw a marker where each ring will drop, for the set editor */
static void line_ring_v_draw(struct task *task)
{
    struct line_ring_v_motion *motion =
        (struct line_ring_v_motion *)task->motion;
    struct task_work *work = task->work;
    Float distance;

    motion->ang = NJM_DEG_ANG(-90);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njTranslate(NULL, 0.0F, motion->radius, 0.0F);
    TURN(njRotateY, work->ang.y);
    distance = 0.0F;
    while (distance < VARS(work)->arc) {
        Float x;
        Float y;
        Angle ang = motion->ang;

        njPushMatrix(NULL);
        x = njCos(ang) * motion->radius;
        y = njSin(ang) * motion->radius;
        njTranslate(NULL, x, y, 0.0F);
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        draw_tree(&marker_object);
        njPopMatrix(1);
        motion->ang += motion->step;
        distance += motion->space;
    }
    distance -= VARS(work)->arc;
    njTranslate(NULL, motion->radius, distance, 0.0F);
    while (distance < work->scl.y) {
        njPushMatrix(NULL);
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        draw_tree(&marker_object);
        njPopMatrix(1);
        njTranslate(NULL, 0.0F, motion->space, 0.0F);
        distance += motion->space;
    }
    njPopMatrix(1);
}
