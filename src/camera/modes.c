/* the steps of the camera's modes and adjust modes, the steps other code
 * hands camera_override, and camera_mode_list */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "b_chaos7/camera.h"
#include "b_e101_r/camera.h"
#include "b_egm3/camera.h"
#include "camera/camera.h"
#include "camera/pathcam.h"
#include "collision/collision.h"
#include "collision/polygon.h"
#include "draw/motion.h"
#include "event/play.h"
#include "path/path.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "shooting/camera.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "system/vector.h"

/* whether a trigger is pulled: a trigger let go reads 0 */
/* @hack spelling: the trigger shifted up a byte and offset by 32768, which
 * SHC tests as SHLL8 and TST (0x8c01ecfa, 0x8c01ecfc). a trigger is a
 * Uint16, whose shift is 0 only for 0; as (trigger) != 0, summed over the
 * four functions that use it, 735 of 790 words differ */
#define TRIGGER_PULLED(trigger) (((trigger) << 8) - 32768 != -32768)
/* how far LR's and POINT's orbits turn a step, per unit of L less R, each
 * shifted up a byte */
#define ORBIT_TURN 0.0056875F
/* how far FOLLOW's and FOLLOW_G's target rises above player 0's eyes, per
 * unit of camera_state's distance */
#define RISE 0.06F
/* FOLLOW's and FOLLOW_G's pitch, a 32nd of a turn down. FOLLOW_G tips it
 * further down by gravity's lean */
#define MANUAL_PITCH (-0x800)
/* the pitch the KNUCKLES modes keep, as SONIC and NEWFOLLOW do */
#define KNUCKLES_PITCH NJM_DEG_ANG(-20)
/* how far a pitch may lie off the one its step keeps */
#define PITCH_SLACK NJM_DEG_ANG(10)
/* how far a pitch past PITCH_SLACK turns back a step */
#define PITCH_EASE NJM_DEG_ANG(1)
/* how far the d-pad turns SURVEY a step */
#define SURVEY_TURN NJM_DEG_ANG(2)
/* how far SURVEY's pitch may lean either way */
#define SURVEY_PITCH_MAX NJM_DEG_ANG(89)
/* how far the stick may turn TAIHO either way */
#define TAIHO_TURN_MAX NJM_DEG_ANG(45)
/* the sound TAIHO plays while its stick turns it */
#define SE_TAIHO_TURN 197
/* how near y angle 0 the drift stops */
#define DRIFT_SLACK NJM_DEG_ANG(5)
/* how far off the aim camera_adjust_ease leaves the x and y angles at its
 * end */
#define EASE_LIMIT_X NJM_DEG_ANG(40)
#define EASE_LIMIT_Y NJM_DEG_ANG(20)

/* what PATHCAM's step keeps in camera_work's vars. the speed it plays at,
 * in keys a step, is the work's scl.x */
struct pathcam_vars {
    Uint8 path; /* which of the route's paths it plays */
    Uint8 unk_01[3];
    Float place; /* how far along the path, in keys */
    struct pathcam_route *route;
};

#define PATHCAM_VARS(work) ((struct pathcam_vars *)&(work)->vars)

/* what camera_start_path's step keeps in camera_work's vars */
struct path_play_vars {
    Uint8 mode; /* the mode camera_start_path gave */
    Uint8 unk_01;
    Uint16 point; /* the path's point the camera is at, one on a step */
    /* the steps CAMERA_PATH_HOLD and CAMERA_PATH_HOLD_2 end after */
    Uint32 steps;
    struct path *path;
};

#define PATH_PLAY_VARS(work) ((struct path_play_vars *)&(work)->vars)

/* camera_swing_phase's values: camera_start_swing's swing closes in, then
 * turns, then stays where it is once round */
enum swing_phase { SWING_CLOSING, SWING_TURNING, SWING_ROUND };

/* how an adjust mode from THREE1 to RELATIVE6C eases the camera */
struct adjust_ease {
    Bool sweeps; /* walls stop the camera's move from where it was */
    Float steps; /* the steps the ease takes */
    /* the first steps, over which the camera's move from before the ease
     * blends out */
    int blend;
};

/* FOLLOW's distance from its target. while Y is held, L adds 2 a step and
 * R takes 2 off */
static Float manual_distance = 50.0F;
/* the board tricks' paths. each point is the camera's offset from player
 * 0, before the offset turns the way he faces */
static struct path_point trick_a_points[56] = {
#include "camera/modes/trick_a_points.inc"
};
static struct path trick_a_path = {PATH_STOP, 56, 111.10587F, trick_a_points,
                                   NULL};
static struct path_point trick_b_points[80] = {
#include "camera/modes/trick_b_points.inc"
};
static struct path trick_b_path = {PATH_STOP, 80, 127.36393F, trick_b_points,
                                   NULL};
/* camera_set_boss_offsets's defaults, and camera_set_boss_limits's */
static NJS_POINT3 default_target_offset = {0.0F, 0.0F, 0.0F};
static NJS_POINT3 default_offset = {0.0F, 15.0F, 30.0F};
static struct angles default_turn_step = {72, 72, 72};
static struct angles default_spin_limit = {509, 509, 509};
static NJS_POINT3 default_move_limit = {2.5F, 1.5F, 0.0F};
/* with from_anchor set, camera_boss_move puts camera_boss_place 30 from player
 * 0 on the way to this point */
static NJS_POINT3 boss_place_anchor = {270.0F, 0.0F, 322.0F};
/* 1 while camera_start_path's step has the camera: from its first step
 * until camera_end_path */
static Sint8 path_playing = 0;

Float trigger_orbit_distance;
Float trigger_orbit_height;
Angle trigger_orbit_angle;
int trigger_orbit_active;
Uint8 dat_8c6c0ce0[4];
int camera_point_follows;
NJS_POINT3 camera_boss_place;
NJS_POINT3 camera_boss_target_offset;
NJS_POINT3 camera_boss_offset;
struct angles camera_boss_turn_step;
struct angles camera_boss_spin_limit;
NJS_POINT3 camera_boss_move_limit;
Angle camera_swing_turned;
Angle camera_swing_turn;
struct task_work *camera_swing_work;
Float camera_swing_height;
Float camera_swing_distance;
Float camera_swing_near;
Float camera_swing_close_step;
Sint8 camera_swing_done;
Sint8 camera_swing_phase;
NJS_POINT3 camera_swing_target;
Angle camera_swing_heading;
Float camera_klamath_height;
int camera_klamath_follows;
Float camera_klamath_sink;
int camera_ashland_follows;
Bool camera_pose_taken;
NJS_POINT3 camera_pose_pos;
struct angles camera_pose_ang;
/* how far TAIHO's stick has turned it from taiho_base */
static struct angles taiho_turn;
/* the angles TAIHO turns from: camera_place's ang as the mode begins */
static struct angles taiho_base;
static struct angles taiho_last_turn; /* taiho_turn the step before */

/* roll the camera on Sky Deck's act 2, whose ship tilts, and level it
 * elsewhere. the roll is how far gravity's angles lean the world's up from
 * vertical. it is scaled by the squared sine of the angle between that lean
 * and the camera's line of sight, each taken level. its sign says which
 * side of that line the lean lies on */
static void camera_roll_with_gravity(struct camera_view *view)
{
    NJS_VECTOR look;
    NJS_VECTOR up = {0.0F, 1.0F, 0.0F};
    float x;
    float z;
    int angle;
    int wide;
    float dot;
    float cross;
    float scale;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_SKY_DECK, 2)) {
        camera_state->ang_z = 0;
        return;
    }
    look.x = camera_state->pos.x - camera_state->target.x;
    look.y = camera_state->pos.y - camera_state->target.y;
    look.z = camera_state->pos.z - camera_state->target.z;
    njUnitVector(&up);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    TURN(njRotateZ, world_gravity_ang_z);
    TURN(njRotateX, world_gravity_ang_x);
    njCalcVector(NULL, &up, &up);
    njPopMatrix(1);
    /* @hack implicit: x and z set inside their own squares, unsequenced and
     * undefined in C89 (3.3), which SHC evaluates as the image does: x stored
     * at sp+4 (0x8c01e9c6) and read back (0x8c01e9d4), z stored at sp+0
     * (0x8c01e9d0) and squared from registers; set before the sum, x and z
     * take registers, the function runs 32 bytes short and 171 of 177 words
     * differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unsequenced) */
    angle = (Angle)(asinf(njSqrt(((x = up.x) * x) + ((z = up.z) * z))) *
                    ANGLE_PER_RADIAN);
    up.y = look.y = 0.0F;
    njUnitVector(&look);
    njUnitVector(&up);
    cross = (look.z * up.x) - (look.x * up.z);
    dot = (up.x * look.x) + (up.z * look.z);
    scale = 1.0F - (dot * dot);
    /* @hack redundant: wide set from dot and then overwritten: SHC deletes
     * the store, but its test of dot keeps look.z in FR7 and cross in FR6
     * (0x8c01ea08, 0x8c01ea28); without it, 6 of 177 words differ */
    wide = dot ? camera_state->ang_x : 0;
    wide = (angle & 0x8000) ? (angle | 0xffff0000) : (angle & 0xffff);
    if (cross >= 0.0F) {
        camera_state->ang_z = (Angle)((float)wide * scale);
    } else {
        camera_state->ang_z = (Angle) - ((float)wide * scale);
    }
    camera_state->ang_z &= 0xffff;
}

/* LR's step. the camera circles player 0 at the distance across and the
 * height it starts at, and looks at his eyes. L and R turn it, each its own
 * way, by how far each is pulled. a move whose sweep hits leaves the camera
 * where it was */
static void camera_step_trigger_orbit(struct camera_view *view)
{
    NJS_POINT3 old;

    old.x = camera_state->pos.x;
    old.y = camera_state->pos.y;
    old.z = camera_state->pos.z;
    if (!view->steps) {
        NJS_VECTOR to_player;

        to_player.x = player_work[0]->pos.x - camera_state->pos.x;
        to_player.z = player_work[0]->pos.z - camera_state->pos.z;
        trigger_orbit_distance =
            njSqrt((to_player.x * to_player.x) + (to_player.z * to_player.z));
        trigger_orbit_height = camera_state->pos.y - player_work[0]->pos.y;
        trigger_orbit_angle =
            (Angle)(atan2f(to_player.x, to_player.z) * ANGLE_PER_RADIAN) +
            0x8000;
    }
    trigger_orbit_angle +=
        (Angle)(((pad_records[0].l << 8) - (pad_records[0].r << 8)) *
                ORBIT_TURN);
    camera_state->pos.x = player_work[0]->pos.x +
                          (trigger_orbit_distance * njSin(trigger_orbit_angle));
    camera_state->pos.y = player_work[0]->pos.y + trigger_orbit_height;
    camera_state->pos.z = player_work[0]->pos.z +
                          (trigger_orbit_distance * njCos(trigger_orbit_angle));
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y =
        player_work[0]->pos.y + player_get(0)->consts.eye_height;
    camera_state->target.z = player_work[0]->pos.z;
    {
        NJS_VECTOR move;

        move.x = camera_state->pos.x - old.x;
        move.y = camera_state->pos.y - old.y;
        move.z = camera_state->pos.z - old.z;
        if (camera_sweep_hits(&old, &move, NULL, 8.0F)) {
            camera_state->pos.x = old.x;
            camera_state->pos.y = old.y;
            camera_state->pos.z = old.z;
        }
    }
    camera_place_look_at(camera_work);
}

/* run POINT's trigger orbit, which turns the camera about its target as
 * LR's orbit does. a trigger pulled at adjust mode NONE starts the orbit.
 * the stick, the d-pad, or camera_player_still clear stops it. stopping a
 * running orbit gives the camera adjust mode THREE2. it returns 1 while the
 * orbit runs, and 0 otherwise */
static int camera_trigger_orbit(void)
{
    NJS_VECTOR to_target;
    Float distance;
    Float height;
    Angle angle;

    if ((pad_records[0].x1 << 8) != 0 || (pad_records[0].y1 << 8) != 0 ||
        (per_info[0]->on & PAD_DIRECTIONS) || !camera_player_still) {
        if (trigger_orbit_active == 1) {
            camera_set_adjust(CAMERA_ADJUST_THREE2);
        }
        trigger_orbit_active = 0;
    } else if (TRIGGER_PULLED(pad_records[0].l) ||
               TRIGGER_PULLED(pad_records[0].r)) {
        if (!camera_get_adjust()) {
            trigger_orbit_active = 1;
        }
    }
    /* @hack spelling: trigger_orbit_active, 0 here, returned by name: its load
     * into R4 puts MOV R4,R0 in the BRA's slot (0x8c01ed88, 0x8c01ed90); as
     * return 0, 7 of 176 words differ */
    if (!trigger_orbit_active) {
        return trigger_orbit_active;
    }
    to_target.x = camera_state->target.x - camera_state->pos.x;
    to_target.z = camera_state->target.z - camera_state->pos.z;
    distance =
        njSqrt((to_target.x * to_target.x) + (to_target.z * to_target.z));
    height = camera_state->pos.y - camera_state->target.y;
    angle =
        (Angle)(atan2f(to_target.x, to_target.z) * ANGLE_PER_RADIAN) + 0x8000;
    angle += (Angle)(((pad_records[0].l << 8) - (pad_records[0].r << 8)) *
                     ORBIT_TURN);
    camera_state->pos.x = camera_state->target.x + (njSin(angle) * distance);
    camera_state->pos.y = camera_state->target.y + height;
    camera_state->pos.z = camera_state->target.z + (njCos(angle) * distance);
    return trigger_orbit_active;
}

/* sweep a mode's move of the camera from last to pos as camera_sweep_move
 * does. while camera_adjust_is_c, the adjust mode's step sweeps the move
 * instead, and last takes pos */
static void camera_sweep_unless_c(NJS_POINT3 *last, NJS_POINT3 *pos)
{
    NJS_VECTOR move;
    /* @hack type: the ways from last and from pos to player 0 as one array,
     * whose address SHC keeps in R12 and its second vector's in R11
     * (0x8c01eed8, 0x8c01ef1c); as two vectors, 154 of 158 words differ and
     * the function runs 32 bytes short */
    NJS_VECTOR to_player[2];

    if (camera_adjust_is_c()) {
        *last = *pos;
        return;
    }
    move.x = pos->x - last->x;
    move.y = pos->y - last->y;
    move.z = pos->z - last->z;
    to_player[0].x = player_work[0]->pos.x - last->x;
    to_player[0].y = player_work[0]->pos.y - last->y;
    to_player[0].z = player_work[0]->pos.z - last->z;
    to_player[1].x = player_work[0]->pos.x - pos->x;
    to_player[1].y = player_work[0]->pos.y - pos->y;
    to_player[1].z = player_work[0]->pos.z - pos->z;
    if (njScalor(&to_player[0]) < njScalor(&to_player[1])) {
        if (camera_sweep_hits(last, &move, NULL, 8.0F)) {
            pos->x = last->x + move.x;
            pos->y = last->y + move.y;
            pos->z = last->z + move.z;
        }
    } else if (camera_sweep_hits(last, &move, NULL, 4.0F)) {
        pos->x = last->x + move.x;
        pos->y = last->y + move.y;
        pos->z = last->z + move.z;
    }
}

/* stop the camera's move from last to pos where a sweep hits. the sweep's
 * radius is 8 while last is nearer player 0 than pos is, and 4 otherwise */
static void camera_sweep_move(NJS_POINT3 *last, NJS_POINT3 *pos)
{
    NJS_VECTOR ray;
    /* @hack type: the ways from last and from pos to player 0 as one array,
     * whose address SHC keeps in R12 and its second vector's in R11
     * (0x8c01f05c, 0x8c01f094); as two vectors, 142 of 147 words differ and
     * the function runs 32 bytes short */
    NJS_VECTOR to_player[2];

    ray.x = pos->x - last->x;
    ray.y = pos->y - last->y;
    ray.z = pos->z - last->z;
    to_player[0].x = player_work[0]->pos.x - last->x;
    to_player[0].y = player_work[0]->pos.y - last->y;
    to_player[0].z = player_work[0]->pos.z - last->z;
    to_player[1].x = player_work[0]->pos.x - pos->x;
    to_player[1].y = player_work[0]->pos.y - pos->y;
    to_player[1].z = player_work[0]->pos.z - pos->z;
    if (njScalor(&to_player[0]) < njScalor(&to_player[1])) {
        if (camera_sweep_hits(last, &ray, NULL, 8.0F)) {
            pos->x = last->x + ray.x;
            pos->y = last->y + ray.y;
            pos->z = last->z + ray.z;
        }
    } else if (camera_sweep_hits(last, &ray, NULL, 4.0F)) {
        pos->x = last->x + ray.x;
        pos->y = last->y + ray.y;
        pos->z = last->z + ray.z;
    }
}

/* FOLLOW's step. the camera sits manual_distance from its target, at
 * MANUAL_PITCH. the target is above player 0's eyes by RISE times the
 * distance camera_state held as the step began. while Knuckles pushes, the
 * camera turns a quarter of the way a step toward behind him, 0xc000 less
 * his y angle. otherwise L and R turn it 0x200 a step, or with Y held change
 * manual_distance */
static void camera_step_manual(struct camera_view *view)
{
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y +
                             player_get(0)->consts.eye_height +
                             (camera_state->distance * RISE);
    camera_state->target.z = player_work[0]->pos.z;
    camera_state->distance = manual_distance;
    camera_state->ang_x = MANUAL_PITCH;
    camera_state->ang_z = 0;
    if (PLAYER_VARS(player_work[0])->character == CHARACTER_KNUCKLES &&
        player_work[0]->mode == KNUCKLES_PUSH) {
        camera_state->ang_y +=
            angle_difference(camera_state->ang_y,
                             0xc000 - player_work[0]->ang.y) >>
            2;
    } else if (pad_records[0].on & PDD_DGT_TY) {
        if (pad_records[0].on & PDD_DGT_TL) {
            manual_distance += 2.0F;
        } else if (pad_records[0].on & PDD_DGT_TR) {
            manual_distance -= 2.0F;
        }
    } else if (pad_records[0].on & PDD_DGT_TL) {
        camera_state->flags |= CAMERA_TURNED;
        camera_state->ang_y -= 0x200;
    } else if (pad_records[0].on & PDD_DGT_TR) {
        camera_state->flags |= CAMERA_TURNED;
        camera_state->ang_y += 0x200;
    }
}

/* ADVERTISE's step: the camera looks at the origin, level, from 50 */
static void camera_step_origin(struct camera_view *view)
{
    camera_state->target.x = 0.0F;
    camera_state->target.y = 0.0F;
    camera_state->target.z = 0.0F;
    camera_state->distance = 50.0F;
    camera_state->ang_x = 0;
    camera_state->ang_z = 0;
    camera_state->ang_y = 0;
}

void camera_orbit(struct camera_view *view)
{
    Sint16 x;
    Sint16 y;
    /* NOLINTBEGIN(clang-diagnostic-unused-but-set-variable) */
    unsigned short right;
    unsigned short left;
    /* NOLINTEND(clang-diagnostic-unused-but-set-variable) */

    camera_state->target.x = camera_take_work->pos.x;
    camera_state->target.y = camera_take_work->pos.y;
    camera_state->target.z = camera_take_work->pos.z;
    camera_state->ang_z = 0;
    if (per_info[0]->on & PDD_DGT_ST) {
        x = per_info[0]->x1 << 8;
        y = per_info[0]->y1 << 8;
        if (x > -5000 && x < 5000) {
            x = 0;
        }
        if (y > -5000 && y < 5000) {
            y = 0;
        }
        /* @hack redundant: the triggers, set and unread: SHC loads
         * per_info[0] once for both, into R7, and reads r (0x8c01f3b0,
         * 0x8c01f3ba); without both lines, 95 of 117 words differ, and with
         * right's alone, 14 */
        right = ((Sint16)per_info[0]->r << 8) + 0x8000;
        left = ((Sint16)per_info[0]->l << 8) + 0x8000;
        camera_state->ang_y += x / 64;
        camera_state->ang_x += y / 64;
        if (per_info[0]->on & PDD_DGT_TL) {
            camera_state->distance += 2.0F;
        } else if (per_info[0]->on & PDD_DGT_TR) {
            camera_state->distance -= 2.0F;
        }
    }
    if (camera_state->distance < 5.0F) {
        camera_state->distance = 5.0F;
    }
    camera_place_from_target(camera_work);
}

/* the editor's step for the ASHLAND, FIXED, KLAMATH, LINE and POINT modes:
 * the camera at the view's pos, looking at the view's target */
static void camera_preview_view(struct camera_view *view)
{
    camera_state->target.x = view->target.x;
    camera_state->target.y = view->target.y;
    camera_state->target.z = view->target.z;
    camera_state->pos.x = view->pos.x;
    camera_state->pos.y = view->pos.y;
    camera_state->pos.z = view->pos.z;
    camera_state->ang_z = 0;
    camera_place_look_at(camera_work);
}

/* GURIGURI's step, a debug camera that looks at player 0. pad 0's stick
 * turns it, and C and Z move it out and in by 2. the difference of the
 * triggers moves it too. pad 1's B puts it back 50 out and 0x800 down. it
 * keeps at least 5 from player 0 */
