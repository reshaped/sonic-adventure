/* CANNON_H, the big cannon. it opens as a player comes near. it then lobs
 * shells three at a time, along a heading it picks at random. a rocket
 * flown into its barrel breaks it, which takes the stage to its next act. a
 * rocket too near the cannon loses its rider */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

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
#include "stg06/cannon_h.h"
#include "stg06/debris.h"
#include "stg06/models.h"
#include "stg06/shell.h"
#include "stg06/sky_deck.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT cannon_h_object;
extern NJS_MOTION cannon_h_motion;

/* it opens once a player comes within CANNON_NEAR. its barrel sits
 * BARREL_HEIGHT up its turned body, and it aims from that height */
#define CANNON_NEAR 1000.0F
#define BARREL_HEIGHT 180.0F
/* the collision record's reach. collision_work's radius is a distance, so
 * 360 squared reaches far past the 310 ball */
#define CANNON_REACH (360.0F * 360.0F)
#define CANNON_VIEW 430.0F
/* the shots fly along one of WAYS headings, WAY_APART degrees apart from
 * WAY_FIRST. the headings lie about the world's y, whatever the cannon's
 * own turn. each of the SHOTS after the first along a heading lands
 * NEXT_SHOT past the last */
#define WAYS 5
#define WAY_FIRST (-90.0F)
#define WAY_APART 45.0F
#define SHOTS 3
#define NEXT_SHOT 100.0F
/* the most the barrel turns a step toward its aim, in pitch and in yaw */
#define PITCH_SPEED 18
#define YAW_SPEED 72
/* on target, the kick adds RECOIL_KICK to the barrel's speed, sending it
 * back. a spring takes RECOIL_IN of the barrel's offset from its speed a
 * step. it takes RECOIL_OUT instead while the barrel is behind home. the
 * speed keeps RECOIL_DAMP of itself */
#define RECOIL_KICK (-8.0F)
#define RECOIL_IN 0.05F
#define RECOIL_OUT 0.1F
#define RECOIL_DAMP 0.92F
/* after a shot, it picks the next once its timer passes RELOAD_STEPS */
#define RELOAD_STEPS 60
/* a rocket within ROCKET_TOO_NEAR of the cannon's position is too near to
 * fly in. one within ROCKET_IN_BARREL of the barrel, between its ends, is
 * in it */
#define ROCKET_TOO_NEAR 300.0F
#define ROCKET_IN_BARREL 70.0F
/* the puff's scale as a rocket's rider lets go, and the explosion's as the
 * cannon breaks */
#define RELEASE_PUFF 2.0F
#define BREAK_BLAST 40.0F

#define SE_CANNON_TURN 167
#define CANNON_VOLUME 96
#define TURN_VOLUME 64
#define TURN_SOUND_UPDATES 2

/* a shell flies SHELL_PACE across a step, and falls SHELL_GRAVITY a step
 * faster */
#define SHELL_PACE 10.0F
#define SHELL_GRAVITY 0.1F

/* the cannon's own layout of task_work */
struct cannon_h_work {
    Sint8 mode;
    Sint8 shots; /* picked along way so far, back to 0 after SHOTS */
    Uint8 unk_02;
    Uint8 way; /* the heading the shots fly along, 0 to WAYS - 1 */
    Sint16 flags;
    Uint16 timer;
    Uint16 aim[2]; /* the barrel's pitch and yaw */
    Float recoil_speed;
    Float recoil; /* the barrel's offset along its length from home */
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 target; /* where the next shot lands */
    struct collision_work *collision;
    struct event_work *event;
};

enum cannon_h_mode {
    CANNON_WAIT, /* closed, until a player comes near */
    CANNON_OPEN, /* its motion plays */
    CANNON_PICK, /* it picks where the next shot lands */
    CANNON_AIM, /* the barrel turns to the shot */
    CANNON_FIRE, /* it fires once the barrel stops moving back */
    CANNON_RELOAD,
    CANNON_BROKEN
};

static struct shell_spec cannon_h_shell = {SHELL_SPOT | SHELL_SHADOW, 0,
                                           -SHELL_GRAVITY, 7.0F, NULL};
