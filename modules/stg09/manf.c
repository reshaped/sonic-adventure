/* O MANF hinged flap, which opens while its switch is on */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/manf.h"
#include "system/task.h"
#include "system/util.h"

/* the flap's fixed part, whose child is the flap */
extern NJS_OBJECT manf_object;
extern NJS_MOTION manf_motion;
extern NJS_ACTION manf_action;

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)
#define REACH 23.0F /* the radius of its copy in the collision world */

#define SE_FLAP 265

enum manf_mode { FLAP_SHUT, FLAP_OPENING, FLAP_OPEN, FLAP_CLOSING };

static Float manf_frame_rate = 0.3F; /* the action's frames a step */
/* no code uses these words. they keep manf_shape at its address in the
 * image, and hold sr_door_open_range's and sr_door_close_range's values */
static Float dat_0cb13bbc = 40.0F;
static Float dat_0cb13bc0 = 50.0F;
/* the flap's shape, which manf_run moves with the flap */
static struct collision_shape manf_shape = {
    SHAPE_RECTANGLE2,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, -0.22000027F},
    8.17F,
    0.5F,
    7.93F,
    {0, NJM_DEG_ANG(90), 0},
    0,
    NULL,
};

/* pose the flap at frame of manf_action. point gets the middle of the flap,
 * in manf_object's model space. vector gets the way the flap points from its
 * hinge, and angle, unless null, the flap's pitch. returns TRUE, as pose_action
 * does */
static Bool manf_pose(Float frame, NJS_POINT3 *point, NJS_VECTOR *vector,
                      Angle *angle)
{
    /* Ninja needs a matrix on an 8-byte boundary, and the stack is aligned
     * only to 4. matrix points at the boundary at or below room's start, and
     * pad keeps that boundary inside the struct */
    struct {
        NJS_MATRIX *matrix;
        Float pad;
        NJS_MATRIX room;
    } pose;
    NJS_POINT3 tip;
    Float *at;

    pose.matrix = (NJS_MATRIX *)((Uint32)&pose.room & ~7);
    njPushMatrix(&_unit_matrix_);
    if (pose_action(NULL, &manf_action, frame, 1, pose.matrix)) {
        /* @hack spelling: tip set through a cursor over its floats, which SHC
         * steps with ADD #4,R4 (0x0c90f9aa, 0x0c90f9b2). through the fields,
         * 60 of manf_pose's 91 words differ */
        at = &tip.x;
        *at++ = 0.0F;
        *at++ = -0.25F;
        *at = 7.93F;
        njTranslateV(NULL, (NJS_VECTOR *)manf_object.child->pos);
        njCalcVector(pose.matrix, &tip, vector);
        njCalcPoint(pose.matrix, &tip, point);
        njPopMatrix(1);
        if (angle) {
            *angle = (Angle)(atan2f(vector->y, vector->z) * ANGLE_PER_RADIAN);
        }
        return TRUE;
    }
    njPopMatrix(1);
    return FALSE;
}

static void manf_init(struct task *task);
static void manf_destroy(struct task *task);
static void manf_run(struct task *task);
static void manf_draw(struct task *task);

void manf_step(struct task *task)
{
    /* @hack redundant: a read of task->work that nothing uses, which keeps task
     * in R14 across the range check (MOV R4,R14 at 0x0c90fa1e). without it,
     * SHC keeps task on the stack, and 12 of manf_step's 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        manf_init(task);
    }
}

static void manf_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT object;

    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        /* the collision world holds the fixed part without its child, the
         * flap */
        object = manf_object;
        object.child = NULL;
        casino_world_add(task, &object, REACH, WORLD_MOVES);
        /* @bug memory: a count of 2 shapes from manf_shape, which is
         * one (MOV #2,R3 at 0x0c90fa9c). the record's second shape copies
         * the 52 bytes past manf_shape, counter.c's data at 0x0cb13bf8 */
        collision_init(task, &manf_shape, COLLISION_OBJECT, 1, 2);
    }
    task->draw = manf_draw;
    task->step = manf_run;
    task->destructor = manf_destroy;
    work->timer = 0;
    work->mode = FLAP_SHUT;
}

