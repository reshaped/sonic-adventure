/* O BRIDGE, a walkway on an arm that turns about a hub. each time switch 6
 * turns on, the bridge swings nearly a quarter turn to its other stop */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg12/bridge.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the walkway, the arm and the hub as drawn, then as the collision world
 * holds them */
extern NJS_MODEL shelter_bridge_model;
extern NJS_MODEL shelter_bridge_arm_model;
extern NJS_MODEL shelter_bridge_hub_model;
extern NJS_OBJECT shelter_bridge_object;
extern NJS_OBJECT shelter_bridge_arm_object;
extern NJS_OBJECT shelter_bridge_hub_object;

/* destroyed beyond 1500 units of the camera and player 0 */
#define RANGE (1500.0F * 1500.0F)

/* the switch that sends the bridge to its other stop. the bridge turns it off
 * once there */
#define SWITCH 6

/* where init puts the bridge, whatever position its placement gives */
#define BRIDGE_X 0.0F
#define BRIDGE_Y 68.0F
#define BRIDGE_Z (-1810.0F)

/* the walkway's reach along the arm at the first and second stops */
#define REACH_FIRST 39.0F
#define REACH_SECOND 41.0F
/* the steps the walkway takes to draw back or to slide out */
#define SLIDE_STEPS 120
/* the arm turns TURN_SPEED angle units a step for TURN_STEPS steps. the
 * division drops its fraction, so TURN_SPEED is 68. the arm thus falls 64
 * units short of a quarter turn */
#define TURN_STEPS 240
#define TURN_SPEED ((Float)(NJM_DEG_ANG(90) / TURN_STEPS))

/* collision world flags of the walkway and arm, which move, and of the hub */
#define MOVING_FLAGS (WORLD_SOLID | WORLD_MOVES | WORLD_TURNED)
#define HUB_FLAGS WORLD_SOLID

/* sound effects: a clank as the bridge starts and stops, and its motor while it
 * moves */
#define SE_CLANK 307
#define SE_BRIDGE_MOTOR 328

/* from each stop, the walkway draws back and the arm turns. the walkway then
 * slides out at the other stop. no code sets BRIDGE_SLIDE, in which the
 * walkway slides on without end */
enum shelter_bridge_mode {
    BRIDGE_INIT,
    BRIDGE_FIRST,
    BRIDGE_DRAW_BACK,
    BRIDGE_SLIDE_OUT,
    BRIDGE_TURN,
    BRIDGE_SECOND,
    BRIDGE_DRAW_BACK_HOME,
    BRIDGE_SLIDE_OUT_HOME,
    BRIDGE_TURN_HOME,
    BRIDGE_SLIDE
};

enum shelter_bridge_part_mode { PART_INIT, PART_RUN };

struct shelter_bridge_vars {
    Sint8 reverse; /* a negative scale x turns the bridge the other way */
    Uint8 unk_01[7];
    NJS_OBJECT *object; /* the part this task added to the collision world */
};

#define VARS(work) ((struct shelter_bridge_vars *)&(work)->vars)
#define FORCE(task) ((struct task_force *)(task)->force)
#define TURN_STEP(work) ((work)->scl.x)
#define SLIDE_STEP(work) ((work)->scl.y)
#define REACH(work) ((work)->scl.z)

static void shelter_bridge_init(struct task *task);
static void shelter_bridge_draw(struct task *task);
static void shelter_bridge_destroy(struct task *task);
static void shelter_bridge_turn(struct task *task);
static void shelter_bridge_slide(struct task *task);
static void shelter_bridge_arm_step(struct task *task);
static void shelter_bridge_arm_init(struct task *task);
static void shelter_bridge_arm_follow(struct task *task);
static void shelter_bridge_arm_draw(struct task *task);
static void shelter_bridge_part_destroy(struct task *task);
static void shelter_bridge_hub_step(struct task *task);
static void shelter_bridge_hub_init(struct task *task);
static void shelter_bridge_hub_draw(struct task *task);

