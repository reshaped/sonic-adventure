/* E-101 mkII's fight camera, CAMERA_MODE_E101R. it follows player 0 and
 * turns toward the boss, gets behind the boss, or circles the boss */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_e101_r/camera.h"
#include "camera/camera.h"
#include "player/player.h"
#include "system/math.h"
#include "system/util.h"

/* the most E101_R_CAMERA_TURN turns the heading in a step */
#define TURN_MOST NJM_DEG_ANG(3)
/* a following camera looks LOOK_AHEAD past player 0 along its heading. it
 * looks LOOK_UP above player 0's height at e101_r_camera_start. it eases to
 * FOLLOW_UP above player 0, and to FOLLOW_BACK or more behind it */
#define LOOK_AHEAD 70.0F
#define LOOK_UP 20.0F
#define FOLLOW_BACK 20.0F
#define FOLLOW_UP 23.0F
/* the depths and the distance back by which camera_follow sets the mark it
 * eases back to */
#define DEPTH_NEAR 10.0F
#define DEPTH_FAR 30.0F
#define FAR_BACK 80.0F
/* how much farther back E101_R_CAMERA_BACK_OFF eases than
 * E101_R_CAMERA_FOLLOW */
#define BACK_OFF 100.0F
/* while the wide view is on, the least distance back, and the height */
#define WIDE_BACK 150.0F
#define WIDE_UP 60.0F
/* E101_R_CAMERA_BEHIND eases to BEHIND_BACK past the boss from player 0,
 * and BEHIND_UP above that */
#define BEHIND_BACK 75.0F
#define BEHIND_UP 50.0F
/* the circling camera's distance from the boss and height above it, and its
 * turn each step in degrees */
#define ORBIT_BACK 60.0F
#define ORBIT_UP 50.0F
#define ORBIT_SPEED 1.0F

static void camera_follow(void);
static void camera_behind(void);
static void camera_circle(void);

/* where the camera looks and where it sits. each step copies both to
 * camera_state */
static NJS_POINT3 camera_target;
static NJS_POINT3 camera_pos;
/* the following camera's target y, less target_rise */
static Float target_height;
static Angle camera_heading; /* as e101_r_camera_get_heading gives it */
static Angle turn_speed; /* the heading's turn each step while following */
static Float orbit_speed; /* the same while circling, in degrees */
static NJS_POINT3 *camera_boss; /* the caller's point, not a copy */
static Float player_speed;
static Angle camera_aim; /* the heading the camera turns toward */
static int camera_mode; /* an enum e101_r_camera_mode */
static Float camera_height; /* above player 0, while following */
/* behind player 0 while following, or from the boss while circling */
static Float camera_distance;
static Float target_rise; /* above target_height, easing to rise_aim */
static Float rise_aim;
static Float blast_size;
static Bool camera_wide;

/* #pragma section HEAD puts the step in PHEAD, which the module's link
 * places first, at 0x0c900000. 1ST_READ.BIN's camera_mode_list names it
 * there, as CAMERA_MODE_E101R's step */
#pragma section HEAD
void e101_r_camera_step(struct camera_view *view)
{
    switch (camera_mode) {
    case E101_R_CAMERA_ORBIT:
        orbit_speed = ORBIT_SPEED;
        blast_size = 0.0F;
        /* fall through */
    case E101_R_CAMERA_ORBIT_END:
        camera_circle();
        break;
    case E101_R_CAMERA_BEHIND:
        camera_behind();
        break;
    default:
        camera_follow();
        break;
    }
}
#pragma section

/* the heading of pos seen from origin, in x and z */
static Angle heading_to(NJS_POINT3 *pos, NJS_POINT3 *origin)
{
    Float dx = pos->x - origin->x;
    Float dz = pos->z - origin->z;

    return angle_arc_tan2(dz, dx);
}

void e101_r_camera_reset(void)
{
    turn_speed = 0;
    camera_mode = E101_R_CAMERA_FOLLOW;
    camera_distance = FOLLOW_BACK;
    camera_height = FOLLOW_UP;
    target_rise = rise_aim = 0.0F;
    camera_wide = FALSE;
}

