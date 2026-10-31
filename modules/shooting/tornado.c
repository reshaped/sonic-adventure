/* the tornado, player 0 in the sky chase. it rides the path's point for
 * shooting_path_index, offset from it by the stick. it fires bullets, and
 * missiles at the locks' targets. each hit takes half a life. with none
 * left, it falls, and player 0 goes down */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/bomb.h"
#include "player/player.h"
#include "shooting/act1_textures.h"
#include "shooting/act2_textures.h"
#include "shooting/camera.h"
#include "shooting/fire.h"
#include "shooting/reticle.h"
#include "shooting/shooting.h"
#include "shooting/shot.h"
#include "shooting/tornado.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the frame of its pose's action. vars.f[0] is the first four bytes of
 * PLAYER_VARS. on this little-endian machine, the Float's low two bytes are
 * the player and character bytes. a store of the frame overwrites them */
#define TORNADO_FRAME(work) ((work)->vars.f[0])

enum tornado_mode {
    TORNADO_INIT,
    TORNADO_FLY,
    TORNADO_HIT, /* blinking, until its timer runs out */
    TORNADO_STILL, /* held, as PLAYER_MODE_STILL asks */
    TORNADO_FALL, /* shot down, until its pose has played once */
    TORNADO_DOWN, /* fallen. after 60 steps, player 0 goes down */
    TORNADO_EVENT /* parked at 0, 0, EVENT_Z while an event plays */
};

#define EVENT_Z (-525.0F)

/* a pose's index in tornado_poses. TORNADO_HIT, TORNADO_FALL and
 * TORNADO_DOWN each show a pose of their own, and the other modes POSE_FLY */
enum tornado_pose_index { POSE_FLY, POSE_HIT, POSE_FALL, POSE_DOWN, POSES };

struct tornado_pose {
    NJS_TEXLIST *texlist;
    NJS_ACTION *action;
};

#define FIRE_BUTTONS (pad_action_buttons | pad_jump_buttons)

/* a count in shooting_flight.charge past FULL_CHARGE is a full charge.
 * tornado_count_charge tests the count before adding the step. so a fire
 * button held 32 steps in a row charges fully. the reticle takes locks while
 * the charge is full. a release with a full charge fires the missiles */
#define FULL_CHARGE 30

#define BURST 5 /* bullets to a press */
#define HIT_STEPS 30 /* after a hit, before the next can count */
/* the value work->timer starts TORNADO_DOWN with. the step and
 * tornado_set_mode each count it down, so it runs out in 60 steps */
#define DOWN_TIMER 120
/* the submode a request gives to park the tornado as an event does */
#define SUBMODE_EVENT 27

/* the bank, shooting_flight.ang.z, drops BANK_STEP a step while the stick
 * leans left. it rises as much while the stick leans right, and stops at
 * BANK_LIMIT either side */
#define BANK_STEP 0xc0
#define BANK_LIMIT 0x1800
/* each step eases the bank BANK_EASE back toward 0, and the pitch and the
 * yaw TURN_EASE */
#define BANK_EASE 64
#define TURN_EASE 32

/* the tornado's sound as it flies: SE_FLIGHT until act 2's event, and
 * SE_FLIGHT_SECOND after it */
#define SE_FLIGHT 668
#define SE_FLIGHT_SECOND 669
#define SE_MISSILES 671
#define SE_BULLET 673
#define SE_HURT 681 /* a hit it survives */

/* how many path points ahead of its own the tornado faces */
#define FACE_AHEAD 2

/* the voices as its fall ends, in act 1 and act 2 */
#define VOICE_DOWN_ACT1 0x404
#define VOICE_DOWN_ACT2 0x42f

extern NJS_ACTION dat_0c966f1c;
extern NJS_ACTION dat_0c969c28;
extern NJS_ACTION dat_0c96d2c4;
extern NJS_ACTION dat_0c96ff00;
extern NJS_ACTION dat_0c9ee880;
extern NJS_ACTION dat_0c9f1544;
extern NJS_ACTION dat_0c9f4bc8;
extern NJS_ACTION dat_0c9f740c;
extern NJS_ACTION dat_0ca03794;
extern NJS_ACTION dat_0ca06238;
extern NJS_ACTION dat_0ca092ec;
extern NJS_ACTION dat_0ca0ba60;

