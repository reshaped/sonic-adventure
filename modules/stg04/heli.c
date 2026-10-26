/* O HeliA and O HeliB, helicopters that hover with a searchlight, and
 * heli_route's helicopter, which carries player 0 */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/common.h"
#include "path/path.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg04/heli.h"
#include "stg04/paths.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT heli_object;
/* the main rotor at rest, and the spinning rotor with its blur */
extern NJS_MODEL heli_rotor_model;
extern NJS_OBJECT heli_rotor_object;
extern NJS_OBJECT heli_light_object;

/* the turn of the main rotor, and of the searchlight */
struct heli_vars {
    Angle rotor;
    Angle tilt; /* about x */
    Angle pan; /* about y, from the helicopter's heading */
};

#define HELI(work) ((struct heli_vars *)&(work)->vars)

/* the rotor's speed, in angle units a step. from ROTOR_BLUR up, heli_draw
 * draws the spinning rotor. the route's helicopter spins up by ROTOR_GAIN a
 * step to ROTOR_LIFT. the hovering ones spin at ROTOR_HOVER */
#define ROTOR_SPEED(work) TASK_TIMER(work)
#define ROTOR_BLUR 0xc00
#define ROTOR_GAIN 64
#define ROTOR_LIFT 0xf00
#define ROTOR_HOVER 0x1000

/* DISTANCE is how far along heli_route the route's helicopter is. SPEED is
 * how far it goes each step. heli_animate adds BOB to y. a hovering
 * helicopter keeps the sum, so BOB is how far it rises each step. the
 * route's helicopter goes back onto heli_route, so BOB is its height above
 * the route */
#define DISTANCE(work) ((work)->scl.x)
#define SPEED(work) ((work)->scl.y)
#define BOB(work) ((work)->scl.z)

/* in the work's flags, the side of the route's helicopter player 0 hangs
 * on. HANG_ALONG and HANG_ALONG_TURNED put player 0 toward x njSin, z njCos
 * of the heading. HANG_AGAINST and HANG_AGAINST_TURNED put it the other way.
 * a _TURNED flag turns player 0 a half turn more */
#define HANG_AGAINST_TURNED 0x100
#define HANG_AGAINST 0x200
#define HANG_ALONG_TURNED 0x400
#define HANG_ALONG 0x800
#define HANG_ANY 0xf00
/* set while heli_carry_step runs the skids' steps once, as boarding opens.
 * each skid's step then sets COLLISION_TARGET in its collision record,
 * where it stays */
#define HELI_TARGET_SKIDS 0x1000
/* while HELI_BOB is set, heli_animate swings BOB by BOB_STEP a step, between
 * -BOB_MOST and BOB_MOST. HELI_RISE is set while BOB grows. the swing clears
 * HELI_RISE by adding it, and sets it by subtracting it. so each change of
 * HELI_RISE also flips bit 0x8000 */
#define HELI_BOB 0x2000
#define HELI_RISE 0x4000
#define BOB_STEP 0.2F
#define BOB_MOST 2.0F

/* in the helicopter's frame, the searchlight's mount is LIGHT_X along -x and
 * LIGHT_Y along -y from its center */
#define LIGHT_X 28.884885F
#define LIGHT_Y 11.80179F

/* the searchlight's tilt as a helicopter starts */
#define TILT_START 0x800

/* a sweep steps the tilt by SWEEP_TILT and the pan by SWEEP_PAN. it turns
 * back at a tilt of SWEEP_MOST or -SWEEP_MOST */
#define SWEEP_TILT 0x180
#define SWEEP_PAN 0x100
#define SWEEP_MOST 0x1800

/* LIGHT_FOLLOW aims the tilt a degree for each unit of heli_aim_length,
 * which it holds to AIM_LONGEST. it steps the tilt by FOLLOW_TILT while more
 * than that off its aim. it steps the pan toward its aim by FOLLOW_PAN */
#define AIM_LONGEST 50.0F
#define FOLLOW_TILT 64
#define FOLLOW_PAN 0x180

#define SE_ROTOR 96

#define ANIMATE_NEAR 1000.0F
#define ROUTE_TURN 240