static void camera_step_guriguri(struct camera_view *view)
{
    short x;
    short y;
    unsigned short right;
    unsigned short left;

    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y;
    camera_state->target.z = player_work[0]->pos.z;
    camera_state->ang_z = 0;
    if (pad_records[1].press & PDD_DGT_TB) {
        camera_state->distance = 50.0F;
        camera_state->ang_x = -0x800;
    }
    x = pad_records[0].x1 << 8;
    y = pad_records[0].y1 << 8;
    if (x > -5000 && x < 5000) {
        x = 0;
    }
    if (y > -5000 && y < 5000) {
        y = 0;
    }
    right = ((Sint16)pad_records[0].r << 8) + 0x8000;
    left = ((Sint16)pad_records[0].l << 8) + 0x8000;
    camera_state->distance += right * 0.0001F;
    camera_state->distance -= left * 0.0001F;
    camera_state->ang_y += x / 64;
    camera_state->ang_x += y / 64;
    if (pad_records[0].on & PDD_DGT_TC) {
        camera_state->distance += 2.0F;
    } else if (pad_records[0].on & PDD_DGT_TZ) {
        camera_state->distance -= 2.0F;
    }
    if (camera_state->distance < 5.0F) {
        camera_state->distance = 5.0F;
    }
}

/* MAGONOTE's step. its lead is 8 steps of player 0's speed ahead of his
 * eyes. the camera's target takes the lead on the mode's first step, and
 * eases a tenth of the way to it a step after. past 40 from the lead the
 * camera closes in, to no more than 70, and under 15 it backs off. it rises
 * or falls to bring its pitch toward that of player 0's heading less 0xa00.
 * R and L move it 2 a step sideways. a sweep that hits stops its move */
static void camera_step_magonote(struct camera_view *view)
{
    NJS_POINT3 previous;
    NJS_POINT3 lead;
    NJS_POINT3 eyes;
    NJS_VECTOR offset;
    NJS_VECTOR heading;
    /* @hack spelling: lead, heading and offset through lp, hp and op, which
     * keep their addresses in R10, R13 and R11 (0x8c01f6b4, 0x8c01f6a8,
     * 0x8c01f7b8); by name, 118, 203 and 484 of 535 words differ, and with
     * offset the code runs 32 bytes long */
    NJS_POINT3 *lp;
    NJS_VECTOR *hp;
    NJS_VECTOR *op;
    Float length;
    Angle diff;
    PDS_PERIPHERAL *pad;

    previous.x = camera_work->pos.x;
    previous.y = camera_work->pos.y;
    previous.z = camera_work->pos.z;
    eyes = player_work[0]->pos;
    eyes.y += player_get(0)->consts.eye_height;
    hp = &heading;
    *hp = player_motion[0]->speed;
    lp = &lead;
    lp->x = eyes.x + (hp->x * 8.0F);
    lp->y = eyes.y + (hp->y * 8.0F);
    lp->z = eyes.z + (hp->z * 8.0F);
    /* @hack redundant: two arms that never run, ahead of the test of the
     * view's steps: their jumps keep the view's address loading into R1 and
     * its steps into R2 (0x8c01f6ea, 0x8c01f6ee), and the label keeps the
     * test at a block's start, so camera_state's load stays ahead of it and
     * the BF at 0x8c01f6f4 has no delay slot. without the label 3 of 535
     * words differ, without the second arm 3, and without both 5 */
    if (0) {
    /* NOLINTNEXTLINE(clang-diagnostic-unused-label) */
    split:;
    }
    /* @hack redundant: the second arm that never runs, empty, whose jump, with
     * the first arm's, keeps the view's address loading into R1 and its steps
     * into R2 (0x8c01f6ea, 0x8c01f6ee); without it, the two trade and 3 of 535
     * words differ */
    else if (0) {
    } else if (view->steps == 0) {
        camera_state->target.x = lp->x;
        camera_state->target.y = lp->y;
        camera_state->target.z = lp->z;
    } else {
        camera_state->target.x += (lp->x - camera_state->target.x) * 0.1F;
        camera_state->target.y += (lp->y - camera_state->target.y) * 0.1F;
        camera_state->target.z += (lp->z - camera_state->target.z) * 0.1F;
    }
    /* @hack redundant: three stores that never run, which give camera_work's
     * address R12 and the offset's pointer R11 (0x8c01f662, 0x8c01f7b2);
     * without them, the two trade and 52 of 535 words differ */
    if (0) {
        camera_work->ang.x = 1;
        camera_work->ang.x = 1;
        camera_work->ang.x = 1;
    }
    /* @hack redundant: an empty test that never runs, after which
     * camera_state's next load takes R3 (0x8c01f7ac); without it, 2 of 535
     * words differ */
    if (0) {
    }
    op = &offset;
    op->x = lp->x - camera_work->pos.x;
    op->y = lp->y - camera_work->pos.y;
    op->z = lp->z - camera_work->pos.z;
    length = njScalor(op);
    /* @hack redundant: a read of lead that keeps its pointer across njScalor,
     * in R10 (0x8c01f6b4); without it, the pointer takes R4 and 31 of 535
     * words differ */
    (void)lp->x;
    {
        NJS_VECTOR dir;

        dir = *op;
        njUnitVector(&dir);
        if (length > 40.0F) {
            length = ((length - 40.0F) * 0.9F) + 40.0F;
            if (length > 70.0F) {
                length = 70.0F;
            }
        } else if (length < 15.0F) {
            length *= 1.06F;
            if (length <= 0.0F) {
                length = 15.0F;
            }
        }
        camera_state->pos.x += op->x - (dir.x * length);
        camera_state->pos.y += op->y - (dir.y * length);
        camera_state->pos.z += op->z - (dir.z * length);
    }
    {
        NJS_MATRIX matrix;
        NJS_VECTOR unit;
        Angle angle;

        unit.x = 1.0F;
        unit.y = unit.z = 0.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX_VIA(njRotateZ, &matrix, player_work[0]->ang.z, angle);
        TURN_MATRIX_VIA(njRotateX, &matrix, player_work[0]->ang.x, angle);
        TURN_MATRIX_VIA(njRotateY, &matrix, -player_work[0]->ang.y, angle);
        njCalcVector(&matrix, &unit, hp);
    }
    hp->z = njSqrt((hp->x * hp->x) + (hp->z * hp->z));
    hp->x = 0.0F;
    njUnitVector(hp);
    op->z = njSqrt((op->x * op->x) + (op->z * op->z));
    op->x = 0.0F;
    njUnitVector(op);
    diff = angle_difference(camera_work->ang.x,
                            (Angle)(atan2f(hp->y, hp->z) * ANGLE_PER_RADIAN) -
                                0xa00);
    if (diff < -0x1000) {
        camera_state->pos.y += 2.0F;
    } else if (diff < -0x800) {
        camera_state->pos.y += 0.5F;
    } else if (diff < -0x400) {
        camera_state->pos.y += 0.1F;
    } else if (diff > 0x1000) {
        camera_state->pos.y -= 2.0F;
    } else if (diff > 0x800) {
        camera_state->pos.y -= 0.5F;
    } else if (diff > 0x400) {
        camera_state->pos.y -= 0.1F;
    }
    pad = pad_records;
    /* @hack redundant: two tests of pad that never run, which keep 8.0 in
     * FR13 and 0.1 in FR14 (0x8c01f6c6, 0x8c01f6ec) and the squares in their
     * frame slots; without them, 12 of 535 words differ */
    if (0) {
        if (pad) {
        }
    }
    /* @hack redundant: the same test of pad again, which with the one above
     * keeps 8.0 in FR13 and 0.1 in FR14 (0x8c01f6c6, 0x8c01f6ec); without it,
     * the two trade and 12 of 535 words differ */
    if (0) {
        if (pad) {
        }
    }
    if (pad->on & PDD_DGT_TR) {
        camera_state->pos.x += njCos(camera_work->ang.y) * 2.0F;
        camera_state->pos.z -= njSin(camera_work->ang.y) * 2.0F;
    } else if (pad->on & PDD_DGT_TL) {
        camera_state->pos.x -= njCos(camera_work->ang.y) * 2.0F;
        camera_state->pos.z += njSin(camera_work->ang.y) * 2.0F;
    }
    {
        NJS_VECTOR dir;

        dir.x = camera_state->pos.x - previous.x;
        dir.y = camera_state->pos.y - previous.y;
        dir.z = camera_state->pos.z - previous.z;
        if (world_sweep_all_hit(&previous, &dir, NULL, 8.0F)) {
            camera_state->pos.x = previous.x + dir.x;
            camera_state->pos.y = previous.y + dir.y;
            camera_state->pos.z = previous.z + dir.z;
        }
    }
}

/* TORNADE's step. the camera circles the view's target at the view's
 * distance, and the view's pos.y above player 0. it looks at the target. L
 * and R turn it 0xff a step. otherwise it turns 0xff a step toward the
 * heading it has from player 0. with neither player 0's work nor his
 * motion, the step calls camera_restore */
static void camera_step_tornade(struct camera_view *view)
{
    struct task_work *player;
    Angle angle;

    /* @hack spelling: player set in the then-arm, and the restore in the else:
     * SHC ends the then-arm with a BRA over the else and puts
     * camera_step_magonote's last pool after it (0x8c01fbc6, 0x8c01fbca); as
     * an early return, the pool follows the restore's JMP, 114 of 128 words
     * differ and the code runs 64 bytes long */
    if (player_work[0] || player_motion[0]) {
        player = player_work[0];
    } else {
        camera_restore();
        return;
    }
    angle = (Angle)(atan2f(camera_state->pos.x - player->pos.x,
                           camera_state->pos.z - player->pos.z) *
                    ANGLE_PER_RADIAN);
    if (pad_records[0].on & PDD_DGT_TL) {
        camera_state->ang_y -= 0xff;
    } else if (pad_records[0].on & PDD_DGT_TR) {
        camera_state->ang_y += 0xff;
    } else {
        camera_state->ang_y = angle_approach(camera_state->ang_y, angle, 0xff);
    }
    camera_state->pos.x =
        (njSin(camera_state->ang_y) * view->distance) + camera_state->target.x;
    camera_state->pos.z =
        (njCos(camera_state->ang_y) * view->distance) + camera_state->target.z;
    camera_state->pos.y = player->pos.y + view->pos.y;
    camera_state->target.x = view->target.x;
    camera_state->target.y = view->target.y;
    camera_state->target.z = view->target.z;
}

/* PATHCAM's step, which plays the paths of pathcam_routes' route for the
 * stage and act. each step moves the place along the path by the speed.
 * the camera takes the place between the keys either side. A picks the
 * next path. B, or the place reaching the path's length, starts the path
 * again. up and down change the speed by 0.05, within 0.05 to 1. with no
 * route for the stage and act, the step calls camera_restore */
static void camera_step_pathcam(struct camera_view *view)
{
    unsigned short stage_act = stage_get_stage_act();
    struct pathcam_route *route;
    struct pathcam_path **paths;
    unsigned short count;
    struct pathcam_path *path;
    Float length;
    struct pathcam_key *key;
    struct pathcam_key *next;
    Float part;
    unsigned short index;

    for (route = pathcam_routes; route->stage_act != stage_act; route++) {
        if (route->stage_act == PATHCAM_END) {
            camera_restore();
            return;
        }
    }
    PATHCAM_VARS(camera_work)->route = route;
    count = route->count;
    paths = route->paths;
    if (per_info[0]->press & PDD_DGT_TA) {
        PATHCAM_VARS(camera_work)->place = 0.0F;
        if (++PATHCAM_VARS(camera_work)->path >= count) {
            PATHCAM_VARS(camera_work)->path = 0;
        }
    }
    path = paths[PATHCAM_VARS(camera_work)->path];
    length = path->length;
    key = path->keys;
    if (per_info[0]->press & PDD_DGT_KU) {
        camera_work->scl.x += 0.05F;
    }
    if (per_info[0]->press & PDD_DGT_KD) {
        camera_work->scl.x -= 0.05F;
    }
    if (camera_work->scl.x < 0.05F) {
        camera_work->scl.x = 0.05F;
    }
    if (camera_work->scl.x > 1.0F) {
        camera_work->scl.x = 1.0F;
    }
    PATHCAM_VARS(camera_work)->place += camera_work->scl.x;
    if (PATHCAM_VARS(camera_work)->place < 0.0F ||
        PATHCAM_VARS(camera_work)->place >= length) {
        PATHCAM_VARS(camera_work)->place = 0.0F;
    }
    if (per_info[0]->press & PDD_DGT_TB) {
        PATHCAM_VARS(camera_work)->place = 0.0F;
    }
    index = (int)PATHCAM_VARS(camera_work)->place;
    part = PATHCAM_VARS(camera_work)->place - index;
    key += index;
    /* @hack spelling: next set to key and stepped on its own, which keeps the
     * stage search's stage in R6 and PATHCAM_END in R5 (0x8c01fd18,
     * 0x8c01fd1e); as key + 1, the two trade and 20 of 232 words differ */
    next = key;
    next++;
    camera_state->pos.x = key->pos.x + ((next->pos.x - key->pos.x) * part);
    camera_state->pos.y = key->pos.y + ((next->pos.y - key->pos.y) * part);
    camera_state->pos.z = key->pos.z + ((next->pos.z - key->pos.z) * part);
    camera_state->ang_x =
        (Angle)(angle_difference(key->ang_x, next->ang_x) * part) + key->ang_x;
    camera_state->ang_y =
        (Angle)(angle_difference(key->ang_y, next->ang_y) * part) + key->ang_y;
    camera_state->ang_z =
        (Angle)(angle_difference(key->ang_z, next->ang_z) * part) + key->ang_z;
    camera_place_at_angles(camera_work);
}

/* BUILDING's step. down to y -10200 the camera sits at x 35, 40 above
 * player 0, and looks 90 below him at x 0. down to -18380 it sits at x 20,
 * 30 above him, and looks 100 below. there it also rolls toward half a turn
 * past his y angle. it rolls 0x800 a step while its roll is more than 0xc00
 * from a half turn, and 64 after. lower down, it only levels */
static void camera_step_building(struct camera_view *view)
{
    player_get(0);
    if (player_work[0]->pos.y > -10200.0F) {
        camera_state->target.x = 0.0F;
        camera_state->target.y = player_work[0]->pos.y - 90.0F;
        camera_state->target.z = player_work[0]->pos.z;
        camera_state->pos.x = 35.0F;
        camera_state->pos.y = player_work[0]->pos.y + 40.0F;
        camera_state->pos.z = player_work[0]->pos.z;
        camera_state->ang_z = 0;
    } else if (player_work[0]->pos.y > -18380.0F) {
        camera_state->target.x = 0.0F;
        camera_state->target.y = player_work[0]->pos.y - 100.0F;
        camera_state->target.z = player_work[0]->pos.z;
        camera_state->pos.x = 20.0F;
        camera_state->pos.y = player_work[0]->pos.y + 30.0F;
        camera_state->pos.z = player_work[0]->pos.z;
        if (angle_distance(camera_state->ang_z, 0x8000) > 0xc00) {
            camera_state->ang_z = angle_approach(
                camera_state->ang_z, player_work[0]->ang.y + 0x8000, 0x800);
        } else {
            camera_state->ang_z = angle_approach(
                camera_state->ang_z, player_work[0]->ang.y + 0x8000, 64);
        }
    } else {
        camera_state->ang_z = 0;
    }
}

static void board_fixed_view(struct camera_view *view);

/* @hack spelling: camera_step_board's ease as a helper SHC expands, whose
 * parameter, a copy of lead's pointer, keeps the pointer in R12
 * (0x8c020144); written in place, it takes R4 and 36 of 457 words differ */
#pragma inline(board_target_ease)
static void board_target_ease(NJS_POINT3 *lead)
{
    camera_state->target.x += (lead->x - camera_state->target.x) * 0.16F;
    camera_state->target.y += (lead->y - camera_state->target.y) * 0.16F;
    camera_state->target.z += (lead->z - camera_state->target.z) * 0.16F;
}

/* the step for Ice Cap's board run. its target is 16 steps of player 0's speed
 * ahead of him. the target goes there at once on the first step, and 0.16 of
 * the way a step after. the camera keeps 18 to 22 from player 0. below speed 3
 * it also falls back and rises, by up to 40 and 3 at a standstill. a sweep from
 * 4 out along the ground's normal stops its move. on Ice Cap's act 2, above x
 * -5400 and y -3900, board_fixed_view places the camera instead. camera_work's
 * submode is 1 there, and 0 elsewhere */
void camera_step_board(struct camera_view *view)
{
    NJS_POINT3 previous;
    NJS_POINT3 lead;
    NJS_POINT3 pos;
    NJS_VECTOR offset;
    NJS_VECTOR speed;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[3]. the image's ADD #-120,R15 reserves them
     * (0x8c0200b0); without them, 7 of 457 words differ */
    Uint32 unused[3];
    NJS_VECTOR dir;
    NJS_VECTOR delta;
    NJS_VECTOR normal;
    /* @hack spelling: speed, lead and offset through sp, lp and op, which keep
     * their addresses in R14, R12 and R12 (0x8c020100, 0x8c020148,
     * 0x8c02023c); by name, 427, 375 and 281 of 457 words differ, and with
     * speed the code runs 32 bytes long */
    NJS_VECTOR *sp;
    NJS_POINT3 *lp;
    NJS_VECTOR *op;
    Float length;
    Float distance;
    Float slowness;
    Angle angle;

    if (stage_get_stage_act() == STAGE_ACT(STAGE_ICECAP, 2) &&
        player_work[0]->pos.x > -5400.0F && player_work[0]->pos.y > -3900.0F) {
        camera_work->submode = 1;
        board_fixed_view(view);
    } else {
        camera_work->submode = 0;
        /* @hack redundant: a store across a call that never runs, which
         * makes the prologue save R8 (0x8c0200aa), which no instruction
         * uses; without it, 138 of 457 words differ */
        if (0) {
            camera_work->ang.x += matrix_push_unit();
        }
        previous = camera_work->pos;
        pos = player_work[0]->pos;
        sp = &speed;
        *sp = player_get(0)->speed;
        player_turn_to_world(player_work[0], sp);
        lp = &lead;
        lp->x = pos.x + (sp->x * 16.0F);
        lp->y = pos.y + (sp->y * 16.0F);
        lp->z = pos.z + (sp->z * 16.0F);
        if (view->steps == 0) {
            camera_state->target.x = lp->x;
            camera_state->target.y = lp->y;
            camera_state->target.z = lp->z;
        } else {
            board_target_ease(lp);
        }
        op = &offset;
        op->x = camera_work->pos.x - pos.x;
        op->y = camera_work->pos.y - pos.y;
        op->z = camera_work->pos.z - pos.z;
        length = njScalor(op);
        distance = length;
        dir = *op;
        njUnitVector(&dir);
        if (length > 22.0F) {
            length = 22.0F;
        } else if (length < 18.0F) {
            length = 18.0F;
        }
        camera_state->pos.x += dir.x * (length - distance);
        camera_state->pos.y += dir.y * (length - distance);
        camera_state->pos.z += dir.z * (length - distance);
        slowness = (-njScalor(&player_motion[0]->speed) + 3.0F) * 0.33333333F;
        if (slowness < 0.0F) {
            slowness = 0.0F;
        }
        dir.x = -(slowness * 40.0F);
        dir.y = slowness * 3.0F;
        dir.z = 0.0F;
        player_turn_to_world(player_work[0], &dir);
        camera_state->pos.x += dir.x;
        camera_state->pos.y += dir.y;
        camera_state->pos.z += dir.z;
        dir = previous;
        normal = player_get(0)->ground_normal;
        normal.x *= 4.0F;
        normal.y *= 4.0F;
        normal.z *= 4.0F;
        njAddVector(&dir, &normal);
        delta.x = camera_state->pos.x - previous.x;
        delta.y = camera_state->pos.y - previous.y;
        delta.z = camera_state->pos.z - previous.z;
        if (world_sweep_all_hit(&dir, &delta, NULL, 14.0F)) {
            camera_state->pos.x = dir.x + delta.x - normal.x;
            camera_state->pos.y = dir.y + delta.y - normal.y;
            camera_state->pos.z = dir.z + delta.z - normal.z;
        }
        /* turn normal into the camera's frame. nothing reads the result
         * (0x8c020430 to 0x8c02046c) */
        matrix_push_unit();
        TURN_VIA(njRotateX, -camera_work->ang.x, angle);
        TURN_VIA(njRotateY, -camera_work->ang.y, angle);
        njCalcVector(NULL, &normal, &normal);
        njPopMatrix(1);
    }
}

/* place the camera for camera_step_board above x -5400 and y -3900 on Ice
 * Cap's act 2. it sits 20 plus 6 times player 0's speed from him, in a
 * fixed direction. it looks at the point as far on his other side. its move
 * is swept as camera_step_board's is */
static void board_fixed_view(struct camera_view *view)
{
    NJS_POINT3 previous;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as a Uint32[6]. the image's ADD #-84,R15 reserves them
     * (0x8c02050e); without them, 3 of 228 words differ */
    Uint32 unused[6];
    NJS_VECTOR dir;
    NJS_POINT3 origin;
    NJS_VECTOR delta;
    NJS_VECTOR normal;
    Float distance;

    previous = camera_work->pos;
    distance = (njScalor(&player_motion[0]->speed) * 6.0F) + 20.0F;
    dir.x = njCos(-0xccc) * njSin(-0x471c);
    dir.y = njSin(-0xccc);
    dir.z = njCos(-0xccc) * njCos(-0x471c);
    camera_state->pos.x = player_work[0]->pos.x + (dir.x * distance);
    camera_state->pos.y = player_work[0]->pos.y + (dir.y * distance);
    camera_state->pos.z = player_work[0]->pos.z + (dir.z * distance);
    camera_state->target.x = player_work[0]->pos.x - (dir.x * distance);
    camera_state->target.y = player_work[0]->pos.y - (dir.y * distance);
    camera_state->target.z = player_work[0]->pos.z - (dir.z * distance);
    origin = previous;
    normal = player_get(0)->ground_normal;
    normal.x *= 4.0F;
    normal.y *= 4.0F;
    normal.z *= 4.0F;
    njAddVector(&origin, &normal);
    delta.x = camera_state->pos.x - previous.x;
    delta.y = camera_state->pos.y - previous.y;
    delta.z = camera_state->pos.z - previous.z;
    if (world_sweep_all_hit(&origin, &delta, NULL, 14.0F)) {
        camera_state->pos.x = origin.x + delta.x - normal.x;
        camera_state->pos.y = origin.y + delta.y - normal.y;
        camera_state->pos.z = origin.z + delta.z - normal.z;
    }
}

