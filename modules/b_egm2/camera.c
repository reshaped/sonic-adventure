/* the Egg Walker fight's camera, its shake, and the sky drawn about it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm2/camera.h"
#include "b_egm2/egg_walker.h"
#include "b_egm2/parts.h"
#include "b_egm2/texlists.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "enemy/enemy.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

/* the street the fight is on: its height, and its bounds in x and z. the
 * camera's position stays in the bounds, and no lower than CAMERA_FLOOR */
#define STREET_Y 0.0F
#define STREET_X_MIN (-537.0F)
#define STREET_X_MAX (-350.0F)
#define STREET_Z_MIN 910.0F
#define STREET_Z_MAX 1500.0F
#define CAMERA_FLOOR 2.5F

#define QUARTER_TURN 0x4000
#define HALF_TURN 0x8000

/* near_boss turns TRUE once player 0 is within NEAR_IN of the boss along
 * z. it turns FALSE once player 0 is more than NEAR_OUT from it */
#define NEAR_IN 90.0F
#define NEAR_OUT 100.0F

/* away from the boss, the goal lies PLAYER_REACH from player 0 */
#define PLAYER_REACH 50.0F
/* near the boss, the goal lies BOSS_REACH farther from the boss's body than
 * player 0 does. these distances are in x and z. that holds while player 0
 * is more than BOSS_INSIDE from the body, on the side beyond_boss records.
 * otherwise the goal lies BOSS_CLOSE from the body */
#define BOSS_REACH 50.0F
#define BOSS_CLOSE 110.0F
#define BOSS_INSIDE 60.0F
/* while face_boss is set away from the boss, the goal lies FACE_REACH from
 * player 0 */
#define FACE_REACH 10.0F

/* goal_height moves HEIGHT_STEP a step: up to HEIGHT_MAX near the boss,
 * and down to 1 away from it. while look_past is set, it moves down to 0 */
#define HEIGHT_STEP 0.3F
#define HEIGHT_MAX 30.0F

/* while look_past is set, orbit_yaw turns half the way to its aim a step,
 * at most LOOK_TURN */
#define LOOK_TURN NJM_DEG_ANG(3)

/* each step, the camera's velocities keep EASE_KEEP of themselves and gain
 * EASE_PULL of the distance to their goals. the shake's velocity keeps
 * SHAKE_KEEP of itself and loses SHAKE_PULL of the shake */
#define EASE_KEEP 0.7F
#define EASE_PULL 0.03F
#define SHAKE_KEEP 0.95F
#define SHAKE_PULL 0.1F

/* egg_walker_camera_target_create's task stands SWING_RISE above pos */
#define SWING_RISE 40.0F

/* @hack spelling: the clear as one chain, which stores z, then y, then x
 * (FMOV.S FR4,@(R0,R2) with R0 8 at 0x0c902134); as util.h's VECTOR_CLEAR,
 * 35 of 77 words differ */
#define VECTOR_ZERO(v) ((v)->x = (v)->y = (v)->z = 0.0F)
/* @hack spelling: one statement per component, under which SHC loads
 * player_last's z with R2 as the index and R0 as the base (FMOV.S
 * @(R0,R2),FR3 at 0x0c90231c); as util.h's VECTOR_DIFF, a comma
 * expression, 80 of 116 words differ */
#define VECTOR_DIFF_EACH(out, a, b)                                            \
    do {                                                                       \
        (out)->x = (a)->x - (b)->x;                                            \
        (out)->y = (a)->y - (b)->y;                                            \
        (out)->z = (a)->z - (b)->z;                                            \
    } while (0)
/* @hack spelling: one statement per component, under which SHC picks the
 * image's registers (in follow_goals from 0x0c902ad2); as util.h's
 * VECTOR_SCALE, a comma expression, summed over follow_goals and
 * shake_settle, 95 of 388 words differ */
#define VECTOR_SCALE_EACH(out, v, s)                                           \
    do {                                                                       \
        (out)->x = (v)->x * (s);                                               \
        (out)->y = (v)->y * (s);                                               \
        (out)->z = (v)->z * (s);                                               \
    } while (0)
