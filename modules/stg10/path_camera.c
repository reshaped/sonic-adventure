/* path cameras that run the camera along a stretch of a Final Egg path */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/polygon.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg10/path_camera.h"
#include "system/task.h"
#include "system/util.h"

/* in a stretch's flags: a wall stops the camera on its way to its place */
#define PATH_CAMERA_WALLS 0x1
/* the task takes the camera only away from the stretch's ends, and gives it
 * back at either end */
#define PATH_CAMERA_ENDS 0x2

/* a stretch of a path, and how the camera runs along it. the stretch runs
 * from point first to the point skip before the path's last. the camera
 * follows a point above the player. it sits by the stretch, sit_back from
 * that point toward the path's start. it looks at the stretch, look_ahead
 * from that point toward the path's end */
struct final_path_camera {
    Sint32 flags;
    Float look_ahead;
    Float sit_back;
    /* shares of the followed point's offset from the stretch. each is added
     * to the place the camera looks at or sits at */
    Float look_share;
    Float sit_share;
    /* the followed point lies height + 0.01 above the player, along its up.
     * the sweep PATH_CAMERA_WALLS makes has radius height */
    Float height;
    Float turn; /* the share of its remaining turn the camera takes a step */
    Float ease; /* the share of the way to its place the camera moves a step */
    Sint16 wait; /* steps the task holds the camera before it sets adjust */
    Sint8 first_adjust; /* the adjust mode the task takes the camera with */
    Uint8 adjust; /* the adjust mode the task sets each step after wait */
    /* the player's distance from the stretch within which the task takes the
     * camera */
    Float reach;
    /* the followed point's distance from the stretch beyond which the task
     * gives the camera back */
    Float leave;
    Sint32 first;
    Sint32 skip;
};

/* the task's place along the stretch, on the heap from init until the task
 * is destroyed. each index names the segment that starts at that point */
struct final_path_camera_track {
    int follow_index; /* the segment nearest the followed point */
    int sit_index; /* the segment the camera sits by */
    int look_index; /* the segment the camera looks at */
    /* the direction toward the path's start. each step it keeps
     * DIRECTION_KEPT of itself */
    NJS_VECTOR direction;
};

/* a path camera's view of its task_work, with stage_act and path where
 * path_vars has them */
struct final_path_camera_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 held; /* HELD while the task holds the camera */
    Sint16 flags;
    /* steps since init, or since the task last took or gave back the camera.
     * the count holds at 65535 */
    Uint16 timer;
    struct final_path_camera *camera;
    Sint16 stage_act;
    Uint8 unk_0e[2];
    struct path *path;
    /* the corners of the box around the stretch's points */
    NJS_POINT3 low;
    NJS_POINT3 high;
    NJS_POINT3 scl;
};

#define CAMERA(task) ((struct final_path_camera_work *)(task)->work)
#define HELD 0x1
/* @hack type: the track, kept in scl.x's word and reached through a cast of
 * that word's address. init forms the address in R8 before its call to
 * heap_alloc, and stores the track through it after (0x0c916db2 and
 * 0x0c916dc0). through a pointer field in scl's place, init stores by
 * displacement after the call, and 732 of the 800 words of the four
 * functions that use it differ */
#define TRACK(work) (*(struct final_path_camera_track **)&(work)->scl.x)

/* the task can take the camera only after more than this many steps. it
 * counts them from init, or from when it last gave the camera back */
#define REST 60

/* the share of its direction the track keeps each step. the rest comes from
 * the followed segment's direction */
#define DIRECTION_KEPT 0.7F

static void final_path_camera_init(struct task *task);
static Bool final_path_camera_take(struct final_path_camera_work *work,
                                   struct task_work *player,
                                   struct task_work *camera);
static Bool final_path_camera_follow(struct final_path_camera_work *work,
                                     struct task_work *player,
                                     struct task_work *camera);

/* the two stretches. the first runs the whole path, and the second leaves
 * ten points off each end */
