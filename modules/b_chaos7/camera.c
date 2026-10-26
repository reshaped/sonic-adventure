/* the Perfect Chaos fight's cameras: camera mode CHAOS_W's step, the shots
 * the fight plays, and a step that watches player 0 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/camera.h"
#include "b_chaos7/fight.h"
#include "camera/camera.h"
#include "draw/motion.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* shot_mode's values. under SHOT_ONCE a shot holds its end, and under
 * SHOT_LOOP it starts over. under SHOT_FIXED no shot plays, and the camera
 * holds shot_fixed_pos, looking at shot_fixed_target. nothing changes
 * shot_mode from SHOT_ONCE, or sets shot_fixed_pos or shot_fixed_target */
#define SHOT_ONCE 0
#define SHOT_LOOP 1
#define SHOT_FIXED 2

#pragma section HEAD
/* nothing reads dat_0c901b78 */
static Float dat_0c901b78 = 400.0F;
/* while player 0 is within camera_near of Chaos, the camera's target is
 * camera_near_height above Chaos, not camera_boss_target_offset.y. the
 * distance is across the ground, and squared */
static Float camera_near = 1200.0F * 1200.0F;
static Float camera_near_height = 100.0F;
#pragma section

static Sint8 shot_mode = SHOT_ONCE;
/* how far the playing shot has come, from 0 to 1 */
static Float shot_progress = 0.0F;

/* a shot's vector, as the Ninja vector calls take it, and as an array of its
 * components */
union shot_vector {
    NJS_VECTOR vector;
    Float component[3];
};

Sint8 chaos7_shot_done;
Sint8 chaos7_shot_held;
static struct chaos7_shot *shot_playing;
static NJS_POINT3 shot_fixed_target;
static NJS_POINT3 shot_fixed_pos;
/* what the playing shot adds each step to shot_progress, the target's height
 * and the camera's position */
static Float shot_rate;
static Float shot_rise;
static union shot_vector shot_move;
/* where the shot has the camera, and its target */
static union shot_vector shot_pos;
static NJS_POINT3 shot_target;

/* player 0's squared distance from Chaos, across the ground */
static Float boss_distance_squared(void)
{
    Float x = player_work[0]->pos.x - boss_work->pos.x;
    Float z = player_work[0]->pos.z - boss_work->pos.z;

    return (x * x) + (z * z);
}

/* put the camera chaos7_camera_back from player 0, on his side away from
 * Chaos, and chaos7_camera_height above him. its target is
 * camera_boss_target_offset from Chaos, but camera_near_height above Chaos
 * while player 0 is within camera_near */
static void chaos7_camera_step(struct camera_view *view)
{
    struct task_work *player = player_work[0];
    Angle ang;
    Float x;
    Float z;

    if (boss_distance_squared() > camera_near) {
        camera_state->target.x = boss_work->pos.x + camera_boss_target_offset.x;
        camera_state->target.y = boss_work->pos.y + camera_boss_target_offset.y;
        camera_state->target.z = boss_work->pos.z + camera_boss_target_offset.z;
    } else {
        camera_state->target.x = boss_work->pos.x + camera_boss_target_offset.x;
        camera_state->target.y = boss_work->pos.y + camera_near_height;
        camera_state->target.z = boss_work->pos.z + camera_boss_target_offset.z;
    }
    x = boss_work->pos.x - player->pos.x;
    z = boss_work->pos.z - player->pos.z;
    ang = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
    camera_state->pos.x = player->pos.x - (chaos7_camera_back * njCos(ang));
    camera_state->pos.y = player->pos.y + chaos7_camera_height;
    camera_state->pos.z = player->pos.z - (chaos7_camera_back * njSin(ang));
}

/* in HEAD, at 0x0c9003a0, which 1ST_READ.BIN's camera_step_chaos_w jumps to */
#pragma section HEAD
void chaos7_camera_entry(struct camera_view *view)
{
    chaos7_camera_step(view);
}
#pragma section

/* the playing shot's camera step. at progress 0, it puts the camera at the
 * shot's from, and the target from_height above Chaos. each step
 * chaos7_shot_held is clear, it moves both a step on. once the progress passes
 * 1, it sets chaos7_shot_done. under SHOT_ONCE it then holds the shot's end */