static void tornado_set_mode(struct task_work *work, struct task_motion *motion,
                             struct player *record);
static void tornado_destroy(struct task *task);
static void tornado_draw(struct task *task);
static void tornado_animate(struct task *task);
static Bool tornado_clear_hurt(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record);
static Bool tornado_take_request(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record);
static void tornado_fly(struct task *task);
static void tornado_fall(struct task *task);
static void tornado_face_path(struct task_work *work, int point, Angle roll);
static void tornado_fire_missiles(struct task *task);
static void tornado_fire_bullets(struct task *task);
static void tornado_play_flight_sound(int id);

/* while set, a hit takes no life and keeps the mode */
static int tornado_invincible = 0;
/* @hack type: an array of one, not a record, which makes
 * shooting_player_step pass collision_init's count as a 1 in R2 (MOV #1,R2
 * at 0x0c901772); as a plain record, SHC pushes it from R9, and 81 of 283
 * words differ */
static struct collision_shape tornado_shape[1] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(3) | GUARD(3) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES |
         SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_OTHERS),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     20.0F,
     20.0F,
     {0},
     0,
     NULL},
};
/* each pose's texlist and action, by shooting_course_part and then by
 * pose */
static struct tornado_pose tornado_poses[3][POSES] = {
    {
        {&shooting1_texlist, &dat_0c966f1c},
        {&shooting1_texlist, &dat_0c969c28},
        {&shooting1_texlist, &dat_0c96d2c4},
        {&shooting1_texlist, &dat_0c96ff00},
    },
    {
        {&shooting2_texlist, &dat_0c9ee880},
        {&shooting2_texlist, &dat_0c9f1544},
        {&shooting2_texlist, &dat_0c9f4bc8},
        {&shooting2_texlist, &dat_0c9f740c},
    },
    {
        {&shooting2_texlist, &dat_0ca03794},
        {&shooting2_texlist, &dat_0ca06238},
        {&shooting2_texlist, &dat_0ca092ec},
        {&shooting2_texlist, &dat_0ca0ba60},
    },
};
/* how far a step the stick moves the offset at full tilt */
static Float tornado_steer = 2.0F;
/* how far the offset may go from the path's point while
 * shooting_path_running is set */
static Float tornado_reach_x = 50.0F; /* across */
static Float tornado_reach_y = 40.0F; /* up or down */
/* the speed down it gains each step as it falls */
static Float tornado_sink = 0.02F;

/* the parity of the bullets fired: the sound plays for every other one */
static int bullet_parity;

