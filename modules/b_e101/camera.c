/* the fight's camera: held at a point watching player 0 and the boss, or
 * sweeping round a point */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101/camera.h"
#include "camera/camera.h"
#include "player/player.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* the farthest the holding camera's target lies from player 0 */
#define HOLD_REACH 40.0F

#define SWEEP_DISTANCE 50.0F /* the camera's distance, less the reach */
#define SWEEP_ELEVATION NJM_DEG_ANG(30) /* the angle it looks down from */
#define SWEEP_ARC NJM_DEG_ANG(100) /* the turn of its whole sweep */

/* the camera's offset from its target before the sweep turns and scales
 * it */
static NJS_POINT3 sweep_unit = {1.0F, 0.0F, 0.0F};

static NJS_POINT3 hold_pos;
static NJS_POINT3 *hold_target; /* the caller's point, not a copy */
static NJS_POINT3 sweep_target; /* the point the camera faces and circles */
static Angle sweep_ang; /* the camera's turn about y round its target */
static Angle sweep_turn; /* the turn each step */
static Float sweep_reach; /* the camera's distance beyond SWEEP_DISTANCE */
static int sweep_elapsed; /* the steps run so far */
static int sweep_steps; /* the steps the camera turns for */

static void hold_step(struct camera_view *view);
static void sweep_step(struct camera_view *view);

void e101_camera_hold_start(NJS_POINT3 *target, NJS_POINT3 *pos)
{
    hold_target = target;
    VECTOR_COPY(&hold_pos, pos);
    camera_override(hold_step, 0, CAMERA_LOOK_AT);
}

void e101_camera_sweep_start(NJS_POINT3 *target, Angle ang, int steps)
{
    VECTOR_COPY(&sweep_target, target);
    sweep_ang = ang - (SWEEP_ARC / 2);
    sweep_turn = SWEEP_ARC / steps;
    sweep_elapsed = 0;
    sweep_steps = steps;
    sweep_reach = 0.0F;
    camera_override(sweep_step, 0, CAMERA_LOOK_AT);
}

void e101_camera_sweep_set_reach(Float reach)
{
    sweep_reach = reach;
}

static void hold_step(struct camera_view *view)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR half;
    Float reach;

    camera_state->pos.x = hold_pos.x;
    camera_state->pos.y = hold_pos.y;
    camera_state->pos.z = hold_pos.z;
    VECTOR_DIFF(&half, hold_target, &player->pos);
    VECTOR_SCALE(&half, &half, 0.5F);
    reach = vector_magnitude(&half);
    if (reach >= HOLD_REACH) {
        vector_unit(&half, &half);
        VECTOR_SCALE(&half, &half, HOLD_REACH);
    }
    camera_state->target.x = player->pos.x + half.x;
    camera_state->target.y = player->pos.y + half.y;
    camera_state->target.z = player->pos.z + half.z;
}

static void sweep_step(struct camera_view *view)
{
    NJS_POINT3 pos;
    Float distance;

    if (++sweep_elapsed > sweep_steps) {
        return;
    }
    sweep_ang += sweep_turn;
    camera_state->target.x = sweep_target.x;
    camera_state->target.y = sweep_target.y;
    camera_state->target.z = sweep_target.z;
    distance = sweep_reach + SWEEP_DISTANCE;
    njPushMatrix(&_unit_matrix_);
    njScale(NULL, distance, distance, distance);
    TURN(njRotateY, sweep_ang);
    njRotateZ(NULL, SWEEP_ELEVATION);
    njCalcPoint(NULL, &sweep_unit, &pos);
    njPopMatrix(1);
    VECTOR_SUM(&pos, &pos, &sweep_target);
    camera_state->pos.x = pos.x;
    camera_state->pos.y = pos.y;
    camera_state->pos.z = pos.z;
}