void shelter_bridge_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BRIDGE_INIT:
            shelter_bridge_init(task);
            break;
        case BRIDGE_FIRST:
            shelter_bridge_draw(task);
            shelter_bridge_turn(task);
            if (switch_is_on(SWITCH)) {
                TURN_STEP(work) = 0.0F;
                SLIDE_STEP(work) = -REACH_FIRST / SLIDE_STEPS;
                TASK_TIMER(work) = SLIDE_STEPS;
                work->mode = BRIDGE_DRAW_BACK;
                sound_play(SE_CLANK, 0, 0, 0);
            }
            break;
        case BRIDGE_DRAW_BACK:
            shelter_bridge_draw(task);
            shelter_bridge_slide(task);
            if (--TASK_TIMER(work) <= 0) {
                TASK_TIMER(work) = TURN_STEPS;
                /* NOLINTBEGIN(bugprone-integer-division) */
                TURN_STEP(work) =
                    VARS(work)->reverse ? -TURN_SPEED : TURN_SPEED;
                /* NOLINTEND(bugprone-integer-division) */
                work->mode = BRIDGE_TURN;
            }
            sound_play_for(SE_BRIDGE_MOTOR, (int)task, 1, 0, 2);
            break;
        case BRIDGE_TURN:
            shelter_bridge_draw(task);
            shelter_bridge_turn(task);
            if (--TASK_TIMER(work) <= 0) {
                TURN_STEP(work) = 0.0F;
                SLIDE_STEP(work) = REACH_SECOND / SLIDE_STEPS;
                TASK_TIMER(work) = SLIDE_STEPS;
                work->mode = BRIDGE_SLIDE_OUT;
            }
            sound_play_for(SE_BRIDGE_MOTOR, (int)task, 1, 0, 2);
            break;
        case BRIDGE_SLIDE_OUT:
            shelter_bridge_draw(task);
            shelter_bridge_slide(task);
            if (--TASK_TIMER(work) <= 0) {
                TURN_STEP(work) = 0.0F;
                SLIDE_STEP(work) = 0.0F;
                switch_set(SWITCH, FALSE);
                work->mode = BRIDGE_SECOND;
                sound_play(SE_CLANK, 0, 0, 0);
            }
            sound_play_for(SE_BRIDGE_MOTOR, (int)task, 1, 0, 2);
            break;
        case BRIDGE_SECOND:
            shelter_bridge_draw(task);
            shelter_bridge_turn(task);
            if (switch_is_on(SWITCH)) {
                TURN_STEP(work) = 0.0F;
                SLIDE_STEP(work) = -REACH_SECOND / SLIDE_STEPS;
                TASK_TIMER(work) = SLIDE_STEPS;
                work->mode = BRIDGE_DRAW_BACK_HOME;
                sound_play(SE_CLANK, 0, 0, 0);
            }
            break;
        case BRIDGE_DRAW_BACK_HOME:
            shelter_bridge_draw(task);
            shelter_bridge_slide(task);
            if (--TASK_TIMER(work) <= 0) {
                TASK_TIMER(work) = TURN_STEPS;
                /* NOLINTBEGIN(bugprone-integer-division) */
                TURN_STEP(work) =
                    VARS(work)->reverse ? TURN_SPEED : -TURN_SPEED;
                /* NOLINTEND(bugprone-integer-division) */
                work->mode = BRIDGE_TURN_HOME;
            }
            sound_play_for(SE_BRIDGE_MOTOR, (int)task, 1, 0, 2);
            break;
        case BRIDGE_TURN_HOME:
            shelter_bridge_draw(task);
            shelter_bridge_turn(task);
            if (--TASK_TIMER(work) <= 0) {
                TURN_STEP(work) = 0.0F;
                SLIDE_STEP(work) = REACH_FIRST / SLIDE_STEPS;
                TASK_TIMER(work) = SLIDE_STEPS;
                work->mode = BRIDGE_SLIDE_OUT_HOME;
            }
            sound_play_for(SE_BRIDGE_MOTOR, (int)task, 1, 0, 2);
            break;
        case BRIDGE_SLIDE_OUT_HOME:
            shelter_bridge_draw(task);
            shelter_bridge_slide(task);
            if (--TASK_TIMER(work) <= 0) {
                TURN_STEP(work) = 0.0F;
                SLIDE_STEP(work) = 0.0F;
                switch_set(SWITCH, FALSE);
                work->mode = BRIDGE_FIRST;
                sound_play(SE_CLANK, 0, 0, 0);
            }
            sound_play_for(SE_BRIDGE_MOTOR, (int)task, 1, 0, 2);
            break;
        case BRIDGE_SLIDE:
            shelter_bridge_draw(task);
            shelter_bridge_slide(task);
            break;
        default:
            /* @hack redundant: a call of sound_play that never runs. it emits
             * no code, but it keeps sound_play's address in R8 for the step's
             * calls, at 0x0c90249c, and sound_play_for's in R10, at
             * 0x0c90249e. without it, sound_play_for's address takes R11,
             * SLIDE_STEPS takes R8 and each call loads sound_play's address
             * itself. the function then runs 32 bytes long, and 254 of 328
             * words differ */
            if (FALSE) {
                sound_play(SE_BRIDGE_MOTOR, SE_CLANK, 0, 0);
            }
        }
    } else {
        switch (work->mode) {
        case BRIDGE_INIT:
            shelter_bridge_init(task);
            break;
        case BRIDGE_FIRST:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ROT MODE");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> No Use");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> No Use");
            shelter_bridge_draw(task);
            shelter_bridge_turn(task);
            break;
        }
    }
    task_step_children(task);
}

