/* the gardens' camera. each step it aims ahead of player 0, and eases
 * camera_state's target toward the aim. garden_camera_pos is where the
 * camera is headed. its speed gathers pushes: in from too far and out from
 * too near, toward its height, and along with the aim's move. the
 * surfaces of the world's objects round it push it clear, and
 * camera_state's pos eases toward it */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "al_main/alife.h"
#include "al_main/camera.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "object/common.h"
#include "player/player.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the bound camera_looks_up tests pad_records[0]'s x1 and y1 against,
 * each shifted up 8 bits */
#define LOOK_UP_STICK 20

/* the aim lies AIM_AHEAD ahead of player 0, plus a tenth of camera_look's
 * rate, which grows with player 0's speed. its height is AIM_HEIGHT of
 * player 0's eye height */
#define AIM_AHEAD 5.0F
#define AIM_HEIGHT 0.7F
/* while the camera looks up, the aim rises LOOK_UP_RISE, and the camera
 * heads for LOOK_UP_DROP below the aim */
#define LOOK_UP_RISE 9.0F
#define LOOK_UP_DROP 12.0F
/* the share of the way to its goal camera_state's target eases each step.
 * along x and z it is TARGET_EASE times camera_look's rate, up TARGET_RISE
 * and down TARGET_FALL */
#define TARGET_EASE 0.1F
#define TARGET_RISE 0.035F
#define TARGET_FALL 0.13F

/* garden_camera_trail's bounds. past TRAIL_DRAW_IN the camera is drawn
 * in, and harder past TRAIL_DRAW_IN_HARD. within TRAIL_BACK_OFF it backs
 * off. past TRAIL_MAX, garden_camera_pos is set back to TRAIL_MAX. within
 * TRAIL_MIN, camera_state's pos is set out to TRAIL_MIN */
#define TRAIL_DRAW_IN 45.0F
#define TRAIL_DRAW_IN_HARD 55.0F
#define TRAIL_MAX 150.0F
#define TRAIL_BACK_OFF 30.0F
#define TRAIL_MIN 23.0F
/* each step's push toward the aim: DRAW_IN_PUSH past TRAIL_DRAW_IN, and
 * DRAW_IN_HARD_PUSH past TRAIL_DRAW_IN_HARD. within TRAIL_BACK_OFF, the
 * push away from it is BACK_OFF_PUSH */
#define DRAW_IN_PUSH 0.12F
#define DRAW_IN_HARD_PUSH 0.37F
#define BACK_OFF_PUSH 0.01F
/* the camera heads for 1 above the aim, and ZOOM_HEIGHT more for each
 * zoom, or for LOOK_UP_DROP below the aim. its push toward that height is
 * HEIGHT_RISE of the distance up to it, or HEIGHT_FALL of the distance
 * down */
#define ZOOM_HEIGHT 1.2F
#define HEIGHT_RISE 0.03F
#define HEIGHT_FALL 0.02F
/* the share of the aim's move since the last step the push follows */
#define AIM_FOLLOW 0.1F

/* a wall that faces the camera pushes it out while it lies within
 * WALL_CLEAR of the wall's plane. each step's push is WALL_PUSH of the
 * depth inside that */
#define WALL_CLEAR 5.0F
#define WALL_PUSH 0.35F
/* a ceiling within CEILING_CLEAR above the camera holds it CEILING_HOLD
 * under the ceiling */
#define CEILING_CLEAR 3.0F
#define CEILING_HOLD 2.7F
/* a floor within FLOOR_CLEAR below the camera lifts it FLOOR_LIFT of the
 * way to FLOOR_CLEAR above the floor */
#define FLOOR_CLEAR 3.0F
#define FLOOR_LIFT 0.7F
/* while the floor under the camera faces up, a ceiling within LOW_CEILING
 * above the camera counts as its floor */
#define LOW_CEILING 8.0F
/* the level camera_find_sides gives a side with no surface: -SIDE_FAR
 * below and on the minus sides, SIDE_FAR above and on the plus sides */
