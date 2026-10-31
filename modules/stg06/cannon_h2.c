/* CANNON_H2, the big cannon hung upside down. it opens as a player comes
 * near while player 0 stands on a target. it then lobs shells at the
 * platform or cylinder player 0 stands on. for a cylinder, it fires at the
 * lowest below it, if any. a shell hits the target it was fired at, and
 * knocks back player 0 standing there. a rocket flown into the barrel
 * breaks the cannon, which takes the stage to its next act. a rocket near
 * the barrel, or too near the cannon, loses its rider */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/explosion.h"
#include "object/placement.h"
#include "object/rocket.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg06/cannon_h2.h"
#include "stg06/debris.h"
#include "stg06/knock_back.h"
#include "stg06/models.h"
#include "stg06/shell.h"
#include "stg06/sky_deck.h"
#include "stg06/target.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cannon_h_object;
extern NJS_MOTION cannon_h_motion;

/* it opens once a player comes within CANNON_NEAR, and fires only while
 * player 0 is that near. its barrel sits BARREL_HEIGHT down its turned
 * body, and it aims from that depth */
#define CANNON_NEAR 1000.0F
#define BARREL_HEIGHT 180.0F
/* the collision record's reach. collision_work's radius is a distance, so
 * 360 squared reaches far past the 330 ball */
#define CANNON_REACH (360.0F * 360.0F)
#define CANNON_VIEW 430.0F
/* the most the barrel turns a step toward its aim, in pitch and in yaw */
#define TURN_SPEED 145
/* the kick adds RECOIL_KICK to the barrel's speed, sending it back. a
 * spring takes RECOIL_IN of the barrel's offset from its speed a step. it
 * takes RECOIL_OUT instead while the barrel is behind home. the speed keeps
 * RECOIL_DAMP of itself */
#define RECOIL_KICK (-8.0F)
#define RECOIL_IN 0.05F
#define RECOIL_OUT 0.1F
#define RECOIL_DAMP 0.92F
/* on target, it kicks once its timer passes HOLD_STEPS. reloading, it
 * picks again once its timer passes RELOAD_STEPS */
#define HOLD_STEPS 15
#define RELOAD_STEPS 120
/* a rocket within ROCKET_TOO_NEAR of the cannon's position is too near to
 * fly in. one within ROCKET_IN_BARREL of the barrel, between its ends, is
 * in it. one within ROCKET_NEAR_BARREL there is near it */
#define ROCKET_TOO_NEAR 300.0F
#define ROCKET_IN_BARREL 50.0F
#define ROCKET_NEAR_BARREL 80.0F
/* the puff's scale as a rocket's rider lets go. the explosion's scale as
 * the cannon breaks, and the floor its pieces land on */
#define RELEASE_PUFF 2.0F
#define BREAK_BLAST 40.0F
#define BREAK_FLOOR (-10000.0F)
/* the cylinder it fires at lies within CYLINDER_BELOW of player 0's,
 * along x and along z */
#define CYLINDER_BELOW 25.0F

#define SE_CANNON_TURN 167
#define CANNON_VOLUME 96
#define TURN_VOLUME 64
#define TURN_SOUND_UPDATES 2

/* a shell flies SHELL_PACE across a step, and falls SHELL_GRAVITY a step
 * faster */
#define SHELL_PACE 10.0F
#define SHELL_GRAVITY 0.1F

/* the cannon's own layout of task_work */
struct cannon_h2_work {
    Sint8 mode;
    Sint8 unk_01;
    Uint8 unk_02;
    Uint8 unk_03;
    Sint16 flags;
    Uint16 timer;
    Uint16 aim[2]; /* the barrel's pitch and yaw */
    struct sky_deck_target *target; /* its own */
    struct sky_deck_target *aimed; /* the one it aims and fires at */
    struct angles ang;
    NJS_POINT3 pos;
    /* the barrel's offset along its length from home. it starts as the
     * placement's scl.x, the target's id. so the barrel starts out that far
     * from home, and springs back */
    Float recoil;
    Float recoil_speed; /* the barrel's speed along its length */
    Float unk_34;
    struct collision_work *collision;
    struct event_work *event;
};

