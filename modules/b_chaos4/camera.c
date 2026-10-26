/* Chaos 4's cameras, and the dome round the swamp. before the fight, one
 * camera climbs while he is under the swamp, and another circles him. the
 * fight's camera sits behind player 0 and looks past him at Chaos 4. while
 * a wall stands near player 0, it swings round to where there is room */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/camera.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/textures.h"
#include "b_chaos4/util.h"
#include "camera/camera.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/util.h"

/* the fight's camera follows a point that trails player 0. each step the
 * point halves its gap to him, but while he is more than FOLLOW_SLACK
 * away, it moves as he does. the camera sits camera_distance out from the
 * point and CAMERA_HEIGHT above it. it looks at a spot LOOK_BEYOND past the
 * point, on the far side from the camera */
#define CAMERA_HEIGHT 20.0F
#define LOOK_BEYOND 20.0F
#define FOLLOW_SLACK 30.0F
/* the share of the way to its place the camera moves on a frame's first
 * step */
#define CAMERA_EASE 0.21F
/* the share of its turn to its aim the camera makes a step. it takes
 * TURN_SLOW while player 0 moves SLOW_TURN_SPEED a step or more and the
 * point it follows is above SLOW_TURN_HEIGHT, and TURN_FAST otherwise */
#define TURN_FAST 0.1F
#define TURN_SLOW 0.01F
#define SLOW_TURN_SPEED 0.5F
#define SLOW_TURN_HEIGHT 30.0F
/* the lowest the fight's camera sits */
#define CAMERA_FLOOR 30.0F

/* player 0 has room while no wall stands within PLAYER_ROOM of him in x or
 * z. without room, the search tries points where the camera could sit,
 * SEARCH_REACH out from him and SEARCH_TURN apart, up to SEARCH_STEPS
 * turns each way. a point has room while no wall stands within SEARCH_ROOM
 * of it */
#define PLAYER_ROOM 45.0F
#define SEARCH_REACH 45.0F
#define SEARCH_STEPS 10
#define SEARCH_TURN NJM_DEG_ANG(15)
#define SEARCH_ROOM 30.0F

/* while Chaos 4 is below SWAMP_Y, the swamp's surface, the rising camera
 * climbs RISE_SPEED a step, up to RISE_TOP above rise_pos */
#define RISE_SPEED 2.5F
#define RISE_TOP 200.0F

/* the circling camera's distance, its first angle and its turn a step */
#define ORBIT_DISTANCE 50.0F
#define ORBIT_START NJM_DEG_ANG(30)
#define ORBIT_TURN NJM_DEG_ANG(0.5)

#define THREE_QUARTER_TURN NJM_DEG_ANG(270)

/* @hack spelling: util.h's VECTOR_DIFF, VECTOR_SCALE and VECTOR_ADD as a
 * statement for each member, not one comma expression, which keeps
 * follow's address in both R2 and R3 (0x0c901f24 to 0x0c901f2a). as
 * util.h's comma expressions, camera.c's code runs 32 bytes long, and
 * summed over follow_player and take_aim, 131 of 348 words differ */
#define VECTOR_DIFF_EACH(out, a, b)                                            \
    do {                                                                       \
        (out)->x = (a)->x - (b)->x;                                            \
        (out)->y = (a)->y - (b)->y;                                            \
        (out)->z = (a)->z - (b)->z;                                            \
    } while (0)
#define VECTOR_SCALE_EACH(out, v, s)                                           \
    do {                                                                       \
        (out)->x = (v)->x * (s);                                               \
        (out)->y = (v)->y * (s);                                               \
        (out)->z = (v)->z * (s);                                               \
    } while (0)
#define VECTOR_ADD_EACH(v, d)                                                  \
    do {                                                                       \
        (v)->x = (v)->x + (d)->x;                                              \
        (v)->y = (v)->y + (d)->y;                                              \
        (v)->z = (v)->z + (d)->z;                                              \
    } while (0)
/* @hack spelling: the square of member m's difference, read through pointers
 * a and b, as follow_player passes &player_last and &follow for b. read
 * from the variables themselves, follow_player runs 32 bytes short, and
 * 262 of 267 words differ */
#define SQUARE_DIFF(a, b, m) SQUARE((a)->m - (b)->m)
/* the shorter turn from angle b to angle a, from -0x7fff to 0x8000 */
#define ANGLE_TURN(a, b)                                                       \
    ((Uint16)((a) - (b)) <= 0x8000 ? (Uint16)((a) - (b))                       \
                                   : (Uint16)((a) - (b)) - 0x10000)

static void fight_step(struct camera_view *view);
static void follow_player(void);
static void place_camera(void);
static Bool player_has_room(void);
static void search_for_room(void);
static void keep_above_floor(void);
static Angle take_aim(void);
static void turn_to_aim(void);
static Bool wall_near(NJS_POINT3 *pos, Float room);
static void rise_step(struct camera_view *view);
static void orbit_step(struct camera_view *view);

