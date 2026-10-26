/* O SRDOOR pair of sliding doors that open as a player approaches */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/sr_door.h"
#include "system/task.h"
#include "system/util.h"

/* the first door, with the second as its child */
extern NJS_OBJECT sr_door_object;
extern NJS_MOTION sr_door_motion;
extern NJS_ACTION sr_door_action;

/* the doors are destroyed beyond about 600 units of the camera and player 0.
 * RANGE is that distance squared */
#define RANGE 360010.0F

enum sr_door_mode { DOOR_SHUT, DOOR_OPENING, DOOR_OPEN, DOOR_CLOSING };

static Float sr_door_speed = 0.3F; /* motion frames a step */
static Float sr_door_open_range = 40.0F;
static Float sr_door_close_range = 50.0F;
/* a box for each door, which collision_init copies into the record. while
 * the doors move, sr_door_run sets each copy's center to its door's position
 * plus the center here */
static struct collision_shape sr_door_shapes[2] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {7.5F, 15.0F, -0.325F},
        7.5F,
        15.0F,
        -0.675F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-7.5F, 15.0F, -0.325F},
        7.5F,
        15.0F,
        -0.675F,
        {0},
        0,
        NULL,
    },
};

/* set first and second to the doors' positions at frame of the action. it
 * returns TRUE, since pose_action always does */
static Bool sr_door_pose(Float frame, NJS_POINT3 *first, NJS_POINT3 *second)
{
    /* Ninja needs a matrix on an 8-byte boundary, and the stack is aligned
     * only to 4. matrix points at the boundary at or below room's start, and
     * pad keeps that boundary inside the struct */
    struct {
        /* @hack type: matrix in the struct with room, which keeps it in the
         * frame (MOV.L R2,@R14 at 0x0c90f400); as a local of its own it takes
         * R14, the function runs 6 bytes short and 58 of 63 words differ */
        NJS_MATRIX *matrix;
        Float pad;
        NJS_MATRIX room;
    } pose;

    pose.matrix = (NJS_MATRIX *)((Uint32)&pose.room & ~7);
    njPushMatrix(&_unit_matrix_);
    if (pose_action(NULL, &sr_door_action, frame, 0, pose.matrix)) {
        njCalcPoint(pose.matrix, (NJS_POINT3 *)sr_door_object.pos, first);
        njUnitMatrix(NULL);
        if (pose_action(NULL, &sr_door_action, frame, 1, pose.matrix)) {
            njCalcPoint(pose.matrix, (NJS_POINT3 *)sr_door_object.child->pos,
                        second);
            njPopMatrix(1);
            return TRUE;
        }
        njPopMatrix(1);
        return FALSE;
    }
    njPopMatrix(1);
    return FALSE;
}

static void sr_door_init(struct task *task);
static void sr_door_destroy(struct task *task);
static void sr_door_run(struct task *task);
static void sr_door_draw(struct task *task);

