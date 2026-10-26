/* Perfect Chaos's ball of light, which falls short of player 0 and raises a
 * column of water */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/ball.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 16
#define TEXT_2_LENGTH 17
#define TEXT_3_LENGTH 17
#define TEXT_4_LENGTH 16
#define TEXT_5_LENGTH 16
#define TEXT_6_LENGTH 16
#define TEXT_7_LENGTH 16
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 10
#include "b_chaos7/ball/text.inc"

extern NJS_OBJECT ball_splash_0_object;
extern NJS_OBJECT ball_splash_1_object;
extern NJS_OBJECT ball_splash_2_object;
extern NJS_OBJECT ball_splash_3_object;
extern NJS_OBJECT ball_splash_4_object;

#define SE_BALL_HIT 443

/* the ball's starting height, and the sea's, where it splashes */
#define BALL_HEIGHT 600.0F
#define BALL_SEA 0.0F
/* the sphere's radius while the ball falls, and from its splash on */
#define BALL_RADIUS 80.0F
#define SPLASH_RADIUS 120.0F
/* how far the ball's shapes reach from its pos, as its collision's radius
 * says: past the sphere at either radius */
#define BALL_REACH 200.0F
/* the sprite holds each frame for SPRITE_STEPS steps. as the ball falls it
 * shows frame 0, then 1 and 2 in turn, wrapping to 1 at BALL_FALL_FRAMES.
 * as it hits it shows frames 0 to 4 once, and hides at BALL_HIT_FRAMES */
#define SPRITE_STEPS 2
#define BALL_FALL_FRAMES 3
#define BALL_HIT_FRAMES 5
/* the splash's motions run to frame SPLASH_END, where the task ends. the
 * splash fades out over their last SPLASH_FADE_FRAMES, from SPLASH_FADE */
#define SPLASH_END 49.0F
#define SPLASH_FADE 35.0F
#define SPLASH_FADE_FRAMES (SPLASH_END - SPLASH_FADE)
/* the column shows c7_watercolumn01 to 05 in turn, COLUMN_STEPS steps
 * each */
#define COLUMN_TEXTURES 5
#define COLUMN_STEPS 2
/* the turn ang.x takes each step, a sixteenth of a turn */
#define BALL_SPIN 0x1000

/* in work->flags, the sprite draws while BALL_SHOWN is set, and the splash
 * while SPLASH_SHOWN is */
#define BALL_SHOWN 0x100
#define SPLASH_SHOWN 0x200

struct ball_vars {
    Uint8 sprite_steps;
    Uint8 sprite_frame;
    Uint8 column_steps;
    Uint8 column_texture;
    Uint8 unk_04[4];
    Float splash_frame;
};

#define VARS(work) ((struct ball_vars *)&(work)->vars)
#define SPLASH_SCALE(work) ((work)->scl.x)

enum ball_mode { BALL_FALLING, BALL_HIT, BALL_SPLASHING };

/* the ball's speed in units a step, how far short of player 0 it falls, and
 * the splash's motion frames a step */
#pragma section HEAD
static Float ball_speed = 30.0F;
static Float ball_short = 150.0F;
static Float ball_splash_pace = 0.5F;
#pragma section

static const Uint8 ball_shape_count = 1;

/* the splash's five parts, each a motion of fifty frames that starts near a
 * scale of 0. from frame 9, parts 0 and 1 turn a quarter turn about y, 0
 * one way and 1 the other */
static NJS_MKEY_A ball_splash_0_ang[3] = {
#include "b_chaos7/ball/ball_splash_0_ang.inc"
};
static NJS_MKEY_F ball_splash_0_scl[3] = {
#include "b_chaos7/ball/ball_splash_0_scl.inc"
};
static NJS_MDATA3 ball_splash_0_mdata[1] = {
#include "b_chaos7/ball/ball_splash_0_mdata.inc"
};
static NJS_MOTION ball_splash_0_motion = {
#include "b_chaos7/ball/ball_splash_0_motion.inc"
};
static NJS_ACTION ball_splash_0_action = {
#include "b_chaos7/ball/ball_splash_0_action.inc"
};

static NJS_MKEY_A ball_splash_1_ang[3] = {
#include "b_chaos7/ball/ball_splash_1_ang.inc"
};
static NJS_MKEY_F ball_splash_1_scl[3] = {
#include "b_chaos7/ball/ball_splash_1_scl.inc"
};
static NJS_MDATA3 ball_splash_1_mdata[1] = {
#include "b_chaos7/ball/ball_splash_1_mdata.inc"
};
static NJS_MOTION ball_splash_1_motion = {
#include "b_chaos7/ball/ball_splash_1_motion.inc"
};
static NJS_ACTION ball_splash_1_action = {
#include "b_chaos7/ball/ball_splash_1_action.inc"
};

