/* Mtn SpiderA, a spider that climbs and drops on a thread, and Mtn SpiderB,
 * one that walks the surface under it and turns back where it ends */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/helpers.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "stg05/spider.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_ACTION mountain_spider_action;

enum mountain_spider_mode { SPIDER_INIT, SPIDER_PAUSE, SPIDER_MOVE };

/* spider A's thread, a line from points[0] down to points[1]. points[1]
 * moves with the spider. points[0] stays 40 times the scale above where
 * points[1] started */
struct mountain_spider_thread {
    NJS_POINT3COL line;
    NJS_POINT3 points[2];
    NJS_COLOR colors[2];
};

struct mountain_spider_vars {
    NJS_ACTION *action;
    Float frame;
    /* spider A's, from the first of its steps that the set editor doesn't
     * hold, and null before. spider B's is null */
    struct mountain_spider_thread *thread;
};

struct mountain_spider_a_extra {
    Float bottom; /* the height its moves stay above */
    Float timer; /* steps left in the mode */
    Float speed; /* its move each step, up or down */
};

struct mountain_spider_b_motion {
    /* the object the last cast crossed, or null, which the next cast tries
     * first */
    NJS_OBJECT *ground;
    Float speed; /* how far it walks each step, along its heading */
    Uint8 unk_08[8];
    NJS_POINT3 home; /* where it stood when it started */
    Uint8 unk_1c[28];
    /* how far it stands off the ground: the y of its action's root object,
     * times the scale */
    Float height;
    Float timer; /* steps left in the mode */
};

#define VARS(work) ((struct mountain_spider_vars *)&(work)->vars)
#define A_EXTRA(task) ((struct mountain_spider_a_extra *)(task)->extra)
#define B_MOTION(task) ((struct mountain_spider_b_motion *)(task)->motion)
/* the height spider A starts at, which it climbs no higher than */
#define TOP(work) ((work)->scl.y)
/* the set entry's bottom for spider A, or 0 for one it picks */
#define BOTTOM(work) ((work)->scl.z)

/* the scale a set entry's scl.x of 0 stands for */
#define DEFAULT_SCALE 0.06F
/* spider A's move speed is picked between SPEED_FROM and SPEED_FROM +
 * SPEED_SPAN */
/* @hack spelling: SPEED_FROM and SPEED_SPAN spelled just past -0.3 and 0.6.
 * SHC converts a literal to a Float toward zero, so these give the Floats
 * nearest -0.3 and 0.6. the image holds SPEED_FROM at 0x0c90ecc0, and
 * SPEED_SPAN times RAND_SCALE at 0x0c90ecc4; as -0.3F and 0.6F, 0 of 182
 * words and 2 of 58 other words differ */
#define SPEED_FROM (-0.30000002F)
#define SPEED_SPAN 0.60000004F
#define DRAW_RADIUS 10.0F

static void mountain_spider_a_init(struct task *task);
static void mountain_spider_b_init(struct task *task);
static void mountain_spider_a_draw(struct task *task);
static void mountain_spider_a_destroy(struct task *task);
static void mountain_spider_b_walk(struct task *task);
static void mountain_spider_b_draw(struct task *task);
static void mountain_spider_b_destroy(struct task *task);