static void manf_destroy(struct task *task)
{
    casino_world_remove(task);
}

static void manf_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;
    NJS_POINT3 point;
    NJS_VECTOR vector;
    /* @hack spelling: a pitch for each way, which SHC keeps apart, opening's at
     * sp+4 and closing's at sp+0 (0x0c90fcd8, 0x0c90fd74). as one pitch, 59
     * of manf_run's 237 words differ */
    Angle opening;
    Angle closing;
    Float frame;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    manf_draw(task);
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> ID ");
        return;
    }
    collision = work->collision;
    switch (work->mode) {
    case FLAP_SHUT:
        if ((Sint16)work->scl.z) {
            if (switch_is_on((Sint16)work->scl.z)) {
                work->mode = FLAP_OPENING;
                work->timer = 0;
            }
        }
        break;
    case FLAP_OPEN:
        /* the image makes this call, and nothing reads its result */
        player_nearest(&work->pos);
        if ((Sint16)work->scl.z) {
            if (!switch_is_on((Sint16)work->scl.z)) {
                work->mode = FLAP_CLOSING;
                work->timer = 0;
            }
        }
        break;
    case FLAP_OPENING:
        frame = TASK_TIMER(work) * manf_frame_rate;
        if (frame >= manf_motion.nbFrame - 1.0F) {
            sound_play(SE_FLAP, 0, 0, 0);
            work->mode = FLAP_OPEN;
            work->timer = 0;
        } else {
            if (manf_pose(frame, &point, &vector, &opening)) {
                collision->shapes->center.x = point.x;
                collision->shapes->center.y = point.y;
                collision->shapes->center.z = point.z;
                collision->shapes->ang.z = -opening;
            }
        }
        break;
    case FLAP_CLOSING:
        frame = TASK_TIMER(work) * manf_frame_rate;
        if (frame >= manf_motion.nbFrame - 1.0F) {
            sound_play(SE_FLAP, 0, 0, 0);
            work->mode = FLAP_SHUT;
            work->timer = 0;
        } else {
            frame = manf_motion.nbFrame - frame - 1.0F;
            if (manf_pose(frame, &point, &vector, &closing)) {
                collision->shapes->center.x = point.x;
                collision->shapes->center.y = point.y;
                collision->shapes->center.z = point.z;
                collision->shapes->ang.z = -closing;
            }
        }
        break;
    }
    casino_world_test_near(task, REACH);
    collision_register(work);
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the image's
     * CMP/PL (0x0c90fd94). TASK_TIMER is a Uint16, so <= 0 holds only at 0.
     * the decrement undoes the wrap, which holds the timer at 65535. as == 0,
     * the test is TST, and 2 of manf_run's 237 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
}

static void manf_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &manf_object;
    NJS_MOTION *motion = &manf_motion;
    Angle ang;
    Float frame;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch (work->mode) {
    case FLAP_OPEN:
        draw_motion(object, motion, motion->nbFrame - 1.0F);
        break;
    case FLAP_SHUT:
        draw_tree(object);
        break;
    case FLAP_OPENING:
        frame = TASK_TIMER(work) * manf_frame_rate;
        if (frame >= motion->nbFrame - 1.0F) {
            frame = motion->nbFrame - 1.0F;
        }
        draw_motion(object, motion, frame);
        break;
    case FLAP_CLOSING:
        frame = motion->nbFrame - (TASK_TIMER(work) * manf_frame_rate) - 1.0F;
        if (frame < 0.0F) {
            frame = 0.0F;
        }
        draw_motion(object, motion, frame);
        break;
    default:
        break;
    }
    njPopMatrix(1);
}
