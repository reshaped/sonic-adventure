/* the fight's camera. following the player, it stands behind the player
 * and looks ahead along its heading. the heading turns toward the center,
 * Zero. circling, the camera looks at the center from round it */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_robo/camera.h"
#include "b_robo/erobo.h"
#include "camera/camera.h"
#include "player/amy.h"
#include "player/player.h"
#include "system/math.h"
#include "system/task.h"
#include "system/util.h"

/* how far behind the player the camera stands, and how high above it */
#define DISTANCE 40.0F
#define HEIGHT 29.0F
/* how far ahead of the player the camera looks */
#define LOOK_AHEAD 40.0F

/* the least distance and height of EROBO_CAMERA_FAR */
#define FAR_LEAST 60.0F

/* the part of the heading's gap to the center that the camera turns a
 * step. EROBO_CAMERA_SLOW turns SLOW_RATE of it. EROBO_CAMERA_TURN,
 * EROBO_CAMERA_FAR and EROBO_CAMERA_SPEED turn TURN_RATE */
#define TURN_RATE 0.05F
#define SLOW_RATE 0.1F
/* the most the heading turns a step, in degrees. EROBO_CAMERA_TURN and
 * EROBO_CAMERA_FAR turn up to TURN_MOST. EROBO_CAMERA_SPEED turns up to
 * SPEED_TOP less the player's speed, and never less than SPEED_LEAST.
 * EROBO_CAMERA_SLOW turns up to SLOW_TOP less the player's speed. it turns
 * only while the player is no faster than SLOW_TOP */
#define TURN_MOST 3.0F
#define SPEED_TOP 2.0F
#define SPEED_LEAST 1.0F
#define SLOW_TOP 1.5F
/* the part of its turn EROBO_CAMERA_COAST keeps each step */
#define COAST_DRAG 0.98F

/* each step, the camera's distance closes DISTANCE_RATE of the way to its
 * goal. its height closes HEIGHT_RATE of the way, and the height it looks
 * at LOOK_RATE */
#define DISTANCE_RATE 0.04F
#define HEIGHT_RATE 0.02F
#define LOOK_RATE 0.02F
/* while view_lifted, the camera stands LIFTED_HEIGHT higher and
 * LIFTED_DISTANCE further off. its height then closes LIFTED_RATE of the way
 * a step */
#define LIFTED_HEIGHT 10.0F
#define LIFTED_DISTANCE 5.0F
#define LIFTED_RATE 0.3F

/* the circling camera's first speed, in degrees a step, and its distance
 * from the center with no reach */
#define CIRCLE_SPEED 0.7F
#define CIRCLE_NEAR 60.0F
/* how far above the center the circling camera stands. its distance closes
 * CIRCLE_RATE of the way to its goal each step */
#define CIRCLE_HEIGHT 5.0F
#define CIRCLE_RATE 0.03F
/* outside EROBO_CAMERA_CIRCLE, the part of its speed the circling keeps
 * each step */
#define CIRCLE_DRAG 0.98F

/* the larger of a and b, which it evaluates twice */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static NJS_POINT3 view_look;
static NJS_POINT3 view_pos;
static Float view_floor_y;
/* the camera's heading, and its turn this step */
static Angle view_heading;
static Angle view_turn;
/* how far the circling camera turns a step, in degrees */
static Float view_circle_speed;
static NJS_POINT3 *view_center;
/* player 0's speed across the deck, and the heading from the player to
 * the center */
static Float view_player_speed;
static Angle view_center_ang;
static enum erobo_camera_mode view_mode;
/* TRUE from Amy's hammer jump until she lands, while the camera stands
 * higher and further off */
static Bool view_lifted;
static Float view_height; /* how far above the floor the camera stands */
/* how far the camera stands from the player, or circling, from the
 * center */
static Float view_distance;
static Float view_lift; /* how far above the floor the camera looks */
static Float view_lift_target; /* the height view_lift moves toward */
/* how much further off the circling camera stands */
static Float view_reach;

static void view_step(struct camera_view *view);

/* the heading from from to to, across the deck */
static Angle view_angle_to(NJS_POINT3 *to, NJS_POINT3 *from)
{
    Float dx;
    Float dz;

    dx = to->x - from->x;
    dz = to->z - from->z;
    return angle_arc_tan2(dz, dx);
}

