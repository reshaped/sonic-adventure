/* the doors, which the set editor names O DOOR, O DOOR B and O DOUBLE DOOR.
 * the first two are rows of panels. they rise for player 0 from one side,
 * and shut for good from the other. a double door's two leaves swing open */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg03/doors.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the objects declared as arrays, so a step that stores one
 * twice loads its address once, as door_panel_step does into R7 at
 * 0x0c904334. declared as single objects and named by their addresses,
 * summed over the five functions that use them, 476 of 634 words differ */
extern NJS_OBJECT door_panel_object[];
extern NJS_OBJECT door_b_panel_object[];
extern NJS_OBJECT double_door_object[];

/* O DOOR and O DOOR B destroy themselves once neither the camera nor
 * player 0 is within 610. O DOUBLE DOOR's range is 1010 */
#define DOOR_RANGE (610.0F * 610.0F)
#define DOUBLE_DOOR_RANGE (1010.0F * 1010.0F)

/* door_set_mode acts only while player 0 is within DOOR_WATCH. O DOOR takes
 * DOOR_NEAR and DOOR_SIDE as its OPEN_NEAR and SIDE. O DOOR B takes
 * DOOR_B_NEAR and DOOR_B_SIDE. their panels rise RISE_SPEED a step, to
 * DOOR_TOP and DOOR_B_TOP. their collision records reach DOOR_REACH and
 * DOOR_B_REACH */
#define DOOR_WATCH 100.0F
#define DOOR_NEAR 25.0F
#define DOOR_SIDE 90.0F
#define DOOR_REACH 39.0F
#define DOOR_TOP 39.0F
#define DOOR_B_NEAR 60.0F
#define DOOR_B_SIDE 180.0F
#define DOOR_B_REACH 40.0F
#define DOOR_B_TOP 40.0F
#define RISE_SPEED 1.0F

/* O DOUBLE DOOR's collision record reaches DOUBLE_DOOR_REACH. with
 * KEY_NEAR, the door stays open while player 0 is within DOUBLE_DOOR_NEAR */
#define DOUBLE_DOOR_REACH 80.0F
#define DOUBLE_DOOR_NEAR 50.0F
/* the hinge opens to LEAF_OPEN, a quarter turn back from 0, and shuts to
 * 0. it turns LEAF_SPEED a step */
#define LEAF_SPEED 0x100
#define LEAF_OPEN 0xc000
/* in MIRRORS_ACT, the double door draws its arch's first material unlit,
 * and each leaf's first LEAF_MATERIALS */
#define MIRRORS_ACT (STAGE_TWINKLE_MIRRORS & 0xff)
#define LEAF_MATERIALS 3

struct door_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    Angle hinge; /* how far O DOUBLE DOOR's leaves have turned */
};

#define VARS(work) ((struct door_vars *)&(work)->vars)
/* a door's work->scl. OPEN_NEAR is how near player 0 comes to open the
 * door. RISE is how far its panels have risen. SIDE is the degrees
 * door_set_mode takes from ang.y for the heading of the door's way in. a
 * panel's work->scl.y holds its door's RISE */
#define OPEN_NEAR(work) ((work)->scl.x)
#define RISE(work) ((work)->scl.y)
#define SIDE(work) ((work)->scl.z)

enum door_mode {
    DOOR_INIT,
    DOOR_WAIT, /* the panels lower */
    DOOR_OPEN, /* the panels rise */
    DOOR_SHUT /* the panels lower, and the door is solid for good */
};

enum panel_mode { PANEL_INIT, PANEL_RUN };

enum double_door_mode {
    DOUBLE_DOOR_INIT,
    DOUBLE_DOOR_SHUT,
    DOUBLE_DOOR_OPEN,
    DOUBLE_DOOR_LOCKED /* it shuts, and never opens */
};

/* what opens O DOUBLE DOOR, in work->submode. the set entry's scale x picks
 * it: KEY_SWITCHES above 0, and KEY_NEAR at 0. below 0, the door starts
 * DOUBLE_DOOR_LOCKED */
enum double_door_key {
    KEY_SWITCHES, /* switches 0, 1 and 2 all on, which open it for good */
    KEY_NEAR /* player 0 within DOUBLE_DOOR_NEAR */
};

static void door_panel_step(struct task *task);
static void door_b_panel_step(struct task *task);

/* clang-format off */
static struct collision_shape door_shape[1] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 19.5F, 0.0F}, 1.0F, 19.5F, 23.0F, {0, 0, 0}, 0, NULL},
};
/* shape 0 is the shut door. shapes 1 to 3, on while the door is open, are
 * the arch and each leaf turned out */
