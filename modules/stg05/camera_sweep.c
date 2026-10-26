/* the camera's sweep around an E-100 series robot as it goes down */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "stg05/camera_sweep.h"
#include "system/util.h"

#define SWEEP_DISTANCE 50.0F /* the camera's distance, less the reach */
#define SWEEP_ELEVATION NJM_DEG_ANG(30) /* the angle it looks down from */
#define SWEEP_ARC NJM_DEG_ANG(100) /* the turn of its whole sweep */

static NJS_POINT3 sweep_unit = {1.0F, 0.0F, 0.0F};

static NJS_POINT3 sweep_target; /* the point the camera faces and circles */
static Angle sweep_ang; /* the camera's turn about y round its target */
static Angle sweep_turn; /* the turn each step */
static Float sweep_reach; /* the camera's distance beyond SWEEP_DISTANCE */
static int sweep_step; /* the steps run so far */
static int sweep_steps; /* the steps the camera turns for */

static void camera_sweep_step(struct camera_view *view);

void camera_sweep_start(NJS_POINT3 *target, Angle ang, int steps)
{
    VECTOR_COPY(&sweep_target, target);
    sweep_ang = ang - (SWEEP_ARC / 2);
    sweep_turn = SWEEP_ARC / steps;
    sweep_step = 0;
    sweep_steps = steps;
    sweep_reach = 0.0F;
    camera_override(camera_sweep_step, 0, CAMERA_LOOK_AT);
}

void camera_sweep_set_reach(Float reach)
{
    sweep_reach = reach;
}

static void camera_sweep_step(struct camera_view *view)
{
    NJS_POINT3 pos;
    Float distance;

    if (++sweep_step > sweep_steps) {
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