/* player 0 hangs HANG_BELOW the route's helicopter, plus its bob, and drops
 * from LET_GO_BELOW. a flag's _OUT, as HANG_ALONG_OUT, is how far across
 * from the center that flag holds player 0. each skid is SKID_BELOW the
 * helicopter and SKID_SIDE from it along z */
#define HANG_BELOW 27.0F
#define LET_GO_BELOW 30.0F
#define SKID_BELOW 25.0F
#define SKID_SIDE 12.0F
#define HANG_AGAINST_TURNED_OUT 11.2F
#define HANG_AGAINST_OUT 13.8F
#define HANG_ALONG_TURNED_OUT 14.2F
#define HANG_ALONG_OUT 12.0F

/* player 0 wakes the route's helicopter by entering the wake box. the box
 * bounds x and z, and leaves y free */
#define WAKE_X_MIN 3120.0F
#define WAKE_X_MAX 3500.0F
#define WAKE_Z_MIN 1220.0F
#define WAKE_Z_MAX 1600.0F

/* player 0 boards from between GRAB_TOP and GRAB_BOTTOM below the
 * helicopter. it must also be within GRAB_REACH of the helicopter across,
 * in x and z alone */
#define GRAB_TOP 10.0F
#define GRAB_BOTTOM 32.0F
#define GRAB_REACH 20.0F

/* distances along the route, as DISTANCE measures them */
#define BOARD_DISTANCE 30.0F
#define BRAKE_DISTANCE 940.0F
#define LET_GO_DISTANCE 1100.0F
#define RETURN_END 2200.0F

/* the route's helicopter creeps by CREEP a step to BOARD_DISTANCE. carrying
 * player 0, it gains CARRY_GAIN of speed a step up to CARRY_SPEED. each such
 * step also leans it CARRY_LEAN further about z. past BRAKE_DISTANCE, it
 * loses BRAKE of speed a step, and levels by BRAKE_LEVEL a step. once it
 * lets player 0 go, its speed starts at RETURN_START. it then gains
 * RETURN_GAIN a step up to RETURN_SPEED, leaning RETURN_LEAN further each
 * step */
#define CREEP 0.5F
#define CARRY_SPEED 6.0F
#define CARRY_GAIN 0.06F
#define CARRY_LEAN 32
#define BRAKE 0.1F
#define BRAKE_LEVEL 96
#define RETURN_START 1.0F
#define RETURN_SPEED 4.0F
#define RETURN_GAIN 0.12F
#define RETURN_LEAN 64

/* @hack redundant: a 1 behind a select on a constant float, which the image
 * loads afresh for the rotor's collision_init (MOV #1,R2 at 0x0c903832). as
 * a plain 1, which SHC keeps in R14 across the calls, 76 of 81 words
 * differ */
#define ROTOR_ONE (1.0F ? 1 : 1)
/* @hack redundant: the same on another constant, for the first skid's
 * collision_init, whose 1 the image loads afresh too (MOV #1,R3 at
 * 0x0c903870). as a plain 1, 68 of 81 words differ. as ROTOR_ONE, the
 * function outgrows its row */
/* NOLINTNEXTLINE(clang-diagnostic-literal-conversion) */
#define SKID_ONE (2.0F ? 1 : 1)

/* the searchlight's motion, in the work's submode. LIGHT_SWEEP_UP and
 * LIGHT_SWEEP_DOWN step the tilt and pan up and down in turn */
enum heli_light { LIGHT_STILL, LIGHT_SWEEP_UP, LIGHT_SWEEP_DOWN, LIGHT_FOLLOW };

/* the route's helicopter, in the work's mode */
enum heli_mode {
    HELI_WAIT, /* until player 0 enters the wake box */
    HELI_SPIN_UP, /* the rotor to ROTOR_LIFT, then on to BOARD_DISTANCE */
    HELI_BOARD, /* until player 0 comes under it and hangs on */
    HELI_CARRY, /* on to LET_GO_DISTANCE, where it lets player 0 go */
    HELI_RETURN /* on to RETURN_END, then back to BOARD_DISTANCE */
};

