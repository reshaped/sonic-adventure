/* O GUNSIGHT gun sight that stands up as player 0 comes near and sways when
 * knocked */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/gunsight.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the sight is a base and two parts, each the child of the one before */
extern NJS_OBJECT gunsight_object;
extern NJS_ACTION gunsight_action;

/* player 0's distance within which the sight stands up, and beyond which it
 * lies down */
#define STAND_RANGE 40.0F
#define LIE_RANGE 50.0F
/* the steps the sight stands before it lies down, however near player 0 is */
#define STAND_STEPS 800

/* the action frames over which the top capsule turns between lying and
 * standing */
#define TILT_FRAMES 18.0F
#define LIE_DEGREES 90.0F /* how far the sight turns as it lies down */
#define POSE_SPEED 0.5F /* the action frames the sight plays each step */

/* the untouched steps after which a touch sets the sight swaying */
#define TOUCH_WAIT 10
/* the share of the lean the sway's speed loses each step, which pulls the
 * sight back upright */
#define SWAY_SPRING (1.0F / 28.0F)
/* the share of its speed the sway keeps, reversed, as it bounces off upright */
#define SWAY_BOUNCE 0.55F

struct gunsight_vars {
    Float speed; /* the sway's speed, in degrees each step */
    /* the steps since anything last touched the sight, which wrap from 127 to
     * -128 */
    Sint8 untouched_steps;
    Uint8 unk_05[3];
    Float lean; /* the sway's lean in degrees: 0 upright, below 0 leaning */
};

#define VARS(work) ((struct gunsight_vars *)&(work)->vars)

/* sight modes, in work->mode */
enum gunsight_mode {
    SIGHT_STAND, /* rising, until the action ends */
    SIGHT_LIE, /* lying down, until the action ends */
    SIGHT_STILL, /* holding the pose in work->submode */
    SIGHT_WAIT /* lying flat until player 0 moves beyond LIE_RANGE */
};

/* sight poses, in work->submode */
enum gunsight_pose {
    POSE_UP = 4, /* standing, swaying */
    POSE_DOWN, /* lying flat */
    POSE_MOVING /* playing the action to stand up or lie down */
};

static void gunsight_init(struct task *task);
static void gunsight_destroy(struct task *task);
static void gunsight_step(struct task *task);
static Bool gunsight_draw(struct task *task);

/* capsules for the sight's top and base, along the sight as it stands */
static struct collision_shape gunsight_shapes[] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-0.14499999F, 0.46499992F, 0.0F},
     3.2F,
     8.485F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     2.25F,
     1.75F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
};

void egg_carrier_gunsight_step(struct task *task)
{
    /* @hack spelling: copy, a copy of task, which SHC keeps in R14 across
     * placement_try_leave (MOV R4,R14 at 0x0c928c68); through task, SHC
     * keeps it at @R15, the function runs 2 bytes long and 10 of 14 words
     * differ */
    struct task *copy = task;

    if (!placement_try_leave(copy)) {
        gunsight_init(copy);
    }
}

static void gunsight_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        collision_init(task, gunsight_shapes, COLLISION_OBJECT, 1, 2);
    }
    /* @bug mismatched-call: gunsight_draw, a Bool that gunsight_step tests
     * (0x0c928d0e), is the task's draw, which the draw dispatch calls as a
     * void function and whose R0 it ignores (stored at 0x0c928caa) */
    /* NOLINTNEXTLINE(clang-diagnostic-cast-function-type-*) */
    task->draw = (void (*)(struct task *))gunsight_draw;
    task->step = gunsight_step;
    task->destructor = gunsight_destroy;
    work->timer = 0;
    VARS(work)->lean = 0.0F;
    VARS(work)->speed = 0.0F;
    work->mode = SIGHT_STILL;
    work->submode = POSE_DOWN;
    VARS(work)->untouched_steps = 0;
}

static void gunsight_destroy(struct task *task)
{
}

/* stand the sight up when player 0 comes within STAND_RANGE while it lies
 * still. lay it down once player 0 is beyond LIE_RANGE, or once it has stood
 * for STAND_STEPS. while it stands, a touch sways it like a spring. the top
 * capsule follows the sight */