static NJS_MKEY_F ball_splash_2_scl[4] = {
#include "b_chaos7/ball/ball_splash_2_scl.inc"
};
static NJS_MDATA3 ball_splash_2_mdata[1] = {
#include "b_chaos7/ball/ball_splash_2_mdata.inc"
};
static NJS_MOTION ball_splash_2_motion = {
#include "b_chaos7/ball/ball_splash_2_motion.inc"
};
static NJS_ACTION ball_splash_2_action = {
#include "b_chaos7/ball/ball_splash_2_action.inc"
};

static NJS_MKEY_F ball_splash_3_scl[4] = {
#include "b_chaos7/ball/ball_splash_3_scl.inc"
};
static NJS_MDATA3 ball_splash_3_mdata[1] = {
#include "b_chaos7/ball/ball_splash_3_mdata.inc"
};
static NJS_MOTION ball_splash_3_motion = {
#include "b_chaos7/ball/ball_splash_3_motion.inc"
};
static NJS_ACTION ball_splash_3_action = {
#include "b_chaos7/ball/ball_splash_3_action.inc"
};

static NJS_MKEY_F ball_splash_4_scl[4] = {
#include "b_chaos7/ball/ball_splash_4_scl.inc"
};
static NJS_MDATA3 ball_splash_4_mdata[1] = {
#include "b_chaos7/ball/ball_splash_4_mdata.inc"
};
static NJS_MOTION ball_splash_4_motion = {
#include "b_chaos7/ball/ball_splash_4_motion.inc"
};
static NJS_ACTION ball_splash_4_action = {
#include "b_chaos7/ball/ball_splash_4_action.inc"
};

/* the splash's textures. ball_step points texture 0, c7_watercolumn01, at
 * each of the column's five in turn, as ball_sprite_textures loaded them */
static NJS_TEXNAME ball_splash_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST ball_splash_texlist = {
#include "b_chaos7/ball/ball_splash_texlist.inc"
};
/* c7_watercolumn01 to 05, whose addresses ball_step copies to the splash's
 * texture 0, then the sprite's c7_flash01 to 08 */
