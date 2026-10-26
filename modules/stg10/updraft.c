/* O Fun fan whose wind lifts player 0 */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg10/updraft.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: updraft_action as an array. SHC then pools its address once,
 * for FRAMES and the draw, and reads its motion at @(4,R3) (0x0c911436). as
 * one NJS_ACTION, SHC pools the address of its motion field too. that long
 * makes the code section 32 bytes longer. 3 of updraft_blow's 81 words
 * differ, and 3 of updraft_draw's 42 */
extern NJS_ACTION updraft_action[];

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)
/* the wind carries player 0 in a column of radius 24 around the fan's axis.
 * the column starts BELOW units under the fan and has no top */
#define BELOW 3.0F
#define REACH (24.0F * 24.0F) /* the column's radius, squared */
#define FRAMES (updraft_action->motion->nbFrame)

struct updraft_vars {
    Float frame;
};

#define VARS(work) ((struct updraft_vars *)&(work)->vars)

enum updraft_mode { UPDRAFT_INIT, UPDRAFT_RUN };

static void updraft_init(struct task *task);
static void updraft_blow(struct task *task);
static void updraft_draw(struct task *task);
static void updraft_destroy(struct task *task);

/* a cylinder 34 units across and 3 high, around the fan's position. it
 * pushes every group, and no group's shapes push it */
static struct collision_shape updraft_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* the wind's lift at the fan's height */
static Float updraft_lift = 0.6F;
/* the lift the wind loses for each unit above the fan. the lift reaches 0 at
 * 30 units up. above that, the wind still puts player 0 in
 * PLAYER_MODE_UPDRAFT, with no lift */
static Float updraft_falloff = 0.02F;

/* the set editor labels all three angles, though only angle y turns the fan */
void updraft_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case UPDRAFT_INIT:
            updraft_init(task);
            break;
        case UPDRAFT_RUN:
            updraft_draw(task);
            updraft_blow(task);
            break;
        }
    } else {
        switch (work->mode) {
        case UPDRAFT_INIT:
            updraft_init(task);
            break;
        case UPDRAFT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            updraft_draw(task);
            updraft_blow(task);
            break;
        }
    }
}

static void updraft_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = updraft_draw;
    task->destructor = updraft_destroy;
    collision_init(task, &updraft_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->frame = 0.0F;
    work->mode = UPDRAFT_RUN;
}

static void updraft_blow(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float y;
    Float z;
    Float lift;

    if (player_work[0]->pos.y > work->pos.y - BELOW) {
        x = player_work[0]->pos.x - work->pos.x;
        z = player_work[0]->pos.z - work->pos.z;
        y = player_work[0]->pos.y - work->pos.y;
        if (SQUARE(x) + SQUARE(z) < REACH) {
            lift = updraft_lift - (updraft_falloff * y);
            if (lift < 0.0F) {
                lift = 0.0F;
            }
            player_updraft(0, 0.0F, lift, 0.0F);
        }
    }
    VARS(work)->frame += 1.0F;
    if (VARS(work)->frame >= FRAMES) {
        VARS(work)->frame = 0.0F;
    }
    /* @hack spelling: the record reached through task->work, not work. SHC then
     * keeps task in the frame and loads its work again (MOV.L @(32,R4),R4 at
     * 0x0c911454); through work, updraft_blow runs 8 bytes short and 81 of
     * its 81 words differ */
    collision_register(task->work);
}

static void updraft_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_action(updraft_action, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void updraft_destroy(struct task *task)
{
}
