/* the camera in a battle with a rival */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/battle.h"
#include "camera/camera.h"
#include "player/player.h"
#include "player/rival.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* how far ORBIT moves the camera out or in, and turns it round player 0, in
 * a step. ORBIT_PITCH tilts the orbit about x, a 32nd of a turn */
#define ORBIT_STEP 2.0F
#define ORBIT_TURN 0x200
#define ORBIT_PITCH (-0x800)
/* how far the camera rises above player 0's eyes for each unit of
 * camera_state's distance */
#define RISE 0.06F
/* what camera_start_battle sets turn.x and turn.y to: a 256th of a turn */
#define AIM_TURN 0x100

/* how battle_camera_step places the camera. FACE and BETWEEN aim it from
 * battle_distance beyond player 0, on the side away from the rival, at the
 * point battle_distance past the rival. both distances are in x and z.
 * battle_camera_aim swings the camera toward that aim round player 0, no
 * faster than turn allows */
enum battle_camera_mode {
    BATTLE_CAMERA_ORBIT, /* circling player 0, steered by pad 0 */
    BATTLE_CAMERA_FACE, /* looking at the height of player 0's feet */
    BATTLE_CAMERA_BETWEEN /* looking at the mean height of both players' feet */
};

struct battle_camera {
    Sint8 mode; /* a battle_camera_mode */
    NJS_POINT3 pos;
    NJS_POINT3 target;
    /* y is the camera's height above player 0, and z its distance away */
    NJS_POINT3 offset;
    /* y is the heading from player 0 to pos. x follows the pitch from pos to
     * target, as battle_camera_aim turns it */
    struct angles ang;
    Uint8 unk_34[12];
    /* the most battle_camera_aim turns ang.x and ang.y in a step */
    struct angles turn;
    /* pos and ang as they were before battle_camera_aim's last call */
    NJS_POINT3 last_pos;
    struct angles last_ang;
    struct task *task; /* the task that drives the rival */
    struct task_work *player;
    struct task_work *rival;
};

static void battle_camera_step(struct camera_view *view);
static void battle_camera_aim(struct battle_camera *camera,
                              struct task_work *player,
                              struct task_work *rival);

/* how far the camera keeps from player 0 */
static Float battle_distance = 50.0F;

static struct battle_camera battle_camera;

void camera_start_battle(struct task *task)
{
    struct battle_camera *camera = &battle_camera;
    struct rival *rival = task->extra;
    struct player *record;

    camera_override(battle_camera_step, 0, CAMERA_LOOK_AT);
    fill_bytes(camera, 0, sizeof(*camera));
    camera->mode = BATTLE_CAMERA_BETWEEN;
    camera->task = task;
    camera->turn.x = AIM_TURN;
    camera->turn.y = AIM_TURN;
    /* @hack spelling: record assigned inside the test, which tests R0 and
     * copies it to R4 in the BT/S's slot (0x8c04ba04, 0x8c04ba08); assigned
     * ahead of the test, 78 of 85 words differ */
    if ((record = player_get(0)) != NULL) {
        camera->offset.x = 0.0F;
        camera->offset.y =
            record->consts.eye_height + (camera_state->distance * RISE);
        camera->offset.z = 0.0F;
    } else {
        camera->offset.y = 50.0F;
        camera->offset.z = 50.0F;
    }
    switch (rival->kind) {
    case BATTLE_SONIC_KNUCKLES:
    case BATTLE_KNUCKLES_SONIC:
    case BATTLE_KNUCKLES_TAILS:
        battle_distance = 50.0F;
        break;
    case BATTLE_SONIC_E102:
    case BATTLE_E102_SONIC:
    case BATTLE_E102_TAILS:
        battle_distance = 70.0F;
        break;
    }
}