static void chaos7_shot_step(struct camera_view *view)
{
    struct chaos7_shot *shot = shot_playing;

    /* @hack redundant: shot_rate's address, taken and dropped, which with the
     * vector's dropped below puts shot_rate's address in R11 and
     * shot_target's in R10 (0x0c91aeea, 0x0c91aeee); without it, the two
     * trade, and 16 of 192 words and 2 pool words differ */
    (void)&shot_rate;
    if (shot_mode == SHOT_FIXED) {
        camera_state->target.x = shot_fixed_target.x;
        camera_state->target.y = shot_fixed_target.y;
        camera_state->target.z = shot_fixed_target.z;
        camera_state->pos.x = shot_fixed_pos.x;
        camera_state->pos.y = shot_fixed_pos.y;
        camera_state->pos.z = shot_fixed_pos.z;
        return;
    }
    if (shot_progress == 0.0F) {
        chaos7_shot_done = 0;
        shot_rate = 1.0F / shot->steps;
        shot_target = boss_work->pos;
        shot_target.y += shot->from_height;
        shot_rise = (shot->to_height - shot->from_height) * shot_rate;
        shot_pos.vector = shot->from;
        shot_move.vector = shot->to;
        njSubVector(&shot_move.vector, &shot->from);
        {
            /* @hack spelling: shot_move's components reached through its
             * address, which reloads the address from the pool after
             * njSubVector, into R3 (0x0c91af58); without it, SHC keeps it
             * in R9, saved, the function runs 2 bytes longer, and 117 of 192
             * words differ */
            Float *x = (&shot_move)->component;
            /* @hack spelling: a second pointer, at, that each product goes
             * through, which puts the cursor over shot_move in R3 (0x0c91af58
             * to 0x0c91af74); without it, the cursor takes R4, and 9 of 192
             * words differ */
            Float *at = x;
            Float *y;
            Float *z;

            *at *= shot_rate;
            y = x + 1;
            at = y;
            *at *= shot_rate;
            z = y + 1;
            at = z;
            *z = *at * shot_rate;
            /* @hack redundant: the address of the vector z steps through, taken
             * and dropped, which with shot_rate's dropped above puts
             * shot_rate's address in R11 and shot_target's in R10 (0x0c91aeea,
             * 0x0c91aeee); without it, the two trade, and 16 of 192 words and
             * 2 pool words differ */
            (void)&((union shot_vector *)(z - 2))->vector;
            /* @hack redundant: a block that never runs, which keeps the cursor
             * over shot_move out of shot's R13, in R3 (0x0c91af58 to
             * 0x0c91af74); without it, the cursor takes R13, and 9 of 192
             * words differ */
            if (0) {
                struct camera_state *camera;
                struct camera_state *state;

                *&shot_progress = 0.5F;
                camera = camera_state;
                state = camera;
                state->ang_x = 5;
                camera->ang_y = 7;
                camera->distance = shot->steps;
            }
        }
    }
    if (!chaos7_shot_held) {
        shot_progress += shot_rate;
        if (shot_progress > 1.0F) {
            chaos7_shot_done = 1;
            shot_progress = 1.0F;
            if (shot_mode == SHOT_LOOP) {
                shot_progress = 0.0F;
            }
        } else {
            shot_target.y += shot_rise;
            /* @hack spelling: shot_move reached through its address, which
             * reloads the address from the pool for the call (0x0c91afb0);
             * without it, SHC keeps it in R8, saved, the function runs 4
             * bytes longer, and 185 of 192 words and 7 pool words differ */
            njAddVector(&shot_pos.vector, &(&shot_move)->vector);
        }
    }
    camera_state->target.x = shot_target.x;
    camera_state->target.y = shot_target.y;
    camera_state->target.z = shot_target.z;
    /* @hack type: shot_pos read as an array of three, which loads each
     * component's own address from the pool (0x0c91afbc, 0x0c91afc8,
     * 0x0c91afe6); without it, SHC keeps shot_pos's address in R9, saved,
     * the function runs 2 bytes longer, and 178 of 192 words and 12 pool words
     * differ */
    camera_state->pos.x = (*(Float(*)[3])(&shot_pos))[0];
    camera_state->pos.y = (*(Float(*)[3])(&shot_pos))[1];
    camera_state->pos.z = (*(Float(*)[3])(&shot_pos))[2];
}

void chaos7_shot_start(struct chaos7_shot *shot)
{
    shot_playing = shot;
    shot_progress = 0.0F;
    camera_override_high(chaos7_shot_step, CAMERA_ADJUST_NONE, CAMERA_LOOK_AT);
    chaos7_shot_held = 0;
}

void chaos7_camera_watch_step(struct camera_view *view)
{
    struct task_work *player = player_work[0];
    /* @hack spelling: the view's pos.y written through a pointer to its pos, in
     * R7 (0x0c91b026, 0x0c91b02a), so the store at 0x0c91b04e and the read
     * of pos.x after it share R0's 4; without it, 33 of 39 words differ */
    NJS_POINT3 *pos = &view->pos;

    camera_state->target.x = player->pos.x;
    camera_state->target.y = player->pos.y;
    camera_state->target.z = player->pos.z;
    pos->y = player->pos.y;
    camera_state->pos.x = view->pos.x;
    camera_state->pos.y = view->pos.y;
    camera_state->pos.z = view->pos.z;
}