void erobo_camera_start(void)
{
    view_turn = 0;
    view_mode = EROBO_CAMERA_SLOW;
    view_distance = DISTANCE;
    view_height = HEIGHT;
    view_lift = 0.0F;
    view_lift_target = 0.0F;
    view_lifted = FALSE;
    camera_override(view_step, 0, CAMERA_LOOK_AT);
}

void erobo_camera_set_center(NJS_POINT3 *center)
{
    NJS_POINT3 *player = &player_work[0]->pos;

    view_center = center;
    view_heading = view_angle_to(center, player);
    view_floor_y = view_look.y = DECK_FLOOR_Y;
    view_pos.x = player->x;
    view_pos.z = player->z;
    view_pos.y = player->y;
}

void erobo_camera_set_lift(Float lift)
{
    view_lift_target = lift;
}

void erobo_camera_set_reach(Float reach)
{
    view_reach = reach;
}

void erobo_camera_set_mode(enum erobo_camera_mode mode)
{
    view_mode = mode;
    if (view_mode == EROBO_CAMERA_CIRCLE) {
        view_circle_speed = CIRCLE_SPEED;
        view_reach = 0.0F;
    }
}

Angle erobo_camera_center_gap(void)
{
    return angle_distance(view_heading, view_center_ang);
}

Angle erobo_camera_get_heading(void)
{
    return view_heading;
}

/* turn the heading toward the center as the mode says. then stand behind
 * the player on it, further off the faster the player runs. look ahead of
 * the player along it */
static void view_follow(void)
{
    struct task_work *work = player_work[0];
    NJS_POINT3 *player = &work->pos;
    Angle limit;
    Float sine;
    Float cosine;
    NJS_POINT3 near;
    NJS_POINT3 middle;
    Float far;
    Float high;
    Float rate;
    Float dz;

    view_center_ang = view_angle_to(view_center, player);
    switch (view_mode) {
    case EROBO_CAMERA_COAST:
        view_turn *= COAST_DRAG;
        view_heading += view_turn;
        break;
    case EROBO_CAMERA_TURN:
    case EROBO_CAMERA_FAR:
        view_turn =
            CLAMP(angle_difference(view_heading, view_center_ang) * TURN_RATE,
                  -(Angle)(TURN_MOST * ANGLE_PER_DEGREE),
                  (Angle)(TURN_MOST * ANGLE_PER_DEGREE));
        view_heading += view_turn;
        break;
    case EROBO_CAMERA_SPEED:
        limit =
            MAX((Angle)(SPEED_LEAST * ANGLE_PER_DEGREE),
                (Angle)((SPEED_TOP - view_player_speed) * ANGLE_PER_DEGREE));
        view_turn =
            CLAMP(angle_difference(view_heading, view_center_ang) * TURN_RATE,
                  -limit, limit);
        view_heading += view_turn;
        break;
    default:
        if (view_player_speed <= SLOW_TOP) {
            limit = (SLOW_TOP - view_player_speed) * ANGLE_PER_DEGREE;
            view_turn = CLAMP(angle_difference(view_heading, view_center_ang) *
                                  SLOW_RATE,
                              -limit, limit);
            view_heading += view_turn;
        }
        break;
    }
    far = DISTANCE;
    high = HEIGHT;
    njCalcPoint(NULL, player, &near);
    njCalcPoint(NULL, view_center, &middle);
    dz = near.z - middle.z;
    /* SHC folds 0.1F * 20.0F and 0.1F * 160.0F to just under 2 and 16,
     * 0x3fffffff and 0x417fffff */
    if (dz >= 30.0F) {
        /* @hack redundant: far set again to the DISTANCE it holds, which is
         * the FMOV FR12,FR15 in the slot of the BRA at 0x0c904dec; without
         * it, 59 of 432 words differ */
        far = DISTANCE;
    } else if (dz >= 10.0F) {
        far += (dz - 10.0F) * (0.1F * 20.0F);
    } else {
        /* @hack spelling: 10 - dz stored back in dz, which works it out in dz's
         * FR4 after copies at 0x0c904e00 and 0x0c904e02; as one expression,
         * 7 of 432 words differ */
        dz = 10.0F - dz;
        far += dz * 0.1F;
    }
    far += view_player_speed * (0.1F * 160.0F);
    view_lift_target = 0.0F;
    high += player->y - view_floor_y;
    /* @hack spelling: a switch of one case, which the image tests with CMP/EQ
     * #4,R0 (0x0c904e26); as an if, SHC picks other registers from
     * 0x0c904e4c and 4 of 432 words differ */
    switch (view_mode) {
    case EROBO_CAMERA_FAR:
        if (far <= FAR_LEAST) {
            far = FAR_LEAST;
        }
        if (high <= FAR_LEAST) {
            high = FAR_LEAST;
        }
        break;
    default:
        break;
    }
    /* @hack spelling: a switch of two cases and no default, which tests
     * view_lifted against 0 and 1 in turn (CMP/EQ at 0x0c904e40 and
     * 0x0c904e44). view_lifted is only ever set to FALSE or TRUE, so an if and
     * an else run the same; as if (!view_lifted) and an else, 17 of 432 words
     * differ */
#ifdef GCC
    /* GCC can't see that every path sets rate: view_lifted is only ever
     * FALSE or TRUE, as the tag says */
    rate = HEIGHT_RATE;
#endif
    switch (view_lifted) {
    case FALSE:
        if (amy_get_action(0) == AMY_HAMMER_JUMPING) {
            view_lifted = TRUE;
        }
        rate = HEIGHT_RATE;
        break;
    case TRUE:
        if (work->flags & PLAYER_ON_GROUND) {
            view_lifted = FALSE;
        }
        rate = LIFTED_RATE;
        high += LIFTED_HEIGHT;
        far += LIFTED_DISTANCE;
        break;
    }
    view_distance += (far - view_distance) * DISTANCE_RATE;
    view_height += (high - view_height) * rate;
    view_lift += (view_lift_target - view_lift) * LOOK_RATE;
    _builtin_fsca(view_heading, &sine, &cosine);
    view_look.x = player->x + (cosine * LOOK_AHEAD);
    view_look.z = player->z + (sine * LOOK_AHEAD);
    view_look.y = view_floor_y + view_lift;
    view_pos.x = player->x - (view_distance * cosine);
    view_pos.z = player->z - (view_distance * sine);
    view_pos.y = view_floor_y + view_height;
    camera_state->target.x = view_look.x;
    camera_state->target.y = view_look.y;
    camera_state->target.z = view_look.z;
    camera_state->pos.x = view_pos.x;
    camera_state->pos.y = view_pos.y;
    camera_state->pos.z = view_pos.z;
}