/* how far out from the point it follows the fight's camera sits. nothing
 * changes it */
static Float camera_distance = 45.0F;
/* the side the search swings the camera to: TRUE the way its angle grows,
 * FALSE the other way */
static Bool search_up = FALSE;

static NJS_MATERIAL dome_materials[3] = {
#include "b_chaos4/camera/dome_materials.inc"
};

static Sint16 dome_wall_meshes[208] = {
#include "b_chaos4/camera/dome_wall_meshes.inc"
};

static Sint16 dome_cap_meshes[34] = {
#include "b_chaos4/camera/dome_cap_meshes.inc"
};

static NJS_COLOR dome_wall_uvs[192] = {
#include "b_chaos4/camera/dome_wall_uvs.inc"
};

static NJS_COLOR dome_cap_uvs[28] = {
#include "b_chaos4/camera/dome_cap_uvs.inc"
};

static NJS_MESHSET dome_meshsets[2] = {
#include "b_chaos4/camera/dome_meshsets.inc"
};

static NJS_POINT3 dome_points[97] = {
#include "b_chaos4/camera/dome_points.inc"
};

static NJS_VECTOR dome_normals[97] = {
#include "b_chaos4/camera/dome_normals.inc"
};

static NJS_MODEL dome_model = {
#include "b_chaos4/camera/dome_model.inc"
};

static NJS_OBJECT dome_object = {
#include "b_chaos4/camera/dome_object.inc"
};

/* where the rising camera starts */
static const NJS_POINT3 rise_pos = {30.0F, 50.0F, 30.0F};

/* the fight camera's angle round the point it follows */
static Angle camera_ang;
static NJS_POINT3 player_last; /* player 0's pos the step before */
static NJS_VECTOR player_moved; /* his move since */
static NJS_POINT3 follow; /* the point the camera follows */
static Bool turn_slow;
/* TRUE for the fight camera's first step, which puts it at its place and
 * its aim at once */
static Bool camera_snap;
/* TRUE from a search's first step until player 0 has room again. the
 * camera doesn't turn to its aim meanwhile, and keeps to search_up */
static Bool searching;
/* chaos4_camera_look_at's pos, until take_aim uses it up */
static NJS_POINT3 *look_at;
static Float rise_height; /* how far the rising camera is above rise_pos */
static Angle orbit_ang;

void chaos4_camera_start(NJS_POINT3 *pos)
{
    camera_snap = TRUE;
    player_last = follow = player_work[0]->pos;
    camera_ang = 0;
    look_at = NULL;
    searching = FALSE;
    camera_override(fight_step, 0, CAMERA_LOOK_AT);
}

static void fight_step(struct camera_view *view)
{
    follow_player();
    if (!searching) {
        turn_to_aim();
    }
    /* the test of player 0's distance from Chaos 4 changes nothing: each of
     * its branches places the camera alike */
    if (njSqrt(SQUARE(chaos4_pos->x - player_work[0]->pos.x) +
               SQUARE(chaos4_pos->z - player_work[0]->pos.z)) > 10.0F) {
        place_camera();
    } else {
        place_camera();
    }
    keep_above_floor();
    camera_snap = FALSE;
}

static void follow_player(void)
{
    NJS_VECTOR gap;

    /* @hack spelling: player_moved and player_last through their addresses,
     * in statements outside a block, which loads player_moved's address
     * again for each member (0x0c901e08, 0x0c901e1c). as members, 176 of
     * 267 words differ, and as VECTOR_DIFF_EACH, in its block, 17 */
    (&player_moved)->x = player_work[0]->pos.x - (&player_last)->x;
    (&player_moved)->y = player_work[0]->pos.y - (&player_last)->y;
    (&player_moved)->z = player_work[0]->pos.z - (&player_last)->z;
    if (follow.y > SLOW_TURN_HEIGHT &&
        njSqrt(SQUARE_DIFF(&player_work[0]->pos, &player_last, x) +
               SQUARE_DIFF(&player_work[0]->pos, &player_last, y) +
               SQUARE_DIFF(&player_work[0]->pos, &player_last, z)) >=
            SLOW_TURN_SPEED) {
        turn_slow = TRUE;
    } else {
        turn_slow = FALSE;
    }
    player_last = player_work[0]->pos;
    if (njSqrt(SQUARE_DIFF(&player_work[0]->pos, &follow, x) +
               SQUARE_DIFF(&player_work[0]->pos, &follow, y) +
               SQUARE_DIFF(&player_work[0]->pos, &follow, z)) > FOLLOW_SLACK) {
        VECTOR_ADD_EACH(&follow, &player_moved);
    } else {
        VECTOR_DIFF_EACH(&gap, &player_work[0]->pos, &follow);
        VECTOR_SCALE_EACH(&gap, &gap, 0.5F);
        VECTOR_ADD_EACH(&follow, &gap);
    }
}