static void gunsight_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR normal;
    NJS_VECTOR ahead;
    Float frame;
    Float tilt;
    Float bend;
    Float along;
    NJS_OBJECT *object;
    Angle ang;
    Angle twist;
    Bool moved;

    if (placement_try_leave(task)) {
        return;
    }
    work->timer++;
    if (gunsight_draw(task)) {
        switch (work->mode) {
        case SIGHT_STAND:
            work->mode = SIGHT_STILL;
            work->submode = POSE_UP;
            break;
        case SIGHT_LIE:
            work->mode = SIGHT_WAIT;
            work->submode = POSE_DOWN;
            break;
        default:
            work->mode = SIGHT_STILL;
            break;
        }
        VARS(work)->lean = 0.0F;
        VARS(work)->speed = 0.0F;
        work->timer = 0;
    }
    if (placement_editing(task)) {
        return;
    }
    if (work->submode == POSE_UP && work->mode == SIGHT_STILL &&
        TASK_TIMER(work) > STAND_STEPS) {
        work->mode = SIGHT_LIE;
        work->submode = POSE_MOVING;
        VARS(work)->lean = 0.0F;
        VARS(work)->speed = 0.0F;
        work->timer = 0;
    }
    if (njDistanceP2P(&player_work[0]->pos, &work->pos) <= STAND_RANGE &&
        work->submode == POSE_DOWN && work->mode == SIGHT_STILL) {
        work->mode = SIGHT_STAND;
        work->submode = POSE_MOVING;
        VARS(work)->lean = 0.0F;
        VARS(work)->speed = 0.0F;
        work->timer = 0;
    } else if (njDistanceP2P(&player_work[0]->pos, &work->pos) > LIE_RANGE) {
        if (work->mode == SIGHT_WAIT) {
            work->mode = SIGHT_STILL;
        }
        if (work->submode == POSE_UP) {
            work->mode = SIGHT_LIE;
            work->submode = POSE_MOVING;
            VARS(work)->lean = 0.0F;
            VARS(work)->speed = 0.0F;
            work->timer = 0;
        }
    }
    if (work->mode == SIGHT_STILL && work->submode == POSE_UP) {
        /* a touch after more than TOUCH_WAIT untouched steps sets the sight
         * swaying, at a speed from the hit's normal along z */
        if (work->collision->hit != NULL &&
            VARS(work)->untouched_steps > TOUCH_WAIT) {
            ahead.x = 0.0F;
            ahead.y = 0.0F;
            ahead.z = 1.0F;
            collision_get_hit_normal(work, &normal);
            normal.y = 0.0F;
            along = njInnerProduct(&normal, &ahead);
            VARS(work)->speed = -((along + 0.1F) * 6.0F);
        }
        /* @hack spelling: the spring's pull negated and added, an FNEG and an
         * FADD (0x0c928ea4, 0x0c928ea6); with -=, an FSUB, the function runs
         * 2 bytes short and 40 of 475 words differ */
        VARS(work)->speed += -(VARS(work)->lean * SWAY_SPRING);
        VARS(work)->lean += VARS(work)->speed;
        /* the sight swings no further than upright, where it bounces back */
        if (VARS(work)->lean + VARS(work)->speed > 0.0F &&
            VARS(work)->speed > 0.0F) {
            VARS(work)->speed = -(VARS(work)->speed * SWAY_BOUNCE);
            VARS(work)->lean = 0.0F;
        }
    }
    /* @bug logic: untouched_steps is a Sint8, so its test against 255 always
     * holds (CMP/GE at 0x0c928ee6). the count runs past 127 to -128 (ADD #1 at
     * 0x0c928eee), and a touch then sets no sway until the count climbs back
     * past TOUCH_WAIT */
    /* NOLINTNEXTLINE(clang-diagnostic-tautological-*) */
    if (work->collision->hit == NULL && VARS(work)->untouched_steps < 255) {
        VARS(work)->untouched_steps++;
    } else {
        VARS(work)->untouched_steps = 0;
    }
    if (placement_editing(task)) {
        return;
    }
    moved = FALSE;
    /* move the top capsule with the sight's top part. that part turns about z
     * by the lean or the tilt, and twists about y by a fifth of that */
    switch (work->submode) {
    case POSE_UP:
        /* bend, a Float, holds the lean truncated to a whole angle */
        bend = (int)(VARS(work)->lean * ANGLE_PER_DEGREE);
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        object = &gunsight_object;
        TURN_VIA(njRotateZ, (int)bend, ang);
        object = object->child;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        object = object->child;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        TURN_VIA(njRotateY, ang / 5, twist);
        njCalcPoint(NULL, &gunsight_shapes[0].center,
                    &work->collision->shapes[0].center);
        work->collision->shapes[0].ang.y = gunsight_shapes[0].ang.y + twist;
        work->collision->shapes[0].ang.z = gunsight_shapes[0].ang.z + ang;
        njPopMatrix(1);
        moved = TRUE;
        break;
    case POSE_DOWN:
    case POSE_MOVING:
        frame = TASK_TIMER(work) * POSE_SPEED;
        tilt = TILT_FRAMES;
        if (work->submode == POSE_DOWN) {
            frame = 0.0F;
        } else if (work->mode == SIGHT_STAND) {
            if (frame > tilt) {
                frame = tilt;
            }
        } else {
            frame -= gunsight_action.motion->nbFrame - tilt;
            if (frame < 0.0F) {
                frame = 0.0F;
            }
            frame = tilt - frame;
            if (frame < 0.0F) {
                frame = 0.0F;
            }
        }
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        frame /= tilt;
        ang = -((1.0F - frame) * (LIE_DEGREES * ANGLE_PER_DEGREE));
        object = &gunsight_object;
        TURN(njRotateZ, ang);
        object = object->child;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        object = object->child;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        TURN_VIA(njRotateY, ang / 5, twist);
        njCalcPoint(NULL, &gunsight_shapes[0].center,
                    &work->collision->shapes[0].center);
        work->collision->shapes[0].ang.y = gunsight_shapes[0].ang.y + twist;
        work->collision->shapes[0].ang.z = gunsight_shapes[0].ang.z + ang;
        njPopMatrix(1);
        moved = TRUE;
        break;
    /* @hack redundant: an empty default, which has collision_register's address
     * loaded into R3 (0x0c929152); without it, into R2, and 2 of 475 words
     * differ */
    default:
        break;
    }
    /* @hack spelling: moved compared with TRUE, CMP/EQ #1 at 0x0c92914e. the
     * step sets moved only to FALSE and TRUE, so a plain test of it is the
     * same; as one, the function runs 2 bytes short and 23 of 475 words
     * differ */
    if (moved == TRUE) {
        collision_register(work);
    }
}