void sr_door_step(struct task *task)
{
    /* @hack redundant: work set and never read, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c90f466); without it, task is
     * kept at sp+0, the function is 2 bytes longer and 12 of 16 words
     * differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        sr_door_init(task);
    }
}

static void sr_door_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        collision_init(task, sr_door_shapes, COLLISION_OBJECT, 1, 2);
    }
    task->draw = sr_door_draw;
    task->step = sr_door_run;
    task->destructor = sr_door_destroy;
    work->timer = 0;
    work->mode = DOOR_SHUT;
}

static void sr_door_destroy(struct task *task)
{
}

static void sr_door_run(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;
    NJS_POINT3 first;
    NJS_POINT3 second;
    struct task_work *player;
    Float frame;
    Float *center;
    Float *pos;
    Float *offset;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    sr_door_draw(task);
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case DOOR_SHUT:
        player = player_work[player_nearest(&work->pos)];
        if (njDistanceP2P(&player->pos, &work->pos) < sr_door_open_range) {
            sound_play(SE_DOOR, 0, 0, 0);
            work->mode = DOOR_OPENING;
            work->timer = 0;
        }
        break;
    case DOOR_OPEN:
        player = player_work[player_nearest(&work->pos)];
        if (njDistanceP2P(&player->pos, &work->pos) > sr_door_close_range) {
            sound_play(SE_DOOR, 0, 0, 0);
            work->mode = DOOR_CLOSING;
            work->timer = 0;
        }
        break;
    case DOOR_OPENING:
        frame = TASK_TIMER(work) * sr_door_speed;
        if (frame >= sr_door_motion.nbFrame - 1.0F) {
            work->mode = DOOR_OPEN;
            work->timer = 0;
        } else {
            collision = work->collision;
            if (sr_door_pose(frame, &first, &second)) {
                /* @hack spelling: the first door's center summed through Float
                 * cursors, which SHC walks with FMOV.S @Rn+ (0x0c90f6b4,
                 * 0x0c90f6b6); through the members, the function runs 32
                 * bytes long and 136 of 267 words differ */
                center = &collision->shapes[0].center.x;
                pos = &first.x;
                offset = &sr_door_shapes[0].center.x;
                *center++ = *pos++ + *offset++;
                *center++ = *pos++ + *offset++;
                *center = *pos + *offset;
                /* @hack spelling: the second door's sum the same way; through
                 * the members, the function runs 32 bytes long and 141 of 267
                 * words differ */
                center = &collision->shapes[1].center.x;
                pos = &second.x;
                offset = &sr_door_shapes[1].center.x;
                *center++ = *pos++ + *offset++;
                *center++ = *pos++ + *offset++;
                *center = *pos + *offset;
            }
        }
        break;
    case DOOR_CLOSING:
        frame = TASK_TIMER(work) * sr_door_speed;
        if (frame >= sr_door_motion.nbFrame - 1.0F) {
            work->mode = DOOR_SHUT;
            work->timer = 0;
        } else {
            collision = work->collision;
            frame = sr_door_motion.nbFrame - frame - 1.0F;
            if (sr_door_pose(frame, &first, &second)) {
                /* @hack spelling: the first door's sum as in the opening;
                 * through the members, the function runs 10 bytes long and 100
                 * of 267 words differ */
                center = &collision->shapes[0].center.x;
                pos = &first.x;
                offset = &sr_door_shapes[0].center.x;
                *center++ = *pos++ + *offset++;
                *center++ = *pos++ + *offset++;
                *center = *pos + *offset;
                /* @hack spelling: the second door's sum as in the opening;
                 * through the members, the function runs 18 bytes long and 83
                 * of 267 words differ */
                center = &collision->shapes[1].center.x;
                pos = &second.x;
                offset = &sr_door_shapes[1].center.x;
                *center++ = *pos++ + *offset++;
                *center++ = *pos++ + *offset++;
                *center = *pos + *offset;
            }
        }
        break;
    default:
        break;
    }
    collision_register(work);
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the
     * image's CMP/PL (0x0c90f7b2). TASK_TIMER is a Uint16, so <= 0 holds only
     * at 0. the decrement undoes the wrap, which holds the timer at 65535.
     * as == 0, the test is TST, and 2 of 267 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
}

static void sr_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &sr_door_object;
    NJS_MOTION *motion = &sr_door_motion;
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
    case DOOR_OPEN:
        draw_motion(object, motion, motion->nbFrame - 1.0F);
        break;
    case DOOR_SHUT:
        draw_tree(object);
        break;
    case DOOR_OPENING:
        frame = TASK_TIMER(work) * sr_door_speed;
        if (frame >= motion->nbFrame - 1.0F) {
            frame = motion->nbFrame - 1.0F;
        }
        draw_motion(object, motion, frame);
        break;
    case DOOR_CLOSING:
        frame = motion->nbFrame - (TASK_TIMER(work) * sr_door_speed) - 1.0F;
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
