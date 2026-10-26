/* Emerald Coast's path cameras, which run the camera along a curve through a
 * path's points */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "path/curve.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg01/path_camera.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* flags of a path camera's settings. with PATH_CAMERA_WALLS, a wall between
 * the camera and its place stops it at the wall */
#define PATH_CAMERA_WALLS 0x1
/* the path takes the camera only while player 0 is off the curve's ends. it
 * gives the camera back once the point it follows above player 0 reaches
 * one */
#define PATH_CAMERA_ENDS 0x2
/* the take reverses the track's direction when it points more than a quarter
 * turn from camera_work's y angle */
#define PATH_CAMERA_ALIGN 0x4
/* the take reverses the track's direction. PATH_CAMERA_ALIGN, when set,
 * decides instead */
#define PATH_CAMERA_BACKWARD 0x8
/* the searches for the nearest samples measure in x and z alone */
#define PATH_CAMERA_FLAT 0x10

/* a path camera's settings. the followed point lies height + 0.01 above
 * player 0, along his up axis. the sit point is the curve's point nearest a
 * place sit_back from there along the track's direction. the look point is
 * the curve's point nearest a place look_ahead the other way */
struct beach_path_camera {
    Sint32 flags;
    Float look_ahead;
    Float sit_back;
    /* the camera looks at the look point plus look_share times the followed
     * point's offset from the curve. it sits at the sit point plus sit_share
     * times that offset */
    Float look_share;
    Float sit_share;
    Float height;
    Float turn; /* the share of the remaining turn taken each frame */
    Float ease; /* the share of the way to its place moved each frame */
    Sint16 wait; /* frames from the take until follow sets adjust */
    Sint8 first_adjust; /* the camera's adjust mode from the take */
    Uint8 adjust; /* the adjust mode follow sets each frame from wait on */
    /* the path samples the curve once player 0 is within reach of the box
     * from low to high. it takes the camera once he is within reach of the
     * curve */
    Float reach;
    /* the path gives the camera back once the followed point is farther than
     * leave from the curve */
    Float leave;
    /* first and skip trim the path's points, but their two uses read them
     * the other way round. the box runs from point first and leaves skip
     * points off the end. the curve runs from point skip and leaves first
     * points off the end. every setting here gives both 0, so the box and the
     * curve take the same points */
    Sint32 first;
    Sint32 skip;
};

/* a track flag: the track's direction is reversed */
#define TRACK_BACKWARD 0x2

/* what a path camera keeps on the heap while its task runs. samples is NULL
 * while the curve isn't sampled */
struct beach_path_camera_track {
    /* the samples last found nearest the followed point, and those found
     * for the sit and look points. each search looks within SEARCH of its
     * last */
    int follow_index;
    int sit_index;
    int look_index;
    /* the way along the curve at the followed point, as the searches' lines
     * give it. follow makes it a unit vector */
    NJS_VECTOR direction;
    NJS_POINT3 *samples;
    int count;
    Sint32 flags;
};

/* a path task's work, as a path camera lays it out. path_create_tasks sets
 * stage_act and path, which sit where struct path_vars has them */
struct beach_path_camera_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 held; /* HELD while the path holds the camera */
    Sint16 flags;
    /* counts frames, held at 65535. the task's start, a take and a give back
     * start it at 0 */
    Uint16 timer;
    struct beach_path_camera *camera;
    Sint16 stage_act;
    Uint8 unk_0e[2];
    struct path *path;
    /* the box round the path's points from point first, leaving skip points
     * off the end */
    NJS_POINT3 low;
    NJS_POINT3 high;
    NJS_POINT3 scl; /* scl.x holds the track, as TRACK reads it */
};

/* a request to path_curve_sample, which reads its curve and not unk_18. the
 * step zeroes unk_18 anyway */
struct beach_path_camera_curve {
    struct path_curve curve;
    Sint32 unk_18;
};

#define CAMERA(task) ((struct beach_path_camera_work *)(task)->work)
#define HELD 0x1
/* @hack type: the track, kept in scl.x's word and reached through a cast of
 * that word's address. SHC forms that address from a copy of work's, as at
 * 0x0c90c876 (MOV R5,R2). through a pointer field in scl's place, read from
 * work directly, 1280 of the 1521 words of the eight functions that use it
 * differ */
#define TRACK(work) (*(struct beach_path_camera_track **)&(work)->scl.x)
/* @hack spelling: the track's direction by index, which adds its offset to the
 * track's address for each access (ADD #12, as at 0x0c90ca38). by member,
 * each is a displacement from the track, and 883 of the 1055 words of the two
 * functions that use it differ */
#define DIRECTION(work) (VECTOR_FLOATS(&TRACK(work)->direction))

/* the path samples, frees and takes nothing until its timer passes REST. the
 * timer counts frames from the task's start, and from each give back */
#define REST 20

/* path_curve_sample takes SAMPLES_PER_POINT samples of each key's span, the
 * stretch of curve after the key */
#define SAMPLES_PER_POINT 5
#define POINT_SAMPLES_SIZE (SAMPLES_PER_POINT * (int)sizeof(NJS_POINT3))
/* the curve closes, so keys from the path's other end shape its first span
 * and its last two. the track skips the first SAMPLES_SKIPPED samples: the
 * first span's, and one more */
#define SAMPLES_SKIPPED 6
/* the track drops the last SAMPLES_DROPPED samples: the last two spans', and
 * two more */
#define SAMPLES_DROPPED 12
/* the samples have room for SPARE_SPANS spans past the curve's */
#define SPARE_SPANS 3

/* a path camera samples its curve only while at most SAMPLED_MAX others hold
 * samples. so no more than SAMPLED_MAX + 1 hold them at once */
#define SAMPLED_MAX 2

/* follow's searches look at the samples within SEARCH of the ones they last
 * found nearest */
#define SEARCH 100

/* the share of its direction the track keeps each frame. follow turns the
 * rest toward the direction at the nearest sample */
#define DIRECTION_KEPT 0.1F

static void beach_path_camera_init(struct task *task);
static int beach_path_camera_find(NJS_POINT3 *pos, NJS_POINT3 *nearest,
                                  NJS_VECTOR *direction, Float *distance,
                                  struct beach_path_camera_work *work);
