/* FlipperL and FlipperR: the left and right flippers of the pinball tables.
 * FlipperLx and FlipperRx: the same, with one more flag, which no code of
 * theirs reads */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/flipper.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "system/vector.h"

extern NJS_OBJECT flipper_left_object;
extern NJS_OBJECT flipper_right_object;

struct flipper_vars {
    NJS_MODEL *model;
    Angle swing; /* the blade's turn about y away from its rest */
    /* the blade's y angle from x in the model. the blade's collision box and
     * the bat's tests lie along x, so each turns by swing plus rest */
    Angle rest;
};

#define VARS(work) ((struct flipper_vars *)&(work)->vars)

/* in work->flags */
#define FLIPPER_UP 0x2000 /* swung all the way up */
#define FLIPPER_LEFT 0x4000
#define FLIPPER_RIGHT 0x8000
/* FlipperLx and FlipperRx set it, and no code in this file reads it */
#define FLIPPER_UNK_200 0x200

#define FLIPPER_SWING_MAX 0x3000
#define FLIPPER_LEFT_REST (-0x1b60)
#define FLIPPER_RIGHT_REST (NJM_DEG_ANG(180) - FLIPPER_LEFT_REST)

#define FLIPPER_LEFT_BUTTON PDD_DGT_TL
#define FLIPPER_RIGHT_BUTTON PDD_DGT_TR
#define FLIPPER_BUTTON PDD_DGT_TA /* swings both flippers */

#define SE_FLIPPER 246

/* player 0 touching a flipper that is all the way up slows while faster than
 * FLIPPER_SLOW_SPEED. it keeps FLIPPER_SLOW_KEEP of its speed each step */
#define FLIPPER_SLOW_SPEED 0.6F
#define FLIPPER_SLOW_KEEP 0.9F

/* the blade and its pivot */
static struct collision_shape flipper_shapes[2] = {
    {SHAPE_RECTANGLE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     18.0F,
     15.0F,
     3.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     3.5F,
     15.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static Float flipper_impulse = 8.3F;

/* the push as a line from pos, length along dir, in color. it never
 * draws */
static void flipper_draw_push(NJS_POINT3 *pos, NJS_VECTOR *dir, Float length,
                              Uint32 color)
{
    NJS_POINT3 points[2];
    NJS_COLOR colors[1];
    NJS_POINT3COL line;

    /* @hack redundant: the line's draw, which never runs. it gives the
     * function the image's 44-byte frame at 0x0c901e40, and keeps SHC from
     * expanding the function into flipper_bat. without it, SHC writes no
     * flipper_draw_push, so its 3 of 3 words differ, and without the call,
     * 310 of flipper_bat's 411 words differ */
    if (0) {
        points[0] = *pos;
        points[1].x = pos->x + (dir->x * length);
        points[1].y = pos->y + (dir->y * length);
        points[1].z = pos->z + (dir->z * length);
        colors[0].color = color;
        line.p = points;
        line.col = colors;
        line.tex = NULL;
        line.num = 2;
        njDrawLine3D(&line, 1, 0);
    }
}

static void flipper_destroy(struct task *task)
{
}

static void flipper_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y + VARS(work)->swing, ang);
    draw_model(VARS(work)->model);
    njPopMatrix(1);
}

static void flipper_move_shape(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Angle turn;
    NJS_POINT3 center;

    center.x = 14.0F;
    center.y = 0.0F;
    center.z = 0.0F;
    turn = VARS(work)->swing + VARS(work)->rest;
    work->collision->shapes->ang.y = turn;
    matrix_push_unit();
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN(njRotateY, turn);
    njCalcPoint(NULL, &center, &center);
    work->collision->shapes->center = center;
    njPopMatrix(1);
}

static void flipper_slow_player(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_VECTOR away;
    NJS_VECTOR speed;
    Float length;

    if ((work->collision->flags & COLLISION_HIT) &&
        work->collision->hit->kind == COLLISION_PLAYER) {
        away.x = work->pos.x - player->pos.x;
        away.y = work->pos.y - player->pos.y;
        away.z = work->pos.z - player->pos.z;
        if (njScalor(&away) >= 1.0F) {
            speed = player_motion[0]->speed;
            if ((length = njScalor(&speed)) >= 0.05F) {
                njUnitVector(&away);
                njUnitVector(&speed);
                if (work->flags & FLIPPER_UP) {
                    if (length > FLIPPER_SLOW_SPEED) {
                        length *= FLIPPER_SLOW_KEEP;
                        player_set_speed(0, speed.x * length, speed.y * length,
                                         speed.z * length);
                    }
                }
            }
        }
    }
}

/* the result's sign gives which side of a line player 0 was on a step
 * before. the line runs through the pivot, 15 degrees off the blade. turn is
 * the blade's y angle */
