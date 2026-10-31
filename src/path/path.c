/* paths: lines of points through a stage, the tasks each act makes for
 * them, and places along them by distance */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/polygon.h"
#include "path/path.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the knots path_run_cubic holds, and the most samples cubic_sample takes */
#define CUBIC_KNOTS 32
#define CUBIC_SAMPLES 256

/* a point's coordinate by axis, 0 to 2 for x to z, as cubic_solve takes
 * them in turn */
/* @hack spelling: the axis indexed on from x to y and z; as a byte offset into
 * the point, 163 of cubic_solve's 167 words differ */
#define AXIS(point, axis) ((&(point).x)[axis])

/* the stage_act that ends path_lists */
#define PATH_LIST_END 0xffff

/* the path_ends a path_spline's flags hold */
#define SPLINE_ENDS(flags) (((flags) >> 2) & 3)

/* a knot of a cubic spline, and its row of the tridiagonal system that
 * gives the bends */
struct cubic_knot {
    Float distance; /* from the first knot */
    Float length; /* from the knot before */
    /* the next length's share of the two around the knot, and the share of
     * the length before */
    Float next_share;
    Float prev_share;
    Float factor; /* what elimination scales the row before by */
    Float pivot; /* the row's diagonal, after elimination */
    NJS_POINT3 bend; /* the second derivative of pos by distance */
    NJS_POINT3 pos;
};

enum path_manager_mode { PATH_MANAGER_START, PATH_MANAGER_WATCH };

static void path_manager_step(struct task *task);
static void path_create_tasks(void);

void fun_8c08e0e0(void)
{
}

/* set each knot's distance and length from the knots' points, and each
 * inner knot's shares, its row of the system. then eliminate forward: the
 * first inner knot's pivot is 2, and each inner knot after it gets a factor
 * and a pivot */
static void cubic_setup(struct cubic_knot *knots, int count)
{
    struct cubic_knot *knot;
    int i;

    knots->distance = 0.0F;
    for (i = 1, knot = knots + 1; i < count; i++, knot++) {
        knot->distance =
            knot[-1].distance + njDistanceP2P(&knot[-1].pos, &knot->pos);
        knot->length = knot->distance - knot[-1].distance;
    }
    for (i = 1, knot = knots + 1; i < count - 1; i++, knot++) {
        knot->next_share = knot[1].length / (knot->length + knot[1].length);
        knot->prev_share = 1.0F - knot->next_share;
    }
    knots[1].pivot = 2.0F;
    for (i = 2, knot = knots + 2; i < count - 1; i++, knot++) {
        knot->factor = knot->prev_share / knot[-1].pivot;
        knot->pivot = 2.0F - (knot->factor * knot[-1].next_share);
    }
}

/* set the knots' bends, an axis at a time, by forward and back
 * substitution through the system cubic_setup eliminated */
static void cubic_solve(struct cubic_knot *knots, int count)
{
    Float rhs[CUBIC_KNOTS];
    Float forward[CUBIC_KNOTS];
    struct cubic_knot *knot;
    Float *in;
    Float *out;
    int axis;
    int i;

    for (axis = 0; axis < 3; axis++) {
        for (i = 1, knot = knots + 1, in = rhs + 1; i < count - 1;
             i++, knot++, in++) {
            *in = 6.0F *
                  (((AXIS(knot[1].pos, axis) - AXIS(knot->pos, axis)) /
                    knot[1].length) -
                   ((AXIS(knot->pos, axis) - AXIS(knot[-1].pos, axis)) /
                    knot->length)) /
                  (knot->length + knot[1].length);
        }
        forward[1] = rhs[1];
        for (i = 2, knot = knots + 2, in = rhs + 2, out = forward + 2;
             i < count - 1; i++, knot++, in++, out++) {
            *out = *in - (knot->factor * out[-1]);
        }
        AXIS(knot->bend, axis) = 0.0F;
        /* @bug memory: the second end zero indexes from knot, which
         * the forward loop leaves at the last knot, and lands count - 1
         * knots past it (FMOV.S FR4,@(R0,R14) at 0x8c08e2c8) */
        AXIS(knot[count - 1].bend, axis) = 0.0F;
        /* @bug memory: back substitution runs on down to the first
         * knot (CMP/PZ R6 at 0x8c08e306), where it reads a bend from before
         * the start of knots */
        /* @bug uninitialized: each bend takes the bend of the knot before,
         * which this pass has yet to set, where back substitution needs the
         * knot after's (FMOV.S @R2,FR3 at 0x8c08e2f6) */
        for (i = count - 2, knot = knots + i, out = &forward[i]; i >= 0;
             i--, knot--, out--) {
            AXIS(knot->bend, axis) =
                (*out - (knot->next_share * AXIS(knot[-1].bend, axis))) /
                knot->pivot;
        }
    }
}