static struct debris_spec cannon_h_debris = {&cannon_h_object,
                                             {0.0F, 0.0F, 0.0F},
                                             {0, 0, 0},
                                             {0.0F, 4.0F, 0.0F},
                                             {0.0F, 0.0F, 0.0F},
                                             800.0F,
                                             {1.0F, 1.0F, 1.0F},
                                             1.0F,
                                             300.0F,
                                             4.0F,
                                             0.98F,
                                             0,
                                             &obj_skydeck_texlist,
                                             0.0F};
static struct collision_shape cannon_h_shape = {SHAPE_SPHERE,
                                                0,
                                                PUSH_ALL | PUSH_FIRM(PUSH_ALL),
                                                {0.0F, 0.0F, 0.0F},
                                                310.0F,
                                                0.0F,
                                                0.0F,
                                                {0, 0, 0},
                                                0,
                                                NULL};

/* the barrel's breech and mouth, the ends cannon_h_in_barrel tests
 * between, in the barrel's own frame */
static const NJS_POINT3 cannon_h_breech = {0.0F, 0.0F, 0.0F};
static const NJS_POINT3 cannon_h_mouth = {0.0F, 15.0F, 360.0F};
/* the muzzle, at the mouth, and the way the barrel points */
static const NJS_POINT3 cannon_h_muzzle = {0.0F, 15.0F, 360.0F};
static const NJS_POINT3 cannon_h_forward = {0.0F, 0.0F, 1.0F};
/* the speed passed to cannon_h_in_barrel, which ignores it. the speed and
 * acceleration of the pieces as the cannon breaks */
static const NJS_VECTOR cannon_h_still = {0.0F, 0.0F, 0.0F};
static const NJS_VECTOR cannon_h_debris_speed = {0.0F, 2.0F, 0.0F};
static const NJS_VECTOR cannon_h_debris_accel = {0.0F, -0.12F, 0.0F};

static void cannon_h_init(struct task *task);
static void cannon_h_run(struct task *task);
static void cannon_h_draw(struct task *task);
static void cannon_h_destroy(struct task *task);

/* 1 for a point within ROCKET_IN_BARREL of the barrel, between its ends.
 * -1 for one within ROCKET_TOO_NEAR of the cannon's position, tested first.
 * else 0. speed goes unused */
static int cannon_h_in_barrel(NJS_POINT3 *point, NJS_VECTOR *speed,
                              struct cannon_h_work *work)
{
    NJS_POINT3 breech;
    NJS_POINT3 mouth;
    NJS_POINT3 from;
    NJS_POINT3 to;
    NJS_POINT3 nearest;
    NJS_LINE line;
    Float distance;
    Angle ang;

    breech = cannon_h_breech;
    mouth = cannon_h_mouth;
    if (njDistanceP2P(point, &work->pos) < ROCKET_TOO_NEAR) {
        return -1;
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
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
    if (distance < ROCKET_IN_BARREL &&
        njDistanceP2P(&nearest, &from) + njDistanceP2P(&nearest, &to) <=
            njDistanceP2P(&from, &to)) {
        return 1;
    }
    return 0;
}

/* set point to the muzzle, with the barrel turned to pitch and yaw */
static void cannon_h_find_muzzle(const Angle *pitch, const Angle *yaw,
                                 NJS_POINT3 *point, struct cannon_h_work *work)
{
    NJS_POINT3 muzzle;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 forward;
    Angle ang;

    muzzle = cannon_h_muzzle;
    /* @hack redundant: forward, copied from cannon_h_forward and never read
     * (0x0c9120ba to 0x0c9120c8); without it, 72 of 81 words differ and
     * cannon_h_aim starts 32 bytes early */
    forward = cannon_h_forward;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateY, *yaw - work->ang.y, ang);
    TURN_VIA(njRotateX, *pitch, ang);
    njTranslate(NULL, 0.0F, BARREL_HEIGHT, 0.0F);
    njCalcPoint(NULL, &muzzle, point);
    njPopMatrix(1);
}

/* set speed to carry a shell from start to end at SHELL_PACE across a
 * step. set yaw to face end, and pitch to point at the top of the arc. a
 * shell that starts falling takes pitch along speed instead. returns 1 */