void e101_r_camera_start(NJS_POINT3 *boss)
{
    /* @hack redundant: a read of player 0's x that nothing uses, MOV.L @R2,R2
     * at 0x0c903912; without it, 39 of 42 words differ */
    Float unused = player_work[0]->pos.x;

    camera_boss = boss;
    camera_heading = heading_to(boss, &player_work[0]->pos);
    target_height = camera_target.y = player_work[0]->pos.y + LOOK_UP;
    camera_pos.x = player_work[0]->pos.x;
    camera_pos.z = player_work[0]->pos.z;
    camera_pos.y = player_work[0]->pos.y;
}

void e101_r_camera_set_target_rise(Float rise)
{
    rise_aim = rise;
}

void e101_r_camera_set_aim(Angle aim, Float speed)
{
    player_speed = speed;
    camera_aim = aim;
}

void e101_r_camera_set_blast_size(Float size)
{
    blast_size = size;
}

void e101_r_camera_set_wide(Bool wide)
{
    camera_wide = wide;
}

void e101_r_camera_set_mode(int mode)
{
    camera_mode = mode;
}

Angle e101_r_camera_get_turn(void)
{
    return angle_difference(camera_heading, camera_aim) < 0
               ? -angle_difference(camera_heading, camera_aim)
               : angle_difference(camera_heading, camera_aim);
}

Angle e101_r_camera_get_heading(void)
{
    return camera_heading;
}

/* follow behind player 0, looking LOOK_AHEAD past it along the heading,
 * which turns as camera_mode says. the distance back eases to a mark. depth
 * is player 0's z less the boss's, each through the current matrix. the
 * mark starts at FOLLOW_BACK. each unit of depth past DEPTH_NEAR adds a
 * twentieth of FAR_BACK. each unit short of it adds 0.2. from a depth of
 * DEPTH_FAR on, the mark is FAR_BACK instead. each unit of player 0's speed
 * then adds a fifth of FAR_BACK */
static void camera_follow(void)
{
    NJS_POINT3 player_view;
    NJS_POINT3 boss_view;
    Float back;
    Float up;
    Float depth;
    Float sine;
    Float cosine;
    Angle most;
    NJS_POINT3 *player = &player_work[0]->pos;

    camera_aim = heading_to(camera_boss, player);
    switch (camera_mode) {
    case E101_R_CAMERA_COAST:
        turn_speed *= 0.98F;
        camera_heading += turn_speed;
        break;
    case E101_R_CAMERA_TURN:
        turn_speed = CLAMP(angle_difference(camera_heading, camera_aim) * 0.05F,
                           -TURN_MOST, TURN_MOST);
        camera_heading += turn_speed;
        break;
    case E101_R_CAMERA_TRACK:
        most =
            NJM_DEG_ANG(1) > (Angle)((2.0F - player_speed) * ANGLE_PER_DEGREE)
                ? NJM_DEG_ANG(1)
                : (Angle)((2.0F - player_speed) * ANGLE_PER_DEGREE);
        turn_speed = CLAMP(angle_difference(camera_heading, camera_aim) * 0.05F,
                           -most, most);
        camera_heading += turn_speed;
        break;
    default:
        if (player_speed <= 2.5F) {
            most = (Angle)((2.5F - player_speed) * ANGLE_PER_DEGREE);
            turn_speed =
                CLAMP(angle_difference(camera_heading, camera_aim) * 0.1F,
                      -most, most);
            camera_heading += turn_speed;
        }
        break;
    }
    back = FOLLOW_BACK;
    up = FOLLOW_UP;
    njCalcPoint(NULL, player, &player_view);
    njCalcPoint(NULL, camera_boss, &boss_view);
    depth = player_view.z - boss_view.z;
    if (depth >= DEPTH_FAR) {
        back = FAR_BACK;
    } else if (depth >= DEPTH_NEAR) {
        back += (depth - DEPTH_NEAR) * (0.05F * FAR_BACK);
    } else {
        depth = DEPTH_NEAR - depth;
        back += depth * 0.2F;
    }
    back += player_speed * (0.2F * FAR_BACK);
    if (camera_mode == E101_R_CAMERA_BACK_OFF) {
        back += BACK_OFF;
    }
    /* @hack spelling: camera_wide compared with TRUE, which SHC tests with
     * CMP/EQ #1 at 0x0c903ce6. camera_wide holds only the init's FALSE or the
     * comparison e101_r_camera_set_wide is passed, so a plain test of it is the
     * same; as one, 4 of 388 words differ */
    if (camera_wide == TRUE) {
        if (back <= WIDE_BACK) {
            back = WIDE_BACK;
        }
        up = WIDE_UP;
    }
    camera_distance += (back - camera_distance) * 0.04F;
    camera_height += (up - camera_height) * 0.02F;
    target_rise += (rise_aim - target_rise) * 0.02F;
    _builtin_fsca(camera_heading, &sine, &cosine);
    camera_target.x = player->x + (cosine * LOOK_AHEAD);
    camera_target.z = player->z + (sine * LOOK_AHEAD);
    camera_target.y = target_height + target_rise;
    camera_pos.x = player->x - (cosine * camera_distance);
    camera_pos.z = player->z - (sine * camera_distance);
    camera_pos.y = player->y + camera_height;
    camera_state->target.x = camera_target.x;
    camera_state->target.y = camera_target.y;
    camera_state->target.z = camera_target.z;
    camera_state->pos.x = camera_pos.x;
    camera_state->pos.y = camera_pos.y;
    camera_state->pos.z = camera_pos.z;
}