enum cannon_h2_mode {
    CANNON_H2_WAIT = 0, /* closed, until a player comes near */
    CANNON_H2_OPEN = 1, /* its motion plays */
    CANNON_H2_PICK = 2, /* it picks what player 0 stands on, if anything */
    CANNON_H2_AIM = 4, /* the barrel turns to the target */
    CANNON_H2_HOLD = 5, /* on target, it waits to kick */
    CANNON_H2_FIRE = 6, /* it fires once the barrel stops moving back */
    CANNON_H2_RELOAD = 7,
    CANNON_H2_BROKEN = 8
};

/* with nothing to fire at, the barrel turns level toward player 0. it
 * does so while player 0 is within cannon_h2_watch_range of this spot */
static NJS_POINT3 cannon_h2_watch_spot = {755.0F, -339.0F, 5477.0F};
static Float cannon_h2_watch_range = 100.0F;
static struct debris_spec cannon_h2_debris = {&cannon_h_object,
                                              {0.0F, 0.0F, 0.0F},
                                              {0, 0, 0},
                                              {0.0F, 8.0F, 0.0F},
                                              {0.0F, 0.0F, 0.0F},
                                              800.0F,
                                              {1.0F, -1.0F, 1.0F},
                                              1.0F,
                                              300.0F,
                                              4.0F,
                                              0.98F,
                                              0,
                                              &obj_skydeck_texlist,
                                              0.0F};
static struct collision_shape cannon_h2_shape = {SHAPE_SPHERE,
                                                 0,
                                                 PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                                                 {0.0F, 0.0F, 0.0F},
                                                 330.0F,
                                                 0.0F,
                                                 0.0F,
                                                 {0, 0, 0},
                                                 0,
                                                 NULL};

static void cannon_h2_arrive(struct sky_deck_target *target, NJS_POINT3 *pos,
                             NJS_VECTOR *speed);

static struct shell_spec cannon_h2_shell = {
    SHELL_SPOT | SHELL_BURST | SHELL_HARMLESS, 0, -SHELL_GRAVITY, 6.0F,
    cannon_h2_arrive};

/* the muzzle, then the barrel's breech and mouth, the ends
 * cannon_h2_in_barrel tests between, in the barrel's own frame */
static const NJS_POINT3 cannon_h2_muzzle = {0.0F, 15.0F, 360.0F};
static const NJS_POINT3 cannon_h2_breech = {0.0F, 0.0F, 0.0F};
static const NJS_POINT3 cannon_h2_mouth = {0.0F, 15.0F, 360.0F};

static void cannon_h2_init(struct task *task);
static void cannon_h2_destroy(struct task *task);
static void cannon_h2_run(struct task *task);
static void cannon_h2_draw(struct task *task);

/* cannon_h2_shell's arrive. it hits the target, and knocks back player 0
 * standing on it */
static void cannon_h2_arrive(struct sky_deck_target *target, NJS_POINT3 *pos,
                             NJS_VECTOR *speed)
{
    if (target && target->work && target->hit) {
        target->hit(pos, speed, target->work);
        if (target->state & TARGET_STOOD_ON) {
            sky_deck_knock_back(target->work);
        }
    }
}

/* set point to the muzzle, with the barrel turned to pitch and yaw. the
 * cannon hangs flipped in y */
static void cannon_h2_find_muzzle(const Angle *pitch, const Angle *yaw,
                                  NJS_POINT3 *point,
                                  struct cannon_h2_work *work)
{
    NJS_POINT3 muzzle;
    Angle ang;

    muzzle = cannon_h2_muzzle;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    TURN_VIA(njRotateY, *yaw - work->ang.y, ang);
    TURN_VIA(njRotateX, *pitch, ang);
    njTranslate(NULL, 0.0F, BARREL_HEIGHT, 0.0F);
    njCalcPoint(NULL, &muzzle, point);
    njPopMatrix(1);
}

/* set speed to carry a shell from start to end at SHELL_PACE across a
 * step. set yaw to face end, and pitch to point at the top of the arc. a
 * shell that starts falling takes pitch along speed instead. returns 1 */