/* the board tricks' steps. each step, the camera takes the trick path's
 * next point, turned the way player 0 faces. it sits there from where he
 * will be a step on. it looks at his first collision shape's center, a
 * step on. once the path ends or he lands, the step hands the camera back
 * to camera_step_board */
void camera_step_trick_a(struct camera_view *view)
{
    struct path_sample sample;
    NJS_VECTOR offset;
    NJS_VECTOR turned;
    Angle angle;
    Float distance;
    /* @hack spelling: the path and player 0's work in locals, which keep the
     * path's address in R14 (0x8c020728) and store the work to the frame
     * before the calls (0x8c020730); passed and read directly, 178 and 180
     * of 184 words differ */
    /* @hack spelling: path declared ahead of player, which puts path's pool
     * word at 0x8c020900 and player_work's at 0x8c020904, in the pool the
     * tricks share; the other way round, 3 of 184 words differ here, and 1 word
     * and 4 pool words in camera_step_trick_b */
    struct path *path = &trick_a_path;
    struct task_work *player = player_work[0];
    int valid;

    valid = path_get_distance(path, view->steps, &distance);
    view->steps++;
    sample.distance = distance;
    if (!valid || (player->flags & PLAYER_ON_LAND) ||
        !path_sample(path, &sample)) {
        camera_restore();
        camera_override(camera_step_board, CAMERA_ADJUST_SLOW, CAMERA_LOOK_AT);
        return;
    }
    offset.x = sample.pos.x;
    offset.y = sample.pos.y;
    offset.z = sample.pos.z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, -player_work[0]->ang.y + 0x8000, angle);
    njCalcVector(NULL, &offset, &turned);
    sample.pos.x = turned.x;
    sample.pos.y = turned.y;
    sample.pos.z = turned.z;
    njPopMatrix(1);
    camera_state->pos.x =
        player_work[0]->pos.x + player_motion[0]->speed.x + sample.pos.x;
    camera_state->pos.y =
        player_work[0]->pos.y + player_motion[0]->speed.y + sample.pos.y;
    camera_state->pos.z =
        player_work[0]->pos.z + player_motion[0]->speed.z + sample.pos.z;
    camera_state->target.x =
        player_work[0]->collision->shapes->center.x + player_motion[0]->speed.x;
    camera_state->target.y =
        player_work[0]->collision->shapes->center.y + player_motion[0]->speed.y;
    camera_state->target.z =
        player_work[0]->collision->shapes->center.z + player_motion[0]->speed.z;
}

void camera_step_trick_b(struct camera_view *view)
{
    struct path_sample sample;
    NJS_VECTOR offset;
    NJS_VECTOR turned;
    Angle angle;
    Float distance;
    /* @hack spelling: the path and player 0's work in locals, which keep the
     * path's address in R14 (0x8c020898) and store the work to the frame
     * before the calls (0x8c0208a0); passed and read directly, 43 and 64 of
     * 184 words differ */
    struct path *path = &trick_b_path;
    struct task_work *player = player_work[0];
    int valid;

    valid = path_get_distance(path, view->steps, &distance);
    view->steps++;
    sample.distance = distance;
    if (!valid || (player->flags & PLAYER_ON_LAND) ||
        !path_sample(path, &sample)) {
        camera_restore();
        camera_override(camera_step_board, CAMERA_ADJUST_SLOW, CAMERA_LOOK_AT);
        return;
    }
    offset.x = sample.pos.x;
    offset.y = sample.pos.y;
    offset.z = sample.pos.z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, -player_work[0]->ang.y + 0x8000, angle);
    njCalcVector(NULL, &offset, &turned);
    sample.pos.x = turned.x;
    sample.pos.y = turned.y;
    sample.pos.z = turned.z;
    njPopMatrix(1);
    camera_state->pos.x =
        player_work[0]->pos.x + player_motion[0]->speed.x + sample.pos.x;
    camera_state->pos.y =
        player_work[0]->pos.y + player_motion[0]->speed.y + sample.pos.y;
    camera_state->pos.z =
        player_work[0]->pos.z + player_motion[0]->speed.z + sample.pos.z;
    camera_state->target.x =
        player_work[0]->collision->shapes->center.x + player_motion[0]->speed.x;
    camera_state->target.y =
        player_work[0]->collision->shapes->center.y + player_motion[0]->speed.y;
    camera_state->target.z =
        player_work[0]->collision->shapes->center.z + player_motion[0]->speed.z;
}

/* POINT's step. the camera looks at 7.5 above player 0 from the view's
 * distance beyond him, on the line from the view's target through him.
 * camera_trigger_orbit can turn it about its target instead. a trigger
 * pulled while pad_is_enabled hands the camera to camera_step_follow from
 * the next step. once both triggers are let go while camera_player_still is
 * clear, POINT takes it back, eased by adjust mode RELATIVE6C. the move is
 * swept only while the view's ang_x is 0 */
static void camera_step_point(struct camera_view *view)
{
    NJS_POINT3 camera;
    NJS_VECTOR direction;
    NJS_POINT3 target;

    if (!view->steps) {
        camera_point_follows = 0;
    }
    if (camera_point_follows) {
        camera_step_follow(view);
        if (!TRIGGER_PULLED(pad_records[0].l) &&
            !TRIGGER_PULLED(pad_records[0].r) && !camera_player_still) {
            camera_point_follows = 0;
            camera_set_adjust(CAMERA_ADJUST_RELATIVE6C);
        }
        return;
    }
    if (TRIGGER_PULLED(pad_records[0].l) || TRIGGER_PULLED(pad_records[0].r)) {
        if (pad_is_enabled()) {
            camera_point_follows = 1;
        }
    }
    camera = camera_work->pos;
    if (!camera_trigger_orbit()) {
        target.x = view->target.x;
        target.y = view->target.y;
        target.z = view->target.z;
        camera_state->target.x = player_work[0]->pos.x;
        camera_state->target.y = player_work[0]->pos.y + 7.5F;
        camera_state->target.z = player_work[0]->pos.z;
        direction.x = player_work[0]->pos.x - target.x;
        direction.y = player_work[0]->pos.y - target.y;
        direction.z = player_work[0]->pos.z - target.z;
        njUnitVector(&direction);
        camera_state->pos.x =
            camera_state->target.x + (view->distance * direction.x);
        camera_state->pos.y =
            camera_state->target.y + (view->distance * direction.y);
        camera_state->pos.z =
            camera_state->target.z + (view->distance * direction.z);
    }
    camera_push_out(&camera_state->pos);
    camera_ease(&camera, &camera_state->pos);
    if (!(Uint16)view->ang_x) {
        camera_sweep_unless_c(&camera, &camera_state->pos);
    }
    camera_roll_with_gravity(view);
}

void camera_boss_turn(int unbounded)
{
    Float dz;
    Float dx;
    Float dist;
    Angle turn_y;
    Angle turn_x;
    Angle turn_z;
    Angle step_x;
    Angle step_y;
    Angle step_z;

    /* @hack spelling: dx and dz set inside atan2f's arguments, dz's subtraction
     * first (0x8c020c98, 0x8c020ca2); set before the call, 18 of 225 words
     * differ */
    turn_y = (Angle)(atan2f(dx = camera_state->pos.x - camera_state->target.x,
                            dz = camera_state->pos.z - camera_state->target.z) *
                     ANGLE_PER_RADIAN);
    dist = njSqrt((dx * dx) + (dz * dz));
    turn_x =
        -(Angle)(atan2f(camera_state->pos.y - camera_state->target.y, dist) *
                 ANGLE_PER_RADIAN);
    turn_z = camera_state->ang_z;
    turn_x = angle_difference(camera_state->ang_x, turn_x) / 4;
    turn_y = angle_difference(camera_state->ang_y, turn_y) / 4;
    turn_z = angle_difference(camera_state->ang_z, turn_z) / 4;
    step_x = turn_x - camera_state->spin_x;
    step_y = turn_y - camera_state->spin_y;
    step_z = turn_z - camera_state->spin_z;
    if (!unbounded) {
        step_x =
            CLAMP(step_x, -camera_boss_turn_step.x, camera_boss_turn_step.x);
        step_y =
            CLAMP(step_y, -camera_boss_turn_step.y, camera_boss_turn_step.y);
        step_z =
            CLAMP(step_z, -camera_boss_turn_step.z, camera_boss_turn_step.z);
    }
    camera_state->spin_x += step_x;
    camera_state->spin_y += step_y;
    camera_state->spin_z += step_z;
    if (!unbounded) {
        camera_state->spin_x =
            CLAMP(camera_state->spin_x, -camera_boss_spin_limit.x,
                  camera_boss_spin_limit.x);
        camera_state->spin_y =
            CLAMP(camera_state->spin_y, -camera_boss_spin_limit.y,
                  camera_boss_spin_limit.y);
        camera_state->spin_z =
            CLAMP(camera_state->spin_z, -camera_boss_spin_limit.z,
                  camera_boss_spin_limit.z);
    }
    camera_state->ang_x += camera_state->spin_x;
    camera_state->ang_y += camera_state->spin_y;
    camera_state->ang_z += camera_state->spin_z;
}

void camera_boss_place_from_player(int below_only)
{
    Float dx;
    Float dz;
    Float dist;
    Angle pitch;
    Angle angle;

    njPushMatrix(&_unit_matrix_);
    dx = camera_state->target.x - player_work[0]->pos.x;
    dz = camera_state->target.z - player_work[0]->pos.z;
    if (player_work[0]->flags & PLAYER_ON_LAND) {
        dist = njSqrt((dx * dx) + (dz * dz));
        pitch = (Angle)(atan2f(camera_state->target.y - player_work[0]->pos.y,
                               dist) *
                        ANGLE_PER_RADIAN);
    } else {
        pitch = 0;
    }
    TURN_VIA(njRotateY, camera_state->ang_y, angle);
    if (below_only == 1) {
        pitch = pitch < 0 ? pitch : 0;
    }
    TURN(njRotateX, pitch);
    njCalcPoint(NULL, &camera_boss_offset, &camera_boss_place);
    njPopMatrix(1);
    njAddVector(&camera_boss_place, &player_work[0]->pos);
}

int camera_boss_move(int from_anchor)
{
    Float dx;
    Float dz;
    Angle angle;

    if (from_anchor) {
        dx = player_work[0]->pos.x - boss_place_anchor.x;
        dz = player_work[0]->pos.z - boss_place_anchor.z;
        angle = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
        camera_boss_place.x = player_work[0]->pos.x - (njSin(angle) * 30.0F);
        camera_boss_place.z = player_work[0]->pos.z - (njCos(angle) * 30.0F);
    }
    camera_state->move.x = camera_boss_place.x - camera_state->pos.x;
    camera_state->move.y = camera_boss_place.y - camera_state->pos.y;
    camera_state->move.z = camera_boss_place.z - camera_state->pos.z;
    camera_state->move.x =
        CLAMP(camera_state->move.x, -camera_boss_move_limit.x,
              camera_boss_move_limit.x);
    camera_state->move.y =
        CLAMP(camera_state->move.y, -camera_boss_move_limit.y,
              camera_boss_move_limit.y);
    camera_state->move.z =
        CLAMP(camera_state->move.z, -camera_boss_move_limit.x,
              camera_boss_move_limit.x);
    njAddVector(&camera_state->pos, &camera_state->move);
    return 0;
}

/* CHAOS's step. in phase 0 the camera starts 30 across from player 0 and 15
 * up, looking at the boss. after that, its target is
 * camera_boss_target_offset from the boss while player 0 is on land, and
 * from player 0 otherwise. the camera then moves toward camera_boss_place,
 * which camera_boss_place_from_player pitches only while the target lies below
 * player 0. it then turns toward the target */
static void camera_step_chaos(struct camera_view *view)
{
    /* @hack redundant: register on point, which keeps camera_state's
     * address in R12, boss_work's in R14 and player_work's in R11
     * (0x8c021130, 0x8c02113c, 0x8c021140); as a plain local, they take R14,
     * R12 and R13, and 29 of 163 words and 4 pool words differ */
    register NJS_POINT3 *point;
    Float dx;
    Float dz;
    Angle angle;

    switch (camera_state->phase) {
    case 0:
        dx = boss_work->pos.x - player_work[0]->pos.x;
        dz = boss_work->pos.z - player_work[0]->pos.z;
        angle = -(Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
        camera_state->pos.x = player_work[0]->pos.x + (njSin(angle) * 30.0F);
        camera_state->pos.y = player_work[0]->pos.y + 15.0F;
        camera_state->pos.z = player_work[0]->pos.z + (njCos(angle) * 30.0F);
        /* @hack type: the target through its x's address, which SHC forms
         * with ADD #24,R2 and stores through (0x8c0211c4); as a whole
         * member, the copy stores at displacements and 47 of 163 words
         * differ */
        *(NJS_POINT3 *)&camera_state->target.x = boss_work->pos;
        camera_state->ang_y =
            (Angle)(atan2f(camera_state->pos.x - boss_work->pos.x,
                           camera_state->pos.z - boss_work->pos.z) *
                    ANGLE_PER_RADIAN);
        camera_state->ang_x = camera_state->ang_z = 0;
        camera_state->phase = 1;
        break;
    case 1:
        if (player_work[0]->flags & PLAYER_ON_LAND) {
            point = &boss_work->pos;
        } else {
            point = &player_work[0]->pos;
        }
        camera_state->target.x = point->x + camera_boss_target_offset.x;
        camera_state->target.y = point->y + camera_boss_target_offset.y;
        camera_state->target.z = point->z + camera_boss_target_offset.z;
        camera_boss_place_from_player(1);
        camera_boss_move(0);
        camera_boss_turn(0);
        break;
    }
}

/* move camera_state's pos toward camera_boss_place as camera_boss_move does,
 * without its placement. with unbounded set, camera_boss_move_limit
 * doesn't bound the move. no code calls it. it isn't static, since SHC
 * would leave a static out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void fun_8c0212b6(int unbounded)
{
    camera_state->move.x = camera_boss_place.x - camera_state->pos.x;
    camera_state->move.y = camera_boss_place.y - camera_state->pos.y;
    camera_state->move.z = camera_boss_place.z - camera_state->pos.z;
    if (!unbounded) {
        camera_state->move.x =
            CLAMP(camera_state->move.x, -camera_boss_move_limit.x,
                  camera_boss_move_limit.x);
        camera_state->move.y =
            CLAMP(camera_state->move.y, -camera_boss_move_limit.y,
                  camera_boss_move_limit.y);
        camera_state->move.z =
            CLAMP(camera_state->move.z, -camera_boss_move_limit.x,
                  camera_boss_move_limit.x);
    }
    njAddVector(&camera_state->pos, &camera_state->move);
}

/* CHAOS_P's step: the camera turns toward the boss, and moves toward
 * camera_boss_place */
static void camera_step_chaos_p(struct camera_view *view)
{
    camera_state->target.x = boss_work->pos.x;
    camera_state->target.y = boss_work->pos.y;
    camera_state->target.z = boss_work->pos.z;
    camera_boss_turn(0);
    camera_boss_place_from_player(0);
    camera_boss_move(0);
}

/* CHAOS_W's step: B_CHAOS7.PRS's chaos7_camera_entry, at 0x0c9003a0 */
static void camera_step_chaos_w(struct camera_view *view)
{
    chaos7_camera_entry(view);
}

/* CHAOS_STD's step. the camera turns toward camera_boss_target_offset from
 * the boss. it moves toward camera_boss_place, which
 * camera_boss_place_from_player pitches only while that target lies below
 * player 0 */
static void camera_step_chaos_std(struct camera_view *view)
{
    camera_state->target.x = boss_work->pos.x + camera_boss_target_offset.x;
    camera_state->target.y = boss_work->pos.y + camera_boss_target_offset.y;
    camera_state->target.z = boss_work->pos.z + camera_boss_target_offset.z;
    camera_boss_turn(0);
    camera_boss_place_from_player(1);
    camera_boss_move(0);
}

void camera_set_boss_offsets(NJS_POINT3 *target, NJS_POINT3 *offset)
{
    if (!target) {
        target = &default_target_offset;
    }
    if (!offset) {
        offset = &default_offset;
    }
    camera_boss_target_offset = *target;
    camera_boss_offset = *offset;
    camera_set_boss_limits(NULL, NULL, NULL);
}

void camera_set_boss_limits(struct angles *step, struct angles *spin,
                            NJS_VECTOR *move)
{
    if (!step) {
        step = &default_turn_step;
    }
    if (!spin) {
        spin = &default_spin_limit;
    }
    if (!move) {
        move = &default_move_limit;
    }
    camera_boss_turn_step = *step;
    camera_boss_spin_limit = *spin;
    camera_boss_move_limit = *move;
}

/* set out to the point distance from camera_state's target, in the
 * direction of camera_state's y angle */
static void point_on_heading(float distance, NJS_POINT3 *out)
{
    NJS_POINT3 point;
    Angle angle;

    point.x = 0.0F;
    point.y = 0.0F;
    point.z = distance;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, camera_state->ang_y, angle);
    njCalcPoint(NULL, &point, out);
    njPopMatrix(1);
    njAddVector(out, &camera_state->target);
}

/* CHAOS_STINIT's step, which runs camera_start_swing's swing. the camera
 * closes in on the swing's target, then swings a quarter turn round it.
 * once round, it sets camera_swing_done, enables the pads, and stays where
 * it is */
static void camera_step_chaos_stinit(struct camera_view *view)
{
    switch (camera_swing_phase) {
    case SWING_CLOSING:
        camera_swing_distance += camera_swing_close_step;
        if (camera_swing_distance <= camera_swing_near) {
            camera_swing_phase = SWING_TURNING;
        }
        break;
    case SWING_TURNING:
        camera_swing_heading += camera_swing_turn;
        camera_swing_turned += camera_swing_turn;
        if (camera_swing_turned >= 0x4000) {
            camera_swing_phase = SWING_ROUND;
            camera_swing_done = 1;
            pad_enable();
        }
        break;
    case SWING_ROUND:
        return;
    }
    camera_state->ang_y = camera_swing_heading;
    camera_state->target.x = camera_swing_target.x;
    camera_state->target.y = camera_swing_target.y;
    camera_state->target.z = camera_swing_target.z;
    point_on_heading(camera_swing_distance, &camera_state->pos);
}

void camera_start_swing(struct task_work *work, Float height, Float distance,
                        int swing_steps)
{
    camera_swing_done = 0;
    camera_swing_work = work;
    camera_swing_height = height;
    camera_swing_near = distance;
    camera_swing_distance = distance * 4.0F;
    camera_swing_close_step =
        (camera_swing_near - camera_swing_distance) * (1.0F / 60.0F);
    camera_swing_target.x = camera_state->target.x = camera_swing_work->pos.x;
    camera_swing_target.y = camera_state->target.y =
        camera_swing_work->pos.y + height;
    camera_swing_target.z = camera_state->target.z = camera_swing_work->pos.z;
    camera_swing_turn = (Angle)(90.0F / swing_steps * ANGLE_PER_DEGREE);
    camera_state->ang_x = camera_state->ang_z = 0;
    camera_swing_heading = camera_state->ang_y = work->ang.y + 0xa000;
    camera_swing_turned = 0;
    point_on_heading(camera_swing_distance, &camera_state->pos);
    camera_swing_phase = SWING_CLOSING;
    pad_disable();
}

/* LINE's step. the camera sits 40 across from player 0, beyond him from
 * the view's target, and 20 above him. it looks at the view's target,
 * lowered to player 0's height less 0.3 * 1.25 of their distance across */
static void camera_step_line(struct camera_view *view)
{
    Float dx;
    Float dz;
    Float length;
    Float drop;

    dx = player_work[0]->pos.x - view->target.x;
    dz = player_work[0]->pos.z - view->target.z;
    length = njSqrt((dx * dx) + (dz * dz));
    /* the product, not 0.375F: SHC folds it to 0.37499997, 0x3ebfffff. that
     * is the word at 0x8c021830, whose address the MOVA at 0x8c021728 takes.
     * 0.375F is 0x3ec00000 */
    drop = length * (0.3F * 1.25F);
    dx = dx / length * 40.0F;
    dz = dz / length * 40.0F;
    camera_state->pos.x = player_work[0]->pos.x + dx;
    camera_state->pos.y = player_work[0]->pos.y + 20.0F;
    camera_state->pos.z = player_work[0]->pos.z + dz;
    camera_state->target.x = view->target.x;
    camera_state->target.y = player_work[0]->pos.y - drop;
    camera_state->target.z = view->target.z;
}

/* KLAMATH's step. the triggers hand the camera to camera_step_follow as
 * POINT's do. otherwise the camera looks at player 0 from the view's
 * distance across, beyond him from the view's target. it sits
 * camera_klamath_height above him, less camera_klamath_sink. its target
 * rises above him by camera_klamath_height, scaled by his distance across
 * from the view's target over the camera's */