/* ease to BEHIND_BACK past the boss from player 0, and BEHIND_UP above
 * that. the target eases to player 0 */
static void camera_behind(void)
{
    NJS_VECTOR dir;
    NJS_POINT3 *player = &player_work[0]->pos;

    VECTOR_DIFF(&dir, camera_boss, player);
    vector_unit(&dir, &dir);
    VECTOR_SCALE(&dir, &dir, BEHIND_BACK);
    dir.y += BEHIND_UP;
    camera_target.x += (player->x - camera_target.x) * 0.04F;
    camera_target.y += (player->y - camera_target.y) * 0.04F;
    camera_target.z += (player->z - camera_target.z) * 0.04F;
    camera_pos.x += (camera_boss->x + dir.x - camera_pos.x) * 0.04F;
    camera_pos.y += (camera_boss->y + dir.y - camera_pos.y) * 0.04F;
    camera_pos.z += (camera_boss->z + dir.z - camera_pos.z) * 0.04F;
    camera_state->target.x = camera_target.x;
    camera_state->target.y = camera_target.y;
    camera_state->target.z = camera_target.z;
    camera_state->pos.x = camera_pos.x;
    camera_state->pos.y = camera_pos.y;
    camera_state->pos.z = camera_pos.z;
}

/* circle the boss at orbit_speed, ORBIT_UP above it and looking at it.
 * under E101_R_CAMERA_ORBIT, the distance eases to ORBIT_BACK plus three
 * times blast_size, which e101_r_camera_step has just set to 0. under
 * E101_R_CAMERA_ORBIT_END, the circling slows by 2% each step, and the
 * distance eases to ORBIT_BACK */
static void camera_circle(void)
{
    Float sine;
    Float cosine;
    Float reach;
    /* @hack redundant: a point nothing uses, which takes 12 of the frame's 20
     * bytes, ADD #-20,R15 at 0x0c903fa2; without it, 2 of 118 words
     * differ */
    NJS_POINT3 unused;

    camera_heading += (Angle)(orbit_speed * ANGLE_PER_DEGREE);
    _builtin_fsca(camera_heading, &sine, &cosine);
    camera_target.x = camera_boss->x;
    camera_target.z = camera_boss->z;
    camera_target.y = camera_boss->y;
    camera_pos.x = camera_boss->x + (cosine * camera_distance);
    camera_pos.z = camera_boss->z + (sine * camera_distance);
    camera_pos.y = camera_boss->y + ORBIT_UP;
    if (camera_mode == E101_R_CAMERA_ORBIT) {
        reach = ORBIT_BACK + (3.0F * blast_size);
        camera_distance += (reach - camera_distance) * 0.1F;
    } else {
        orbit_speed *= 0.98F;
        camera_distance += (ORBIT_BACK - camera_distance) * 0.03F;
    }
    camera_state->target.x = camera_target.x;
    camera_state->target.y = camera_target.y;
    camera_state->target.z = camera_target.z;
    camera_state->pos.x = camera_pos.x;
    camera_state->pos.y = camera_pos.y;
    camera_state->pos.z = camera_pos.z;
}