static struct final_path_camera final_path_cameras[2] = {
    {PATH_CAMERA_ENDS, 55.0F, 45.0F, 0.6F, 0.9F, 15.0F, 0.1F, 0.5F, 180, 3, 0,
     260.0F, 300.0F, 0, 0},
    {PATH_CAMERA_ENDS, 30.0F, 70.0F, 1.0F, 0.3F, 10.0F, 0.1F, 0.3F, 180, 3, 3,
     100.0F, 150.0F, 10, 10},
};

void final_path_camera_0(struct task *task)
{
    struct final_path_camera *camera = &final_path_cameras[0];
    struct final_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    final_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to init and its RTS (0x0c916a46). without it, SHC branches to init with a
     * BRA, the function runs 8 bytes short, and 8 of 8 words differ */
    (void)0;
}

void final_path_camera_1(struct task *task)
{
    struct final_path_camera *camera = &final_path_cameras[0];
    struct final_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    final_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to init and its RTS (0x0c916a56). without it, SHC branches to init with a
     * BRA, the function runs 8 bytes short, and 8 of 8 words differ */
    (void)0;
}

void final_path_camera_2(struct task *task)
{
    struct final_path_camera *camera = &final_path_cameras[1];
    struct final_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    final_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to init and its RTS (0x0c916a66). without it, SHC branches to init with a
     * BRA, the function runs 8 bytes short, and 8 of 8 words differ */
    (void)0;
}

void final_path_camera_3(struct task *task)
{
    struct final_path_camera *camera = &final_path_cameras[1];
    struct final_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    final_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to init and its RTS (0x0c916a76). without it, SHC branches to init with a
     * BRA, the function runs 8 bytes short, and 8 of 8 words differ */
    (void)0;
}

/* floats_copy sets to = from, and floats_diff to = from - by. each works
 * over three floats in order */
/* @hack spelling: floats_copy and floats_diff, helpers the pragma has SHC
 * expand at each call, as the image does. without it, SHC writes the two as
 * functions of their own: the code runs 64 bytes short and 334 of
 * final_path_camera_walk's and final_path_camera_follow's 841 words
 * differ */
#pragma inline(floats_copy, floats_diff)

static void floats_copy(Float *to, Float *from)
{
    *to++ = *from++;
    *to++ = *from++;
    *to = *from;
}

static void floats_diff(Float *to, Float *from, Float *by)
{
    *to++ = *from++ - *by++;
    *to++ = *from++ - *by++;
    *to = *from - *by;
}

/* set *low and *high to the box around a stretch of path, grown by margin on
 * each side. the stretch runs from point first to the point skip before
 * path's last */
static void final_path_camera_box(struct path *path, int first, int skip,
                                  NJS_VECTOR *margin, NJS_POINT3 *low,
                                  NJS_POINT3 *high)
{
    struct path_point *point;
    Float low_x;
    Float high_x = low_x = path->points[first].pos.x;
    Float low_y;
    Float high_y = low_y = path->points[first].pos.y;
    Float low_z;
    Float high_z = low_z = path->points[first].pos.z;
    int last = path->count - skip;
    int i;

    for (i = first, point = &path->points[first]; i < last; point++, i++) {
        if (point->pos.x > high_x) {
            high_x = point->pos.x;
        }
        if (point->pos.y > high_y) {
            high_y = point->pos.y;
        }
        if (point->pos.z > high_z) {
            high_z = point->pos.z;
        }
        if (point->pos.x < low_x) {
            low_x = point->pos.x;
        }
        if (point->pos.y < low_y) {
            low_y = point->pos.y;
        }
        if (point->pos.z < low_z) {
            low_z = point->pos.z;
        }
    }
    /* @hack redundant: tests of the bounds that never run, which keep R11 for
     * the argument at @(16,R15) and FR8 for the load at 0x0c916a9a (0x0c916a82
     * to 0x0c916a9a); without them, R13 and FR6 take those and 30 of 89 words
     * differ */
    if (0) {
        if (high_x) {
        }
        if (high_x) {
        }
        if (high_y) {
        }
        if (high_z) {
        }
        if (high_z) {
        }
        if (high_z) {
        }
        if (low_x) {
        }
        if (low_y) {
        }
        if (low_y) {
        }
    }
    low_x -= margin->x;
    low_y -= margin->y;
    low_z -= margin->z;
    high_y += margin->y;
    high_z += margin->z;
    high_x += margin->x;
    low->x = low_x;
    low->y = low_y;
    low->z = low_z;
    high->x = high_x;
    high->y = high_y;
    high->z = high_z;
}