static int cannon_h2_aim(NJS_POINT3 *start, NJS_POINT3 *end, NJS_VECTOR *speed,
                         Angle *pitch, Angle *yaw)
{
    Float *from;
    Float *to;
    Float *out;
    Float dx;
    Float dy;
    Float dz;
    Float steps;
    Float x;
    Float y;
    Float z;
    Float height;
    Float distance;

    from = &start->x;
    to = &end->x;
    dx = *from++;
    dx = *to++ - dx;
    dy = *to - *from;
    /* @hack spelling: to and from stepped after dy, not inside it, which keeps
     * speed at sp+4, pitch at sp+0 and dy at sp+8 (0x0c912c8a, 0x0c912c94,
     * 0x0c912ca4); stepped inside it, 6 of 106 words differ */
    to++;
    from++;
    dz = *to - *from;
    /* @hack redundant: never-run stores of dz, which keep dx in FR14 and dz
     * in FR15 (0x0c912c96, 0x0c912ca8); without them, the two trade and 99
     * of 106 words differ */
    if (0) {
        *to = dz;
        *to = dz;
        *to = dz;
        *to = dz;
    }
    steps = njSqrt((dx * dx) + (dz * dz));
    steps *= 1.0F / SHELL_PACE;
    x = dx / steps;
    y = (dy + (steps * steps * (0.5F * SHELL_GRAVITY))) / steps;
    z = dz / steps;
    out = &speed->x;
    *out++ = x;
    *out++ = y;
    *out = z;
    if (y >= 0.0F) {
        steps = y * (1.0F / SHELL_GRAVITY);
        x *= steps;
        z *= steps;
        y *= steps;
        height = y - (steps * steps * (0.5F * SHELL_GRAVITY));
        distance = njSqrt((x * x) + (z * z));
    } else {
        height = y;
        distance = SHELL_PACE;
    }
    *pitch = (Angle)(atan2f(height, distance) * ANGLE_PER_RADIAN);
    *yaw = (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN);
    return 1;
}

/* 1 for a point within ROCKET_IN_BARREL of the barrel, between its ends.
 * -1 for one within ROCKET_NEAR_BARREL there, or within ROCKET_TOO_NEAR of
 * the cannon's position, tested first. else 0. speed goes unused */
static int cannon_h2_in_barrel(NJS_POINT3 *point, NJS_VECTOR *speed,
                               struct cannon_h2_work *work)
{
    NJS_POINT3 breech;
    NJS_POINT3 mouth;
    NJS_POINT3 from;
    NJS_POINT3 to;
    NJS_POINT3 nearest;
    NJS_LINE line;
    Float distance;
    Angle ang;

