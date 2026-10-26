/* O TORBRI: bridge sections that break apart as the tornado nears. the
 * tornado also throws sections, which break near the point they fly at */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "draw/helpers.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "stg02/bridge.h"
#include "stg02/tornado.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT windy_bridge_object;
extern NJS_OBJECT windy_bridge_object_1;
extern NJS_OBJECT windy_bridge_object_2;
extern NJS_OBJECT windy_bridge_object_3;
extern NJS_OBJECT windy_bridge_piece_0_object;
extern NJS_OBJECT windy_bridge_piece_1_object;
extern NJS_OBJECT windy_bridge_piece_2_object;
extern NJS_OBJECT windy_bridge_piece_3_object;
extern NJS_OBJECT windy_bridge_piece_4_object;
extern NJS_OBJECT windy_bridge_piece_5_object;
extern NJS_OBJECT windy_bridge_piece_6_object;
extern NJS_OBJECT windy_bridge_piece_7_object;
extern NJS_OBJECT windy_bridge_piece_8_object;
extern NJS_OBJECT windy_bridge_piece_9_object;
extern NJS_OBJECT windy_bridge_piece_10_object;
extern NJS_OBJECT windy_bridge_piece_11_object;
extern NJS_OBJECT windy_bridge_piece_12_object;
extern NJS_OBJECT windy_bridge_piece_13_object;
extern NJS_OBJECT windy_bridge_piece_14_object;
extern NJS_OBJECT windy_bridge_piece_15_object;
extern NJS_OBJECT windy_bridge_piece_16_object;
extern NJS_OBJECT windy_bridge_piece_17_object;
extern NJS_OBJECT windy_bridge_piece_18_object;
extern NJS_OBJECT windy_bridge_piece_19_object;
extern NJS_OBJECT windy_bridge_piece_20_object;
extern NJS_OBJECT windy_bridge_piece_21_object;
extern NJS_OBJECT windy_bridge_piece_22_object;
extern NJS_OBJECT windy_bridge_piece_23_object;
extern NJS_OBJECT windy_bridge_piece_24_object;
extern NJS_OBJECT windy_bridge_piece_25_object;
extern NJS_OBJECT windy_bridge_piece_26_object;
extern NJS_OBJECT windy_bridge_piece_27_object;
extern NJS_OBJECT windy_bridge_piece_28_object;
extern NJS_OBJECT windy_bridge_piece_29_object;
extern NJS_OBJECT windy_bridge_piece_30_object;
extern NJS_OBJECT windy_bridge_piece_31_object;
extern NJS_OBJECT windy_bridge_piece_32_object;

/* a section destroys itself once the camera and player 0 are farther than
 * BRIDGE_RANGE */
#define BRIDGE_RANGE 1500.0F

/* once the tornado comes within BREAK_REACH across the ground, a standing
 * section, scl.y 0 or more, breaks. a rising section, scl.y below 0, starts
 * to rise then. it breaks once its timer passes RISE_STEPS. a broken section
 * runs its pieces for PIECES_STEPS steps, then destroys them */
#define BREAK_REACH 190.0F
#define RISE_STEPS 120
#define PIECES_STEPS 600

/* a rising section puffs dust each step the tornado is within DUST_REACH
 * across the ground. a standing section counts the steps the tornado is
 * within SHAKE_REACH. it puffs dust each time the count passes SHAKE_WAIT */
#define DUST_REACH 95.0F
#define SHAKE_REACH 200.0F
#define SHAKE_WAIT 20
/* a rising section puffs dust once tick passes RISE_DUST_WAIT, which never
 * happens: only a standing section counts tick */
#define RISE_DUST_WAIT 5

/* a rising section climbs RISE_SPEED a step, and turns RISE_TUMBLE about x */
#define RISE_SPEED 0.14F
#define RISE_TUMBLE 63

/* a thrown section's distance a step grows DEBRIS_GAIN for each step it has
 * flown. a mark that isn't still climbs MARK_CLIMB a step. the section turns
 * TUMBLE about x and about y a step */
#define DEBRIS_GAIN 0.012F
#define MARK_CLIMB 0.85F
#define TUMBLE 255

/* a thrown section runs its pieces for DEBRIS_STEPS steps */
#define DEBRIS_STEPS 250

/* a section breaks into the first PIECES piece models. the tornado's spiral,
 * in debris.c, draws the last ten */
#define PIECES 18

/* a piece flies at once at a chance above FLY_CHANCE, two in five. the rest
 * wait WAIT_STEPS */
#define FLY_CHANCE 0.6F
#define WAIT_STEPS 5