void shooting_player_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = motion->record;
    Uint16 stage;

    if ((work->flags & COLLISION_ON_TOP) && motion->speed.y <= 0.0F) {
        work->flags |= PLAYER_ON_TASK;
        /* work->collision's owner is this task, so these stores copy its
         * own angles back unchanged */
        work->ang.x = work->collision->owner->work->ang.x;
        work->ang.z = work->collision->owner->work->ang.z;
    }
    tornado_set_mode(work, motion, record);
    if (work->collision) {
        struct collision_shape *shape = work->collision->shapes;

        shape->levels = (shape->levels & ~SHAPE_ATTACK) | ATTACK(1);
        /* @hack spelling: the AND spelled out, which reads the byte with
         * MOV #4,R0 and MOV.B @(R0,R4),R0 (0x0c9016e0); as &=, SHC reads it
         * with MOV.B @(4,R4),R0, and 234 of 283 words differ */
        shape->levels = shape->levels & ~SHAPE_GUARD;
    }
    switch (work->mode) {
    case TORNADO_INIT:
        /* @hack redundant: the character copied onto itself, read through
         * task->work (MOV.B @(9,R0),R0 at 0x0c901766); without the copy, 87
         * of 283 words differ */
        PLAYER_VARS(work)->character = PLAYER_VARS(task->work)->character;
        player_init(PLAYER_VARS(work)->player, task);
        record = motion->record;
        collision_init(task, tornado_shape, COLLISION_PLAYER, 1, 1);
        work->collision->shapes->radius = record->consts.radius;
        /* @hack redundant: TORNADO_FLY stored here and again before
         * locks_clear, by the image's MOV.B R9,@R14 at 0x0c901798 and
         * 0x0c9017d2; stored once, before locks_clear, 272 of 283 words
         * differ */
        work->mode = TORNADO_FLY;
        task->destructor = tornado_destroy;
        task->draw = tornado_draw;
        work->timer = 0;
        shooting_flight.offset.x = 0.0F;
        shooting_flight.offset.y = 0.0F;
        shooting_flight.offset.z = 0.0F;
        shooting_flight.ang.x = 0;
        shooting_flight.ang.y = 0;
        shooting_flight.ang.z = 0;
        shooting_flight.bullets = 0;
        shooting_flight.charge = 0;
        shooting_flight.alpha = 0.0F;
        shooting_flight.phase = 0;
#ifdef GCC
        /* repairs TORNADO_FLY's first step, whose tornado_draw and
         * tornado_fly read shooting_flight.pos before anything sets it
         * (FMOV @(R0,R14) at 0x0c901b9c, 0x0c901ba0 and 0x0c901ba4 in the
         * draw, and at 0x0c901df2, 0x0c901e08 and 0x0c901e24 in the move).
         * SHOOTING's file ends before its bss, so the image reads what RAM
         * held there. GCC's position starts at 0 */
        shooting_flight.pos.x = 0.0F;
        shooting_flight.pos.y = 0.0F;
        shooting_flight.pos.z = 0.0F;
#endif
        work->scl.x = -2.0F;
        work->scl.z = -4.0F;
        TORNADO_FRAME(work) = 0.0F;
        work->mode = TORNADO_FLY;
        locks_clear();
        task_create(TASK_WORK, 6, reticle_step);
        break;
    case TORNADO_FLY:
        tornado_draw(task);
        tornado_fly(task);
        tornado_play_flight_sound((int)task);
        tornado_fire_missiles(task);
        tornado_fire_bullets(task);
        tornado_animate(task);
        break;
    case TORNADO_HIT:
        /* @hack type: the timer read unsigned, which gives the image's
         * EXTU.W at 0x0c901804; as work->timer, SHC tests the read as it is,
         * and 6 of 283 words differ */
        if (TASK_TIMER(work) & 3) {
            shooting_flight.alpha = 0.0F;
        } else {
            shooting_flight.alpha = -0.8F;
        }
        if (work->timer == 0) {
            shooting_flight.alpha = 0.0F;
            work->mode = TORNADO_FLY;
        }
        tornado_draw(task);
        tornado_fly(task);
        tornado_play_flight_sound((int)task);
        tornado_fire_missiles(task);
        tornado_fire_bullets(task);
        tornado_animate(task);
        break;
    case TORNADO_EVENT:
        work->pos.x = 0.0F;
        work->pos.y = 0.0F;
        work->pos.z = EVENT_Z;
        return;
    case TORNADO_FALL:
        tornado_draw(task);
        tornado_fall(task);
        tornado_animate(task);
        if (TORNADO_FRAME(work) == 0.0F) {
            work->mode = TORNADO_DOWN;
            shooting_camera_start();
            work->timer = DOWN_TIMER;
            /* @hack spelling: the stage and act through a local, which
             * copies them to R4 to AND them (MOV R0,R4 at 0x0c9018ce); in one
             * expression, SHC ANDs R0, and 85 of 283 words differ */
            stage = stage_get_stage_act();
            /* @hack type: the cast to Uint16, which gives the image's SHLR8
             * at 0x0c9018d4; uncast, SHC shifts with SHAD, and 4 of 283 words
             * differ */
            stage = (Uint16)(stage & STAGE_MASK) >> 8;
            if (stage == STAGE_SHOOTING2) {
                stream_request_voice(VOICE_DOWN_ACT2);
            } else {
                stream_request_voice(VOICE_DOWN_ACT1);
            }
        }
        break;
    case TORNADO_DOWN:
        tornado_draw(task);
        tornado_fall(task);
        tornado_animate(task);
        /* @hack type: the timer read unsigned, which gives the image's
         * EXTU.W at 0x0c90193e; as work->timer, SHC sign-extends it with
         * EXTS.W, and 1 of 283 words differ */
        if (--TASK_TIMER(work) == 0) {
            player_down(0);
        }
        break;
    /* @hack redundant: case TORNADO_STILL, which only breaks, keeps the image's
     * CMP/EQ #3 at 0x0c9016fe; without it, 216 of 283 words differ */
    case TORNADO_STILL:
        break;
    }
    task_step_children(task);
    collision_register(work);
    player_finish_step(work, motion, record);
}