static void shelter_bridge_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from;
    NJS_OBJECT *object;
    Angle ang;
    NJS_POINT3 offset;
    NJS_POINT3 point;

    task->draw = shelter_bridge_draw;
    task->destructor = shelter_bridge_destroy;
    if (work->scl.x < 0.0F) {
        VARS(work)->reverse = 1;
    } else {
        VARS(work)->reverse = 0;
    }
    work->pos.x = BRIDGE_X;
    work->pos.y = BRIDGE_Y;
    work->pos.z = BRIDGE_Z;
    TURN_STEP(work) = 0.0F;
    SLIDE_STEP(work) = 0.0F;
    REACH(work) = REACH_FIRST;
    VARS(work)->object = object = world_object_alloc();
    from = &shelter_bridge_object;
    offset.x = 0.0F;
    offset.y = 0.0F;
    offset.z = REACH(work);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &offset, &point);
    njPopMatrix(1);
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    object->pos[0] = point.x;
    object->pos[1] = point.y;
    object->pos[2] = point.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(MOVING_FLAGS, task, object);
    work->mode = BRIDGE_FIRST;
    task_create_child(TASK_WORK | TASK_FORCE, shelter_bridge_arm_step, task);
    task_create_child(TASK_WORK | TASK_FORCE, shelter_bridge_hub_step, task);
}

static void shelter_bridge_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njTranslate(NULL, 0.0F, 0.0F, REACH(work));
        draw_model(&shelter_bridge_model);
        njPopMatrix(1);
    }
}

static void shelter_bridge_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

/* turn the bridge by TURN_STEP(work), and the walkway's object in the collision
 * world with it. its force for player 0 turns a player 0 standing on the
 * walkway about the hub with it */
static void shelter_bridge_turn(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;
    NJS_POINT3 offset;
    NJS_POINT3 point;

    work->ang.y += (Angle)TURN_STEP(work);
    offset.x = 0.0F;
    offset.y = 0.0F;
    offset.z = REACH(work);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &offset, &point);
    njPopMatrix(1);
    object = VARS(work)->object;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = point.x;
    object->pos[1] = point.y;
    object->pos[2] = point.z;
    offset.x = player_work[0]->pos.x - work->pos.x;
    offset.y = 0.0F;
    offset.z = player_work[0]->pos.z - work->pos.z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, (Angle)TURN_STEP(work), ang);
    njCalcVector(NULL, &offset, &point);
    njPopMatrix(1);
    njSubVector(&point, &offset);
    FORCE(task)->speed = point;
    FORCE(task)->turn.y = -(Angle)TURN_STEP(work);
}

/* slide the walkway along the arm by SLIDE_STEP(work), and its object in the
 * collision world with it. its force for player 0 moves a player 0 standing on
 * the walkway by the same distance */