/* whether pos lies within margin of the box from low to high */
static Bool final_path_camera_in_box(NJS_POINT3 *pos, NJS_POINT3 *low,
                                     NJS_POINT3 *high, NJS_VECTOR *margin)
{
    /* @hack spelling: a cursor over low's floats, for the image's
     * post-increment loads; through the members, 31 of 38 words differ */
    Float *bottom = VECTOR_FLOATS(low);
    /* @hack spelling: a cursor over high's floats, the same way; through the
     * members, 24 of 38 words differ */
    Float *top = VECTOR_FLOATS(high);
    /* @hack spelling: one cursor over pos's floats and then margin's, the same
     * way; through the members, 38 of 38 words differ */
    Float *walk = VECTOR_FLOATS(pos);
    Float x = *walk++;
    Float y = *walk++;
    Float z = *walk;
    Float margin_x;
    Float margin_y;
    Float margin_z;

    walk = VECTOR_FLOATS(margin);
    margin_x = *walk++;
    margin_y = *walk++;
    margin_z = *walk;
    if (x > *top++ + margin_x) {
        return FALSE;
    }
    if (y > *top++ + margin_y) {
        return FALSE;
    }
    if (z > *top + margin_z) {
        return FALSE;
    }
    if (x < *bottom++ - margin_x) {
        return FALSE;
    }
    if (y < *bottom++ - margin_y) {
        return FALSE;
    }
    if (z < *bottom - margin_z) {
        return FALSE;
    }
    return TRUE;
}

/* the squared distance between two points */
static Float final_path_camera_distance_squared(NJS_POINT3 *one,
                                                NJS_POINT3 *other)
{
    /* @hack spelling: cursors over the two points' floats, for the image's
     * post-increment loads; through the members, the function runs 4 bytes long
     * and 16 of 17 words differ */
    Float *from = VECTOR_FLOATS(one);
    Float *to = VECTOR_FLOATS(other);
    Float x = *from++ - *to++;
    Float y = *from++ - *to++;
    Float z = *from - *to;

    return (x * x) + (y * y) + (z * z);
}

/* the squared distance from x, y, z to point's pos */
static Float final_path_camera_point_distance_squared(Float x, Float y, Float z,
                                                      struct path_point *point)
{
    x -= point->pos.x;
    y -= point->pos.y;
    z -= point->pos.z;
    return (x * x) + (y * y) + (z * z);
}

/* return the index of the point nearest pos, from point first to the point
 * skip before path's last. of points equally near, the first wins.
 * *distance gets pos's distance from it, unless distance is null */
static int final_path_camera_nearest(NJS_POINT3 *pos, struct path *path,
                                     int first, int skip, Float *distance)
{
    Float x;
    Float y;
    Float z;
    Float nearest_squared;
    Float squared;
    int last;
    int nearest;
    int i;
    struct path_point *point;
    /* @hack spelling: a cursor over pos's floats, for the image's
     * post-increment loads; through the members, the function runs 4 bytes long
     * and 62 of 72 words differ */
    Float *walk = VECTOR_FLOATS(pos);

    x = *walk++;
    y = *walk++;
    z = *walk;
    last = path->count - skip;
    point = &path->points[first];
    nearest = first;
    nearest_squared = final_path_camera_point_distance_squared(x, y, z, point);
    point++;
    for (i = first + 1; i < last; i++) {
        squared = final_path_camera_point_distance_squared(x, y, z, point);
        point++;
        if (nearest_squared > squared) {
            nearest_squared = squared;
            nearest = i;
        }
    }
    if (distance) {
        *distance = njSqrt(nearest_squared);
    }
    return nearest;
}

