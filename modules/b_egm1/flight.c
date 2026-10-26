/* the Egg Hornet's flight, in rounds. in each, the boss crosses the arena
 * in an arc to another third. it hovers and fires its volleys, then crosses
 * again. it then dives at player 0, sticks in the ground, and climbs back to
 * start the next round */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm1/barrier.h"
#include "b_egm1/camera.h"
#include "b_egm1/egg_hornet.h"
#include "b_egm1/flight.h"
#include "b_egm1/ground.h"
#include "b_egm1/jets.h"
#include "b_egm1/rumble.h"
#include "b_egm1/shapes.h"
#include "b_egm1/shots.h"
#include "b_egm1/util.h"
#include "collision/collision.h"
#include "draw/motion.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

#define SE_TURN 459
#define SE_SHOUT 460
#define SE_DIVE_LAND 461
#define VOICE_DIVE 173

#define ZONE_LOWER_X 752.0F
#define ZONE_UPPER_X 1008.0F

/* the steps between volleys */
#define VOLLEY_STEPS 140
/* the steps FLIGHT_CHOOSE waits after the last volley */
#define CHOOSE_STEPS 80
/* the steps the jets boost before a cross */
#define BOOST_STEPS 30
/* the steps the boss stays stuck in the ground */
#define STUCK_STEPS 180
/* the steps it stays stuck after a hit there */
#define HIT_STUCK_STEPS 15
/* the climb's speed along its chord, each step */
#define CLIMB_SPEED 2.5F

/* where a cross into each third ends. the round's first cross ends past
 * player 0 in z instead */
static const NJS_POINT3 upper_point = {1098.0F, 190.0F, 1258.0F};
static const NJS_POINT3 middle_point = {863.0F, 190.0F, 1312.0F};
static const NJS_POINT3 lower_point = {609.0F, 190.0F, 1228.0F};
/* the climb ends at climb_point's height. its x and z go unread */
static const NJS_POINT3 climb_point = {832.0F, 190.0F, 1172.0F};

/* nothing reads it */
static Angle dat_0c91e5a8 = 764;
static Float dive_speed = 4.0F; /* along the dive's chord, each step */
/* the most the boss sways either way while stuck in the ground */
static Angle stuck_sway = NJM_DEG_ANG(10);
static Angle arc_speed = NJM_DEG_ANG(1.5); /* a cross's turn, each step */
static Angle bank_angle = NJM_DEG_ANG(60); /* the bank at a cross's middle */

Angle egg_hornet_face_player(struct task_work *work)
{
    struct task_work *player = player_work[0];
    Angle ang;

    /* @hack spelling: the angle set in a local, then returned, which SHC
     * works out in R4 and copies to R0 (MOV R4,R0 at 0x0c902772); returned
     * directly, 7 of 30 words differ */
    ang = -(Angle)(atan2f(work->pos.z - player->pos.z,
                          work->pos.x - player->pos.x) *
                   ANGLE_PER_RADIAN) -
          0x4000;
    return ang;
}

Angle egg_hornet_face_player_unsigned(struct task_work *work)
{
    return (Uint16)egg_hornet_face_player(work);
}

Bool egg_hornet_diving(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;

    if ((work->mode == EGG_HORNET_FIGHT || work->mode == EGG_HORNET_BEATEN) &&
        record->state >= FLIGHT_DIVE) {
        return TRUE;
    }
    return FALSE;
}

/* blend from toward to by share, the shorter way round. the result isn't
 * wrapped, so it can lie past a turn */
static Angle angle_blend(Angle from, Angle to, Float share)
{
    from = (Uint16)from;
    to = (Uint16)to;
    if ((from - to > 0 ? from - to : to - from) >= 0x8000) {
        if (from < to) {
            from += 0x10000;
        } else {
            to += 0x10000;
        }
    }
    from = (Angle)((from * (1.0F - share)) + (to * share));
    return from;
}

/* angle_blend at three times share, held to 1 at most */
static Angle angle_blend_fast(Angle from, Angle to, Float share)
{
    from = (Uint16)from;
    to = (Uint16)to;
    share *= 3.0F;
    if (share > 1.0F) {
        share = 1.0F;
    }
    if ((from - to > 0 ? from - to : to - from) >= 0x8000) {
        if (from < to) {
            from += 0x10000;
        } else {
            to += 0x10000;
        }
    }
    from = (Angle)((from * (1.0F - share)) + (to * share));
    return from;
}