static void place_camera(void)
{
    NJS_POINT3 pos;
    NJS_POINT3 target;

    if (frame_step != 0) {
        return;
    }
    if (!player_has_room()) {
        search_for_room();
    } else {
        searching = FALSE;
    }
    pos = follow;
    pos.x += camera_distance * njSin(camera_ang);
    pos.z += camera_distance * njCos(camera_ang);
    pos.y += CAMERA_HEIGHT;
    target = follow;
    target.x -= LOOK_BEYOND * njSin(camera_ang);
    target.z -= LOOK_BEYOND * njCos(camera_ang);
    if (camera_snap) {
        camera_state->pos.x = pos.x;
        camera_state->pos.y = pos.y;
        camera_state->pos.z = pos.z;
    } else {
        camera_state->pos.x += (pos.x - camera_state->pos.x) * CAMERA_EASE;
        camera_state->pos.y += (pos.y - camera_state->pos.y) * CAMERA_EASE;
        camera_state->pos.z += (pos.z - camera_state->pos.z) * CAMERA_EASE;
    }
    camera_state->target.x = target.x;
    camera_state->target.y = target.y;
    camera_state->target.z = target.z;
}

static Bool player_has_room(void)
{
    /* @hack spelling: TRUE and FALSE returned from two branches, which SHC
     * folds to MOVT R0 (0x0c9021d6); as one return of the test's negation,
     * 3 of 12 words differ */
    if (!wall_near(&player_work[0]->pos, PLAYER_ROOM)) {
        return TRUE;
    }
    return FALSE;
}

/* turn the camera toward the point with room nearest outward, the way
 * from the swamp's middle out through player 0. on each side of outward,
 * the search tries points from SEARCH_STEPS steps of SEARCH_TURN off outward
 * in to outward itself. the side's aim is its last point with room before
 * the first without, or its first point when even that lacks room. a
 * search's first step takes the side whose aim lies nearer outward, and
 * keeps to it until player 0 has room */
static void search_for_room(void)
{
    Angle best_up = SEARCH_STEPS;
    Angle best_down = -SEARCH_STEPS;
    Bool up_walled = FALSE;
    Bool down_walled = FALSE;
    int step;
    int side;
    Angle turn;
    Angle outward;
    Angle aim;
    NJS_POINT3 pos;

    /* @hack redundant: a never-run test of up_walled, which keeps the
     * image's FR13 and FR14 (0x0c9022dc, 0x0c9022e6); without it, the two
     * trade and 2 of 183 words differ */
    if (0) {
        if (up_walled) {
        }
    }
    outward = THREE_QUARTER_TURN -
              (Angle)(atan2f(-player_work[0]->pos.z, -player_work[0]->pos.x) *
                      ANGLE_PER_RADIAN);
    for (step = SEARCH_STEPS; step >= 0; step--) {
        for (side = -1; side <= 1; side += 2) {
            if ((side > 0 && !up_walled) || (side < 0 && !down_walled)) {
                turn = step * side;
                pos = player_work[0]->pos;
                pos.x += SEARCH_REACH * njSin((turn * SEARCH_TURN) + outward);
                pos.z += SEARCH_REACH * njCos((turn * SEARCH_TURN) + outward);
                pos.y += CAMERA_HEIGHT;
                if (wall_near(&pos, SEARCH_ROOM)) {
                    if (side > 0) {
                        up_walled = TRUE;
                    } else {
                        down_walled = TRUE;
                    }
                } else {
                    if (side > 0) {
                        best_up = turn;
                    } else {
                        best_down = turn;
                    }
                }
            }
        }
        /* @hack redundant: a never-run test of down_walled, which keeps the
         * image's registers, with SEARCH_TURN's 0x0aaa in R8 (0x0c902244,
         * 0x0c902266); without it, R8 and R9 trade and 7 of 183 words
         * differ */
        if (0) {
            if (down_walled) {
            }
        }
    }
    if (!searching) {
        searching = TRUE;
        if (best_up < -best_down) {
            search_up = TRUE;
        } else {
            search_up = FALSE;
        }
    }
    aim = search_up ? best_up : best_down;
    aim *= SEARCH_TURN;
    /* @hack redundant: a never-run test of step * SEARCH_TURN, with an empty
     * else, which keeps the image's 32 bytes of stack, with SEARCH_STEPS's
     * 10 at R15 + 16 (0x0c902236); without it, the stack takes
     * 28 bytes, the function runs 38 bytes long, the functions after it
     * move, and 151 of 183 words differ */
    if (0) {
        if (step * SEARCH_TURN) {
        } else {
        }
    }
    aim += outward;
    /* @hack redundant: an empty never-run block, which keeps 0xffff0000 in R3
     * and its load where the image has it (0x0c90239a); without it, it takes R2
     * and 9 of 183 words differ */
    if (0) {
    }
    {
        /* @hack spelling: camera_ang turned through a pointer to it, which
         * keeps its address in R4 for both reads and the store (0x0c902382
         * to 0x0c9023b4). as camera_ang, 139 of 183 words differ */
        Angle *ang = &camera_ang;
        *ang += (Float)ANGLE_TURN(aim, *ang) * TURN_FAST;
    }
}

