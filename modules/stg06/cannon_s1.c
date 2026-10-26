/* CANNON_S1, a sweeping turret. it opens once the nearest player is near
 * and in front of it. it turns to the end of its sweep on that player's
 * side. it then sweeps a quarter turn to the other end, firing pairs of
 * shots. it then turns back to the middle, closes and rests. a hit breaks
 * it, and so does the land's collapse */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/explosion.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg06/cannon_s1.h"
#include "stg06/collapse.h"
#include "stg06/debris.h"
#include "stg06/models.h"
#include "stg06/shots.h"
#include "stg06/sky_deck.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cannon_s1_object;
extern NJS_MOTION cannon_s1_motion;

/* it opens once the nearest player is within TURRET_NEAR, and within
 * TURRET_ARC either side of its front */
#define TURRET_NEAR 180.0F
#define TURRET_ARC NJM_DEG_ANG(45)
/* how far the barrel turns each step */
#define TURN_SPEED 218
/* each step of the sweep, it holds fire with the chance HOLD_FIRE. its
 * shots land SHOT_REACH ahead of the pivot and up to SHOT_REACH_RANGE more.
 * they land SHOT_SIDE to either side. sky_deck_shot_fire never reads
 * SHOT_SCALE */
#define HOLD_FIRE 0.5F
#define SHOT_REACH 180.0F
#define SHOT_REACH_RANGE 10.0F
#define SHOT_SIDE 3.43F
#define SHOT_SCALE 3.0F
/* the model's pivot node sits PIVOT_UP above the body, turned by
 * PIVOT_TURN. its barrel node sits BARREL_AHEAD toward the muzzles. the
 * muzzles and the draw take these in place of the nodes' own */
#define PIVOT_UP 8.0F
#define PIVOT_TURN NJM_DEG_ANG(180)
#define BARREL_AHEAD 4.0F
#define CANNON_VIEW 280.0F
/* it rests closed until its timer passes REST_STEPS */
#define REST_STEPS 40
/* the barrel's spring takes RECOIL_PULL of its offset from its speed each
 * step. the speed keeps RECOIL_DAMP of itself. a barrel that would pass
 * rest stops there, and its speed takes RECOIL_BOUNCE of itself. nothing
 * moves the barrel off rest, so it never recoils */
#define RECOIL_PULL 0.3F
#define RECOIL_DAMP 0.95F
#define RECOIL_BOUNCE (-0.65F)
/* the explosion's scale as it breaks, and its height above the turret */
#define BREAK_BLAST 5.0F
#define BREAK_UP 4.0F

/* the turret's own layout of task_work */
struct cannon_s1_work {
    Sint8 mode;
    Sint8 start_side; /* 1 or -1: the end of the sweep it turns to first */
    Uint8 unk_02;
    Uint8 unk_03;
    Sint16 flags;
    Uint16 timer;
    Uint8 unk_08[4];
    Float recoil_speed;
    Float recoil; /* the barrel's offset from rest, back along its length */
    struct angles ang; /* z is the barrel's turn */
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

enum cannon_s1_mode {
    CANNON_WAIT, /* closed, until the nearest player is near and in front */
    CANNON_OPEN, /* its motion plays */
    CANNON_TURN, /* to the end of the sweep it starts from */
    CANNON_SWEEP, /* to the other end, firing */
    CANNON_CENTER, /* the barrel turns back to the middle */
    CANNON_CLOSE, /* its motion plays backward */
    CANNON_REST, /* closed, before it waits again */
    CANNON_BROKEN
};

static struct debris_spec cannon_s1_debris = {&cannon_s1_object,
                                              {0.0F, 0.0F, 0.0F},
                                              {0, 0, 0},
                                              {0.0F, 1.5F, 0.0F},
                                              {0.0F, 0.0F, 0.0F},
                                              200.0F,
                                              {1.0F, 1.0F, 1.0F},
                                              1.5F,
                                              20.0F,
                                              1.0F,
                                              0.98F,
                                              0,
                                              &obj_skydeck_texlist,
                                              0.0F};
static struct collision_shape cannon_s1_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_UNTURNED,
    SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    16.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};
/* the muzzles, from the barrel. the pivot turns half round, so each fires
 * across to the mark on the other side */
static NJS_POINT3 cannon_s1_left_muzzle = {-SHOT_SIDE, 2.0F, -7.5F};
static NJS_POINT3 cannon_s1_right_muzzle = {SHOT_SIDE, 2.0F, -7.5F};

static void cannon_s1_init(struct task *task);
static void cannon_s1_destroy(struct task *task);
static void cannon_s1_run(struct task *task);
static void cannon_s1_draw(struct task *task);

