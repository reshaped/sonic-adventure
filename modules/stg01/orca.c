/* the chasing orca, which leaps out of the sea after player 0 in act 0 */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg01/orca.h"
#include "stg01/orca_control.h"
#include "stg01/sea.h"
#include "stg01/water_column.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT orca_object;
extern NJS_MOTION orca_leap_motion;
extern NJS_MOTION orca_leap_shape;
extern NJS_MKEY_A orca_leap_root_keys[];
extern NJS_MOTION orca_swim_motion;
extern NJS_MOTION orca_swim_shape;

struct orca_vars {
    /* the leap motion's frame, or the swim's once the orca leaves */
    Float frame;
    Float frame_speed; /* the leap motion's frames each step */
    Float height; /* the orca's height above its position, negative below */
};

/* the orca's state, laid over its task's motion record */
struct orca_motion {
    Float speed; /* the orca's speed along its heading, in units a step */
    /* the orca's move each step while it leaves. move.y is also its rise
     * each step while it rises */
    NJS_VECTOR move;
    NJS_VECTOR ahead; /* the orca's heading as a unit vector */
    Uint8 unk_1c[4];
    /* the heading orca_summon gives. ang.y, the orca's facing, sways about
     * it */
    Angle heading;
    Uint8 unk_24[8];
    /* ang.y's turn each step. ORCA_LEAP_DOWN subtracts it, and ORCA_UNDER
     * adds it. the end of each leap down negates it. ang.y thus turns one way
     * through a leap down and its dip, then back through the next */
    Angle turn;
    /* ang.z's turn each step of a leap up and a leap down. the end of each
     * leap down negates it too */
    Angle roll;
    /* a leap and the dip follow the arc time * time * curve + vertex in
     * height. vertex is the height where the arc turns */
    Float vertex;
    /* the arc's time, which advances with the frame, by frame_speed each
     * step. it is 0 where the arc turns: at LEAP_TOP for a leap, and at
     * LEAP_START for the dip */
    Float time;
    Float curve; /* below 0 for a leap, and above 0 for the dip */
};

#define VARS(work) ((struct orca_vars *)&(work)->vars)
#define MOTION(task) ((struct orca_motion *)(task)->motion)

/* @hack type: ang and step summed as Uint32, which keeps SHC's choices of
 * R4 and R5, and of FR4 and FR5, through orca_step, as at 0x0c9097f4 and
 * 0x0c909c6c. as += and -=, the pairs trade, and 35 of 765 words differ */
#define ANGLE_ADD(ang, step) ((ang) = (Angle)((Uint32)(ang) + (Uint32)(step)))
#define ANGLE_SUB(ang, step) ((ang) = (Angle)((Uint32)(ang) - (Uint32)(step)))

/* the orca rises from below the sea, then cycles. it leaps out of the sea
 * and back in, then dips under it until the next leap. sent away, it leaves
 * at the end of its next leap down, swimming off and down */
enum orca_mode {
    ORCA_RISE = 1,
    ORCA_LEAVE,
    ORCA_UNDER,
    ORCA_LEAP_UP,
    ORCA_LEAP_DOWN
};

/* the leap motion's frames at which ORCA_UNDER, ORCA_LEAP_UP and
 * ORCA_LEAP_DOWN end. the leap's frame wraps at FRAMES, as the swim's does */
#define LEAP_START 8.0F
#define LEAP_TOP 18.0F
#define FRAMES 30.0F

/* each leap rises LEAP_RISE, the first from height 0. the others rise from
 * -DIP_DEPTH, the bottom of the dip. a leaving orca's task ends at height
 * -LEAVE_DEPTH */
#define LEAP_RISE 100.0F
#define DIP_DEPTH 40.0F
#define LEAVE_DEPTH 350.0F

/* the orca ripples the sea about orca_head once TASK_TIMER(work) passes
 * RIPPLE_WAIT, but not while it leaves. the timer clears at each ripple, at
 * a leap's top and at the end of a leap down */
#define RIPPLE_WAIT 18

/* while the orca rises, orca_head lies PARKED_DEPTH below the origin, and the
 * head's shape with it */