/* the task's step from init on. the task ends once the stage or act changes.
 * while the camera runs at CAMERA_LEVEL_COMP, the step only clears
 * camera_pose_taken. while the task doesn't hold the camera, it tries to take
 * it for player 0. it tries only after more than REST steps, with player 0
 * within reach of the stretch's box. it never tries at CAMERA_LEVEL_EVENT.
 * while it holds the camera, it gives it back once final_path_camera_follow
 * returns FALSE */
static void final_path_camera_step(struct task *task)
{
    struct final_path_camera_work *work = CAMERA(task);
    struct task_work *player = player_work[0];
    struct task_work *camera = camera_work;
    NJS_VECTOR margin;

    if (stage_get_stage_act() != work->stage_act) {
        task_kill(task);
        return;
    }
    if (camera_at_comp_level()) {
        camera_pose_taken = FALSE;
        return;
    }
    if (!(work->held & HELD)) {
        if (!camera_at_event_level() && work->timer > REST) {
            margin.x = work->camera->reach;
            margin.y = work->camera->reach;
            margin.z = work->camera->reach;
            if (final_path_camera_in_box(&player->pos, &work->low, &work->high,
                                         &margin) &&
                final_path_camera_take(work, player, camera)) {
                work->timer = 0;
                work->held |= HELD;
            }
        }
    } else {
        if (!final_path_camera_follow(work, player, camera)) {
            camera_restore();
            work->timer = 0;
            work->held &= ~HELD;
        }
    }
    /* count steps, holding at 65535 rather than wrapping to 0 */
    if (++work->timer <= 0) {
        work->timer--;
    }
}

static void final_path_camera_destroy(struct task *task)
{
    struct final_path_camera_work *work = CAMERA(task);

    heap_free(TRACK(work));
}

static void final_path_camera_init(struct task *task)
{
    struct final_path_camera_work *work = CAMERA(task);
    NJS_VECTOR margin;

    work->timer = 0;
    work->held = 0;
    margin.x = 0.0F;
    margin.y = 0.0F;
    margin.z = 0.0F;
    TRACK(work) = heap_alloc(sizeof(struct final_path_camera_track));
    final_path_camera_box(work->path, work->camera->first, work->camera->skip,
                          &margin, &work->low, &work->high);
    task->step = final_path_camera_step;
    task->destructor = final_path_camera_destroy;
}

/* TRUE at either end of the stretch, and FALSE otherwise. the segment at
 * index is at the start when it starts at or before point first. it is at
 * the end when it starts at one of the stretch's last three points */
static Bool final_path_camera_at_end(int index, int first, int skip,
                                     struct final_path_camera_work *work)
{
    int last = work->path->count - skip - 3;

    if (index >= last || index <= first) {
        return TRUE;
    }
    return FALSE;
}

/* set *out to the point nearest pos on the line through the segment at
 * *index. an index at or past the stretch's last point counts as the segment
 * before it. direction, unless null, gets the vector from the segment's end
 * to its start. when *out lies past an end of the segment, *index steps one
 * segment that way. *out then becomes the point the two segments share. but
 * where *index reaches point first or the stretch's last point, *out stays on
 * the line. returns TRUE when *out became a shared point, and FALSE
 * otherwise */
static Bool final_path_camera_walk(int *index, int first, int skip,
                                   NJS_POINT3 *out, NJS_POINT3 *pos,
                                   NJS_VECTOR *direction,
                                   struct final_path_camera_work *work)
{
    int last = work->path->count - skip - 1;
    int i = *index;
    int next;
    struct path_point *start;
    struct path_point *end;
    NJS_POINT3 at_start;
    NJS_POINT3 at_end;
    NJS_POINT3 nearest;
    union line_view line;
    Float length;
    Float to_start;
    Float to_end;