/* circle the center a step, looking at it. the camera's distance from the
 * center closes on CIRCLE_NEAR plus the reach. outside EROBO_CAMERA_CIRCLE,
 * the circling slows each step */
static void view_circle(void)
{
    Float sine;
    Float cosine;
    Float far;

    view_heading += (Angle)(view_circle_speed * ANGLE_PER_DEGREE);
    _builtin_fsca(view_heading, &sine, &cosine);
    view_look.x = view_center->x;
    view_look.z = view_center->z;
    view_look.y = view_center->y;
    view_pos.x = view_center->x + (cosine * view_distance);
    view_pos.z = view_center->z + (sine * view_distance);
    view_pos.y = view_center->y + CIRCLE_HEIGHT;
    if (view_mode != EROBO_CAMERA_CIRCLE) {
        view_circle_speed *= CIRCLE_DRAG;
    }
    far = view_reach + CIRCLE_NEAR;
    view_distance += (far - view_distance) * CIRCLE_RATE;
    camera_state->target.x = view_look.x;
    camera_state->target.y = view_look.y;
    camera_state->target.z = view_look.z;
    camera_state->pos.x = view_pos.x;
    camera_state->pos.y = view_pos.y;
    camera_state->pos.z = view_pos.z;
}

static void view_step(struct camera_view *view)
{
    view_player_speed = vector_magnitude_across(&player_motion[0]->speed);
    switch (view_mode) {
    case EROBO_CAMERA_CIRCLE:
    case EROBO_CAMERA_CIRCLE_OUT:
        view_circle();
        break;
    default:
        view_follow();
        break;
    }
}