/* the rotor's blades, which hurt players */
static struct collision_shape heli_rotor_shape = {
    SHAPE_CYLINDER | SHAPE_HURTABLE,
    SHAPE_HURTS_PLAYERS | SHAPE_GUARD | SHAPE_ATTACK,
    0,
    {0.0F, 20.0F, 0.0F},
    75.0F,
    1.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* each skid's: a sphere that pushes nothing. HELI_TARGET_SKIDS makes its
 * record a target, which players can aim at */
static struct collision_shape heli_skid_shape = {
    SHAPE_SPHERE,
    SHAPE_GUARD | SHAPE_ATTACK,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    20.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};

/* heli_sample_route's results, and heli_animate's aim at player 0. heli_aim
 * is first the tilt the light turns toward, then how far its pan is off */
static NJS_POINT3 heli_route_pos;
static Angle heli_route_ang;
static Angle heli_aim;
static Float heli_aim_length;
static Float heli_aim_x;
static Float heli_aim_z;
static Angle heli_route_turn;
static Angle heli_route_dir;

static void heli_destroy(struct task *task)
{
    task_destroy_children(task);
}

static void heli_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    draw_tree(&heli_object);
    njPushMatrix(NULL);
    TURN_VIA(njRotateY, HELI(work)->rotor, ang);
    if (ROTOR_SPEED(work) < ROTOR_BLUR) {
        draw_model(&heli_rotor_model);
    } else {
        draw_tree(&heli_rotor_object);
    }
    njPopMatrix(1);
    njTranslate(NULL, -LIGHT_X, -LIGHT_Y, 0.0F);
    TURN_VIA(njRotateY, HELI(work)->pan, ang);
    TURN_VIA(njRotateX, HELI(work)->tilt, ang);
    draw_tree(&heli_light_object);
    njPopMatrix(1);
}

/* within ANIMATE_NEAR of the camera, turn the light, bob, turn the rotor and
 * draw */
static void heli_animate(struct task *task, struct task_work *player)
{
    struct task_work *work = task->work;

    if (njDistanceP2P(&camera_work->pos, &work->pos) < ANIMATE_NEAR) {
        switch (work->submode) {
        case LIGHT_SWEEP_UP:
            HELI(work)->tilt += SWEEP_TILT;
            HELI(work)->pan += SWEEP_PAN;
            if (HELI(work)->tilt >= SWEEP_MOST) {
                work->submode = LIGHT_SWEEP_DOWN;
            }
            break;
        case LIGHT_SWEEP_DOWN:
            HELI(work)->tilt -= SWEEP_TILT;
            HELI(work)->pan -= SWEEP_PAN;
            if (HELI(work)->tilt <= -SWEEP_MOST) {
                work->submode = LIGHT_SWEEP_UP;
            }
            break;
        case LIGHT_FOLLOW:
            /* the aim runs from player 0 to the helicopter's center, plus
             * the mount's offset negated. the heading doesn't turn the
             * offset */
            heli_aim_x = work->pos.x - player->pos.x + LIGHT_X;
            heli_aim_z = work->pos.z - player->pos.z + -0.0F;
            heli_aim_length =
                njSqrt((heli_aim_x * heli_aim_x) + (heli_aim_z * heli_aim_z));
            if (heli_aim_length > AIM_LONGEST) {
                heli_aim_length = AIM_LONGEST;
            }
            heli_aim = (Angle)(heli_aim_length * ANGLE_PER_DEGREE);
            if (HELI(work)->tilt > heli_aim + FOLLOW_TILT ||
                HELI(work)->tilt < heli_aim - FOLLOW_TILT) {
                if (HELI(work)->tilt > heli_aim) {
                    HELI(work)->tilt -= FOLLOW_TILT;
                } else {
                    HELI(work)->tilt += FOLLOW_TILT;
                }
            }
            heli_aim =
                (HELI(work)->pan + work->ang.y -
                 (Angle)(atan2f(heli_aim_x, heli_aim_z) * ANGLE_PER_RADIAN)) &
                0xffff;
            /* @bug logic: the & 0xffff above leaves heli_aim from 0 to
             * 0xffff, so it is never below -FOLLOW_PAN (CMP/GE at
             * 0x0c902eca). a pan short of its aim by up to FOLLOW_PAN still
             * turns, and the pan settles from 0 to FOLLOW_PAN past its aim,
             * never short of it */
            if (heli_aim > FOLLOW_PAN || heli_aim < -FOLLOW_PAN) {
                if (heli_aim < 0x8000) {
                    HELI(work)->pan -= FOLLOW_PAN;
                } else {
                    HELI(work)->pan += FOLLOW_PAN;
                }
            }
            break;
        }
        if (work->flags & HELI_BOB) {
            if (work->flags & HELI_RISE) {
                BOB(work) += BOB_STEP;
                if (BOB(work) >= BOB_MOST) {
                    BOB(work) = BOB_MOST;
                    work->flags += HELI_RISE;
                }
            } else {
                BOB(work) -= BOB_STEP;
                if (BOB(work) <= -BOB_MOST) {
                    BOB(work) = -BOB_MOST;
                    work->flags -= HELI_RISE;
                }
            }
        }
        work->pos.y += BOB(work);
        HELI(work)->rotor += ROTOR_SPEED(work);
        heli_draw(task);
    }
}