static Float flipper_player_side(struct task_work *work, Angle turn)
{
    Angle ang;
    NJS_VECTOR to;
    NJS_VECTOR side;
    /* @hack spelling: side's y and z in locals, set ahead of the FLIPPER_RIGHT
     * test, so that SHC loads z into FR5 and y into FR4 before it (0x0c9020ce,
     * 0x0c9020d6). stored to side's fields after the test, 16 of 104 words
     * differ */
    Float y;
    Float z;

    player_get_history(0, 1, &to, NULL);
    to.x -= work->pos.x;
    to.y -= work->pos.y;
    to.z -= work->pos.z;
    njUnitVector(&to);
    z = 0.259F;
    y = 0.0F;
    side.x = (work->flags & FLIPPER_RIGHT) ? -0.966F : 0.966F;
    side.y = y;
    side.z = z;
    matrix_push_unit();
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN(njRotateY, turn);
    njCalcPoint(NULL, &side, &side);
    njPopMatrix(1);
    njOuterProduct(&side, &to, &side);
    return side.y;
}

/* the angle at the pivot between the blade and where player 0 was a step
 * before */
static Angle flipper_player_angle(struct task_work *work)
{
    Angle ang;
    NJS_VECTOR to;
    NJS_VECTOR blade;

    player_get_history(0, 1, &to, NULL);
    to.x -= work->pos.x;
    to.y -= work->pos.y;
    to.z -= work->pos.z;
    njUnitVector(&to);
    blade.x = (work->flags & FLIPPER_RIGHT) ? -1.0F : 1.0F;
    blade.y = blade.z = 0.0F;
    matrix_push_unit();
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y + VARS(work)->swing + VARS(work)->rest,
             ang);
    njCalcPoint(NULL, &blade, &blade);
    njPopMatrix(1);
    return acosf(njInnerProduct(&blade, &to)) * ANGLE_PER_RADIAN;
}

/* send player 0 off the blade's face. turn is the swing this step */
static void flipper_bat(struct task_work *work, Angle turn)
{
    struct task_work *player = player_work[0];
    Angle ang;
    Angle most;
    Angle lead;
    NJS_PLANE plane;
    NJS_VECTOR away;
    NJS_VECTOR speed;
    NJS_VECTOR push;
    NJS_VECTOR bounce;
    NJS_VECTOR along;
    NJS_VECTOR normal;
    Float distance;
    Float sine;
    Float impulse;
    Float scale;
    /* @hack spelling: a local zero for the stores below, which SHC keeps in
     * FR14 (FLDI0 FR14 at 0x0c9022b4). with 0.0F in each store, SHC keeps it
     * in FR15, saves no FR13, and 58 of 411 words differ */
    Float zero = 0.0F;

    /* speed holds the normal of the blade's face, before the blade's turn */
    speed.x = zero;
    speed.y = zero;
    if (work->flags & FLIPPER_LEFT) {
        speed.z = -1.0F;
    } else {
        speed.z = 1.0F;
    }
    matrix_push_unit();
    most = flipper_player_angle(work);
    lead = angle_difference(0, turn);
    lead /= 2;
    if (lead > most) {
        lead = most;
    } else if (lead < -most) {
        lead = -most;
    }
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY,
             work->ang.y + VARS(work)->swing + VARS(work)->rest + lead, ang);
    njCalcPoint(NULL, &speed, &normal);
    njPopMatrix(1);
    if (flipper_player_side(work, work->ang.y + VARS(work)->swing +
                                      VARS(work)->rest) < 0.0F) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }
    flipper_draw_push(&player->collision->shapes->center, &normal, 15.0F,
                      0xffffffff);
    bounce = normal;
    plane.vx = bounce.x;
    plane.vy = bounce.y;
    plane.vz = bounce.z;
    plane.pz = plane.py = plane.px = zero;
    away.x = player->pos.x - work->pos.x;
    away.y = player->pos.y - work->pos.y;
    away.z = player->pos.z - work->pos.z;
    distance = njScalor(&away);
    sine = njSin(turn);
    if (work->flags & FLIPPER_RIGHT) {
        sine = -sine;
    }
    impulse = distance * sine * _builtin_fabsf(sine) * flipper_impulse *
              (work->scl.x + 1.0F);
    speed = player_motion[0]->speed;
    njDistanceP2PL(&speed, &plane, &along);
    bounce.x = speed.x - along.x;
    bounce.y = speed.y - along.y;
    bounce.z = speed.z - along.z;
    push.x = normal.x * impulse;
    push.y = normal.y * impulse;
    push.z = normal.z * impulse;
    if (njInnerProduct(&speed, &push) >= 0.0F) {
        if (njScalor(&bounce) < njScalor(&push)) {
            bounce = push;
        }
    } else if (njInnerProduct(&normal, &push) >= 0.0F) {
        scale = impulse == 0.0F ? zero : -0.5F;
        bounce.x *= scale;
        bounce.y *= scale;
        bounce.z *= scale;
        bounce.x += push.x;
        bounce.y += push.y;
        bounce.z += push.z;
    } else {
        bounce = push;
    }
    if (njScalor(&along) > 1.0F) {
        along.x *= 0.81F;
        along.y *= 0.81F;
        along.z *= 0.81F;
    }
    speed.x = bounce.x + along.x;
    speed.y = bounce.y + along.y;
    speed.z = bounce.z + along.z;
    player_set_speed(0, speed.x, speed.y, speed.z);
}

