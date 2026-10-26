/* O COWGIRL: a cowgirl figure that plays her action round, with a platform
 * that carries players */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/cowgirl.h"
#include "stg09/helpers.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION cowgirl_action;
/* the action's node PLATFORM_NODE, whose model the platform's copy takes */
extern NJS_OBJECT cowgirl_platform_object;
extern NJS_MESHSET cowgirl_meshset_0;
extern NJS_MESHSET cowgirl_meshset_1;
extern NJS_MESHSET cowgirl_meshset_2;
extern NJS_MESHSET cowgirl_meshset_3;
extern NJS_MESHSET cowgirl_meshset_4;
extern NJS_MESHSET cowgirl_meshset_5;
extern NJS_MESHSET cowgirl_meshset_6;
extern NJS_MESHSET cowgirl_meshset_7;
extern NJS_MESHSET cowgirl_meshset_8;
extern NJS_MESHSET cowgirl_meshset_9;
extern NJS_MESHSET cowgirl_meshset_10;
extern NJS_MESHSET cowgirl_meshset_11;
extern NJS_MESHSET cowgirl_meshset_12;
extern NJS_MESHSET cowgirl_meshset_13;
extern NJS_MESHSET cowgirl_meshset_14;

/* the task destroys itself beyond about 800 of the camera and player 0.
 * placement_try_leave_by takes the range squared */
#define RANGE ((800.0F * 800.0F) + 10.0F)
/* she draws only while a sphere of this radius about her is in view */
#define VIEW_RADIUS 300.0F
/* the collision world tests the platform's copy only while player 0 or 1 is
 * this near it */
#define NEAR 100.0F

#define PLATFORM_NODE 6
#define PLATFORM_FLAGS (WORLD_MEDIUM | WORLD_MOVES | WORLD_SOLID)
/* the copy's turn: PLATFORM_TURN_X about x, the work's y angle plus
 * PLATFORM_TURN_Y about y, and PLATFORM_TURN_Z about z. the copy keeps this
 * turn while the action moves it */
#define PLATFORM_TURN_X (-447)
#define PLATFORM_TURN_Y (-5290)
#define PLATFORM_TURN_Z 147

#define SE_STRUCK 278
#define STRUCK_STEPS 120

#define COWGIRL_SHIFTED 15 /* the meshsets whose UVs a phase shifts */
/* the phases after phase 0, which shows the meshsets' own UVs */
#define COWGIRL_PHASES 3

struct cowgirl_vars {
    NJS_OBJECT *platform; /* its copy in the collision world */
    Uint8 unk_04[4];
    Float frame; /* the frame cowgirl_action shows */
};

#define VARS(work) ((struct cowgirl_vars *)&(work)->vars)
/* the phase showing, 0 to COWGIRL_PHASES */
#define PHASE(work) ((work)->submode)
/* the steps left before SE_STRUCK can play again */
#define STRUCK(work) ((work)->tick)

/* COWGIRL_SHIFTED rows for each phase from 1. each phase shows the texture
 * half further along in u than the phase before, and cowgirl_meshset_14's a
 * quarter */
static struct uv_shift cowgirl_shifts[COWGIRL_PHASES * COWGIRL_SHIFTED] = {
#include "stg09/cowgirl/cowgirl_shifts.inc"
};
static struct collision_shape cowgirl_shapes[4] = {
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 103.7F, 16.52F},
        70.0F,
        225.0F,
        0.0F,
        {0, 0, NJM_DEG_ANG(90)},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE | SHAPE_HURTABLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 38.7F, 16.52F},
        221.0F,
        41.0F,
        50.0F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {131.93F, 147.7F, -11.44F},
        111.0F,
        25.0F,
        0.0F,
        {NJM_DEG_ANG(90), 0, NJM_DEG_ANG(270)},
        0,
        NULL,
    },
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {-28.89F, 121.7F, 18.81F},
        111.0F,
        35.0F,
        0.0F,
        {NJM_DEG_ANG(90), 0, NJM_DEG_ANG(270)},
        0,
        NULL,
    },
};

/* set pos to the platform node's origin at frame, in the world. the work's
 * position and y angle place the figure. returns TRUE, as pose_action always
 * does */
static Bool cowgirl_pose(Float frame, NJS_POINT3 *pos, struct task_work *work)
{
    /* Ninja's matrix calls need the matrix to start on 8 bytes, and the stack
     * holds it on 4. matrix is room's address rounded down to 8: room itself,
     * or pad, the 4 bytes below it. the matrix fits from either. the struct
     * keeps matrix ahead of pad and room in the frame, as the image has it */
    struct {
        NJS_MATRIX *matrix;
        Float pad;
        NJS_MATRIX room;
    } pose;
    NJS_POINT3 origin;
    Float *to;

    pose.matrix = (NJS_MATRIX *)((Uint32)&pose.room & ~7);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    if (pose_action(NULL, &cowgirl_action, frame, PLATFORM_NODE, pose.matrix)) {
        /* @hack spelling: origin stored through a Float cursor, which ADD #4
         * steps (0x0c91c97a to 0x0c91c986). as a byte cursor, 4 of 71 words
         * differ, and through the members, 30 */
        to = &origin.x;
        *to++ = 0.0F;
        *to++ = 0.0F;
        *to = 0.0F;
        njCalcPoint(pose.matrix, &origin, pos);
        njPopMatrix(1);
        return TRUE;
    }
    njPopMatrix(1);
    return FALSE;
}