/* sample heli_route at the helicopter's DISTANCE. heli_route_pos takes the
 * route's point there. heli_route_dir takes the route's direction a quarter
 * turn on. heli_route_ang takes the heading turned toward it by at most
 * ROUTE_TURN, and heli_route_turn the turn that makes. sample, passed by
 * value, needn't be set: this fills it before it reads it */
static void heli_sample_route(struct task_work *work, struct path *route,
                              struct path_sample sample)
{
    sample.distance = DISTANCE(work);
    path_sample(route, &sample);
    heli_route_dir =
        ((Angle)(atan2f(sample.ahead.x, sample.ahead.z) * ANGLE_PER_RADIAN) +
         NJM_DEG_ANG(90)) &
        0xffff;
    heli_route_turn = work->ang.y;
    heli_route_pos.x = sample.pos.x;
    heli_route_pos.y = sample.pos.y;
    heli_route_pos.z = sample.pos.z;
    heli_route_ang = angle_approach(work->ang.y, heli_route_dir, ROUTE_TURN);
    heli_route_turn = angle_difference(heli_route_ang, heli_route_turn);
}

/* hold player 0 HANG_BELOW the helicopter, on the side and facing its HANG_
 * flag gives. that flag's _OUT is player 0's distance from the center */
static void heli_hold(struct task_work *player, struct task_work *work)
{
    Angle ang = work->ang.y;

    player->pos.y = work->pos.y + BOB(work) - HANG_BELOW;
    if (work->flags & HANG_AGAINST_TURNED) {
        player->pos.x =
            work->pos.x + (njSin(ang - 0x8000) * HANG_AGAINST_TURNED_OUT);
        player->pos.z =
            work->pos.z + (njCos(ang - 0x8000) * HANG_AGAINST_TURNED_OUT);
        player->ang.y = 0x8000 - ang;
    } else if (work->flags & HANG_AGAINST) {
        player->pos.x = work->pos.x + (njSin(ang - 0x8000) * HANG_AGAINST_OUT);
        player->pos.z = work->pos.z + (njCos(ang - 0x8000) * HANG_AGAINST_OUT);
        player->ang.y = -ang;
    } else if (work->flags & HANG_ALONG_TURNED) {
        player->pos.x =
            work->pos.x - (njSin(ang + 0x8000) * HANG_ALONG_TURNED_OUT);
        player->pos.z =
            work->pos.z - (njCos(ang + 0x8000) * HANG_ALONG_TURNED_OUT);
        player->ang.y = 0x8000 - ang;
    } else {
        player->pos.x = work->pos.x - (njSin(ang + 0x8000) * HANG_ALONG_OUT);
        player->pos.z = work->pos.z - (njCos(ang + 0x8000) * HANG_ALONG_OUT);
        player->ang.y = -ang;
    }
}

/* a skid's step. work->mode 0 puts the skid along -z from the helicopter,
 * and 1 along +z, whatever the helicopter's heading. heli_carry_step runs it
 * only while the helicopter waits for player 0 to board */