/* set the mode by hits, requests and the stage task's mode. a hit is
 * dropped while work->timer runs: for HIT_STEPS after a hit, and through
 * TORNADO_DOWN. each hit taken costs half a life, and the hit that leaves
 * none starts the fall. a request is taken in TORNADO_FLY or TORNADO_STILL.
 * the stage's events park the tornado in TORNADO_EVENT, and SHOOTING_AFTER
 * sets it flying */
static void tornado_set_mode(struct task_work *work, struct task_motion *motion,
                             struct player *record)
{
    if (work->timer == 0) {
        if (tornado_clear_hurt(work, motion, record)) {
            work->flags |= PLAYER_RECOILING;
            work->flags &= ~(PLAYER_ON_PATH | PLAYER_REQUEST | PLAYER_HOLDING |
                             PLAYER_BALL);
            work->timer = HIT_STEPS;
            bomb_create(&work->pos, 2.0F);
            if (!tornado_invincible) {
                shooting_lives -= 0.5F;
                vibration_start_fading(0, 7, 59, 6);
                if (shooting_lives <= 0.0F) {
                    work->mode = TORNADO_FALL;
                    TORNADO_FRAME(work) = 0.0F;
                    work->scl.y = 0.0F;
                } else {
                    work->mode = TORNADO_HIT;
                    sound_play(SE_HURT, 0, 0, 0);
                }
            }
        } else {
            work->flags &= ~PLAYER_RECOILING;
        }
    } else {
        work->timer--;
        work->flags &= ~PLAYER_HURT;
    }
    /* @hack redundant: each request's result tested, with a break either way,
     * which keeps the image's TST R0,R0 at 0x0c901ac6 and 0x0c901ad6; as
     * bare calls, 33 of 125 words differ */
    switch (work->mode) {
    case TORNADO_FLY:
        if (tornado_take_request(work, motion, record)) {
            break;
        }
        break;
    /* @hack redundant: case TORNADO_HIT, which only breaks, keeps the image's
     * CMP/EQ #2 at 0x0c901ab2; without it, 41 of 125 words differ */
    case TORNADO_HIT:
        break;
    case TORNADO_STILL:
        if (tornado_take_request(work, motion, record)) {
            break;
        }
    }
    switch (shooting_stage_mode) {
    case SHOOTING_EVENT1:
    case SHOOTING_EVENT2:
        work->mode = TORNADO_EVENT;
        break;
    case SHOOTING_AFTER:
        work->mode = TORNADO_FLY;
        break;
    }
}

static void tornado_destroy(struct task *task)
{
    struct task_work *work = task->work;

    /* the player byte is TORNADO_FRAME's low byte. the frame is a whole
     * number under 60, and such a Float has its low 16 bits clear. so the
     * byte reads 0, the tornado's slot */
    player_free(PLAYER_VARS(work)->player);
}

/* draw the pose of its mode, on a frame's first step only. the pose stands
 * at shooting_flight.offset from the path's point, turned by the path's
 * turn and then by shooting_flight.ang */