static int player_zone(void)
{
    struct task_work *player = player_work[0];

    if (player->pos.x < ZONE_LOWER_X) {
        return ZONE_LOWER;
    }
    if (player->pos.x > ZONE_UPPER_X) {
        return ZONE_UPPER;
    }
    return ZONE_MIDDLE;
}

static int egg_hornet_zone(struct task_work *work)
{
    if (work->pos.x < ZONE_LOWER_X) {
        return ZONE_LOWER;
    }
    if (work->pos.x > ZONE_UPPER_X) {
        return ZONE_UPPER;
    }
    return ZONE_MIDDLE;
}

/* start the path over, with a time step that covers speed units of its
 * chord's length each step. a path whose start is its end takes one step */
static void flight_set_speed(struct egg_hornet *record, Float speed)
{
    NJS_VECTOR chord;
    Float length;

    record->time = 0.0F;
    VECTOR_DIFF(&chord, &record->path->end, &record->path->start);
    if ((length = njScalor(&chord)) > 0.0F) {
        record->time_step = speed / length;
    } else {
        record->time_step = 1.0F;
    }
}

/* plan the cross from the boss's position to to_zone's point, an arc round
 * the middle of the way. a cross toward lower x leans with ACTION_LEAN_1,
 * and its arc turns by arc_speed. one toward higher x leans with
 * ACTION_LEAN_2, and its arc turns by -arc_speed */
static void flight_plan_cross(struct task *task, struct task_work *work,
                              struct egg_hornet *record)
{
    struct egg_hornet *boss = task->extra;
    struct task_work *player = player_work[0];
    NJS_VECTOR out;

    record->path->start.x = work->pos.x;
    record->path->start.y = work->pos.y;
    record->path->start.z = work->pos.z;
    switch (record->to_zone) {
    case ZONE_UPPER:
        VECTOR_COPY(&record->path->end, &upper_point);
        break;
    case ZONE_MIDDLE:
        VECTOR_COPY(&record->path->end, &middle_point);
        break;
    default:
        VECTOR_COPY(&record->path->end, &lower_point);
        break;
    }
    if (record->round_phase < 1) {
        record->path->end.z = player->pos.z + 250.0F;
    }
    out.x = record->path->control.x =
        (record->path->start.x + record->path->end.x) * 0.5F;
    out.y = record->path->control.y =
        (record->path->start.y + record->path->end.y) * 0.5F;
    out.z = record->path->control.z =
        (record->path->start.z + record->path->end.z) * 0.5F;
    VECTOR_DIFF(&out, &out, &record->path->start);
    record->radius = njScalor(&out);
    record->arc_start = record->arc =
        (Angle)(atan2f(out.z, out.x) * ANGLE_PER_RADIAN) + 0x8000;
    if (record->from_zone == ZONE_UPPER) {
        boss->motion.wanted = ACTION_LEAN_1;
        record->arc_step = arc_speed;
    } else if (record->from_zone == ZONE_LOWER) {
        boss->motion.wanted = ACTION_LEAN_2;
        record->arc_step = -arc_speed;
    } else if (record->to_zone == ZONE_UPPER) {
        boss->motion.wanted = ACTION_LEAN_2;
        record->arc_step = -arc_speed;
    } else {
        boss->motion.wanted = ACTION_LEAN_1;
        record->arc_step = arc_speed;
    }
}

/* place the boss on the cross's arc, at arc round the path's middle. the
 * arc's radius swells to three times halfway across. the boss's height goes
 * straight from the start's to the end's */
static void flight_cross(struct task_work *work, struct egg_hornet *record)
{
    Float swell = 1.0F + (2.0F * njSin((Angle)(record->time * 32768.0F)));

    work->pos.x = record->path->control.x +
                  ((njCos(record->arc) * record->radius) * swell);
    work->pos.z = record->path->control.z +
                  ((njSin(record->arc) * record->radius) * swell);
    work->pos.y = (record->path->start.y * (1.0F - record->time)) +
                  (record->path->end.y * record->time);
}