    breech = cannon_h2_breech;
    mouth = cannon_h2_mouth;
    if (njDistanceP2P(point, &work->pos) < ROCKET_TOO_NEAR) {
        return -1;
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    TURN_VIA(njRotateY, work->aim[1] - work->ang.y, ang);
    TURN_VIA(njRotateX, work->aim[0], ang);
    njTranslate(NULL, 0.0F, BARREL_HEIGHT, 0.0F);
    njCalcPoint(NULL, &breech, &from);
    njCalcPoint(NULL, &mouth, &to);
    njPopMatrix(1);
    line.px = from.x;
    line.py = from.y;
    line.pz = from.z;
    line.vx = to.x - from.x;
    line.vy = to.y - from.y;
    line.vz = to.z - from.z;
    distance = njDistanceP2L(point, &line, &nearest);
    if (njDistanceP2P(&nearest, &from) + njDistanceP2P(&nearest, &to) >
        njDistanceP2P(&from, &to)) {
        return 0;
    }
    if (distance < ROCKET_IN_BARREL) {
        return 1;
    }
    if (distance < ROCKET_NEAR_BARREL) {
        return -1;
    }
    return 0;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c912f3a); without it, task is kept at sp+0 and 9 of 14 words
 * and 1 pool word differ */
void cannon_h2_step(register struct task *task)
{
    if (!placement_try_leave(task)) {
        cannon_h2_init(task);
    }
}

/* the cannon starts closed, with a TARGET_CANNON_H2 target of its own. the
 * target's id is the placement's scl.x */
static void cannon_h2_init(struct task *task)
{
    struct cannon_h2_work *work = (struct cannon_h2_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, &cannon_h2_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = CANNON_REACH;
    }
    task->draw = cannon_h2_draw;
    task->step = cannon_h2_run;
    task->destructor = cannon_h2_destroy;
    work->timer = 0;
    work->mode = CANNON_H2_WAIT;
    work->ang.x = 0;
    work->ang.z = 0;
    work->target = sky_deck_target_add((struct task_work *)work);
    work->target->id = (int)work->recoil;
    work->target->group = TARGET_CANNON_H2;
    work->target->hit = NULL;
}

static void cannon_h2_destroy(struct task *task)
{
    struct cannon_h2_work *work = (struct cannon_h2_work *)task->work;

    sky_deck_target_remove(work->target);
}

/* after its mode's work, each step tests every flying rocket. one in the
 * barrel breaks the cannon. one near the barrel, or too near the cannon,
 * loses its rider */
static void cannon_h2_run(struct task *task)
{
    struct cannon_h2_work *work = (struct cannon_h2_work *)task->work;
    struct task_work *player;
    struct task_work *holder;
    struct sky_deck_target *entry;
    struct sky_deck_target *lower;
    struct rocket_work *rocket;
    int i;
    int nearest;

    if (placement_try_leave(task) || work->mode == CANNON_H2_BROKEN) {
        return;
    }
    cannon_h2_draw(task);
    if (placement_editing(task)) {
        return;
    }
    work->timer++;
    switch (work->mode) {
    case CANNON_H2_WAIT:
        nearest = player_nearest(&work->pos);
        player = player_work[nearest];
        if (njDistanceP2P(&player->pos, &work->pos) < CANNON_NEAR) {
            /* sky_deck_target_find_stood_on ignores its argument. the image
             * passes 4 (MOV #4,R4 at 0x0c91316e and 0x0c9131c8) */
            entry = sky_deck_target_find_stood_on(4);
            if (entry) {
                work->mode = CANNON_H2_OPEN;
                work->timer = 0;
            }
        }
        break;
    case CANNON_H2_OPEN:
        sound_play_at_for(SE_CANNON_TURN, (int)task, 1, CANNON_VOLUME,
                          work->pos.x, work->pos.y, work->pos.z,
                          TURN_SOUND_UPDATES);
        if (work->timer >= cannon_h_motion.nbFrame) {
            work->aim[0] = 0;
            work->aim[1] = work->ang.y;
            work->mode = CANNON_H2_PICK;
            work->timer = 0;
        }
        break;
    case CANNON_H2_PICK:
        /* the first target player 0 stands on, if a platform. if a
         * cylinder, the lowest cylinder below it whose state is 0, or that
         * one when there's none */
        entry = sky_deck_target_find_stood_on(4);
        if (entry) {
            if (entry->group & TARGET_PLATFORM) {
                work->aimed = entry;
                work->mode = CANNON_H2_AIM;
                work->timer = 0;
                break;
            }
            if (entry->group & TARGET_CYLINDER) {
                lower = sky_deck_target_find_lowest_below(
                    TARGET_CYLINDER, &entry->work->pos, 0, CYLINDER_BELOW);
                if (!lower) {
                    lower = entry;
                }
                work->aimed = lower;
                work->mode = CANNON_H2_AIM;
                work->timer = 0;
                break;
            }
        }
        if (njDistanceP2P(&cannon_h2_watch_spot, &player_work[0]->pos) <
            cannon_h2_watch_range) {
            NJS_POINT3 origin;
            NJS_VECTOR speed;
            Angle player_pitch;
            Angle player_yaw;
            Sint16 *to_pitch;
            Sint16 *to_yaw;
            Uint16 *at_pitch;
            Uint16 *at_yaw;
            Sint16 pitch_left;
            Sint16 yaw_left;

            /* @hack type: the turns read and rewrite player_pitch's low half
             * in place through a Sint16 pointer (0x0c913272 to 0x0c913318);
             * as union angle_halves, 73 of 725 words differ */
            to_pitch = (Sint16 *)&player_pitch;
            /* @hack type: player_yaw's the same way; as union angle_halves,
             * 39 of 725 words differ */
            to_yaw = (Sint16 *)&player_yaw;
            at_pitch = &work->aim[0];
            at_yaw = &work->aim[1];
            origin = work->pos;
            origin.y -= BARREL_HEIGHT;
            if (cannon_h2_aim(&origin, &player_work[0]->pos, &speed,
                              &player_pitch, &player_yaw)) {
                player_pitch = 0;
                pitch_left = *to_pitch - *at_pitch;
                if (pitch_left > TURN_SPEED) {
                    *to_pitch = *at_pitch + TURN_SPEED;
                } else if (pitch_left < -TURN_SPEED) {
                    *to_pitch = *at_pitch - TURN_SPEED;
                }
                yaw_left = *to_yaw - *at_yaw;
                if (yaw_left > TURN_SPEED) {
                    *to_yaw = *at_yaw + TURN_SPEED;
                } else if (yaw_left < -TURN_SPEED) {
                    *to_yaw = *at_yaw - TURN_SPEED;
                }
                *at_pitch = *to_pitch;
                *at_yaw = *to_yaw;
            }
        }
        break;
    case CANNON_H2_AIM: {
        NJS_POINT3 origin;
        NJS_POINT3 muzzle;
        NJS_VECTOR speed;
        Angle aim_pitch;
        Angle aim_yaw;
        Sint16 *to_pitch;
        Sint16 *to_yaw;
        Uint16 *at_pitch;
        Uint16 *at_yaw;
        Sint16 pitch_left;
        Sint16 yaw_left;

        player = player_work[0];
        /* @hack type: the turns read and rewrite aim_pitch's low half through
         * a Sint16 pointer (0x0c9133ac to 0x0c913410); as union angle_halves,
         * 139 of 725 words differ */
        to_pitch = (Sint16 *)&aim_pitch;
        /* @hack type: aim_yaw's the same way; as union angle_halves, 141 of
         * 725 words differ */
        to_yaw = (Sint16 *)&aim_yaw;
        at_pitch = &work->aim[0];
        at_yaw = &work->aim[1];
        sound_play_at_for(SE_CANNON_TURN, (int)task, 1, TURN_VOLUME,
                          work->pos.x, work->pos.y, work->pos.z,
                          TURN_SOUND_UPDATES);
        origin = work->pos;
        origin.y -= BARREL_HEIGHT;
        if (cannon_h2_aim(&origin, &work->aimed->work->pos, &speed, &aim_pitch,
                          &aim_yaw)) {
            cannon_h2_find_muzzle(&aim_pitch, &aim_yaw, &muzzle, work);
            pitch_left = *to_pitch - *at_pitch;
            if (pitch_left > TURN_SPEED) {
                *to_pitch = *at_pitch + TURN_SPEED;
            } else if (pitch_left < -TURN_SPEED) {
                *to_pitch = *at_pitch - TURN_SPEED;
            }
            yaw_left = *to_yaw - *at_yaw;
            if (yaw_left > TURN_SPEED) {
                *to_yaw = *at_yaw + TURN_SPEED;
            } else if (yaw_left < -TURN_SPEED) {
                *to_yaw = *at_yaw - TURN_SPEED;
            }
            *at_pitch = *to_pitch;
            *at_yaw = *to_yaw;
            /* on target, with player 0 within CANNON_NEAR, it holds to fire. it
             * reloads instead when the target's work has an ang.z other than 0.
             * a target placed tilted about z has one */
            if (pitch_left == 0 && yaw_left == 0 &&
                njDistanceP2P(&player->pos, &work->pos) < CANNON_NEAR) {
                holder = work->aimed->work;
                if (holder && holder->ang.z) {
                    work->mode = CANNON_H2_RELOAD;
                    work->timer = 0;
                    break;
                }
                work->mode = CANNON_H2_HOLD;
                work->timer = 0;
            }
        } else {
            work->mode = CANNON_H2_RELOAD;
            work->timer = 0;
            /* @hack redundant: a break that repeats the case's own, which makes
             * this arm's mode and timer stores the tail, entered at
             * 0x0c91345e and 0x0c913460, that the pick's two arms and the
             * reload above branch to (0x0c9131d8, 0x0c9131fe, 0x0c91344e);
             * without it, 126 of 725 words and 2 pool words differ */
            break;
        }
        break;
    }
    case CANNON_H2_HOLD:
        if (work->timer > HOLD_STEPS) {
            work->recoil_speed += RECOIL_KICK;
            work->mode = CANNON_H2_FIRE;
            work->timer = 0;
        }
        break;
    case CANNON_H2_FIRE:
        if (work->recoil_speed >= 0.0F) {
            NJS_POINT3 origin;
            NJS_POINT3 muzzle;
            NJS_VECTOR speed;
            Angle pitch;
            Angle yaw;

            origin = work->pos;
            origin.y -= BARREL_HEIGHT;
            if (cannon_h2_aim(&origin, &work->aimed->work->pos, &speed, &pitch,
                              &yaw)) {
                cannon_h2_find_muzzle(&pitch, &yaw, &muzzle, work);
                sound_play_at(SE_CANNON_FIRE, 0, 0, CANNON_VOLUME, work->pos.x,
                              work->pos.y, work->pos.z);
                cannon_h2_aim(&muzzle, &work->aimed->work->pos, &speed, &pitch,
                              &yaw);
                sky_deck_shell_fire_at(&muzzle, &work->aimed->work->pos, &speed,
                                       work->aimed, &cannon_h2_shell);
            }
            work->mode = CANNON_H2_RELOAD;
            work->timer = 0;
        }
        break;
    case CANNON_H2_RELOAD:
        if (work->timer > RELOAD_STEPS) {
            work->mode = CANNON_H2_PICK;
            work->timer = 0;
        }
        break;
    }
    work->recoil_speed += work->recoil > 0.0F ? -(work->recoil * RECOIL_IN)
                                              : -(work->recoil * RECOIL_OUT);
    work->recoil_speed *= RECOIL_DAMP;
    work->recoil += work->recoil_speed;
    collision_register((struct task_work *)work);
    if (work->mode != CANNON_H2_BROKEN) {
        for (i = 0; i < ROCKET_COUNT && work->mode != CANNON_H2_BROKEN; i++) {
            rocket = rockets[i];
            if (rocket && (rocket->state & ROCKET_FLYING)) {
                NJS_VECTOR still = {0.0F, 0.0F, 0.0F};

                switch (cannon_h2_in_barrel(&rocket->pos, &still, work)) {
                case 1: {
                    NJS_VECTOR speed = {0.0F, 0.0F, 0.0F};
                    NJS_VECTOR accel = {0.0F, -0.12F, 0.0F};
                    /* NOLINTBEGIN(clang-diagnostic-unused-but-set-variable) */
                    NJS_POINT3 blast;
                    /* NOLINTEND(clang-diagnostic-unused-but-set-variable) */

                    explosion_create((struct task_work *)work, BREAK_BLAST);
                    sky_deck_debris_create(&work->pos, &work->ang, &speed,
                                           &accel, BREAK_FLOOR,
                                           &cannon_h2_debris);
                    /* @hack redundant: blast, a copy of the position never
                     * read (0x0c91369a to 0x0c9136b2); without it, 127 of 725
                     * words differ */
                    blast = work->pos;
                    sound_play_at(SE_CANNON_BREAK, 0, 0, CANNON_VOLUME,
                                  work->pos.x, work->pos.y, work->pos.z);
                    work->mode = CANNON_H2_BROKEN;
                    work->timer = 0;
                    task_create(TASK_WORK, 0, sky_deck_voice_change_act_step);
                    break;
                }
                case -1:
                    rocket->state |= ROCKET_RELEASE;
                    bomb_create(&rocket->pos, RELEASE_PUFF);
                    break;
                }
            }
        }
    }
}

/* the cannon, flipped in y. closed or opening, it draws as its motion. open, it
 * draws node by node. the node that holds the barrel turns to the barrel's yaw.
 * the barrel takes its pitch and recoil */
static void cannon_h2_draw(struct task *task)
{
    struct cannon_h2_work *work = (struct cannon_h2_work *)task->work;
    NJS_OBJECT *object = &cannon_h_object;
    NJS_MOTION *motion = &cannon_h_motion;
    NJS_OBJECT *barrel;
    NJS_OBJECT *barrel_rest;
    NJS_OBJECT *rest;
    Angle ang;
    int frame;

    if (placement_try_leave(task) || work->mode == CANNON_H2_BROKEN) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, CANNON_VIEW)) {
        return;
    }
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    switch (work->mode) {
    case CANNON_H2_WAIT:
        draw_motion(object, motion, 0.0F);
        break;
    case CANNON_H2_OPEN:
        frame =
            work->timer >= motion->nbFrame ? motion->nbFrame - 1 : work->timer;
        draw_motion(object, motion, (Float)frame);
        break;
    default:
        draw_model(object->model);
        object = object->child;
        draw_model(object->model);
        object = object->sibling;
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)object->pos);
        draw_model(object->model);
        njPopMatrix(1);
        object = object->sibling;
        njPushMatrix(NULL);
        TURN_VIA(njRotateY, work->aim[1] - work->ang.y, ang);
        barrel = object->child;
        barrel_rest = barrel->sibling;
        draw_model(object->model);
        rest = object->sibling;
        TURN_VIA(njRotateX, work->aim[0], ang);
        njTranslate(NULL, 0.0F, 0.0F, work->recoil);
        draw_tree(barrel);
        draw_tree(barrel_rest);
        njPopMatrix(1);
        draw_tree(rest);
        break;
    }
    njPopMatrix(1);
}