void mountain_spider_a_step(struct task *task)
{
    struct task_work *work = task->work;
    struct mountain_spider_a_extra *extra = A_EXTRA(task);
    struct mountain_spider_a_extra *copy;
    Float y;

    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "Ito no Sita no Position");
    }
    switch (work->mode) {
    case SPIDER_INIT:
        mountain_spider_a_init(task);
        break;
    case SPIDER_PAUSE:
        action_advance(VARS(work)->action, &VARS(work)->frame, 1.0F);
        if ((extra->timer -= 1.0F) < 0.0F) {
            work->mode = SPIDER_MOVE;
            /* the move's timer starts from 48 up to 60. SHC converts 0.2F to
             * a Float toward zero. 0.2F times 60 is then one below 12 in the
             * last place. that gives the image's factor at 0x0c90ecbc,
             * 0x39bfffff */
            extra->timer = 48.0F + (rand() * (0.2F * RAND_SCALE * 60.0F));
            /* the speed starts from -0.3 up to 0.3. it then moves 0.2 further
             * from 0 */
            if ((extra->speed = SPEED_FROM +
                                (rand() * (SPEED_SPAN * RAND_SCALE))) > 0.0F) {
                extra->speed = extra->speed + 0.2F;
            } else {
                /* @hack type: speed by its byte offset, for the MOVA and FMOV.S
                 * of 0.2F at 0x0c90ebba; as extra->speed, SHC loads it by
                 * MOV.L, LDS and FSTS, and 4 of 182 words differ */
                *(Float *)((char *)extra + 8) -= 0.2F;
            }
        }
        break;
    case SPIDER_MOVE:
        /* @hack redundant: the same copy in both arms of a constant test,
         * which keeps a second pointer to extra, copied to R4 at 0x0c90ebca,
         * with work in R14 and extra in R13; as one copy, the function is 2
         * bytes shorter and 114 of 182 words differ */
        if (1) {
            copy = extra;
        } else {
            copy = extra;
        }
        y = work->pos.y + copy->speed;
        copy->timer -= 1.0F;
        /* @hack redundant: a goto to the next statement, which makes SHC reload
         * the timer it stored, at 0x0c90ebdc; without it, the function is 2
         * bytes shorter and 96 of 182 words differ */
        goto test_timer;
    test_timer:
        if (copy->timer < 0.0F || y > TOP(work) || y <= copy->bottom) {
            work->mode = SPIDER_PAUSE;
            extra->timer = RANDOM(90.0F, 120.0F);
        } else {
            VARS(work)->thread->points[1].y += extra->speed;
            work->pos.y = y;
        }
        break;
    }
    if (placement_editing(task)) {
        scratch_point.x = work->pos.x;
        scratch_point.y = TOP(work);
        scratch_point.z = work->pos.z;
        scratch_vector.x = 0.0F;
        scratch_vector.y = -400.0F;
        scratch_vector.z = 0.0F;
        arrow_draw_y(&scratch_point, &scratch_vector);
        scratch_vector.x = 0.0F;
        scratch_vector.y = extra->bottom - TOP(work);
        scratch_vector.z = 0.0F;
        arrow_draw_y(&scratch_point, &scratch_vector);
    }
    object_step_draw(task);
    placement_try_leave(task);
}

void mountain_spider_b_step(struct task *task)
{
    struct task_work *work = task->work;
    struct mountain_spider_b_motion *motion = B_MOTION(task);

    switch (work->mode) {
    case SPIDER_INIT:
        mountain_spider_b_init(task);
        break;
    case SPIDER_PAUSE:
        if ((motion->timer -= 1.0F) < 0.0F) {
            work->mode = SPIDER_MOVE;
            work->ang.y += (Angle)(((rand() * RAND_SCALE) - 0.5F) *
                                   (80.0F * ANGLE_PER_DEGREE));
            motion->timer = RANDOM(60.0F, 120.0F);
            /* @hack redundant: a break inside the if, which shares case
             * SPIDER_MOVE's timer reset, the BRA at 0x0c90ed5e to 0x0c90ed74;
             * with one break after the if, the reset repeats here, the
             * function is 18 bytes longer and 85 of 129 words differ */
            break;
        }
        break;
    case SPIDER_MOVE:
        if ((motion->timer -= 1.0F) < 0.0F) {
            work->mode = SPIDER_PAUSE;
            motion->timer = RANDOM(60.0F, 120.0F);
        } else {
            action_advance(VARS(work)->action, &VARS(work)->frame,
                           motion->speed * 3.0F);
            mountain_spider_b_walk(task);
        }
        break;
    }
    if (edit_mode == 1) {
        marker_draw(&motion->home, 2.0F);
        scratch_vector.x = work->pos.x - motion->home.x;
        scratch_vector.y = work->pos.y - motion->home.y;
        scratch_vector.z = work->pos.z - motion->home.z;
        arrow_draw_y(&motion->home, &scratch_vector);
    }
    object_step_draw(task);
    placement_try_leave(task);
}

