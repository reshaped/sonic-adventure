/* the gate the object list names O HIDDEN GATE: the way into the Final Egg.
 * touching one of its six switches flips it and the switches beside it, and
 * the gate opens while all six are on */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/hidden_gate.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: declared an array, not the one NJS_ACTION it is, so SHC pools
 * its address at 0x0c90ab0c and reads motion through it, MOV.L @(4,R4),R3 at
 * 0x0c90aa84; declared as one, 47 of hidden_gate_update's 217 words and a pool
 * word differ */
extern NJS_ACTION hidden_gate_action[];
extern NJS_OBJECT hidden_gate_shut_object;
extern NJS_OBJECT hidden_gate_open_object;
/* the switches' tree. the root's children are the posts, and each post's
 * child is where its switch stands */
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address once, at 0x0c90ad84, and reads pos and child through it, ADD
 * #8,R13 at 0x0c90ad06 and MOV.L @(44,R2),R3 at 0x0c90ad1c; declared as one,
 * 40 of hidden_gate_draw's 283 words and 11 words of its pool differ */
extern NJS_OBJECT hidden_gate_switches_object[];
extern NJS_MODEL hidden_gate_switch_model;

#define SWITCHES 6

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 600 */
#define RANGE (600.0F * 600.0F)
/* the collision world tests the gate only while a player is this near. init
 * gives the gate's collision record this radius too */
#define NEAR 150.0F

#define GATE_FLAGS (WORLD_MEDIUM | WORLD_MOVES)

/* the action frames the gate moves through each step */
#define FRAME_SPEED 0.2F

#define SE_ALL_ON 584
#define SE_OPEN 705

enum hidden_gate_mode { HIDDEN_GATE_INIT, HIDDEN_GATE_RUN };

/* the gate's states, in work->submode */
enum hidden_gate_state { GATE_OPENING, GATE_OPEN, GATE_SHUTTING, GATE_SHUT };

struct hidden_gate_vars {
    NJS_OBJECT *object; /* the gate's copy in the collision world */
    Uint8 unk_04[4];
    Float frame;
};

#define VARS(work) ((struct hidden_gate_vars *)&(work)->vars)
/* TRUE in work->tick if a switch was touched at the last step */
#define TOUCHED(work) ((work)->tick)

static void hidden_gate_init(struct task *task);
static void hidden_gate_reset(struct task *task);
static void hidden_gate_add_to_world(struct task *task, NJS_OBJECT *from);
static void hidden_gate_update(struct task *task);
static void hidden_gate_draw(struct task *task);
static void hidden_gate_destroy(struct task *task);

/* a cylinder at each switch, which reset places */
static struct collision_shape hidden_gate_shapes[SWITCHES] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     1.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     1.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     1.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     1.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     1.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     5.0F,
     1.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static Uint8 hidden_gate_start_on[SWITCHES] = {0, 1, 0, 0, 1, 0};
static Uint8 hidden_gate_on[SWITCHES];

void hidden_gate_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case HIDDEN_GATE_INIT:
            hidden_gate_init(task);
            break;
        case HIDDEN_GATE_RUN:
            hidden_gate_draw(task);
            hidden_gate_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case HIDDEN_GATE_INIT:
            hidden_gate_init(task);
            break;
        case HIDDEN_GATE_RUN:
            hidden_gate_destroy(task);
            hidden_gate_reset(task);
            hidden_gate_draw(task);
            hidden_gate_update(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            break;
        }
    }
}

static void hidden_gate_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = hidden_gate_draw;
    task->destructor = hidden_gate_destroy;
    VARS(work)->frame = 0.0F;
    collision_init(task, hidden_gate_shapes, COLLISION_OBJECT, 1, SWITCHES);
    work->collision->radius = NEAR;
    hidden_gate_reset(task);
    work->mode = HIDDEN_GATE_RUN;
}

static void hidden_gate_reset(struct task *task)
{
    struct task_force *force = task->force;
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct collision_shape *shape;
    NJS_OBJECT *post;
    Float x;
    Float y;
    Float z;
    int i;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    hidden_gate_add_to_world(task, &hidden_gate_shut_object);
    x = hidden_gate_switches_object->pos[0];
    y = hidden_gate_switches_object->pos[1];
    z = hidden_gate_switches_object->pos[2];
    post = hidden_gate_switches_object->child;
    shape = collision->shapes;
    for (i = 0; i < SWITCHES; i++) {
        shape->center.x = x + post->pos[0] + post->child->pos[0];
        shape->center.y = y + post->pos[1] + post->child->pos[1];
        shape->center.z = z + post->pos[2] + post->child->pos[2];
        post = post->sibling;
        shape++;
    }
    for (i = 0; i < SWITCHES; i++) {
        hidden_gate_on[i] = hidden_gate_start_on[i];
    }
    TOUCHED(work) = FALSE;
    work->submode = GATE_SHUT;
}