    if (i >= last) {
        i = last - 1;
    }
    next = i + 1;
    start = &work->path->points[i];
    end = &work->path->points[next];
    /* @hack spelling: the two path points copied through floats_copy, for the
     * image's post-increment loads; as struct copies, the function runs 32
     * bytes short and 179 of 211 words differ */
    floats_copy(VECTOR_FLOATS(&at_start), VECTOR_FLOATS(&start->pos));
    floats_copy(VECTOR_FLOATS(&at_end), VECTOR_FLOATS(&end->pos));
    /* @hack spelling: the line's point and direction written through
     * floats_copy and floats_diff, the same way; through the members, 150 of
     * 211 words differ */
    floats_copy(line.coordinates, VECTOR_FLOATS(&at_start));
    floats_diff(&line.coordinates[3], VECTOR_FLOATS(&at_start),
                VECTOR_FLOATS(&at_end));
    /* @hack type: the union's address converted to NJS_LINE *, which SHC forms
     * from R15 at each use; as &line.line, SHC keeps it in a saved register,
     * and 205 of 211 words differ */
    njDistanceP2L(pos, (NJS_LINE *)&line, &nearest);
    length = final_path_camera_distance_squared(&at_start, &at_end);
    to_start = final_path_camera_distance_squared(&at_start, &nearest);
    to_end = final_path_camera_distance_squared(&at_end, &nearest);
    if (direction) {
        /* @hack spelling: cursors over the points' floats, the same way;
         * through the members, 200 of 211 words differ */
        Float *to = VECTOR_FLOATS(direction);
        Float *from = VECTOR_FLOATS(&at_start);
        Float *by = VECTOR_FLOATS(&at_end);

        *to++ = *from++ - *by++;
        *to++ = *from++ - *by++;
        *to = *from - *by;
    }
    *index = i;
    *out = nearest;
    if (to_start + to_end > length) {
        if (to_start > to_end) {
            *index = i + 1;
            if (*index < last) {
                *out = at_end;
            } else {
                return FALSE;
            }
        } else {
            *index = i > first ? i - 1 : first;
            if (*index > first) {
                *out = at_start;
            } else {
                return FALSE;
            }
        }
        return TRUE;
    }
    return FALSE;
}

/* return the index of the segment the task takes as nearest pos. the search
 * starts at the stretch's point nearest pos. from there it calls
 * final_path_camera_walk once, and again if that call returns TRUE. *nearest
 * gets the last call's *out. *direction gets the vector from the end of the
 * last call's segment to its start. *distance gets the distance from pos to
 * *nearest. direction and distance may be null */
static int final_path_camera_find(NJS_POINT3 *pos, NJS_POINT3 *nearest,
                                  NJS_VECTOR *direction, Float *distance,
                                  struct final_path_camera_work *work)
{
    int index;
    Float point_distance;

    index = final_path_camera_nearest(pos, work->path, work->camera->first,
                                      work->camera->skip, &point_distance);
    if (final_path_camera_walk(&index, work->camera->first, work->camera->skip,
                               nearest, pos, direction, work)) {
        final_path_camera_walk(&index, work->camera->first, work->camera->skip,
                               nearest, pos, direction, work);
    }
    if (distance) {
        *distance = njDistanceP2P(nearest, pos);
    }
    return index;
}

/* out = from + by * scale */
static void final_path_camera_add(NJS_POINT3 *out, NJS_POINT3 *from,
                                  NJS_VECTOR *by, Float scale)
{
    /* @hack spelling: one cursor over from's floats, then by's and out's, for
     * the image's post-increment loads and its stores through out's address
     * stepped by 4; through the members, the function runs 8 bytes long and 18
     * of 19 words differ */
    Float *walk = VECTOR_FLOATS(from);
    Float x;
    Float by_x;
    Float y;
    Float by_y;
    Float z;
    Float by_z;

    x = *walk++;
    y = *walk++;
    z = *walk;
    walk = VECTOR_FLOATS(by);
    by_x = *walk++;
    by_y = *walk++;
    by_z = *walk;
    {
        Float out_x = x + (scale * by_x);
        Float out_y = y + (scale * by_y);
        Float out_z = z + (scale * by_z);

        walk = VECTOR_FLOATS(out);
        *walk++ = out_x;
        *walk++ = out_y;
        *walk = out_z;
    }
}