/* set left and right to the muzzles, with the barrel turned by turn */
static void cannon_s1_muzzles(NJS_POINT3 *left, NJS_POINT3 *right, Angle turn,
                              struct cannon_s1_work *work)
{
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, PIVOT_UP, 0.0F);
    TURN_VIA(njRotateY, turn + PIVOT_TURN, ang);
    njTranslate(NULL, 0.0F, 0.0F, -BARREL_AHEAD);
    njCalcPoint(NULL, &cannon_s1_left_muzzle, left);
    njCalcPoint(NULL, &cannon_s1_right_muzzle, right);
    njPopMatrix(1);
}

/* set target to the nearest player's x and z, at the height of the floor
 * under that player */
static void cannon_s1_find_target(NJS_POINT3 *target,
                                  struct cannon_s1_work *work)
{
    int nearest;
    struct player *player = player_get(nearest = player_nearest(&work->pos));
    struct task_work *player_task = player_work[nearest];

    target->x = player_task->pos.x;
    target->y = player->shadow.below;
    target->z = player_task->pos.z;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c90e148); without it, task is kept at sp+0, 10 of 14 words
 * differ and cannon_s1_init starts 2 bytes late */
void cannon_s1_step(register struct task *task)
{
    if (!placement_try_leave(task)) {
        cannon_s1_init(task);
    }
}

static void cannon_s1_init(struct task *task)
{
    struct cannon_s1_work *work = (struct cannon_s1_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, &cannon_s1_shape, COLLISION_ITEM, 1, 1);
    }
    task->draw = cannon_s1_draw;
    task->step = cannon_s1_run;
    task->destructor = cannon_s1_destroy;
    work->timer = 0;
    work->mode = CANNON_WAIT;
}

static void cannon_s1_destroy(struct task *task)
{
}

