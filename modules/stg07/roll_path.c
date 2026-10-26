/* the roll: the step of one of act 1's paths. a touch of the sphere at the
 * path's start curls player 0 up. the task then carries player 0 along the
 * path. near its end, the task sends player 0 off, standing again, and
 * ends */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg07/roll_path.h"
#include "system/task.h"
#include "system/util.h"

/* the carry's distance along the path, and the distance at which it ends,
 * in the work's scale */
#define ROLL_ALONG(work) ((work)->scl.x)
#define ROLL_END(work) ((work)->scl.y)

enum roll_mode { ROLL_START, ROLL_WAIT, ROLL_CARRY };

/* the carry moves ROLL_SPEED a step, and ends ROLL_SHORT short of the
 * path's end */
#define ROLL_SPEED 15.0F
#define ROLL_SHORT 10.0F
/* player 0 leaves at LEAVE_SPEED in x and z, and LEAVE_RISE in y */
#define LEAVE_SPEED 3.0F
#define LEAVE_RISE 1.0F
/* the path's first point. the work starts there, and roll_path_sample puts
 * it there again */
#define ROLL_START_X 6950.0F
#define ROLL_START_Y (-2440.0F)
#define ROLL_START_Z 993.0F

/* a sphere at the path's start, whose touch starts the carry */
/* clang-format off */
static struct collision_shape roll_path_shape = {
    SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_PLAYERS), {0.0F, 0.0F, 0.0F}, 20.0F, 5.0F,
    10.0F, {0}, 0, NULL,
};
/* clang-format on */

static void roll_path_sample(struct path *path, struct task_work *work,
                             Float along);
static void roll_path_carry(struct task_work *work);

/* the task ends once the stage or act changes, and once the carry ends */
void ruin_path_roll_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (stage_get_stage_act() != PATH_VARS(work)->stage_act) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case ROLL_START:
        work->mode = ROLL_WAIT;
        work->timer = 0;
        ROLL_ALONG(work) = 0.0F;
        work->pos.x = ROLL_START_X;
        work->pos.y = ROLL_START_Y;
        work->pos.z = ROLL_START_Z;
        collision_init(task, &roll_path_shape, COLLISION_OBJECT, 1, 1);
        roll_path_sample(PATH_VARS(work)->path, work, ROLL_ALONG(work));
        ROLL_END(work) = PATH_VARS(work)->path->length - ROLL_SHORT;
        break;
    case ROLL_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            player_set_mode(0, PLAYER_MODE_BALL);
            work->mode = ROLL_CARRY;
        }
        collision_register(work);
        break;
    case ROLL_CARRY:
        ROLL_ALONG(work) += ROLL_SPEED;
        roll_path_sample(PATH_VARS(work)->path, work, ROLL_ALONG(work));
        roll_path_carry(work);
        if (ROLL_ALONG(work) > ROLL_END(work)) {
            ang = -work->ang.y + NJM_DEG_ANG(90);
            player_set_speed(0, njCos(ang) * LEAVE_SPEED, LEAVE_RISE,
                             -(njSin(ang) * LEAVE_SPEED));
            player_set_mode(0, PLAYER_MODE_RESET);
            task_kill(task);
            return;
        }
        break;
    }
}

/* put the work along that far on the path, tilted as the path is there.
 * its y angle comes from the way the path runs there */
static void roll_path_sample(struct path *path, struct task_work *work,
                             Float along)
{
    struct path_sample sample;
    NJS_VECTOR ahead;
    Angle ang;

    sample.distance = along;
    if (path_sample(path, &sample)) {
        work->pos = sample.pos_copy;
        work->ang.x = sample.smooth_x;
        work->ang.z = sample.smooth_z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateX, -work->ang.x, ang);
        TURN_VIA(njRotateZ, -work->ang.z, ang);
        njCalcVector(NULL, &sample.ahead, &ahead);
        njPopMatrix(1);
        work->ang.y = (Angle)(atan2f(ahead.x, ahead.z) * ANGLE_PER_RADIAN);
    }
}

/* put player 0 where the work is, turned as the work is */
static void roll_path_carry(struct task_work *work)
{
    struct task_work *player = player_work[0];

    player->pos = work->pos;
    player->ang.x = work->ang.x;
    player->ang.y = -work->ang.y + NJM_DEG_ANG(90);
    player->ang.z = work->ang.z;
}