static struct collision_shape double_door_shapes[4] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 40.0F, -2.5F}, 40.0F, 40.0F, 3.75F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 67.66F, -2.5F}, 40.0F, 12.35F, 3.75F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {37.5F, 27.66F, 17.5F}, 3.75F, 27.66F, 20.0F, {0, 0, 0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-37.5F, 27.66F, 17.5F}, 3.75F, 27.66F, 20.0F, {0, 0, 0}, 0, NULL},
};
static struct collision_shape door_b_shape[1] = {
    {SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 20.0F, 0.0F}, 1.0F, 20.0F, 20.0F, {0, 0x4000, 0}, 0, NULL},
};
/* clang-format on */

/* a door's panel: a child at x, z from the door, whose var is its object */
#define PANEL(step, x, z, object)                                              \
    {(step), TASK_WORK, 0, {0, 0, 0}, {(x), 0.0F, (z)}, (Sint32)(object)}
#define NO_PANEL {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0}

static struct task_spec door_panels[8] = {
    PANEL(door_panel_step, 0.0F, -150.0F, door_panel_object),
    PANEL(door_panel_step, 0.0F, -100.0F, door_panel_object),
    PANEL(door_panel_step, 0.0F, -50.0F, door_panel_object),
    PANEL(door_panel_step, 0.0F, 0.0F, door_panel_object),
    PANEL(door_panel_step, 0.0F, 50.0F, door_panel_object),
    PANEL(door_panel_step, 0.0F, 100.0F, door_panel_object),
    PANEL(door_panel_step, 0.0F, 150.0F, door_panel_object),
    NO_PANEL,
};
static struct task_spec door_b_panels[3] = {
    PANEL(door_b_panel_step, -50.0F, 0.0F, door_b_panel_object),
    PANEL(door_b_panel_step, 0.0F, 0.0F, door_b_panel_object),
    NO_PANEL,
};

static void door_destroy(struct task *task)
{
    task_kill(task);
}

/* on a frame's first step, draw the task's object at its position plus
 * work->scl, turned by ang.y */
static void door_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!frame_step) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->scl);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

/* set the door's mode while player 0 is within DOOR_WATCH. within
 * OPEN_NEAR, player 0 opens the door. farther, the door waits while player
 * 0 comes at it within a quarter turn of its way in. otherwise it shuts for
 * good */
static void door_set_mode(struct task_work *work)
{
    Float distance = njDistanceP2P(&work->pos, &player_work[0]->pos);

    if (distance < DOOR_WATCH) {
        if (distance < OPEN_NEAR(work)) {
            work->mode = DOOR_OPEN;
        } else if (angle_distance(
                       work->ang.y - (Angle)(SIDE(work) * ANGLE_PER_DEGREE),
                       (Angle)(atan2f(work->pos.x - player_work[0]->pos.x,
                                      work->pos.z - player_work[0]->pos.z) *
                               ANGLE_PER_RADIAN)) > 0x4000) {
            work->mode = DOOR_SHUT;
        } else {
            work->mode = DOOR_WAIT;
        }
    }
}

static void door_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *door = task->parent->work;

    if (edit_mode) {
        VARS(work)->object = door_panel_object;
        door_draw(task);
    } else {
        switch (work->mode) {
        case PANEL_INIT:
            work->mode = PANEL_RUN;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            VARS(work)->object = door_panel_object;
            task->destructor = door_destroy;
            task->draw = door_draw;
            break;
        case PANEL_RUN:
            RISE(work) = RISE(door);
            door_draw(task);
            break;
        }
    }
}

void door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, DOOR_RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        VARS(work)->object = door_panel_object;
        door_draw(task);
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        work->mode = DOOR_WAIT;
        OPEN_NEAR(work) = DOOR_NEAR;
        RISE(work) = 0.0F;
        SIDE(work) = DOOR_SIDE;
        task->destructor = door_destroy;
        task_create_children(door_panels, task);
        collision_init(task, door_shape, COLLISION_OBJECT, 1, 1);
        task->work->collision->radius = DOOR_REACH;
        break;
    case DOOR_WAIT:
        door_set_mode(work);
        RISE(work) -= RISE_SPEED;
        if (RISE(work) < 0.0F) {
            RISE(work) = 0.0F;
        }
        task_step_children(task);
        break;
    case DOOR_OPEN:
        door_set_mode(work);
        if ((RISE(work) += RISE_SPEED) > DOOR_TOP) {
            RISE(work) = DOOR_TOP;
        }
        task_step_children(task);
        break;
    case DOOR_SHUT:
        RISE(work) -= RISE_SPEED;
        if (RISE(work) < 0.0F) {
            RISE(work) = 0.0F;
        }
        task_step_children(task);
        collision_register(work);
        break;
    }
}