static void camera_step_klamath(struct camera_view *view)
{
    NJS_POINT3 camera;
    NJS_VECTOR direction;
    NJS_POINT3 target;
    Float length;

    if (!view->steps) {
        if ((Uint16)view->ang_x) {
            camera_klamath_height = (Uint16)view->ang_x * 0.1F;
        } else {
            camera_klamath_height = view->distance * 0.5F;
        }
        camera_klamath_sink = 0.0F;
        camera_klamath_follows = 0;
    }
    if (camera_klamath_follows) {
        camera_step_follow(view);
        if (!TRIGGER_PULLED(pad_records[0].l) &&
            !TRIGGER_PULLED(pad_records[0].r) && !camera_player_still) {
            camera_klamath_follows = 0;
            camera_set_adjust(CAMERA_ADJUST_RELATIVE6C);
        }
        return;
    }
    if (TRIGGER_PULLED(pad_records[0].l) || TRIGGER_PULLED(pad_records[0].r)) {
        if (pad_is_enabled()) {
            camera_klamath_follows = 1;
        }
    }
    camera = camera_work->pos;
    target.x = view->target.x;
    target.y = player_work[0]->pos.y;
    target.z = view->target.z;
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y;
    camera_state->target.z = player_work[0]->pos.z;
    direction.x = player_work[0]->pos.x - target.x;
    direction.y = 0.0F;
    direction.z = player_work[0]->pos.z - target.z;
    njUnitVector(&direction);
    camera_state->pos.x =
        camera_state->target.x + (view->distance * direction.x);
    camera_state->pos.y = camera_klamath_height + camera_state->target.y;
    camera_state->pos.z =
        camera_state->target.z + (view->distance * direction.z);
    /* @hack spelling: the two tests joined by &, which runs both to a bit
     * and tests the bits together (MOVT at 0x8c0219c8 and 0x8c0219d2, TST
     * R0,R3 at 0x8c0219d4); with &&, 91 of 279 words differ */
    if (((player_work[0]->flags & PLAYER_ON_GROUND) == 0) &
        ((Uint16)view->ang_y == 0)) {
        camera_klamath_sink -= 0.2F;
    } else {
        camera_klamath_sink *= 0.9F;
    }
    camera_state->pos.y -= camera_klamath_sink;
    direction.x = camera_state->pos.x - target.x;
    direction.y = 0.0F;
    direction.z = camera_state->pos.z - target.z;
    length = njUnitVector(&direction);
    camera_state->target.y +=
        camera_klamath_height * (length - view->distance) / length;
    camera_push_out(&camera_state->pos);
    camera_ease(&camera, &camera_state->pos);
    camera_sweep_unless_c(&camera, &camera_state->pos);
    camera_roll_with_gravity(view);
}

/* ASHLAND's step: the camera sits at the view's pos and looks at 7 above
 * player 0. in the ASHLAND modes, not the ASHLAND_I ones, the triggers hand
 * it to camera_step_follow as POINT's do */
static void camera_step_ashland(struct camera_view *view)
{
    struct task_work *player = player_work[0];

    if (!view->steps) {
        camera_ashland_follows = 0;
    }
    if (camera_get_mode() >= CAMERA_MODE_ASHLAND &&
        camera_get_mode() <= CAMERA_MODE_C_ASHLAND) {
        if (camera_ashland_follows) {
            camera_step_follow(view);
            if (!TRIGGER_PULLED(pad_records[0].l) &&
                !TRIGGER_PULLED(pad_records[0].r) && !camera_player_still) {
                camera_ashland_follows = 0;
                camera_set_adjust(CAMERA_ADJUST_RELATIVE6C);
            }
            return;
        }
        if (TRIGGER_PULLED(pad_records[0].l) ||
            TRIGGER_PULLED(pad_records[0].r)) {
            if (pad_is_enabled()) {
                camera_ashland_follows = 1;
            }
        }
    }
    camera_state->pos.x = view->pos.x;
    camera_state->pos.y = view->pos.y;
    camera_state->pos.z = view->pos.z;
    camera_state->target.x = player->pos.x;
    camera_state->target.y = player->pos.y + 7.0F;
    camera_state->target.z = player->pos.z;
    camera_roll_with_gravity(view);
}

void camera_step_event(struct camera_view *view)
{
    event_camera_step();
    camera_state->pos.x = camera_work->pos.x;
    camera_state->pos.y = camera_work->pos.y;
    camera_state->pos.z = camera_work->pos.z;
    camera_state->ang_x = camera_work->ang.x;
    camera_state->ang_y = camera_work->ang.y;
    camera_state->ang_z = camera_work->ang.z;
}

void camera_step_view_target(struct camera_view *view)
{
    float dx;
    float dy;
    float dz;
    float length;
    float target_x;
    float target_y;
    float target_z;
    float player_x;
    float player_y;
    float player_z;

    target_x = view->target.x;
    target_y = view->target.y;
    target_z = view->target.z;
    player_x = player_work[0]->pos.x;
    player_y = player_work[0]->pos.y;
    player_z = player_work[0]->pos.z;
    dx = player_x - target_x;
    dy = player_y - target_y + 7.5F;
    dz = player_z - target_z;
    length = njSqrt((dx * dx) + (dy * dy) + (dz * dz));
    dx = dx / length * 50.0F;
    dy = dy / length * 50.0F;
    dz = dz / length * 50.0F;
    camera_state->pos.x = player_x + dx;
    camera_state->pos.y = player_y + dy + 7.5F;
    camera_state->pos.z = player_z + dz;
    camera_state->target.x = target_x;
    camera_state->target.y = target_y;
    camera_state->target.z = target_z;
}

void camera_step_fixed(struct camera_view *view)
{
    camera_state->pos.x = view->pos.x;
    camera_state->pos.y = view->pos.y;
    camera_state->pos.z = view->pos.z;
    camera_state->target.x = view->target.x;
    camera_state->target.y = view->target.y;
    camera_state->target.z = view->target.z;
}

/* a step that looks at player 0's eyes. the camera closes to 8 and tilts to
 * 0xfc00, spinning 0x800 a step till it has both. it then turns toward
 * 0x3000 less player 0's y angle. no code calls it. it isn't static, since
 * SHC would leave a static out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void fun_8c021da0(void)
{
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y =
        player_work[0]->pos.y + player_get(0)->consts.eye_height;
    camera_state->target.z = player_work[0]->pos.z;
    camera_state->distance = float_approach(camera_state->distance, 8.0F, 2.0F);
    camera_state->ang_x = angle_approach(camera_state->ang_x, 0xfc00, 0x100);
    camera_state->ang_z = 0;
    if (camera_state->distance != 8.0F || camera_state->ang_x != 0xfc00) {
        camera_state->ang_y -= 0x800;
    } else {
        camera_state->ang_y = angle_approach(
            camera_state->ang_y, 0x3000 - player_work[0]->ang.y, 0x800);
    }
    camera_place_from_target(camera_work);
}

static void camera_step_along_path(struct camera_view *view);

void camera_start_path(struct path *path, int mode, Uint32 steps)
{
    camera_override(camera_step_along_path, CAMERA_ADJUST_NONE, CAMERA_LOOK_AT);
    PATH_PLAY_VARS(camera_work)->path = path;
    PATH_PLAY_VARS(camera_work)->mode = mode;
    PATH_PLAY_VARS(camera_work)->steps = steps;
    PATH_PLAY_VARS(camera_work)->point = 0;
}

/* end camera_start_path's step with camera_restore, if the step has the
 * camera. only modes.c calls it. it isn't static, since SHC would expand a
 * static at its calls and write no copy of its own */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void camera_end_path(void)
{
    if (path_playing == 1) {
        camera_restore();
        path_playing = 0;
    }
}

/* camera_start_path's step. each step, the camera takes the path's next
 * point, turned the way player 0 faces. it sits there from where he will
 * be a step on, and looks at his eyes a step on. its mode says when it
 * ends */
static void camera_step_along_path(struct camera_view *view)
{
    NJS_VECTOR offset;
    NJS_VECTOR turned;
    struct path_sample sample;
    Angle angle;
    Float distance;
    struct path *path = PATH_PLAY_VARS(camera_work)->path;
    int valid;

    if (!view->steps) {
        PATH_PLAY_VARS(camera_work)->point = 0;
        path_playing = 1;
    }
    PATH_PLAY_VARS(camera_work)->point++;
    valid =
        path_get_distance(path, PATH_PLAY_VARS(camera_work)->point, &distance);
    switch (PATH_PLAY_VARS(camera_work)->mode) {
    case CAMERA_PATH_ONCE:
        if (!valid) {
            camera_end_path();
            return;
        }
        break;
    case CAMERA_PATH_HOLD:
        if ((Uint32)view->steps >= PATH_PLAY_VARS(camera_work)->steps) {
            camera_end_path();
            return;
        }
        /* fall through */
    case CAMERA_PATH_KEEP:
        if (!valid) {
            PATH_PLAY_VARS(camera_work)->point--;
            valid = path_get_distance(path, PATH_PLAY_VARS(camera_work)->point,
                                      &distance);
        }
        break;
    case CAMERA_PATH_HOLD_2:
        if ((Uint32)view->steps >= PATH_PLAY_VARS(camera_work)->steps) {
            camera_end_path();
            return;
        }
        /* fall through */
    case CAMERA_PATH_KEEP_2:
        if (!valid) {
            PATH_PLAY_VARS(camera_work)->point--;
            valid = path_get_distance(path, PATH_PLAY_VARS(camera_work)->point,
                                      &distance);
        }
        break;
    /* @hack redundant: case 5, which shares the default's code, keeps the
     * image's CMP/EQ #5 at 0x8c021f56; without it, 57 of 297 words differ */
    case 5:
    default:
        camera_end_path();
        return;
    }
    if (valid) {
        sample.distance = distance;
        path_sample(path, &sample);
        offset.x = sample.pos.x;
        offset.y = sample.pos.y;
        offset.z = sample.pos.z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, -player_work[0]->ang.y + 0x8000, angle);
        njCalcVector(NULL, &offset, &turned);
        sample.pos.x = turned.x;
        sample.pos.y = turned.y;
        sample.pos.z = turned.z;
        njPopMatrix(1);
        camera_state->pos.x =
            player_work[0]->pos.x + player_motion[0]->speed.x + sample.pos.x;
        camera_state->pos.y =
            player_work[0]->pos.y + player_motion[0]->speed.y + sample.pos.y;
        camera_state->pos.z =
            player_work[0]->pos.z + player_motion[0]->speed.z + sample.pos.z;
        turned.x = turned.z = 0.0F;
        turned.y = player_get(0)->consts.eye_height;
        player_turn_to_world(player_work[0], &turned);
        camera_state->target.x =
            player_work[0]->pos.x + player_motion[0]->speed.x + turned.x;
        camera_state->target.y =
            player_work[0]->pos.y + player_motion[0]->speed.y + turned.y;
        camera_state->target.z =
            player_work[0]->pos.z + player_motion[0]->speed.z + turned.z;
    }
}

void camera_step_eyes(struct camera_view *view)
{
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y =
        player_work[0]->pos.y + player_get(0)->consts.eye_height;
    camera_state->target.z = player_work[0]->pos.z;
    camera_place_look_at(camera_work);
}

/* the squared distance between two points */
static float vec3_distance_squared(const NJS_POINT3 *from, const NJS_POINT3 *to)
{
    /* @hack spelling: byte cursors over the two points, for the image's @Rn+
     * loads (0x8c022210 to 0x8c022218); through the members, the function
     * runs 4 bytes long and 16 of 17 words differ */
    const char *at_from = (const char *)from;
    const char *at_to = (const char *)to;
    float dx;
    float dy;
    float dz;

    dx = *(const float *)at_from - *(const float *)at_to;
    at_from += sizeof(float);
    at_to += sizeof(float);
    dy = *(const float *)at_from - *(const float *)at_to;
    at_from += sizeof(float);
    at_to += sizeof(float);
    dz = *(const float *)at_from - *(const float *)at_to;
    return (dx * dx) + (dy * dy) + (dz * dz);
}

/* 1 when index is the path's first point or one of its last three, and 0
 * otherwise. point goes unread */
static int at_path_end(int index, NJS_POINT3 *point,
                       struct camera_path *stretch)
{
    int limit = stretch->path->count - 3;

    if (index >= limit || index == 0) {
        return 1;
    }
    return 0;
}

/* follow point along the stretch's path, each path point moved by
 * path_shift. the segment runs from point *index, clamped to the last but
 * one, to the next point. out takes the point of the segment's line nearest
 * point. dir, unless null, takes the way from the segment's second point
 * back to its first. while out lies on the segment, *index takes the
 * segment's first point, and the function returns 0. once out lies past an
 * end, *index moves one point that way. the function then returns 1 and
 * moves out onto that end. where *index reaches the path's first or last
 * point, it returns 0 and leaves out on the line */
