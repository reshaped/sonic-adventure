/* paths: lines of points through a stage, the tasks each act makes for
 * them, and places along them by distance */
#ifndef PATH_PATH_H
#define PATH_PATH_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* what a path does at its last point: stop there, or run on to its first */
enum path_ends { PATH_STOP, PATH_LOOP };

/* up at a point is the y axis turned by ang_x about x, then by ang_z about
 * z */
struct path_point {
    Sint16 ang_x;
    Sint16 ang_z;
    Float length; /* to the next point, or from a loop's last to its first */
    NJS_POINT3 pos;
};

struct path {
    Uint8 ends; /* a path_ends */
    Uint16 count;
    Float length;
    struct path_point *points;
    /* the step of the task path_create_tasks makes for the path, or null
     * for none */
    void (*step)(struct task *task);
};

struct path_vars {
    /* a count of steps for each of players 0 and 1. a path task that
     * doesn't hold the player counts the steps since it let the player go,
     * and stops at the task's wait. a camera stretch's count goes on past its
     * wait while the camera runs at the comp or event level. a stretch that
     * holds the player counts instead the steps on which camera_step_path's
     * done bits were clear, and stops at 3 */
    Uint16 wait[2];
    Sint16 stage_act; /* the stage and act the task was made for */
    struct path *path;
};

/* a path task's vars, in its work */
#define PATH_VARS(work) ((struct path_vars *)&(work)->vars)

struct path_spline_point {
    /* the length of the span this point is the middle control point of */
    Float length;
    NJS_POINT3 pos;
};

/* a path as a quadratic b-spline. span i is the curve that points i - 1 to
 * i + 1 shape */
struct path_spline {
    Uint8 flags; /* bits 2 and 3 hold a path_ends */
    Uint16 count;
    Float length;
    Float reach; /* how far behind and ahead up is measured, in spans */
    Uint8 unk_0c[4];
    struct path_spline_point *points;
    /* a second spline beside it, to say which way is up */
    struct path_spline_point *side;
};

/* a place along a path. the caller sets distance, and the sampler fills in
 * the rest. path_sample takes ang_x and ang_z from the point the segment
 * starts at, and turns smooth_x and smooth_z smoothly along the segment.
 * path_spline_sample takes ang_x and ang_z from up, and sets smooth_x and
 * smooth_z equal to them */
struct path_sample {
    Angle ang_x;
    Angle ang_z;
    Angle smooth_x;
    Angle smooth_z;
    Float distance;
    NJS_POINT3 pos_copy; /* equal to pos: each sampler writes both */
    NJS_POINT3 pos;
    NJS_VECTOR up; /* the way up ang_x and ang_z give, as at a path_point */
    NJS_VECTOR smooth_up; /* the way up smooth_x and smooth_z give */
    NJS_VECTOR ahead; /* a unit vector the way the path runs */
};

/* do nothing */
void fun_8c08e0e0(void);
/* run a cubic spline's setup, solve and walk on knots and samples in its
 * own frame, and keep no result. nothing sets the count or the knots'
 * points, so the run reads whatever the stack holds */
void path_run_cubic(void);
/* create the task that makes the path tasks of the act, and makes them again
 * whenever the stage or act changes. returns TRUE, or FALSE when
 * task_create fails */
Bool path_create_manager(void);
/* fill sample at sample->distance along path. a loop's distance first
 * wraps into its length, in place. a point whose length is 0 counts as 1
 * long. returns TRUE, or FALSE with the sample left alone when the distance
 * is off a path that stops */
Bool path_sample(struct path *path, struct path_sample *sample);
/* set *index to the point that starts the segment holding distance, and
 * return TRUE. a distance past the points' lengths summed sets count, after
 * reading one point past the last */
Bool path_find_segment(struct path *path, Float distance, Uint32 *index);
/* set *distance to how far along path point index is, the lengths of the
 * points before it summed, and return TRUE. returns FALSE, leaving
 * *distance alone, when index > count */
Bool path_get_distance(struct path *path, Uint32 index, Float *distance);
/* distance, wrapped into 0 to the path's length as float_wrap wraps it */
Float path_wrap(struct path *path, Float distance);
/* return how far pos is from path. unless null, nearest gets the point of
 * the path nearest pos, and along that point's distance along the path. a
 * path that stops needs 2 points or more */
Float path_find_nearest(struct path *path, NJS_POINT3 *pos, NJS_POINT3 *nearest,
                        Float *along);
/* path_sample for a b-spline, which needs 5 points or more. up is the
 * normal of the strip between the spline and its side, from reach behind
 * the place to reach ahead of it. smooth_up, smooth_x and smooth_z equal up,
 * ang_x and ang_z. returns FALSE, leaving the sample alone, when the
 * distance is off a spline that stops. otherwise it returns a nonzero
 * value other than TRUE: the address of a pool word, which R0 keeps from
 * the MOVA at 0x8c08ee6a */
Bool path_spline_sample(struct path_spline *spline, struct path_sample *sample);

#endif