static void tornado_draw(struct task *task)
{
    struct task_work *work = task->work;
    int pose;

    if (frame_step) {
        return;
    }
    switch (work->mode) {
    /* @hack redundant: case TORNADO_FLY, which does what the default does,
     * keeps the image's CMP/EQ #1 at 0x0c901b30; without it, 133 of 144 words
     * differ */
    case TORNADO_FLY:
        pose = POSE_FLY;
        break;
    case TORNADO_HIT:
        pose = POSE_HIT;
        break;
    case TORNADO_FALL:
        pose = POSE_FALL;
        break;
    case TORNADO_DOWN:
        pose = POSE_DOWN;
        break;
    default:
        pose = POSE_FLY;
    }
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, shooting_flight.pos.x, shooting_flight.pos.y,
                shooting_flight.pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njPushMatrix(NULL);
    njTranslate(NULL, shooting_flight.offset.x, shooting_flight.offset.y,
                shooting_flight.offset.z);
    njRotateXYZ(NULL, shooting_flight.ang.x, shooting_flight.ang.y,
                shooting_flight.ang.z);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(shooting_flight.alpha, 0.0F, 0.0F, 0.0F);
    njSetTexture(tornado_poses[shooting_course_part][pose].texlist);
    draw_action(tornado_poses[shooting_course_part][pose].action,
                TORNADO_FRAME(work));
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
    njPopMatrix(1);
}

/* move TORNADO_FRAME on by one through the action of its mode's pose. it
 * wraps to 0 at the action's frame count */
static void tornado_animate(struct task *task)
{
    struct task_work *work = task->work;
    int pose;
    Float end;

    switch (work->mode) {
    /* @hack redundant: case TORNADO_FLY, as in tornado_draw, keeps the image's
     * CMP/EQ #1 at 0x0c901c6a; without it, 30 of 57 words differ */
    case TORNADO_FLY:
        pose = POSE_FLY;
        break;
    case TORNADO_HIT:
        pose = POSE_HIT;
        break;
    case TORNADO_FALL:
        pose = POSE_FALL;
        break;
    case TORNADO_DOWN:
        pose = POSE_DOWN;
        break;
    default:
        pose = POSE_FLY;
    }
    TORNADO_FRAME(work) += 1.0F;
    /* @hack spelling: the action's frame count through a Float, whose
     * conversion tests the float's sign with FCMP/GT (0x0c901d0c); compared
     * in place, SHC tests the count's sign with CMP/PZ, and 27 of 57 words
     * differ */
    end = tornado_poses[shooting_course_part][pose].action->motion->nbFrame;
    if (TORNADO_FRAME(work) >= end) {
        TORNADO_FRAME(work) = 0.0F;
    }
}

/* clear work's PLAYER_HURT. it returns TRUE if the flag was set, else
 * FALSE */
static Bool tornado_clear_hurt(struct task_work *work,
                               struct task_motion *motion,
                               struct player *record)
{
    if (work->flags & PLAYER_HURT) {
        work->flags &= ~PLAYER_HURT;
        return TRUE;
    }
    return FALSE;
}

/* take the submode a request asks for. PLAYER_MODE_STILL holds the
 * tornado still, PLAYER_MODE_RESET sets it flying, and SUBMODE_EVENT parks
 * it. it returns TRUE for those, and for PLAYER_MODE_SPRING and
 * PLAYER_MODE_LAUNCH, which change nothing. it returns FALSE for any other
 * submode, or with no request */