static void door_b_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *door = task->parent->work;

    if (edit_mode) {
        VARS(work)->object = door_b_panel_object;
        door_draw(task);
    } else {
        switch (work->mode) {
        case PANEL_INIT:
            work->mode = PANEL_RUN;
            work->scl.x = work->scl.y = work->scl.z = 0.0F;
            VARS(work)->object = door_b_panel_object;
            task->destructor = door_destroy;
            task->draw = door_draw;
            break;
        case PANEL_RUN:
            RISE(work) = RISE(door);
            door_draw(task);
            break;
        }
    }
}

void door_b_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, DOOR_RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        VARS(work)->object = door_b_panel_object;
        door_draw(task);
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        work->mode = DOOR_WAIT;
        OPEN_NEAR(work) = DOOR_B_NEAR;
        RISE(work) = 0.0F;
        SIDE(work) = DOOR_B_SIDE;
        task->destructor = door_destroy;
        collision_init(task, door_b_shape, COLLISION_OBJECT, 1, 1);
        task->work->collision->radius = DOOR_B_REACH;
        task_create_children(door_b_panels, task);
        break;
    case DOOR_WAIT:
        door_set_mode(work);
        RISE(work) -= RISE_SPEED;
        if (RISE(work) < 0.0F) {
            RISE(work) = 0.0F;
        }
        task_step_children(task);
        break;
    case DOOR_OPEN:
        door_set_mode(work);
        if ((RISE(work) += RISE_SPEED) > DOOR_B_TOP) {
            RISE(work) = DOOR_B_TOP;
        }
        task_step_children(task);
        break;
    case DOOR_SHUT:
        RISE(work) -= RISE_SPEED;
        if (RISE(work) < 0.0F) {
            RISE(work) = 0.0F;
        }
        task_step_children(task);
        collision_register(work);
        break;
    }
}

/* on a frame's first step, draw the arch and its two leaves. the leaves are
 * the arch's child and that child's sibling, which the hinge turns opposite
 * ways */
static void double_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 act = stage_get_stage_act() & 0xff;
    /* @hack spelling: draw_model through draw, which keeps njPushMatrix in
     * R10 from 0x0c904772 and the nodes in R9 from 0x0c90477c; called by
     * name, the two trade, and 11 of 197 words differ */
    void (*draw)(NJS_MODEL *model);
    NJS_OBJECT *object;
    NJS_OBJECT *child;
    NJS_OBJECT *sibling;

    if (frame_step) {
        return;
    }
    if (act == MIRRORS_ACT) {
        NJS_OBJECT *node = VARS(work)->object;
        NJS_MATERIAL *mat;
        int i;

        node->model->mats[0].attrflags |= NJD_FLAG_IGNORE_LIGHT;
        node = node->child;
        for (mat = node->model->mats, i = 0; i < LEAF_MATERIALS; i++, mat++) {
            mat->attrflags |= NJD_FLAG_IGNORE_LIGHT;
        }
        node = node->sibling;
        for (mat = node->model->mats, i = 0; i < LEAF_MATERIALS; i++, mat++) {
            mat->attrflags |= NJD_FLAG_IGNORE_LIGHT;
        }
    }
    texture_set_stage();
    /* @hack redundant: a never-run call of njPushMatrix, which, as in draw,
     * keeps njPushMatrix in R10 and the nodes in R9, and ang.y's load at
     * 0x0c904784 in R2, not R3; without it, 14 of 197 words differ */
    if (0) {
        njPushMatrix(NULL);
    }
    njPushMatrix(NULL);
    object = VARS(work)->object;
    njTranslateV(NULL, &work->pos);
    /* @hack redundant: a goto to the next statement, which keeps ang.y's load
     * at 0x0c904784 in R2; without it, R3, and 3 of 197 words differ */
    goto turn;
