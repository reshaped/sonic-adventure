/* O PO SUMMON's path orca, which leaps in and out of the sea along
 * path_orca_path */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "path/path.h"
#include "sound/sound.h"
#include "stg01/orca_control.h"
#include "stg01/path_orca.h"
#include "stg01/sea.h"
#include "stg01/water_column.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT orca_object;
extern NJS_MOTION orca_swim_motion;
extern NJS_MOTION orca_swim_shape;
extern NJS_MOTION path_orca_leap_motion;
extern NJS_MOTION path_orca_leap_shape;
extern struct path path_orca_path;

struct path_orca_vars {
    Float frame; /* both motions' frame, from 0 up to FRAMES */
    Float distance; /* the orca's distance along the path */
    Float speed; /* how far the orca moves along the path each step */
};

#define VARS(work) ((struct path_orca_vars *)&(work)->vars)

enum path_orca_mode {
    PATH_ORCA_UNDER = 0,
    PATH_ORCA_RISE = 1,
    PATH_ORCA_FALL = 2,
    PATH_ORCA_END = 6 /* past the path's end, where the step kills the task */
};

/* the orca always moves SPEED_MAX along the path: it starts there, and
 * nothing lowers its speed. the step's regain of SPEED_GAIN a step under the
 * sea, for a speed below SPEED_MAX, never applies */
#define SPEED_MAX 4.0F
#define SPEED_GAIN 0.1F
/* both motions advance FRAME_SPEED frames a step, and wrap at FRAMES */
#define FRAME_SPEED 0.1F
#define FRAMES 30.0F

/* the orca starts START_DISTANCE along the path */
#define START_DISTANCE 330.0F
/* the orca heads away from the path's point HEADING_BACK behind it. it tilts
 * as the path does TILT_BACK behind it */
#define HEADING_BACK 200.0F
#define TILT_BACK 100.0F
/* the orca's model is drawn MODEL_BACK behind its position, along its
 * heading */
#define MODEL_BACK 160.0F

/* sound of the orca leaving and entering the sea */
#define SE_ORCA_SPLASH 47

/* speeds for the water columns the orca throws up leaving the sea and falling
 * back in. no column moves by them */
static const NJS_VECTOR path_orca_rise_splash_speed = {1.0F, 1.0F, 1.0F};
static const NJS_VECTOR path_orca_fall_splash_speed = {1.0F, 1.0F, 1.0F};

static void path_orca_step(struct task *task);
static void path_orca_draw(struct task *task);
static void path_orca_destroy(struct task *task);

/* start the orca rising, at full speed. it sets submode 4, which nothing
 * reads */
void path_orca_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.y = 0;
    work->ang.x = 0;
    work->ang.z = 0;
    VARS(work)->frame = 0.0F;
    VARS(work)->distance = START_DISTANCE;
    VARS(work)->speed = SPEED_MAX;
    work->mode = PATH_ORCA_RISE;
    work->submode = 4;
    /* @hack redundant: a test of placement_editing's result that does nothing,
     * which keeps the image's TST R0,R0 at 0x0c90a076. as a bare call, the
     * function runs 2 bytes short and 13 of 36 words differ */
    if (!placement_editing(task)) {
    }
    task->step = path_orca_step;
    task->draw = path_orca_draw;
    task->destructor = path_orca_destroy;
}

/* move the orca along the path by its speed. kill the task once the orca
 * passes the path's end. a rising orca falls once it is no higher than the
 * point HEADING_BACK behind it. crossing the sea's surface either way throws up
 * a water column, ripples the sea and plays SE_ORCA_SPLASH. leaving the sea
 * also shakes pad 0 */