/* plan the dive from the boss's position to just above player 0, offset a
 * little in z. the curve bows up over the way */
static void flight_plan_dive(struct task_work *work, struct egg_hornet *record)
{
    struct task_work *player = player_work[0];

    record->path->start.x = work->pos.x;
    record->path->start.y = work->pos.y;
    record->path->start.z = work->pos.z;
    record->path->end.x = player->pos.x;
    record->path->end.y = player->pos.y + 10.0F;
    record->path->end.z =
        player->pos.z + (player->pos.z < 980.0F ? 20.0F : -20.0F);
    record->path->control.x =
        ((record->path->start.x + record->path->end.x) * 0.5F) + 9.0F;
    record->path->control.y =
        ((record->path->start.y + record->path->end.y) * 0.5F) + 200.0F;
    record->path->control.z =
        (record->path->start.z + record->path->end.z) * 0.5F;
    flight_set_speed(record, dive_speed);
}

/* plan the climb from the boss's position up to climb_point's height, a
 * little further along z. the curve rises most at the start */
static void flight_plan_climb(struct task_work *work, struct egg_hornet *record)
{
    record->path->start.x = work->pos.x;
    record->path->start.y = work->pos.y;
    record->path->start.z = work->pos.z;
    record->path->end.x = work->pos.x;
    record->path->end.y = work->pos.y;
    record->path->end.z = work->pos.z;
    record->path->end.y = climb_point.y;
    record->path->end.z += 60.0F;
    record->path->control.x =
        (record->path->start.x + record->path->end.x) * 0.5F;
    record->path->control.y =
        (record->path->start.y * 0.2F) + (record->path->end.y * 0.8F);
    record->path->control.z =
        (record->path->start.z + record->path->end.z) * 0.5F;
    flight_set_speed(record, CLIMB_SPEED);
}

static void flight_hover(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;

    record->bob += 0x222;
    record->bob &= 0xffff;
    work->pos.y = record->hover_y - (njSin(record->bob) * 10.0F);
    work->ang.y = egg_hornet_face_player(work);
}

static void flight_fire(struct task_work *work)
{
    struct volley volley;

    volley.owner = work;
    volley.target = &player_work[0]->pos;
    volley.interval = 14;
    volley.follow_interval = 22;
    volley.count = 6;
    egg_hornet_volley_create(&volley);
}

void egg_hornet_flight_reset(struct task *task, struct task_work *work)
{
    struct egg_hornet *record = task->extra;

    record->round_phase = -1;
    record->state = FLIGHT_START;
}

