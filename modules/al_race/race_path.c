/* the race's paths: the course, the other paths beside it, and a target on
 * the course */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/course.h"
#include "al_race/game.h"
#include "al_race/race_path.h"
#include "path/path.h"
#include "system/task.h"
#include "system/util.h"

/* the target's x when there's none */
#define NO_TARGET (-10000.0F)
/* a point closer than this to the target across x and z is at it */
#define TARGET_RANGE 40.0F
/* an along past a path's length less this ends race_path_locate's search */
#define END_MARGIN 10.0F
#define CLEAR_TARGET(path)                                                     \
    ((path)->target.x = NO_TARGET, (path)->target.y = 0.0F,                    \
     (path)->target.z = 0.0F)

static int race_path_point_at(struct race_path *path, NJS_POINT3 *point,
                              Float distance);
static void race_path_measure(struct race_path_span *span, struct path *path,
                              struct path *course);

/* the other paths each race kind allows, as bits: 1 for the first, 2 for the
 * second */
static const Uint8 race_kind_paths[5] = {0, 1, 2, 2, 3};

/* @hack spelling: heap_calloc called through an inline function, which
 * stores count, then spans, in the frame's word at sp+0 (0x0cb90e46,
 * 0x0cb90e4c); called directly, race_path_init is 10 bytes shorter and 67 of
 * its 81 words differ */
#pragma inline(race_path_alloc_spans)
static struct race_path_span *race_path_alloc_spans(int count)
{
    struct race_path_span *spans = heap_calloc(1, count * sizeof *spans);

    return spans;
}

void race_path_init(struct race_path *path, struct path **paths, int count,
                    Float distance)
{
    int i;

    path->spans = race_path_alloc_spans(count);
    path->paths = paths;
    path->count = count;
    if (path->spans) {
        for (i = 0; i < count; i++) {
            race_path_measure(&path->spans[i], paths[i], paths[count]);
        }
        if (!race_path_point_at(path, &path->target, distance)) {
            CLEAR_TARGET(path);
        }
    } else {
        CLEAR_TARGET(path);
    }
}

void race_path_locate(struct race_path *path, NJS_POINT3 *point,
                      struct race_path_position *position, int mode)
{
    struct path **paths = path->paths;
    Float best;
    Float distance;
    Float along;
    int kind;
    int i;
    struct path *other;
    struct race_path_span *span;
    NJS_VECTOR delta;

    best =
        path_find_nearest(paths[path->count], point, NULL, &position->distance);
    position->along = position->distance;
    position->path = paths[path->count];
    if (!path->spans) {
        return;
    }
    if (!race_get_kind(&kind)) {
        kind = 4;
    }
#ifdef GCC
    /* repairs the unset read the tag in the loop names. along starts at 0,
     * how far a path's first point lies from its start, until
     * path_find_nearest first sets it */
    along = 0.0F;
#endif
    for (i = 0; i < path->count; i++) {
        int allowed = race_kind_paths[kind];
        int bit;
        if (mode != LOCATE_ALL) {
            allowed &= ~1;
        }
        bit = 1 << i;
        if (!(allowed & bit)) {
            continue;
        }
        /* @hack redundant: a test of mode and bit with an empty body, which the
         * image makes from 0x0cb90f48 to 0x0cb90f52; without it, 124 of 179
         * words differ */
        if (mode == LOCATE_ALL || bit == 1) {
        }
        span = &path->spans[i];
        other = paths[i];
        if (span->start >= position->distance) {
            delta.x = point->x - other->points[0].pos.x;
            delta.y = point->y - other->points[0].pos.y;
            delta.z = point->z - other->points[0].pos.z;
            distance = njSqrt(VECTOR_SQUARED(&delta));
        } else {
            distance = path_find_nearest(other, point, NULL, &along);
        }
        /* @bug uninitialized: a path measured to its first point reads along
         * as the last path measured to its nearest point left it, unset before
         * any. the load at 0x0cb9100e reads sp+0, which only
         * path_find_nearest's call at 0x0cb91008 writes */
        if (along > other->length - END_MARGIN) {
            break;
        }
        if (!(distance > best)) {
            best = distance;
            position->along = along;
            position->path = other;
        }
    }
}

Float race_path_distance(struct race_path *path, NJS_POINT3 *point)
{
    Float along;

    path_find_nearest(race_paths[path->count], point, NULL, &along);
    return along;
}

/* set point to the course's point distance along it and return 1. where
 * path_sample gives none, return 0 and leave point alone */
static int race_path_point_at(struct race_path *path, NJS_POINT3 *point,
                              Float distance)
{
    struct path_sample sample;

    sample.distance = distance;
    if (path_sample(race_paths[path->count], &sample)) {
        VECTOR_COPY(point, &sample.pos);
        return 1;
    }
    return 0;
}

int race_path_get_target(struct race_path *path, NJS_POINT3 *point)
{
    if (path->target.x == NO_TARGET) {
        VECTOR_CLEAR(point);
        return 0;
    } else {
        /* @hack redundant: an else after the return, whose copy stores the
         * target's x from FR2, where the test left it, at 0x0cb910da; without
         * it, 9 of 23 words differ */
        VECTOR_COPY(point, &path->target);
        return 1;
    }
}

struct path *race_path_course(struct race_path *path)
{
    return race_paths[path->count];
}

int race_path_at_target(struct race_path *path, NJS_POINT3 *point)
{
    NJS_VECTOR delta;

    if (path->target.x == NO_TARGET) {
        return 1;
    } else {
        /* @hack redundant: an else after the return, which loads the target's x
         * again, into FR1, at 0x0cb91102; without it, 23 of 45 words differ */
        VECTOR_DIFF(&delta, &path->target, point);
        if (ACROSS_SQUARED(&delta) < SQUARE(TARGET_RANGE)) {
            return 1;
        }
        return 0;
    }
}

void race_path_free(struct race_path *path)
{
    if (path->spans) {
        heap_free(path->spans);
    }
    path->spans = NULL;
}

void race_path_set_target(struct race_path *path, Float distance)
{
    if (!race_path_point_at(path, &path->target, distance)) {
        CLEAR_TARGET(path);
    }
}

/* set span to how far along course path's first and last points lie. a null
 * path leaves span alone */
static void race_path_measure(struct race_path_span *span, struct path *path,
                              struct path *course)
{
    /* @hack spelling: last declared before first, which puts first at sp+8 and
     * last at sp+20 (0x0cb911e2, 0x0cb91206); declared in order, 2 of 58
     * words differ */
    NJS_POINT3 last;
    NJS_POINT3 first;

    if (path) {
        Uint16 count = path->count;
        struct path_point *points = path->points;
        /* @hack spelling: first and last reached through from and to, which
         * hold their addresses in R5 and R14 for the copies and the calls
         * (0x0cb911da, 0x0cb91206); by name, the function grows and 58 of 58
         * words differ */
        NJS_POINT3 *from = &first;
        NJS_POINT3 *to = &last;
        struct path_point *end = &points[count - 1];

        /* @hack redundant: a goto to the next statement, which compiles to no
         * branch but keeps count in R7, not R6, at 0x0cb911dc; without it, 2
         * of 58 words differ */
        goto copy;
    copy:
        VECTOR_COPY(from, &points[0].pos);
        VECTOR_COPY(to, &end->pos);
        path_find_nearest(course, from, NULL, &span->start);
        path_find_nearest(course, to, NULL, &span->end);
    }
}