static void cannon_s1_run(struct task *task)
{
    struct cannon_s1_work *work = (struct cannon_s1_work *)task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (work->mode == CANNON_BROKEN) {
        return;
    }
    cannon_s1_draw(task);
    if (placement_editing(task)) {
        return;
    }
    work->timer++;
    switch (work->mode) {
    case CANNON_WAIT: {
        /* @hack type: target and way as union point_floats, which keeps the
         * look's 3 in R6 for its test (0x0c90e206, 0x0c90e284) and puts the
         * walk's from in R6 and to in R5 (0x0c90e2b0 to 0x0c90e2bc); as an
         * NJS_POINT3 and an NJS_VECTOR, 70 of 696 words differ */
        union point_floats target;
        union point_floats way;
        Angle bearing;
        Sint16 *bearing_low;
        Float *at;
        Float *from;
        Float *to;

        /* a look every fourth step */
        if (!(work->timer & 3)) {
            /* @hack type: the bearing stored whole and its low half read back
             * from memory through a Sint16 pointer (0x0c90e2f0, 0x0c90e2f2);
             * as a Sint16 local set to (Sint16)bearing, 94 of 696 words
             * differ */
            bearing_low = (Sint16 *)&bearing;
            cannon_s1_find_target(&target.point, work);
            if (njDistanceP2P(&target.point, &work->pos) > TURRET_NEAR) {
                work->timer = 0;
                break;
            }
            /* @hack spelling: way taken through cursors, which load by
             * post-increment and store as ADD #4 steps R5 (0x0c90e2b8 to
             * 0x0c90e2d8); by member, 90 of 696 words differ */
            to = way.elements;
            from = VECTOR_FLOATS(&work->pos);
            /* @hack spelling: at set inside the first load, which puts at in R4
             * and from in R6; set ahead of it, the two trade and 10 of 696
             * words differ */
            *to++ = *(at = target.elements) - *from++;
            at++;
            *to++ = *at++ - *from++;
            *to = *at - *from;
            bearing =
                (Angle)(atan2f(way.vector.x, way.vector.z) * ANGLE_PER_RADIAN) -
                work->ang.y;
            if (*bearing_low >= TURRET_ARC || *bearing_low <= -TURRET_ARC) {
                work->timer = 0;
            }
            if (work->timer > 1) {
                if (*bearing_low > 0) {
                    work->start_side = 1;
                } else {
                    work->start_side = -1;
                }
                sound_play_at(SE_TURRET, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
                work->mode = CANNON_OPEN;
                work->timer = 0;
            }
        }
    } break;
    case CANNON_OPEN:
        if (work->timer >= cannon_s1_motion.nbFrame) {
            work->ang.z = 0;
            work->mode = CANNON_TURN;
            work->timer = 0;
        }
        break;
    case CANNON_TURN: {
        Sint32 aim;
        Sint32 gap;
        /* @hack type: ang.z's low half read and stepped through a Sint16
         * pointer, by MOV.W (0x0c90e388 to 0x0c90e3a0); read as
         * (Sint16)work->ang.z and stepped whole, 527 of 696 words differ and
         * the object ends 64 bytes early */
        Sint16 *turn = (Sint16 *)&work->ang.z;

        if (work->start_side > 0) {
            aim = TURRET_ARC;
        } else {
            aim = -TURRET_ARC;
        }
        aim = aim - *turn;
        if ((gap = (Sint16)aim) > TURN_SPEED) {
            *turn += TURN_SPEED;
        }
        if (gap < -TURN_SPEED) {
            *turn -= TURN_SPEED;
        }
        if (work->timer > 1 && gap < TURN_SPEED && gap > -TURN_SPEED) {
            work->mode = CANNON_SWEEP;
            work->timer = 0;
        }
    } break;
    case CANNON_SWEEP: {
        NJS_POINT3 left_muzzle;
        NJS_POINT3 right_muzzle;
        NJS_POINT3 center;
        NJS_POINT3 left_at;
        NJS_POINT3 right_at;
        struct sides_query query;
        NJS_OBJECT *pivot = cannon_s1_object.child;
        /* @hack type: the same for the sweep (0x0c90e442 to 0x0c90e45a); read
         * as (Sint16)work->ang.z and stepped whole, 493 of 696 words differ
         * and the object ends 64 bytes early */
        Sint16 *turn = (Sint16 *)&work->ang.z;
        Sint32 aim;
        Sint32 gap;
        Angle ang;
        Float reach;
        Float height;
        int found;
        struct surface *side;

        if (work->start_side > 0) {
            aim = -TURRET_ARC;
        } else {
            aim = TURRET_ARC;
        }
        aim = aim - *turn;
        if ((gap = (Sint16)aim) > TURN_SPEED) {
            *turn += TURN_SPEED;
        }
        if (gap < -TURN_SPEED) {
            *turn -= TURN_SPEED;
        }
        if (gap < TURN_SPEED && gap > -TURN_SPEED) {
            work->mode = CANNON_CENTER;
            work->timer = 0;
        }
        if (rand() * RAND_SCALE < HOLD_FIRE) {
            break;
        }
        reach = SHOT_REACH + (rand() * (SHOT_REACH_RANGE * RAND_SCALE));
        center.x = 0.0F;
        center.y = 0.0F;
        center.z = reach;
        left_at.x = -SHOT_SIDE;
        left_at.y = 0.0F;
        left_at.z = reach;
        right_at.x = SHOT_SIDE;
        right_at.y = 0.0F;
        right_at.z = reach;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njTranslateV(NULL, (NJS_VECTOR *)pivot->pos);
        TURN_VIA(njRotateY, work->ang.z, ang);
        njCalcPoint(NULL, &center, &center);
        njCalcPoint(NULL, &left_at, &left_at);
        njCalcPoint(NULL, &right_at, &right_at);
        njPopMatrix(1);
        /* ground_find_sides looks below and above center, the point between the
         * marks. the marks take the height of the surface it finds there, the
         * nearer to center if both. with neither found, the turret holds
         * fire */
        /* @hack redundant: height, set 10000 below the turret, which every
         * path that reads height sets again first; without it, 355 of 696
         * words differ and the object ends 32 bytes early */
        height = work->pos.y - 10000.0F;
        query.pos = center;
        if (ground_find_sides(&query)) {
            found = 0;
            side = &query.sides[SIDE_BELOW];
            /* @hack redundant: an if (0.0F) block that never runs, which SHC
             * writes none of but counts in choosing registers. its read of
             * side keeps side in R9 (0x0c90e55e); without it, side takes R5
             * and 4 of 696 words differ. tested as 0, 89 differ */
            if (0.0F) {
                njSin(0);
                found = side->found;
            }
            if (side->found) {
                found = 1;
                height = side->level;
            }
            side = &query.sides[SIDE_ABOVE];
            /* @hack redundant: the same for the side above, which keeps side
             * in R9 (0x0c90e5ea); without it, 5 of 696 words differ */
            if (0.0F) {
                njCos(0);
                found = side->found;
            }
            if (side->found) {
                if (!found || _builtin_fabsf(center.y - height) >
                                  _builtin_fabsf(center.y - side->level)) {
                    height = side->level;
                }
                found = 1;
            }
            if (found) {
                left_at.y = height;
                right_at.y = height;
                cannon_s1_muzzles(&left_muzzle, &right_muzzle, work->ang.z,
                                  work);
                sky_deck_shot_fire(&left_muzzle, &left_at, SHOT_SCALE);
                sky_deck_shot_fire(&right_muzzle, &right_at, SHOT_SCALE);
                sound_play_at(SE_TURRET, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
            }
        }
    } break;
    case CANNON_CENTER: {
        /* @hack type: ang.z's low half read through a Sint16 pointer, by
         * MOV.W (0x0c90e670, 0x0c90e6c0), though the center steps it whole;
         * read as (Sint16)work->ang.z, 281 of 696 words differ and the
         * object ends 32 bytes early */
        Sint16 *turn = (Sint16 *)&work->ang.z;

        if ((*turn & 0x7fff) < TURN_SPEED) {
            work->mode = CANNON_CLOSE;
            work->timer = 0;
            break;
        }
        if (*turn > 0) {
            work->ang.z -= TURN_SPEED;
            break;
        }
        work->ang.z += TURN_SPEED;
    } break;
    case CANNON_CLOSE:
        if (work->timer >= cannon_s1_motion.nbFrame) {
            work->mode = CANNON_REST;
            work->timer = 0;
        }
        break;
    case CANNON_REST:
        if (work->timer > REST_STEPS) {
            work->mode = CANNON_WAIT;
            work->timer = 0;
        }
        break;
    }
    work->recoil_speed += -work->recoil * RECOIL_PULL;
    work->recoil_speed *= RECOIL_DAMP;
    if (work->recoil + work->recoil_speed <= 0.0F) {
        work->recoil = 0.0F;
        work->recoil_speed *= RECOIL_BOUNCE;
    } else {
        work->recoil += work->recoil_speed;
    }
    if ((work->flags & COLLISION_DAMAGED) && work->mode != CANNON_BROKEN) {
        NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};
        NJS_VECTOR accel = {0.0F, -0.1F, 0.0F};

        sky_deck_debris_create(&work->pos, &work->ang, &speed, &accel,
                               work->pos.y, &cannon_s1_debris);
        explosion_create_at(work->pos.x, work->pos.y + BREAK_UP, work->pos.z,
                            BREAK_BLAST);
        work->mode = CANNON_BROKEN;
        work->timer = 0;
        sky_deck_hop((struct task_work *)work);
        sound_play_at(SE_TURRET_BREAK, 0, 0, 64, work->pos.x, work->pos.y,
                      work->pos.z);
        placement_kill(task);
    } else if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
        NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};
        NJS_VECTOR accel = {0.0F, -0.1F, 0.0F};

        sky_deck_debris_create(&work->pos, &work->ang, &speed, &accel,
                               work->pos.y, &cannon_s1_debris);
        explosion_create_at(work->pos.x, work->pos.y + BREAK_UP, work->pos.z,
                            BREAK_BLAST);
        sound_play_at(SE_CANNON_BREAK, 0, 0, 32, work->pos.x, work->pos.y,
                      work->pos.z);
        work->mode = CANNON_BROKEN;
        work->timer = 0;
        placement_kill(task);
    } else if (work->mode != CANNON_BROKEN) {
        collision_register((struct task_work *)work);
    }
}