/* out = from + (to - from) * share */
static void final_path_camera_blend(NJS_VECTOR *out, NJS_VECTOR *to,
                                    NJS_VECTOR *from, Float share)
{
    /* @hack spelling: one cursor over to's floats, then from's and out's, as in
     * final_path_camera_add; through the members, the function runs 6 bytes
     * long and 21 of 21 words differ */
    Float *walk = VECTOR_FLOATS(to);
    Float x;
    Float from_x;
    Float y;
    Float from_y;
    Float z;
    Float from_z;

    x = *walk++;
    y = *walk++;
    z = *walk;
    walk = VECTOR_FLOATS(from);
    from_x = *walk++;
    from_y = *walk++;
    from_z = *walk;
    {
        Float off_x = x - from_x;
        Float off_y = y - from_y;
        Float off_z = z - from_z;
        Float out_x = from_x + (share * off_x);
        Float out_y = from_y + (share * off_y);
        Float out_z = from_z + (share * off_z);

        walk = VECTOR_FLOATS(out);
        *walk++ = out_x;
        *walk++ = out_y;
        *walk = out_z;
    }
}

/* take the camera when player is within reach of the stretch. with
 * PATH_CAMERA_ENDS, player must also be away from either end. the pose starts
 * at the camera's own position and angles. returns TRUE once the camera is
 * taken, and FALSE otherwise. either way, the track starts from the segment
 * nearest player. the parameter camera goes unread: take reads camera_work */
static Bool final_path_camera_take(struct final_path_camera_work *work,
                                   struct task_work *player,
                                   struct task_work *camera)
{
    Float distance;
    NJS_POINT3 nearest;
    NJS_VECTOR direction;
    int index;

    index = final_path_camera_find(&player->pos, &nearest, &direction,
                                   &distance, work);
    TRACK(work)->follow_index = TRACK(work)->sit_index =
        TRACK(work)->look_index = index;
    /* @hack spelling: the direction stored through the address of the track's
     * vector, which SHC adds its offset to for each store, as the image does;
     * as TRACK(work)->direction's members, 76 of 105 words differ */
    (&TRACK(work)->direction)->x = direction.x;
    (&TRACK(work)->direction)->y = direction.y;
    (&TRACK(work)->direction)->z = direction.z;
    if (work->camera->flags & PATH_CAMERA_ENDS) {
        if (final_path_camera_at_end(TRACK(work)->follow_index,
                                     work->camera->first, work->camera->skip,
                                     work)) {
            return FALSE;
        }
    }
    if (distance < work->camera->reach) {
        camera_override(camera_step_pose, work->camera->first_adjust,
                        CAMERA_BY_STEP);
        camera_pose_taken = FALSE;
        camera_pose_pos = camera_work->pos;
        camera_pose_ang = camera_work->ang;
        return TRUE;
    }
    return FALSE;
}

/* set the pose camera_step_pose gives the camera this step. returns FALSE
 * once the player has left the stretch, and TRUE otherwise. the player
 * leaves when the followed point is farther than leave from the stretch.
 * with PATH_CAMERA_ENDS, the player also leaves at either end. while
 * camera_pose_taken is set, each of the track's segments walks on from where
 * it was. otherwise, as on the first step after the take, each is found
 * afresh */
static Bool final_path_camera_follow(struct final_path_camera_work *work,
                                     struct task_work *player,
                                     struct task_work *camera)
{
    NJS_POINT3 sit_from;
    NJS_POINT3 place;
    NJS_POINT3 sit;
    NJS_POINT3 nearest;
    NJS_POINT3 followed;
    NJS_VECTOR offset;
    NJS_VECTOR sit_offset;
    NJS_VECTOR up;
    NJS_POINT3 look_from;
    NJS_POINT3 aim;
    NJS_POINT3 look;
    NJS_VECTOR look_offset;
    struct angles ang;
    NJS_VECTOR way;
    NJS_VECTOR to;
    Angle rotate;
    Angle turn;
    Float dx;
    Float dz;
    Float share;
    Float above;
    Bool gone = FALSE;

