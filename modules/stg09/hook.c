/* O FOOK hook, which pulls a player it touches toward a spot below it */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/hook.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT hook_object;

/* the hook starts only within about 700 units of the camera and player 0.
 * otherwise it's destroyed on its next step */
#define RANGE ((700.0F * 700.0F) + 10.0F)
/* the hook draws only within this distance of the camera */
#define DRAW_DISTANCE 1000.0F
/* the radius of the sphere hook_draw tests against the view.
 * casino_world_add and casino_world_test_near take it too, for the hook's
 * copy */
#define REACH 80.0F
/* the spot a player is pulled toward, in the hook's model space */
#define SPOT_X 0.0F
#define SPOT_Y (-30.0F)
#define SPOT_Z (-15.0F)

/* a player touching this box is pulled */
static struct collision_shape hook_shape = {
    SHAPE_RECTANGLE | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, -24.0F, 30.0F},
    35.0F,
    20.0F,
    26.0F,
    {0},
    0,
    NULL,
};
/* the speed of the pull toward the spot */
static Float hook_pull = 1.5F;

static void hook_destroy(struct task *task)
{
    casino_world_remove(task);
}

static void hook_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (njDistanceP2P(&camera_work->pos, &work->pos) > DRAW_DISTANCE) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, REACH)) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&hook_object);
    njPopMatrix(1);
}

/* the hook pulls only on a frame's steps after the first, while frame_step
 * is past 0 */
static void hook_run(struct task *task)
{
    struct task_work *work = task->work;
    int player;
    NJS_POINT3 spot;
    NJS_VECTOR pull;
    Float scale;
    Float *out;
    Float *spot_at;
    Float *player_at;

    hook_draw(task);
    if (placement_editing(task)) {
        return;
    }
    if (frame_step > 0 && (work->collision->flags & COLLISION_HIT)) {
        player = -1;
        if (work->collision->hit->owner->work == player_work[0]) {
            player = 0;
        }
        if (work->collision->hit->owner->work == player_work[1]) {
            player = 1;
        }
        if (player != -1) {
            /* @hack spelling: the spot, the pull and its scaling written
             * through walking Float pointers, which SHC reads with FMOV.S @Rm+,
             * as at 0x0c91b022. as field arithmetic in hook_run, 102 of its 177
             * words differ */
            out = &spot.x;
            *out++ = SPOT_X;
            *out++ = SPOT_Y;
            *out = SPOT_Z;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y);
            njCalcPoint(NULL, &spot, &spot);
            njPopMatrix(1);
            out = &pull.x;
            spot_at = &spot.x;
            player_at = &player_work[player]->pos.x;
            *out++ = *spot_at++ - *player_at++;
            *out++ = *spot_at++ - *player_at++;
            *out = *spot_at - *player_at;
            scale = hook_pull / njScalor(&pull);
            out = &pull.x;
            *out++ *= scale;
            *out++ *= scale;
            *out *= scale;
            if (njInnerProduct(&player_motion[player]->speed, &pull) >= 0.0F) {
                player_updraft(player, pull.x, pull.y > 0.0F ? 0.0F : pull.y,
                               pull.z);
            }
        }
    }
    casino_world_test_near(task, REACH);
    collision_register(work);
}

void hook_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    task->draw = hook_draw;
    task->step = hook_run;
    task->destructor = hook_destroy;
    if (placement_editing(task)) {
        return;
    }
    work->ang.x = 0;
    work->ang.z = 0;
    collision_init(task, &hook_shape, COLLISION_OBJECT, 1, 1);
    casino_world_add(task, &hook_object, REACH, 0);
}