static void mountain_spider_a_init(struct task *task)
{
    struct task_work *work = task->work;
    struct mountain_spider_a_extra *extra = A_EXTRA(task);
    struct segment_query *query = &scratch_segment_query;
    struct mountain_spider_thread *thread;
    Float scale;
    Float bottom;
    Float drop;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[3]. the image's ADD #-28,R15 reserves them
     * (0x0c90ee70); without them, 2 of 298 words differ */
    Uint32 unused[3];
    NJS_POINT3 point;

    VARS(work)->action = &mountain_spider_action;
    task->draw = mountain_spider_a_draw;
    task->destructor = mountain_spider_a_destroy;
    if (work->scl.x == 0.0F) {
        work->scl.x = DEFAULT_SCALE;
    }
    scale = work->scl.x;
    VECTOR_COPY(&query->pos, &work->pos);
    query->dir.x = 0.0F;
    query->dir.y = 400.0F;
    query->dir.z = 0.0F;
    if (polygon_cast_segment(query, NULL)) {
        work->pos.y = query->hit.y - (scale * 23.5F);
    }
    work->ang.x = 0x4000;
    if (placement_editing(task)) {
        scratch_vector.x = 0.0F;
        scratch_vector.y = scale - work->pos.y;
        scratch_vector.z = 0.0F;
        arrow_draw_y(&work->pos, &scratch_vector);
        /* @hack redundant: a never-run test of scale, which keeps scale live
         * past the subtraction, which works in FR2 at 0x0c90eef2, not in
         * scale's FR15; without it, 31 of 298 words differ */
        if (0) {
            if (scale) {
            }
        }
        return;
    }
    work->mode = SPIDER_PAUSE;
    TOP(work) = work->pos.y;
    if (BOTTOM(work) != 0.0F) {
        bottom = BOTTOM(work);
    } else {
        VECTOR_COPY(&query->pos, &work->pos);
        query->dir.x = 0.0F;
        query->dir.y = -1000.0F;
        query->dir.z = 0.0F;
        /* @bug uninitialized: on a miss, no path sets bottom: the BT at
         * 0x0c90efb0 skips the hit's load at 0x0c90efb4, and the FSUB at
         * 0x0c90efbe subtracts FR4 as polygon_cast_segment leaves it */
#ifdef GCC
        /* repairs the unset bottom the tag names: a miss takes the cast's
         * end, 1000 below, so the drop passes 80 */
        bottom = query->pos.y + query->dir.y;
#endif
        /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
        if (polygon_cast_segment(query, NULL)) {
            bottom = query->hit.y;
        }
        drop = work->pos.y - bottom;
        if (drop > 80.0F) {
            bottom = work->pos.y + (rand() * (40.0F * RAND_SCALE)) - 80.0F;
        } else {
            bottom = work->pos.y - (drop * 0.6F) -
                     (drop * rand() * (0.2F * RAND_SCALE));
        }
    }
    extra->bottom = bottom;
    extra->timer = RANDOM(90.0F, 120.0F);
    thread = heap_alloc(sizeof *thread);
    VARS(work)->thread = thread;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    njScale(NULL, scale, scale, scale);
    point.x = -0.5F;
    point.y = 23.5F;
    point.z = -8.5F;
    njCalcPoint(NULL, &point, &thread->points[1]);
    njPopMatrix(1);
    VECTOR_ADD(&thread->points[1], &work->pos);
    thread->points[0].x = thread->points[1].x;
    thread->points[0].y = thread->points[1].y + (scale * 40.0F);
    thread->points[0].z = thread->points[1].z;
    thread->colors[0].color = thread->colors[1].color = 0x20ffffff;
    thread->line.p = thread->points;
    thread->line.col = thread->colors;
    thread->line.tex = NULL;
    thread->line.num = 2;
}

