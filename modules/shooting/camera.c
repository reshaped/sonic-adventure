/* the sky chase's camera steps. they look along the course's path, trail
 * the tornado, or swing round a target */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "player/player.h"
#include "shooting/camera.h"
#include "shooting/shooting.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* shooting_camera_start puts the watch point WATCH_BEHIND path points
 * behind the tornado's */
#define WATCH_BEHIND 10
/* shooting_camera_look_back sits LOOK_BACK_AHEAD path points ahead of the
 * tornado's, and looks at the point LOOK_BACK_BEHIND behind it */
#define LOOK_BACK_AHEAD 7
#define LOOK_BACK_BEHIND 15
/* a swing starts SWING_FAR times its distance away and closes in over
 * SWING_CLOSE_STEPS */
#define SWING_FAR 4.0F
#define SWING_CLOSE_STEPS 120.0F

/* the trailing camera's distance and angles, and its offset from its point
 * on the path */
static Float shooting_camera_distance = 80.0F;
static Angle shooting_camera_ang_x = 0;
static Angle shooting_camera_ang_y = 0;
Angle shooting_camera_ang_z = 0;
static Float shooting_camera_offset_x = 0.0F;
static Float shooting_camera_offset_y = 0.0F;
static Float shooting_camera_offset_z = 0.0F;
/* in path points from the tornado's point: the trailing camera behind it,
 * and its target ahead */
static int shooting_camera_behind = 10;
static int shooting_camera_ahead = 15;

static int watch_point; /* the path point shooting_camera_watch sits at */
/* how far the swing has turned the camera round the target */
static Angle swing_turned;
static Angle swing_turn; /* how far it turns the camera a step */
static struct task_work *swing_work; /* the work the camera swings round */
static Float swing_height; /* the target's height over the work */
static Float swing_distance; /* the camera's distance from the target */
static Float swing_near; /* the distance the camera closes in to */
static Float swing_close_step; /* how far it closes in a step */
static Sint8 swing_done; /* set once the swing is round. nothing reads it */
static Sint8 swing_phase;
static NJS_POINT3 swing_target; /* the point the camera swings round */
static Angle swing_heading; /* the camera's heading */

/* swing_phase's values: the swing closes in, then turns, then holds its
 * distance and heading once round */
enum swing_phase { SWING_CLOSING, SWING_TURNING, SWING_ROUND };

void shooting_camera_watch(struct camera_view *view)
{
    /* @hack redundant: offset, set and never read. its initializer keeps
     * the image's copy of {0, 0, 80} into the stack at R15+12 (0x0c900002 to
     * 0x0c900014); without it, the constants run 12 bytes short, and 75 of
     * 77 words differ */
    NJS_VECTOR offset = {0.0F, 0.0F, 80.0F};
    /* @hack redundant: turned, 12 bytes of stack no instruction reads or
     * writes, of the 24 that ADD #-24,R15 at 0x0c900000 makes; without it,
     * 75 of 77 words differ */
    NJS_POINT3 turned;
    int behind;

    /* @hack redundant: behind, set and clamped but never read (ADD #-10,R4
     * at 0x0c90001c, CMP/GE at 0x0c90001e, CMP/PZ at 0x0c900026); without
     * it, 72 of 77 words differ */
    behind = shooting_path_index - WATCH_BEHIND;
    if (behind >= shooting_path_count) {
        behind = 0;
    }
    if (behind < 0) {
        behind = 0;
    }
    camera_state->target.x = player_work[0]->pos.x;
    camera_state->target.y = player_work[0]->pos.y;
    camera_state->target.z = player_work[0]->pos.z;
    camera_state->pos.x = shooting_path[watch_point].x;
    camera_state->pos.y = shooting_path[watch_point].y;
    camera_state->pos.z = shooting_path[watch_point].z;
}