static void heli_skid_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *heli = task->parent->work;
    NJS_POINT3 *pos = &work->pos;
    NJS_POINT3 *at = &heli->pos;

    pos->x = at->x;
    pos->y = at->y - SKID_BELOW;
    if (work->mode == 0) {
        pos->z = at->z - SKID_SIDE;
    } else {
        pos->z = at->z + SKID_SIDE;
    }
    work->ang.y = heli->ang.y;
    if (heli->flags & HELI_TARGET_SKIDS) {
        work->collision->flags |= COLLISION_TARGET;
    }
    collision_register(work);
    object_clear_touch(work, NULL);
}

static void heli_carry_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 *at = &player->pos;
    struct path_sample sample;
    NJS_POINT3 flat;
    NJS_POINT3 player_flat;
    Angle facing;

    switch (work->mode) {
    case HELI_WAIT:
        if (at->x > WAKE_X_MIN && at->x < WAKE_X_MAX && at->z > WAKE_Z_MIN &&
            at->z < WAKE_Z_MAX) {
            work->mode++;
        }
        break;
    case HELI_SPIN_UP:
        if (ROTOR_SPEED(work) < ROTOR_LIFT) {
            ROTOR_SPEED(work) += ROTOR_GAIN;
        } else if (DISTANCE(work) < BOARD_DISTANCE) {
            DISTANCE(work) += CREEP;
        } else {
            work->flags |= HELI_TARGET_SKIDS | HELI_BOB;
            task_step_children(task);
            work->flags &= ~HELI_TARGET_SKIDS;
            work->mode++;
        }
        break;
    case HELI_BOARD:
        if (DISTANCE(work) >= BOARD_DISTANCE) {
            flat.x = work->pos.x;
            flat.z = work->pos.z;
            player_flat.x = at->x;
            player_flat.z = at->z;
            flat.y = player_flat.y = 0.0F;
            if (work->pos.y - GRAB_TOP > at->y &&
                work->pos.y - GRAB_BOTTOM < at->y &&
                njDistanceP2P(&flat, &player_flat) < GRAB_REACH) {
                /* the flag keeps the half of the helicopter player 0 is
                 * under. facing is player 0's turn from the way an unturned
                 * flag holds it. past a quarter turn either way, player 0
                 * takes a _TURNED flag */
                facing = (work->ang.y + player->ang.y) & 0xffff;
                if ((((Angle)(atan2f(work->pos.z - at->z, work->pos.x - at->x) *
                              ANGLE_PER_RADIAN) +
                      work->ang.y) &
                     0xffff) < 0x8000) {
                    if (facing > 0x4000 && facing < 0xc000) {
                        work->flags += HANG_AGAINST_TURNED;
                    } else {
                        work->flags += HANG_AGAINST;
                    }
                } else {
                    if (facing > 0x4000 && facing < 0xc000) {
                        work->flags += HANG_ALONG_TURNED;
                    } else {
                        work->flags += HANG_ALONG;
                    }
                }
                player_set_mode(0, PLAYER_MODE_HANG_ON);
                work->flags &= ~HELI_BOB;
                work->mode++;
            } else {
                task_step_children(task);
            }
        }
        break;
    case HELI_CARRY:
        if (DISTANCE(work) > LET_GO_DISTANCE) {
            work->flags &= ~HANG_ANY;
            at->y = work->pos.y - LET_GO_BELOW;
            player_set_mode(0, PLAYER_MODE_RESET);
            SPEED(work) = RETURN_START;
            work->mode++;
        } else {
            if (DISTANCE(work) > BRAKE_DISTANCE) {
                SPEED(work) -= BRAKE;
                if (work->ang.z > 0) {
                    work->ang.z -= BRAKE_LEVEL;
                } else {
                    work->ang.z = 0;
                }
            } else if (SPEED(work) < CARRY_SPEED) {
                SPEED(work) += CARRY_GAIN;
                work->ang.z += CARRY_LEAN;
            }
            heli_hold(player, work);
        }
        break;
    case HELI_RETURN:
        if (DISTANCE(work) >= RETURN_END) {
            SPEED(work) = 0.0F;
            DISTANCE(work) = BOARD_DISTANCE;
            work->ang.z = 0;
            work->mode = HELI_SPIN_UP;
        } else if (SPEED(work) < RETURN_SPEED) {
            SPEED(work) += RETURN_GAIN;
            work->ang.z += RETURN_LEAN;
        }
        break;
    }
    DISTANCE(work) += SPEED(work);
    if (!(player->flags & PLAYER_RECOILING)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    heli_sample_route(work, &heli_route, sample); /* NOLINT(*-uninitialized) */
    heli_animate(task, player);
    work->pos = heli_route_pos;
    work->ang.y = heli_route_ang;
    if (work->mode != HELI_WAIT) {
        sound_play_at_for(SE_ROTOR, (int)work, 1, 64, work->pos.x, work->pos.y,
                          work->pos.z, 2);
    }
}