#define PARKED_DEPTH 100000.0F

/* along the orca's turned and pitched axes, orca_head lies HEAD_AHEAD ahead
 * of the orca and HEAD_DROP below it */
#define HEAD_AHEAD 120.0F
#define HEAD_DROP 5.0F

/* the orca's sway across its heading each step is SWAY times the sine of
 * ang.y's turn from it */
#define SWAY 1.8F

/* the orca's task ends farther than 1000 units from the camera. RANGE is
 * that distance squared */
#define RANGE (1000.0F * 1000.0F)

/* sound of the orca falling back into the sea */
#define SE_ORCA_SPLASH 47

/* speeds of the water columns the orca throws up as it leaves the sea and
 * as it falls back in */
static const NJS_VECTOR orca_rise_splash_speed = {1.0F, 1.0F, 1.0F};
static const NJS_VECTOR orca_fall_splash_speed = {1.0F, 1.0F, 1.0F};

static void orca_step(struct task *task);
static void orca_draw(struct task *task);
static void orca_destroy(struct task *task);

/* the head, which hurts players, and the body, which is off until the orca
 * has risen. the head's center lies in the world, and each step sets it to
 * orca_head. the body's center stays at the task's position, whatever the
 * orca's height */
static struct collision_shape orca_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
     ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     40.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     60.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* start the orca below the sea, rising. ang.y starts as far past the
 * heading as the first leap down turns it back. ang.z starts as far below 0
 * as the first leap up turns it back. the position starts as far back
 * across the heading as the rise sways it */
void orca_init(struct task *task)
{
    struct task_work *work = task->work;
    struct orca_motion *motion = MOTION(task);
    Float sway;
    Float x;
    Float z;
    Float turn;

    work->mode = ORCA_RISE;
    VARS(work)->height = 0.0F;
    VARS(work)->frame = LEAP_START;
    VARS(work)->frame_speed = 0.3F;
    motion->time = -10.0F;
    motion->curve = -1.0F;
    motion->vertex = LEAP_RISE;
    motion->ahead.x = njSin(work->ang.y);
    motion->ahead.y = 0.0F;
    motion->ahead.z = njCos(work->ang.y);
    motion->heading = work->ang.y;
    motion->turn = 80;
    motion->roll = 72;
    /* @hack spelling: the local turn, which has SHC copy each converted angle
     * to FR4 before the multiply, by the FMOVs at 0x0c909712 and
     * 0x0c909728. as motion->turn * 40.0F and the like, the code runs 4
     * bytes short, and 100 of 150 words differ */
    turn = (Float)motion->turn;
    work->ang.y += (Angle)(turn * 40.0F);
    turn = (Float)motion->roll;
    work->ang.z = (Angle)(-(turn * (100.0F / 3.0F)));
    motion->move.x = 0.0F;
    motion->move.z = 0.0F;
    motion->move.y = 10.0F;
    VARS(work)->height = -100.0F;
    motion->speed = 0.0F;
    sway = njSin(work->ang.y - motion->heading) * SWAY;
    x = motion->ahead.z * sway;
    z = -(motion->ahead.x * sway);
    work->pos.x -= x * 10.0F;
    work->pos.z -= z * 10.0F;
    work->timer = RIPPLE_WAIT;
    collision_init(task, orca_shapes, COLLISION_ITEM, 1, 2);
    work->collision->radius = 150.0F;
    work->collision->shapes[1].flags |= SHAPE_OFF;
    task->step = orca_step;
    task->draw = orca_draw;
    task->destructor = orca_destroy;
}

/* run the orca's cycle, then chase player 0 along the heading, faster the
 * farther away player 0 is. each step sways the orca across the heading,
 * by more the farther ang.y has turned from it. a touch by Sonic throws
 * player 0 along the heading */