turn:
    TURN(njRotateY, work->ang.y);
    /* @hack redundant: a never-run call of njTranslateV, which keeps
     * njTranslateV in R12 from 0x0c90477a and njPushMatrix in R10, which
     * otherwise trade; without it, 12 of 197 words differ */
    if (0) {
        njTranslateV(NULL, &work->pos);
    }
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    draw = draw_model;
    draw(object->model);
    njPushMatrix(NULL);
    child = object->child;
    njTranslateV(NULL, (NJS_VECTOR *)child->pos);
    TURN(njRotateY, -VARS(work)->hinge);
    draw(child->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    sibling = child->sibling;
    njTranslateV(NULL, (NJS_VECTOR *)sibling->pos);
    TURN(njRotateY, VARS(work)->hinge);
    draw(sibling->model);
    njPopMatrix(1);
    njPopMatrix(1);
    if (act == MIRRORS_ACT) {
        NJS_OBJECT *node = VARS(work)->object;
        NJS_MATERIAL *mat;
        int i;

        node->model->mats[0].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        node = node->child;
        for (mat = node->model->mats, i = 0; i < LEAF_MATERIALS; i++, mat++) {
            mat->attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        }
        node = node->sibling;
        for (mat = node->model->mats, i = 0; i < LEAF_MATERIALS; i++, mat++) {
            mat->attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        }
    }
}

void double_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, DOUBLE_DOOR_RANGE)) {
        return;
    }
    if (edit_mode) {
        VARS(work)->object = double_door_object;
        double_door_draw(task);
    } else {
        switch (work->mode) {
        case DOUBLE_DOOR_INIT:
            work->mode = DOUBLE_DOOR_SHUT;
            work->scl.y = work->scl.z = 0.0F;
            if (work->scl.x > 0.0F) {
                work->submode = KEY_SWITCHES;
            } else if (work->scl.x == 0.0F) {
                work->submode = KEY_NEAR;
            } else {
                work->mode = DOUBLE_DOOR_LOCKED;
            }
            VARS(work)->object = double_door_object;
            task->destructor = door_destroy;
            task->draw = double_door_draw;
            collision_init(task, double_door_shapes, COLLISION_OBJECT, 1, 4);
            work->collision->radius = DOUBLE_DOOR_REACH;
            work->collision->shapes[1].flags |= SHAPE_OFF;
            work->collision->shapes[2].flags |= SHAPE_OFF;
            work->collision->shapes[3].flags |= SHAPE_OFF;
            break;
        case DOUBLE_DOOR_SHUT:
            switch (work->submode) {
            case KEY_SWITCHES:
                if (!switch_is_on(0)) {
                    break;
                }
                if (!switch_is_on(1)) {
                    break;
                }
                if (switch_is_on(2)) {
                    work->mode = DOUBLE_DOOR_OPEN;
                    work->collision->shapes[0].flags |= SHAPE_OFF;
                    work->collision->shapes[1].flags &= ~SHAPE_OFF;
                    work->collision->shapes[2].flags &= ~SHAPE_OFF;
                    work->collision->shapes[3].flags &= ~SHAPE_OFF;
                }
                break;
            case KEY_NEAR:
                if (njDistanceP2P(&work->pos, &player_work[0]->pos) <
                    DOUBLE_DOOR_NEAR) {
                    work->mode = DOUBLE_DOOR_OPEN;
                    work->collision->shapes[0].flags |= SHAPE_OFF;
                    work->collision->shapes[1].flags &= ~SHAPE_OFF;
                    work->collision->shapes[2].flags &= ~SHAPE_OFF;
                    work->collision->shapes[3].flags &= ~SHAPE_OFF;
                }
                break;
            default:
                work->mode = DOUBLE_DOOR_LOCKED;
                break;
            }
            VARS(work)->hinge =
                angle_approach(VARS(work)->hinge, 0, LEAF_SPEED);
            collision_register(work);
            double_door_draw(task);
            break;
        case DOUBLE_DOOR_OPEN:
            if (work->submode == KEY_NEAR) {
                if (njDistanceP2P(&work->pos, &player_work[0]->pos) >
                    DOUBLE_DOOR_NEAR) {
                    work->mode = DOUBLE_DOOR_SHUT;
                    work->collision->shapes[0].flags &= ~SHAPE_OFF;
                    work->collision->shapes[1].flags |= SHAPE_OFF;
                    work->collision->shapes[2].flags |= SHAPE_OFF;
                    work->collision->shapes[3].flags |= SHAPE_OFF;
                }
            }
            VARS(work)->hinge =
                angle_approach(VARS(work)->hinge, LEAF_OPEN, LEAF_SPEED);
            collision_register(work);
            double_door_draw(task);
            break;
        case DOUBLE_DOOR_LOCKED:
            VARS(work)->hinge =
                angle_approach(VARS(work)->hinge, 0, LEAF_SPEED);
            collision_register(work);
            double_door_draw(task);
            break;
        }
    }
}