static void keep_above_floor(void)
{
    if (camera_state->pos.y < CAMERA_FLOOR) {
        camera_state->pos.y = CAMERA_FLOOR;
    }
}

/* the camera's aim: the angle from look_at, or else from Chaos 4, out
 * through player 0. it uses up look_at */
static Angle take_aim(void)
{
    NJS_VECTOR to;
    Angle ang;

    if (look_at != NULL) {
        VECTOR_DIFF_EACH(&to, look_at, &player_work[0]->pos);
        look_at = NULL;
    } else {
        VECTOR_DIFF_EACH(&to, chaos4_pos, &player_work[0]->pos);
    }
    ang = THREE_QUARTER_TURN - (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
    return ang;
}

void chaos4_camera_look_at(NJS_POINT3 *pos)
{
    look_at = pos;
}

static void turn_to_aim(void)
{
    Angle aim = take_aim();

    if (camera_snap) {
        camera_ang = aim;
    } else {
        camera_ang += (Float)ANGLE_TURN(aim, camera_ang) *
                      (turn_slow ? TURN_SLOW : TURN_FAST);
    }
}

void chaos4_dome_draw(void)
{
    if (frame_step == 0) {
        njSetTexture(&chaos4_object_texlist);
        chaos4_material_reset();
        njPushMatrix(NULL);
        draw_tree(&dome_object);
        njPopMatrix(1);
    }
}

/* TRUE when a side of a world object that doesn't move stands within room
 * of pos, in x or in z, else FALSE */
static Bool wall_near(NJS_POINT3 *pos, Float room)
{
    struct sides_query query;
    struct world_object *objects;
    Uint16 i;
    Float distance;

    query.pos = *pos;
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, 1.0F);
    for (i = 0; i < world_count; i++) {
        if (objects[i].flags & WORLD_MOVES) {
            continue;
        }
        if (polygon_find_sides(&query, objects[i].object, objects[i].flags)) {
            if (query.sides[SIDE_MINUS_X].found) {
                distance = njAbs(query.sides[SIDE_MINUS_X].level - pos->x);
                if (distance < room) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_PLUS_X].found) {
                distance = njAbs(query.sides[SIDE_PLUS_X].level - pos->x);
                if (distance < room) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_MINUS_Z].found) {
                distance = njAbs(query.sides[SIDE_MINUS_Z].level - pos->z);
                if (distance < room) {
                    return TRUE;
                }
            }
            if (query.sides[SIDE_PLUS_Z].found) {
                distance = njAbs(query.sides[SIDE_PLUS_Z].level - pos->z);
                if (distance < room) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

void chaos4_camera_rise_start(void)
{
    rise_height = 0.0F;
    camera_override(rise_step, 0, CAMERA_LOOK_AT);
}

static void rise_step(struct camera_view *view)
{
    if (chaos4_pos->y < SWAMP_Y) {
        /* @hack spelling: the sum tested in the statement that stores it,
         * which stores it in the slot of the test's branch (BF/S at
         * 0x0c902770). stored and then tested, it is stored before the
         * test, and 15 of 55 words differ */
        if ((rise_height += RISE_SPEED) > RISE_TOP) {
            rise_height = RISE_TOP;
        }
    }
    camera_state->pos.x = rise_pos.x;
    camera_state->pos.y = rise_pos.y + rise_height;
    camera_state->pos.z = rise_pos.z;
    camera_state->target.x = chaos4_pos->x;
    camera_state->target.y = chaos4_pos->y;
    camera_state->target.z = chaos4_pos->z;
}

void chaos4_camera_orbit_start(void)
{
    orbit_ang = ORBIT_START;
    camera_override(orbit_step, 0, CAMERA_LOOK_AT);
}

static void orbit_step(struct camera_view *view)
{
    orbit_ang += ORBIT_TURN;
    camera_state->pos.x = chaos4_pos->x + (ORBIT_DISTANCE * njSin(orbit_ang));
    camera_state->pos.y = chaos4_pos->y;
    camera_state->pos.z = chaos4_pos->z + (ORBIT_DISTANCE * njCos(orbit_ang));
    camera_state->target.x = chaos4_pos->x;
    camera_state->target.y = chaos4_pos->y;
    camera_state->target.z = chaos4_pos->z;
}