static void shelter_bridge_slide(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;
    NJS_POINT3 offset;
    NJS_POINT3 point;

    REACH(work) += SLIDE_STEP(work);
    offset.x = 0.0F;
    offset.y = 0.0F;
    offset.z = REACH(work);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcPoint(NULL, &offset, &point);
    njPopMatrix(1);
    object = VARS(work)->object;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = point.x;
    object->pos[1] = point.y;
    object->pos[2] = point.z;
    offset.z = SLIDE_STEP(work);
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &offset, &point);
    njPopMatrix(1);
    FORCE(task)->speed = point;
    FORCE(task)->turn.y = 0;
}

static void shelter_bridge_arm_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    switch (work->mode) {
    case PART_INIT:
        shelter_bridge_arm_init(task);
        break;
    case PART_RUN:
        shelter_bridge_arm_draw(task);
        shelter_bridge_arm_follow(task);
        break;
    }
}

static void shelter_bridge_arm_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    work->pos = task->parent->work->pos;
    work->ang = task->parent->work->ang;
    task->draw = shelter_bridge_arm_draw;
    task->destructor = shelter_bridge_part_destroy;
    VARS(work)->object = object = world_object_alloc();
    object->evalflags = shelter_bridge_arm_object.evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    object->pos[0] = work->pos.x + shelter_bridge_arm_object.pos[0];
    object->pos[1] = work->pos.y + shelter_bridge_arm_object.pos[1];
    object->pos[2] = work->pos.z + shelter_bridge_arm_object.pos[2];
    object->model = shelter_bridge_arm_object.model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(MOVING_FLAGS, task, object);
    work->mode = PART_RUN;
}

/* turn the arm, and its object in the collision world, with the bridge. its
 * force for player 0 turns a player 0 standing on the arm about the hub with
 * it */
static void shelter_bridge_arm_follow(struct task *task)
{
    struct task_work *parent = task->parent->work;
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;
    NJS_POINT3 offset;
    NJS_POINT3 point;

    work->ang = parent->ang;
    work->scl = parent->scl;
    object = VARS(work)->object;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x + shelter_bridge_arm_object.pos[0];
    object->pos[1] = work->pos.y + shelter_bridge_arm_object.pos[1];
    object->pos[2] = work->pos.z + shelter_bridge_arm_object.pos[2];
    offset.x = player_work[0]->pos.x - work->pos.x;
    offset.y = 0.0F;
    offset.z = player_work[0]->pos.z - work->pos.z;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, (Angle)TURN_STEP(work), ang);
    njCalcVector(NULL, &offset, &point);
    njPopMatrix(1);
    njSubVector(&point, &offset);
    FORCE(task)->speed = point;
    FORCE(task)->turn.y = -(Angle)TURN_STEP(work);
}

static void shelter_bridge_arm_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(&shelter_bridge_arm_model);
        njPopMatrix(1);
    }
}

static void shelter_bridge_part_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

static void shelter_bridge_hub_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    switch (work->mode) {
    case PART_INIT:
        shelter_bridge_hub_init(task);
        break;
    case PART_RUN:
        shelter_bridge_hub_draw(task);
        break;
    }
}

static void shelter_bridge_hub_init(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    work->pos = task->parent->work->pos;
    work->ang = task->parent->work->ang;
    task->draw = shelter_bridge_hub_draw;
    task->destructor = shelter_bridge_part_destroy;
    VARS(work)->object = object = world_object_alloc();
    object->evalflags = shelter_bridge_hub_object.evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = work->ang.x;
    object->ang[1] = work->ang.y;
    object->ang[2] = work->ang.z;
    object->pos[0] = work->pos.x + shelter_bridge_hub_object.pos[0];
    object->pos[1] = work->pos.y + shelter_bridge_hub_object.pos[1];
    object->pos[2] = work->pos.z + shelter_bridge_hub_object.pos[2];
    object->model = shelter_bridge_hub_object.model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(HUB_FLAGS, task, object);
    work->mode = PART_RUN;
}

/* draw the hub without the bridge's turn */
static void shelter_bridge_hub_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        draw_model(&shelter_bridge_hub_model);
        njPopMatrix(1);
    }
}