static int path_track_point(int *index, NJS_POINT3 *out, NJS_POINT3 *point,
                            NJS_VECTOR *dir, struct camera_path *stretch)
{
    int i;
    int last;
    int next;
    NJS_POINT3 p0;
    NJS_POINT3 p1;
    NJS_POINT3 cp;
    NJS_LINE line;
    Float d0;
    Float d1;
    Float d2;

    /* @hack spelling: last set before i, which copies out to R10 (0x8c022294)
     * ahead of point's store to the frame (0x8c022296); set after i, the
     * two trade and 2 of 239 words differ */
    last = stretch->path->count - 1;
    i = *index;
    if (i >= last) {
        i = last - 1;
    }
    next = i + 1;
    p0.x = stretch->path->points[i].pos.x + stretch->path_shift.x;
    p0.y = stretch->path->points[i].pos.y + stretch->path_shift.y;
    p0.z = stretch->path->points[i].pos.z + stretch->path_shift.z;
    p1.x = stretch->path->points[next].pos.x + stretch->path_shift.x;
    p1.y = stretch->path->points[next].pos.y + stretch->path_shift.y;
    p1.z = stretch->path->points[next].pos.z + stretch->path_shift.z;
    line.px = p0.x;
    line.py = p0.y;
    line.pz = p0.z;
    line.vx = p0.x - p1.x;
    line.vy = p0.y - p1.y;
    line.vz = p0.z - p1.z;
    njDistanceP2L(point, &line, &cp);
    d0 = vec3_distance_squared(&p0, &p1);
    d1 = vec3_distance_squared(&p0, &cp);
    d2 = vec3_distance_squared(&p1, &cp);
    if (dir) {
        dir->x = p0.x - p1.x;
        dir->y = p0.y - p1.y;
        dir->z = p0.z - p1.z;
    }
    *index = i;
    *out = cp;
    /* @hack spelling: each end returns 0 from an else, which keeps p1's and
     * p0's addresses in R1 and i - 1 in R2 (0x8c022410, 0x8c022426,
     * 0x8c022436); as early returns, 16 of 239 words differ */
    if (d1 + d2 > d0) {
        if (d1 > d2) {
            /* @hack spelling: i + 1, not next, which SHC reads from next's
             * frame slot (0x8c022406); written next, next takes R9 and 134 of
             * 239 words differ */
            *index = i + 1;
            if (*index < last) {
                *out = p1;
            } else {
                return 0;
            }
        } else {
            *index = i > 0 ? i - 1 : 0;
            if (*index > 0) {
                *out = p0;
            } else {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

/* the index of the path's point nearest point, each moved by the stretch's
 * path_shift */
static int path_nearest_index(NJS_POINT3 *point, struct camera_path *stretch)
{
    int count = stretch->path->count;
    int i;
    int nearest;
    Float best;
    Float distance;
    NJS_POINT3 shifted;

    for (i = 0; i < count; i++) {
        /* @hack spelling: byte cursors over the point and the shift, for the
         * image's @Rn+ loads (0x8c0224b4 to 0x8c0224c2); through the
         * members, the function runs 22 bytes long and 49 of 64 words
         * differ */
        const char *along = (const char *)&stretch->path->points[i].pos;
        const char *shift = (const char *)&stretch->path_shift;

        shifted.x = *(const float *)along + *(const float *)shift;
        along += sizeof(float);
        shift += sizeof(float);
        shifted.y = *(const float *)along + *(const float *)shift;
        along += sizeof(float);
        shift += sizeof(float);
        shifted.z = *(const float *)along + *(const float *)shift;
        distance = vec3_distance_squared(&shifted, point);
        if (i == 0 || best > distance) {
            best = distance;
            nearest = i;
        }
    }
    return nearest;
}

void camera_step_path(struct camera_view *view)
{
    /* the unit way back along the path at stretch_focus */
    static NJS_VECTOR stretch_back = {0.0F, 0.0F, 0.0F};
    /* the move CAMERA_STRETCH_KEEP_MOVE keeps between steps */
    static NJS_VECTOR stretch_move = {0.0F, 0.0F, 0.0F};
    /* the point the camera follows. each step it moves 0.98 of the way to the
     * point that lies the stretch's height above player 0, along his up */
    static NJS_POINT3 stretch_focus = {0.0F, 0.0F, 0.0F};
    /* the indexes of the path points path_track_point last left */
    static int stretch_sit = 0; /* for where the camera sits */
    static int stretch_look = 0; /* for where it looks */
    static int stretch_near = 0; /* for stretch_focus */
    /* the path the statics above are for. another path resets them */
    static struct path *stretch_last = NULL;
    NJS_POINT3 sit_point;
    NJS_POINT3 position;
    NJS_POINT3 sit;
    NJS_POINT3 nearest;
    NJS_POINT3 followed;
    NJS_VECTOR offset;
    NJS_VECTOR reach;
    NJS_POINT3 look_point;
    NJS_POINT3 target;
    NJS_POINT3 look;
    NJS_VECTOR lift;
    struct angles angles;
    struct camera_path *stretch;
    int reset;
    Angle angle;
    Angle dx;
    Angle dy;
    Angle dz;
    Sint16 *low_x;
    Sint16 *low_y;
    Sint16 *low_z;
    Float fx;
    Float fy;
    Float fz;
    Float rate;
    Float length;
    Float length2;
    Float scale;
    Float x;
    Float z;
    Float blend;
    NJS_VECTOR *pull;

    stretch = CAMERA_VARS(camera_work)->path;
    if (stretch->path != stretch_last || view->steps++ == 0 ||
        (stretch_back.x == 0.0F && stretch_back.y == 0.0F &&
         stretch_back.z == 0.0F)) {
        view->steps = 1;
        camera_state->pos.x = camera_work->pos.x;
        camera_state->pos.y = camera_work->pos.y;
        camera_state->pos.z = camera_work->pos.z;
        stretch_last = stretch->path;
        /* @hack redundant: a store that never runs, whose uses of the view and
         * of camera_state's address keep the squared-length test's floats in
         * FR5 and FR7 to FR9 (0x8c022c3a to 0x8c022c48); without it, 15 of
         * 1025 words differ */
        if (0) {
            view->steps = camera_state->ang_x;
        }
        reset = 1;
    } else {
        reset = 0;
    }
    /* @hack redundant: an empty test of wait against the view's steps, CMP/HI
     * at 0x8c0225ca, whose result nothing reads; without it, 211 of 1025 words
     * differ */
    if (stretch->wait > (Uint32)view->steps) {
    }
    camera_state->ang_x = camera_work->ang.x;
    camera_state->ang_y = camera_work->ang.y;
    camera_state->ang_z = camera_work->ang.z;
    {
        NJS_VECTOR way;

        way.x = 0.0F;
        way.y = stretch->height;
        way.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &player_work[0]->pos);
        TURN_VIA(njRotateZ, player_work[0]->ang.z, angle);
        TURN_VIA(njRotateX, player_work[0]->ang.x, angle);
        TURN_VIA(njRotateY, player_work[0]->ang.y, angle);
        njCalcPoint(NULL, &way, &followed);
        njPopMatrix(1);
        if (reset) {
            stretch_focus = followed;
            camera_work->submode = 0;
            stretch_move.x = 16.0F;
            stretch_move.y = 16.0F;
            stretch_move.z = 16.0F;
            stretch_sit = stretch_look = stretch_near =
                path_nearest_index(&stretch_focus, stretch);
        }
        /* @hack spelling: way through pull, which keeps its address in R4
         * (0x8c0226a2); by name, 50 of 1025 words differ */
        pull = &way;
        pull->x = followed.x - stretch_focus.x;
        pull->y = followed.y - stretch_focus.y;
        pull->z = followed.z - stretch_focus.z;
        stretch_focus.x += pull->x * 0.98F;
        stretch_focus.y += pull->y * 0.98F;
        stretch_focus.z += pull->z * 0.98F;
        path_track_point(&stretch_near, &nearest, &stretch_focus, &way,
                         stretch);
        if (stretch->flags & CAMERA_STRETCH_MEASURE) {
            if (njDistanceP2P(&stretch_focus, &nearest) > stretch->far) {
                camera_work->submode |= CAMERA_PATH_FAR;
            } else {
                camera_work->submode &= ~CAMERA_PATH_FAR;
            }
        }
        njUnitVector(&way);
        if (reset) {
            stretch_back = way;
        } else {
            stretch_back.x = ((way.x - stretch_back.x) * 0.3F) + stretch_back.x;
            stretch_back.y = ((way.y - stretch_back.y) * 0.3F) + stretch_back.y;
            stretch_back.z = ((way.z - stretch_back.z) * 0.3F) + stretch_back.z;
            njUnitVector(&stretch_back);
        }
        sit_point.x = stretch_focus.x + (stretch_back.x * stretch->sit_back);
        sit_point.y = stretch_focus.y + (stretch_back.y * stretch->sit_back);
        sit_point.z = stretch_focus.z + (stretch_back.z * stretch->sit_back);
        camera_work->submode &= ~CAMERA_PATH_AT_END;
        path_track_point(&stretch_sit, &sit, &sit_point, NULL, stretch);
        if (at_path_end(stretch_sit, &sit, stretch)) {
            camera_work->submode |= CAMERA_PATH_AT_END;
        }
        look_point.x = stretch_focus.x - (stretch_back.x * stretch->look_ahead);
        look_point.y = stretch_focus.y - (stretch_back.y * stretch->look_ahead);
        look_point.z = stretch_focus.z - (stretch_back.z * stretch->look_ahead);
        path_track_point(&stretch_look, &look, &look_point, NULL, stretch);
        if (at_path_end(stretch_look, &look, stretch)) {
            camera_work->submode |= CAMERA_PATH_AT_END;
        }
    }
    offset.x = stretch_focus.x - nearest.x;
    offset.y = stretch_focus.y - nearest.y;
    offset.z = stretch_focus.z - nearest.z;
    reach.x = offset.x;
    reach.y = offset.y;
    reach.z = offset.z;
    lift.x = offset.x;
    lift.y = offset.y;
    lift.z = offset.z;
    if (stretch->flags & CAMERA_STRETCH_WALLS) {
        struct ray_query query;
        struct ray_query query2;

        length = njScalor(&reach);
        query.length = length;
        /* @hack redundant: two stores that never run, which keep the first
         * length in FR15 and 0.0 in FR14 (0x8c0229ca, 0x8c0225d2); without
         * them, or with one, the two trade and 31 of 1025 words differ, and
         * without them and the store at the reset, 25 */
        if (0) {
            query.length = length;
            query.length = length;
        }
        query.origin = sit;
        query.dir = reach;
        camera_work->submode &= ~CAMERA_PATH_WALLED;
        if (polygon_cast_ray(&query) == 1 && !(query.distance > length)) {
            if (stretch->flags & CAMERA_STRETCH_SIGHT) {
                length2 = njScalor(&offset);
                query2.length = length2;
                query2.origin = nearest;
                query2.dir = offset;
                if (polygon_cast_ray(&query2) == 1 &&
                    !(query2.distance > length2)) {
                    camera_work->submode |= CAMERA_PATH_WALLED;
                }
            }
            reach.x = query.hit.x - query.origin.x;
            reach.y = query.hit.y - query.origin.y;
            reach.z = query.hit.z - query.origin.z;
        }
        length = njScalor(&reach) / length;
        lift.x *= length;
        lift.y *= length;
        lift.z *= length;
    }
    if (stretch->short_by != 0.0F) {
        length = njScalor(&reach);
        scale = (length - stretch->short_by) / length;
        if (scale < 0.0F) {
            scale = 0.0F;
        }
        reach.x *= scale;
        reach.y *= scale;
        reach.z *= scale;
        lift.x *= scale;
        lift.y *= scale;
        lift.z *= scale;
    }
    position.x = sit.x + (reach.x * stretch->sit_share) + stretch->shift.x;
    position.y = sit.y + (reach.y * stretch->sit_share) + stretch->shift.y;
    position.z = sit.z + (reach.z * stretch->sit_share) + stretch->shift.z;
    target.x = look.x + (lift.x * stretch->look_share);
    target.y = look.y + (lift.y * stretch->look_share);
    target.z = look.z + (lift.z * stretch->look_share);
    if (stretch->flags & CAMERA_STRETCH_KEEP_MOVE) {
        NJS_VECTOR move;
        NJS_VECTOR step;

        move.x = position.x - camera_state->pos.x;
        move.y = position.y - camera_state->pos.y;
        move.z = position.z - camera_state->pos.z;
        step.x = move.x - stretch_move.x;
        step.y = move.y - stretch_move.y;
        step.z = move.z - stretch_move.z;
        length = njScalor(&step);
        /* the camera's distance from stretch_focus. nothing reads it */
        njDistanceP2P(&position, &stretch_focus);
        /* @hack redundant: an empty test of the squared lengths, FCMP/GT at
         * 0x8c022c64, whose result nothing reads; without it, 342 of 1025
         * words differ and the code runs 32 bytes short */
        if ((move.x * move.x) + (move.y * move.y) + (move.z * move.z) <
            (stretch_move.x * stretch_move.x) +
                (stretch_move.y * stretch_move.y) +
                (stretch_move.z * stretch_move.z)) {
        }
        /* @bug logic: length > length never holds (FCMP/GT FR15,FR15 at
         * 0x8c022c66), so stretch_move takes the whole change to the new
         * move, never a scaled part of it */
        /* NOLINTNEXTLINE(misc-redundant-expression) */
        if (length > length) {
            /* NOLINTNEXTLINE(misc-redundant-expression) */
            scale = length / length;
            step.x *= scale;
            step.y *= scale;
            step.z *= scale;
        }
        stretch_move.x += step.x;
        stretch_move.y += step.y;
        stretch_move.z += step.z;
        position.x = camera_state->pos.x + stretch_move.x;
        position.y = camera_state->pos.y + stretch_move.y;
        position.z = camera_state->pos.z + stretch_move.z;
    }
    if (stretch->flags & CAMERA_STRETCH_EASE) {
        NJS_VECTOR move;

        move.x = position.x - camera_state->pos.x;
        move.y = position.y - camera_state->pos.y;
        move.z = position.z - camera_state->pos.z;
        blend = stretch->ease;
        position.x = camera_state->pos.x + (move.x * blend);
        position.y = camera_state->pos.y + (move.y * blend);
        position.z = camera_state->pos.z + (move.z * blend);
    }
    x = position.x - target.x;
    z = position.z - target.z;
    angles.y = (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN) & 0xffff;
    angles.x =
        (Angle)(atan2f(target.y - position.y, njSqrt((x * x) + (z * z))) *
                ANGLE_PER_RADIAN) &
        0xffff;
    angles.z = (Uint16)camera_state->ang_z;
    /* turn each of camera_state's angles toward angles, the aim from
     * position at target, by the share stretch->turn of the short way. that
     * way is the low half of the angles' difference, read as a Sint16. the
     * cast to Uint16 drops the high half left in the word */
    /* @hack type: dx, dy and dz are words, stored whole (0x8c022e12,
     * 0x8c022e1c, 0x8c022e26), whose low halves the scaling reads and
     * rewrites through Sint16 pointers (0x8c022e28 to 0x8c022e54) before the
     * words are read back whole (0x8c022e58, 0x8c022e64, 0x8c022e70); as
     * union angle_halves, the function runs 8 bytes long and 375 of 1025
     * words differ */
    /* @hack spelling: the pointers set ahead of the deltas, which issues ADD
     * #8,R4 before MOV R15,R6 (0x8c022df6, 0x8c022df8); set after them, 2 of
     * 1025 words differ */
    low_x = (Sint16 *)&dx;
    low_y = (Sint16 *)&dy;
    low_z = (Sint16 *)&dz;
    dx = angles.x - camera_state->ang_x;
    dy = angles.y - camera_state->ang_y;
    dz = angles.z - camera_state->ang_z;
    fx = *low_x;
    fy = *low_y;
    fz = *low_z;
    rate = stretch->turn;
    /* @hack spelling: fx scaled in a statement of its own, which keeps fx, fy
     * and fz in FR5 to FR7 (0x8c022e32 to 0x8c022e3e) and the squared-length
     * test's floats in theirs; as *low_x = (Sint16)(Sint32)(fx * rate), 27 of
     * 1025 words differ */
    fx *= rate;
    *low_x = (Sint16)(Sint32)fx;
    *low_y = (Sint16)(Sint32)(fy * rate);
    *low_z = (Sint16)(Sint32)(fz * rate);
    angles.x = (Uint16)(dx + camera_state->ang_x);
    angles.y = (Uint16)(dy + camera_state->ang_y);
    angles.z = (Uint16)(dz + camera_state->ang_z);
    camera_state->pos.x = position.x;
    camera_state->pos.y = position.y;
    camera_state->pos.z = position.z;
    camera_state->ang_x = angles.x;
    camera_state->ang_y = angles.y;
    camera_state->ang_z = angles.z;
    camera_place_at_angles(camera_work);
}

/* SURVEY's offsets of the camera from player 0's eyes, before they turn
 * the way he faces. it takes the second where, as the mode begins, a sweep
 * from his eyes along the first hits */
static NJS_POINT3 survey_eye_offsets[2] = {
    {0.0F, 10.0F, -10.0F},
    {0.0F, 10.0F, 0.2F},
};
/* CART's views, by camera_state's view. each moves the camera and its
 * target from where cart_view_above puts them, turned by camera_place's
 * ang */
static struct camera_key cart_views[3] = {
#include "camera/modes/cart_views.inc"
};
/* FOLLOW_G's distance from its target. while Y is held, L adds 2 a step
 * and R takes 2 off */
static Float follow_g_distance = 50.0F;

void camera_step_pose(struct camera_view *view)
{
    NJS_POINT3 pos;
    struct angles ang;

    camera_state->pos.x = camera_work->pos.x;
    camera_state->pos.y = camera_work->pos.y;
    camera_state->pos.z = camera_work->pos.z;
    camera_state->ang_x = camera_work->ang.x;
    camera_state->ang_y = camera_work->ang.y;
    camera_state->ang_z = camera_work->ang.z;
    /* @hack redundant: the camera's place, as camera_work gives it, copied
     * into pos and ang, which nothing reads (0x8c022f52, 0x8c022f6e);
     * without the copies, 94 of 96 words differ and the code runs 32 bytes
     * short */
    pos.x = camera_state->pos.x;
    pos.y = camera_state->pos.y;
    pos.z = camera_state->pos.z;
    ang.x = camera_state->ang_x;
    ang.y = camera_state->ang_y;
    ang.z = camera_state->ang_z;
    camera_state->pos.x = camera_pose_pos.x;
    camera_state->pos.y = camera_pose_pos.y;
    camera_state->pos.z = camera_pose_pos.z;
    camera_state->ang_x = camera_pose_ang.x;
    camera_state->ang_y = camera_pose_ang.y;
    camera_state->ang_z = camera_pose_ang.z;
    camera_pose_taken = 1;
    camera_place_at_angles(camera_work);
}

static void turn_vector(struct angles *ang, NJS_VECTOR *vector);

/* TAIHO's step. as the mode begins, the camera takes camera_place's pos,
 * moved 2 out along the place's ang. it turns by that ang, and half a turn
 * more about y. pad 0's stick turns it further, up to TAIHO_TURN_MAX either
 * way. the turn is a degree a step per 16000 of the stick's reading shifted
 * up a byte, about 2 degrees a step at full tilt. SE_TAIHO_TURN plays while
 * the stick turns it more than 50 a step */
static void camera_step_taiho(struct camera_view *view)
{
    /* @hack spelling: camera_state through its address, which keeps
     * taiho_turn's address in R14 and camera_state's in R13 (0x8c022fea,
     * 0x8c022fec); written camera_state->, they trade, and 3 of 178 words and 4
     * pool words differ */
    struct camera_state *const *cam = &camera_state;
    NJS_VECTOR ahead;
    Float turn;
    /* @hack spelling: taiho_last_turn and taiho_turn through previous and
     * offset, which load taiho_last_turn's address into R4 beside pad_records'
     * in R5 (0x8c023072, 0x8c02306e), and taiho_turn's into R3 for the copy
     * (0x8c023070); by name, 14 and 16 of 178 words differ */
    struct angles *previous;
    struct angles *offset = &taiho_turn;
    Angle angle;

    if (!view->steps) {
        offset->x = 0;
        offset->y = 0;
        offset->z = 0;
        (*cam)->pos.x = camera_place.pos.x;
        (*cam)->pos.y = camera_place.pos.y;
        (*cam)->pos.z = camera_place.pos.z;
        taiho_base.x = camera_place.ang.x;
        taiho_base.y = camera_place.ang.y;
        taiho_base.z = camera_place.ang.z;
        ahead.x = 0.0F;
        ahead.y = 0.0F;
        ahead.z = 2.0F;
        turn_vector(&taiho_base, &ahead);
        (*cam)->pos.x += ahead.x;
        (*cam)->pos.y += ahead.y;
        (*cam)->pos.z += ahead.z;
    }
    previous = &taiho_last_turn;
    *previous = taiho_turn;
    /* @hack redundant: a copy of taiho_base, in an arm of ?:, that never
     * runs: with it, SHC keeps no register for taiho_base's address, and each
     * member loads its own pool word. without it, the address takes R12,
     * 174 of 178 words and 66 pool words differ, and the code runs 32 bytes
     * short */
    if (0) {
        /* NOLINTBEGIN(readability-redundant-casting) */
        /* NOLINTBEGIN(clang-diagnostic-self-assign) */
        (void)(1 ? (void)(taiho_base = taiho_base) : (void)0);
        /* NOLINTEND(clang-diagnostic-self-assign) */
        /* NOLINTEND(readability-redundant-casting) */
    }
    /* @hack spelling: taiho_turn's x through its address, which reaches it
     * through the R3 the copy above leaves (0x8c023086, 0x8c023096); as
     * offset->x, through R14, 14 of 178 words differ */
    (&taiho_turn)->x -= (Angle)((float)(pad_records[0].y1 << 8) *
                                (ANGLE_PER_DEGREE * (1.0F / 16000.0F)));
    offset->y -= (Angle)((float)(pad_records[0].x1 << 8) *
                         (ANGLE_PER_DEGREE * (1.0F / 16000.0F)));
    if (offset->x > TAIHO_TURN_MAX) {
        offset->x = TAIHO_TURN_MAX;
    } else if (offset->x < -TAIHO_TURN_MAX) {
        offset->x = -TAIHO_TURN_MAX;
    }
    angle = offset->y;
    if (angle > TAIHO_TURN_MAX) {
        offset->y = TAIHO_TURN_MAX;
    } else if (angle < -TAIHO_TURN_MAX) {
        offset->y = -TAIHO_TURN_MAX;
    }
    turn = _builtin_fabsf((float)(previous->x - offset->x)) +
           _builtin_fabsf((float)(previous->y - offset->y));
    if (turn > 50.0F) {
        sound_play(SE_TAIHO_TURN, 0, 0, 0);
    } else {
        sound_stop(SE_TAIHO_TURN);
    }
    (*cam)->ang_x = offset->x + taiho_base.x;
    (*cam)->ang_y = offset->y + taiho_base.y + 0x8000;
    (*cam)->ang_z = offset->z + taiho_base.z;
    camera_place_at_angles(camera_work);
}

/* turn vector by ang, through a matrix turned about z, then x, then y */
static void turn_vector(struct angles *ang, NJS_VECTOR *vector)
{
    Angle angle;

    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, ang->z, angle);
    TURN_VIA(njRotateX, ang->x, angle);
    TURN_VIA(njRotateY, ang->y, angle);
    njCalcVector(NULL, vector, vector);
    njPopMatrix(1);
}

/* SURVEY's step, the pad's look round. the camera sits at player 0's eyes,
 * moved by an entry of survey_eye_offsets turned the way he faces. the
 * d-pad turns it SURVEY_TURN a step from looking the way he faces. its
 * pitch stays within SURVEY_PITCH_MAX of level */
static void camera_step_survey(struct camera_view *view)
{
    static struct angles survey_turn;
    static struct angles survey_eye_ang;
    static int survey_hit;
    /* @hack spelling: the pointers declared hit, offset, cam, which loads
     * camera_state's address and hit's at entry in the image's order
     * (0x8c023272, 0x8c023274); declared cam, offset, hit, they trade and 2
     * of 228 words and 4 pool words differ */
    int *hit = &survey_hit;
    struct angles *offset = &survey_turn;
    /* @hack spelling: camera_state through its address, which keeps
     * camera_state's address in R14 and survey_turn's in R13 (0x8c023272,
     * 0x8c023278); written camera_state->, they trade and 32 of 228 words
     * differ */
    struct camera_state *const *cam = &camera_state;
    NJS_VECTOR from_eyes;
    Angle angle;

    if (!view->steps) {
        offset->x = 0;
        offset->y = 0;
        offset->z = 0;
        *hit = 0;
        (*cam)->pos.x = player_work[0]->pos.x;
        (*cam)->pos.y =
            player_work[0]->pos.y + player_get(0)->consts.eye_height;
        (*cam)->pos.z = player_work[0]->pos.z;
        survey_eye_ang.x = player_work[0]->ang.x;
        survey_eye_ang.y = 0x4000 - player_work[0]->ang.y;
        survey_eye_ang.z = player_work[0]->ang.z;
        from_eyes = survey_eye_offsets[0];
        turn_vector(&survey_eye_ang, &from_eyes);
        if (camera_sweep_hits(&(*cam)->pos, &from_eyes, NULL, 4.0F)) {
            *hit = 1;
        }
    }
    (*cam)->pos.x = player_work[0]->pos.x;
    (*cam)->pos.y = player_work[0]->pos.y + player_get(0)->consts.eye_height;
    (*cam)->pos.z = player_work[0]->pos.z;
    survey_eye_ang.x = player_work[0]->ang.x;
    survey_eye_ang.y = 0x4000 - player_work[0]->ang.y;
    survey_eye_ang.z = player_work[0]->ang.z;
    from_eyes = survey_eye_offsets[*hit];
    turn_vector(&survey_eye_ang, &from_eyes);
    (*cam)->pos.x += from_eyes.x;
    (*cam)->pos.y += from_eyes.y;
    (*cam)->pos.z += from_eyes.z;
    if (per_info[0]->on & PDD_DGT_KU) {
        offset->x += SURVEY_TURN;
    } else if (per_info[0]->on & PDD_DGT_KD) {
        offset->x -= SURVEY_TURN;
    }
    if (per_info[0]->on & PDD_DGT_KL) {
        offset->y += SURVEY_TURN;
    } else if (per_info[0]->on & PDD_DGT_KR) {
        offset->y -= SURVEY_TURN;
    }
    if ((angle = offset->x) > SURVEY_PITCH_MAX) {
        offset->x = SURVEY_PITCH_MAX;
    } else if (angle < -SURVEY_PITCH_MAX) {
        offset->x = -SURVEY_PITCH_MAX;
    }
    (*cam)->ang_x = offset->x;
    (*cam)->ang_y = offset->y + survey_eye_ang.y + 0x8000;
    (*cam)->ang_z = offset->z;
    camera_place_at_angles(camera_work);
}

/* where CART's camera and its target were last step, for the ease */
static NJS_POINT3 cart_last_pos;
static NJS_POINT3 cart_last_target;
/* the index of the key CAMERA_CART_PLAY plays. it counts on past the last */
static int cart_key;
static Float cart_rise; /* how far view 1 has risen */
/* camera/cart.h declares it, and says why this file leaves that header
 * out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
int camera_cart_action;
static Float sonic_far;
static Float sonic_near;
static Float sonic_far_limit;
static Float sonic_near_limit;
static Angle sonic_pitch;
static int sonic_drift;
/* sonic_shared_step's states */
enum sonic_state {
    /* the mode's first step, which starts the target at player 0 */
    SONIC_START,
    SONIC_FOLLOW, /* camera_player_still is clear */
    SONIC_DRIFT, /* camera_player_still is set, and the drift runs */
    /* camera_player_still stays set, and a slide or a trigger ended the
     * drift */
    SONIC_DRIFTED
};

static int sonic_state; /* a sonic_state */
static NJS_POINT3 sonic_last_pos;
static int sonic_triggers_idle;
static int sonic_slide_steps;
/* the work and the line camera_set_e103 gave E103's step */
static struct task_work *e103_target;
static NJS_POINT3 *e103_line;
/* how far last's x and y angles lay off ease_aim as the ease began */
static struct angles ease_off_aim;
/* the angles from where the camera was to player 0, set each step */
static struct angles ease_aim;
/* how far the x and y angles may lie off ease_aim. it goes from
 * ease_off_aim to EASE_LIMIT_X and EASE_LIMIT_Y over the ease */
static struct angles ease_limit;
/* the share of the way the camera goes this step: adjust's steps over
 * ease_steps, held to 1 */
static Float ease_part;
/* the camera's move the step before the ease, which blends out over the
 * ease's first ease_blend steps */
static NJS_POINT3 ease_start_move;
/* the adjust mode's entry of adjust_ease_start's eases */
static Float ease_steps;
static int ease_blend;
static int ease_sweeps;
static int dat_8c6c0e68; /* adjust_ease_start zeroes it. nothing reads it */

static void cart_view_above(Float height);
static void cart_offset_pos(void);
static void cart_offset_target(void);
static void cart_ease_target(void);
static void cart_ease_both(void);
static void cart_keep_last(void);
static void cart_play_keys(struct camera_view *view);
static void cart_orbit(struct camera_view *view);
static void orbit_with_pad1(struct camera_view *view, NJS_POINT3 *out,
                            NJS_POINT3 *target);
static void take_short_ease(void);

/* CART's step. at CAMERA_CART_PLAY and CAMERA_CART_ORBIT, the camera does
 * as camera_cart_action says. otherwise it shows camera_place's pos in one
 * of three views, which up and down on the d-pad step through. view 0 sits
 * a quarter of player 0's eye height above the place, and eases only its
 * target. views 1 and 2 sit his eye height above, and ease both. view 1
 * also rises 0.5 more a step while camera_place's target.x isn't 0 */
static void camera_step_cart(struct camera_view *view)
{
    NJS_POINT3 previous;

    if (!view->steps) {
        camera_state->view = 1;
        cart_last_pos = player_work[0]->pos;
        cart_last_target = player_work[0]->pos;
        cart_last_pos.y += player_get(0)->consts.eye_height;
    }
    if (camera_cart_action == CAMERA_CART_PLAY) {
        camera_set_adjust(CAMERA_ADJUST_NONE);
        camera_set_type(CAMERA_LOOK_AT);
        cart_play_keys(view);
    } else if (camera_cart_action == CAMERA_CART_ORBIT) {
        camera_set_adjust(CAMERA_ADJUST_NONE);
        cart_orbit(view);
        camera_set_type(CAMERA_FROM_TARGET);
        cart_key = 0;
    } else {
        cart_key = 0;
        camera_set_type(CAMERA_LOOK_AT);
        previous = camera_work->pos;
        switch (camera_state->view) {
        case 0:
            cart_view_above(player_get(0)->consts.eye_height * 0.25F);
            cart_offset_pos();
            cart_offset_target();
            cart_ease_target();
            break;
        case 1:
            if (camera_place.target.x != 0.0F) {
                cart_rise += 0.5F;
            } else {
                cart_rise = 0.0F;
            }
            cart_view_above(player_get(0)->consts.eye_height);
            cart_offset_pos();
            cart_offset_target();
            cart_ease_both();
            break;
        case 2:
            cart_view_above(player_get(0)->consts.eye_height);
            cart_offset_pos();
            cart_offset_target();
            cart_ease_both();
            break;
        default:
            camera_state->view = 1;
            return;
            /* @hack redundant: a break after the return, which SHC deletes, but
             * whose jump leaves camera_state's next load in R2 (0x8c02365c);
             * without it, 4 of 201 words differ */
            break;
        }
        camera_state->ang_z = 0;
        camera_sweep_unless_c(&previous, &camera_state->pos);
        cart_keep_last();
        if (per_info[0]->press & PDD_DGT_KU) {
            camera_state->view++;
            if (camera_state->view >= 3) {
                camera_state->view = 0;
            }
        } else if (per_info[0]->press & PDD_DGT_KD) {
            camera_state->view--;
            /* @bug logic: view is unsigned, so the wrap to view
             * 2 never runs: from view 0 the press leaves 0xff, and the next
             * step's default puts view 1, at 0x8c0236d0, 0x8c023654 */
            if (camera_state->view < 0) {
                camera_state->view = 2;
            }
        }
    }
}

/* put the camera height and cart_rise above camera_place's pos, looking
 * at the pos */
static void cart_view_above(Float height)
{
    camera_state->pos.x = camera_place.pos.x;
    camera_state->pos.y = camera_place.pos.y + height + cart_rise;
    camera_state->pos.z = camera_place.pos.z;
    camera_state->target.x = camera_place.pos.x;
    camera_state->target.y = camera_place.pos.y;
    camera_state->target.z = camera_place.pos.z;
}

/* move the camera by its view's pos in cart_views, turned by
 * camera_place's ang */
static void cart_offset_pos(void)
{
    NJS_VECTOR offset;

    offset = cart_views[camera_state->view].pos;
    turn_vector(&camera_place.ang, &offset);
    camera_state->pos.x += offset.x;
    camera_state->pos.y += offset.y;
    camera_state->pos.z += offset.z;
}

/* move the camera's target by its view's target in cart_views, turned the
 * same way */
static void cart_offset_target(void)
{
    NJS_VECTOR offset;

    offset = cart_views[camera_state->view].target;
    turn_vector(&camera_place.ang, &offset);
    camera_state->target.x += offset.x;
    camera_state->target.y += offset.y;
    camera_state->target.z += offset.z;
}

/* ease the camera's target a fifth of the way from where it was last step
 * to where the view puts it */
static void cart_ease_target(void)
{
    /* @hack redundant: each of the camera's pos members stored back
     * unchanged (0x8c023804, 0x8c02380e, 0x8c023818); without the stores, 48
     * of 50 words differ */
    camera_state->pos.x = camera_state->pos.x;
    camera_state->pos.y = camera_state->pos.y;
    camera_state->pos.z = camera_state->pos.z;
    camera_state->target.x =
        cart_last_target.x +
        ((camera_state->target.x - cart_last_target.x) * 0.2F);
    camera_state->target.y =
        cart_last_target.y +
        ((camera_state->target.y - cart_last_target.y) * 0.2F);
    camera_state->target.z =
        cart_last_target.z +
        ((camera_state->target.z - cart_last_target.z) * 0.2F);
}

/* ease the camera and its target a fifth of the way from where they were
 * last step to where the view puts them */
static void cart_ease_both(void)
{
    camera_state->pos.x =
        cart_last_pos.x + ((camera_state->pos.x - cart_last_pos.x) * 0.2F);
    camera_state->pos.y =
        cart_last_pos.y + ((camera_state->pos.y - cart_last_pos.y) * 0.2F);
    camera_state->pos.z =
        cart_last_pos.z + ((camera_state->pos.z - cart_last_pos.z) * 0.2F);
    camera_state->target.x =
        cart_last_target.x +
        ((camera_state->target.x - cart_last_target.x) * 0.2F);
    camera_state->target.y =
        cart_last_target.y +
        ((camera_state->target.y - cart_last_target.y) * 0.2F);
    camera_state->target.z =
        cart_last_target.z +
        ((camera_state->target.z - cart_last_target.z) * 0.2F);
}

/* keep where the camera and its target are, for the next step's ease */
static void cart_keep_last(void)
{
    cart_last_pos.x = camera_state->pos.x;
    cart_last_pos.y = camera_state->pos.y;
    cart_last_pos.z = camera_state->pos.z;
    cart_last_target.x = camera_state->target.x;
    cart_last_target.y = camera_state->target.y;
    cart_last_target.z = camera_state->target.z;
}

/* CAMERA_CART_PLAY's work. the camera takes camera_place's keys, one a
 * step, up to the last. it holds there 10 steps, then eases a twentieth of
 * the way a step toward view 1. at 70 steps past the last key, it clears
 * camera_cart_action and takes view 1 */
static void cart_play_keys(struct camera_view *view)
{
    struct camera_key *keys = camera_place.args[0].keys;
    int mode;

    if (cart_key == camera_place.args[1].last + 70) {
        camera_cart_action = 0;
        camera_state->view = 1;
        camera_state->pos.x =
            cart_last_pos.x + ((camera_state->pos.x - cart_last_pos.x) * 0.05F);
        camera_state->pos.y =
            cart_last_pos.y + ((camera_state->pos.y - cart_last_pos.y) * 0.05F);
        camera_state->pos.z =
            cart_last_pos.z + ((camera_state->pos.z - cart_last_pos.z) * 0.05F);
        camera_state->target.x =
            cart_last_target.x +
            ((camera_state->target.x - cart_last_target.x) * 0.05F);
        camera_state->target.y =
            cart_last_target.y +
            ((camera_state->target.y - cart_last_target.y) * 0.05F);
        camera_state->target.z =
            cart_last_target.z +
            ((camera_state->target.z - cart_last_target.z) * 0.05F);
    } else if (cart_key > camera_place.args[1].last + 10) {
        mode = camera_state->view;
        camera_state->view = 1;
        cart_view_above(player_get(0)->consts.eye_height);
        cart_offset_pos();
        cart_offset_target();
        camera_state->view = mode;
        camera_state->pos.x =
            cart_last_pos.x + ((camera_state->pos.x - cart_last_pos.x) * 0.05F);
        camera_state->pos.y =
            cart_last_pos.y + ((camera_state->pos.y - cart_last_pos.y) * 0.05F);
        camera_state->pos.z =
            cart_last_pos.z + ((camera_state->pos.z - cart_last_pos.z) * 0.05F);
        camera_state->target.x =
            cart_last_target.x +
            ((camera_state->target.x - cart_last_target.x) * 0.05F);
        camera_state->target.y =
            cart_last_target.y +
            ((camera_state->target.y - cart_last_target.y) * 0.05F);
        camera_state->target.z =
            cart_last_target.z +
            ((camera_state->target.z - cart_last_target.z) * 0.05F);
    } else if (cart_key <= camera_place.args[1].last) {
        camera_state->pos.x = keys[cart_key].pos.x;
        camera_state->pos.y = keys[cart_key].pos.y;
        camera_state->pos.z = keys[cart_key].pos.z;
        camera_state->target.x = keys[cart_key].target.x;
        camera_state->target.y = keys[cart_key].target.y;
        camera_state->target.z = keys[cart_key].target.z;
    }
    cart_key++;
    cart_keep_last();
}

/* CAMERA_CART_ORBIT's work: orbit_with_pad1 about the point camera_place's
 * args[1] holds, keeping the camera's position in the point args[0]
 * holds */
static void cart_orbit(struct camera_view *view)
{
    NJS_POINT3 *out = camera_place.args[0].point;
    NJS_POINT3 *target = camera_place.args[1].point;

    orbit_with_pad1(view, out, target);
}

/* place the camera to look at target, from its angles and distance. while
 * pad 1 holds Start, its stick turns the camera, and L and R move it out
 * and in by 2. the camera keeps at least 5 away. out takes where it then
 * is */
static void orbit_with_pad1(struct camera_view *view, NJS_POINT3 *out,
                            NJS_POINT3 *target)
{
    Sint16 x;
    Sint16 y;
    /* NOLINTBEGIN(clang-diagnostic-unused-but-set-variable) */
    unsigned short right;
    unsigned short left;
    /* NOLINTEND(clang-diagnostic-unused-but-set-variable) */

    camera_state->target.x = target->x;
    camera_state->target.y = target->y;
    camera_state->target.z = target->z;
    camera_state->ang_z = 0;
    if (pad_records[1].on & PDD_DGT_ST) {
        x = per_info[1]->x1 << 8;
        y = per_info[1]->y1 << 8;
        if (x > -5000 && x < 5000) {
            x = 0;
        }
        if (y > -5000 && y < 5000) {
            y = 0;
        }
        /* @hack redundant: the triggers, set and unread: SHC keeps
         * &per_info[1] and per_info[1] in the frame and reads r (0x8c023bf0,
         * 0x8c023c2e, 0x8c023c30); without both lines, or with right's alone,
         * 69 of 126 words differ */
        right = ((Sint16)per_info[1]->r << 8) + 0x8000;
        left = ((Sint16)per_info[1]->l << 8) + 0x8000;
        camera_state->ang_y += x / 64;
        camera_state->ang_x += y / 64;
        if (pad_records[1].on & PDD_DGT_TL) {
            camera_state->distance += 2.0F;
        } else if (pad_records[1].on & PDD_DGT_TR) {
            camera_state->distance -= 2.0F;
        }
    }
    if (camera_state->distance < 5.0F) {
        camera_state->distance = 5.0F;
    }
    camera_place_from_target(camera_work);
    *out = camera_work->pos;
}

/* where KNUCKLES2 starts the camera, 47.5 from player 0, before
 * camera_step_follow turns it by KNUCKLES_PITCH and round behind him */
static const NJS_VECTOR trail_offset = {0.0F, 0.0F, -47.5F};

/* the KNUCKLES and KNUCKLES2 modes' step, which other steps also hand the
 * camera to. the target eases ahead of player 0. the camera eases its pitch
 * toward KNUCKLES_PITCH and keeps its distance from him. it is then pushed
 * out of the act's spheres and blocks, swept from where it was, and
 * rolled */
void camera_step_follow(struct camera_view *view)
{
    NJS_POINT3 previous;
    NJS_VECTOR vector;
    struct angles ang;
    /* @hack spelling: vector and camera_work's x angle through eye and pitch,
     * which keep their addresses in R11 and R9 (0x8c023d72, 0x8c023fd6); by
     * name, 214 and 148 of 570 words differ */
    NJS_POINT3 *eye;
    Angle *pitch;
    Float distance;
    Sint16 turn;

    if (!view->steps) {
        camera_state->target.x = player_work[0]->pos.x;
        camera_state->target.y = player_work[0]->pos.y;
        camera_state->target.z = player_work[0]->pos.z;
    }
    previous = camera_work->pos;
    /* the target eases a tenth of the way a step to 8 steps of player 0's
     * speed ahead of his eyes */
    eye = &vector;
    *eye = player_work[0]->pos;
    eye->y += player_get(0)->consts.eye_height;
    eye->x += player_motion[0]->speed.x * 8.0F;
    eye->y += player_motion[0]->speed.y * 8.0F;
    eye->z += player_motion[0]->speed.z * 8.0F;
    camera_state->target.x += (eye->x - camera_state->target.x) * 0.1F;
    camera_state->target.y += (eye->y - camera_state->target.y) * 0.1F;
    camera_state->target.z += (eye->z - camera_state->target.z) * 0.1F;
    /* in the KNUCKLES2 modes, while camera_get_steps is below 5, the camera
     * sits where trail_offset puts it. after that, it drifts 0.2 a step
     * sideways toward behind player 0, -0x4000 less his y angle */
    if (camera_get_mode() >= CAMERA_MODE_KNUCKLES2 &&
        camera_get_mode() <= CAMERA_MODE_COL_KNUCKLES2) {
        if (camera_get_steps() < 5) {
            vector = trail_offset;
            ang.x = KNUCKLES_PITCH;
            ang.y = 0x4000 - player_work[0]->ang.y;
            ang.z = 0;
            turn_vector(&ang, &vector);
            camera_state->pos.x = player_work[0]->pos.x + vector.x;
            camera_state->pos.y = player_work[0]->pos.y - vector.y;
            camera_state->pos.z = player_work[0]->pos.z + vector.z;
        } else if ((Sint16)angle_difference(-0x4000 - player_work[0]->ang.y,
                                            camera_work->ang.y) > 0) {
            camera_state->pos.x -= njCos(camera_work->ang.y) * 0.2F;
            camera_state->pos.z += njSin(camera_work->ang.y) * 0.2F;
        } else {
            camera_state->pos.x += njCos(camera_work->ang.y) * 0.2F;
            camera_state->pos.z -= njSin(camera_work->ang.y) * 0.2F;
        }
    }
    /* while camera_work's pitch is more than PITCH_SLACK off
     * KNUCKLES_PITCH, it turns PITCH_EASE toward KNUCKLES_PITCH. the camera
     * then rises or falls by the sine of how far the turned pitch lies from
     * KNUCKLES_PITCH. the rise or fall brings its pitch toward
     * KNUCKLES_PITCH. the turned pitch itself is lost when
     * camera_place_look_at places the camera */
    pitch = &camera_work->ang.x;
    turn = angle_difference(KNUCKLES_PITCH, *pitch);
    if (turn < 0) {
        turn = -turn;
    }
    if (turn > PITCH_SLACK) {
        *pitch = angle_approach(*pitch, KNUCKLES_PITCH, PITCH_EASE);
        /* @hack redundant: the sine times 1, as FLDI1 and FMAC at 0x8c024012
         * multiply it; as the sine alone, 264 of 570 words differ */
        camera_state->pos.y += njSin(*pitch - KNUCKLES_PITCH) * 1.0F;
    }
    /* R and L each move the camera 2 sideways, and call take_short_ease */
    if (pad_records[0].on & PDD_DGT_TR) {
        camera_state->pos.x += njCos(camera_work->ang.y) * 2.0F;
        camera_state->pos.z -= njSin(camera_work->ang.y) * 2.0F;
        take_short_ease();
    }
    if (pad_records[0].on & PDD_DGT_TL) {
        camera_state->pos.x -= njCos(camera_work->ang.y) * 2.0F;
        camera_state->pos.z += njSin(camera_work->ang.y) * 2.0F;
        take_short_ease();
    }
    /* past 60 from player 0 the camera closes in, to no more than 90, and
     * under 35 it backs off, to no nearer than 20. above player 0 it comes no
     * nearer than 30 across */
    vector.x = player_work[0]->pos.x - camera_state->pos.x;
    vector.y = player_work[0]->pos.y - camera_state->pos.y;
    vector.z = player_work[0]->pos.z - camera_state->pos.z;
    distance = njScalor(&vector);
    njUnitVector(&vector);
    if (distance > 60.0F) {
        distance = 60.0F + ((distance - 60.0F) * 0.9F);
        if (distance > 90.0F) {
            distance = 90.0F;
        }
    } else if (distance < 35.0F) {
        distance *= 1.06F;
        if (distance < 20.0F) {
            distance = 20.0F;
        }
    }
    camera_state->pos.x = player_work[0]->pos.x - (vector.x * distance);
    camera_state->pos.y = player_work[0]->pos.y - (vector.y * distance);
    camera_state->pos.z = player_work[0]->pos.z - (vector.z * distance);
    if (camera_state->pos.y > player_work[0]->pos.y) {
        vector.x = camera_state->pos.x - player_work[0]->pos.x;
        vector.z = camera_state->pos.z - player_work[0]->pos.z;
        if ((vector.x * vector.x) + (vector.z * vector.z) < 900.0F) {
            vector.y = 0.0F;
            njUnitVector(&vector);
            camera_state->pos.x = (vector.x * 30.0F) + player_work[0]->pos.x;
            camera_state->pos.z = (vector.z * 30.0F) + player_work[0]->pos.z;
        }
    }
    camera_push_out(&camera_state->pos);
    camera_sweep_unless_c(&previous, &camera_state->pos);
    camera_roll_with_gravity(view);
}

/* give the camera adjust mode RELATIVE1C, a 60-step ease. NONE stays, as
 * do the adjust modes that ease over 60 steps already: THREE1, THREE1C,
 * RELATIVE1 and RELATIVE1C */
static void take_short_ease(void)
{
    switch (camera_get_adjust()) {
    case CAMERA_ADJUST_NORMAL:
    case CAMERA_ADJUST_NORMAL_S:
    case CAMERA_ADJUST_SLOW:
    case CAMERA_ADJUST_SLOW_S:
    case CAMERA_ADJUST_TIME:
    case CAMERA_ADJUST_THREE2:
    case CAMERA_ADJUST_THREE2C:
    case CAMERA_ADJUST_THREE3:
    case CAMERA_ADJUST_THREE3C:
    case CAMERA_ADJUST_THREE4:
    case CAMERA_ADJUST_THREE4C:
    case CAMERA_ADJUST_THREE5:
    case CAMERA_ADJUST_THREE5C:
    case CAMERA_ADJUST_RELATIVE2:
    case CAMERA_ADJUST_RELATIVE2C:
    case CAMERA_ADJUST_RELATIVE3:
    case CAMERA_ADJUST_RELATIVE3C:
    case CAMERA_ADJUST_RELATIVE4:
    case CAMERA_ADJUST_RELATIVE4C:
    case CAMERA_ADJUST_RELATIVE5:
    case CAMERA_ADJUST_RELATIVE5C:
    case CAMERA_ADJUST_RELATIVE6C:
        camera_set_adjust(CAMERA_ADJUST_RELATIVE1C);
        break;
    case CAMERA_ADJUST_THREE1:
    case CAMERA_ADJUST_THREE1C:
    case CAMERA_ADJUST_RELATIVE1:
    case CAMERA_ADJUST_RELATIVE1C:
        break;
    }
}

void camera_step_fishing_point(struct camera_view *view)
{
    NJS_POINT3 previous;
    NJS_VECTOR away;

    previous = camera_work->pos;
    camera_state->target.x = camera_place.target.x;
    camera_state->target.y = camera_place.target.y;
    camera_state->target.z = camera_place.target.z;
    away.x = player_work[0]->pos.x - camera_state->target.x;
    away.y = player_work[0]->pos.y - camera_state->target.y + 20.0F;
    away.z = player_work[0]->pos.z - camera_state->target.z;
    njUnitVector(&away);
    away.x *= 50.0F;
    away.y *= 50.0F;
    away.z *= 50.0F;
    camera_state->pos.x = player_work[0]->pos.x + away.x;
    camera_state->pos.y = player_work[0]->pos.y + away.y + 20.0F;
    camera_state->pos.z = player_work[0]->pos.z + away.z;
    camera_push_out(&camera_state->pos);
    camera_ease(&previous, &camera_state->pos);
    camera_sweep_unless_c(&previous, &camera_state->pos);
    camera_state->ang_z = 0;
}

/* TWO_HARES's step. the camera looks at the middle of camera_place's pos
 * and target. it keeps its direction from that middle, at twice their
 * distance apart */
static void camera_step_two_hares(struct camera_view *view)
{
    NJS_POINT3 position;
    NJS_POINT3 target;
    NJS_VECTOR offset;
    NJS_VECTOR direction;
    Float distance;

    position = camera_place.pos;
    target = camera_place.target;
    camera_state->target.x = (position.x + target.x) * 0.5F;
    camera_state->target.y = (position.y + target.y) * 0.5F;
    camera_state->target.z = (position.z + target.z) * 0.5F;
    offset.x = camera_state->pos.x - camera_state->target.x;
    offset.y = camera_state->pos.y - camera_state->target.y;
    offset.z = camera_state->pos.z - camera_state->target.z;
    direction.x = position.x - target.x;
    direction.y = position.y - target.y;
    direction.z = position.z - target.z;
    distance = njScalor(&direction) * 2.0F;
    njUnitVector(&offset);
    camera_state->pos.x = camera_state->target.x + (offset.x * distance);
    camera_state->pos.y = camera_state->target.y + (offset.y * distance);
    camera_state->pos.z = camera_state->target.z + (offset.z * distance);
}

static void sonic_shared_step(void);
static void camera_target_follow(void);
static void camera_pitch_ease(void);
static void sonic_slide(void);
static void sonic_keep_distance(void);
static void sonic_slide_if_blocked(void);
static void sonic_push_and_sweep(void);
static void sonic_drift_move(void);

/* SONIC_P's step: sonic_shared_step, on numbers from the view. target.x,
 * y and z give sonic_far, sonic_near and sonic_far_limit, and distance
 * gives sonic_near_limit. ang_x gives the pitch, and an ang_y other than 0
 * turns the drift on */
static void camera_step_sonic_p(struct camera_view *view)
{
    if (!view->steps) {
        sonic_state = SONIC_START;
    }
    sonic_far = view->target.x;
    sonic_near = view->target.y;
    sonic_far_limit = view->target.z;
    sonic_near_limit = view->distance;
    sonic_pitch = (Uint16)view->ang_x;
    sonic_drift = (Uint16)view->ang_y;
    sonic_shared_step();
}

/* SONIC's step: sonic_shared_step at camera_step_follow's distances and
 * KNUCKLES_PITCH, without the drift */
static void camera_step_sonic(struct camera_view *view)
{
    if (!view->steps) {
        sonic_state = SONIC_START;
    }
    sonic_far = 60.0F;
    sonic_near = 35.0F;
    sonic_far_limit = 90.0F;
    sonic_near_limit = 20.0F;
    sonic_pitch = KNUCKLES_PITCH;
    sonic_drift = 0;
    sonic_shared_step();
}

/* the step both SONIC modes run. the target eases ahead of player 0. the
 * camera eases its pitch toward sonic_pitch and keeps its distance from
 * him. the triggers or sonic_slide_steps slide it sideways. a sweep from it
 * to his eyes that hits sets the slide steps. while camera_player_still
 * holds, the drift moves it toward y angle 0, till a slide moves it */
static void sonic_shared_step(void)
{
    switch (sonic_state) {
    case SONIC_START:
        sonic_slide_steps = 0;
        camera_state->target.x = player_work[0]->pos.x;
        camera_state->target.y = player_work[0]->pos.y;
        camera_state->target.z = player_work[0]->pos.z;
        sonic_state = SONIC_FOLLOW;
        /* fall through */
    case SONIC_FOLLOW:
        sonic_last_pos = camera_work->pos;
        camera_target_follow();
        camera_pitch_ease();
        sonic_slide();
        sonic_keep_distance();
        sonic_slide_if_blocked();
        sonic_push_and_sweep();
        if (camera_player_still == 1) {
            sonic_state = SONIC_DRIFT;
        }
        break;
    case SONIC_DRIFT:
        sonic_last_pos = camera_work->pos;
        camera_target_follow();
        camera_pitch_ease();
        sonic_slide();
        sonic_drift_move();
        sonic_keep_distance();
        sonic_slide_if_blocked();
        sonic_push_and_sweep();
        if (sonic_slide_steps || !sonic_triggers_idle) {
            sonic_state = SONIC_DRIFTED;
        }
        if (!camera_player_still) {
            sonic_state = SONIC_FOLLOW;
        }
        break;
    case SONIC_DRIFTED:
        sonic_last_pos = camera_work->pos;
        camera_target_follow();
        camera_pitch_ease();
        sonic_slide();
        sonic_keep_distance();
        sonic_slide_if_blocked();
        sonic_push_and_sweep();
        if (!camera_player_still) {
            sonic_state = SONIC_FOLLOW;
        }
        break;
    }
}

/* ease the camera's target a tenth of the way to 8 steps of player 0's
 * speed ahead of his eyes */
static void camera_target_follow(void)
{
    NJS_POINT3 aim;

    aim = player_work[0]->pos;
    aim.y += player_get(0)->consts.eye_height;
    aim.x += player_motion[0]->speed.x * 8.0F;
    aim.y += player_motion[0]->speed.y * 8.0F;
    aim.z += player_motion[0]->speed.z * 8.0F;
    camera_state->target.x += (aim.x - camera_state->target.x) * 0.1F;
    camera_state->target.y += (aim.y - camera_state->target.y) * 0.1F;
    camera_state->target.z += (aim.z - camera_state->target.z) * 0.1F;
}

/* while camera_work's pitch is more than PITCH_SLACK off sonic_pitch, turn
 * it PITCH_EASE toward sonic_pitch. a camera below player 0 then comes up
 * to his height. otherwise it rises or falls by the sine of how far the
 * turned pitch lies from sonic_pitch. the turned pitch itself is lost when
 * camera_place_look_at places the camera */
static void camera_pitch_ease(void)
{
    /* @hack spelling: camera_work's x angle through ang_x, which keeps its
     * address in R14 (0x8c02484e); by name, 52 of 62 words differ and the
     * code runs 32 bytes long */
    Angle *ang_x = &camera_work->ang.x;
    Sint16 diff;

    diff = angle_difference(sonic_pitch, *ang_x);
    if (diff < 0) {
        diff = -diff;
    }
    if (diff > PITCH_SLACK) {
        *ang_x = angle_approach(*ang_x, sonic_pitch, PITCH_EASE);
        if (camera_state->pos.y < player_work[0]->pos.y) {
            camera_state->pos.y = player_work[0]->pos.y;
        } else {
            /* @hack redundant: the sine times 1, as FLDI1 and FMAC at
             * 0x8c0248ac multiply it; as the sine alone, 56 of 62 words
             * differ */
            camera_state->pos.y += njSin(*ang_x - sonic_pitch) * 1.0F;
        }
    }
}

/* slide the camera 2 sideways: one way for R or slide steps above 0, and
 * the other for L or slide steps below 0. each slide step used raises it
 * 0.25. sonic_triggers_idle ends set unless a trigger slid it with no slide
 * steps left */
static void sonic_slide(void)
{
    sonic_triggers_idle = 1;
    if ((pad_records[0].on & PDD_DGT_TR) || sonic_slide_steps > 0) {
        camera_state->pos.x += njCos(camera_work->ang.y) * 2.0F;
        camera_state->pos.z -= njSin(camera_work->ang.y) * 2.0F;
        if (sonic_slide_steps) {
            sonic_slide_steps--;
            camera_state->pos.y += 0.25F;
        } else {
            sonic_triggers_idle = 0;
        }
    }
    if ((pad_records[0].on & PDD_DGT_TL) || sonic_slide_steps < 0) {
        camera_state->pos.x -= njCos(camera_work->ang.y) * 2.0F;
        camera_state->pos.z += njSin(camera_work->ang.y) * 2.0F;
        if (sonic_slide_steps) {
            sonic_slide_steps++;
            camera_state->pos.y += 0.25F;
        } else {
            sonic_triggers_idle = 0;
        }
    }
}

/* past sonic_far from player 0 the camera closes in, to no more than
 * sonic_far_limit. under sonic_near it backs off, to no nearer than
 * sonic_near_limit. above player 0 it comes no nearer than 30 across */
static void sonic_keep_distance(void)
{
    NJS_VECTOR to_player;
    Float distance;

    to_player.x = player_work[0]->pos.x - camera_state->pos.x;
    to_player.y = player_work[0]->pos.y - camera_state->pos.y;
    to_player.z = player_work[0]->pos.z - camera_state->pos.z;
    distance = njScalor(&to_player);
    njUnitVector(&to_player);
    if (distance > sonic_far) {
        distance = sonic_far + ((distance - sonic_far) * 0.9F);
        if (distance > sonic_far_limit) {
            distance = sonic_far_limit;
        }
    } else if (distance < sonic_near) {
        distance *= 1.06F;
        if (distance < sonic_near_limit) {
            distance = sonic_near_limit;
        }
    }
    camera_state->pos.x = player_work[0]->pos.x - (to_player.x * distance);
    camera_state->pos.y = player_work[0]->pos.y - (to_player.y * distance);
    camera_state->pos.z = player_work[0]->pos.z - (to_player.z * distance);
    if (camera_state->pos.y > player_work[0]->pos.y) {
        to_player.x = camera_state->pos.x - player_work[0]->pos.x;
        to_player.z = camera_state->pos.z - player_work[0]->pos.z;
        if ((to_player.x * to_player.x) + (to_player.z * to_player.z) <
            900.0F) {
            to_player.y = 0.0F;
            njUnitVector(&to_player);
            camera_state->pos.x = (to_player.x * 30.0F) + player_work[0]->pos.x;
            camera_state->pos.z = (to_player.z * 30.0F) + player_work[0]->pos.z;
        }
    }
}

/* with no slide steps left and sonic_triggers_idle set, sweep from the
 * camera toward player 0's eyes. a hit sets 30 slide steps, or -30 where
 * the turn from the direct way to the swept way is negative */
static void sonic_slide_if_blocked(void)
{
    NJS_VECTOR offset;
    NJS_VECTOR vector;
    NJS_POINT3 position;

    if (sonic_slide_steps) {
        return;
    }
    if (!sonic_triggers_idle) {
        return;
    }
    position.x = camera_state->pos.x;
    position.y = camera_state->pos.y;
    position.z = camera_state->pos.z;
    offset.x = player_work[0]->pos.x - camera_state->pos.x;
    offset.y = player_work[0]->pos.y + player_get(0)->consts.eye_height -
               camera_state->pos.y;
    offset.z = player_work[0]->pos.z - camera_state->pos.z;
    vector = offset;
    if (!camera_sweep_hits(&position, &vector, NULL, 4.0F)) {
        return;
    }
    if ((angle_difference(
             (Angle)(atan2f(offset.z, offset.x) * ANGLE_PER_RADIAN),
             (Angle)(atan2f(vector.z, vector.x) * ANGLE_PER_RADIAN)) &
         0xffff) > 0x8000) {
        sonic_slide_steps = -30;
    } else {
        sonic_slide_steps = 30;
    }
}

/* push the camera out of the act's spheres and blocks, and sweep its move
 * from where it was */
static void sonic_push_and_sweep(void)
{
    camera_push_out(&camera_state->pos);
    camera_sweep_unless_c(&sonic_last_pos, &camera_state->pos);
}

/* the drift. with sonic_drift set and camera_work's y angle more than
 * DRIFT_SLACK from 0, move the camera 0.5 sideways toward y angle 0 */
static void sonic_drift_move(void)
{
    Angle distance = angle_distance(0, camera_work->ang.y);

    if (sonic_drift && distance > DRIFT_SLACK) {
        if ((Sint16)angle_difference(0, camera_work->ang.y) > 0) {
            camera_state->pos.x -= njCos(camera_work->ang.y) * 0.5F;
            camera_state->pos.z += njSin(camera_work->ang.y) * 0.5F;
        } else {
            camera_state->pos.x += njCos(camera_work->ang.y) * 0.5F;
            camera_state->pos.z -= njSin(camera_work->ang.y) * 0.5F;
        }
    }
}

/* FIXED's step. the camera sits at the view's pos and looks at its target.
 * camera_roll_with_gravity rolls it */
static void camera_step_fixed_rolled(struct camera_view *view)
{
    camera_state->target.x = view->target.x;
    camera_state->target.y = view->target.y;
    camera_state->target.z = view->target.z;
    camera_state->pos.x = view->pos.x;
    camera_state->pos.y = view->pos.y;
    camera_state->pos.z = view->pos.z;
    camera_state->ang_z = 0;
    camera_roll_with_gravity(view);
}

void camera_step_none(struct camera_view *view)
{
}

/* NEWFOLLOW's step. the camera looks at player 0's eyes from the view's
 * distance, at KNUCKLES_PITCH. R and L turn it 0x222 a step.
 * camera_sweep_unless_c sweeps its move */
static void camera_step_newfollow(struct camera_view *view)
{
    NJS_POINT3 previous;

    previous = camera_work->pos;
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y =
        player_work[0]->pos.y + player_get(0)->consts.eye_height;
    camera_state->target.z = player_work[0]->pos.z;
    if (pad_records[0].on & PDD_DGT_TR) {
        camera_state->ang_y += 0x222;
    } else if (pad_records[0].on & PDD_DGT_TL) {
        camera_state->ang_y -= 0x222;
    }
    camera_state->distance = view->distance;
    camera_state->ang_x = KNUCKLES_PITCH;
    camera_place_from_target(camera_work);
    camera_state->pos.x = camera_work->pos.x;
    camera_state->pos.y = camera_work->pos.y;
    camera_state->pos.z = camera_work->pos.z;
    camera_state->ang_x = camera_work->ang.x;
    camera_state->ang_y = camera_work->ang.y;
    camera_state->ang_z = 0;
    if (!camera_adjust_is_c()) {
        camera_sweep_unless_c(&previous, &camera_state->pos);
    }
}

/* LEAVE's step: the camera's target is player 0 */
static void camera_step_leave(struct camera_view *view)
{
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y;
    camera_state->target.z = player_work[0]->pos.z;
}

void camera_step_work_view(struct camera_view *view)
{
    camera_state->pos.x = camera_work->scl.x;
    camera_state->pos.y = camera_work->scl.y;
    camera_state->pos.z = camera_work->scl.z;
    camera_state->target.x = CAMERA_VARS(camera_work)->target.x;
    camera_state->target.y = CAMERA_VARS(camera_work)->target.y;
    camera_state->target.z = CAMERA_VARS(camera_work)->target.z;
    camera_state->ang_z = 0;
}

/* FOLLOW_G's step: FOLLOW's, at follow_g_distance, with its angles from
 * world_gravity. while gravity has no x or z, the pitch is MANUAL_PITCH.
 * otherwise the y angle takes the lean's heading, and the pitch tips down
 * further by the lean */
static void camera_step_follow_g(struct camera_view *view)
{
    /* @hack spelling: a Float cursor stepped across world_gravity's members,
     * for the image's @Rn+ loads (0x8c0250e4, 0x8c0250e6); as
     * world_gravity.x, .y and .z, 150 of 182 words differ */
    Float *gravity = &world_gravity.x;
    Float gx;
    Float gy;
    Float gz;
    Float across;
    Angle ang_x;
    Angle ang_y;

    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y +
                             player_get(0)->consts.eye_height +
                             (camera_state->distance * RISE);
    camera_state->target.z = player_work[0]->pos.z;
    camera_state->distance = follow_g_distance;
    gx = *gravity++;
    gy = *gravity++;
    gz = *gravity;
    if (gx == 0.0F && gz == 0.0F) {
        camera_state->ang_x = MANUAL_PITCH;
    } else {
        across = njSqrt((gx * gx) + (gz * gz));
        ang_y = (Angle)(atan2f(gx, gz) * ANGLE_PER_RADIAN);
        ang_x = (Angle)(atan2f(across, -gy) * ANGLE_PER_RADIAN);
        camera_state->ang_x = MANUAL_PITCH - ang_x;
        camera_state->ang_y = ang_y;
    }
    camera_state->ang_z = 0;
    if (PLAYER_VARS(player_work[0])->character == CHARACTER_KNUCKLES &&
        player_work[0]->mode == KNUCKLES_PUSH) {
        camera_state->ang_y +=
            angle_difference(camera_state->ang_y,
                             0xc000 - player_work[0]->ang.y) >>
            2;
    } else if (pad_records[0].on & PDD_DGT_TY) {
        if (pad_records[0].on & PDD_DGT_TL) {
            follow_g_distance += 2.0F;
        } else if (pad_records[0].on & PDD_DGT_TR) {
            follow_g_distance -= 2.0F;
        }
    } else if (pad_records[0].on & PDD_DGT_TL) {
        camera_state->flags |= CAMERA_TURNED;
        camera_state->ang_y -= 0x200;
    } else if (pad_records[0].on & PDD_DGT_TR) {
        camera_state->flags |= CAMERA_TURNED;
        camera_state->ang_y += 0x200;
    }
}

void camera_set_e103(struct task_work *work, NJS_POINT3 *points)
{
    e103_target = work;
    e103_line = points;
}

/* E103's step. the camera looks at the middle of player 0 and e103_target.
 * it sits toward the point of e103_line nearest that middle. its distance
 * from the middle is how far the two are apart, held to 100 to 350. it
 * keeps at least 14 above player 0 */
static void camera_step_e103(struct camera_view *view)
{
    NJS_VECTOR half;
    NJS_POINT3 point;
    NJS_LINE line;
    NJS_POINT3 *player = &player_work[0]->pos;
    Float distance;

    half.x = (e103_target->pos.x - player->x) * 0.5F;
    half.y = (e103_target->pos.y - player->y) * 0.5F;
    half.z = (e103_target->pos.z - player->z) * 0.5F;
    camera_state->target.x = player->x + half.x;
    camera_state->target.z = player->z + half.z;
    camera_state->target.y = player->y + half.y;
    line.px = e103_line[0].x;
    line.py = e103_line[0].y;
    line.pz = e103_line[0].z;
    line.vx = e103_line[1].x - e103_line[0].x;
    line.vy = e103_line[1].y - e103_line[0].y;
    line.vz = e103_line[1].z - e103_line[0].z;
    njDistanceP2L(&camera_state->target, &line, &point);
    distance = njDistanceP2P(&e103_target->pos, &player_work[0]->pos);
    if (distance < 100.0F) {
        distance = 100.0F;
    } else if (distance > 350.0F) {
        distance = 350.0F;
    }
    point.x -= camera_state->target.x;
    point.y -= camera_state->target.y;
    point.z -= camera_state->target.z;
    njUnitVector(&point);
    /* @hack spelling: the point through its address, which SHC forms again for
     * each load and store; as point.x *= distance and the rest, 128 of 242
     * words and 30 pool words differ, and the code runs 32 bytes short */
    VECTOR_SCALE(&point, &point, distance);
    point.x += camera_state->target.x;
    point.y += camera_state->target.y;
    point.z += camera_state->target.z;
    camera_state->pos.x = point.x;
    camera_state->pos.y = point.y;
    camera_state->pos.z = point.z;
    if (camera_state->pos.y <= player->y + 14.0F) {
        camera_state->pos.y = player->y + 14.0F;
    }
}

/* an empty function no code calls. it isn't static, since SHC would leave
 * a static out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void fun_8c02548c(void)
{
}

/* NONE's step leaves the camera as the mode put it */
void camera_adjust_none(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust)
{
    Float dx;
    Float dy;
    Float dz;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float distance;

    dx = work->pos.x - last->pos.x;
    dy = work->pos.y - last->pos.y;
    dz = work->pos.z - last->pos.z;
    /* @hack redundant: the distance kept in a local that nothing reads; as
     * a bare call, njSqrt's becomes a tail call and 25 of 30 words differ */
    distance = njSqrt((dx * dx) + (dy * dy) + (dz * dz));
}

/* NORMAL's step. each angle turns a quarter of the way from last's to
 * work's. the turn passes the last step's turn by at most 64 for y and 128
 * for x. it stays within 0x100 for y and 0x200 for x. an angle of work's
 * within 64 for y, or 128 for x, of last's is taken whole. a step that
 * CAMERA_TURNED marks leaves y as the mode set it. the camera then looks at
 * 15 above player 0. its distance is last's from where he was two steps
 * back, eased 0.3 of the way to work's from him, and at least 20 */
void camera_adjust_normal(struct task_work *work, struct task_work *last,
                          struct camera_adjust_work *adjust)
{
    Float player_distance;
    Float distance_gap;
    NJS_POINT3 past;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 position;
    Float distance;
    Angle diff;
    Angle turn;
    Angle limit;

    /* @hack redundant: work's position copied to position, which nothing
     * reads (0x8c0254e2 to 0x8c0254ee); without the copy, 218 of 223 words
     * differ and the code runs 32 bytes short */
    position = work->pos;
    player_distance = njDistanceP2P(&work->pos, &player_work[0]->pos);
    player_get_history(0, 2, &past, NULL);
    distance = njDistanceP2P(&last->pos, &past);
    distance_gap = player_distance - distance;
    distance += distance_gap * 0.3F;
    if (distance < 20.0F) {
        distance = 20.0F;
    }
    /* @hack redundant: four stores of 0 that never run, which keep the 0
     * the step stores in R6 (0x8c02552a), so the prologue saves no R11;
     * without them, 118 of 223 words differ */
    if (0) {
        adjust->turn_x = 0;
        adjust->turn_y = 0;
        adjust->turn_x = 0;
        adjust->turn_y = 0;
    }
    if (camera_state->flags & CAMERA_TURNED) {
        camera_state->flags &= ~CAMERA_TURNED;
    } else {
        diff = work->ang.y - last->ang.y;
        if (diff & 0x8000) {
            diff |= 0xffff0000;
        } else {
            diff = (Uint16)diff;
        }
        if (diff > -64 && diff < 64) {
            camera_state->ang_y = work->ang.y;
            adjust->turn_y = 0;
            adjust->flags &= ~0x40;
        } else {
            turn = diff >> 2;
            if (diff < 0) {
                limit = adjust->turn_y - 64;
                /* @hack redundant: stores of the limit that never run, here and
                 * in the two arms below, which keep the limit in R7 and the
                 * quarter turn in R0 (0x8c0255ca, 0x8c0255c0); without them,
                 * 34 of 223 words differ */
                if (0) {
                    adjust->turn_y = limit;
                    adjust->turn_y = limit;
                }
                if (turn > limit) {
                    diff = turn;
                } else {
                    diff = limit;
                }
                if (diff <= -0x100) {
                    diff = -0x100;
                } else {
                    /* @hack redundant: an empty else, here and in the two arms
                     * below, whose jump leaves the next load of last's angle
                     * in R2 and camera_state's address in R3 (0x8c0255e2,
                     * 0x8c0255e4); without them, 12 of 223 words differ */
                }
                adjust->turn_y = diff;
                camera_state->ang_y = diff + last->ang.y;
            } else {
                limit = adjust->turn_y + 64;
                /* @hack redundant: two stores of the limit that never run,
                 * which keep the next load of last's angle in R2 and
                 * camera_state's address in R3 (0x8c025606, 0x8c025608);
                 * without them, the two trade and 4 of 223 words differ */
                if (0) {
                    adjust->turn_y = limit;
                    adjust->turn_y = limit;
                }
                if (turn < limit) {
                    diff = turn;
                } else {
                    diff = limit;
                }
                if (diff >= 0x100) {
                    diff = 0x100;
                } else {
                }
                adjust->turn_y = diff;
                camera_state->ang_y = diff + last->ang.y;
            }
        }
    }
    diff = work->ang.x - last->ang.x;
    if (diff & 0x8000) {
        diff |= 0xffff0000;
    } else {
        diff = (Uint16)diff;
    }
    if (diff > -128 && diff < 128) {
        camera_state->ang_x = work->ang.x;
        adjust->turn_x = 0;
        adjust->flags &= ~0x20;
    } else {
        turn = diff >> 2;
        if (diff < 0) {
            limit = adjust->turn_x - 128;
            /* @hack redundant: three stores of the limit that never run, as in
             * the arms above; without them, 17 of 223 words differ */
            if (0) {
                adjust->turn_x = limit;
                adjust->turn_x = limit;
                adjust->turn_x = limit;
            }
            if (turn > limit) {
                diff = turn;
            } else {
                diff = limit;
            }
            if (diff <= -0x200) {
                diff = -0x200;
            } else {
            }
            adjust->turn_x = diff;
            camera_state->ang_x = diff + last->ang.x;
        } else {
            limit = adjust->turn_x + 128;
            if (turn < limit) {
                diff = turn;
            } else {
                diff = limit;
            }
            if (diff >= 0x200) {
                diff = 0x200;
            }
            adjust->turn_x = diff;
            camera_state->ang_x = diff + last->ang.x;
        }
    }
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y + 15.0F;
    camera_state->target.z = player_work[0]->pos.z;
    camera_state->distance = distance;
    camera_state->ang_z = 0;
    camera_place_from_target(work);
}

/* SLOW's step. the angles turn as NORMAL's do, by other bounds. the turn
 * passes the last step's turn by at most 0x180 for y and 32 for x. only the
 * x turn stays within 0x200. an angle of work's within 0x180 for y, or 32
 * for x, of last's is taken whole. the camera then moves from last toward
 * work by adjust's move. the gap is the distance from last to work less the
 * last move. while the gap lies within 0.7 of 0, the camera takes work's
 * place and the move goes back to 0. a gap above 0 adds 0.7 to the move,
 * which is then held to at most a quarter of the gap and 80. a gap below 0
 * takes 0.7 off it, and holds it to at least a quarter of the gap and -80 */
void camera_adjust_slow(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust)
{
    Float dx;
    Float dy;
    Float dz;
    Float distance;
    Float move;
    Float change;
    Float step;
    Angle diff;
    Angle turn;
    Angle limit;
    Angle quarter;

    if (camera_state->flags & CAMERA_TURNED) {
        camera_state->flags &= ~CAMERA_TURNED;
    } else {
        diff = work->ang.y - last->ang.y;
        diff = (diff & 0x8000) ? (diff | 0xffff0000) : (Uint16)diff;
        if (diff > -0x180 && diff < 0x180) {
            camera_state->ang_y = work->ang.y;
            adjust->turn_y = 0;
            adjust->flags &= ~0x40;
        } else {
            /* @hack spelling: the quarter turn copied into turn, here and in
             * x's half, which keeps the turn in R0 and the 0 in R9, which the
             * prologue saves (0x8c0257be, 0x8c025760, 0x8c02574c); with turn
             * = diff >> 2, the 0 takes R10 and 67 of 280 words differ */
            quarter = diff >> 2;
            turn = quarter;
            if (diff < 0) {
                limit = adjust->turn_y - 0x180;
                if (turn > limit) {
                    diff = turn;
                } else {
                    diff = limit;
                }
                adjust->turn_y = diff;
                camera_state->ang_y = diff + last->ang.y;
            } else {
                limit = adjust->turn_y + 0x180;
                if (turn < limit) {
                    diff = turn;
                } else {
                    diff = limit;
                }
                adjust->turn_y = diff;
                camera_state->ang_y = diff + last->ang.y;
            }
        }
    }
    diff = work->ang.x - last->ang.x;
    diff = (diff & 0x8000) ? (diff | 0xffff0000) : (Uint16)diff;
    if (diff > -32 && diff < 32) {
        camera_state->ang_x = work->ang.x;
        adjust->turn_x = 0;
        adjust->flags &= ~0x20;
    } else {
        quarter = diff >> 2;
        turn = quarter;
        if (diff < 0) {
            limit = adjust->turn_x - 32;
            if (turn > limit) {
                diff = turn;
            } else {
                diff = limit;
            }
            /* @hack redundant: a test of diff that never runs, which keeps
             * change in FR5 and step in FR7 (0x8c02590c, 0x8c025986); without
             * it, the two trade and 11 of 280 words differ */
            if (0) {
                if (diff) {
                }
            }
            if (diff <= -0x200) {
                diff = -0x200;
            }
            adjust->turn_x = diff;
            camera_state->ang_x = diff + last->ang.x;
        } else {
            limit = adjust->turn_x + 32;
            if (turn < limit) {
                diff = turn;
            } else {
                diff = limit;
            }
            if (diff >= 0x200) {
                diff = 0x200;
            }
            adjust->turn_x = diff;
            camera_state->ang_x = diff + last->ang.x;
        }
    }
    dx = work->pos.x - last->pos.x;
    dy = work->pos.y - last->pos.y;
    dz = work->pos.z - last->pos.z;
    distance = njSqrt((dx * dx) + (dy * dy) + (dz * dz));
    move = adjust->move;
    change = distance - move;
    if (change < 0.7F && change > -0.7F) {
        camera_state->pos.x = work->pos.x;
        camera_state->pos.y = work->pos.y;
        camera_state->pos.z = work->pos.z;
        adjust->move = 0.0F;
    } else {
        step = change * 0.25F;
        if (change < 0.0F) {
            move += -0.7F;
            if (move < step) {
                move = step;
            }
            if (move < -80.0F) {
                move = -80.0F;
            }
        } else {
            move += 0.7F;
            if (move > step) {
                move = step;
            }
            if (move > 80.0F) {
                move = 80.0F;
            }
        }
        adjust->move = move;
        /* @hack spelling: 0 on the left, which keeps FCMP/EQ FR6,FR3's operands
         * (0x8c0259ba); written distance == 0.0F, 1 of 280 words differs */
        if (0.0F == distance) {
            camera_state->pos.x = last->pos.x;
            camera_state->pos.y = last->pos.y;
            camera_state->pos.z = last->pos.z;
        } else {
            move /= distance;
            camera_state->pos.x = last->pos.x + (dx * move);
            camera_state->pos.y = last->pos.y + (dy * move);
            camera_state->pos.z = last->pos.z + (dz * move);
        }
    }
    camera_state->ang_z = 0;
    camera_place_at_angles(work);
}

/* TIME's step. each step the camera goes a share of the way from where it
 * was to the mode's place and angles. the share is adjust's steps over 120.
 * at 120 steps it arrives, and the adjust mode goes to NONE.
 * camera_sweep_move stops its move at walls */
void camera_adjust_time(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust)
{
    Float part;

    part = (Float)adjust->steps * (1.0F / 120.0F);
    if (part >= 1.0F) {
        part = 1.0F;
        camera_set_adjust(CAMERA_ADJUST_NONE);
    }
    work->pos.x = ((work->pos.x - last->pos.x) * part) + last->pos.x;
    work->pos.y = ((work->pos.y - last->pos.y) * part) + last->pos.y;
    work->pos.z = ((work->pos.z - last->pos.z) * part) + last->pos.z;
    work->ang.x = angle_approach(
        last->ang.x, work->ang.x,
        (Angle)(angle_distance(work->ang.x, last->ang.x) * part));
    work->ang.y = angle_approach(
        last->ang.y, work->ang.y,
        (Angle)(angle_distance(work->ang.y, last->ang.y) * part));
    work->ang.z = angle_approach(
        last->ang.z, work->ang.z,
        (Angle)(angle_distance(work->ang.z, last->ang.z) * part));
    camera_sweep_move(&last->pos, &work->pos);
}

static void adjust_ease_start(struct task_work *work, struct task_work *last,
                              struct camera_adjust_work *adjust);
static void adjust_ease_aim(struct task_work *work, struct task_work *last,
                            struct camera_adjust_work *adjust);
static void adjust_ease_follow(struct task_work *work, struct task_work *last,
                               struct camera_adjust_work *adjust);
static void adjust_ease_trail(struct task_work *work, struct task_work *last,
                              struct camera_adjust_work *adjust);
static void adjust_ease_turn(struct task_work *work, struct task_work *last,
                             struct camera_adjust_work *adjust);
static void adjust_ease_limit(struct task_work *work, struct task_work *last,
                              struct camera_adjust_work *adjust);

/* the THREE and RELATIVE modes' step. each step the camera goes ease_part
 * of the way from where it was to the mode's place and angles. once
 * ease_part reaches 1, the adjust mode goes to NONE. the THREE modes ease
 * the place with a lead, and the RELATIVE modes ease it about player 0. the
 * x and y angles stay within ease_limit of ease_aim. a C mode's ease stops
 * the move at walls */
void camera_adjust_ease(struct task_work *work, struct task_work *last,
                        struct camera_adjust_work *adjust)
{
    adjust_ease_aim(work, last, adjust);
    if (adjust->steps == 0) {
        adjust_ease_start(work, last, adjust);
    }
    ease_part = (Float)adjust->steps / ease_steps;
    if (ease_part >= 1.0F) {
        ease_part = 1.0F;
        camera_set_adjust(CAMERA_ADJUST_NONE);
    }
    ease_limit.x =
        ((EASE_LIMIT_X - ease_off_aim.x) * ease_part) + ease_off_aim.x;
    ease_limit.y =
        ((EASE_LIMIT_Y - ease_off_aim.y) * ease_part) + ease_off_aim.y;
    if (camera_get_adjust() <= CAMERA_ADJUST_THREE5C) {
        adjust_ease_follow(work, last, adjust);
    } else {
        adjust_ease_trail(work, last, adjust);
    }
    adjust_ease_turn(work, last, adjust);
    adjust_ease_limit(work, last, adjust);
    if (ease_sweeps) {
        camera_sweep_move(&last->pos, &work->pos);
    }
}

/* start the ease. keep how far last's angles lie off the aim, the camera's
 * move the step before, and the adjust mode's entry of eases */
static void adjust_ease_start(struct task_work *work, struct task_work *last,
                              struct camera_adjust_work *adjust)
{
    /* the eases of the adjust modes from THREE1 on, by number. a C mode's
     * ease sweeps */
    struct adjust_ease eases[21] = {
        {0, 60.0F, 0},     {1, 60.0F, 0},    {0, 180.0F, 3},   {1, 180.0F, 3},
        {0, 600.0F, 10},   {1, 600.0F, 10},  {0, 1800.0F, 30}, {1, 1800.0F, 30},
        {0, 3600.0F, 60},  {1, 3600.0F, 60}, {0, 60.0F, 0},    {1, 60.0F, 0},
        {0, 180.0F, 3},    {1, 180.0F, 3},   {0, 600.0F, 10},  {1, 600.0F, 10},
        {0, 1800.0F, 30},  {1, 1800.0F, 30}, {0, 3600.0F, 60}, {1, 3600.0F, 60},
        {1, 30000.0F, 60},
    };

    ease_off_aim.x = angle_distance(last->ang.x, ease_aim.x);
    ease_off_aim.y = angle_distance(last->ang.y, ease_aim.y);
    ease_start_move = camera_moved;
    ease_sweeps =
        eases[(unsigned char)(camera_get_adjust() - CAMERA_ADJUST_THREE1)]
            .sweeps;
    ease_steps =
        eases[(unsigned char)(camera_get_adjust() - CAMERA_ADJUST_THREE1)]
            .steps;
    ease_blend =
        eases[(unsigned char)(camera_get_adjust() - CAMERA_ADJUST_THREE1)]
            .blend;
    dat_8c6c0e68 = 0;
}

/* set ease_aim to the angles from where the camera was to player 0 */
static void adjust_ease_aim(struct task_work *work, struct task_work *last,
                            struct camera_adjust_work *adjust)
{
    NJS_POINT3 from_player;

    from_player.x = last->pos.x - player_work[0]->pos.x;
    from_player.z = last->pos.z - player_work[0]->pos.z;
    ease_aim.y =
        (Angle)(atan2f(from_player.x, from_player.z) * ANGLE_PER_RADIAN);
    ease_aim.x = (Angle)(atan2f(player_work[0]->pos.y - last->pos.y,
                                njSqrt((from_player.x * from_player.x) +
                                       (from_player.z * from_player.z))) *
                         ANGLE_PER_RADIAN);
}

/* move the camera ease_part of the way from where it was to the mode's
 * place, moved by a lead. the lead is a fifth of the camera's offset from
 * player 0, and shrinks to nothing as the ease ends. over the ease's first
 * ease_blend steps, the camera's move from before the ease blends out */
static void adjust_ease_follow(struct task_work *work, struct task_work *last,
                               struct camera_adjust_work *adjust)
{
    NJS_POINT3 lead;
    NJS_POINT3 eased;

    lead.x = (last->pos.x - player_work[0]->pos.x) * (1.0F - ease_part) * 0.2F;
    lead.y = (last->pos.y - player_work[0]->pos.y) * (1.0F - ease_part) * 0.2F;
    lead.z = (last->pos.z - player_work[0]->pos.z) * (1.0F - ease_part) * 0.2F;
    if (adjust->steps < ease_blend) {
        eased.x = (work->pos.x - last->pos.x + lead.x) * ease_part;
        eased.y = (work->pos.y - last->pos.y + lead.y) * ease_part;
        eased.z = (work->pos.z - last->pos.z + lead.z) * ease_part;
        work->pos.x = last->pos.x +
                      (((eased.x * adjust->steps) +
                        (ease_start_move.x * (ease_blend - adjust->steps))) /
                       ease_blend);
        work->pos.y = last->pos.y +
                      (((eased.y * adjust->steps) +
                        (ease_start_move.y * (ease_blend - adjust->steps))) /
                       ease_blend);
        work->pos.z = last->pos.z +
                      (((eased.z * adjust->steps) +
                        (ease_start_move.z * (ease_blend - adjust->steps))) /
                       ease_blend);
    } else {
        work->pos.x =
            last->pos.x + ((work->pos.x - last->pos.x + lead.x) * ease_part);
        work->pos.y =
            last->pos.y + ((work->pos.y - last->pos.y + lead.y) * ease_part);
        work->pos.z =
            last->pos.z + ((work->pos.z - last->pos.z + lead.z) * ease_part);
    }
}

/* move the camera about player 0. its offset from him goes ease_part of the
 * way from the offset it had last step to the mode's */
static void adjust_ease_trail(struct task_work *work, struct task_work *last,
                              struct camera_adjust_work *adjust)
{
    NJS_POINT3 past;
    NJS_POINT3 lag;
    NJS_POINT3 offset;

    player_get_history(0, 1, &past, NULL);
    lag.x = last->pos.x - past.x;
    lag.y = last->pos.y - past.y;
    lag.z = last->pos.z - past.z;
    offset.x = work->pos.x - player_work[0]->pos.x;
    offset.y = work->pos.y - player_work[0]->pos.y;
    offset.z = work->pos.z - player_work[0]->pos.z;
    work->pos.x =
        lag.x + ((offset.x - lag.x) * ease_part) + player_work[0]->pos.x;
    work->pos.y =
        lag.y + ((offset.y - lag.y) * ease_part) + player_work[0]->pos.y;
    work->pos.z =
        lag.z + ((offset.z - lag.z) * ease_part) + player_work[0]->pos.z;
}

/* turn each of the camera's angles ease_part of the way from where it was
 * to the mode's */
static void adjust_ease_turn(struct task_work *work, struct task_work *last,
                             struct camera_adjust_work *adjust)
{
    work->ang.x = angle_approach(
        last->ang.x, work->ang.x,
        (Angle)(angle_distance(work->ang.x, last->ang.x) * ease_part));
    work->ang.y = angle_approach(
        last->ang.y, work->ang.y,
        (Angle)(angle_distance(work->ang.y, last->ang.y) * ease_part));
    work->ang.z = angle_approach(
        last->ang.z, work->ang.z,
        (Angle)(angle_distance(work->ang.z, last->ang.z) * ease_part));
}

/* keep the camera's x and y angles within ease_limit of the aim */
static void adjust_ease_limit(struct task_work *work, struct task_work *last,
                              struct camera_adjust_work *adjust)
{
    if (angle_distance(work->ang.x, ease_aim.x) > ease_limit.x) {
        work->ang.x = angle_approach(ease_aim.x, work->ang.x, ease_limit.x);
    }
    if (angle_distance(work->ang.y, ease_aim.y) > ease_limit.y) {
        work->ang.y = angle_approach(ease_aim.y, work->ang.y, ease_limit.y);
    }
}

/* the modes, by number. the comment over each group names its entries in
 * order, as editor.c's camera_mode_names spells them */
/* clang-format off */
struct camera_mode camera_mode_list[CAMERA_MODES] = {
    /* FOLLOW, A_FOLLOW, C_FOLLOW, COL_FOLLOW */
    {camera_step_manual, CAMERA_LEVEL_NORMAL, CAMERA_FROM_TARGET, {0, 0},
     TRUE, NULL},
    {camera_step_manual, CAMERA_LEVEL_AREA, CAMERA_FROM_TARGET, {0, 0},
     FALSE, NULL},
    {camera_step_manual, CAMERA_LEVEL_COMP, CAMERA_FROM_TARGET, {0, 0},
     FALSE, NULL},
    {camera_step_manual, CAMERA_LEVEL_COLI, CAMERA_FROM_TARGET, {0, 0},
     FALSE, NULL},
    /* KNUCKLES, A_KNUCKLES, C_KNUCKLES, COL_KNUCKLES */
    {camera_step_follow, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     TRUE, NULL},
    {camera_step_follow, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_follow, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_follow, CAMERA_LEVEL_COLI, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* KNUCKLES2, A_KNUCKLES2, C_KNUCKLES2, COL_KNUCKLES2 */
    {camera_step_follow, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     TRUE, NULL},
    {camera_step_follow, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_follow, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_follow, CAMERA_LEVEL_COLI, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* MAGONOTE, A_MAGONOTE, C_MAGONOTE, COL_MAGONOTE */
    {camera_step_magonote, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     TRUE, NULL},
    {camera_step_magonote, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_magonote, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_magonote, CAMERA_LEVEL_COLI, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* SONIC, A_SONIC, C_SONIC, COL_SONIC */
    {camera_step_sonic, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     TRUE, NULL},
    {camera_step_sonic, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_sonic, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_sonic, CAMERA_LEVEL_COLI, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* ASHLAND, A_ASHLAND, C_ASHLAND, ASHLAND_I, A_ASHLAND_I, C_ASHLAND_I */
    {camera_step_ashland, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_ashland, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_ashland, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_ashland, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_ashland, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_ashland, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    /* FISHING, A_FISHING */
    {camera_step_fishing_point, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_fishing_point, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* FIXED, A_FIXED, C_FIXED */
    {camera_step_fixed_rolled, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_fixed_rolled, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_fixed_rolled, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    /* KLAMATH, A_KLAMATH, C_KLAMATH */
    {camera_step_klamath, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_klamath, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_klamath, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    /* LINE, A_LINE */
    {camera_step_line, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_line, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    /* NEWFOLLOW, A_NEWFOLLOW, C_NEWFOLLOW */
    {camera_step_newfollow, CAMERA_LEVEL_NORMAL, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    {camera_step_newfollow, CAMERA_LEVEL_AREA, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    {camera_step_newfollow, CAMERA_LEVEL_COMP, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    /* POINT, A_POINT, C_POINT */
    {camera_step_point, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_point, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    {camera_step_point, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, camera_preview_view},
    /* SONIC_P, A_SONIC_P, C_SONIC_P */
    {camera_step_sonic_p, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_sonic_p, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_sonic_p, CAMERA_LEVEL_COMP, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* ADVERTISE */
    {camera_step_origin, CAMERA_LEVEL_EVENT, CAMERA_FROM_TARGET, {0, 0},
     FALSE, NULL},
    /* BACK, BACK2 */
    {shooting_camera_swing, CAMERA_LEVEL_EVENT, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    {shooting_camera_follow, CAMERA_LEVEL_EVENT, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* BUILDING */
    {camera_step_building, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* CART */
    {camera_step_cart, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* CHAOS, CHAOS_P, CHAOS_STINIT, CHAOS_STD, CHAOS_W */
    {camera_step_chaos, CAMERA_LEVEL_EVENT, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    {camera_step_chaos_p, CAMERA_LEVEL_NORMAL, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    {camera_step_chaos_stinit, CAMERA_LEVEL_NORMAL, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    {camera_step_chaos_std, CAMERA_LEVEL_NORMAL, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    {camera_step_chaos_w, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* E101R, E103, EGM3 */
    {e101_r_camera_step, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {camera_step_e103, CAMERA_LEVEL_AREA, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    {eggmobile3_camera_step, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* FOLLOW_G, A_FOLLOW_G */
    {camera_step_follow_g, CAMERA_LEVEL_NORMAL, CAMERA_FROM_TARGET, {0, 0},
     TRUE, NULL},
    {camera_step_follow_g, CAMERA_LEVEL_AREA, CAMERA_FROM_TARGET, {0, 0},
     FALSE, NULL},
    /* LR */
    {camera_step_trigger_orbit, CAMERA_LEVEL_EVENT, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* COLLI */
    {camera_step_none, CAMERA_LEVEL_COLI, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* RuinWaka1 */
    {camera_step_work_view, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* SNOWBOARD */
    {camera_step_board, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* SURVEY */
    {camera_step_survey, CAMERA_LEVEL_COLI, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    /* TAIHO */
    {camera_step_taiho, CAMERA_LEVEL_NORMAL, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
    /* TORNADE */
    {camera_step_tornade, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* TWO_HARES */
    {camera_step_two_hares, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* LEAVE */
    {camera_step_leave, CAMERA_LEVEL_NORMAL, CAMERA_LOOK_AT, {0, 0},
     FALSE, NULL},
    /* EDITOR */
    {camera_orbit, CAMERA_LEVEL_DEBUG, CAMERA_FROM_TARGET, {0, 0},
     FALSE, NULL},
    /* GURIGURI */
    {camera_step_guriguri, CAMERA_LEVEL_DEBUG, CAMERA_FROM_TARGET, {0, 0},
     FALSE, NULL},
    /* PATHCAM */
    {camera_step_pathcam, CAMERA_LEVEL_EVENT, CAMERA_AT_ANGLES, {0, 0},
     FALSE, NULL},
};
/* clang-format on */