/* a piece that isn't thrown flies up at RISE_MIN to RISE_MAX a step. it
 * flies at up to SPREAD along x and along z. a thrown piece flies up at up
 * to SPREAD */
#define SPREAD 2.0F
#define RISE_MIN 2.5F
#define RISE_MAX 4.0F

/* a piece spins up to half SPIN_X about x and half SPIN_Y about y a step,
 * either way */
#define SPIN_X 3000.0F
#define SPIN_Y 1800.0F

/* a flying piece loses PIECE_GRAVITY of y speed a step. a thrown piece moves
 * THROWN_PACE times its speed a step as it flies */
#define PIECE_GRAVITY 0.1F
#define THROWN_PACE 3.0F

/* a falling piece loses DEBRIS_GRAVITY of y speed a step, down to
 * DEBRIS_FALL_LIMIT. it moves FALL_PACE times its x and z speeds a step, and
 * DROP_PACE times its y speed */
#define DEBRIS_GRAVITY 0.14F
#define DEBRIS_FALL_LIMIT (-0.12F)
#define FALL_PACE 2.0F
#define DROP_PACE 7.0F

/* a circling piece starts its heading at CIRCLE_START and turns it
 * CIRCLE_TURN a step, in degrees. it moves CIRCLE_SPEED a step along its
 * heading, and rises CIRCLE_RISE. it stops being drawn once its timer passes
 * CIRCLE_STEPS */
#define CIRCLE_START 90.0F
#define CIRCLE_TURN 3.0F
#define CIRCLE_SPEED 3.4F
#define CIRCLE_RISE 0.1F
#define CIRCLE_STEPS 225

enum bridge_mode { BRIDGE_WAIT = 0, BRIDGE_BROKEN = 1, BRIDGE_DONE = 6 };

/* a rising section's stages, in work->submode. once submode leaves
 * RISE_START, the step moves the section on however far the tornado is */
enum bridge_rise { RISE_START, RISE_UP };

enum piece_mode {
    PIECE_WAIT = 2,
    PIECE_FLY,
    PIECE_FALL, /* a thrown piece, after it flies */
    PIECE_CIRCLE, /* any other piece, after it flies */
    PIECE_DONE
};

/* the modes of a section the tornado throws */
enum debris_mode { DEBRIS_START, DEBRIS_FLY, DEBRIS_BROKEN, DEBRIS_DONE };

struct bridge_vars {
    Float heading; /* the way a rising section moves, in degrees about y */
};

/* a piece's speed and spins, laid over its task's motion. a spin is the turn
 * the piece adds each step */
struct piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle spin_x;
    Angle spin_y;
};

#define BRIDGE(work) ((struct bridge_vars *)&(work)->vars)
#define PIECE_MOTION(task) ((struct piece_motion *)(task)->motion)

/* the bridge's piece models, ended by NULL, though each loop here stops at
 * PIECES */
static NJS_OBJECT *windy_bridge_pieces[] = {
    &windy_bridge_piece_0_object,  &windy_bridge_piece_1_object,
    &windy_bridge_piece_2_object,  &windy_bridge_piece_3_object,
    &windy_bridge_piece_4_object,  &windy_bridge_piece_5_object,
    &windy_bridge_piece_6_object,  &windy_bridge_piece_7_object,
    &windy_bridge_piece_8_object,  &windy_bridge_piece_9_object,
    &windy_bridge_piece_10_object, &windy_bridge_piece_11_object,
    &windy_bridge_piece_12_object, &windy_bridge_piece_13_object,
    &windy_bridge_piece_14_object, &windy_bridge_piece_15_object,
    &windy_bridge_piece_16_object, &windy_bridge_piece_17_object,
    &windy_bridge_piece_18_object, &windy_bridge_piece_19_object,
    &windy_bridge_piece_20_object, &windy_bridge_piece_21_object,
    &windy_bridge_piece_22_object, &windy_bridge_piece_23_object,
    &windy_bridge_piece_24_object, &windy_bridge_piece_25_object,
    &windy_bridge_piece_26_object, &windy_bridge_piece_27_object,
    &windy_bridge_piece_28_object, &windy_bridge_piece_29_object,
    &windy_bridge_piece_30_object, &windy_bridge_piece_31_object,
    &windy_bridge_piece_32_object, NULL,
};

static void windy_bridge_step(struct task *task);
static void windy_bridge_draw(struct task *task);
static void windy_bridge_destroy(struct task *task);
static void windy_bridge_piece_step(struct task *task);
static void windy_bridge_piece_draw(struct task *task);
static void windy_bridge_piece_destroy(struct task *task);
static void windy_bridge_break(struct task *task, int thrown);
static void windy_bridge_rise(struct task *task, Float distance);
static void windy_bridge_debris_draw(struct task *task);