/* @hack spelling: one statement per component, under which SHC picks the
 * image's registers (in follow_goals from 0x0c902c46); as util.h's
 * VECTOR_ADD, a comma expression, the object runs 32 bytes long. built with
 * egg_walker_camera_target_create cut 32 bytes to fit, summed over
 * follow_goals and shake_settle, 118 of 388 words differ */
#define VECTOR_ADD_EACH(v, d)                                                  \
    do {                                                                       \
        (v)->x = (v)->x + (d)->x;                                              \
        (v)->y = (v)->y + (d)->y;                                              \
        (v)->z = (v)->z + (d)->z;                                              \
    } while (0)
/* @hack spelling: one statement per component, under which SHC picks the
 * image's registers (in shake_settle from 0x0c902ede); as util.h's
 * VECTOR_SUB, a comma expression, the object runs 32 bytes long. built with
 * egg_walker_camera_target_create cut 32 bytes to fit, 42 of 75 words
 * differ */
#define VECTOR_SUB_EACH(v, d)                                                  \
    do {                                                                       \
        (v)->x = (v)->x - (d)->x;                                              \
        (v)->y = (v)->y - (d)->y;                                              \
        (v)->z = (v)->z - (d)->z;                                              \
    } while (0)
/* the square of a->m - b->m, which reads a and b twice */
/* @hack spelling: a and b passed as pointers, under which SHC reads
 * player_last through copies of its address (0x0c902334 to 0x0c90236a);
 * with each member named directly, summed over watch_player and
 * place_by_boss, 227 of 341 words differ */
#define SQUARE_DIFF(a, b, m) SQUARE((a)->m - (b)->m)
static void egg_walker_camera_step(struct camera_view *view);
static void watch_player(void);
static void update_near(void);
static void place_by_boss(void);
static void place_away_from_boss(void);
static void goal_from_player(struct angles *ang);
static Bool outside_street(NJS_POINT3 *pos);
static void heading_away(NJS_POINT3 *from, NJS_POINT3 *to, struct angles *ang);
static void follow_goals(void);
static void keep_in_street(void);
static void anchor_create(void);
static void anchor_step(struct task *task);
static void shake_settle(void);
static void swing_target_step(struct task *task);

/* a point past each end of the street, the low z end first. heading_away
 * reads only their x and z */
static const NJS_POINT3 street_ends[2] = {
    {-442.5F, 72.0F, 762.5F},
    {-442.5F, 72.0F, 1640.0F},
};

static NJS_MATERIAL egm2_sky_materials[2] = {
#include "b_egm2/camera/egm2_sky_materials.inc"
};

static Sint16 egm2_sky_meshes0[105] = {
#include "b_egm2/camera/egm2_sky_meshes0.inc"
};

static Sint16 egm2_sky_meshes1[153] = {
#include "b_egm2/camera/egm2_sky_meshes1.inc"
};

static NJS_COLOR egm2_sky_uvs0[94] = {
#include "b_egm2/camera/egm2_sky_uvs0.inc"
};

static NJS_COLOR egm2_sky_uvs1[150] = {
#include "b_egm2/camera/egm2_sky_uvs1.inc"
};

static NJS_MESHSET egm2_sky_meshsets[2] = {
#include "b_egm2/camera/egm2_sky_meshsets.inc"
};

static NJS_POINT3 egm2_sky_points[121] = {
#include "b_egm2/camera/egm2_sky_points.inc"
};

static NJS_VECTOR egm2_sky_normals[121] = {
#include "b_egm2/camera/egm2_sky_normals.inc"
};

static NJS_MODEL egm2_sky_model = {
#include "b_egm2/camera/egm2_sky_model.inc"
};

static NJS_OBJECT egm2_sky_object = {
#include "b_egm2/camera/egm2_sky_object.inc"
};

static struct collision_shape anchor_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     0,
     {0.0F, 0.0F, 0.0F},
     20.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the boss's work position and angles, from egg_walker_camera_start */
static NJS_POINT3 *boss_pos;
static struct angles *boss_ang;
/* player 0's position as the camera's step last read it. watch_player
 * compares the new one with player_last before it sets both */
static NJS_POINT3 player_pos;
static NJS_POINT3 player_last;
static NJS_VECTOR player_move; /* player 0's move over the last step. unread */
/* TRUE when player 0 was above the street or had moved 0.5 or more from
 * player_last. the camera's last step set it. unread */