static Bool tornado_take_request(struct task_work *work,
                                 struct task_motion *motion,
                                 struct player *record)
{
    if (work->flags & PLAYER_REQUEST) {
        switch (work->submode) {
        case PLAYER_MODE_SPRING:
        case PLAYER_MODE_LAUNCH:
            goto taken;
        case PLAYER_MODE_STILL:
            work->mode = TORNADO_STILL;
            goto taken;
        case PLAYER_MODE_RESET:
            work->mode = TORNADO_FLY;
            goto taken;
        case SUBMODE_EVENT:
            work->mode = TORNADO_EVENT;
            goto taken;
        /* @hack redundant: case PLAYER_MODE_DOWN, which only breaks, keeps the
         * image's CMP/EQ #50 at 0x0c901d7e; without it, 19 of 33 words
         * differ */
        case PLAYER_MODE_DOWN:
            break;
        }
    }
    return FALSE;
    /* @hack spelling: one return of TRUE for every request taken, which SHC
     * writes after the pool, at 0x0c901dc0; returned in each case, it comes
     * before the pool, and 16 of 33 words differ */
taken:
    return TRUE;
}

/* fly at the path's point for shooting_path_index, offset from it along
 * the axes of the path's turn. work->scl takes the point's move, and
 * shooting_flight.speed its length. the stick moves the offset and banks
 * the tornado */
static void tornado_fly(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR origin;
    NJS_POINT3 point;
    Float roll;
    Float bank;

    njGetPeripheral(0);
    work->scl.x = shooting_path[shooting_path_index].x - shooting_flight.pos.x;
    work->scl.y = shooting_path[shooting_path_index].y - shooting_flight.pos.y;
    work->scl.z = shooting_path[shooting_path_index].z - shooting_flight.pos.z;
    shooting_flight.pos.x = shooting_path[shooting_path_index].x;
    shooting_flight.pos.y = shooting_path[shooting_path_index].y;
    shooting_flight.pos.z = shooting_path[shooting_path_index].z;
    tornado_face_path(work, shooting_path_index, 0);
    origin.x = 0.0F;
    origin.y = 0.0F;
    origin.z = 0.0F;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, shooting_flight.pos.x, shooting_flight.pos.y,
                shooting_flight.pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njTranslate(NULL, shooting_flight.offset.x, shooting_flight.offset.y,
                shooting_flight.offset.z);
    njCalcPoint(NULL, &origin, &point);
    njPopMatrix(1);
    work->pos.x = point.x;
    work->pos.y = point.y;
    work->pos.z = point.z;
    if (shooting_flight.ang.x > 0) {
        shooting_flight.ang.x -= TURN_EASE;
        if (shooting_flight.ang.x < 0) {
            shooting_flight.ang.x = 0;
        }
    }
    if (shooting_flight.ang.x < 0) {
        shooting_flight.ang.x += TURN_EASE;
        if (shooting_flight.ang.x > 0) {
            shooting_flight.ang.x = 0;
        }
    }
    if (shooting_flight.ang.z > 0) {
        shooting_flight.ang.z -= BANK_EASE;
        if (shooting_flight.ang.z < 0) {
            shooting_flight.ang.z = 0;
        }
    }
    if (shooting_flight.ang.z < 0) {
        shooting_flight.ang.z += BANK_EASE;
        if (shooting_flight.ang.z > 0) {
            shooting_flight.ang.z = 0;
        }
    }
    if (shooting_flight.ang.y > 0) {
        shooting_flight.ang.y -= TURN_EASE;
        if (shooting_flight.ang.y < 0) {
            shooting_flight.ang.y = 0;
        }
    }
    if (shooting_flight.ang.y < 0) {
        shooting_flight.ang.y += TURN_EASE;
        if (shooting_flight.ang.y > 0) {
            shooting_flight.ang.y = 0;
        }
    }
    shooting_flight.offset.x -=
        (Float)(pad_records[0].x1 * 256) * tornado_steer * (1.0F / 32768.0F);
    if (shooting_path_running) {
        if (shooting_flight.offset.x > tornado_reach_x) {
            shooting_flight.offset.x = tornado_reach_x;
        }
        if (shooting_flight.offset.x < -tornado_reach_x) {
            shooting_flight.offset.x = -tornado_reach_x;
        }
    }
    shooting_flight.offset.y -=
        (Float)(pad_records[0].y1 * 256) * tornado_steer * (1.0F / 32768.0F);
    if (shooting_path_running) {
        if (shooting_flight.offset.y > tornado_reach_y) {
            shooting_flight.offset.y = tornado_reach_y;
        }
        if (shooting_flight.offset.y < -tornado_reach_y) {
            shooting_flight.offset.y = -tornado_reach_y;
        }
    }
    /* @hack spelling: the stick tested times 256, which keeps the image's
     * SHLL8 before each test (0x0c902044, 0x0c90205c); tested as it is, 91
     * of 363 words differ */
    if (pad_records[0].x1 * 256 < 0) {
        shooting_flight.ang.z -= BANK_STEP;
        if (shooting_flight.ang.z < -BANK_LIMIT) {
            shooting_flight.ang.z = -BANK_LIMIT;
        }
    }
    if (pad_records[0].x1 * 256 > 0) {
        shooting_flight.ang.z += BANK_STEP;
        if (shooting_flight.ang.z > BANK_LIMIT) {
            shooting_flight.ang.z = BANK_LIMIT;
        }
    }
    /* the camera rolls by half the bank, times the square of
     * shooting_flight.offset.x / tornado_reach_x */
    /* @hack spelling: a statement to each step, which makes the bank a float
     * first (FLOAT at 0x0c902080), multiplies 0.5F in before it (FMUL at
     * 0x0c902090), and moves njRoundOff's result to FR4 to truncate
     * (0x0c902096); in one expression, 307 of 363 words differ */
    bank = shooting_flight.ang.z;
    roll = shooting_flight.offset.x * shooting_flight.offset.x /
           (tornado_reach_x * tornado_reach_x) * 0.5F;
    roll *= bank;
    roll = njRoundOff(roll);
    shooting_camera_ang_z = roll;
    /* player_work[0] is the tornado's work. any submode set there starts
     * pom's six flames on the tornado, and is cleared */
    if (player_work[0]->submode) {
        task_create_child(TASK_WORK, pom_step, task);
        player_work[0]->submode = 0;
    }
    shooting_flight.speed =
        njSqrt((work->scl.x * work->scl.x) + (work->scl.y * work->scl.y) +
               (work->scl.z * work->scl.z));
}