static int beach_path_camera_find_flat(NJS_POINT3 *pos, NJS_POINT3 *nearest,
                                       NJS_VECTOR *direction, Float *distance,
                                       struct beach_path_camera_work *work);
static Bool beach_path_camera_take(struct beach_path_camera_work *work,
                                   struct task_work *player,
                                   struct task_work *camera);
static Bool beach_path_camera_follow(struct beach_path_camera_work *work,
                                     struct task_work *player,
                                     struct task_work *camera);
static Bool beach_path_camera_release(struct beach_path_camera_work *work,
                                      struct task_work *player,
                                      struct task_work *camera);

/* the keys' format for path_curve_sample: a point's three Floats */
static const char beach_path_camera_format[] = "FFF";

/* the settings the steps give their tasks. steps 0 to 4 and 7 take the
 * first, step 5 the second and step 6 the third */
static struct beach_path_camera beach_path_cameras[3] = {
    {PATH_CAMERA_FLAT | PATH_CAMERA_ENDS | PATH_CAMERA_WALLS, 55.0F, 45.0F,
     0.7F, 0.95F, 11.0F, 0.15F, 0.6F, 180, 3, 0, 50.0F, 70.0F, 0, 0},
    {PATH_CAMERA_FLAT | PATH_CAMERA_ENDS | PATH_CAMERA_WALLS, 30.0F, 50.0F,
     0.6F, 0.9F, 12.0F, 0.2F, 0.7F, 160, 3, 0, 43.0F, 70.0F, 0, 0},
    {PATH_CAMERA_ENDS | PATH_CAMERA_WALLS, 20.0F, 55.0F, 0.7F, 0.95F, 11.0F,
     0.3F, 0.6F, 160, 3, 0, 53.0F, 120.0F, 0, 0},
};

/* how many path cameras hold samples */
static int beach_path_camera_sampled = 0;

