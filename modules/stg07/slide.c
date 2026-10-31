/* the slide: the steps of one of act 1's paths. once player 0 enters the
 * slide's mouth, the task slides the player down the path. the stick steers
 * the player across it, and the camera rides behind. at the end, the player
 * drops round an arc and is sent off. the task then waits for player 0
 * again */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/ground.h"
#include "effect/splash.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/paths.h"
#include "stg07/slide.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: int, not stage.h's Uint16, so SHC compares the result without
 * extending it (MOV R0,R3 at 0x0c90aaec). as stage.h declares it, an EXTU.W
 * stands there, and 1 of slide_run's 303 words differs */
#ifdef GCC
#include "stage/stage.h"
#else
extern int stage_get_stage_act(void);
#endif

/* a path task's vars, laid out as path_vars. player_x and player_z lie
 * where path_vars has wait and path. they hold where the slide puts player 0
 * on its next step */
struct slide_vars {
    Float player_x;
    Sint16 stage_act;
    Float player_z;
};

#define VARS(work) ((struct slide_vars *)&(work)->vars)

/* in the work's scale. SLIDE_ALONG is how far down the path the slide is,
 * and SLIDE_SPEED how far it moves a step. SLIDE_ACROSS is how far the
 * stick has steered the player off the path's line */
#define SLIDE_ALONG(work) ((work)->scl.x)
#define SLIDE_SPEED(work) ((work)->scl.y)
#define SLIDE_ACROSS(work) ((work)->scl.z)
/* the drop's angle, in the work's timer, read as a Uint16 */
#define DROP_ANG(work) ((Uint16)(work)->timer)

#define SE_SLIDE 198

/* past SLIDE_FAST_AFTER along the path, the speed gains SLIDE_SPEED_UP a
 * step until it reaches SLIDE_FASTEST */
#define SLIDE_FAST_AFTER 100.0F
#define SLIDE_FASTEST 6.0F
#define SLIDE_SPEED_UP 0.1F
/* the slide ends at SLIDE_END along the path, short of its length of about
 * 2568 */
#define SLIDE_END 2521.0F

/* the stick steers once it leans past its dead zone. it moves the player
 * STEER_STEP a step across the path, up to STEER_MOST either side */
#define STEER_LEAN ((Float)PAD_DEAD_ZONE)
#define STEER_STEP 0.5F
#define STEER_MOST 15.0F

/* the camera sits CAMERA_BEHIND back along the path, and its target at the
 * slide. both sit CAMERA_HEIGHT above the path, half as far across as the
 * player */
#define CAMERA_BEHIND 40.0F
#define CAMERA_HEIGHT 6.0F

/* the drop starts at DROP_START and turns DROP_STEP a step. the player
 * moves DROP_SPEED a step round its arc */
#define DROP_START NJM_DEG_ANG(100)
#define DROP_STEP 0x100
#define DROP_SPEED 5.0F

/* the slide's mouth: a box from MOUTH_LOW to MOUTH_HIGH in x and z, below
 * MOUTH_TOP */
#define MOUTH_LOW_X 270.0F
#define MOUTH_HIGH_X 400.0F
#define MOUTH_LOW_Z 1660.0F
#define MOUTH_HIGH_Z 1725.0F
#define MOUTH_TOP 140.0F

static void slide_wait(struct task *task);

static void slide_destroy(struct task *task)
{
}

/* drop the player round an arc. once one more step would take the angle to
 * 0, send the player off. then wait for player 0 again */
static void slide_drop(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR speed;
    Angle ang[3];

    work->timer -= DROP_STEP;
    player_work[0]->pos.y -= njSin(DROP_ANG(work)) * DROP_SPEED;
    player_work[0]->pos.z -= njCos(DROP_ANG(work)) * DROP_SPEED;
    /* @hack spelling: < DROP_STEP + 1, not <= DROP_STEP, which loads 0x101
     * and tests with CMP/GE (0x0c90aa1a). as <= DROP_STEP, 12 of slide_drop's
     * 79 words differ */
    if (DROP_ANG(work) < DROP_STEP + 1) {
        player_set_mode(0, PLAYER_MODE_RESET);
        ang[0] = 0;
        ang[1] = NJM_DEG_ANG(90);
        ang[2] = 0;
        speed.x = 0.0F;
        speed.y = -1.0F;
        speed.z = 0.0F;
        player_send_flying(0, &speed, ang, 60);
        task->step = slide_wait;
    }
}

/* one step of the slide. it does nothing in another stage or act. the
 * player takes the x and z worked out on the step before. a splash marks its
 * next place. past SLIDE_END, the drop starts */