/* follow the path's point as tornado_fly does, with no stick. the offset
 * falls by work->scl.y, which loses tornado_sink each step */
static void tornado_fall(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR origin;
    NJS_POINT3 point;

    njGetPeripheral(0);
    work->scl.x = shooting_path[shooting_path_index].x - shooting_flight.pos.x;
    work->scl.z = shooting_path[shooting_path_index].z - shooting_flight.pos.z;
    shooting_flight.pos.x = shooting_path[shooting_path_index].x;
    shooting_flight.pos.y = shooting_path[shooting_path_index].y;
    shooting_flight.pos.z = shooting_path[shooting_path_index].z;
    tornado_face_path(work, shooting_path_index, 0);
    origin.x = 0.0F;
    origin.y = 0.0F;
    origin.z = 0.0F;
    work->scl.y -= tornado_sink;
    shooting_flight.offset.y += work->scl.y;
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, shooting_flight.pos.x, shooting_flight.pos.y,
                shooting_flight.pos.z);
    njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
    njTranslate(NULL, shooting_flight.offset.x, shooting_flight.offset.y,
                shooting_flight.offset.z);
    njCalcPoint(NULL, &origin, &point);
    njPopMatrix(1);
    work->pos.x = point.x;
    work->pos.y = point.y;
    work->pos.z = point.z;
}

/* turn work from the path's point toward the one FACE_AHEAD on, and roll
 * it by roll. a point past the path's last is its first */
static void tornado_face_path(struct task_work *work, int point, Angle roll)
{
    int ahead;
    Float dx;
    Float dy;
    Float dz;
    Float flat;
    Angle ang_x;
    Angle ang_y;

    if (point >= shooting_path_count) {
        point = 0;
    }
    ahead = point + FACE_AHEAD;
    if (ahead >= shooting_path_count) {
        ahead = 0;
    }
    dx = shooting_path[ahead].x - shooting_path[point].x;
    dy = shooting_path[ahead].y - shooting_path[point].y;
    dz = shooting_path[ahead].z - shooting_path[point].z;
    flat = njSqrt((dx * dx) + (dz * dz));
    ang_y = 0x4000 - (Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN);
    ang_x = -(Angle)(atan2f(dy, flat) * ANGLE_PER_RADIAN);
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = roll;
}