    if (!camera_pose_taken) {
        Float distance;
        int index;

        index =
            final_path_camera_find(&player->pos, &way, &to, &distance, work);
        TRACK(work)->follow_index = TRACK(work)->sit_index =
            TRACK(work)->look_index = index;
    }
    way.x = 0.0F;
    way.y = work->camera->height + 0.01F;
    way.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &player->pos);
    TURN_VIA(njRotateZ, player->ang.z, rotate);
    TURN_VIA(njRotateX, player->ang.x, rotate);
    njCalcPoint(NULL, &way, &followed);
    njCalcVector(NULL, &way, &up);
    njPopMatrix(1);
    /* @hack redundant: three tests of player that never run, which keep the
     * loads at 0x0c9176b8 and 0x0c917704 in FR14 and FR13; without them, the
     * two trade and 8 of 638 words differ */
    if (0) {
        if (player) {
        }
        if (player) {
        }
        if (player) {
        }
    }
    if (camera_pose_taken) {
        if (final_path_camera_walk(&TRACK(work)->follow_index,
                                   work->camera->first, work->camera->skip,
                                   &nearest, &followed, &way, work)) {
            final_path_camera_walk(&TRACK(work)->follow_index,
                                   work->camera->first, work->camera->skip,
                                   &nearest, &followed, &way, work);
        }
    } else {
        final_path_camera_walk(&TRACK(work)->follow_index, work->camera->first,
                               work->camera->skip, &nearest, &followed, &way,
                               work);
    }
    njUnitVector(&way);
    final_path_camera_blend(&TRACK(work)->direction, &TRACK(work)->direction,
                            &way, DIRECTION_KEPT);
    njUnitVector(&TRACK(work)->direction);
    if (work->camera->flags & PATH_CAMERA_ENDS) {
        if (final_path_camera_at_end(TRACK(work)->follow_index,
                                     work->camera->first, work->camera->skip,
                                     work)) {
            gone = TRUE;
        }
    }
    if (work->camera->leave < njDistanceP2P(&followed, &nearest)) {
        gone = TRUE;
    }
    final_path_camera_add(&sit_from, &followed, &TRACK(work)->direction,
                          work->camera->sit_back);
    if (camera_pose_taken) {
        if (final_path_camera_walk(&TRACK(work)->sit_index, work->camera->first,
                                   work->camera->skip, &sit, &sit_from, NULL,
                                   work)) {
            final_path_camera_walk(&TRACK(work)->sit_index, work->camera->first,
                                   work->camera->skip, &sit, &sit_from, NULL,
                                   work);
        }
    } else {
        TRACK(work)->sit_index =
            final_path_camera_find(&sit_from, &sit, NULL, NULL, work);
    }
    final_path_camera_add(&look_from, &followed, &TRACK(work)->direction,
                          -work->camera->look_ahead);
    if (camera_pose_taken) {
        if (final_path_camera_walk(&TRACK(work)->look_index,
                                   work->camera->first, work->camera->skip,
                                   &look, &look_from, NULL, work)) {
            final_path_camera_walk(&TRACK(work)->look_index,
                                   work->camera->first, work->camera->skip,
                                   &look, &look_from, NULL, work);
        }
    } else {
        TRACK(work)->look_index =
            final_path_camera_find(&look_from, &look, NULL, NULL, work);
    }
    /* offset = followed - nearest: the followed point's offset from the
     * stretch */
    {
        /* @hack spelling: cursors over the three points' floats, for the
         * image's post-increment loads; through the members, 183 of 638 words
         * differ */
        Float *diff = VECTOR_FLOATS(&offset);
        Float *from = VECTOR_FLOATS(&followed);
        Float *by = VECTOR_FLOATS(&nearest);

        *diff++ = *from++ - *by++;
        *diff++ = *from++ - *by++;
        *diff = *from - *by;
    }
    sit_offset = offset;
    look_offset = offset;
    final_path_camera_add(&place, &sit, &sit_offset, work->camera->sit_share);
    final_path_camera_add(&aim, &look, &look_offset, work->camera->look_share);
    /* with PATH_CAMERA_WALLS, sweep a ball of radius height along the
     * camera's move to its place. a hit stops the move at the wall. a place
     * 0.1 or more below player 0, along its ground's normal, keeps the whole
     * move */
    if (work->camera->flags & PATH_CAMERA_WALLS) {
        NJS_VECTOR from_player;

        to = camera->pos;
        {
            /* @hack spelling: way = place - to through cursors, the same way;
             * through the members, 239 of 638 words differ */
            Float *diff = VECTOR_FLOATS(&way);
            Float *from = VECTOR_FLOATS(&place);
            Float *by = VECTOR_FLOATS(&to);

            *diff++ = *from++ - *by++;
            *diff++ = *from++ - *by++;
            *diff = *from - *by;
        }
        if (world_sweep_all_hit(&to, &way, NULL, work->camera->height)) {
            /* @hack spelling: from_player = place - the player's pos through
             * floats_diff, the same way; through the members, 225 of 638 words
             * differ */
            floats_diff(VECTOR_FLOATS(&from_player), VECTOR_FLOATS(&place),
                        VECTOR_FLOATS(&player_work[0]->pos));
            above =
                njInnerProduct(&player_get(0)->contact->normal, &from_player);
            if (above > -0.1F) {
                /* @hack spelling: place = the camera's pos + way through
                 * cursors, the same way; through the members, 213 of 638 words
                 * differ */
                Float *sum = VECTOR_FLOATS(&place);
                Float *from = VECTOR_FLOATS(&camera->pos);
                Float *by = VECTOR_FLOATS(&way);

                *sum++ = *from++ + *by++;
                *sum++ = *from++ + *by++;
                *sum = *from + *by;
            }
        }
    }
    final_path_camera_blend(&place, &place, &camera->pos, work->camera->ease);
    share = work->camera->turn;
    dx = place.x - aim.x;
    dz = place.z - aim.z;
    turn =
        (Angle)(atan2f(place.x - aim.x, dz) * ANGLE_PER_RADIAN) - camera->ang.y;
    /* read as a Sint16, turn's low half is the same turn the shorter way round.
     * it lies within half a turn either way. the camera turns by share of it.
     * the high half stays, since a rotation reads only the low half */
    {
        /* @hack type: the low half read and written through a Sint16 view of
         * turn's slot, its MOV.W at 0x0c917738 and 0x0c917744 before the whole
         * word is read back at 0x0c917746; as union angle_halves, 150 of 638
         * words differ */
        Angle *half = &turn;
        Float scaled = *(Sint16 *)half;

        scaled *= share;
        *(Sint16 *)half = (Sint16)scaled;
    }
    ang.y = turn + camera->ang.y;
    turn = (Angle)(atan2f(aim.y - place.y, njSqrt((dx * dx) + (dz * dz))) *
                   ANGLE_PER_RADIAN) -
           camera->ang.x;
    {
        /* @hack type: the pitch's low half the same way, its MOV.W at
         * 0x0c91777a and 0x0c917786 and the word at 0x0c917788; as union
         * angle_halves, 92 of 638 words differ */
        Angle *half = &turn;
        Float scaled = *(Sint16 *)half;

        scaled *= share;
        *(Sint16 *)half = (Sint16)scaled;
    }
    ang.x = turn + camera->ang.x;
    ang.z = 0;
    camera_pose_pos = place;
    camera_pose_ang = ang;
    if (work->timer >= work->camera->wait) {
        camera_set_adjust(work->camera->adjust);
    }
    camera_pose_taken = FALSE;
    return !gone;
}