void windy_bridge_start(struct task *task)
{
    task->step = windy_bridge_step;
    task->draw = windy_bridge_draw;
    task->destructor = windy_bridge_destroy;
}

/* a waiting section draws itself each step. until the tornado's first run
 * step, that is all it does */
static void windy_bridge_step(struct task *task)
{
    static const NJS_VECTOR bridge_dust_near = {0.0F, 1.0F, 0.0F};
    static const NJS_VECTOR bridge_dust_rising = {0.0F, 1.0F, 0.0F};
    static const NJS_VECTOR bridge_dust_shaking = {5.0F, 4.0F, 2.0F};
    struct task_work *work = task->work;
    Float distance;
    NJS_VECTOR speed;

    if (placement_try_leave_by(task, SQUARE(BRIDGE_RANGE))) {
        return;
    }
    switch (work->mode) {
    case BRIDGE_WAIT:
        if (windy_tornado_distance(&work->pos, &distance)) {
            if (distance < BREAK_REACH || work->submode) {
                if (distance < DUST_REACH) {
                    speed = bridge_dust_near;
                    dust_create(&work->pos, &speed, 10.0F);
                }
                if (work->scl.y >= 0.0F) {
                    windy_bridge_break(task, FALSE);
                    work->mode = BRIDGE_BROKEN;
                } else {
                    windy_bridge_rise(task, distance);
                    /* @bug logic: a rising section waits for tick to pass
                     * RISE_DUST_WAIT before it puffs dust (CMP/GT at
                     * 0x0c905112). only a standing section counts tick
                     * (0x0c905152), so the test never holds */
                    if (work->tick > RISE_DUST_WAIT) {
                        speed = bridge_dust_rising;
                        work->tick = 0;
                        dust_create(&work->pos, &speed, 10.0F);
                    }
                }
            }
            if (work->scl.y >= 0.0F && distance < SHAKE_REACH) {
                work->tick++;
                if (work->tick > SHAKE_WAIT) {
                    speed = bridge_dust_shaking;
                    work->tick = 0;
                    dust_create(&work->pos, &speed, 18.0F);
                }
            }
        }
        windy_bridge_draw(task);
        break;
    case BRIDGE_BROKEN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > PIECES_STEPS) {
            work->mode = BRIDGE_DONE;
            task_destroy_children(task);
        } else {
            task_step_children(task);
        }
        break;
    case BRIDGE_DONE:
        break;
    }
}

static void windy_bridge_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    switch ((int)work->scl.x) {
    case 0:
        draw_tree(&windy_bridge_object);
        break;
    case 1:
        draw_tree(&windy_bridge_object_1);
        break;
    case 2:
        draw_tree(&windy_bridge_object_2);
        break;
    case 3:
        draw_tree(&windy_bridge_object_3);
        break;
    default:
        draw_tree(&windy_bridge_object);
        break;
    }
    njPopMatrix(1);
}

static void windy_bridge_destroy(struct task *task)
{
    task_destroy_children(task);
}

void windy_bridge_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);
    struct task_work *parent;
    Float chance;

    work->timer = 0;
    chance = rand() * RAND_SCALE;
    work->mode = PIECE_WAIT;
    /* @bug logic: the waits of 2 and 3 steps test for a chance above 0.75
     * and above 0.95 (FCMP/GT at 0x0c905364 and 0x0c905372). a chance above
     * FLY_CHANCE has already flown at once, so neither test holds */
    if (chance > FLY_CHANCE) {
        work->timer = 0;
        work->mode = PIECE_FLY;
    } else if (chance > 0.75F) {
        work->timer = 2;
    } else if (chance > 0.95F) {
        work->timer = 3;
    } else {
        work->timer = WAIT_STEPS;
    }
    parent = task->parent->work;
    if (PIECE(work)->thrown[0] == FALSE) {
        motion->speed.x = rand() * (SPREAD * RAND_SCALE);
        motion->speed.y = RANDOM(RISE_MIN, RISE_MAX);
        motion->speed.z = rand() * (SPREAD * RAND_SCALE);
    } else {
        motion->speed.x = (rand() * parent->scl.x) * RAND_SCALE;
        motion->speed.y = rand() * (SPREAD * RAND_SCALE);
        motion->speed.z = (rand() * parent->scl.z) * RAND_SCALE;
    }
    motion->spin_y = ((rand() * RAND_SCALE) - 0.5F) * SPIN_Y;
    motion->spin_x = ((rand() * RAND_SCALE) - 0.5F) * SPIN_X;
    task->step = windy_bridge_piece_step;
    task->draw = windy_bridge_piece_draw;
    task->destructor = windy_bridge_piece_destroy;
    windy_bridge_piece_draw(task);
}