static int cannon_h_aim(NJS_POINT3 *start, NJS_POINT3 *end, NJS_VECTOR *speed,
                        Angle *pitch, Angle *yaw)
{
    /* @hack spelling: start and end read, and speed written, a Float at a time
     * through byte cursors over the whole points, as the image steps them;
     * read through the members, the function runs 2 bytes long and 98 of
     * 107 words differ, and written through them, it runs 2 bytes long and
     * 56 differ */
    char *from = (char *)start;
    char *to = (char *)end;
    char *out;
    Float dx;
    Float dy;
    Float dz;
    Float steps;
    Float x;
    Float y;
    Float z;
    Float height;
    Float distance;

    dx = *(Float *)to - *(Float *)from;
    from += sizeof(Float);
    to += sizeof(Float);
    dy = *(Float *)to - *(Float *)from;
    to += sizeof(Float);
    from += sizeof(Float);
    dz = *(Float *)to - *(Float *)from;
    /* @hack redundant: three never-run stores of dz, which keep dx in FR14
     * and dz in FR15 (0x0c9121b8, 0x0c9121ca). without them, 20 of 107 words
     * differ. with one, the function runs 4 bytes short and 100 differ; with
     * two, 26 differ */
    if (0) {
        *(Float *)to = dz;
        *(Float *)to = dz;
        *(Float *)to = dz;
    }
    steps = njSqrt((dx * dx) + (dz * dz));
    steps *= 1.0F / SHELL_PACE;
    x = dx / steps;
    y = (dy + (steps * steps * (0.5F * SHELL_GRAVITY))) / steps;
    z = dz / steps;
    out = (char *)speed;
    *(Float *)out = x;
    out += sizeof(Float);
    *(Float *)out = y;
    out += sizeof(Float);
    *(Float *)out = z;
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
    *pitch = -(int)(atan2f(height, distance) * ANGLE_PER_RADIAN);
    *yaw = (int)(atan2f(x, z) * ANGLE_PER_RADIAN);
    return 1;
}

/* @hack redundant: register on task, which keeps it in R14 across the range
 * check (0x0c91227e); without it, task is kept at sp+0, 10 of 14 words
 * differ and cannon_h_init starts 2 bytes late */
void cannon_h_step(register struct task *task)
{
    if (!placement_try_leave(task)) {
        cannon_h_init(task);
    }
}

static void cannon_h_init(struct task *task)
{
    struct cannon_h_work *work = (struct cannon_h_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, &cannon_h_shape, COLLISION_OBJECT, 1, 1);
        work->collision->radius = CANNON_REACH;
    }
    task->draw = cannon_h_draw;
    task->step = cannon_h_run;
    task->destructor = cannon_h_destroy;
    work->timer = 0;
    work->mode = CANNON_WAIT;
    work->ang.x = 0;
    work->ang.z = 0;
}

static void cannon_h_destroy(struct task *task)
{
}

/* after its mode's work, each step tests every flying rocket. one in the
 * barrel breaks the cannon, and one too near it loses its rider */