/* walk the spline from the first knot a unit of distance at a time, and
 * return the samples taken, CUBIC_SAMPLES at most. pass i walks the span
 * that ends at knot i - 1, so the last span goes unwalked */
static int cubic_sample(struct cubic_knot *knots, NJS_POINT3 *out, int count)
{
    Bool active = TRUE;
    int samples = 0;
    int i;
    Float distance;
    Float ahead;
    Float behind;
    Float ahead_cubed;
    Float behind_cubed;
    Float inverse;

    distance = knots->distance;
    out->x = knots->pos.x;
    out->y = knots->pos.y;
    out->z = knots->pos.z;
    /* @bug logic: out moves on once a pass, not once a sample (ADD
     * #12,R5 at 0x8c08e4d6), so each sample in a span writes over the one
     * before */
    for (i = 1; i < count; i++, knots++, out++) {
        inverse = 1.0F / knots->length;
        while (knots->distance > distance) {
            distance += 1.0F;
            if (distance > knots->distance) {
                distance = knots->distance;
            }
            ahead = knots->distance - distance;
            behind = distance - knots[-1].distance;
            ahead_cubed = ahead * ahead * ahead * inverse;
            behind_cubed = behind * behind * behind * inverse;
            out->x =
                (((knots[-1].bend.x * (ahead_cubed - (ahead * knots->length))) +
                  (knots->bend.x * (behind_cubed - (behind * knots->length)))) *
                 (1.0F / 6.0F)) +
                (((ahead * knots[-1].pos.x) + (behind * knots->pos.x)) *
                 inverse);
            out->y =
                (((knots[-1].bend.y * (ahead_cubed - (ahead * knots->length))) +
                  (knots->bend.y * (behind_cubed - (behind * knots->length)))) *
                 (1.0F / 6.0F)) +
                (((ahead * knots[-1].pos.y) + (behind * knots->pos.y)) *
                 inverse);
            out->z =
                (((knots[-1].bend.z * (ahead_cubed - (ahead * knots->length))) +
                  (knots->bend.z * (behind_cubed - (behind * knots->length)))) *
                 (1.0F / 6.0F)) +
                (((ahead * knots[-1].pos.z) + (behind * knots->pos.z)) *
                 inverse);
            if (++samples == CUBIC_SAMPLES) {
                active = FALSE;
                break;
            }
        }
        if (!active) {
            break;
        }
    }
    return samples;
}

void path_run_cubic(void)
{
    /* @bug uninitialized: count, which no path writes, read from @R15 before
     * each call (0x8c08e4f6, 0x8c08e4fe and 0x8c08e506) */
    /* @hack type: volatile on count, which has each call read it from
     * @R15 (MOV.L @R15,R5 at 0x8c08e4f6); without it, SHC passes its slot's
     * address, the function runs 2 bytes short and 11 of 22 words differ */
#ifdef GCC
    /* repairs the count the first tag names: GCC's is 0, which samples
     * nothing */
    int count = 0;
#else
    volatile int count;
#endif
    struct cubic_knot knots[CUBIC_KNOTS];
    NJS_POINT3 out[CUBIC_SAMPLES];

    /* NOLINTNEXTLINE(clang-diagnostic-uninitialized) */
    cubic_setup(knots, count);
    cubic_solve(knots, count);
    cubic_sample(knots, out, count);
}

