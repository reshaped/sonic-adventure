/* the node of Chaos 0's figure that wobbles and blinks. its points sway
 * about its z axis, the more the farther they are from the axis */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos0/wobble.h"
#include "chaos/chaos.h"
#include "draw/motion.h"
#include "system/task.h"
#include "system/util.h"

/* the wobble task's motion. its speed.z is the wobble's size, and its
 * phase the sway's */
extern struct task_motion *chaos0_wobble_motion;
/* the model's own points, while the copy stands in for them */
extern NJS_POINT3 *chaos0_wobble_rest;
/* each wobbling point's distance from the z axis, times RADIUS_ANGLE, in
 * wobble_indices' order */
extern Angle *chaos0_wobble_radii;

/* the attrflags chaos0_blink gives the node's first material while frames
 * is even */
#define STEADY_FLAGS                                                           \
    (NJD_SA_SRC | NJD_DA_INV_SRC | NJD_FLAG_USE_ENV | NJD_FLAG_USE_TEXTURE |   \
     NJD_FLAG_USE_ALPHA | NJD_FLAG_CLAMP_U | NJD_FLAG_CLAMP_V | NJD_D_100)

/* the points that wobble, of the model's 159 */
#define WOBBLE_POINTS 42
/* each step, the phase moves on PHASE_STEP, and work->ang.z becomes
 * sin(phase) * size * SWAY */
#define PHASE_STEP 0x1000
#define SWAY 2048.0F
/* a wobbling point r from the z axis turns about it by work->ang.z times
 * (r * RADIUS_ANGLE + SWAY_AT_AXIS) / SWAY_AT_AXIS. so a point on the axis
 * turns by work->ang.z, and each unit farther out adds 3/16 of it */
#define RADIUS_ANGLE 1536.0F
#define SWAY_AT_AXIS 0x2000
/* a wobble whose size is WOBBLE_LEAST or less stops, and its phase starts
 * again from 0. a larger one keeps WOBBLE_DECAY of its size each step */
#define WOBBLE_LEAST 0.1F
#define WOBBLE_DECAY 0.98F
/* chaos0_wobble_push takes a size at most WOBBLE_MOST, and keeps PUSH_SHARE
 * of it */
#define WOBBLE_MOST 2.0F
#define PUSH_SHARE 0.6F

/* the wobble task's work->mode: its first step starts it, and the rest run
 * it */
enum wobble_mode { WOBBLE_START, WOBBLE_RUN };

/* the points that wobble, ending at -1 */
static Sint16 wobble_indices[WOBBLE_POINTS + 1] = {
#include "b_chaos0/wobble/wobble_indices.inc"
};

void chaos0_blink(int frames)
{
    /* @hack spelling: the node through its address, which SHC pools to read
     * the model at offset 4 (MOV.L @(4,R5),R5 at 0x0c903586). as
     * chaos0_wobble_node.model, 18 of 19 words differ */
    NJS_MATERIAL *material = (&chaos0_wobble_node)->model->mats;

    if (frames & 1) {
        material->attrflags &= ~NJD_SA_MASK;
        material->attrflags |= NJD_SA_ONE;
    } else {
        material->attrflags = STEADY_FLAGS;
    }
}

void chaos0_set_material(NJS_OBJECT *object)
{
    NJS_MATERIAL *material;

    if (object->model != NULL) {
        material = object->model->mats;
        if (material->attrflags & MATERIAL_SECOND_COLOR) {
            njSetConstantMaterial(&chaos_record->colors[1]);
        } else if (material->attrflags & MATERIAL_BLINKS) {
            njSetConstantMaterial(&chaos_record->colors[0]);
        }
    }
}

static void wobble_destroy(struct task *task)
{
    NJS_OBJECT *object = &chaos0_wobble_node;

    heap_free(object->model->points);
    heap_free(chaos0_wobble_radii);
    object->model->points = chaos0_wobble_rest;
}

void chaos0_wobble_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_OBJECT *object = &chaos0_wobble_node;

    switch (work->mode) {
    case WOBBLE_START: {
        NJS_POINT3 *points;
        NJS_POINT3 *point;
        Sint32 count;
        NJS_MODEL *model = object->model;
        Sint16 *index;
        Angle *radius;

        count = model->nbPoint;
        chaos0_wobble_rest = model->points;
        points = heap_alloc(count * sizeof(NJS_POINT3));
        if (points == NULL) {
            task_destroy(task);
            return;
        }
        chaos0_wobble_radii = heap_alloc(WOBBLE_POINTS * sizeof(Angle));
        if (chaos0_wobble_radii == NULL) {
            heap_free(points);
            task_destroy(task);
            return;
        }
        object->model->points = points;
        njMemCopy4(points, chaos0_wobble_rest, count * 3);
        radius = chaos0_wobble_radii;
        for (index = wobble_indices; *index >= 0;) {
            point = &chaos0_wobble_rest[*index++];
            *radius++ =
                (Angle)(njSqrt((point->x * point->x) + (point->y * point->y)) *
                        RADIUS_ANGLE);
        }
        chaos0_wobble_motion = motion;
        motion->phase = 0;
        task->destructor = wobble_destroy;
        work->mode = WOBBLE_RUN;
    }
        /* fall through */
    case WOBBLE_RUN: {
        NJS_POINT3 *source;
        NJS_POINT3 *dest;
        Sint16 *index;
        Angle *radius;
        Angle distance;
        Angle ang;
        Float swing;

        if (njAbs(motion->speed.z) > WOBBLE_LEAST) {
            motion->speed.z *= WOBBLE_DECAY;
        } else {
            motion->speed.z = 0.0F;
            motion->phase = 0;
        }
        motion->phase += PHASE_STEP;
        swing = njSin(motion->phase);
        work->ang.z = (Angle)(swing * motion->speed.z * SWAY);
        radius = chaos0_wobble_radii;
        for (index = wobble_indices; *index >= 0; index++) {
            source = &chaos0_wobble_rest[*index];
            dest = &object->model->points[*index];
            distance = *radius++;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateZ,
                     work->ang.z * (distance + SWAY_AT_AXIS) / SWAY_AT_AXIS,
                     ang);
            njCalcVector(NULL, source, dest);
            njPopMatrix(1);
        }
        break;
    }
    }
}

void chaos0_wobble_push(Float push)
{
    chaos0_wobble_motion->speed.z += push;
    chaos0_wobble_motion->speed.z = (chaos0_wobble_motion->speed.z < WOBBLE_MOST
                                         ? chaos0_wobble_motion->speed.z
                                         : WOBBLE_MOST) *
                                    PUSH_SHARE;
}