static void cannon_h_run(struct task *task)
{
    struct cannon_h_work *work = (struct cannon_h_work *)task->work;
    NJS_POINT3 point;
    NJS_POINT3 origin;
    NJS_POINT3 muzzle;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 above;
    Angle pitch;
    Angle yaw;
    Angle aim_pitch;
    Angle aim_yaw;
    Angle level;
    Angle ang;
    int way;
    int i;
    int nearest;
    struct task_work *player;
    struct rocket_work *rocket;

    if (placement_try_leave(task) || work->mode == CANNON_BROKEN) {
        return;
    }
    cannon_h_draw(task);
    if (placement_editing(task)) {
        return;
    }
    work->timer++;
    switch (work->mode) {
    case CANNON_WAIT:
        nearest = player_nearest(&work->pos);
        player = player_work[nearest];
        if (njDistanceP2P(&player->pos, &work->pos) < CANNON_NEAR) {
            work->mode = CANNON_OPEN;
            work->timer = 0;
        }
        break;
    case CANNON_OPEN:
        sound_play_at_for(SE_CANNON_TURN, (int)task, 1, CANNON_VOLUME,
                          work->pos.x, work->pos.y, work->pos.z,
                          TURN_SOUND_UPDATES);
        if (work->timer >= cannon_h_motion.nbFrame) {
            work->shots = 0;
            work->aim[0] = 0;
            work->aim[1] = work->ang.y;
            work->mode = CANNON_PICK;
            work->timer = 0;
        }
        break;
    case CANNON_PICK:
        /* a first shot waits for the barrel to level, then picks a heading
         * at random */
        if (work->shots == 0) {
            Sint16 left;
            Sint16 *at;
            Sint16 *to;
            level = 0;
            /* @hack type: level's low half read and rewritten through a
             * Sint16 pointer (0x0c91244e to 0x0c9124cc); as union
             * angle_halves, 21 of 692 words differ */
            to = (Sint16 *)&level;
            at = (Sint16 *)&work->aim[0];
            left = *to - *at;
            if (left > PITCH_SPEED) {
                *to = *at + PITCH_SPEED;
            } else if (left < -PITCH_SPEED) {
                *to = *at - PITCH_SPEED;
            }
            *at = *to;
            if (left == 0) {
                work->shots++;
                way = rand() % WAYS;
                /* the first shot lands 600 from the cannon along the
                 * heading */
                point.x = 0.0F;
                point.y = 0.0F;
                point.z = njSqrt(3600.0F) * 10.0F;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(
                    njRotateY,
                    (Angle)((WAY_FIRST + (way * WAY_APART)) * ANGLE_PER_DEGREE),
                    ang);
                njCalcPoint(NULL, &point, &work->target);
                njPopMatrix(1);
                work->way = way;
                work->mode = CANNON_AIM;
                work->timer = 0;
            }
        } else {
            work->shots++;
            point.x = 0.0F;
            point.y = 0.0F;
            point.z = NEXT_SHOT;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->target);
            TURN_VIA(njRotateY,
                     (Angle)((WAY_FIRST + (work->way * WAY_APART)) *
                             ANGLE_PER_DEGREE),
                     ang);
            njCalcPoint(NULL, &point, &work->target);
            njPopMatrix(1);
            work->mode = CANNON_AIM;
            work->timer = 0;
            if (work->shots >= SHOTS) {
                work->shots = 0;
            }
        }
        break;
    case CANNON_AIM: {
        Sint16 pitch_left;
        Sint16 yaw_left;
        /* @hack type: aim_pitch's low half read and rewritten through a
         * Sint16 pointer (0x0c912654 to 0x0c91270e); as union angle_halves,
         * 253 of 692 words differ */
        Sint16 *to_pitch = (Sint16 *)&aim_pitch;
        Sint16 *at_pitch = (Sint16 *)&work->aim[0];
        /* @hack type: aim_yaw's the same way (0x0c91267a to 0x0c912714); as
         * union angle_halves, 405 of 692 words differ */
        Sint16 *to_yaw = (Sint16 *)&aim_yaw;
        Sint16 *at_yaw = (Sint16 *)work->aim + 1;
        sound_play_at_for(SE_CANNON_TURN, (int)task, 1, TURN_VOLUME,
                          work->pos.x, work->pos.y, work->pos.z,
                          TURN_SOUND_UPDATES);
        point = work->pos;
        point.y += BARREL_HEIGHT;
        if (cannon_h_aim(&point, &work->target, &muzzle, &aim_pitch,
                         &aim_yaw)) {
            cannon_h_find_muzzle(&aim_pitch, &aim_yaw, &origin, work);
            pitch_left = *to_pitch - *at_pitch;
            if (pitch_left > PITCH_SPEED) {
                *to_pitch = *at_pitch + PITCH_SPEED;
            } else if (pitch_left < -PITCH_SPEED) {
                *to_pitch = *at_pitch - PITCH_SPEED;
            }
            yaw_left = *to_yaw - *at_yaw;
            if (yaw_left > YAW_SPEED) {
                *to_yaw = *at_yaw + YAW_SPEED;
            } else if (yaw_left < -YAW_SPEED) {
                *to_yaw = *at_yaw - YAW_SPEED;
            }
            *at_pitch = *to_pitch;
            *at_yaw = *to_yaw;
            if (pitch_left == 0 && yaw_left == 0) {
                work->recoil_speed += RECOIL_KICK;
                work->mode = CANNON_FIRE;
                work->timer = 0;
            }
        } else {
            work->mode = CANNON_RELOAD;
            work->timer = 0;
        }
        break;
    }
    case CANNON_FIRE:
        if (work->recoil_speed >= 0.0F) {
            origin = work->pos;
            origin.y += BARREL_HEIGHT;
            if (cannon_h_aim(&origin, &work->target, &point, &pitch, &yaw)) {
                cannon_h_find_muzzle(&pitch, &yaw, &muzzle, work);
                /* @hack redundant: a never-run call of sound_play_at, which SHC
                 * drops. it keeps the function's address in R11 and
                 * CANNON_RELOAD's 5 in R10 (0x0c91238a, 0x0c912388); without
                 * it, the two trade and 14 of 692 words differ */
                if (0) {
                    sound_play_at(SE_CANNON_FIRE, 0, 0, CANNON_VOLUME,
                                  work->pos.x, work->pos.y, work->pos.z);
                }
                sound_play_at(SE_CANNON_FIRE, 0, 0, CANNON_VOLUME, work->pos.x,
                              work->pos.y, work->pos.z);
                cannon_h_aim(&muzzle, &work->target, &point, &pitch, &yaw);
                sky_deck_shell_fire(&muzzle, &work->target, &point,
                                    &cannon_h_shell);
            }
            work->mode = CANNON_RELOAD;
            work->timer = 0;
        }
        break;
    case CANNON_RELOAD:
        if (work->timer > RELOAD_STEPS) {
            work->mode = CANNON_PICK;
            work->timer = 0;
        }
        break;
    }
    work->recoil_speed += work->recoil > 0.0F ? -(work->recoil * RECOIL_IN)
                                              : -(work->recoil * RECOIL_OUT);
    work->recoil_speed *= RECOIL_DAMP;
    work->recoil += work->recoil_speed;
    collision_register((struct task_work *)work);
    if (work->mode != CANNON_BROKEN) {
        for (i = 0; i < ROCKET_COUNT && work->mode != CANNON_BROKEN; i++) {
            rocket = rockets[i];
            if (rocket && (rocket->state & ROCKET_FLYING)) {
                point = cannon_h_still;
                switch (cannon_h_in_barrel(&rocket->pos, &point, work)) {
                case 1:
                    origin = cannon_h_debris_speed;
                    muzzle = cannon_h_debris_accel;
                    explosion_create((struct task_work *)work, BREAK_BLAST);
                    sky_deck_debris_create(&work->pos, &work->ang, &origin,
                                           &muzzle, work->pos.y,
                                           &cannon_h_debris);
                    sound_play_at(SE_CANNON_BREAK, 0, 0, CANNON_VOLUME,
                                  work->pos.x, work->pos.y, work->pos.z);
                    /* @hack redundant: above, set to a point 200 over the
                     * cannon and never read (0x0c912946 to 0x0c91295e);
                     * without it, 675 of 692 words differ */
                    above = work->pos;
                    above.y += 200.0F;
                    work->mode = CANNON_BROKEN;
                    work->timer = 0;
                    task_create(TASK_WORK, 0, sky_deck_change_act_step);
                    break;
                case -1:
                    rocket->state |= ROCKET_RELEASE;
                    bomb_create(&rocket->pos, RELEASE_PUFF);
                }
            }
        }
    }
}

/* closed or opening, the cannon draws as its motion. open, it draws node by
 * node. the node that holds the barrel turns to the barrel's yaw. the barrel
 * takes its pitch and recoil */
static void cannon_h_draw(struct task *task)
{
    struct cannon_h_work *work = (struct cannon_h_work *)task->work;
    NJS_OBJECT *object = &cannon_h_object;
    NJS_MOTION *motion = &cannon_h_motion;
    NJS_OBJECT *barrel;
    NJS_OBJECT *barrel_rest;
    NJS_OBJECT *rest;
    Angle ang;
    int frame;

    if (placement_try_leave(task) || work->mode == CANNON_BROKEN) {
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
    case CANNON_WAIT:
        draw_motion(object, motion, 0.0F);
        break;
    case CANNON_OPEN:
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