static Bool player_moving;
/* the heading from the camera's target through its position at the start
 * of the last step. unread */
static Angle view_yaw;
static NJS_POINT3 goal_pos; /* where the camera's anchor heads */
/* what the camera looks at, before the shake */
static NJS_POINT3 goal_target;
static NJS_VECTOR pos_speed; /* the anchor's velocity */
/* a velocity toward goal_target that follow_goals updates. nothing applies
 * it */
static NJS_VECTOR target_speed;
/* the index of the street_ends point beyond the boss as seen from player 0 */
static Sint8 street_end;
/* TRUE until the end of the camera's first step. unread */
static Bool first_step;
static Bool near_boss;
/* TRUE when place_by_boss is to check again which side of the boss player 0
 * is on */
static Bool side_stale;
/* TRUE from egg_walker_camera_look_past until the camera's next step ends */
static Bool look_past;
/* the goal's height above the street near the boss, or above player 0 away
 * from it */
static Float goal_height;
/* TRUE if player 0 was at a greater z than the boss's body at the last side
 * check */
static Bool beyond_boss;
/* the anchor task's position: the camera's position before the shake */
static NJS_POINT3 *anchor_pos;
/* TRUE from egg_walker_camera_look_past or egg_walker_camera_face_boss
 * until the camera's next step ends */
static Bool face_boss;
/* the offset the shake adds to the camera's position and target */
static NJS_VECTOR shake;
static NJS_VECTOR shake_speed;
/* egg_walker_camera_start clears it, and nothing else touches it */
static Sint32 dat_0c9929e0;
/* the camera's position at the last step, after the shake and the street's
 * bounds */
static NJS_POINT3 camera_pos;
/* the goal's angle about the boss's body, as place_by_boss turns it */
static Angle orbit_yaw;

void egg_walker_camera_start(NJS_POINT3 *pos, struct angles *ang)
{
    boss_pos = pos;
    boss_ang = ang;
    VECTOR_ZERO(&pos_speed);
    VECTOR_ZERO(&target_speed);
    player_pos = player_work[0]->pos;
    player_last = player_work[0]->pos;
    first_step = TRUE;
    near_boss = FALSE;
    side_stale = FALSE;
    look_past = FALSE;
    goal_height = 1.0F;
    dat_0c9929e0 = 0;
    VECTOR_ZERO(&shake);
    VECTOR_ZERO(&shake_speed);
    camera_override(egg_walker_camera_step, 0, CAMERA_LOOK_AT);
    anchor_create();
}

/* the fight's camera step, which camera_override runs. it sets
 * camera_state's position and target, and leaves view unread */
static void egg_walker_camera_step(struct camera_view *view)
{
    view_yaw = -QUARTER_TURN -
               (Angle)(atan2f(camera_state->target.z - camera_state->pos.z,
                              camera_state->target.x - camera_state->pos.x) *
                       ANGLE_PER_RADIAN);
    watch_player();
    update_near();
    if (near_boss == TRUE || look_past == TRUE) {
        place_by_boss();
    } else {
        place_away_from_boss();
    }
    shake_settle();
    follow_goals();
    keep_in_street();
    first_step = FALSE;
    look_past = FALSE;
    face_boss = FALSE;
}

static void watch_player(void)
{
    VECTOR_DIFF_EACH(&player_move, &player_work[0]->pos, &player_last);
    if (player_work[0]->pos.y > 0.0F ||
        njSqrt(SQUARE_DIFF(&player_work[0]->pos, &player_last, x) +
               SQUARE_DIFF(&player_work[0]->pos, &player_last, y) +
               SQUARE_DIFF(&player_work[0]->pos, &player_last, z)) >= 0.5F) {
        player_moving = TRUE;
    } else {
        player_moving = FALSE;
    }
    player_last = player_work[0]->pos;
    player_pos = player_work[0]->pos;
}

/* set near_boss by player 0's distance from the boss along z. near the
 * boss, goal_height is above the street, and away from it, above player 0.
 * so goal_height gains player 0's height as player 0 comes near, and loses
 * it as player 0 leaves */