#define SIDE_FAR 1000000.0F

/* the most garden_camera_speed.y takes up or down, before each step's
 * drag */
#define RISE_MAX 1.0F
/* the share of the way to its goal camera_state's pos eases each step:
 * POS_EASE, or FIXED_POS_EASE while garden_camera_fixed is set */
#define POS_EASE 0.1F
#define FIXED_POS_EASE 0.15F
/* the height garden_camera_pos stays under in Station Square's garden */
#define STATION_SQUARE_CEILING 75.0F

/* the zooms, 0 to ZOOM_WIDEST, where the camera starts. zoom n's field of
 * view is ZOOM_FOV + n * ZOOM_FOV_STEP degrees: 30, 49 or 68 */
#define ZOOM_WIDEST 2
#define ZOOM_FOV 30.0F
#define ZOOM_FOV_STEP 19

/* garden_camera_fixed_angle's step, and its value at the fixed view, half
 * a turn */
#define FIXED_TURN 0x300
#define FIXED_ANGLE 0x8000

/* how far from player 0 the camera starts, and where it starts headed */
#define START_DISTANCE 20.0F

Bool garden_camera_still;
Uint16 garden_camera_zoom;
NJS_POINT3 garden_camera_aim;
NJS_POINT3 garden_camera_last_aim;
NJS_POINT3 garden_camera_pos;
NJS_VECTOR garden_camera_speed;
NJS_VECTOR garden_camera_accel;
NJS_VECTOR dat_0cb1e4fc[4];
Float garden_camera_trail;
Sint32 dat_0cb1e530;
Sint32 dat_0cb1e534;
Sint32 dat_0cb1e538;
Bool garden_camera_fixed;
Angle garden_camera_fixed_angle;
NJS_POINT3 garden_camera_fixed_pos;
NJS_POINT3 garden_camera_fixed_target;
/* the surfaces round garden_camera_pos, as camera_keep_clear last found
 * them */
static struct sides_query camera_sides;
/* 4 bytes no code reads or writes */
static Sint32 dat_0cb1e628;
/* player 0's eye height and record, as the camera's first step took them */
static Float camera_eye_height;
static struct player *camera_player;
/* 548 bytes no code reads or writes */
static Uint8 dat_0cb1e634[0x224];

/* TRUE while player 0 touches a CARRY_KIND_SHAKE thing, as a garden's
 * tree is, with the stick pushed and B, X or Y held */