static void cowgirl_destroy(struct task *task);
static void cowgirl_draw(struct task *task);
static void cowgirl_run(struct task *task);

void cowgirl_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (!placement_try_leave_by(task, RANGE)) {
        task->draw = cowgirl_draw;
        task->step = cowgirl_run;
        task->destructor = cowgirl_destroy;
        work->timer = 0;
        VARS(work)->frame = 0.0F;
        PHASE(work) = 0;
        STRUCK(work) = 0;
        uv_shift_make(cowgirl_shifts, COWGIRL_PHASES * COWGIRL_SHIFTED);
        if (!placement_editing(task)) {
            object = world_object_alloc();
            VARS(work)->platform = object;
            object->evalflags = 0;
            object->scl[0] = 1.0F;
            object->scl[1] = 1.0F;
            object->scl[2] = 1.0F;
            object->ang[0] = PLATFORM_TURN_X;
            object->ang[1] = work->ang.y + PLATFORM_TURN_Y;
            object->ang[2] = PLATFORM_TURN_Z;
            object->pos[0] = work->pos.x;
            object->pos[1] = work->pos.y;
            object->pos[2] = work->pos.z;
            object->model = cowgirl_platform_object.model;
            object->child = NULL;
            object->sibling = NULL;
            work->flags &= ~WORLD_TESTED;
            world_add(PLATFORM_FLAGS, task, object);
            collision_init(task, cowgirl_shapes, COLLISION_OBJECT, 1, 4);
        }
    }
}

static void cowgirl_destroy(struct task *task)
{
    struct task_work *work = task->work;

    uv_shift_free(cowgirl_shifts, COWGIRL_PHASES * COWGIRL_SHIFTED);
    if (VARS(work)->platform != NULL) {
        world_remove(task, VARS(work)->platform);
        world_object_free(VARS(work)->platform);
    }
}

static void cowgirl_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        return;
    }
    if (PHASE(work) != 0) {
        uv_shift_use(&cowgirl_shifts[(PHASE(work) - 1) * COWGIRL_SHIFTED],
                     COWGIRL_SHIFTED);
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_action(&cowgirl_action, VARS(work)->frame);
    njPopMatrix(1);
    if (PHASE(work) != 0) {
        uv_shift_restore(cowgirl_shifts, COWGIRL_PHASES * COWGIRL_SHIFTED);
    }
}

/* move the platform's copy to the platform node's place at the new frame.
 * while the world tests the copy, the force carries a player on it by the
 * copy's move */
static void cowgirl_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_POINT3 pos;
    struct task_force *force;
    NJS_VECTOR moved;
    Float *to;
    Float *from;
    Float *by;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    cowgirl_draw(task);
    VARS(work)->frame += _builtin_fabsf(work->scl.x);
    if (VARS(work)->frame >= cowgirl_action.motion->nbFrame) {
        VARS(work)->frame = 0.0F;
    }
    if (TASK_TIMER(work)++ > (Sint16)work->scl.z) {
        ++PHASE(work);
        if (PHASE(work) > COWGIRL_PHASES) {
            PHASE(work) = 0;
        }
        work->timer = 0;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Frame Pitch");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Z), "-> UV Pitch");
        return;
    }
    if ((work->flags & COLLISION_DAMAGED) && STRUCK(work) == 0) {
        sound_play(SE_STRUCK, 0, 0, 0);
        STRUCK(work) = STRUCK_STEPS;
    }
    work->flags &= ~COLLISION_DAMAGED;
    if (STRUCK(work) > 0) {
        --STRUCK(work);
    }
    collision_register(work);
    object = VARS(work)->platform;
    cowgirl_pose(VARS(work)->frame, &pos, work);
    if (work->flags & WORLD_TESTED) {
        force = task->force;
        /* @hack spelling: moved summed through Float cursors, which FMOV.S @R6+
         * and @R5+ step (0x0c91cd7a to 0x0c91cd9a). through the members, 36
         * of 181 words differ */
        /* @hack type: to set from (Float *)&moved. set from &moved.x, SHC
         * keeps force in the frame, and 60 of 181 words differ */
        to = (Float *)&moved;
        from = &pos.x;
        by = object->pos;
        *to++ = *from++ - *by++;
        *to++ = *from++ - *by++;
        *to = *from - *by;
        force->speed = moved;
    }
    /* @hack type: object->pos, a Float[3], stored as an NJS_POINT3, which
     * copies the three words as longs (0x0c91cdac to 0x0c91cdba). without
     * it, as three element stores, 47 of 181 words differ */
    *(NJS_POINT3 *)object->pos = pos;
    if (player_in_range((NJS_POINT3 *)object->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}