Bool tornado_count_charge(void)
{
    if (pad_records[0].on & FIRE_BUTTONS) {
        if (shooting_flight.charge++ > FULL_CHARGE) {
            return TRUE;
        }
        return FALSE;
    }
    shooting_flight.charge = 0;
    return FALSE;
}

/* fire the missiles on the step that lets go of a fire button and leaves
 * none held. they fire only with shooting_flight.charge past FULL_CHARGE. a
 * missile goes at each lock's target. SE_MISSILES plays if any went, and
 * the pad shakes either way */
static void tornado_fire_missiles(struct task *task)
{
    int count;
    int i;
    struct task *shot;
    Bool jump_up;
    Bool action_up;

    /* @hack spelling: both tests made before either branch, with MOVT at
     * 0x0c902412 and 0x0c90241c, and jump_up kept at @R15; tested in the
     * condition, SHC branches on each TST, and 193 of 201 words differ */
    jump_up = !(pad_records[0].on & pad_jump_buttons);
    action_up = !(pad_records[0].on & pad_action_buttons);
    if (!jump_up || !action_up) {
        return;
    }
    if (!(pad_records[0].release & FIRE_BUTTONS)) {
        return;
    }
    if (shooting_flight.charge++ > FULL_CHARGE) {
        count = 0;
        for (i = 0; i < LOCKS; i++) {
            if (lock_targets[i]) {
                /* @hack spelling: shot assigned in the test, which tests R0
                 * with TST R0,R0 and copies it to R4 in the delay slot
                 * (0x0c90246c); as a statement of its own, as the bullet's
                 * is, SHC tests the copy in R4, and 15 of 201 words differ */
                if ((shot = task_create_child(TASK_WORK, shooting_shot_step,
                                              task)) != NULL) {
                    shot->extra = lock_targets[i];
                    SHOT_VARS(shot->work)->lock = i;
                    count++;
                }
            }
        }
        if (count) {
            sound_play(SE_MISSILES, 0, 0, 0);
        }
        vibration_start(0, 6, 30, 0);
    }
}

/* a press of a fire button starts a burst of BURST bullets, one every
 * fourth step */
static void tornado_fire_bullets(struct task *task)
{
    struct task *shot;

    if (pad_records[0].press & FIRE_BUTTONS) {
        shooting_flight.bullets = BURST;
    }
    if (shooting_flight.phase == 0 && shooting_flight.bullets > 0) {
        shot = task_create_child(TASK_WORK, shooting_shot_step, task);
        if (shot) {
            /* @hack type: lock set to -1 as 0xff through a Uint8, which the
             * image loads with MOV.W at 0x0c9025f8; stored as -1, SHC makes
             * it with MOV #-1,R0, and 11 of 62 words differ */
            *(Uint8 *)&SHOT_VARS(shot->work)->lock = 0xff;
            shooting_flight.bullets--;
            if (!bullet_parity) {
                sound_play(SE_BULLET, 0, 0, 0);
            }
            bullet_parity++;
            bullet_parity &= 1;
        }
    }
    shooting_flight.phase++;
    shooting_flight.phase &= 3;
}

/* on a frame's first step, keep the course part's flight sound playing for
 * id, at volume -32 */
static void tornado_play_flight_sound(int id)
{
    if (frame_step) {
        return;
    }
    switch (shooting_course_part) {
    case 0:
    case 1:
        sound_play_for(SE_FLIGHT, id, 1, 0, 4);
        sound_set_volume(id, -32);
        break;
    case 2:
        sound_play_for(SE_FLIGHT_SECOND, id, 1, 0, 4);
        sound_set_volume(id, -32);
        break;
    }
}