static NJS_TEXNAME ball_sprite_textures[13] = {
    {TEXT_1, 0, 0},  {TEXT_4, 0, 0},  {TEXT_5, 0, 0},  {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},  {TEXT_10, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
    {TEXT_15, 0, 0},
};
NJS_TEXLIST ball_sprite_texlist = {
#include "b_chaos7/ball/ball_sprite_texlist.inc"
};

/* c7_flash01 to 08, of which the ball shows the first five */
static NJS_TEXANIM ball_sprite_frames[8] = {
#include "b_chaos7/ball/ball_sprite_frames.inc"
};
static NJS_SPRITE ball_sprite = {
#include "b_chaos7/ball/ball_sprite.inc"
};

static struct collision_shape ball_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(2) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_OBJECTS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static NJS_ARGB ball_color = {1.0F, 1.0F, 1.0F, 1.0F};

/* the ball's draw. it draws only on a frame's first step. the sprite faces
 * the camera and adds its light. the splash draws parts 0, 1 and 4 at
 * SPLASH_SCALE, and parts 2 and 3 four times as large. it fades out from
 * SPLASH_FADE */
static void ball_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float alpha;
    Float scale;

    if (frame_step) {
        return;
    }
    njFogDisable();
    njSetTexture(&ball_splash_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (work->flags & BALL_SHOWN) {
        njPushMatrix(NULL);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njSetConstantMaterial(&ball_color);
        njDrawSprite3D(&ball_sprite, VARS(work)->sprite_frame,
                       NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
        njPopMatrix(1);
    }
    if (work->flags & SPLASH_SHOWN) {
        draw_control_save();
        draw_control_clear(NJD_CONTROL_3D_CONSTANT_ATTR |
                           NJD_CONTROL_3D_CONSTANT_MATERIAL |
                           NJD_CONTROL_3D_OFFSET_MATERIAL);
        draw_control_clear(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
        if (VARS(work)->splash_frame >= SPLASH_FADE) {
            alpha = -((VARS(work)->splash_frame - SPLASH_FADE) *
                      (1.0F / SPLASH_FADE_FRAMES));
        } else {
            alpha = 0.0F;
        }
        draw_set_material(alpha, 0.0F, 0.0F, 0.0F);
        scale = SPLASH_SCALE(work);
        njScale(NULL, scale, scale, scale);
        njSetTexture(&ball_splash_texlist);
        draw_action(&ball_splash_0_action, VARS(work)->splash_frame);
        draw_action(&ball_splash_1_action, VARS(work)->splash_frame);
        draw_action(&ball_splash_4_action, VARS(work)->splash_frame);
        njScale(NULL, 4.0F, 4.0F, 4.0F);
        draw_action(&ball_splash_2_action, VARS(work)->splash_frame);
        draw_action(&ball_splash_3_action, VARS(work)->splash_frame);
        draw_control_restore();
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    njPopMatrix(1);
    njFogEnable();
}

/* the ball falls by motion's speed to the sea, flashes as it hits, then
 * plays the splash to SPLASH_END and destroys its task. the column's
 * texture changes every COLUMN_STEPS steps throughout */
static void ball_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    switch (work->mode) {
    case BALL_FALLING:
        work->collision->shapes->radius = BALL_RADIUS;
        if (++VARS(work)->sprite_steps >= SPRITE_STEPS) {
            VARS(work)->sprite_steps = 0;
            if (++VARS(work)->sprite_frame >= BALL_FALL_FRAMES) {
                VARS(work)->sprite_frame = 1;
            }
        }
        njAddVector(&work->pos, &motion->speed);
        if (work->pos.y <= BALL_SEA) {
            VARS(work)->splash_frame = 1.0F;
            work->flags |= SPLASH_SHOWN;
            work->collision->shapes->radius = SPLASH_RADIUS;
            work->mode = BALL_HIT;
            VARS(work)->sprite_steps = 0;
            VARS(work)->sprite_frame = 0;
            sound_play_at(SE_BALL_HIT, 0, 0, 120, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case BALL_HIT:
        if (++VARS(work)->sprite_steps >= SPRITE_STEPS) {
            VARS(work)->sprite_steps = 0;
            if (++VARS(work)->sprite_frame >= BALL_HIT_FRAMES) {
                work->flags &= ~BALL_SHOWN;
                work->mode = BALL_SPLASHING;
            }
        }
        break;
    case BALL_SPLASHING:
        VARS(work)->splash_frame += ball_splash_pace;
        if (VARS(work)->splash_frame >= SPLASH_END) {
            task_destroy(task);
            return;
        }
        break;
    }
    /* the spin changes nothing: ball_draw turns by the camera's angles, and
     * the shape is a sphere at the ball's center */
    work->ang.x += BALL_SPIN;
    VARS(work)->column_steps++;
    if (VARS(work)->column_steps >= COLUMN_STEPS) {
        VARS(work)->column_steps = 0;
        VARS(work)->column_texture++;
        if (VARS(work)->column_texture >= COLUMN_TEXTURES) {
            VARS(work)->column_texture = 0;
        }
    }
    ball_splash_textures[0].texaddr =
        ball_sprite_textures[VARS(work)->column_texture].texaddr;
    ball_draw(task);
    collision_register(work);
}

static void ball_start(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, ball_shape, COLLISION_ITEM, 1, ball_shape_count);
    /* @hack spelling: the collision reached through task->work, not work, which
     * reloads task->work after collision_init (MOV.L @(32,R14),R2 at
     * 0x0c91903c); through work, 28 of 39 words differ */
    task->work->collision->radius = BALL_REACH;
    SPLASH_SCALE(work) = 1.0F;
    VARS(work)->splash_frame = 0.0F;
    work->flags |= BALL_SHOWN;
    task->step = ball_step;
    task->draw = ball_draw;
}

void chaos7_ball_create(void)
{
    struct task *task;
    struct task_work *work;
    struct task_motion *motion;
    NJS_POINT3 target;
    NJS_POINT3 to;
    Angle ang;
    Float scale;

    task = task_create(TASK_WORK | TASK_MOTION, 2, ball_start);
    if (task) {
        work = task->work;
        motion = task->motion;
        work->pos = boss_work->pos;
        work->pos.y = BALL_HEIGHT;
        to = target = player_work[0]->pos;
        njSubVector(&to, &work->pos);
        ang = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
        target.x -= ball_short * njCos(ang);
        target.z -= ball_short * njSin(ang);
        target.y = BALL_SEA;
        motion->speed = target;
        njSubVector(&motion->speed, &work->pos);
        scale = ball_speed / njScalor(&motion->speed);
        motion->speed.x *= scale;
        motion->speed.y *= scale;
        motion->speed.z *= scale;
    }
}