static void flipper_left_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle old;
    Angle swing;
    Angle step;
    Float speed;

    swing = old = VARS(work)->swing;
    speed = work->scl.x + 1.0F;
    flipper_draw(task);
    if (old > 0x2c00) {
        step = 0x100;
    } else if (old > 0x2000) {
        step = 0x400;
    } else if (old > 0x1000) {
        step = 0x800;
    } else {
        step = 0xc00;
    }
    step = step * speed;
    if (get_character() == CHARACTER_SONIC && player_work[1] != NULL) {
        if ((per_info[0]->on & FLIPPER_LEFT_BUTTON) ||
            (per_info[0]->on & FLIPPER_BUTTON) ||
            (per_info[1]->on & FLIPPER_LEFT_BUTTON) ||
            (per_info[1]->on & FLIPPER_BUTTON)) {
            swing += step;
        } else {
            swing -= step;
        }
    } else if ((per_info[0]->on & FLIPPER_LEFT_BUTTON) ||
               (per_info[0]->on & FLIPPER_BUTTON)) {
        swing += step;
    } else {
        swing -= step;
    }
    if (swing > FLIPPER_SWING_MAX) {
        work->flags |= FLIPPER_UP;
        swing = FLIPPER_SWING_MAX;
    } else {
        work->flags &= ~FLIPPER_UP;
    }
    if (swing < 0) {
        swing = 0;
    }
    if (swing == step && old == 0) {
        sound_play(SE_FLIPPER, 0, 0, 0);
    }
    if (work->collision->flags & COLLISION_HIT) {
        flipper_bat(work, swing - old);
    }
    VARS(work)->swing = swing;
    flipper_slow_player(task);
    flipper_move_shape(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void flipper_right_run(struct task *task)
{
    struct task_work *work = task->work;
    Angle old;
    Angle swing;
    Angle step;
    Float speed;

    swing = old = VARS(work)->swing;
    speed = work->scl.x + 1.0F;
    flipper_draw(task);
    if (old < -0x2c00) {
        step = 0x100;
    } else if (old < -0x2000) {
        step = 0x400;
    } else if (old < -0x1000) {
        step = 0x800;
    } else {
        step = 0xc00;
    }
    step = step * speed;
    if (get_character() == CHARACTER_SONIC && player_work[1] != NULL) {
        if ((per_info[0]->on & FLIPPER_RIGHT_BUTTON) ||
            (per_info[0]->on & FLIPPER_BUTTON) ||
            (per_info[1]->on & FLIPPER_RIGHT_BUTTON) ||
            (per_info[1]->on & FLIPPER_BUTTON)) {
            swing -= step;
        } else {
            swing += step;
        }
    } else if ((per_info[0]->on & FLIPPER_RIGHT_BUTTON) ||
               (per_info[0]->on & FLIPPER_BUTTON)) {
        swing -= step;
    } else {
        swing += step;
    }
    if (swing < -FLIPPER_SWING_MAX) {
        work->flags |= FLIPPER_UP;
        swing = -FLIPPER_SWING_MAX;
    } else {
        work->flags &= ~FLIPPER_UP;
    }
    if (swing > 0) {
        swing = 0;
    }
    if (swing == -step && old == 0) {
        sound_play(SE_FLIPPER, 0, 0, 0);
    }
    if (work->collision->flags & COLLISION_HIT) {
        flipper_bat(work, swing - old);
    }
    VARS(work)->swing = swing;
    flipper_slow_player(task);
    flipper_move_shape(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

void flipper_left_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, flipper_shapes, COLLISION_OBJECT, 1, 2);
    VARS(work)->swing = 0;
    VARS(work)->rest = FLIPPER_LEFT_REST;
    work->flags = FLIPPER_LEFT;
    VARS(work)->model = flipper_left_object.model;
    task->step = flipper_left_run;
    task->draw = flipper_draw;
    task->destructor = flipper_destroy;
}

void flipper_right_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, flipper_shapes, COLLISION_OBJECT, 1, 2);
    VARS(work)->swing = 0;
    VARS(work)->rest = FLIPPER_RIGHT_REST;
    work->flags = FLIPPER_RIGHT;
    VARS(work)->model = flipper_right_object.model;
    task->step = flipper_right_run;
    task->draw = flipper_draw;
    task->destructor = flipper_destroy;
}

void flipper_left_x_step(struct task *task)
{
    flipper_left_step(task);
    task->work->flags |= FLIPPER_UNK_200;
}

void flipper_right_x_step(struct task *task)
{
    flipper_right_step(task);
    task->work->flags |= FLIPPER_UNK_200;
}