static Bool camera_looks_up(void)
{
    struct task_work *work = player_work[0];
    struct collision_work *hit = work->collision->hit;

    if (hit != NULL && hit->owner->work->id == CARRY_KIND_SHAKE) {
        /* @bug logic: x1 and y1 shifted up 8 bits are multiples of 256, so
         * these tests pass for any reading but 0, and LOOK_UP_STICK separates
         * no push from another (SHLL8 at 0x0c902e42 and 0x0c902e50, MOV #20
         * at 0x0c902e48). the bound reads as meant for the unshifted reading,
         * an inference */
        if ((pad_records[0].x1 << 8) <= -LOOK_UP_STICK ||
            (pad_records[0].x1 << 8) >= LOOK_UP_STICK ||
            (pad_records[0].y1 << 8) <= -LOOK_UP_STICK ||
            (pad_records[0].y1 << 8) >= LOOK_UP_STICK) {
            if (pad_records[0].on & PDD_DGT_TX ||
                pad_records[0].on & PDD_DGT_TB ||
                pad_records[0].on & PDD_DGT_TY) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* aim ahead of player 0, farther the faster it goes. then, unless
 * garden_camera_still holds the camera, ease camera_state's target toward
 * the aim, or toward garden_camera_fixed_target as the view turns fixed.
 * the target eases faster along x and z the faster player 0 goes, and it
 * falls faster than it rises */
static void camera_look(void)
{
    struct task_work *work = player_work[0];
    Float rate;
    Float ahead;
    Float share;
    Float rest;
    Float x;
    Float y;
    Float z;
    Float dx;
    Float dy;
    Float dz;

    rate = (njScalor(&camera_player->speed) * 3.0F) + 0.15F;
    ahead = (rate * 0.1F) + AIM_AHEAD;
    garden_camera_aim.x = work->pos.x + (ahead * njCos(work->ang.y));
    if (camera_looks_up()) {
        garden_camera_aim.y =
            work->pos.y + (camera_eye_height * AIM_HEIGHT) + LOOK_UP_RISE;
    } else {
        garden_camera_aim.y = work->pos.y + (camera_eye_height * AIM_HEIGHT);
    }
    garden_camera_aim.z = work->pos.z + (ahead * njSin(work->ang.y));
    share = (njCos(garden_camera_fixed_angle) + 1.0F) * 0.5F;
    rest = 1.0F - share;
    x = (garden_camera_aim.x * share) + (garden_camera_fixed_target.x * rest);
    y = (garden_camera_aim.y * share) + (garden_camera_fixed_target.y * rest);
    z = (garden_camera_aim.z * share) + (garden_camera_fixed_target.z * rest);
    dx = x - camera_state->target.x;
    /* @hack spelling: dy in two statements, which subtracts in y's register,
     * FSUB FR3,FR4 at 0x0c902f74; as one statement, the function runs 32
     * bytes longer and 68 of 150 words differ */
    dy = y;
    dy -= camera_state->target.y;
    dz = z - camera_state->target.z;
    if (!garden_camera_still) {
        dx *= rate;
        camera_state->target.x = camera_state->target.x + (dx * TARGET_EASE);
        if (dy > 0.0F) {
            camera_state->target.y =
                camera_state->target.y + (dy * TARGET_RISE);
        } else {
            camera_state->target.y =
                camera_state->target.y + (dy * TARGET_FALL);
        }
        dz *= rate;
        camera_state->target.z = camera_state->target.z + (dz * TARGET_EASE);
    }
}

/* L or R swings garden_camera_pos about the aim, level, L one way and R
 * the other. the shorter garden_camera_trail is, the more it swings a
 * step */
static void camera_swing(void)
{
    Angle turn;
    NJS_VECTOR from;
    NJS_VECTOR to;
    Angle step =
        (Angle)(97.5F / (garden_camera_trail + 15.0F) * ANGLE_PER_DEGREE);

    if (pad_records[0].on & PDD_DGT_TL) {
        from.x = garden_camera_pos.x - garden_camera_aim.x;
        from.y = 0.0F;
        from.z = garden_camera_pos.z - garden_camera_aim.z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, -step, turn);
        njCalcVector(NULL, &from, &to);
        njPopMatrix(1);
        garden_camera_pos.x = garden_camera_aim.x + to.x;
        garden_camera_pos.z = garden_camera_aim.z + to.z;
    } else if (pad_records[0].on & PDD_DGT_TR) {
        from.x = garden_camera_pos.x - garden_camera_aim.x;
        from.y = 0.0F;
        from.z = garden_camera_pos.z - garden_camera_aim.z;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateY, step);
        njCalcVector(NULL, &from, &to);
        njPopMatrix(1);
        garden_camera_pos.x = garden_camera_aim.x + to.x;
        garden_camera_pos.z = garden_camera_aim.z + to.z;
    }
}

/* measure garden_camera_trail, and gather this step's push by its bounds.
 * the speed drags hardest between TRAIL_BACK_OFF and TRAIL_DRAW_IN. the
 * camera rises or falls toward its height over the aim, which rises with
 * the zoom. it heads for LOOK_UP_DROP below the aim instead while it looks
 * up. it also follows the aim's move since the last step */
static void camera_trail_aim(void)
{
    NJS_VECTOR to;

    to.x = garden_camera_aim.x - garden_camera_pos.x;
    to.y = 0.0F;
    to.z = garden_camera_aim.z - garden_camera_pos.z;
    garden_camera_trail = njScalor(&to);
    if (garden_camera_trail > TRAIL_DRAW_IN) {
        if (njScalor(&to) == 0.0F) {
            to.x = 1.0F;
            to.y = 0.0F;
            to.z = 0.0F;
        } else {
            njUnitVector(&to);
        }
        if (garden_camera_trail < TRAIL_DRAW_IN_HARD) {
            garden_camera_accel.x += to.x * DRAW_IN_PUSH;
            /* @hack redundant: 0 added to accel.y, which keeps the two
             * arms' shared FADD FR15,FR2 at 0x0c9031dc; without it, 96 of
             * 334 words differ */
            garden_camera_accel.y += 0.0F;
            garden_camera_accel.z += to.z * DRAW_IN_PUSH;
        } else {
            garden_camera_accel.x += to.x * DRAW_IN_HARD_PUSH;
            /* @hack redundant: 0 added to accel.y, which keeps the two
             * arms' shared FADD FR15,FR2 at 0x0c9031dc; without it, 96 of
             * 334 words differ */
            garden_camera_accel.y += 0.0F;
            garden_camera_accel.z += to.z * DRAW_IN_HARD_PUSH;
        }
        if (garden_camera_trail > TRAIL_MAX) {
            garden_camera_pos.x = garden_camera_aim.x - (to.x * TRAIL_MAX);
            garden_camera_pos.z = garden_camera_aim.z - (to.z * TRAIL_MAX);
        }
        garden_camera_accel.x += -(garden_camera_speed.x * 0.145F);
        garden_camera_accel.y += -(garden_camera_speed.y * 0.2F);
        garden_camera_accel.z += -(garden_camera_speed.z * 0.145F);
    } else if (garden_camera_trail < TRAIL_BACK_OFF) {
        if (njScalor(&to) == 0.0F) {
            to.x = 1.0F;
            to.y = 0.0F;
            to.z = 0.0F;
        } else {
            njUnitVector(&to);
        }
        garden_camera_accel.x += -(to.x * BACK_OFF_PUSH);
        /* @hack redundant: 0 added to accel.y, which keeps FADD FR15,FR3 at
         * 0x0c9032fe; without it, 190 of 334 words differ */
        garden_camera_accel.y += 0.0F;
        garden_camera_accel.z += -(to.z * BACK_OFF_PUSH);
        if (garden_camera_trail < TRAIL_MIN) {
            camera_state->pos.x = garden_camera_aim.x - (to.x * TRAIL_MIN);
            camera_state->pos.z = garden_camera_aim.z - (to.z * TRAIL_MIN);
        }
        garden_camera_accel.x += -(garden_camera_speed.x * 0.2F);
        garden_camera_accel.y += -(garden_camera_speed.y * 0.15F);
        garden_camera_accel.z += -(garden_camera_speed.z * 0.2F);
    } else {
        garden_camera_accel.x += -(garden_camera_speed.x * 0.3F);
        garden_camera_accel.y += -(garden_camera_speed.y * 0.4F);
        garden_camera_accel.z += -(garden_camera_speed.z * 0.3F);
    }
    if (camera_looks_up()) {
        to.y = garden_camera_aim.y - camera_state->pos.y - LOOK_UP_DROP;
    } else {
        to.y = garden_camera_aim.y + (garden_camera_zoom * ZOOM_HEIGHT) -
               camera_state->pos.y + 1.0F;
    }
    if (to.y > 0.0F) {
        garden_camera_accel.y += to.y * HEIGHT_RISE;
    } else {
        garden_camera_accel.y += to.y * HEIGHT_FALL;
    }
    garden_camera_accel.x +=
        (garden_camera_aim.x - garden_camera_last_aim.x) * AIM_FOLLOW;
    garden_camera_accel.z +=
        (garden_camera_aim.z - garden_camera_last_aim.z) * AIM_FOLLOW;
    garden_camera_last_aim = garden_camera_aim;
}

/* set each side of result->pos to the nearest surface of the world's
 * objects that world_list_near lists for it, at a radius of 1. water
 * counts on no side. a task's object counts only while the point lies
 * within its model's radius, times its largest scale, of the object's
 * position */
static void camera_find_sides(struct sides_query *result)
{
    NJS_VECTOR away;
    struct sides_query query;
    NJS_POINT3 *pos = &result->pos;
    struct world_object *objects;
    NJS_OBJECT *object;
    Float distance;
    Float size;
    Float was;
    Float now;
    int side;
    Uint16 i;

    query.pos = result->pos;
    for (side = 0; side < SIDES; side++) {
        result->sides[side].found = FALSE;
    }
    result->sides[SIDE_MINUS_X].level = -SIDE_FAR;
    result->sides[SIDE_PLUS_X].level = SIDE_FAR;
    result->sides[SIDE_BELOW].level = -SIDE_FAR;
    result->sides[SIDE_ABOVE].level = SIDE_FAR;
    result->sides[SIDE_MINUS_Z].level = -SIDE_FAR;
    result->sides[SIDE_PLUS_Z].level = SIDE_FAR;
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, 1.0F);
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner != NULL) {
            away.x = object->pos[0] - pos->x;
            away.y = object->pos[1] - pos->y;
            away.z = object->pos[2] - pos->z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (object->scl[1] > size) {
                size = object->scl[1];
            }
            if (object->scl[2] > size) {
                size = object->scl[2];
            }
            if (distance > size * object->model->r) {
                continue;
            }
        }
        if (objects[i].flags & WORLD_WATER) {
            continue;
        }
        if (polygon_find_sides(&query, objects[i].object, objects[i].flags)) {
            if (query.sides[SIDE_MINUS_X].found) {
                was = njAbs(pos->x - result->sides[SIDE_MINUS_X].level);
                now = njAbs(pos->x - query.sides[SIDE_MINUS_X].level);
                if (was > now) {
                    result->sides[SIDE_MINUS_X] = query.sides[SIDE_MINUS_X];
                }
            }
            if (query.sides[SIDE_PLUS_X].found) {
                was = njAbs(result->sides[SIDE_PLUS_X].level - pos->x);
                now = njAbs(query.sides[SIDE_PLUS_X].level - pos->x);
                if (now < was) {
                    result->sides[SIDE_PLUS_X] = query.sides[SIDE_PLUS_X];
                }
            }
            if (query.sides[SIDE_BELOW].found) {
                was = njAbs(pos->y - result->sides[SIDE_BELOW].level);
                now = njAbs(pos->y - query.sides[SIDE_BELOW].level);
                if (was > now) {
                    result->sides[SIDE_BELOW] = query.sides[SIDE_BELOW];
                }
            }
            if (query.sides[SIDE_ABOVE].found) {
                was = njAbs(result->sides[SIDE_ABOVE].level - pos->y);
                now = njAbs(query.sides[SIDE_ABOVE].level - pos->y);
                if (was > now) {
                    result->sides[SIDE_ABOVE] = query.sides[SIDE_ABOVE];
                }
            }
            if (query.sides[SIDE_MINUS_Z].found) {
                was = njAbs(pos->z - result->sides[SIDE_MINUS_Z].level);
                now = njAbs(pos->z - query.sides[SIDE_MINUS_Z].level);
                if (was > now) {
                    result->sides[SIDE_MINUS_Z] = query.sides[SIDE_MINUS_Z];
                }
            }
            if (query.sides[SIDE_PLUS_Z].found) {
                was = njAbs(result->sides[SIDE_PLUS_Z].level - pos->z);
                now = njAbs(query.sides[SIDE_PLUS_Z].level - pos->z);
                if (was > now) {
                    result->sides[SIDE_PLUS_Z] = query.sides[SIDE_PLUS_Z];
                }
            }
        }
    }
}

