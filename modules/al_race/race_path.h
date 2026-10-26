/* the race's paths: the course, the other paths beside it, and a target on
 * the course */
#ifndef AL_RACE_RACE_PATH_H
#define AL_RACE_RACE_PATH_H
#include <ninja.h>

struct path;

/* how far along the course another path's first and last points lie */
struct race_path_span {
    Float start;
    Float end;
};

/* a race's paths. the spans and race_path_locate take the course from
 * paths[count]. race_path_distance, race_path_course and the target take it
 * from race_paths[count], course.h's list of the race's paths. the two are
 * one course only where paths[count] is race_paths[count] */
struct race_path {
    NJS_POINT3 target; /* x is -10000 when there's none */
    struct race_path_span *spans; /* count spans from the heap, or null */
    struct path **paths; /* count other paths, then the course; borrowed */
    int count;
};

/* where a point lies on the race's paths, as race_path_locate finds it */
struct race_path_position {
    struct path *path; /* the path a racer there follows */
    Float along; /* how far along path */
    Float distance; /* how far along the course */
};

/* race_path_locate's modes: whether it may try the first other path */
enum { LOCATE_SKIP_FIRST, LOCATE_ALL };

/* keep paths and count, and measure each other path's span into spans,
 * which comes from the heap and race_path_free frees. a null path's span
 * stays 0 to 0. then set the target at distance along the course, as
 * race_path_set_target does. with count 0 or no memory, spans is null and
 * there is no target */
void race_path_init(struct race_path *path, struct path **paths, int count,
                    Float distance);
/* find where point lies on the race's paths. position starts on the
 * course, with along and distance both how far along it point's nearest
 * point lies. with spans, race_path_locate then tries in order each other
 * path the race's kind allows. kind 0 allows none, 1 the first, 2 and 3 the
 * second, and 4 both. with no race, the kind is 4. LOCATE_SKIP_FIRST never
 * tries the first.
 *
 * a tried path is measured to its nearest point, which gives its along. a
 * path whose span starts at or past position's distance is measured from
 * point to its first point instead. such a path's along is the one the last
 * path measured to its nearest point gave, unset before any (a @bug in
 * race_path.c).
 * an along past the path's length less 10 ends the search. otherwise a path
 * no farther from point than the nearest so far takes position's path and
 * along */
void race_path_locate(struct race_path *path, NJS_POINT3 *point,
                      struct race_path_position *position, int mode);
/* how far along the course the point on it nearest to point lies */
Float race_path_distance(struct race_path *path, NJS_POINT3 *point);
/* with a target, copy it to point and return 1; without one, zero point and
 * return 0 */
int race_path_get_target(struct race_path *path, NJS_POINT3 *point);
/* the course, race_paths[count] */
struct path *race_path_course(struct race_path *path);
/* 1 with no target, or when point is less than 40 from the target across x
 * and z; else 0 */
int race_path_at_target(struct race_path *path, NJS_POINT3 *point);
/* free spans, which may be null, and null it */
void race_path_free(struct race_path *path);
/* set the target at distance along the course, or clear it where
 * path_sample gives no point there */
void race_path_set_target(struct race_path *path, Float distance);

#endif