static void update_near(void)
{
    Float dz = njAbs(player_pos.z - boss_pos->z);

    if (near_boss == TRUE) {
        if (dz > NEAR_OUT) {
            near_boss = FALSE;
            goal_height -= player_pos.y;
        }
    } else if (dz < NEAR_IN) {
        near_boss = TRUE;
        side_stale = TRUE;
        goal_height += player_pos.y;
    }
}

void egg_walker_camera_look_past(void)
{
    look_past = TRUE;
    side_stale = TRUE;
    face_boss = TRUE;
}

/* place the goal near the boss, looking at player 0. the goal lies at
 * orbit_yaw about the boss's body, as far out as BOSS_REACH says. it lies
 * goal_height above the street. orbit_yaw's aim is the boss's heading while
 * beyond_boss is TRUE, and the opposite heading otherwise. orbit_yaw takes
 * its aim at once, or while look_past is set, turns toward it */
static void place_by_boss(void)
{
    Angle target;
    Angle step;
    Float dist;

    if (side_stale) {
        side_stale = FALSE;
        beyond_boss = player_pos.z > egg_walker_part_pos(0)->z;
    }
    if (beyond_boss) {
        target = boss_ang->y;
    } else {
        target = boss_ang->y + HALF_TURN;
    }
    if (look_past) {
        step = (Angle)(SHORT_TURN((Uint16)(target - orbit_yaw)) * 0.5F);
        if (step > LOOK_TURN) {
            step = LOOK_TURN;
        }
        if (step < -LOOK_TURN) {
            step = -LOOK_TURN;
        }
        orbit_yaw += step;
    } else {
        orbit_yaw = target;
    }
    if (((beyond_boss && player_pos.z > egg_walker_part_pos(0)->z) ||
         (!beyond_boss && player_pos.z < egg_walker_part_pos(0)->z)) &&
        (dist = njSqrt(SQUARE_DIFF(egg_walker_part_pos(0), &player_pos, x) +
                       SQUARE_DIFF(egg_walker_part_pos(0), &player_pos, z))) >
            BOSS_INSIDE) {
        dist += BOSS_REACH;
    } else {
        dist = BOSS_CLOSE;
    }
    goal_pos = *egg_walker_part_pos(0);
    goal_pos.x -= dist * njSin(orbit_yaw);
    goal_pos.z += dist * njCos(orbit_yaw);
    if (look_past) {
        if ((goal_height -= HEIGHT_STEP) < 0.0F) {
            goal_height = 0.0F;
        }
    } else {
        if ((goal_height += HEIGHT_STEP) > HEIGHT_MAX) {
            goal_height = HEIGHT_MAX;
        }
    }
    /* @hack redundant: the sum with STREET_Y, which is 0: an FADD of FR4
     * at 0x0c90267c, which FLDI0 FR4 at 0x0c902652 clears; without it, 40
     * of 225 words differ */
    goal_pos.y = STREET_Y + goal_height;
    goal_target = player_pos;
}

/* place the goal away from the boss, looking at player 0. goal_from_player
 * places it straight away from the street's end beyond the boss */
static void place_away_from_boss(void)
{
    struct angles ang;
    Angle heading;

    if (player_pos.z > boss_pos->z) {
        street_end = 0;
    } else {
        street_end = 1;
    }
    heading_away(&player_pos, (NJS_POINT3 *)&street_ends[street_end], &ang);
    heading = ang.y;
    /* @hack redundant: heading and a never-run store of it to ang.x, which
     * keep the image's load of ang.y into R4 (MOV.L @(4,R15),R4 at
     * 0x0c9026d2), where the call's &ang then goes; without it, 15 of 42
     * words differ */
    if (0) {
        ang.x = heading;
    }
    goal_from_player(&ang);
    goal_target = player_pos;
}

/* place the goal PLAYER_REACH from player 0 along ang->y. goal_height first
 * moves down toward 1, and the goal lies that far above player 0. a goal
 * outside the street comes back along the line to the first street edge
 * the line crosses. the goal then lies 1 above player 0. along_z is the
 * fraction of PLAYER_REACH at which the line meets the z edge. along_x is
 * the same for the x edge */
