/* the camera's sweep around E-105 once it is beaten, and the rays of light
 * it gives off */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "effect/beam.h"
#include "stg12/camera_sweep.h"
#include "system/util.h"

/* the camera circles its target SWEEP_DISTANCE away, at an elevation of
 * SWEEP_ELEVATION. it turns through about SWEEP_ARC, centered on the sweep's
 * angle, before its turn slows */
#define SWEEP_DISTANCE 100.0F
#define SWEEP_ELEVATION NJM_DEG_ANG(20)
#define SWEEP_ARC NJM_DEG_ANG(240)

/* no code names this point. E-105's sensor_create, at 0x0c9163a0, asks
 * collision_init for two shapes from sensor_shape, but sensor_shape holds one.
 * collision_init copies the 52 bytes after it as the second shape. they are
 * this point, shelter_sweep_unit and the first 28 bytes of shelter_ray */
static NJS_POINT3 dat_0ca92058 = {50.0F, 30.0F, 10.0F};

static NJS_POINT3 shelter_sweep_unit = {1.0F, 0.0F, 0.0F};

/* every ray's params. shelter_ray_create sets the length before each */
static struct beam_params shelter_ray = {
    60, {0.3F, 1.8F}, {0xffffffff, 0x00ffffff}, 54, 0.0F, 1.1F,
};

static NJS_POINT3 shelter_sweep_target;
static Angle shelter_sweep_ang;
static Angle shelter_sweep_turn; /* the turn each step */
static int shelter_sweep_steps; /* the steps run so far */
static int shelter_sweep_full_steps; /* the steps it turns at full speed */

static void shelter_camera_sweep_step(struct camera_view *view);

void shelter_camera_restore(struct task *task)
{
    camera_restore();
}

void fun_0c9166a6(void)
{
}

void shelter_camera_sweep_start(NJS_POINT3 *target, Angle ang, int steps)
{
    VECTOR_COPY(&shelter_sweep_target, target);
    shelter_sweep_ang = ang - (SWEEP_ARC / 2);
    shelter_sweep_turn = SWEEP_ARC / steps;
    shelter_sweep_steps = 0;
    shelter_sweep_full_steps = steps;
    camera_override(shelter_camera_sweep_step, 0, CAMERA_LOOK_AT);
}

static void shelter_camera_sweep_step(struct camera_view *view)
{
    NJS_POINT3 pos;
    Float distance;

    if (++shelter_sweep_steps > shelter_sweep_full_steps) {
        shelter_sweep_turn--;
        if (shelter_sweep_turn <= 0) {
            shelter_sweep_turn = 0;
        }
    }
    shelter_sweep_ang += shelter_sweep_turn;
    camera_state->target.x = shelter_sweep_target.x;
    camera_state->target.y = shelter_sweep_target.y;
    camera_state->target.z = shelter_sweep_target.z;
    distance = SWEEP_DISTANCE;
    njPushMatrix(&_unit_matrix_);
    njScale(NULL, distance, distance, distance);
    TURN(njRotateY, shelter_sweep_ang);
    njRotateZ(NULL, SWEEP_ELEVATION);
    njCalcPoint(NULL, &shelter_sweep_unit, &pos);
    njPopMatrix(1);
    VECTOR_SUM(&pos, &pos, &shelter_sweep_target);
    camera_state->pos.x = pos.x;
    camera_state->pos.y = pos.y;
    camera_state->pos.z = pos.z;
}

void shelter_ray_create(NJS_POINT3 *pos, Float length)
{
    shelter_ray.length = length;
    beam_create(pos, &shelter_ray);
}