/* while it waits, opens, closes or rests, it draws its motion. otherwise
 * it draws its parts: the body, the pivot, and the barrel at its turn and
 * recoil */
static void cannon_s1_draw(struct task *task)
{
    struct cannon_s1_work *work = (struct cannon_s1_work *)task->work;
    NJS_OBJECT *object = &cannon_s1_object;
    NJS_MOTION *motion = &cannon_s1_motion;
    Angle ang;
    int frame;

    if (placement_try_leave(task)) {
        return;
    }
    if (work->mode == CANNON_BROKEN) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, CANNON_VIEW)) {
        return;
    }
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch (work->mode) {
    case CANNON_REST:
    case CANNON_WAIT:
        draw_motion(object, motion, 0.0F);
        break;
    case CANNON_OPEN:
        frame =
            work->timer >= motion->nbFrame ? motion->nbFrame - 1 : work->timer;
        draw_motion(object, motion, (Float)frame);
        break;
    case CANNON_CLOSE:
        frame = work->timer >= motion->nbFrame
                    ? 0
                    : motion->nbFrame - work->timer - 1;
        draw_motion(object, motion, (Float)frame);
        break;
    default:
        draw_model(object->model);
        object = object->child;
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, PIVOT_UP, 0.0F);
        njRotateY(NULL, PIVOT_TURN);
        draw_model(object->model);
        object = object->child;
        TURN_VIA(njRotateY, work->ang.z, ang);
        njTranslate(NULL, 0.0F, 0.0F, work->recoil - BARREL_AHEAD);
        draw_model(object->model);
        njPopMatrix(1);
        break;
    }
    njPopMatrix(1);
}