static void goal_from_player(struct angles *ang)
{
    Bool out;
    Float along_z;
    Float dist;
    Float along_x;

    goal_pos = player_pos;
    goal_pos.x += njSin(ang->y) * PLAYER_REACH;
    goal_pos.z += njCos(ang->y) * PLAYER_REACH;
    if ((goal_height -= HEIGHT_STEP) < 1.0F) {
        goal_height = 1.0F;
    }
    goal_pos.y += goal_height;
    if (outside_street(&goal_pos) == TRUE) {
        along_z = 1.0F;
        out = FALSE;
        if (goal_pos.z > STREET_Z_MAX) {
            out = TRUE;
            dist = njAbs(player_pos.z - STREET_Z_MAX);
        } else if (goal_pos.z < STREET_Z_MIN) {
            out = TRUE;
            dist = njAbs(player_pos.z - STREET_Z_MIN);
        }
        if (out == TRUE) {
            along_z = dist / njAbs(njCos(ang->y) * PLAYER_REACH);
            /* @hack spelling: along_x, reused for the distance to the z edge,
             * which SHC keeps at @R15 in a 4-byte frame (FMOV.S FR2,@R15 at
             * 0x0c902832). as dist, the object runs 32 bytes short and 214
             * of 217 words differ */
            along_x = along_z * PLAYER_REACH;
            goal_pos = player_pos;
            goal_pos.x += njSin(ang->y) * along_x;
            goal_pos.z += njCos(ang->y) * along_x;
            goal_pos.y += 1.0F;
        }
        out = FALSE;
        if (goal_pos.x > STREET_X_MAX) {
            out = TRUE;
            dist = njAbs(player_pos.x - STREET_X_MAX);
        } else if (goal_pos.x < STREET_X_MIN) {
            out = TRUE;
            dist = njAbs(player_pos.x - STREET_X_MIN);
        }
        if (out == TRUE) {
            along_x = dist / njAbs(njSin(ang->y) * PLAYER_REACH);
            if (along_z > along_x) {
                dist = along_x * PLAYER_REACH;
                goal_pos = player_pos;
                goal_pos.x += njSin(ang->y) * dist;
                goal_pos.z += njCos(ang->y) * dist;
                goal_pos.y += 1.0F;
            }
        }
    }
}

static Bool outside_street(NJS_POINT3 *pos)
{
    if (pos->x < STREET_X_MIN) {
        return TRUE;
    }
    if (pos->x > STREET_X_MAX) {
        return TRUE;
    }
    if (pos->z < STREET_Z_MIN) {
        return TRUE;
    }
    if (pos->z > STREET_Z_MAX) {
        return TRUE;
    }
    return FALSE;
}

/* set ang->y to the heading that leads from *from directly away from *to.
 * a heading's njSin steps x, and its njCos steps z */
static void heading_away(NJS_POINT3 *from, NJS_POINT3 *to, struct angles *ang)
{
    NJS_VECTOR diff;

    VECTOR_DIFF(&diff, to, from);
    ang->y = -QUARTER_TURN - (Angle)(atan2f(diff.z, diff.x) * ANGLE_PER_RADIAN);
}

/* move the camera toward its goals. while face_boss is set away from the
 * boss, goal_target is the boss's body. goal_pos then lies FACE_REACH from
 * player 0, on the side away from the body. the camera's target is
 * goal_target plus the shake. its position is the anchor's plus the shake.
 * the anchor then moves by pos_speed, which eases toward goal_pos */