static void battle_camera_step(struct camera_view *view)
{
    NJS_VECTOR away;
    struct battle_camera *camera = &battle_camera;
    struct task_work *player;
    struct task_work *rival;
    struct player *record;

    if (camera_work == NULL || camera_state == NULL) {
        return;
    }
    camera->player = player = player_work[0];
    camera->rival = rival = player_work[1];
    record = player_get(0);
    if (player == NULL || rival == NULL || record == NULL) {
        return;
    }
    switch (camera->mode) {
    case BATTLE_CAMERA_ORBIT:
        camera->offset.x = 0.0F;
        camera->offset.y =
            record->consts.eye_height + (camera_state->distance * RISE);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, camera->ang.y);
        njRotateX(NULL, ORBIT_PITCH);
        camera->offset.z = battle_distance;
        njCalcPoint(NULL, &camera->offset, &camera->pos);
        camera->pos.x = player->pos.x + camera->pos.x;
        camera->pos.y = player->pos.y + camera->pos.y;
        camera->pos.z = player->pos.z + camera->pos.z;
        camera->offset.z = -battle_distance;
        njCalcPoint(NULL, &camera->offset, &camera->target);
        camera->target.x = player->pos.x + camera->target.x;
        camera->target.y = player->pos.y + camera->target.y;
        camera->target.z = player->pos.z + camera->target.z;
        njPopMatrix(1);
        if (per_info[0]->on & PDD_DGT_TY) {
            if (per_info[0]->on & PDD_DGT_TL) {
                battle_distance += ORBIT_STEP;
            } else if (per_info[0]->on & PDD_DGT_TR) {
                battle_distance -= ORBIT_STEP;
            }
        } else {
            if (per_info[0]->on & PDD_DGT_TL) {
                camera->ang.y -= ORBIT_TURN;
            } else if (per_info[0]->on & PDD_DGT_TR) {
                camera->ang.y += ORBIT_TURN;
            }
        }
        break;
    case BATTLE_CAMERA_FACE:
        if (record != NULL) {
            camera->offset.y =
                record->consts.eye_height + (camera_state->distance * RISE);
            camera->offset.z = battle_distance;
        }
        VECTOR_DIFF(&away, &player->pos, &rival->pos);
        if (vector_magnitude(&away) != 0.0F) {
            vector_unit_across(&away, &away);
            camera->pos.x = player->pos.x + (away.x * camera->offset.z);
            camera->pos.y = player->pos.y + camera->offset.y;
            camera->pos.z = player->pos.z + (away.z * camera->offset.z);
            camera->target.x = rival->pos.x - (away.x * camera->offset.z);
            camera->target.y = camera->pos.y - camera->offset.y;
            camera->target.z = rival->pos.z - (away.z * camera->offset.z);
            /* @hack spelling: the aim called in each arm, which SHC joins
             * into one call of FACE's own (BSR at 0x8c04bd92); as one call
             * after the if, SHC merges it with BETWEEN's, 223 of 463 words
             * differ and the code runs 32 bytes short */
            battle_camera_aim(camera, player, rival);
        } else {
            battle_camera_aim(camera, player, rival);
        }
        break;
    case BATTLE_CAMERA_BETWEEN:
        if (record != NULL) {
            if (PLAYER_VARS(camera->player)->character == CHARACTER_E102) {
                camera->offset.y = record->consts.eye_height +
                                   (camera_state->distance * RISE) + 10.0F;
            } else {
                camera->offset.y = record->consts.eye_height * 3.0F;
            }
            camera->offset.z = battle_distance;
        }
        VECTOR_DIFF(&away, &player->pos, &rival->pos);
        vector_unit_across(&away, &away);
        camera->pos.x = player->pos.x + (away.x * camera->offset.z);
        camera->pos.y = player->pos.y + camera->offset.y;
        camera->pos.z = player->pos.z + (away.z * camera->offset.z);
        camera->target.x = (player->pos.x + rival->pos.x) * 0.5F;
        camera->target.y = (player->pos.y + rival->pos.y) * 0.5F;
        camera->target.z = (player->pos.z + rival->pos.z) * 0.5F;
        battle_camera_aim(camera, player, rival);
        break;
    }
    camera_state->target.x = camera->target.x;
    camera_state->target.y = camera->target.y;
    camera_state->target.z = camera->target.z;
    camera_state->pos.x = camera->pos.x;
    camera_state->pos.y = camera->pos.y;
    camera_state->pos.z = camera->pos.z;
    camera_state->ang_x = camera_state->ang_y = camera_state->ang_z = 0;
}

/* swing the camera round player 0 toward the view the caller set. ang.y
 * turns toward the heading from the target to pos, by at most turn.y in a
 * step. ang.x turns toward the pitch from pos to the target, by at most
 * turn.x. pos then moves round player 0 to heading ang.y, keeping its
 * distance in x and z. the target moves in x and z to the opposite heading,
 * as far from player 0 as pos was from the rival */
static void battle_camera_aim(struct battle_camera *camera,
                              struct task_work *player, struct task_work *rival)
{
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a vector. the image's ADD #-32,R15 reserves them
     * (0x8c04bf4a); without them, 2 of 125 words differ */
    NJS_VECTOR unused;
    Float sine;
    Float cosine;
    Angle pitch;
    Angle heading;
    Float across;
    Float near;
    Float far;

    camera->last_pos = camera->pos;
    camera->last_ang = camera->ang;
    heading = angle_arc_tan2(camera->pos.x - camera->target.x,
                             camera->pos.z - camera->target.z);
    across = point_distance_across(&camera->pos, &camera->target);
    pitch = angle_arc_tan2(camera->target.y - camera->pos.y, across);
    camera->ang.y = angle_approach(camera->last_ang.y, heading, camera->turn.y);
    camera->ang.x = angle_approach(camera->last_ang.x, pitch, camera->turn.x);
    far = point_distance_across(&camera->pos, &rival->pos);
    near = point_distance_across(&camera->pos, &player->pos);
    _builtin_fsca(camera->ang.y, &sine, &cosine);
    camera->pos.x = player->pos.x + (near * sine);
    camera->pos.z = player->pos.z + (near * cosine);
    camera->target.x = player->pos.x - (sine * far);
    camera->target.z = player->pos.z - (cosine * far);
}

void camera_get_pos(NJS_POINT3 *pos)
{
    *pos = camera_work->pos;
}

void camera_set_battle_turn(Angle turn)
{
    struct battle_camera *camera = &battle_camera;

    camera->turn.y = turn;
}

void camera_keep_battle(void)
{
    camera_override(battle_camera_step, 0, CAMERA_LOOK_AT);
}