Bool path_create_manager(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 2, path_manager_step)) != NULL) {
        struct task_work *work = task->work;

        PATH_VARS(work)->stage_act = stage_get_stage_act();
        return TRUE;
    }
    return FALSE;
}

static void path_manager_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act;

    switch (work->mode) {
    case PATH_MANAGER_START:
        path_create_tasks();
        work->mode = PATH_MANAGER_WATCH;
        break;
    case PATH_MANAGER_WATCH:
        stage_act = stage_get_stage_act();
        if (PATH_VARS(work)->stage_act != stage_act) {
            path_create_tasks();
            PATH_VARS(work)->stage_act = stage_act;
        }
        break;
    }
}

static void path_create_tasks(void)
{
    struct path_list *list = path_lists;
    Uint16 stage_act = stage_get_stage_act();
    struct path **paths;
    struct path *path;
    struct task *task;

    for (;;) {
        if (list->stage_act == stage_act) {
            break;
        }
        if (list->stage_act == PATH_LIST_END) {
            return;
        }
        list++;
    }
    for (paths = list->paths; *paths != NULL; paths++) {
        path = *paths;
        if (path->step != NULL) {
            if ((task = task_create(TASK_WORK, 2, path->step)) != NULL) {
                struct task_work *work = task->work;

                PATH_VARS(work)->path = path;
                PATH_VARS(work)->stage_act = stage_act;
            }
        }
    }
}

/* the ends of the segment from point index to the next. each end takes
 * its point's pos, and the angles halfway from the point before's to its
 * point's. a path that stops repeats its first and last points past its
 * ends, and one that loops wraps round */
static void segment_ends(struct path *path, struct path_sample *sample,
                         int index, struct path_point *out)
{
    struct path_point around[3];
    int before;
    int after;

    around[1] = path->points[index];
    before = index - 1;
    after = index + 1;
    switch (path->ends) {
    case PATH_STOP:
        if (before < 0) {
            before = 0;
        } else if (after >= path->count) {
            after = path->count - 1;
        }
        break;
    case PATH_LOOP:
        if (before < 0) {
            before = path->count - 1;
        } else if (after >= path->count) {
            after = 0;
        }
        break;
    }
    around[0] = path->points[before];
    around[2] = path->points[after];
    out[0].ang_x = around[0].ang_x +
                   (angle_difference(around[0].ang_x, around[1].ang_x) >> 1);
    out[0].ang_z = around[0].ang_z +
                   (angle_difference(around[0].ang_z, around[1].ang_z) >> 1);
    out[0].pos.x = around[1].pos.x;
    out[0].pos.y = around[1].pos.y;
    out[0].pos.z = around[1].pos.z;
    out[1].ang_x = around[1].ang_x +
                   (angle_difference(around[1].ang_x, around[2].ang_x) >> 1);
    out[1].ang_z = around[1].ang_z +
                   (angle_difference(around[1].ang_z, around[2].ang_z) >> 1);
    out[1].pos.x = around[2].pos.x;
    out[1].pos.y = around[2].pos.y;
    out[1].pos.z = around[2].pos.z;
}

#ifdef GCC
/* the mark path_sample_from lends path_sample, or null */
static struct path_mark *sample_mark;

/* the point path_sample's search of path for sample's distance starts at,
 * and in *sum the lengths before it. the search that set sample_mark found no
 * segment before the mark's point that ends past the mark's distance, so none
 * ends past a distance at or past that, and a search from point 0 comes to
 * the mark's point with the mark's sum, by the same additions. it takes no
 * mark at or past count, which no search of path sets, so the search starts
 * on a point */
static int search_start(struct path *path, struct path_sample *sample,
                        Float *sum)
{
    if (sample_mark != NULL && sample_mark->index >= 0 &&
        sample_mark->index < path->count &&
        sample->distance >= sample_mark->distance) {
        *sum = sample_mark->sum;
        return sample_mark->index;
    }
    return 0;
}
#endif