static void orca_step(struct task *task)
{
    struct task_work *work = task->work;
    struct orca_motion *motion;
    Float dx;
    Float dy;
    Float dz;
    Float time;
    Float frame;
    Float whole;
    Float part;
    Float key;
    Float next_key;
    Float sway;
    Float x;
    Float z;
    Float distance;
    Float ripple_x;
    Float ripple_z;
    Angle ang;
    Angle pitch;
    int index;
    int next;
    NJS_POINT3 *center;

    /* @hack spelling: the cursor at, which reads both positions by FMOV.S @R4+
     * after ADD #32,R4, from 0x0c9097f2 to 0x0c909806. as
     * camera_work->pos.x - work->pos.x and the like, 30 of 765 words
     * differ */
    {
        Float *at = VECTOR_FLOATS(&camera_work->pos);

        dx = *at++;
        dy = *at++;
        dz = *at;
        at = VECTOR_FLOATS(&work->pos);
        dx -= *at++;
        dy -= *at++;
        dz -= *at;
    }
    distance = (dx * dx) + (dy * dy) + (dz * dz);
    motion = MOTION(task);
    if (distance > RANGE) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case ORCA_RISE:
        VARS(work)->height += motion->move.y;
        if (VARS(work)->height >= 0.0F) {
            work->collision->shapes[1].flags &= ~SHAPE_OFF;
            VARS(work)->height = 0.0F;
            work->mode = ORCA_LEAP_UP;
        }
        break;
    case ORCA_UNDER:
        time = motion->time;
        VARS(work)->frame += VARS(work)->frame_speed;
        time += VARS(work)->frame_speed;
        ANGLE_ADD(work->ang.y, motion->turn);
        VARS(work)->height = (time * time * motion->curve) + motion->vertex;
        if (VARS(work)->frame >= LEAP_START) {
            NJS_POINT3 splash;
            NJS_VECTOR speed;

            work->mode = ORCA_LEAP_UP;
            time = -10.0F;
            motion->curve = -1.0F;
            motion->vertex = VARS(work)->height + LEAP_RISE;
            speed = orca_rise_splash_speed;
            splash.x = orca_head.x + 0.0F;
            splash.y = SEA_LEVEL;
            splash.z = orca_head.z + 40.0F;
            beach_water_column_create(&splash, &speed, 1.0F);
            vibration_start_fading(0, 7, 59, 6);
        }
        motion->time = time;
        break;
    case ORCA_LEAP_UP:
        time = motion->time;
        VARS(work)->frame += VARS(work)->frame_speed;
        time += VARS(work)->frame_speed;
        ANGLE_ADD(work->ang.z, motion->roll);
        VARS(work)->height = (time * time * motion->curve) + motion->vertex;
        if (VARS(work)->frame >= LEAP_TOP) {
            VARS(work)->frame_speed = 0.3F;
            time = 0.0F;
            work->mode = ORCA_LEAP_DOWN;
            /* the leap down's arc falls from here to height 0 at FRAMES */
            motion->curve = -(VARS(work)->height * (1.0F / 144.0F));
            motion->vertex = VARS(work)->height;
            work->timer = 0;
        }
        motion->time = time;
        break;
    case ORCA_LEAP_DOWN:
        time = motion->time;
        VARS(work)->frame += VARS(work)->frame_speed;
        time += VARS(work)->frame_speed;
        ANGLE_SUB(work->ang.y, motion->turn);
        ANGLE_ADD(work->ang.z, motion->roll);
        VARS(work)->height = (time * time * motion->curve) + motion->vertex;
        if (VARS(work)->frame >= FRAMES) {
            NJS_POINT3 splash;
            NJS_VECTOR speed;

            VARS(work)->frame -= FRAMES;
            VARS(work)->frame_speed = 0.2F;
            VARS(work)->height = 0.0F;
            work->mode = ORCA_UNDER;
            time = -8.0F;
            motion->curve = 0.625F;
            motion->vertex = -DIP_DEPTH;
            motion->turn *= -1;
            motion->roll *= -1;
            work->timer = 0;
            work->submode++;
            speed = orca_fall_splash_speed;
            splash.x = orca_head.x + 0.0F;
            splash.y = SEA_LEVEL;
            splash.z = orca_head.z + 0.0F;
            beach_water_column_create(&splash, &speed, 1.0F);
            sound_play(SE_ORCA_SPLASH, 0, 0, 0);
            if (orca_leaving == TRUE) {
                Float sin;
                Float cos;

                work->mode = ORCA_LEAVE;
                ang = work->ang.y;
                sin = njSin(ang);
                cos = njCos(ang);
                motion->move.x = sin * 3.0F;
                motion->move.z = cos * 3.0F;
                motion->move.y = -2.5F;
                /* @hack redundant: a store of the frame to cos that never
                 * runs. it keeps cos in FR4 and 3.0F in FR5 at 0x0c909afa.
                 * it keeps R4 and R5 as the image has them at 0x0c909c6c,
                 * and FR4 and FR5 at 0x0c909de6. without it, the pairs
                 * trade, and 28 of 765 words differ */
                if (0) {
                    cos = VARS(work)->frame;
                }
            }
        }
        motion->time = time;
        break;
    case ORCA_LEAVE:
        work->pos.x += motion->move.x;
        work->pos.z += motion->move.z;
        VARS(work)->height += motion->move.y;
        VARS(work)->frame += 0.4F;
        if (VARS(work)->frame > FRAMES) {
            VARS(work)->frame -= FRAMES;
        }
        if (VARS(work)->height <= -LEAVE_DEPTH) {
            task_kill(task);
        }
        break;
    }
    {
        /* @hack redundant: the array unused, which makes orca_step's stack
         * frame the 48 bytes that ADD #-48,R15 reserves at 0x0c9097e4.
         * without it, the stack frame takes 44, and 21 of 765 words
         * differ */
        Float unused[4];
        NJS_POINT3 point;
        NJS_MKEY_A *keys = orca_leap_root_keys;

        /* set orca_head to the head in the world. it turns to the orca's
         * facing, and pitches as the leap motion pitches its root node. the
         * pitch blends the root's keys at the whole frames either side of
         * the orca's frame */
        frame = VARS(work)->frame;
        whole = njRoundOff(frame);
        part = frame - whole;
        index = (int)whole;
        if (index == (int)FRAMES - 1) {
            next = 0;
        } else {
            next = index + 1;
        }
        key = (Float)keys[index].key[0];
        next_key = (Float)keys[next].key[0];
        pitch = (Angle)(((1.0F - part) * key) + (part * next_key));
        njPushMatrix(NULL);
        point.x = 0.0F;
        point.y = -HEAD_DROP;
        point.z = HEAD_AHEAD;
        njUnitMatrix(NULL);
        TURN_VIA(njRotateY, work->ang.y, ang);
        if (pitch) {
            njRotateX(NULL, pitch);
        }
        /* @hack redundant: the jump to rotated, which loads njCalcVector's
         * address into R2 at 0x0c909c60. without it, the address goes
         * through R3, and 2 of 765 words differ */
        goto rotated;
    rotated:
        njCalcVector(NULL, &point, &orca_head);
        /* @hack spelling: the cursors head and at, which add the position and
         * the height to orca_head by FMOV.S @R4+ and ADD #4,R5, from
         * 0x0c909c6c to 0x0c909c94. as orca_head.x += work->pos.x and the
         * like, 232 of 765 words differ. with head at &orca_head.x, 31
         * differ */
        {
            Float *head = VECTOR_FLOATS(&orca_head);
            Float *at = VECTOR_FLOATS(&work->pos);

            *head += *at;
            at++;
            head++;
            *head += *at + VARS(work)->height;
            at++;
            head++;
            *head += *at;
        }
        njPopMatrix(1);
        if (work->mode == ORCA_RISE) {
            orca_head.x = 0.0F;
            orca_head.y = -PARKED_DEPTH;
            orca_head.z = 0.0F;
        }
        if (orca_leaving == TRUE) {
            motion->speed = 2.0F;
        } else {
            Float px;
            Float pz;

            player_get_history(0, 0, &point, NULL);
            px = point.x - work->pos.x;
            distance = px * px;
            pz = point.z - work->pos.z;
            distance += pz * pz;
            distance = njSqrt(distance);
            if (distance > 400.0F) {
                if (motion->speed < 12.0F) {
                    motion->speed += 0.1F;
                }
            } else if (distance > 200.0F) {
                if (motion->speed < 6.0F) {
                    motion->speed += 0.1F;
                } else {
                    motion->speed -= 0.1F;
                }
            } else if (motion->speed > 1.0F) {
                motion->speed -= 0.1F;
            } else {
                motion->speed = 1.0F;
            }
        }
        /* x and z get the step's move along the heading. dx and dz get its
         * sway across it, which is the same at any speed */
        if (motion->speed != 0.0F) {
            x = motion->ahead.x * motion->speed;
            z = motion->ahead.z * motion->speed;
            sway =
                njSin(work->ang.y - motion->heading) * (SWAY / motion->speed);
            dx = z * sway;
            dz = -(x * sway);
        } else {
            x = 0.0F;
            z = 0.0F;
            sway = njSin(work->ang.y - motion->heading) * SWAY;
            dx = motion->ahead.z * sway;
            dz = -(motion->ahead.x * sway);
        }
        /* @hack redundant: tests of dx and dz that never run. they keep dx
         * in FR5 and dz in FR4 from 0x0c909de6 to 0x0c909e1a. they keep the
         * position's address in R4 and orca_head's in R5 at 0x0c909c6c.
         * without them, the pairs trade, and 23 of 765 words differ */
        if (0) {
            if (dx) {
            }
            if (dz) {
            }
        }
        x += dx;
        z += dz;
        /* @hack spelling: the cursors at and speed, which store orca_speed and
         * add the move to the position with ADD #4 and ADD #8, from
         * 0x0c909e18 to 0x0c909e3c. as fields, the code runs 2 bytes short,
         * and 169 of 765 words differ */
        {
            Float *at = VECTOR_FLOATS(&work->pos);
            Float *speed = VECTOR_FLOATS(&orca_speed);

            *speed = x;
            speed++;
            *speed = 0.0F;
            speed++;
            *speed = z;
            *at += x;
            at += 2;
            *at += z;
        }
        if ((work->flags & COLLISION_TOUCHING) &&
            player_task_character(work->collision->hit->owner) ==
                CHARACTER_SONIC) {
            player_set_speed(0, motion->ahead.x * 8.0F, 2.0F,
                             motion->ahead.z * 8.0F);
        }
        center = &work->collision->shapes[0].center;
        center->x = orca_head.x;
        center->y = orca_head.y;
        center->z = orca_head.z;
        collision_register(work);
        TASK_TIMER(work)++;
        /* @bug logic: the test leaves out ORCA_LEAVE (CMP/EQ #2,R0 at
         * 0x0c909e96) but not ORCA_RISE, in which orca_head lies parked
         * (stored from 0x0c909ca0). orca_init starts the timer at RIPPLE_WAIT
         * (0x0c909792), so the first step ripples the sea about the world's
         * origin */
        if (TASK_TIMER(work) > RIPPLE_WAIT && work->mode != ORCA_LEAVE) {
            ripple_x = RANDOM(-0.5F, 0.5F) * 250.0F;
            ripple_z = RANDOM(-0.5F, 0.5F) * 250.0F;
            beach_sea_ripple(orca_head.x + ripple_x, orca_head.z + ripple_z,
                             0.0F, 1.0F, 13.0F);
            work->timer = 0;
        }
        orca_draw(task);
    }
}

/* draw the orca leaping, or swimming once it leaves, at its height above
 * its position */
static void orca_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + VARS(work)->height,
                work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    if (work->mode == ORCA_LEAVE) {
        draw_shape_motion(&orca_object, &orca_swim_motion, &orca_swim_shape,
                          VARS(work)->frame);
    } else {
        draw_shape_motion(&orca_object, &orca_leap_motion, &orca_leap_shape,
                          VARS(work)->frame);
    }
    njPopMatrix(1);
}

/* make the controller forget the orca, and stop pad 0's vibration */
static void orca_destroy(struct task *task)
{
    orca_clear_task(task);
    vibration_start(0, 0, 0, 0);
}