/* push garden_camera_pos along wall's normal, level, by WALL_PUSH of depth.
 * push takes the move */
/* @hack spelling: scale set and scaled in one comma expression, which copies
 * depth into it first, FMOV FR5,FR4 at 0x0c90392a; as one product, depth
 * times WALL_PUSH, 555 of 809 words differ */
#define PUSH_CLEAR(wall, depth, push)                                          \
    {                                                                          \
        Float scale;                                                           \
                                                                               \
        scale = (depth), scale *= WALL_PUSH;                                   \
        (push).x = (wall)->normal.x;                                           \
        (push).y = 0.0F;                                                       \
        (push).z = (wall)->normal.z;                                           \
        (push).x *= scale;                                                     \
        (push).y *= scale;                                                     \
        (push).z *= scale;                                                     \
        njAddVector(&garden_camera_pos, &(push));                              \
    }

/* the ways from the camera toward the wall on each level side, and up */
static const NJS_VECTOR toward_minus_x = {-1.0F, 0.0F, 0.0F};
static const NJS_VECTOR toward_plus_x = {1.0F, 0.0F, 0.0F};
static const NJS_VECTOR toward_minus_z = {0.0F, 0.0F, -1.0F};
static const NJS_VECTOR toward_plus_z = {0.0F, 0.0F, 1.0F};
static const NJS_VECTOR toward_up = {0.0F, 1.0F, 0.0F};