static void follow_goals(void)
{
    Angle ang;
    NJS_VECTOR pull;
    struct camera_state *state;

    if (face_boss == TRUE && near_boss == FALSE) {
        goal_target = *egg_walker_part_pos(0);
        ang = -QUARTER_TURN - (Angle)(atan2f(goal_target.z - player_pos.z,
                                             goal_target.x - player_pos.x) *
                                      ANGLE_PER_RADIAN);
        goal_pos = player_pos;
        goal_pos.x += njSin(ang) * FACE_REACH;
        goal_pos.z += njCos(ang) * FACE_REACH;
    }
    VECTOR_SCALE_EACH(&target_speed, &target_speed, EASE_KEEP);
    target_speed.x += (goal_target.x - camera_state->target.x) * EASE_PULL;
    target_speed.y += (goal_target.y - camera_state->target.y) * EASE_PULL;
    /* @hack spelling: state set within the statement, which loads
     * camera_state's address into R6 for the store to target.x (MOV.L
     * @R4,R6 at 0x0c902b24); as two statements, it takes R5 and 2 of 313
     * words differ */
    target_speed.z += (goal_target.z - camera_state->target.z) * EASE_PULL,
        state = camera_state;
    state->target.x = goal_target.x;
    camera_state->target.y = goal_target.y;
    camera_state->target.z = goal_target.z;
    /* @hack redundant: an empty if (1), after which the shake's adds load
     * camera_state's address into R3 (MOV.L @R4,R3 at 0x0c902b42); without
     * it, they take R2 and 89 of 313 words differ */
    if (1) {
    }
    camera_state->target.x += shake.x;
    camera_state->target.y += shake.y;
    camera_state->target.z += shake.z;
    VECTOR_SCALE_EACH(&pos_speed, &pos_speed, EASE_KEEP);
    VECTOR_DIFF(&pull, &goal_pos, anchor_pos);
    VECTOR_SCALE(&pull, &pull, EASE_PULL);
    VECTOR_ADD_EACH(&pos_speed, &pull);
    camera_state->pos.x = anchor_pos->x;
    camera_state->pos.y = anchor_pos->y;
    camera_state->pos.z = anchor_pos->z;
    camera_state->pos.x += shake.x;
    camera_state->pos.y += shake.y;
    camera_state->pos.z += shake.z;
    VECTOR_ADD_EACH(anchor_pos, &pos_speed);
}

/* hold the camera's position in the street's bounds and above
 * CAMERA_FLOOR, and copy it to camera_pos. its target stays as it is */
static void keep_in_street(void)
{
    if (camera_state->pos.y < CAMERA_FLOOR) {
        camera_state->pos.y = CAMERA_FLOOR;
    }
    if (camera_state->pos.x < STREET_X_MIN) {
        camera_state->pos.x = STREET_X_MIN;
    }
    if (camera_state->pos.x > STREET_X_MAX) {
        camera_state->pos.x = STREET_X_MAX;
    }
    if (camera_state->pos.z < STREET_Z_MIN) {
        camera_state->pos.z = STREET_Z_MIN;
    }
    if (camera_state->pos.z > STREET_Z_MAX) {
        camera_state->pos.z = STREET_Z_MAX;
    }
    camera_pos.x = camera_state->pos.x;
    camera_pos.y = camera_state->pos.y;
    camera_pos.z = camera_state->pos.z;
}

void egg_walker_camera_face_boss(void)
{
    face_boss = TRUE;
}

void egg_walker_sky_draw(void)
{
    njSetTexture(&egm2_sky_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, camera_state->pos.x, camera_state->pos.y,
                camera_state->pos.z);
    draw_tree(&egm2_sky_object);
    njPopMatrix(1);
}

/* start the anchor task at player 0's position, and point anchor_pos at
 * it. with no task free, anchor_pos keeps what it held. nothing clears it
 * when the task ends */
static void anchor_create(void)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 3, anchor_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        work->pos = player_work[0]->pos;
        task->step = anchor_step;
        collision_init(task, anchor_shape, COLLISION_OBJECT_MUTUAL, 1, 1);
        anchor_pos = &work->pos;
    }
}

static void anchor_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void egg_walker_camera_shake(Uint8 power)
{
    shake.y += power * 0.5F;
}

static void shake_settle(void)
{
    NJS_VECTOR spring;

    VECTOR_SCALE_EACH(&shake_speed, &shake_speed, SHAKE_KEEP);
    VECTOR_SCALE_EACH(&spring, &shake, SHAKE_PULL);
    VECTOR_SUB_EACH(&shake_speed, &spring);
    VECTOR_ADD_EACH(&shake, &shake_speed);
}

NJS_POINT3 *egg_walker_camera_pos(void)
{
    return &camera_pos;
}

struct task_work *egg_walker_camera_target_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 3, swing_target_step)) != NULL) {
        work = task->work;
        work->pos = *pos;
        work->pos.y += SWING_RISE;
        task->step = swing_target_step;
        return work;
    }
    return NULL;
}

static void swing_target_step(struct task *task)
{
}

Bool egg_walker_camera_beyond_boss(void)
{
    return beyond_boss;
}