void shooting_camera_look_back(struct camera_view *view)
{
    /* @hack redundant: offset, set and never read. its initializer keeps
     * the image's copy of {0, 0, 10} into the stack at R15+12 (0x0c9000a2 to
     * 0x0c9000b4); without it, the constants run 12 bytes short, and 105 of
     * 109 words differ */
    NJS_VECTOR offset = {0.0F, 0.0F, 10.0F};
    /* @hack redundant: turned, 12 bytes of stack no instruction reads or
     * writes, of the 24 that ADD #-24,R15 at 0x0c9000a0 makes; without it,
     * 104 of 109 words differ */
    NJS_POINT3 turned;
    int behind;
    int ahead;
    struct task *player;
    /* work is the dead store tagged below */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct task_work *work;

    behind = shooting_path_index - LOOK_BACK_BEHIND;
    if (behind >= shooting_path_count) {
        behind = 0;
    }
    ahead = shooting_path_index + LOOK_BACK_AHEAD;
    if (ahead >= shooting_path_count) {
        ahead = 0;
    }
    if (ahead < 0) {
        ahead = 0;
    }
    player = player_get_task(0);
    /* @hack redundant: work, set and never read, for which SHC copies
     * player into R4 (MOV R0,R4 at 0x0c9000ec); without it, 65 of 109 words
     * differ */
    work = player->work;
    camera_state->target.x = shooting_path[behind].x;
    camera_state->target.y = shooting_path[behind].y;
    camera_state->target.z = shooting_path[behind].z;
    camera_state->pos.x = shooting_path[ahead].x;
    camera_state->pos.y = shooting_path[ahead].y;
    camera_state->pos.z = shooting_path[ahead].z;
    camera_state->distance = shooting_camera_distance;
    camera_state->ang_x = shooting_camera_ang_x;
    camera_state->ang_y = shooting_camera_ang_y;
    camera_state->ang_z = shooting_camera_ang_z;
}

/* the camera sits shooting_camera_behind path points behind the tornado's
 * point, moved by its offset. its target is the point shooting_camera_ahead
 * past the tornado's. half the tornado's offset from the path moves the
 * target, turned as the tornado turns */
void shooting_camera_follow(struct camera_view *view)
{
    NJS_VECTOR offset = {0.0F, 0.0F, 10.0F};
    NJS_POINT3 turned;
    int ahead;
    int behind;
    struct task *player;
    struct task_work *work;

    ahead = shooting_path_index + shooting_camera_ahead;
    if (ahead >= shooting_path_count) {
        ahead = 0;
    }
    behind = shooting_path_index - shooting_camera_behind;
    if (behind >= shooting_path_count) {
        behind = 0;
    }
    if (behind < 0) {
        behind = 0;
    }
    player = player_get_task(0);
    work = player->work;
    offset.x = shooting_flight.offset.x * 0.5F;
    offset.y = shooting_flight.offset.y * 0.5F;
    offset.z = shooting_flight.offset.z * 0.5F;
    njPushMatrix(&_unit_matrix_);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njRotateXYZ(NULL, shooting_flight.ang.x, shooting_flight.ang.y,
                shooting_flight.ang.z);
    njCalcPoint(NULL, &offset, &turned);
    njPopMatrix(1);
    camera_state->target.x = shooting_path[ahead].x + turned.x;
    camera_state->target.y = shooting_path[ahead].y + turned.y;
    camera_state->target.z = shooting_path[ahead].z + turned.z;
    camera_state->pos.x = shooting_path[behind].x + shooting_camera_offset_x;
    camera_state->pos.y = shooting_path[behind].y + shooting_camera_offset_y;
    camera_state->pos.z = shooting_path[behind].z + shooting_camera_offset_z;
    camera_state->distance = shooting_camera_distance;
    camera_state->ang_x = shooting_camera_ang_x;
    camera_state->ang_y = shooting_camera_ang_y;
    camera_state->ang_z = shooting_camera_ang_z;
}

