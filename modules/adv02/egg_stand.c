/* the egg stand the object list names O EGG STAND. once something touches its
 * lid, the lid slides open, and the drawer pushes the Chao egg into the water.
 * the egg then floats to the bank */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/egg.h"
#include "adv02/egg_stand.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/splash.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/util.h"

/* the base, with the lid as its child and the drawer as the lid's sibling */
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address at 0x0c90fd34 and reads child through it, MOV.L @(44,R13),R13
 * at 0x0c90fc4e; declared as one, 12 of egg_stand_update's 441 words and a
 * pool word differ */
extern NJS_OBJECT egg_stand_object[];

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)

#define RADIUS 250.0F /* the collision record's radius */
#define LID_OPEN 27.0F /* how far the lid slides open */
#define LID_SPEED 0.3F /* how far the lid slides each step */
#define DRAWER_OUT 50.0F /* how far the drawer pushes out */
#define DRAWER_SPEED 2.0F /* how far the drawer and the egg move each step */

/* the shapes' offsets from the lid and the drawer */
#define SHAPE_X 0.0F
#define SHAPE_Y 0.0F
#define SHAPE_Z 0.0F

/* the share of its x and z speed the falling egg keeps each step */
#define DROP_DRAG 0.95F
#define FALL_LIMIT 6.0F /* the fastest the egg falls */
#define WATER_Y (-27.0F) /* the height of the water the egg drops into */
/* the speed the floating egg gains toward the bank each step */
#define FLOAT_PULL 0.035F
/* the share of its speed the floating egg keeps each step */
#define FLOAT_DRAG 0.8F
/* the point on the bank the egg floats to */
#define BANK_X 75.4F
#define BANK_Z (-38.5F)

/* a stand that starts while the set editor holds it goes to EGG_STAND_IDLE,
 * where its step only checks the range */
enum egg_stand_mode { EGG_STAND_INIT, EGG_STAND_RUN, EGG_STAND_IDLE };

/* the stand's states, in work->submode */
enum egg_stand_state {
    STAND_SHUT,
    STAND_OPENING,
    STAND_PUSHING, /* the drawer pushes the egg out */
    STAND_DROPPING, /* the egg falls to the water */
    STAND_FLOATING, /* the egg floats to the bank */
    STAND_EMPTY
};

/* work->timer holds the phase of the egg's bob on the water */
struct egg_stand_vars {
    Float lid; /* the distance it has slid */
    Float drawer; /* the distance it has pushed out */
};

#define VARS(work) ((struct egg_stand_vars *)&(work)->vars)

static void egg_stand_init(struct task *task);
static void egg_stand_update(struct task *task);
static void egg_stand_draw(struct task *task);
static void egg_stand_destroy(struct task *task);

/* where the egg sits in the stand, in the stand's own frame */
static NJS_POINT3 egg_stand_egg_pos = {-150.803F, 56.095F, -177.572F};
/* TRUE once the egg has floated to the bank. nothing sets it FALSE again */
static Bool egg_stand_egg_floated = FALSE;

/* the lid's box, then the drawer's */
enum { LID_SHAPE, DRAWER_SHAPE };

static struct collision_shape egg_stand_shapes[2] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     23.0F,
     9.0F,
     28.0F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     15.0F,
     12.5F,
     28.5F,
     {0},
     0,
     NULL},
};

/* sand_switch.c's sand_switch_flags again: by character, the flag that opens
 * the way to Sand Hill, or -1. nothing reads this copy */
static int egg_stand_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_MR_ENTRANCE_SAND_BOARD,
    -1,
    FLAG_TAILS_MR_ENTRANCE_SAND_BOARD,
    -1,
    -1,
    -1,
    -1,
    -1,
};

/* the egg's speed while the stand moves it */
static NJS_VECTOR egg_stand_egg_speed;
/* the unit vector, in x and z, from where the egg lands to the bank */
static NJS_VECTOR egg_stand_egg_heading;