/* a waiting piece counts its timer down to 0, but a thrown one waits one
 * step at most. a flying piece rises until its y speed turns down. a thrown
 * piece then falls, and any other circles */
static void windy_bridge_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);
    /* @hack spelling: pos and speed read and written back a Float at a time
     * through Float cursors. read through the members, 81 of 245 words
     * differ. written through them, the function runs 32 bytes long and 119
     * differ */
    Float *speed;
    Float *pos;
    Float vx;
    Float x;
    Float y;
    Float z;
    Float vy;
    Float vz;

    speed = &motion->speed.x;
    pos = &work->pos.x;
    x = *pos++;
    y = *pos++;
    z = *pos++;
    vx = *speed++;
    pos -= 3;
    vy = *speed++;
    vz = *speed++;
    speed -= 3;
    switch (work->mode) {
    case PIECE_WAIT:
        TASK_TIMER(work)--;
        if (TASK_TIMER(work) <= 0) {
            work->timer = 0;
            work->mode = PIECE_FLY;
        }
        if (PIECE(work)->thrown[0] == TRUE) {
            work->timer = 0;
            work->mode = PIECE_FLY;
        }
        windy_bridge_piece_draw(task);
        break;
    case PIECE_FLY:
        if (PIECE(work)->thrown[0] == FALSE) {
            x += vx;
            y += vy;
            z += vz;
            vy -= PIECE_GRAVITY;
            work->ang.y += motion->spin_y;
            work->ang.x += motion->spin_x;
            if (vy < 0.0F) {
                work->mode = PIECE_CIRCLE;
                PIECE(work)->heading = CIRCLE_START;
            }
        } else {
            x += vx * THROWN_PACE;
            y += vy * THROWN_PACE;
            z += vz * THROWN_PACE;
            vy -= PIECE_GRAVITY;
            work->ang.y += motion->spin_y;
            work->ang.x += motion->spin_x;
            if (vy < 0.0F) {
                work->mode = PIECE_FALL;
            }
        }
        windy_bridge_piece_draw(task);
        break;
    case PIECE_FALL:
        vy -= DEBRIS_GRAVITY;
        if (vy < DEBRIS_FALL_LIMIT) {
            vy = DEBRIS_FALL_LIMIT;
        }
        x += vx * FALL_PACE;
        y += vy * DROP_PACE;
        z += vz * FALL_PACE;
        work->ang.y += motion->spin_y;
        work->ang.x += motion->spin_x;
        TASK_TIMER(work)++;
        windy_bridge_piece_draw(task);
        break;
    case PIECE_CIRCLE:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > CIRCLE_STEPS) {
            work->mode = PIECE_DONE;
        }
        work->ang.y += motion->spin_y;
        work->ang.x += motion->spin_x;
        PIECE(work)->heading += CIRCLE_TURN;
        x += njSin((Angle)(PIECE(work)->heading * ANGLE_PER_DEGREE)) *
             CIRCLE_SPEED;
        y += CIRCLE_RISE;
        z += njCos((Angle)(PIECE(work)->heading * ANGLE_PER_DEGREE)) *
             CIRCLE_SPEED;
        windy_bridge_piece_draw(task);
        break;
    case PIECE_DONE:
        task->draw = NULL;
        break;
    }
    *pos = x;
    pos++;
    *pos = y;
    pos++;
    *pos = z;
    *speed++ = vx;
    *speed++ = vy;
    *speed = vz;
}

static void windy_bridge_piece_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(PIECE(work)->node->model);
    njPopMatrix(1);
}

static void windy_bridge_piece_destroy(struct task *task)
{
}

/* break the section into PIECES pieces, its children, and puff dust. each
 * piece starts at the section's position plus its model's offset. it takes
 * the section's ang.y, but the offset doesn't turn with it. thrown TRUE makes
 * them thrown pieces */
static void windy_bridge_break(struct task *task, int thrown)
{
    static const NJS_VECTOR bridge_break_dust = {0.0F, 1.0F, 0.0F};
    NJS_VECTOR speed;
    struct task_work *work;
    struct task_work *piece;
    struct task *child;
    NJS_OBJECT *node;
    Float x;
    Float y;
    Float z;
    Angle ang;
    int i;

    speed = bridge_break_dust;
    work = task->work;
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    ang = work->ang.y;
    for (i = 0; i < PIECES; i++) {
        child = task_create_child(TASK_WORK | TASK_MOTION,
                                  windy_bridge_piece_start, task);
        piece = child->work;
        node = windy_bridge_pieces[i];
        PIECE(piece)->node = node;
        piece->ang.y = ang;
        piece->pos.x = x + node->pos[0];
        piece->pos.y = y + node->pos[1];
        piece->pos.z = z + node->pos[2];
        if (thrown == TRUE) {
            PIECE(piece)->thrown[0] = TRUE;
        }
    }
    dust_create(&work->pos, &speed, 13.0F);
}