Bool path_sample(struct path *path, struct path_sample *sample)
{
    NJS_VECTOR delta;
    struct path_point segment[2];
    struct path_point *point;
    Float sum;
    Float length;
    Float part;
    int i;

    switch (path->ends) {
    case PATH_STOP:
        if (sample->distance < 0.0F || sample->distance > path->length) {
            return FALSE;
        }
        break;
    case PATH_LOOP:
        sample->distance = float_wrap(sample->distance, path->length);
        break;
    }
#ifdef GCC
    /* repairs a path with no points, where the image reads length unset:
     * GCC's takes no sample, as for a distance off a path that stops */
    if (path->count == 0) {
        return FALSE;
    }
#endif
    sum = 0.0F;
#ifdef GCC
    /* the last point's break below ends the search, so it needs no test */
    for (i = search_start(path, sample, &sum), point = &path->points[i];;
         i++, point++) {
#else
    for (i = 0, point = path->points; i < path->count; i++, point++) {
#endif
        if (point->length == 0.0F) {
            length = 1.0F;
        } else {
            length = point->length;
        }
        if (sum + length > sample->distance) {
            break;
        }
#ifdef GCC
        /* repairs a distance at or past the sum of the points' lengths but
         * within the path's length, a gap rounding leaves between the two. no
         * point takes it, so the image's loop runs off the last point and
         * reads the next definition's bytes as a point. GCC's last point
         * takes it, at its segment's end */
        if (i == path->count - 1) {
            break;
        }
#endif
        sum += length;
    }
#ifdef GCC
    if (sample_mark != NULL && sample_mark->index < 0) {
        sample_mark->distance = sample->distance;
        sample_mark->sum = sum;
        sample_mark->index = i;
    }
#endif
    segment_ends(path, sample, i, segment);
    part = (sample->distance - sum) / length;
    sample->ang_x = point->ang_x;
    sample->ang_z = point->ang_z;
    sample->smooth_x =
        segment[0].ang_x +
        (Angle)(angle_difference(segment[0].ang_x, segment[1].ang_x) * part);
    sample->smooth_z =
        segment[0].ang_z +
        (Angle)(angle_difference(segment[0].ang_z, segment[1].ang_z) * part);
    delta.x = segment[1].pos.x - segment[0].pos.x;
    delta.y = segment[1].pos.y - segment[0].pos.y;
    delta.z = segment[1].pos.z - segment[0].pos.z;
    sample->pos.x = segment[0].pos.x + (part * delta.x);
    sample->pos.y = segment[0].pos.y + (part * delta.y);
    sample->pos.z = segment[0].pos.z + (part * delta.z);
    sample->pos_copy.x = sample->pos.x;
    sample->pos_copy.y = sample->pos.y;
    sample->pos_copy.z = sample->pos.z;
    sample->up.x = -(njSin(sample->ang_z) * njCos(sample->ang_x));
    sample->up.y = njCos(sample->ang_z) * njCos(sample->ang_x);
    sample->up.z = njSin(sample->ang_x);
    sample->smooth_up.x = -(njSin(sample->smooth_z) * njCos(sample->smooth_x));
    sample->smooth_up.y = njCos(sample->smooth_z) * njCos(sample->smooth_x);
    sample->smooth_up.z = njSin(sample->smooth_x);
    njUnitVector(&delta);
    sample->ahead = delta;
    return TRUE;
}

#ifdef GCC
Bool path_sample_from(struct path *path, struct path_sample *sample,
                      struct path_mark *mark)
{
    Bool sampled;

    sample_mark = mark;
    sampled = path_sample(path, sample);
    sample_mark = NULL;
    return sampled;
}
#endif

Bool path_find_segment(struct path *path, Float distance, Uint32 *index)
{
    struct path_point *point = path->points;
    Float sum = point->length;
    Uint32 i;

    for (i = 0; i < path->count && distance > sum; i++) {
        point++;
        sum += point->length;
    }
    *index = i;
    return TRUE;
}

Bool path_get_distance(struct path *path, Uint32 index, Float *distance)
{
    struct path_point *point;
    Float sum;
    Uint32 i;

    if (index > path->count) {
        return FALSE;
    }
    point = path->points;
    sum = 0.0F;
    i = 0;
    /* @hack spelling: i counted before the length is added, which keeps the
     * points in R7 and i in R4 (0x8c08e9ec, 0x8c08e9ee); counted after, the
     * two trade and 7 of 23 words differ */
    while (i < index) {
        i++;
        sum += point->length;
        point++;
    }
    *distance = sum;
    return TRUE;
}

Float path_wrap(struct path *path, Float distance)
{
    return float_wrap(distance, path->length);
}

/* a path that stops has a segment fewer than it has points, and a loop's
 * last segment runs back to its first point */
Float path_find_nearest(struct path *path, NJS_POINT3 *pos, NJS_POINT3 *nearest,
                        Float *along)
{
    struct path_point *point = path->points;
    int last = 1 - path->ends;
    Float best;
    Float start = 0.0F;
    Float away;
    NJS_POINT3 *to;
    NJS_POINT3 on;
    int i;

#ifdef GCC
    /* repairs a path with no segment, where the image returns best unset:
     * nothing on it is near, so GCC's best is INFINITY */
    best = INFINITY;
#endif
    for (i = path->count; i > last; i--) {
        Float length = point->length;
        NJS_POINT3 *from = &point->pos;
        Float part;

        point++;
        if (i != 1) {
            to = &point->pos;
        } else {
            to = &path->points[0].pos;
        }
        /* start is how far along the path the segment begins. best is unset
         * until a segment sets it. while start is 0, at the first segment and
         * after only 0-long ones, a segment sets best without a comparison */
        if (start == 0.0F) {
            best = collision_segment_distance(pos, from, to, &on);
            if (nearest != NULL) {
                *nearest = on;
            }
            if (along != NULL) {
                part = njDistanceP2P(from, &on) / length;
                *along = start + (part * length);
            }
        } else if ((away = collision_segment_distance(pos, from, to, &on)) <
                   best) {
            best = away;
            if (nearest != NULL) {
                *nearest = on;
            }
            if (along != NULL) {
                part = njDistanceP2P(from, &on) / length;
                *along = start + (part * length);
            }
        }
        start += length;
    }
    return best;
}

/* the point part of the way through span index of a quadratic b-spline,
 * which points index - 1 to index + 1 shape */
static void spline_position(NJS_POINT3 *out, struct path_spline_point *points,
                            int index, Float part)
{
    Float w0 = (1.0F - part) * (1.0F - part) * 0.5F;
    Float w1 = 0.5F + (part * (1.0F - part));
    Float w2 = part * part * 0.5F;
    struct path_spline_point *point = &points[index - 1];

    out->x =
        (w0 * point[0].pos.x) + (w1 * point[1].pos.x) + (w2 * point[2].pos.x);
    out->y =
        (w0 * point[0].pos.y) + (w1 * point[1].pos.y) + (w2 * point[2].pos.y);
    out->z =
        (w0 * point[0].pos.z) + (w1 * point[1].pos.z) + (w2 * point[2].pos.z);
}

/* set *index to the span that holds distance, and *part to how far
 * through it, as a share of its length. the search ends at span count - 3.
 * a distance past the spans before it gets that span, with part measured by
 * span count - 4's length. with count 4 or less the search reads no span,
 * and part divides by an unset length */
static void spline_find_span(Float distance, struct path_spline *spline,
                             int *index, Float *part)
{
    Float sum = 0.0F;
    struct path_spline_point *point = &spline->points[1];
    Float length;
    int i;

#ifdef GCC
    /* repairs the unset length the comment above names: with count 4 or
     * less, GCC's part is measured by span 1's length, the span *index
     * gets */
    length = point->length;
#endif
    for (i = 1; i < spline->count - 3; i++, point++) {
        length = point->length;
        if (sum + length > distance) {
            break;
        }
        sum += length;
    }
    *index = i;
    *part = (distance - sum) / length;
}

Bool path_spline_sample(struct path_spline *spline, struct path_sample *sample)
{
    Uint16 ends = SPLINE_ENDS(spline->flags);
    int index;
    Float part;
    NJS_POINT3 pos;
    NJS_POINT3 behind;
    NJS_POINT3 side_behind;
    NJS_POINT3 ahead;
    NJS_POINT3 side_ahead;
    NJS_VECTOR diagonal1;
    NJS_VECTOR diagonal2;
    Float at;
    int back;
    int front;

    switch (ends) {
    case PATH_STOP:
        if (sample->distance < 0.0F || sample->distance > spline->length) {
            return FALSE;
        }
        break;
    case PATH_LOOP:
        sample->distance = float_wrap(sample->distance, spline->length);
        break;
    }
    spline_find_span(sample->distance, spline, &index, &part);
    spline_position(&pos, spline->points, index, part);
    /* @hack spelling: pos copied float by float, for the image's FMOV.S copies
     * (MOV #80,R0 at 0x8c08ec90 on); as a struct, SHC copies it in MOV.L
     * words, as it does into pos_copy, the function runs 8 bytes short and
     * 217 of 279 words differ */
    sample->pos.x = pos.x;
    sample->pos.y = pos.y;
    sample->pos.z = pos.z;
    sample->pos_copy = pos;

    at = part - spline->reach;
    back = index;
    if (at < 0.0F) {
        at += 1.0F;
        /* @hack spelling: back -= 1, which saves no R10 (FMOV.S FR15,@-R15
         * at 0x8c08ec28); as back--, SHC keeps the constant 1 in R10, the
         * function runs 4 bytes long and 147 of 279 words differ */
        back -= 1;
        if (back < 1) {
            switch (ends) {
            case PATH_STOP:
                back = 1;
                at = 0.0F;
                break;
            case PATH_LOOP:
                back = spline->count - 2;
                break;
            }
        }
    }
    spline_position(&behind, spline->points, back, at);
    spline_position(&side_behind, spline->side, back, at);

    at = part + spline->reach;
    front = index;
    if (at > 1.0F) {
        at -= 1.0F;
        front += 1;
        if (front > spline->count - 2) {
            switch (ends) {
            case PATH_STOP:
                front = spline->count - 2;
                at = 1.0F;
                break;
            case PATH_LOOP:
                front = 1;
                break;
            }
        }
    }
    spline_position(&ahead, spline->points, front, at);
    spline_position(&side_ahead, spline->side, front, at);

    sample->ahead.x = ahead.x - behind.x;
    sample->ahead.y = ahead.y - behind.y;
    sample->ahead.z = ahead.z - behind.z;
    njUnitVector(&sample->ahead);
    diagonal1.x = side_ahead.x - behind.x;
    diagonal1.y = side_ahead.y - behind.y;
    diagonal1.z = side_ahead.z - behind.z;
    diagonal2.x = ahead.x - side_behind.x;
    diagonal2.y = ahead.y - side_behind.y;
    diagonal2.z = ahead.z - side_behind.z;
    njOuterProduct(&diagonal1, &diagonal2, &sample->up);
    njUnitVector(&sample->up);
    sample->smooth_up = sample->up;
    sample->ang_x = sample->smooth_x =
        (Angle)(asinf(sample->up.z) * ANGLE_PER_RADIAN);
    sample->ang_z = sample->smooth_z =
        -(Angle)(atan2f(sample->up.x, sample->up.y) * ANGLE_PER_RADIAN);
    /* @hack spelling: no return value is set, and R0 keeps the address of
     * ANGLE_PER_RADIAN's pool word from the MOVA at 0x8c08ee6a. nothing in the
     * image calls the function, so no caller reads the result. with return
     * TRUE, SHC loads 1 into R0, the function runs 2 bytes long and 16 of 279
     * words differ */
#ifdef GCC
    /* repairs the return the tag names: GCC's is TRUE, for the sample it
     * took */
    return TRUE;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}