/* the stand puts nothing in the collision world, so the set editor's arm has
 * nothing to rebuild, and the step's two arms are the same */
void egg_stand_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case EGG_STAND_INIT:
            egg_stand_init(task);
            break;
        case EGG_STAND_RUN:
            egg_stand_draw(task);
            egg_stand_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case EGG_STAND_INIT:
            egg_stand_init(task);
            break;
        case EGG_STAND_RUN:
            egg_stand_draw(task);
            egg_stand_update(task);
            break;
        }
    }
}

static void egg_stand_init(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;
    NJS_OBJECT *part;

    if (placement_editing(task)) {
        work->mode = EGG_STAND_IDLE;
        return;
    }
    task->draw = egg_stand_draw;
    task->destructor = egg_stand_destroy;
    collision_init(task, egg_stand_shapes, COLLISION_OBJECT, 1, 2);
    collision = work->collision;
    collision->radius = RADIUS;
    part = egg_stand_object->child;
    collision->shapes[LID_SHAPE].ang.y = part->ang[1];
    collision->shapes[LID_SHAPE].center.y = part->pos[1] + SHAPE_Y;
    part = egg_stand_object->child->sibling;
    collision->shapes[DRAWER_SHAPE].center.x = part->pos[0] + SHAPE_X;
    collision->shapes[DRAWER_SHAPE].center.y = part->pos[1] + SHAPE_Y;
    if (egg_stand_floated()) {
        VARS(work)->lid = LID_OPEN;
        VARS(work)->drawer = DRAWER_OUT;
        work->submode = STAND_EMPTY;
        egg_in_stand = FALSE;
    } else {
        VARS(work)->lid = 0.0F;
        VARS(work)->drawer = 0.0F;
        work->submode = STAND_SHUT;
        egg_in_stand = TRUE;
    }
    egg_stand_egg_speed.x = 0.0F;
    egg_stand_egg_speed.y = 0.0F;
    egg_stand_egg_speed.z = 0.0F;
    work->mode = EGG_STAND_RUN;
}