void shooting_camera_start(void)
{
    int point;

    /* @hack spelling: the store and the copy joined by a comma, which keeps
     * watch_point's address in R5 while point takes R4 (0x0c900320,
     * 0x0c90032a). as two statements, the address takes R4, and 9 of 14
     * words differ */
    watch_point = shooting_path_index - WATCH_BEHIND, point = watch_point;
    /* @hack redundant: two tests of point with empty bodies, the image's CMP/GE
     * and CMP/PZ at 0x0c900332 and 0x0c900334; without them, 13 of 14 words
     * differ */
    if (point >= shooting_path_count) {
    }
    if (point < 0) {
    }
    camera_set_mode(CAMERA_MODE_LEAVE, CAMERA_ADJUST_SLOW);
}

void shooting_camera_set_spacing(int spacing)
{
    switch (spacing) {
    case 0:
        shooting_camera_behind = 10;
        shooting_camera_ahead = 10;
        break;
    case 1:
        shooting_camera_behind = 10;
        shooting_camera_ahead = 5;
        break;
    }
}

/* set out to the point distance from camera_state's target, in the
 * direction of camera_state's y angle */
static void point_on_heading(Float distance, NJS_POINT3 *out)
{
    NJS_POINT3 point;
    Angle ang;

    point.x = 0.0F;
    point.y = 0.0F;
    point.z = distance;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, camera_state->ang_y, ang);
    njCalcPoint(NULL, &point, out);
    njPopMatrix(1);
    njAddVector(out, &camera_state->target);
}

void shooting_camera_swing(struct camera_view *view)
{
    switch (swing_phase) {
    case SWING_CLOSING:
        swing_target.x = camera_state->target.x = swing_work->pos.x;
        swing_target.y = camera_state->target.y =
            swing_work->pos.y + swing_height;
        swing_target.z = camera_state->target.z = swing_work->pos.z;
        swing_distance += swing_close_step;
        if (swing_distance <= swing_near) {
            swing_phase = SWING_TURNING;
        }
        break;
    case SWING_TURNING:
        swing_target.x = camera_state->target.x = swing_work->pos.x;
        swing_target.y = camera_state->target.y =
            swing_work->pos.y + swing_height;
        swing_target.z = camera_state->target.z = swing_work->pos.z;
        swing_heading += swing_turn;
        swing_turned += swing_turn;
        if (swing_turned >= 0x8000) {
            swing_phase = SWING_ROUND;
            swing_done = TRUE;
        }
        break;
    case SWING_ROUND:
        swing_target.x = camera_state->target.x = swing_work->pos.x;
        swing_target.y = camera_state->target.y =
            swing_work->pos.y + swing_height;
        swing_target.z = camera_state->target.z = swing_work->pos.z;
        break;
    }
    camera_state->ang_y = swing_heading;
    camera_state->target.x = swing_target.x;
    camera_state->target.y = swing_target.y;
    camera_state->target.z = swing_target.z;
    point_on_heading(swing_distance, &camera_state->pos);
}

void shooting_camera_swing_start(struct task_work *work, Float height,
                                 Float distance, int swing_steps)
{
    swing_done = FALSE;
    swing_work = work;
    swing_height = height;
    swing_near = distance;
    swing_distance = distance * SWING_FAR;
    swing_close_step =
        (swing_near - swing_distance) * (1.0F / SWING_CLOSE_STEPS);
    swing_target.x = camera_state->target.x = swing_work->pos.x;
    swing_target.y = camera_state->target.y = swing_work->pos.y + height;
    swing_target.z = camera_state->target.z = swing_work->pos.z;
    swing_turn = (Angle)(180.0F / swing_steps * ANGLE_PER_DEGREE);
    camera_state->ang_x = camera_state->ang_z = 0;
    swing_heading = camera_state->ang_y = work->ang.y + 0x8000;
    swing_turned = 0;
    point_on_heading(swing_distance, &camera_state->pos);
    swing_phase = SWING_CLOSING;
    pad_disable();
}