void windy_bridge_break_at(NJS_POINT3 *pos, int thrown, struct task *task)
{
    static const NJS_VECTOR bridge_break_at_dust = {0.0F, 1.0F, 0.0F};
    NJS_VECTOR speed;
    struct task_work *work;
    struct task_work *piece;
    struct task *child;
    NJS_OBJECT *node;
    Float x;
    Float y;
    Float z;
    int i;

    speed = bridge_break_at_dust;
    work = task->work;
    x = pos->x;
    y = pos->y;
    z = pos->z;
    for (i = 0; i < PIECES; i++) {
        child = task_create_child(TASK_WORK | TASK_MOTION,
                                  windy_bridge_piece_start, task);
        piece = child->work;
        node = windy_bridge_pieces[i];
        PIECE(piece)->node = node;
        piece->pos.x = x + node->pos[0];
        piece->pos.y = y + node->pos[1];
        piece->pos.z = z + node->pos[2];
        if (thrown == TRUE) {
            PIECE(piece)->thrown[0] = TRUE;
        }
    }
    dust_create(&work->pos, &speed, 13.0F);
}

/* the first call only starts the rise. each later call moves the section a
 * little farther along its heading than the last. the heading turns a little
 * faster with each call. distance is unused */
static void windy_bridge_rise(struct task *task, Float distance)
{
    struct task_work *work = task->work;
    /* @hack redundant: a vector declared and never used, which keeps the frame
     * at 16 bytes (ADD #-16,R15 at 0x0c905b12). without it, 2 of 112 words
     * differ */
    NJS_VECTOR unused;
    Float radius;

    switch (work->submode) {
    case RISE_START:
        work->submode = RISE_UP;
        break;
    case RISE_UP:
        BRIDGE(work)->heading += 0.07F + (0.01F * TASK_TIMER(work)++);
        radius = 0.2F + (0.015F * TASK_TIMER(work));
        work->ang.x += RISE_TUMBLE;
        work->pos.x +=
            njSin((Angle)(BRIDGE(work)->heading * ANGLE_PER_DEGREE)) * radius;
        work->pos.y += RISE_SPEED;
        work->pos.z +=
            njCos((Angle)(BRIDGE(work)->heading * ANGLE_PER_DEGREE)) * radius;
        if (TASK_TIMER(work) > RISE_STEPS) {
            work->timer = 0;
            windy_bridge_break(task, FALSE);
            work->mode = BRIDGE_BROKEN;
        }
        break;
    }
}

/* as the section breaks, work->scl changes from its mark to the unit vector
 * toward it. its thrown pieces fly out along that */
void windy_bridge_debris_step(struct task *task)
{
    struct task_work *work = task->work;
    Float distance;

    switch (work->mode) {
    case DEBRIS_START:
        work->mode = DEBRIS_FLY;
        DEBRIS(work)->speed = _builtin_fabsf(rand() * RAND_SCALE) + 1.0F;
        break;
    case DEBRIS_FLY:
        TASK_TIMER(work)++;
        distance = DEBRIS(work)->speed + (DEBRIS_GAIN * TASK_TIMER(work));
        if (DEBRIS(work)->still != TRUE) {
            work->scl.y += MARK_CLIMB;
        }
        point_toward(&work->pos, &work->scl, distance, &work->pos);
        windy_bridge_debris_draw(task);
        work->ang.x += TUMBLE;
        work->ang.y += TUMBLE;
        if (njDistanceP2P(&work->pos, &work->scl) < DEBRIS(work)->reach) {
            work->timer = 0;
            njSubVector(&work->scl, &work->pos);
            njUnitVector(&work->scl);
            windy_bridge_break(task, TRUE);
            work->mode = DEBRIS_BROKEN;
        }
        break;
    case DEBRIS_BROKEN:
        if (TASK_TIMER(work)++ < DEBRIS_STEPS) {
            task_step_children(task);
        } else {
            work->mode = DEBRIS_DONE;
        }
        break;
    case DEBRIS_DONE:
        task_destroy_children(task);
        break;
    }
}

static void windy_bridge_debris_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&windy_bridge_object);
    njPopMatrix(1);
}