static void egg_stand_update(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct task *egg;
    struct task_work *egg_work;
    NJS_OBJECT *part;
    Angle ang;
    NJS_POINT3 pos;
    NJS_VECTOR ripple;

    if (egg_task == NULL) {
        egg = task_create(TASK_MOTION | TASK_WORK, 3, egg_step);
        egg_work = egg->work;
        if (work->submode == STAND_EMPTY) {
            egg_work->pos.x = BANK_X;
            egg_work->pos.y = WATER_Y;
            egg_work->pos.z = BANK_Z;
        } else {
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN_VIA(njRotateY, work->ang.y, ang);
            njCalcPoint(NULL, &egg_stand_egg_pos, &egg_work->pos);
            njPopMatrix(1);
        }
        egg_step(egg);
    }
    if (egg_task != NULL) {
        egg_work = egg_task->work;
        switch (work->submode) {
        case STAND_SHUT:
            if ((collision->flags & COLLISION_HIT) &&
                collision->own_shape == LID_SHAPE) {
                work->submode = STAND_OPENING;
            }
            break;
        case STAND_OPENING:
            VARS(work)->lid += LID_SPEED;
            if (VARS(work)->lid >= LID_OPEN) {
                VARS(work)->lid = LID_OPEN;
                egg_stand_egg_speed.x = njSin(work->ang.y) * DRAWER_SPEED;
                egg_stand_egg_speed.z = njCos(work->ang.y) * DRAWER_SPEED;
                work->submode = STAND_PUSHING;
            }
            break;
        case STAND_PUSHING:
            egg_work->pos.x += egg_stand_egg_speed.x;
            egg_work->pos.z += egg_stand_egg_speed.z;
            VARS(work)->drawer += DRAWER_SPEED;
            if (VARS(work)->drawer >= DRAWER_OUT) {
                VARS(work)->drawer = DRAWER_OUT;
                work->submode = STAND_DROPPING;
            }
            break;
        case STAND_DROPPING:
            egg_stand_egg_speed.x *= DROP_DRAG;
            egg_stand_egg_speed.z *= DROP_DRAG;
            egg_stand_egg_speed.y -= egg_task->motion->gravity;
            if (egg_stand_egg_speed.y < -FALL_LIMIT) {
                egg_stand_egg_speed.y = -FALL_LIMIT;
            }
            egg_work->pos.x += egg_stand_egg_speed.x;
            egg_work->pos.y += egg_stand_egg_speed.y;
            egg_work->pos.z += egg_stand_egg_speed.z;
            if (egg_work->pos.y <= WATER_Y) {
                egg_work->pos.y = WATER_Y;
                egg_stand_egg_heading.x = -egg_work->pos.x + BANK_X;
                egg_stand_egg_heading.y = 0.0F;
                egg_stand_egg_heading.z = -egg_work->pos.z + BANK_Z;
                njUnitVector(&egg_stand_egg_heading);
                TASK_TIMER(work) = 0;
                work->submode = STAND_FLOATING;
            }
            break;
        case STAND_FLOATING:
            egg_stand_egg_speed.x += egg_stand_egg_heading.x * FLOAT_PULL;
            egg_stand_egg_speed.z += egg_stand_egg_heading.z * FLOAT_PULL;
            egg_stand_egg_speed.x *= FLOAT_DRAG;
            egg_stand_egg_speed.z *= FLOAT_DRAG;
            egg_work->pos.x += egg_stand_egg_speed.x;
            egg_work->pos.z += egg_stand_egg_speed.z;
            egg_work->pos.y = WATER_Y + (njSin(TASK_TIMER(work)) * 0.3F);
            TASK_TIMER(work) += 0x800;
            if (rand() * RAND_SCALE < 0.1F) {
                ripple.x = 0.09F;
                ripple.y = 0.0F;
                ripple.z = 3.4F;
                pos.x = egg_work->pos.x;
                pos.y = WATER_Y;
                pos.z = egg_work->pos.z;
                ripple_create(&pos, &ripple, 0.1F);
            }
            if ((-egg_work->pos.x + BANK_X) * egg_stand_egg_heading.x <= 0.0F &&
                (-egg_work->pos.z + BANK_Z) * egg_stand_egg_heading.z <= 0.0F) {
                egg_stand_set_floated(TRUE);
                egg_in_stand = FALSE;
                work->submode = STAND_EMPTY;
            }
            break;
        }
        part = egg_stand_object->child;
        collision->shapes[LID_SHAPE].center.x =
            part->pos[0] + (SHAPE_Z * njSin(part->ang[1])) +
            ((VARS(work)->lid + SHAPE_X) * njCos(part->ang[1]));
        collision->shapes[LID_SHAPE].center.z =
            part->pos[2] + (SHAPE_Z * njCos(part->ang[1])) -
            ((VARS(work)->lid + SHAPE_X) * njSin(part->ang[1]));
        part = egg_stand_object->child->sibling;
        collision->shapes[DRAWER_SHAPE].center.z =
            part->pos[2] + VARS(work)->drawer + SHAPE_Z;
        collision_register(work);
    }
}

static void egg_stand_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *part;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(egg_stand_object->model);
    part = egg_stand_object->child;
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)part->pos);
    TURN_VIA(njRotateY, part->ang[1], ang);
    njTranslate(NULL, VARS(work)->lid, 0.0F, 0.0F);
    draw_model(part->model);
    njPopMatrix(1);
    part = egg_stand_object->child->sibling;
    njPushMatrix(NULL);
    njTranslate(NULL, part->pos[0], part->pos[1],
                part->pos[2] + VARS(work)->drawer);
    draw_model(part->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void egg_stand_destroy(struct task *task)
{
    egg_in_stand = FALSE;
}

void egg_stand_set_floated(Bool floated)
{
    egg_stand_egg_floated = floated;
}

Bool egg_stand_floated(void)
{
    return egg_stand_egg_floated;
}