void egg_hornet_flight_step(struct task *task, struct task_work *work)
{
    struct egg_hornet *record = task->extra;
    NJS_POINT3 last;
    NJS_VECTOR moved;
    Float ease;

    switch (record->state) {
    case FLIGHT_START:
        record->bob = 0;
        record->hover_y = work->pos.y;
        record->pitch = 0;
        work->ang.x = 0;
        work->ang.y = egg_hornet_face_player_unsigned(work);
        record->volleys = 0;
        record->state =
            record->round_phase >= 1 ? FLIGHT_DIVE_WAIT : FLIGHT_FIRE;
        if (record->motion.current == ACTION_LEAN_1) {
            record->motion.wanted = ACTION_LEAN_1_END;
        } else if (record->motion.current == ACTION_LEAN_2) {
            record->motion.wanted = ACTION_LEAN_2_END;
        }
        break;
    case FLIGHT_FIRE:
        flight_hover(task);
        if (record->round_phase != -1) {
            flight_fire(work);
            ++record->volleys;
            record->count = VOLLEY_STEPS;
            record->state = FLIGHT_RELOAD;
        } else {
            record->round_phase = 0;
            record->count = 1;
            record->state = FLIGHT_CHOOSE;
        }
        break;
    case FLIGHT_RELOAD:
        flight_hover(task);
        if (record->volleys >= record->volley_limit) {
            ++record->round_phase;
            record->count = CHOOSE_STEPS;
            record->state = FLIGHT_CHOOSE;
        } else if (--record->count <= 0) {
            record->state = FLIGHT_FIRE;
        }
        break;
    case FLIGHT_CHOOSE:
        flight_hover(task);
        if (--record->count <= 0) {
            record->from_zone = egg_hornet_zone(work);
            /* from an outer third, cross to the middle. from the middle,
             * cross to the third player 0 is in. with player 0 in the middle
             * too, cross to either outer third at random */
            if (record->from_zone == ZONE_UPPER ||
                record->from_zone == ZONE_LOWER) {
                record->to_zone = ZONE_MIDDLE;
                if (record->from_zone == ZONE_LOWER) {
                    record->motion.wanted = ACTION_TURN_1;
                    record->bank_amount = -bank_angle;
                } else {
                    record->motion.wanted = ACTION_TURN_2;
                    record->bank_amount = bank_angle;
                }
            } else if (player_zone() == ZONE_UPPER) {
                record->to_zone = ZONE_UPPER;
                record->motion.wanted = ACTION_TURN_2;
                record->bank_amount = -bank_angle;
            } else if (player_zone() == ZONE_LOWER) {
                record->to_zone = ZONE_LOWER;
                record->motion.wanted = ACTION_TURN_1;
                record->bank_amount = bank_angle;
            } else if (rand() * RAND_SCALE < 0.5F) {
                record->to_zone = ZONE_LOWER;
                record->motion.wanted = ACTION_TURN_1;
                record->bank_amount = bank_angle;
            } else {
                record->to_zone = ZONE_UPPER;
                record->motion.wanted = ACTION_TURN_2;
                record->bank_amount = -bank_angle;
            }
            record->state = FLIGHT_TURN;
        }
        break;
    case FLIGHT_TURN:
        if (record->motion.current == ACTION_IDLE) {
            egg_hornet_camera_turn();
            sound_play(SE_TURN, 0, 0, 0);
            egg_hornet_jets_set_mode(JETS_BOOST);
            work->ang.y += 0x8000;
            record->state = FLIGHT_BOOST;
        }
        break;
    case FLIGHT_BOOST:
        if (egg_hornet_jets_done_steps() >= BOOST_STEPS) {
            egg_hornet_jets_set_mode(JETS_ON);
            flight_plan_cross(task, work, record);
            record->heading = work->ang.y;
            record->state = FLIGHT_CROSS;
        }
        break;
    case FLIGHT_CROSS:
        /* the cross ends once its arc has turned half a turn */
        record->time = _builtin_fabsf((record->arc - record->arc_start) *
                                      (1.0F / 32768.0F));
        VECTOR_COPY(&last, &work->pos);
        /* @hack redundant: arc_step times 1.0F, the FR15 that FLDI1 sets at
         * 0x0c902e84 (FMUL FR15,FR3 at 0x0c9031ce); without it, 98 of 853
         * words differ */
        record->arc += (Angle)(record->arc_step * 1.0F);
        flight_cross(work, record);
        VECTOR_DIFF(&moved, &work->pos, &last);
        record->speed = njScalor(&moved);
        ease = record->time;
        work->ang.y =
            angle_blend_fast(record->heading, vector_yaw_unsigned(&moved),
                             (ease * 2.0F) - (ease * ease));
        record->bank = (Angle)(njSin((Angle)(record->time * 32768.0F)) *
                               record->bank_amount);
        if (record->time >= 1.0F) {
            record->bank_amount = 0;
            record->bank = 0;
            record->time = 0.0F;
            record->heading = work->ang.y;
            record->state = FLIGHT_BRAKE;
        }
        break;
    case FLIGHT_BRAKE:
        record->time += 0.05F;
        work->ang.y = angle_blend(record->heading, egg_hornet_face_player(work),
                                  record->time);
        work->pos.x += record->speed * njSin(work->ang.y);
        work->pos.z += record->speed * njCos(work->ang.y);
        record->speed *= 0.92F;
        if (record->time >= 1.0F) {
            record->sway = 1;
            record->state = FLIGHT_START;
        }
        break;
    case FLIGHT_DIVE_WAIT:
        flight_hover(task);
        if (!record->sway) {
            record->motion.wanted = ACTION_DIVE;
            record->state = FLIGHT_DIVE_POSE;
        }
        break;
    case FLIGHT_DIVE_POSE:
        flight_hover(task);
        if (motion_flags & MOTION_HOLDING) {
            record->state = FLIGHT_DIVE_SHOUT;
        }
        break;
    case FLIGHT_DIVE_SHOUT:
        egg_hornet_voice(VOICE_DIVE);
        sound_play(SE_SHOUT, 0, 0, 0);
        egg_hornet_barrier_request(&record->barrier, BARRIER_SHOW);
        record->state = FLIGHT_DIVE_AIM;
        /* fall through: the aim starts this step */
    case FLIGHT_DIVE_AIM:
        flight_hover(task);
        if (player_get(0)->last_flags & PLAYER_ON_GROUND) {
            egg_hornet_camera_wide();
            flight_plan_dive(work, record);
            record->pitch = work->ang.x;
            record->state = FLIGHT_DIVE;
        }
        break;
    case FLIGHT_DIVE:
        if (record->time > 0.4F) {
            egg_hornet_barrier_request(&record->barrier, BARRIER_HIDE);
            record->state = FLIGHT_DIVING;
        }
        /* fall through: FLIGHT_DIVE moves the boss as FLIGHT_DIVING does */
    case FLIGHT_DIVING:
        record->time += record->time_step;
        if (record->time >= 1.0F) {
            sound_play(SE_DIVE_LAND, 0, 0, 0);
            work->pos.x = record->path->end.x;
            work->pos.y = record->path->end.y;
            work->pos.z = record->path->end.z;
            record->state = FLIGHT_LAND;
        } else {
            VECTOR_COPY(&last, &work->pos);
            curve_point(&work->pos, record->path, record->time);
            VECTOR_DIFF(&moved, &work->pos, &last);
            work->ang.x =
                angle_blend(record->pitch, vector_pitch(&moved), record->time);
        }
        break;
    case FLIGHT_LAND:
        rumble_strong();
        egg_hornet_camera_near();
        dust_ring_create(&work->pos);
        clods_create(&work->pos);
        egg_hornet_shapes_enable(work, SHAPES_LANDED);
        record->hover_y = work->pos.y;
        record->bob = 0;
        record->count = 0;
        record->heading = work->ang.y;
        record->state = FLIGHT_STUCK;
        landing_puffs(&work->pos);
        break;
    case FLIGHT_STUCK:
        if (++record->count >= STUCK_STEPS) {
            egg_hornet_camera_turn();
            record->state = FLIGHT_CLIMB_START;
        } else {
            record->bob += 0x1111;
            record->bob &= 0xffff;
            work->pos.y =
                _builtin_fabsf(njSin(record->bob) * 0.8F) + record->hover_y;
            work->ang.y =
                (Angle)((njCos(record->bob) * stuck_sway) + record->heading);
        }
        if (work->flags & COLLISION_DAMAGED) {
            record->count = STUCK_STEPS - HIT_STUCK_STEPS;
        }
        break;
    case FLIGHT_CLIMB_START:
        dust_ring_set_mode(GROUND_OFF);
        clods_set_mode(GROUND_OFF);
        record->motion.wanted = ACTION_CLIMB;
        flight_plan_climb(work, record);
        record->heading = work->ang.y;
        record->return_pitch = work->ang.x;
        record->state = FLIGHT_CLIMB;
        break;
    case FLIGHT_CLIMB:
        record->time += record->time_step;
        if (record->time >= 1.0F) {
            egg_hornet_shapes_enable(work, SHAPES_FLYING);
            curve_point(&work->pos, record->path, record->time);
            work->ang.y = egg_hornet_face_player(work);
            work->ang.x = record->pitch;
            record->round_phase = -1;
            record->state = FLIGHT_START;
        } else {
            VECTOR_COPY(&last, &work->pos);
            curve_point(&work->pos, record->path, record->time);
            VECTOR_DIFF(&moved, &work->pos, &last);
            ease = record->time * record->time;
            work->ang.y = angle_blend(
                record->heading, egg_hornet_face_player_unsigned(work), ease);
            work->ang.x =
                angle_blend(record->return_pitch, record->pitch, ease);
        }
        break;
    }
}