static void path_orca_step(struct task *task)
{
    struct task_work *work = task->work;
    struct path *path = &path_orca_path;
    Float distance;
    Float length;
    Float y;
    Float back_y;

    distance = VARS(work)->distance;
    distance += VARS(work)->speed;
    length = path->length;
    if (distance >= length) {
        work->mode = PATH_ORCA_END;
    } else {
        struct path_sample samples[3];
        Float *at;
        Float x;
        Float z;
        Float back_x;
        Float back_z;

        samples[0].distance = distance;
        samples[1].distance = distance - HEADING_BACK;
        samples[2].distance = distance - TILT_BACK;
        path_sample(path, &samples[0]);
        path_sample(path, &samples[1]);
        path_sample(path, &samples[2]);
        /* @hack spelling: the positions read through a Float cursor, for the
         * image's post-increment loads (0x0c90a142 to 0x0c90a156). through
         * the members, 73 of 262 words differ */
        at = &samples[0].pos_copy.x;
        x = *at++;
        y = *at++;
        z = *at++;
        at = &samples[1].pos_copy.x;
        back_x = *at++;
        back_y = *at++;
        back_z = *at++;
        work->pos.x = x;
        work->pos.y = y;
        work->pos.z = z;
        work->ang.x = samples[2].smooth_x;
        work->ang.z = samples[2].smooth_z;
        work->ang.y =
            (Angle)(atan2f(x - back_x, z - back_z) * ANGLE_PER_RADIAN);
        VARS(work)->distance = distance;
    }
    switch (work->mode) {
    case PATH_ORCA_UNDER:
        if (work->pos.y > SEA_LEVEL) {
            NJS_VECTOR speed;

            work->mode = PATH_ORCA_RISE;
            speed = path_orca_rise_splash_speed;
            beach_water_column_create(&work->pos, &speed, 1.0F);
            vibration_start_fading(0, 7, 59, 6);
            beach_sea_ripple(work->pos.x, work->pos.z, 0.0F, 8.0F, 20.0F);
            sound_play(SE_ORCA_SPLASH, 0, 0, 0);
        } else if (VARS(work)->speed < SPEED_MAX) {
            VARS(work)->speed += SPEED_GAIN;
        }
        break;
    case PATH_ORCA_RISE:
        if (y > back_y) {
            break;
        }
        work->mode = PATH_ORCA_FALL;
        break;
    case PATH_ORCA_FALL:
        if (work->pos.y < SEA_LEVEL) {
            NJS_VECTOR speed;

            work->mode = PATH_ORCA_UNDER;
            speed = path_orca_fall_splash_speed;
            beach_water_column_create(&work->pos, &speed, 1.0F);
            beach_sea_ripple(work->pos.x, work->pos.z, 0.0F, 8.0F, 20.0F);
            sound_play(SE_ORCA_SPLASH, 0, 0, 0);
        } else if (VARS(work)->speed < SPEED_MAX) {
            /* @hack redundant: a test of the speed that does nothing, which
             * keeps the image's FCMP/GT at 0x0c90a2d8. without it, the
             * function runs 32 bytes long and 139 of 262 words differ */
        }
        break;
    case PATH_ORCA_END:
        task_kill(task);
        break;
    }
    VARS(work)->frame += FRAME_SPEED;
    if (VARS(work)->frame >= FRAMES) {
        VARS(work)->frame -= FRAMES;
    }
    path_orca_draw(task);
}

/* draw the orca swimming under the sea, and leaping above it */
static void path_orca_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, 0.0F, -MODEL_BACK);
    if (work->mode == PATH_ORCA_UNDER) {
        draw_shape_motion(&orca_object, &orca_swim_motion, &orca_swim_shape,
                          VARS(work)->frame);
    } else {
        draw_shape_motion(&orca_object, &path_orca_leap_motion,
                          &path_orca_leap_shape, VARS(work)->frame);
    }
    njPopMatrix(1);
}

/* make the controller forget the orca, and stop pad 0's vibration */
static void path_orca_destroy(struct task *task)
{
    path_orca_clear_task(task);
    vibration_start(0, 0, 0, 0);
}