/* push garden_camera_pos clear of the surfaces round it. the wall on each
 * level side pushes it along the wall's normal, level. a wall faces the
 * camera when its normal and the way toward it have a dot product below 0.
 * a wall that faces away pushes the camera toward its front, by WALL_PUSH of
 * its distance plus WALL_CLEAR. a ceiling then holds the camera down, and a
 * floor lifts it */
static void camera_keep_clear(void)
{
    struct sides_query *query = &camera_sides;
    NJS_PLANE plane;
    NJS_POINT3 pos;
    NJS_VECTOR toward;
    NJS_POINT3 nearest;
    NJS_VECTOR push;
    Float depth;
    Float floor;
    Float ceiling;

    query->pos = garden_camera_pos;
    camera_find_sides(query);
    pos = query->pos;
    if (query->sides[SIDE_MINUS_X].found) {
        struct surface *wall;

        toward = toward_minus_x;
        wall = &query->sides[SIDE_MINUS_X];
        /* @hack spelling: the bound of this test and of the three like it below
         * spelled -0.0F, which SHC loads once into FR12 (FMOV.S at 0x0c9038a4,
         * from 0x0c903ac8) and compares with FCMP/GT FR0,FR12 (0x0c9038da). as
         * 0.0F, 459 of 809 words differ */
        if (njInnerProduct(&toward, &wall->normal) < -0.0F) {
            plane.px = wall->level;
            plane.py = pos.y;
            plane.pz = pos.z;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            /* @hack spelling: depth assigned in the test, which gives
             * WALL_CLEAR FR13 and WALL_PUSH FR15, FADD FR13,FR5 at
             * 0x0c903920; as two statements, the two trade and 16 of 809
             * words differ */
            if ((depth = -njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR) >
                0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        } else if (njInnerProduct(&toward, &wall->normal) > 0.0F) {
            plane.px = wall->level;
            plane.py = pos.y;
            plane.pz = pos.z;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            depth = njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR;
            if (depth > 0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        }
    }
    if (query->sides[SIDE_PLUS_X].found) {
        struct surface *wall;

        toward = toward_plus_x;
        wall = &query->sides[SIDE_PLUS_X];
        if (njInnerProduct(&toward, &wall->normal) < -0.0F) {
            plane.px = wall->level;
            plane.py = pos.y;
            plane.pz = pos.z;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            depth = -njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR;
            if (depth > 0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        } else if (njInnerProduct(&toward, &wall->normal) > 0.0F) {
            plane.px = wall->level;
            plane.py = pos.y;
            plane.pz = pos.z;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            depth = njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR;
            if (depth > 0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        }
    }
    if (query->sides[SIDE_MINUS_Z].found) {
        struct surface *wall;

        toward = toward_minus_z;
        wall = &query->sides[SIDE_MINUS_Z];
        if (njInnerProduct(&toward, &wall->normal) < -0.0F) {
            plane.px = pos.x;
            plane.py = pos.y;
            plane.pz = wall->level;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            depth = -njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR;
            if (depth > 0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        } else if (njInnerProduct(&toward, &wall->normal) > 0.0F) {
            plane.px = pos.x;
            plane.py = pos.y;
            plane.pz = wall->level;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            depth = njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR;
            if (depth > 0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        }
    }
    if (query->sides[SIDE_PLUS_Z].found) {
        struct surface *wall;

        toward = toward_plus_z;
        wall = &query->sides[SIDE_PLUS_Z];
        if (njInnerProduct(&toward, &wall->normal) < -0.0F) {
            plane.px = pos.x;
            plane.py = pos.y;
            plane.pz = wall->level;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            depth = -njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR;
            if (depth > 0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        } else if (njInnerProduct(&toward, &wall->normal) > 0.0F) {
            plane.px = pos.x;
            plane.py = pos.y;
            plane.pz = wall->level;
            plane.vx = wall->normal.x;
            plane.vy = wall->normal.y;
            plane.vz = wall->normal.z;
            depth = njDistanceP2PL(&pos, &plane, &nearest) + WALL_CLEAR;
            if (depth > 0.0F) {
                PUSH_CLEAR(wall, depth, push);
            }
        }
    }
    if (garden_camera_pos.y + CEILING_CLEAR > query->sides[SIDE_ABOVE].level) {
        garden_camera_pos.y = query->sides[SIDE_ABOVE].level - CEILING_HOLD;
    }
    toward = toward_up;
    ceiling = query->sides[SIDE_ABOVE].level;
    floor = query->sides[SIDE_BELOW].level;
    if (njInnerProduct(&query->sides[SIDE_BELOW].normal, &toward) > 0.0F) {
        if (garden_camera_pos.y + LOW_CEILING > ceiling) {
            floor = ceiling;
        }
    }
    if (floor > garden_camera_pos.y - FLOOR_CLEAR) {
        garden_camera_pos.y +=
            (floor - garden_camera_pos.y + FLOOR_CLEAR) * FLOOR_LIFT;
    }
}

/* move garden_camera_pos by its speed, unless garden_camera_still holds
 * it, and push it clear. then ease camera_state's pos toward it, or toward
 * garden_camera_fixed_pos as the view turns fixed. the speed takes the
 * step's push first, and its y is held within RISE_MAX and dragged. while
 * the view is fixed, garden_camera_fixed_angle turns toward FIXED_ANGLE,
 * and the camera eases faster. otherwise up and down on the pad pick the
 * zoom, and the angle turns back toward 0 */
static void camera_move(void)
{
    Float share;
    Float rest;
    Float rate;
    Float x;
    Float y;
    Float z;

    garden_camera_speed.x += garden_camera_accel.x;
    garden_camera_speed.y += garden_camera_accel.y;
    garden_camera_speed.z += garden_camera_accel.z;
    if (garden_camera_speed.y > RISE_MAX) {
        garden_camera_speed.y = RISE_MAX;
    }
    if (garden_camera_speed.y < -RISE_MAX) {
        garden_camera_speed.y = -RISE_MAX;
    }
    garden_camera_speed.y *= 0.92F;
    if (!garden_camera_still) {
        garden_camera_pos.x += garden_camera_speed.x;
        garden_camera_pos.y += garden_camera_speed.y;
        garden_camera_pos.z += garden_camera_speed.z;
    }
    camera_keep_clear();
    if (garden_camera_fixed) {
        garden_camera_fixed_angle =
            angle_approach(garden_camera_fixed_angle, FIXED_ANGLE, FIXED_TURN);
        rate = FIXED_POS_EASE;
    } else {
        if (pad_records[0].press & PDD_DGT_KU) {
            if (garden_camera_zoom > 0) {
                garden_camera_zoom--;
            }
        }
        if (pad_records[0].press & PDD_DGT_KD) {
            if (garden_camera_zoom < ZOOM_WIDEST) {
                garden_camera_zoom++;
            }
        }
        camera_set_perspective(
            (Sint32)(((garden_camera_zoom * ZOOM_FOV_STEP) + ZOOM_FOV) *
                     ANGLE_PER_DEGREE));
        garden_camera_fixed_angle =
            angle_approach(garden_camera_fixed_angle, 0, FIXED_TURN);
        rate = POS_EASE;
    }
    share = (njCos(garden_camera_fixed_angle) + 1.0F) * 0.5F;
    rest = 1.0F - share;
    if (alife_garden == GARDEN_STATION_SQUARE) {
        if (garden_camera_pos.y > STATION_SQUARE_CEILING) {
            garden_camera_pos.y = STATION_SQUARE_CEILING;
        }
    }
    x = (garden_camera_pos.x * share) + (garden_camera_fixed_pos.x * rest);
    y = (garden_camera_pos.y * share) + (garden_camera_fixed_pos.y * rest);
    z = (garden_camera_pos.z * share) + (garden_camera_fixed_pos.z * rest);
    camera_state->pos.x =
        ((x - camera_state->pos.x) * rate) + camera_state->pos.x;
    camera_state->pos.y =
        ((y - camera_state->pos.y) * rate) + camera_state->pos.y;
    camera_state->pos.z =
        ((z - camera_state->pos.z) * rate) + camera_state->pos.z;
    garden_camera_accel.x = 0.0F;
    garden_camera_accel.y = 0.0F;
    garden_camera_accel.z = 0.0F;
}

/* start the camera at rest, at player 0's eye height, with fog off.
 * camera_state's pos stands START_DISTANCE to player 0's side, and
 * garden_camera_pos START_DISTANCE behind it. the aim and target start
 * AIM_AHEAD from player 0 at the angle 0x4000 less its y angle, where
 * camera_look aims along its y angle itself. clear garden_camera_fixed
 * and garden_camera_still, and take the widest zoom */
static void camera_reset(void)
{
    struct task_work *work = player_work[0];
    Angle ang = -work->ang.y + 0x4000;

    camera_player = player_get(0);
    camera_eye_height = camera_player->consts.eye_height;
    camera_state->pos.x = work->pos.x + (START_DISTANCE * njCos(ang));
    camera_state->pos.y = camera_eye_height + work->pos.y;
    camera_state->pos.z = -(START_DISTANCE * njSin(ang)) + work->pos.z;
    garden_camera_speed.x = 0.0F;
    garden_camera_speed.y = 0.0F;
    garden_camera_speed.z = 0.0F;
    garden_camera_pos.x = work->pos.x + (START_DISTANCE * njCos(ang + 0x4000));
    garden_camera_pos.y = camera_eye_height + work->pos.y;
    garden_camera_pos.z = -(START_DISTANCE * njSin(ang + 0x4000)) + work->pos.z;
    garden_camera_aim.x = work->pos.x + (AIM_AHEAD * njCos(ang));
    garden_camera_aim.y = work->pos.y + (AIM_HEIGHT * camera_eye_height);
    garden_camera_aim.z = work->pos.z + (AIM_AHEAD * njSin(ang));
    camera_state->target.x = garden_camera_aim.x;
    camera_state->target.y = garden_camera_aim.y;
    camera_state->target.z = garden_camera_aim.z;
    garden_camera_last_aim = garden_camera_aim;
    njFogDisable();
    dat_0cb1e530 = 0;
    dat_0cb1e534 = 0;
    dat_0cb1e538 = 0;
    garden_camera_fixed = FALSE;
    garden_camera_still = FALSE;
    garden_camera_zoom = ZOOM_WIDEST;
}

void garden_camera_step(struct camera_view *view)
{
    if (view->steps == 0) {
        camera_reset();
    }
    camera_look();
    camera_trail_aim();
    camera_swing();
    camera_move();
    njFogDisable();
}