static void slide_run(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the position declared first, which SHC keeps in R13 (ADD
     * #32,R13 at 0x0c90aae6) while the angles' address goes to the stack.
     * declared after the angles, 8 of slide_run's 303 words differ */
    NJS_POINT3 *player_pos = &player_work[0]->pos;
    struct angles *player_ang = &player_work[0]->ang;
    struct path_sample ahead;
    struct path_sample behind;
    NJS_VECTOR splash_speed;
    NJS_POINT3 splash_pos;
    Angle ang;
    Float steer;
    Float floor;
    Float dx;
    Float dz;

    if (stage_get_stage_act() != VARS(work)->stage_act) {
        return;
    }
    if (SLIDE_SPEED(work) < SLIDE_FASTEST &&
        SLIDE_ALONG(work) > SLIDE_FAST_AFTER) {
        SLIDE_SPEED(work) += SLIDE_SPEED_UP;
    }
    if (SLIDE_ALONG(work) < SLIDE_END) {
        steer = per_info[0]->x1 << 8;
        if (steer < -STEER_LEAN) {
            SLIDE_ACROSS(work) += STEER_STEP;
            if (SLIDE_ACROSS(work) > STEER_MOST) {
                SLIDE_ACROSS(work) = STEER_MOST;
            }
        } else if (steer > STEER_LEAN) {
            SLIDE_ACROSS(work) -= STEER_STEP;
            if (SLIDE_ACROSS(work) < -STEER_MOST) {
                SLIDE_ACROSS(work) = -STEER_MOST;
            }
        }
        SLIDE_ALONG(work) += SLIDE_SPEED(work);
        ahead.distance = SLIDE_ALONG(work);
        path_sample(&ruin_slide_path, &ahead);
        behind.distance = SLIDE_ALONG(work) - CAMERA_BEHIND;
        path_sample(&ruin_slide_path, &behind);
        work->pos.x = ahead.pos.x;
        work->pos.y = ahead.pos.y;
        work->pos.z = ahead.pos.z;
        ang = (Angle)(atan2f(ahead.ahead.x, ahead.ahead.z) * ANGLE_PER_RADIAN);
        work->ang.y = ang;
        dx = njSin(ang + NJM_DEG_ANG(90)) * SLIDE_ACROSS(work);
        dz = njCos(ang + NJM_DEG_ANG(90)) * SLIDE_ACROSS(work);
        player_ang->y = NJM_DEG_ANG(90) - ang;
        player_pos->x = VARS(work)->player_x;
        floor = player_get(0)->shadow.below;
        if (floor != -GROUND_FAR) {
            player_pos->y = floor - 2.0F;
        } else {
            player_pos->y = player_get(0)->shadow.above - 2.0F;
        }
        player_pos->z = VARS(work)->player_z;
        VARS(work)->player_x = work->pos.x + dx;
        VARS(work)->player_z = work->pos.z + dz;
        splash_speed.x = 0.0F;
        splash_speed.y = 0.2F;
        splash_speed.z = 0.0F;
        splash_pos.x = VARS(work)->player_x;
        splash_pos.y = player_pos->y + 2.0F;
        splash_pos.z = VARS(work)->player_z;
        splash_create(&splash_pos, &splash_speed, 1.0F);
        dx *= 0.5F;
        dz *= 0.5F;
        camera_work->scl.x = behind.pos.x + dx;
        camera_work->scl.y = behind.pos.y + CAMERA_HEIGHT;
        camera_work->scl.z = behind.pos.z + dz;
        CAMERA_VARS(camera_work)->target.x = ahead.pos.x + dx;
        CAMERA_VARS(camera_work)->target.y = ahead.pos.y + CAMERA_HEIGHT;
        CAMERA_VARS(camera_work)->target.z = ahead.pos.z + dz;
        /* @hack redundant: a return where the function returns anyway, which
         * ends the arm with a BRA to the return and its last store in the delay
         * slot (0x0c90ad7c). without it, 26 of slide_run's 303 words differ */
        return; /* NOLINT(readability-redundant-control-flow) */
    } else {
        camera_restore();
        SLIDE_SPEED(work) = 1.0F;
        work->timer = DROP_START;
        task->step = slide_drop;
    }
}

/* wait for player 0 to enter the slide's mouth. then start the slide at the
 * path's point nearest the player */
static void slide_wait(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &player_work[0]->pos;
    NJS_POINT3 nearest;
    Float along;

    if (pos->x > MOUTH_LOW_X && pos->x < MOUTH_HIGH_X && pos->z > MOUTH_LOW_Z &&
        pos->z < MOUTH_HIGH_Z && pos->y < MOUTH_TOP) {
        path_find_nearest(&ruin_slide_path, pos, &nearest, &along);
        VARS(work)->player_x = pos->x;
        VARS(work)->player_z = pos->z;
        SLIDE_ALONG(work) = along;
        player_set_mode(0, PLAYER_MODE_PATH_SLIDE);
        camera_override_mode(CAMERA_MODE_RUIN_WAKA1, CAMERA_ADJUST_RELATIVE1);
        task->step = slide_run;
        sound_play(SE_SLIDE, 0, 0, 0);
    }
}

void ruin_path_slide_step(struct task *task)
{
    struct task_work *work = task->work;

    SLIDE_SPEED(work) = 1.0F;
    SLIDE_ACROSS(work) = 0.0F;
    task->step = slide_wait;
    task->destructor = slide_destroy;
}