/* put a copy of from in the collision world, at the gate's position and y
 * angle, as VARS(work)->object. a caller must first take any old one out and
 * free it */
static void hidden_gate_add_to_world(struct task *task, NJS_OBJECT *from)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    VARS(work)->object = object = world_object_alloc();
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(GATE_FLAGS, task, object);
}

static void hidden_gate_update(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    int character;
    int touched;
    int i;

    if (work->submode == GATE_OPENING) {
        VARS(work)->frame += FRAME_SPEED;
        if (VARS(work)->frame >= hidden_gate_action->motion->nbFrame - 1.0F) {
            VARS(work)->frame = hidden_gate_action->motion->nbFrame - 1.0F;
            world_remove(task, VARS(work)->object);
            world_object_free(VARS(work)->object);
            hidden_gate_add_to_world(task, &hidden_gate_open_object);
            character = get_character();
            if (character == CHARACTER_SONIC) {
                event_flag_set(FLAG_SONIC_MR_ENTRANCE_FINAL_EGG);
            } else if (character == CHARACTER_AMY) {
                event_flag_set(FLAG_AMY_MR_ENTRANCE_FINAL_EGG);
            }
            work->submode = GATE_OPEN;
        }
    } else if (work->submode == GATE_SHUTTING) {
        VARS(work)->frame -= FRAME_SPEED;
        if (VARS(work)->frame <= 0.0F) {
            VARS(work)->frame = 0.0F;
            world_remove(task, VARS(work)->object);
            world_object_free(VARS(work)->object);
            hidden_gate_add_to_world(task, &hidden_gate_shut_object);
            work->submode = GATE_SHUT;
        }
    }
    if (collision->hit != NULL) {
        /* flip only at the first step of a touch */
        if (!TOUCHED(work)) {
            touched = collision->own_shape;
            if (touched == 0) {
                hidden_gate_on[0] ^= 1;
                hidden_gate_on[1] ^= 1;
            } else if (touched == SWITCHES - 1) {
                hidden_gate_on[SWITCHES - 2] ^= 1;
                hidden_gate_on[SWITCHES - 1] ^= 1;
            } else {
                hidden_gate_on[touched - 1] ^= 1;
                hidden_gate_on[touched] ^= 1;
                hidden_gate_on[touched + 1] ^= 1;
            }
            for (i = 0; i < SWITCHES; i++) {
                if (!hidden_gate_on[i]) {
                    break;
                }
            }
            if (i == SWITCHES) {
                if (work->submode == GATE_SHUTTING ||
                    work->submode == GATE_SHUT) {
                    work->submode = GATE_OPENING;
                    sound_play(SE_ALL_ON, 0, 0, 0);
                    sound_play(SE_OPEN, 0, 0, 0);
                }
            } else if (work->submode == GATE_OPENING ||
                       work->submode == GATE_OPEN) {
                work->submode = GATE_SHUTTING;
            }
        }
        TOUCHED(work) = TRUE;
    } else {
        TOUCHED(work) = FALSE;
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    collision_register(work);
}

static void hidden_gate_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *post;
    Angle ang;
    int i;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njSetTexture(&mrobj_texlist);
    draw_action(hidden_gate_action, VARS(work)->frame);
    njTranslate(NULL, hidden_gate_switches_object->pos[0],
                hidden_gate_switches_object->pos[1],
                hidden_gate_switches_object->pos[2]);
    post = hidden_gate_switches_object->child;
    for (i = 0; i < SWITCHES; i++) {
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)post->pos);
        njTranslateV(NULL, (NJS_VECTOR *)post->child->pos);
        njSetTexture(&mrobj_texlist);
        if (hidden_gate_on[i]) {
            draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        } else {
            draw_set_material(-1.0F, -1.0F, -1.0F, -1.0F);
        }
        draw_model(&hidden_gate_switch_model);
        njPopMatrix(1);
        post = post->sibling;
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

static void hidden_gate_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}