/* draw the sight in its pose. return TRUE once the action that stands it up or
 * lays it down has played to its end, else FALSE */
static Bool gunsight_draw(struct task *task)
{
    /* @hack spelling: done declared and set ahead of work, which sets R13 to
     * FALSE before texture_set_stage's call and keeps njTranslateV's address
     * in R9 (0x0c929186, 0x0c929190); declared after work, SHC saves one
     * register fewer, the function runs 2 bytes short and 113 of 197 words
     * differ */
    Bool done = FALSE;
    struct task_work *work = task->work;
    Float bend;
    Float frame;
    Float last;
    NJS_OBJECT *object;
    Angle ang;

    texture_set_stage();
    switch (work->submode) {
    case POSE_UP:
        bend = (int)(VARS(work)->lean * ANGLE_PER_DEGREE);
        njPushMatrix(NULL);
        object = &gunsight_object;
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        /* the sight turns about z by the set entry's x angle plus the lean */
        TURN_VIA(njRotateZ, work->ang.x + (int)bend, ang);
        draw_model(object->model);
        object = object->child;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        draw_model(object->model);
        object = object->child;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        TURN_VIA(njRotateY, (int)(bend * 0.2F), ang);
        draw_model(object->model);
        njPopMatrix(1);
        break;
    case POSE_DOWN:
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&gunsight_action, 0.0F);
        njPopMatrix(1);
        break;
    case POSE_MOVING:
        frame = TASK_TIMER(work) * POSE_SPEED;
        last = gunsight_action.motion->nbFrame - 1;
        if (work->mode == SIGHT_STAND) {
            if (frame > last) {
                frame = last;
                done = TRUE;
            }
        } else {
            frame = last - frame;
            if (frame < 0.0F) {
                frame = 0.0F;
                done = TRUE;
            }
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_action(&gunsight_action, frame);
        njPopMatrix(1);
        break;
    }
    return done;
}