void beach_path_camera_0(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[0];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

void beach_path_camera_1(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[0];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

void beach_path_camera_2(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[0];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

void beach_path_camera_3(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[0];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

void beach_path_camera_4(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[0];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

void beach_path_camera_5(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[1];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

void beach_path_camera_6(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[2];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

void beach_path_camera_7(struct task *task)
{
    struct beach_path_camera *camera = &beach_path_cameras[0];
    struct beach_path_camera_work *work = CAMERA(task);

    work->camera = camera;
    beach_path_camera_init(task);
    /* @hack redundant: a statement after the call, which keeps the image's BSR
     * to beach_path_camera_init and its RTS. without it, SHC branches there
     * with a BRA, and 8 of 8 words differ */
    (void)0;
}

/* set low and high to the box round path's points. the box starts at point
 * first and leaves skip points off the end. it grows by margin on each axis */
static void beach_path_camera_box(struct path *path, int first, int skip,
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
    /* @hack redundant: tests of the bounds that never run. they keep high_x,
     * high_y and high_z in FR5, FR7 and FR4, and low_x, low_y and low_z in
     * FR8, FR6 and FR9; without them, 30 of 89 words differ */
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
static Bool beach_path_camera_in_box(NJS_POINT3 *pos, NJS_POINT3 *low,
                                     NJS_POINT3 *high, NJS_VECTOR *margin)
{
    /* @hack spelling: a cursor over low's floats, for the image's
     * post-increment loads; through the members, 31 of 38 words differ */
    Float *bottom = VECTOR_FLOATS(low);
    /* @hack spelling: a cursor over high's floats, the same way; through the
     * members, 24 of 38 words differ */
    Float *top = VECTOR_FLOATS(high);
    /* @hack spelling: one cursor over pos's floats and then margin's, the same
     * way; through the members, the function runs 2 bytes long and 38 of 38
     * words differ */
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

/* nothing in the image calls it. the object holds it, at 0x0c90bf1e. it isn't
 * static, since SHC leaves an uncalled static function out of the object */
Float beach_path_camera_flat_distance_squared(NJS_POINT3 *one,
                                              NJS_POINT3 *other)
{
    /* @hack spelling: cursors over the two points' floats, for the image's
     * post-increment loads; through the members, 10 of 13 words differ */
    Float *from = VECTOR_FLOATS(one);
    Float *to = VECTOR_FLOATS(other);
    Float x = *from++ - *to++;
    Float z = from[1] - to[1];

    return (x * x) + (z * z);
}

/* the squared distance from x and z to point's x and z. y is unused */
static Float beach_path_camera_point_flat_distance_squared(Float x, Float y,
                                                           Float z,
                                                           NJS_POINT3 *point)
{
    /* @hack spelling: a cursor over point's floats, for the image's
     * post-increment load; through the members, 2 of 12 words differ */
    Float *walk = VECTOR_FLOATS(point);
    Float dx = x - *walk++;
    Float dz = z - walk[1];

    return (dx * dx) + (dz * dz);
}

/* the squared distance between two points */
static Float beach_path_camera_distance_squared(NJS_POINT3 *one,
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

/* the squared distance from x, y, z to point */
static Float beach_path_camera_point_distance_squared(Float x, Float y, Float z,
                                                      NJS_POINT3 *point)
{
    /* @hack spelling: a cursor over point's floats, for the image's
     * post-increment loads; through the members, the function runs 4 bytes long
     * and 14 of 14 words differ */
    Float *walk = VECTOR_FLOATS(point);

    x -= *walk++;
    y -= *walk++;
    z -= *walk;
    return (x * x) + (y * y) + (z * z);
}

/* return the index of the sample nearest pos in x and z. count must be 2 or
 * more, and a tie goes to the later sample. distance, unless NULL, gets the
 * distance to that sample in x and z. the runner-up is the nearest sample
 * before it. when it is the first sample, the runner-up is the second. nearest,
 * unless NULL, gets the point nearest pos on the line through the two.
 * direction, unless it or nearest is NULL, gets the way from the nearest
 * sample to the runner-up */
static int beach_path_camera_nearest_flat(NJS_POINT3 *pos, NJS_POINT3 *points,
                                          int count, Float *distance,
                                          NJS_POINT3 *nearest,
                                          NJS_VECTOR *direction)
{
    /* @hack spelling: a cursor over pos's floats, for the image's
     * post-increment loads; through the members, the function runs 6 bytes long
     * and 146 of 151 words differ */
    Float *walk = VECTOR_FLOATS(pos);
    Float x = *walk++;
    Float y = *walk++;
    Float z = *walk;
    Float first_squared;
    Float best;
    Float squared;
    int index;
    int second;
    int i;
    Float *at;

    first_squared =
        beach_path_camera_point_flat_distance_squared(x, y, z, points);
    best = beach_path_camera_point_flat_distance_squared(x, y, z, points + 1);
    if (best > first_squared) {
        index = 0;
        best = first_squared;
        second = 1;
    } else {
        index = 1;
        second = 0;
    }
    /* @hack redundant: a test of first_squared that never runs. it keeps
     * first_squared in FR13 and y in FR12. it also keeps the function 2 bytes
     * shorter, which keeps beach_path_camera_nearest_flat_near's alignment
     * (121 of its 177 words differ otherwise); without it, 72 of 151 words
     * differ */
    if (0) {
        if (first_squared) {
        }
    }
    /* @hack spelling: one Float cursor from the third sample's x, run on from
     * each sample's floats into the next's, for the image's post-increment
     * loads; through the members, the function runs 10 bytes long and 110 of
     * 151 words differ */
    at = VECTOR_FLOATS(&points[2]);
    for (i = 2; i < count; i++) {
        Float dx = x - *at++;
        Float dz;

        at++;
        dz = z - *at++;
        squared = (dx * dx) + (dz * dz);
        if (squared <= best) {
            second = index;
            best = squared;
            index = i;
        }
    }
    if (distance) {
        *distance = njSqrt(best);
    }
    if (nearest) {
        union line_view line;

        {
            /* @hack spelling: the nearest sample copied into the line through
             * cursors over their floats, for the image's post-increment loads;
             * through the members, 38 of 151 words differ */
            Float *from = VECTOR_FLOATS(&points[index]);
            Float *to = line.coordinates;

            *to++ = *from++;
            *to++ = *from++;
            *to = *from;
        }
        {
            /* @hack spelling: the line's direction, between the two samples,
             * through cursors over their floats, the same way; through the
             * members, the function runs 2 bytes long and 78 of 151 words
             * differ */
            Float *to = &line.coordinates[3];
            Float *from = VECTOR_FLOATS(&points[second]);
            Float *by = VECTOR_FLOATS(&points[index]);

            *to++ = *from++ - *by++;
            *to++ = *from++ - *by++;
            *to = *from - *by;
        }
        /* @hack type: the union's address converted to NJS_LINE *, which SHC
         * forms from R15 at each use; as &line.line, SHC keeps it in a saved
         * register, and 69 of 151 words differ */
        njDistanceP2L(pos, (NJS_LINE *)&line, nearest);
        if (direction) {
            *direction = line.split.direction;
        }
    }
    return index;
}

/* as beach_path_camera_nearest_flat, among the samples within around of
 * sample from */
static int beach_path_camera_nearest_flat_near(
    int from, int around, NJS_POINT3 *pos, NJS_POINT3 *points, int count,
    Float *distance, NJS_POINT3 *nearest, NJS_VECTOR *direction)
{
    /* @hack spelling: a cursor over pos's floats, for the image's
     * post-increment loads; through the members, the function runs 2 bytes long
     * and 91 of 177 words differ */
    Float *walk = VECTOR_FLOATS(pos);
    Float x = *walk++;
    Float y = *walk++;
    Float z = *walk;
    Float first_squared;
    Float best;
    Float squared;
    int index;
    int second;
    int i;
    Float *at;
    int first = from - around;
    int last = from + around;
    int length;

    if (first < 0) {
        first = 0;
    }
    if (last >= count) {
        last = count - 1;
    }
    length = last - first + 1;
    first_squared =
        beach_path_camera_point_flat_distance_squared(x, y, z, &points[first]);
    best = beach_path_camera_point_flat_distance_squared(x, y, z,
                                                         &points[first + 1]);
    if (best > first_squared) {
        index = first;
        best = first_squared;
        second = first + 1;
    } else {
        index = first + 1;
        second = first;
    }
    /* @hack spelling: the samples' cursor run on from one sample into the next,
     * as in beach_path_camera_nearest_flat; through the members, the function
     * runs 8 bytes long and 120 of 177 words differ */
    at = VECTOR_FLOATS(&points[first + 2]);
    for (i = 2, from = first + 2; i < length; i++, from++) {
        Float dx = x - *at++;
        Float dz;

        at++;
        dz = z - *at++;
        squared = (dx * dx) + (dz * dz);
        if (squared <= best) {
            second = index;
            best = squared;
            index = from;
        }
    }
    if (distance) {
        *distance = njSqrt(best);
    }
    if (nearest) {
        union line_view line;

        {
            /* @hack spelling: the nearest sample copied into the line through
             * cursors over their floats, for the image's post-increment loads;
             * through the members, 20 of 177 words differ */
            Float *from_point = VECTOR_FLOATS(&points[index]);
            Float *to = line.coordinates;

            *to++ = *from_point++;
            *to++ = *from_point++;
            *to = *from_point;
        }
        {
            /* @hack spelling: the line's direction, between the two samples,
             * through cursors over their floats, the same way; through the
             * members, the function runs 2 bytes long and 69 of 177 words
             * differ */
            Float *to = &line.coordinates[3];
            Float *from_point = VECTOR_FLOATS(&points[second]);
            Float *by = VECTOR_FLOATS(&points[index]);

            *to++ = *from_point++ - *by++;
            *to++ = *from_point++ - *by++;
            *to = *from_point - *by;
        }
        /* @hack type: the union's address converted to NJS_LINE *, which SHC
         * forms from R15 at each use; as &line.line, SHC keeps it in a saved
         * register, and 105 of 177 words differ */
        njDistanceP2L(pos, (NJS_LINE *)&line, nearest);
        if (direction) {
            *direction = line.split.direction;
        }
    }
    return index;
}

/* as beach_path_camera_nearest_flat, in all three axes */
static int beach_path_camera_nearest(NJS_POINT3 *pos, NJS_POINT3 *points,
                                     int count, Float *distance,
                                     NJS_POINT3 *nearest, NJS_VECTOR *direction)
{
    /* @hack spelling: a cursor over pos's floats, for the image's
     * post-increment loads; through the members, the function runs 32 bytes
     * long and 150 of 155 words differ */
    Float *walk = VECTOR_FLOATS(pos);
    Float x = *walk++;
    Float y = *walk++;
    Float z = *walk;
    Float first_squared;
    Float best;
    Float squared;
    int index;
    int second;
    int i;
    Float *at;
    /* @hack spelling: count copied to total, which keeps points in R13 and
     * second in R12. through count, 11 of 155 words differ */
    int total = count;

    first_squared = beach_path_camera_point_distance_squared(x, y, z, points);
    best = beach_path_camera_point_distance_squared(x, y, z, points + 1);
    if (best > first_squared) {
        index = 0;
        best = first_squared;
        second = 1;
    } else {
        index = 1;
        second = 0;
    }
    /* @hack redundant: seven tests of index that never run. they keep dx in
     * FR6 and dy in FR7, and the line subtraction's cursors to and from in R5
     * and R4; without them, 28 of 155 words differ */
    if (0) {
        if (index) {
        }
        if (index) {
        }
        if (index) {
        }
        if (index) {
        }
        if (index) {
        }
        if (index) {
        }
        if (index) {
        }
    }
    /* @hack spelling: the samples' cursor run on from one sample into the next,
     * as in beach_path_camera_nearest_flat; through the members, the function
     * runs 32 bytes long and 111 of 155 words differ */
    at = VECTOR_FLOATS(&points[2]);
    for (i = 2; i < total; i++) {
        Float dx = x - *at++;
        Float dy = y - *at++;
        Float dz = z - *at++;

        /* @hack redundant: a test of dz that never runs. it keeps dz in FR5
         * and dy in FR7, and the line subtraction's cursors to and from in R5
         * and R4; without it, 28 of 155 words differ */
        if (0) {
            if (dz) {
            }
        }
        squared = (dx * dx) + (dy * dy) + (dz * dz);
        if (squared <= best) {
            second = index;
            best = squared;
            index = i;
        }
    }
    if (distance) {
        *distance = njSqrt(best);
    }
    if (nearest) {
        union line_view line;

        {
            /* @hack spelling: the nearest sample copied into the line through
             * cursors over their floats, for the image's post-increment loads;
             * through the members, 41 of 155 words differ */
            Float *from = VECTOR_FLOATS(&points[index]);
            Float *to = line.coordinates;

            *to++ = *from++;
            *to++ = *from++;
            *to = *from;
        }
        {
            /* @hack spelling: the line's direction, between the two samples,
             * through cursors over their floats, the same way; through the
             * members, the function runs 32 bytes long and 73 of 155 words
             * differ */
            Float *to = &line.coordinates[3];
            Float *from = VECTOR_FLOATS(&points[second]);
            Float *by = VECTOR_FLOATS(&points[index]);

            *to++ = *from++ - *by++;
            *to++ = *from++ - *by++;
            *to = *from - *by;
        }
        /* @hack type: the union's address converted to NJS_LINE *, which SHC
         * forms from R15 at each use; as &line.line, SHC keeps it in a saved
         * register, and 73 of 155 words differ */
        njDistanceP2L(pos, (NJS_LINE *)&line, nearest);
        if (direction) {
            *direction = line.split.direction;
        }
    }
    return index;
}

/* as beach_path_camera_nearest, among the samples within around of sample
 * from */
static int beach_path_camera_nearest_near(int from, int around, NJS_POINT3 *pos,
                                          NJS_POINT3 *points, int count,
                                          Float *distance, NJS_POINT3 *nearest,
                                          NJS_VECTOR *direction)
{
    /* @hack spelling: a cursor over pos's floats, for the image's
     * post-increment loads; through the members, the function runs 8 bytes long
     * and 171 of 181 words differ */
    Float *walk = VECTOR_FLOATS(pos);
    Float x = *walk++;
    Float y = *walk++;
    Float z = *walk;
    Float first_squared;
    Float best;
    Float squared;
    int index;
    int second;
    int i;
    Float *at;
    int first = from - around;
    int last = from + around;
    int length;

    if (first < 0) {
        first = 0;
    }
    if (last >= count) {
        last = count - 1;
    }
    /* @hack spelling: the window's length in two steps, which keeps points in
     * R10 and length in R11, and dx, dy and dz in FR7, FR6 and FR5. as
     * last - first + 1, 20 of 181 words differ */
    last -= first;
    length = last + 1;
    first_squared =
        beach_path_camera_point_distance_squared(x, y, z, &points[first]);
    best =
        beach_path_camera_point_distance_squared(x, y, z, &points[first + 1]);
    if (best > first_squared) {
        index = first;
        best = first_squared;
        second = first + 1;
    } else {
        index = first + 1;
        second = first;
    }
    /* @hack spelling: the samples' cursor run on from one sample into the next,
     * as in beach_path_camera_nearest_flat; through the members, the function
     * runs 12 bytes long and 133 of 181 words differ */
    at = VECTOR_FLOATS(&points[first + 2]);
    for (i = 2, from = first + 2; i < length; i++, from++) {
        Float dx = x - *at++;
        Float dy = y - *at++;
        Float dz = z - *at++;

        squared = (dx * dx) + (dy * dy) + (dz * dz);
        if (squared <= best) {
            second = index;
            best = squared;
            index = from;
        }
    }
    if (distance) {
        *distance = njSqrt(best);
    }
    if (nearest) {
        union line_view line;

        {
            /* @hack spelling: the nearest sample copied into the line through
             * cursors over their floats, for the image's post-increment loads;
             * through the members, 20 of 181 words differ */
            Float *from_point = VECTOR_FLOATS(&points[index]);
            Float *to = line.coordinates;

            *to++ = *from_point++;
            *to++ = *from_point++;
            *to = *from_point;
        }
        {
            /* @hack spelling: the line's direction, between the two samples,
             * through cursors over their floats, the same way; through the
             * members, the function runs 2 bytes long and 71 of 181 words
             * differ */
            Float *to = &line.coordinates[3];
            Float *from_point = VECTOR_FLOATS(&points[second]);
            Float *by = VECTOR_FLOATS(&points[index]);

            *to++ = *from_point++ - *by++;
            *to++ = *from_point++ - *by++;
            *to = *from_point - *by;
        }
        /* @hack type: the union's address converted to NJS_LINE *, which SHC
         * forms from R15 at each use; as &line.line, SHC keeps it in a saved
         * register, and 73 of 181 words differ */
        njDistanceP2L(pos, (NJS_LINE *)&line, nearest);
        if (direction) {
            *direction = line.split.direction;
        }
    }
    return index;
}

/* the task's step. it ends the task once the stage or act changes. otherwise
 * it acts only on a frame's first step */
static void beach_path_camera_step(struct task *task)
{
    struct beach_path_camera_work *work = CAMERA(task);
    struct task_work *player = player_work[0];
    struct task_work *camera = camera_work;

    if (stage_get_stage_act() != work->stage_act) {
        task_kill(task);
        return;
    }
    if (frame_step > 0) {
        return;
    }
    if (!(work->held & HELD)) {
        /* the path samples, frees and takes only past REST, while the camera
         * runs at neither CAMERA_LEVEL_COMP nor CAMERA_LEVEL_EVENT */
        if (!camera_at_comp_level() && !camera_at_event_level() &&
            work->timer > REST) {
            NJS_VECTOR margin;

            margin.x = work->camera->reach;
            margin.y = work->camera->reach;
            margin.z = work->camera->reach;
            /* within reach of the box, the path samples the curve. on a later
             * frame, it takes the camera within reach of the curve. beyond
             * reach of the box, it frees the samples */
            if ((TRACK(work)->samples != NULL ||
                 beach_path_camera_sampled <= SAMPLED_MAX) &&
                beach_path_camera_in_box(&player->pos, &work->low, &work->high,
                                         &margin)) {
                if (TRACK(work)->samples == NULL) {
                    int count = work->path->count - work->camera->first -
                                work->camera->skip;
                    NJS_POINT3 *keys;
                    struct path_point *point;
                    char *to;
                    int i;
                    struct beach_path_camera_curve request;

                    TRACK(work)->samples =
                        heap_alloc((count + SPARE_SPANS) * POINT_SAMPLES_SIZE);
                    keys = heap_alloc(count * sizeof(NJS_POINT3));
                    /* the keys run from point skip and leave first points
                     * off the end. the box reads first and skip the other
                     * way round */
                    point = &work->path->points[work->camera->skip];
                    to = (char *)keys;
                    /* @hack spelling: a byte cursor run on from one key's
                     * floats into the next's, for the image's stores through
                     * R4, each stepped by ADD #4 (0x0c90c656 to 0x0c90c66c).
                     * through an NJS_POINT3 cursor's members, 8 of 289 words
                     * differ */
                    for (i = 0; i < count; i++, point++, to += sizeof(Float)) {
                        *(Float *)to = point->pos.x;
                        to += sizeof(Float);
                        *(Float *)to = point->pos.y;
                        to += sizeof(Float);
                        *(Float *)to = point->pos.z;
                    }
                    request.curve.keys = keys;
                    request.curve.count = count;
                    request.curve.format = beach_path_camera_format;
                    request.curve.step = 1.0F / SAMPLES_PER_POINT;
                    request.curve.out = TRACK(work)->samples;
                    request.curve.kind = PATH_CURVE_BSPLINE;
                    request.unk_18 = 0;
                    /* @hack type: the request's address cast to its curve's,
                     * so SHC stores the request's fields at displacements from
                     * R15 (0x0c90c674 to 0x0c90c68e). as &request.curve, SHC
                     * keeps that address in R12, saves R9, and the function
                     * runs 2 bytes long and 136 of 289 words differ */
                    TRACK(work)->count =
                        (int)(path_curve_sample((struct path_curve *)&request) -
                              (Float)(SAMPLES_SKIPPED + SAMPLES_DROPPED));
                    TRACK(work)->samples += SAMPLES_SKIPPED;
                    heap_free(keys);
                    beach_path_camera_sampled++;
                } else if (beach_path_camera_take(work, player, camera)) {
                    work->timer = 0;
                    work->held |= HELD;
                }
            } else {
                /* @hack spelling: the samples tested bare, which reads
                 * beach_path_camera_sampled into R1 and loads heap_free's
                 * address into R3 before the lowered count is stored
                 * (0x0c90c6e8 to 0x0c90c6ee). against NULL, 3 of 289 words
                 * differ */
                if (TRACK(work)->samples) {
                    beach_path_camera_sampled--;
                    heap_free(TRACK(work)->samples - SAMPLES_SKIPPED);
                    TRACK(work)->samples = NULL;
                    TRACK(work)->count = 0;
                }
            }
        }
    } else if (camera_at_comp_level() || !camera_at_event_level()) {
        /* the camera has left CAMERA_LEVEL_EVENT */
        beach_path_camera_release(work, player, camera);
        work->timer = 0;
        work->held &= ~HELD;
    } else if (!beach_path_camera_follow(work, player, camera)) {
        beach_path_camera_release(work, player, camera);
        work->timer = 0;
        work->held &= ~HELD;
    }
    /* count frames, held at 65535 rather than wrapped to 0 */
    /* @hack spelling: <= 0, which for the unsigned timer is == 0, keeps the
     * image's CMP/PL R0 at 0x0c90c7a6. as == 0, 2 of 289 words differ */
    if (++work->timer <= 0) {
        work->timer--;
    }
}

static void beach_path_camera_destroy(struct task *task)
{
    struct beach_path_camera_work *work = CAMERA(task);

    /* @hack spelling: the samples tested bare, which reaches the track's slot
     * through ADD #44 (0x0c90c7ca). against NULL, SHC reaches it through a
     * displacement, and the function runs 2 bytes short and 26 of 32 words
     * differ */
    if (TRACK(work)->samples) {
        beach_path_camera_sampled--;
        heap_free(TRACK(work)->samples - SAMPLES_SKIPPED);
        TRACK(work)->samples = NULL;
        TRACK(work)->count = 0;
    }
    heap_free(TRACK(work));
}

/* allocate the track, with no samples, and set the box. the task then steps
 * by beach_path_camera_step */
static void beach_path_camera_init(struct task *task)
{
    struct beach_path_camera_work *work = CAMERA(task);
    NJS_VECTOR margin;

    work->timer = 0;
    work->held = 0;
    margin.x = 0.0F;
    margin.y = 0.0F;
    margin.z = 0.0F;
    TRACK(work) = heap_alloc(sizeof(struct beach_path_camera_track));
    TRACK(work)->samples = NULL;
    TRACK(work)->count = 0;
    TRACK(work)->flags = 0;
    beach_path_camera_box(work->path, work->camera->first, work->camera->skip,
                          &margin, &work->low, &work->high);
    task->step = beach_path_camera_step;
    task->destructor = beach_path_camera_destroy;
}

/* whether index is at either end of the curve: one of its last two samples,
 * or its first */
static Bool beach_path_camera_at_ends(int index,
                                      struct beach_path_camera_work *work)
{
    int last = TRACK(work)->count - 2;

    if (index >= last) {
        return TRUE;
    }
    if (index <= 0) {
        return TRUE;
    }
    return FALSE;
}

/* return the index of the sample nearest pos, and set nearest and direction
 * as beach_path_camera_nearest does. distance, unless NULL, gets the distance
 * from pos to nearest. nearest must not be NULL while distance isn't */
static int beach_path_camera_find(NJS_POINT3 *pos, NJS_POINT3 *nearest,
                                  NJS_VECTOR *direction, Float *distance,
                                  struct beach_path_camera_work *work)
{
    Float sample_distance;
    int index;

    index =
        beach_path_camera_nearest(pos, TRACK(work)->samples, TRACK(work)->count,
                                  &sample_distance, nearest, direction);
    if (distance) {
        *distance = njDistanceP2P(nearest, pos);
    }
    return index;
}

/* as beach_path_camera_find, with the search in x and z alone. the distance
 * is still in all three axes */
static int beach_path_camera_find_flat(NJS_POINT3 *pos, NJS_POINT3 *nearest,
                                       NJS_VECTOR *direction, Float *distance,
                                       struct beach_path_camera_work *work)
{
    Float sample_distance;
    int index;

    index = beach_path_camera_nearest_flat(pos, TRACK(work)->samples,
                                           TRACK(work)->count, &sample_distance,
                                           nearest, direction);
    if (distance) {
        *distance = njDistanceP2P(nearest, pos);
    }
    return index;
}

/* out = from + by * scale */
static void beach_path_camera_add(NJS_POINT3 *out, NJS_POINT3 *from,
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
static void beach_path_camera_blend(NJS_VECTOR *out, NJS_VECTOR *to,
                                    NJS_VECTOR *from, Float share)
{
    /* @hack spelling: one cursor over to's floats, then from's and out's, as in
     * beach_path_camera_add; through the members, the function runs 6 bytes
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

/* take the camera if player 0 is within reach of the curve and, with
 * PATH_CAMERA_ENDS, off its ends. it returns TRUE if it takes the camera, and
 * FALSE otherwise. the take hands the camera to camera_step_pose at
 * CAMERA_LEVEL_EVENT, posed at camera_work's place and angles. it sets the
 * track's indexes and direction whether or not it takes the camera. camera is
 * unused */
static Bool beach_path_camera_take(struct beach_path_camera_work *work,
                                   struct task_work *player,
                                   struct task_work *camera)
{
    Float distance;
    NJS_POINT3 nearest;
    NJS_VECTOR direction;
    int index;
    Sint16 turn;

    if (work->camera->flags & PATH_CAMERA_FLAT) {
        index = beach_path_camera_find_flat(&player->pos, &nearest, &direction,
                                            &distance, work);
    } else {
        index = beach_path_camera_find(&player->pos, &nearest, &direction,
                                       &distance, work);
    }
    TRACK(work)->follow_index = TRACK(work)->sit_index =
        TRACK(work)->look_index = index;
    DIRECTION(work)[0] = direction.x;
    DIRECTION(work)[1] = direction.y;
    DIRECTION(work)[2] = direction.z;
    if (!(work->camera->flags & PATH_CAMERA_ENDS) ||
        !beach_path_camera_at_ends(TRACK(work)->follow_index, work)) {
        if (distance < work->camera->reach) {
            camera_override(camera_step_pose, work->camera->first_adjust,
                            CAMERA_BY_STEP);
            camera_pose_taken = FALSE;
            camera_pose_pos = camera_work->pos;
            camera_pose_ang = camera_work->ang;
            if (work->camera->flags & PATH_CAMERA_ALIGN) {
                turn = (Angle)(atan2f(DIRECTION(work)[0], DIRECTION(work)[2]) *
                               ANGLE_PER_RADIAN) -
                       camera_work->ang.y;
                if (turn > NJM_DEG_ANG(90) || turn < -NJM_DEG_ANG(90)) {
                    TRACK(work)->flags |= TRACK_BACKWARD;
                } else {
                    TRACK(work)->flags &= ~TRACK_BACKWARD;
                }
            } else if (work->camera->flags & PATH_CAMERA_BACKWARD) {
                TRACK(work)->flags |= TRACK_BACKWARD;
            }
            if (TRACK(work)->flags & TRACK_BACKWARD) {
                DIRECTION(work)[0] = -DIRECTION(work)[0];
                DIRECTION(work)[1] = -DIRECTION(work)[1];
                DIRECTION(work)[2] = -DIRECTION(work)[2];
            }
            return TRUE;
        }
    }
    return FALSE;
}

/* set this frame's pose, camera_pose_pos and camera_pose_ang, and clear
 * camera_pose_taken. from wait frames after the take on, it also sets the
 * camera's adjust mode to adjust. it returns FALSE once the followed point is
 * farther than leave from the curve. with PATH_CAMERA_ENDS, it also returns
 * FALSE once that point is at either end. it returns TRUE otherwise */
static Bool beach_path_camera_follow(struct beach_path_camera_work *work,
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
    NJS_VECTOR ahead;
    struct angles ang;
    NJS_VECTOR way;
    NJS_VECTOR to;
    Angle rotate;
    Angle turn;
    Float distance;
    Float camera_distance;
    Float place_distance;
    Float dx;
    Float dz;
    Float share;
    int index;
    Bool gone = FALSE;

    way.x = 0.0F;
    way.y = work->camera->height + 0.01F;
    way.z = 0.0F;
    to.x = 0.0F;
    to.z = 1.0F;
    to.y = 0.0F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &player->pos);
    TURN_VIA(njRotateZ, player->ang.z, rotate);
    TURN_VIA(njRotateX, player->ang.x, rotate);
    TURN_VIA(njRotateY, NJM_DEG_ANG(180) - player->ang.y, rotate);
    /* followed is the point height + 0.01 above player 0 along his up axis */
    njCalcPoint(NULL, &way, &followed);
    /* up and ahead, his up and forward vectors, are never read */
    njCalcVector(NULL, &way, &up);
    njCalcVector(NULL, &to, &ahead);
    njPopMatrix(1);
    /* the take and each follow clear camera_pose_taken, and camera_step_pose
     * sets it as it poses the camera. while camera_pose_taken is still FALSE,
     * follow searches the whole curve. that search sets the track's indexes
     * and direction anew, and follow decides again whether to reverse the
     * direction */
    if (!camera_pose_taken) {
        if (work->camera->flags & PATH_CAMERA_FLAT) {
            index = beach_path_camera_find_flat(
                &followed, &way, &TRACK(work)->direction, &distance, work);
        } else {
            index = beach_path_camera_find(
                &followed, &way, &TRACK(work)->direction, &distance, work);
        }
        TRACK(work)->follow_index = TRACK(work)->sit_index =
            TRACK(work)->look_index = index;
        /* @hack redundant: a test of index that never runs. it keeps the angle
         * difference in R5 and off, its low half, in R4 (0x0c90cd54 to
         * 0x0c90cd62); without it, 5 of 847 words differ */
        if (0) {
            if (index) {
            }
        }
        if (work->camera->flags & PATH_CAMERA_ALIGN) {
            Sint16 off =
                (Angle)(atan2f(DIRECTION(work)[0], DIRECTION(work)[2]) *
                        ANGLE_PER_RADIAN) -
                camera_work->ang.y;

            if (off > NJM_DEG_ANG(90) || off < -NJM_DEG_ANG(90)) {
                TRACK(work)->flags |= TRACK_BACKWARD;
            } else {
                TRACK(work)->flags &= ~TRACK_BACKWARD;
            }
        } else if (work->camera->flags & PATH_CAMERA_BACKWARD) {
            TRACK(work)->flags |= TRACK_BACKWARD;
        }
        if (TRACK(work)->flags & TRACK_BACKWARD) {
            DIRECTION(work)[0] = -DIRECTION(work)[0];
            DIRECTION(work)[1] = -DIRECTION(work)[1];
            DIRECTION(work)[2] = -DIRECTION(work)[2];
        }
    }
    if (work->camera->flags & PATH_CAMERA_FLAT) {
        TRACK(work)->follow_index = beach_path_camera_nearest_flat_near(
            TRACK(work)->follow_index, SEARCH, &followed, TRACK(work)->samples,
            TRACK(work)->count, NULL, &nearest, &way);
    } else {
        TRACK(work)->follow_index = beach_path_camera_nearest_near(
            TRACK(work)->follow_index, SEARCH, &followed, TRACK(work)->samples,
            TRACK(work)->count, NULL, &nearest, &way);
    }
    /* turn direction toward way, the direction at the nearest sample, and
     * make it a unit vector. a reversed direction is negated for the blend,
     * and negated again after it */
    if (TRACK(work)->flags & TRACK_BACKWARD) {
        njUnitVector(&way);
        DIRECTION(work)[0] = -DIRECTION(work)[0];
        DIRECTION(work)[1] = -DIRECTION(work)[1];
        DIRECTION(work)[2] = -DIRECTION(work)[2];
        beach_path_camera_blend(&TRACK(work)->direction,
                                &TRACK(work)->direction, &way, DIRECTION_KEPT);
        njUnitVector(&TRACK(work)->direction);
        DIRECTION(work)[0] = -DIRECTION(work)[0];
        DIRECTION(work)[1] = -DIRECTION(work)[1];
        DIRECTION(work)[2] = -DIRECTION(work)[2];
    } else {
        njUnitVector(&way);
        beach_path_camera_blend(&TRACK(work)->direction,
                                &TRACK(work)->direction, &way, DIRECTION_KEPT);
        njUnitVector(&TRACK(work)->direction);
    }
    if ((work->camera->flags & PATH_CAMERA_ENDS) &&
        beach_path_camera_at_ends(TRACK(work)->follow_index, work)) {
        gone = TRUE;
    }
    if (work->camera->flags & PATH_CAMERA_FLAT) {
        if (work->camera->leave <
            njSqrt(beach_path_camera_distance_squared(&followed, &nearest))) {
            gone = TRUE;
        }
    } else {
        if (work->camera->leave < njDistanceP2P(&followed, &nearest)) {
            gone = TRUE;
        }
    }
    beach_path_camera_add(&sit_from, &followed, &TRACK(work)->direction,
                          work->camera->sit_back);
    if (work->camera->flags & PATH_CAMERA_FLAT) {
        TRACK(work)->sit_index = beach_path_camera_nearest_flat_near(
            TRACK(work)->sit_index, SEARCH, &sit_from, TRACK(work)->samples,
            TRACK(work)->count, NULL, &sit, NULL);
    } else {
        TRACK(work)->sit_index = beach_path_camera_nearest_near(
            TRACK(work)->sit_index, SEARCH, &sit_from, TRACK(work)->samples,
            TRACK(work)->count, NULL, &sit, NULL);
    }
    beach_path_camera_add(&look_from, &followed, &TRACK(work)->direction,
                          -work->camera->look_ahead);
    if (work->camera->flags & PATH_CAMERA_FLAT) {
        TRACK(work)->look_index = beach_path_camera_nearest_flat_near(
            TRACK(work)->look_index, SEARCH, &look_from, TRACK(work)->samples,
            TRACK(work)->count, NULL, &look, NULL);
    } else {
        TRACK(work)->look_index = beach_path_camera_nearest_near(
            TRACK(work)->look_index, SEARCH, &look_from, TRACK(work)->samples,
            TRACK(work)->count, NULL, &look, NULL);
    }
    /* offset = followed - nearest, the followed point's offset from the
     * curve */
    {
        /* @hack spelling: cursors over the three points' floats, for the
         * image's post-increment loads and its stores through offset's address
         * stepped by 4; through the members, the function runs 2 bytes long and
         * 272 of 847 words differ */
        Float *diff = VECTOR_FLOATS(&offset);
        Float *from = VECTOR_FLOATS(&followed);
        Float *by = VECTOR_FLOATS(&nearest);

        *diff++ = *from++ - *by++;
        *diff++ = *from++ - *by++;
        *diff = *from - *by;
    }
    sit_offset = offset;
    look_offset = offset;
    beach_path_camera_add(&place, &sit, &sit_offset, work->camera->sit_share);
    beach_path_camera_add(&aim, &look, &look_offset, work->camera->look_share);
    /* with PATH_CAMERA_WALLS, a ball of radius height swept from the camera
     * toward place stops place at a wall. the sweep runs only while the
     * camera is no farther from the sit point than place is */
    if (work->camera->flags & PATH_CAMERA_WALLS) {
        to = camera->pos;
        camera_distance = njDistanceP2P(&sit, &to);
        place_distance = njDistanceP2P(&sit, &place);
        if (camera_distance <= place_distance) {
            /* way = place - to, and after a hit place = camera->pos + way */
            /* @hack spelling: the subtraction's cursors step from each x into y
             * and z. through VECTOR_FLOATS, its first difference takes FR2, not
             * the image's FR1 at 0x0c90d236, and 10 of 847 words differ */
            {
                Float *diff = &way.x;
                Float *from = &place.x;
                Float *by = &to.x;

                *diff++ = *from++ - *by++;
                *diff++ = *from++ - *by++;
                *diff = *from - *by;
            }
            if (world_sweep_all_hit(&to, &way, NULL, work->camera->height)) {
                /* @hack spelling: the sum walked through VECTOR_FLOATS; through
                 * the members, the function runs 4 bytes long and 187 of 847
                 * words differ */
                Float *by = VECTOR_FLOATS(&way);
                Float *sum = VECTOR_FLOATS(&place);
                Float *from = VECTOR_FLOATS(&camera->pos);

                *sum++ = *from++ + *by++;
                *sum++ = *from++ + *by++;
                *sum = *from + *by;
            }
        }
    }
    beach_path_camera_blend(&place, &place, &camera->pos, work->camera->ease);
    share = work->camera->turn;
    dx = place.x - aim.x;
    dz = place.z - aim.z;
    turn = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN) - camera->ang.y;
    /* read as a Sint16, turn's low half is the same turn the shorter way round.
     * it lies within half a turn either way. the camera turns by share of it.
     * the high half stays, since a rotation reads only the low half */
    {
        /* @hack spelling: share copied to scale, which keeps the cursors of the
         * walks for offset, way and the sum, and dx and dz in FR13 and FR12.
         * through share, 51 of 847 words differ */
        Float scale = share;
        /* @hack type: turn's low half read and written through a Sint16
         * pointer to its slot. half holds the slot's address in R4 for the
         * MOV.W at 0x0c90d308 and 0x0c90d31e. as union angle_halves, the
         * function grows by 12 bytes and 180 of 847 words differ */
        Angle *half = &turn;
        Float scaled = *(Sint16 *)half;

        scaled *= scale;
        *(Sint16 *)half = (Sint16)scaled;
        /* @hack redundant: tests of dx and dz that never run. they keep
         * offset's cursors from, by and diff in R4, R5 and R6; without them, 17
         * of 847 words differ */
        if (0) {
            if (dx) {
            }
            if (dz) {
            }
        }
    }
    /* @hack redundant: four tests that never run, each steering the pool or
     * the registers as the comment above it says; without them, 50 of 847
     * words differ */
    if (0) {
        /* this test puts the factor 0x4622f986 ahead of atan2f's address in
         * the pool at 0x0c90ccf4. it also keeps the angle difference in R5
         * and off in R4. it keeps way = place - to's cursors from and by in
         * R6 and R5 */
        if (&atan2f) {
        }
        /* these two tests keep share in FR14 */
        if (share) {
        }
        if (share) {
        }
        /* this test and the two above keep three walks' cursors: offset's,
         * way's and the sum's after a hit. they also keep dx and dz in FR13
         * and FR12 */
        if (ang.z) {
        }
    }
    ang.y = turn + camera->ang.y;
    turn = (Angle)(atan2f(aim.y - place.y, njSqrt((dx * dx) + (dz * dz))) *
                   ANGLE_PER_RADIAN) -
           camera->ang.x;
    {
        /* @hack spelling: share copied to scale, as for the turn about y.
         * through share, 51 of 847 words differ */
        Float scale = share;
        /* @hack type: the pitch's low half the same way, at 0x0c90d34c and
         * 0x0c90d358. as union angle_halves, the function runs 12 bytes long
         * and 219 of 847 words differ */
        Angle *half = &turn;
        Float scaled = *(Sint16 *)half;

        scaled *= scale;
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

/* give the camera back with camera_restore while it runs at exactly
 * CAMERA_LEVEL_EVENT, and clear camera_pose_taken. it returns TRUE. player
 * and camera are unused */
static Bool beach_path_camera_release(struct beach_path_camera_work *work,
                                      struct task_work *player,
                                      struct task_work *camera)
{
    if (!camera_at_comp_level() && camera_at_event_level()) {
        camera_restore();
    }
    camera_pose_taken = FALSE;
    return TRUE;
}