static void mountain_spider_b_init(struct task *task)
{
    struct task_work *work = task->work;
    struct mountain_spider_b_motion *motion = B_MOTION(task);
    struct segment_query *query = &scratch_segment_query;

    VARS(work)->action = &mountain_spider_action;
    task->draw = mountain_spider_b_draw;
    task->destructor = mountain_spider_b_destroy;
    if (work->scl.x == 0.0F) {
        work->scl.x = DEFAULT_SCALE;
    }
    motion->height = VARS(work)->action->object->pos[1] * work->scl.x;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    scratch_vector.x = 0.0F;
    scratch_vector.y = -400.0F;
    scratch_vector.z = 0.0F;
    njCalcVector(NULL, &scratch_vector, &query->dir);
    if (placement_editing(task)) {
        njPopMatrix(1);
        arrow_draw_y(&work->pos, &query->dir);
        return;
    }
    work->mode = SPIDER_PAUSE;
    VECTOR_COPY(&query->pos, &work->pos);
    if (polygon_cast_segment(query, NULL)) {
        work->ang.x = query->ang_x;
        work->ang.z = query->ang_z;
        scratch_vector.x = 0.0F;
        scratch_vector.y = motion->height;
        scratch_vector.z = 0.0F;
        njCalcPoint(NULL, &scratch_vector, &work->pos);
        VECTOR_ADD(&work->pos, &query->hit);
    }
    njPopMatrix(1);
    motion->timer = RANDOM(60.0F, 120.0F);
    motion->speed = (((rand() * RAND_SCALE) - 0.5F) * 0.4F) + 0.4F;
    vector_copy(VECTOR_FLOATS(&work->pos), VECTOR_FLOATS(&motion->home));
}

static void mountain_spider_a_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        njScale(NULL, scale, scale, scale);
        draw_action(VARS(work)->action, VARS(work)->frame);
        njPopMatrix(1);
        if (VARS(work)->thread) {
            njDrawLine3D(&VARS(work)->thread->line, 1, NJD_TRANSPARENT);
        }
    }
}

static void mountain_spider_a_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->thread) {
        heap_free(VARS(work)->thread);
    }
}

static void mountain_spider_b_walk(struct task *task)
{
    struct task_work *work = task->work;
    struct mountain_spider_b_motion *motion = B_MOTION(task);
    struct segment_query *query = &scratch_segment_query;
    NJS_POINT3 point;

    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    scratch_vector.x = 0.0F;
    scratch_vector.y = motion->height;
    scratch_vector.z = motion->speed;
    njCalcPoint(NULL, &scratch_vector, &query->pos);
    VECTOR_ADD(&query->pos, &work->pos);
    scratch_vector.x = 0.0F;
    scratch_vector.y = -10.0F;
    scratch_vector.z = 0.0F;
    njCalcVector(NULL, &scratch_vector, &query->dir);
    njPopMatrix(1);
    if ((motion->ground = polygon_cast_segment(query, motion->ground)) !=
        NULL) {
        work->ang.x = query->ang_x;
        work->ang.z = query->ang_z;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        scratch_vector.x = 0.0F;
        scratch_vector.y = motion->height;
        scratch_vector.z = 0.0F;
        njCalcPoint(NULL, &scratch_vector, &point);
        njPopMatrix(1);
        work->pos.x = query->hit.x + point.x;
        work->pos.y = query->hit.y + point.y;
        work->pos.z = query->hit.z + point.z;
    } else {
        work->ang.y += 0x8000;
    }
}

static void mountain_spider_b_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, DRAW_RADIUS)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, scale, scale, scale);
        draw_action(VARS(work)->action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void mountain_spider_b_destroy(struct task *task)
{
}