static void heli_hover_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    /* @hack type: stage_get_stage_act called as an int function, not as
     * stage.h's Uint16 one. the callee leaves stage << 8 | act in R0
     * unnarrowed (0x8c052ac8, 0x8c052acc), and the step compares the word
     * whole (CMP/EQ R0,R3 at 0x0c9036f8). as a Uint16, EXTU.W replaces the
     * MOV at 0x0c9036f4: without it, 1 of 51 words differ */
    /* @bug logic: the test reads PATH_VARS(work)->stage_act, as a path
     * task's step does, but this task keeps HELI's tilt there. the word it
     * reads is the tilt's low half (MOV.W @(12,R14),R0 at 0x0c9036f6).
     * task_destroy_children then finds nothing to destroy: a hovering
     * helicopter has no children */
    /* NOLINTNEXTLINE(clang-diagnostic-cast-function-type-*) */
    if (((int (*)(void))stage_get_stage_act)() != PATH_VARS(work)->stage_act) {
        task_destroy_children(task);
    }
    if (!(player->flags & PLAYER_RECOILING)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    heli_animate(task, player);
    sound_play_at_for(SE_ROTOR, (int)work, 1, 64, work->pos.x, work->pos.y,
                      work->pos.z, 2);
}

static void heli_hover_start(struct task *task)
{
    struct task_work *work = task->work;

    BOB(work) = 0.0F;
    HELI(work)->tilt = TILT_START;
    HELI(work)->pan = 0;
    ROTOR_SPEED(work) = ROTOR_HOVER;
    work->flags |= HELI_BOB;
    collision_init(task, &heli_rotor_shape, COLLISION_OBJECT, 1, 1);
    task->destructor = heli_destroy;
    task->step = heli_hover_step;
    task->draw = heli_draw;
}

void heli_a_step(struct task *task)
{
    struct task_work *work = task->work;

    heli_hover_start(task);
    work->submode = LIGHT_SWEEP_UP;
}

void heli_b_step(struct task *task)
{
    struct task_work *work = task->work;

    heli_hover_start(task);
    work->submode = LIGHT_FOLLOW;
}

void heli_route_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *skid;
    struct collision_shape *shape;

    if (get_character() != CHARACTER_TAILS) {
        work->mode = HELI_WAIT;
        BOB(work) = 0.0F;
        DISTANCE(work) = 0.0F;
        SPEED(work) = 0.0F;
        ROTOR_SPEED(work) = 0;
        HELI(work)->tilt = TILT_START;
        HELI(work)->pan = 0;
        work->submode = LIGHT_FOLLOW;
        work->flags &= ~(HANG_ANY | HELI_TARGET_SKIDS | HELI_BOB | HELI_RISE);
        collision_init(task, &heli_rotor_shape, COLLISION_OBJECT, ROTOR_ONE,
                       ROTOR_ONE);
        skid = task_create_child(TASK_WORK, heli_skid_step, task);
        skid->work->mode = 0;
        /* @hack spelling: the shape through a local set in the call's
         * arguments, which the image keeps on the stack for the second skid
         * (MOV.L R5,@(8,R15) at 0x0c90387c, MOV.L @(12,R15),R5 at
         * 0x0c903898). set before the call, 50 of 81 words differ, and
         * without the local, 78 */
        collision_init(skid, shape = &heli_skid_shape, COLLISION_OBJECT,
                       SKID_ONE, SKID_ONE);
        skid = task_create_child(TASK_WORK, heli_skid_step, task);
        skid->work->mode = 1;
        collision_init(skid, shape, COLLISION_OBJECT, 1, 1);
        task->destructor = heli_destroy;
        task->draw = heli_draw;
        task->step = heli_carry_step;
    }
}
