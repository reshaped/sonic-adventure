/* O Hall Gate five-part gate that opens once the switch on its fourth part is
 * on */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "object/common.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg10/hall_gate.h"
#include "stg10/hall_gate_motion.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL hall_gate_part_model_1;
extern NJS_MODEL hall_gate_part_model_2;
extern NJS_MODEL hall_gate_part_model_3;
extern NJS_MODEL hall_gate_part_model_4;
extern NJS_MODEL hall_gate_part_model_5;

#define FRAMES (hall_gate_action->motion->nbFrame)

#define SE_GATE_OPEN 301
#define SE_GATE_OPENED 303
#define VOICE_GATE 447

#define SWITCH_ID 1
#define PARTS 5 /* one at each of the action's nodes 1 to 5 */
#define SWITCH_PART 4 /* the node of the part that carries the switch */
#define SWITCH_RISE 6.85F /* the switch's height above its part */
/* the switch's scale x, which switch.c reads as its kind. the kind 3 is a
 * switch that a press turns on for good */
#define SWITCH_KIND_ON_ONLY 3.0F
/* the collision world tests a part while a player is within its model's radius
 * plus NEAR_MARGIN */
#define NEAR_MARGIN 20.0F

#define PART_FLAGS (WORLD_MEDIUM | WORLD_MOVES | WORLD_SOLID)
/* switch.c's flags for the plate, with WORLD_MOVES and WORLD_SOLID added */
#define PLATE_FLAGS (WORLD_SMALL | WORLD_MOVES | WORLD_NO_SLOPE | WORLD_SOLID)

/* the gate's own view of its task_work, which must keep task_work's fields at
 * their offsets */
struct hall_gate_work {
    Sint8 mode;
    Uint8 unk_01;
    Uint8 id;
    Uint8 unk_03;
    Sint16 flags;
    Uint8 unk_06[2];
    Float frame; /* the frame of hall_gate_action the gate shows */
    Uint8 unk_0c[8];
    struct angles ang;
    NJS_POINT3 pos;
    Uint8 unk_2c[12];
    struct collision_work *collision;
    struct event_work *event;
};

/* a part's own view of its task_work, which must keep task_work's fields at
 * their offsets */
struct hall_gate_part_work {
    Sint8 node; /* the action's node that the part follows */
    Uint8 unk_01[3];
    Sint16 flags;
    Uint8 unk_06[2];
    NJS_OBJECT *object; /* the part's object in the collision world */
    struct task *button; /* the switch the part carries, or null */
    /* the switch's own destructor, which hall_gate_switch_destroy calls */
    void (*button_destroy)(struct task *task);
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 last; /* pos at the end of the last hall_gate_part_move */
};

/* the switch's vars as switch.c lays them out, as far as its plate */
struct hall_gate_button_vars {
    Uint8 unk_00[8];
    NJS_OBJECT *plate;
};

#define PART(task) ((struct hall_gate_part_work *)(task)->work)
#define BUTTON(work) ((struct hall_gate_button_vars *)&(work)->vars)

enum hall_gate_mode { GATE_SHUT, GATE_OPENING, GATE_OPEN };

static void hall_gate_run(struct task *task);
static void hall_gate_draw(struct task *task);
static void hall_gate_destroy(struct task *task);
static void hall_gate_part_start(struct task *task, Sint8 node);
static void hall_gate_switch_part_step(struct task *task);
static void hall_gate_part_step(struct task *task);
static void hall_gate_part_destroy(struct task *task);
static void hall_gate_pose(struct task *task);
static void hall_gate_part_follow(struct task *task);
static NJS_OBJECT *hall_gate_part_add(struct task *task, NJS_MODEL *model,
                                      Sint32 flags);
static void hall_gate_part_move(struct task *task);
static void hall_gate_switch_create(struct task *task);
static void hall_gate_switch_step(struct task *task);
static void hall_gate_switch_destroy(struct task *task);

/* each part's model, by node. node 0 has no part */
static NJS_MODEL *hall_gate_part_models[PARTS + 1] = {
    NULL,
    &hall_gate_part_model_1,
    &hall_gate_part_model_2,
    &hall_gate_part_model_3,
    &hall_gate_part_model_4,
    &hall_gate_part_model_5,
};

/* the switch part's one shape: a flat disc centered on its position */
static struct collision_shape hall_gate_shape = {
    SHAPE_CYLINDER | SHAPE_NO_PUSH,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    48.0F,
    4.5F,
    0.0F,
    {0},
    0,
    NULL,
};

void hall_gate_step(struct task *task)
{
    struct hall_gate_work *work = (struct hall_gate_work *)task->work;
    int i;
    struct task *child;

    if (!WORK_SAME_PLACE(struct hall_gate_work, pos) ||
        !WORK_SAME_PLACE(struct hall_gate_work, id) ||
        !WORK_SAME_PLACE(struct hall_gate_work, collision) ||
        !WORK_SAME_PLACE(struct hall_gate_work, event) ||
        !WORK_SAME_PLACE(struct hall_gate_part_work, pos) ||
        !WORK_SAME_PLACE(struct hall_gate_part_work, flags)) {
        return;
    }
    if (placement_used(task)) {
        work->frame = FRAMES - 1.0F;
        work->mode = GATE_OPEN;
        switch_set(SWITCH_ID, TRUE);
    } else {
        work->frame = 0.0F;
        work->mode = GATE_SHUT;
        switch_set(SWITCH_ID, FALSE);
    }
    hall_gate_pose(task);
    for (i = 1; i < PARTS + 1; i++) {
        /* @hack spelling: child stored and tested in one expression. SHC then
         * tests R0 and copies it to R4 in the branch's delay slot (TST R0,R0
         * and BT/S at 0x0c9135bc). as two statements, 15 of hall_gate_step's
         * 159 words differ */
        if ((child = task_create_child(TASK_WORK | TASK_FORCE,
                                       hall_gate_part_step, task)) != NULL) {
            hall_gate_part_start(child, i);
        }
    }
    task->step = hall_gate_run;
    task->draw = hall_gate_draw;
    task->destructor = hall_gate_destroy;
}

/* the gate skips the range check while it opens, so the check can't destroy it
 * before placement_set_used */
static void hall_gate_run(struct task *task)
{
    struct hall_gate_work *work = (struct hall_gate_work *)task->work;

    if (work->mode == GATE_OPENING) {
        work->frame += 1.0F;
        if (work->frame >= FRAMES) {
            sound_play(SE_GATE_OPENED, 0, 0, 0);
            work->frame = FRAMES - 1.0F;
            work->mode++;
            placement_set_used(task);
        }
    } else if (placement_try_leave(task)) {
        return;
    }
    hall_gate_pose(task);
    task_step_children(task);
    hall_gate_draw(task);
}

static void hall_gate_draw(struct task *task)
{
    struct hall_gate_work *work = (struct hall_gate_work *)task->work;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_action(hall_gate_action, work->frame);
        njPopMatrix(1);
    }
}

static void hall_gate_destroy(struct task *task)
{
}

static void hall_gate_part_start(struct task *task, Sint8 node)
{
    struct hall_gate_part_work *work = PART(task);

    if (!WORK_SAME_PLACE(struct hall_gate_work, pos) ||
        !WORK_SAME_PLACE(struct hall_gate_work, id) ||
        !WORK_SAME_PLACE(struct hall_gate_work, collision) ||
        !WORK_SAME_PLACE(struct hall_gate_work, event) ||
        !WORK_SAME_PLACE(struct hall_gate_part_work, pos) ||
        !WORK_SAME_PLACE(struct hall_gate_part_work, flags)) {
        return;
    }
    work->node = node;
    work->object =
        hall_gate_part_add(task, hall_gate_part_models[node], PART_FLAGS);
    if (node == SWITCH_PART) {
        task->step = hall_gate_switch_part_step;
        hall_gate_switch_create(task);
        collision_init(task, &hall_gate_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = NULL;
    task->destructor = hall_gate_part_destroy;
}

static void hall_gate_switch_part_step(struct task *task)
{
    struct hall_gate_part_work *work = PART(task);
    /* @hack spelling: the calls take task through a copy, part, which SHC
     * makes in R13 (MOV R4,R13 at 0x0c91388e). through task, 43 of
     * hall_gate_switch_part_step's 46 words differ */
    struct task *part = task;

    hall_gate_part_follow(part);
    hall_gate_part_move(part);
    collision_register((struct task_work *)work);
    if (work->button != NULL) {
        struct hall_gate_work *gate =
            (struct hall_gate_work *)task->parent->work;

        task_step_children(task);
        if (gate->mode == GATE_SHUT && switch_is_on(SWITCH_ID)) {
            stream_request_voice(VOICE_GATE);
            sound_play(SE_GATE_OPEN, 0, 0, 0);
            gate->mode = GATE_OPENING;
        }
    }
}

static void hall_gate_part_step(struct task *task)
{
    hall_gate_part_follow(task);
    hall_gate_part_move(task);
}

static void hall_gate_part_destroy(struct task *task)
{
    NJS_OBJECT *object = PART(task)->object;

    world_remove(task, object);
    world_object_free(object);
}

static void hall_gate_pose(struct task *task)
{
    struct hall_gate_work *work = (struct hall_gate_work *)task->work;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    pose_action(NULL, hall_gate_action, work->frame, PARTS, NULL);
    njPopMatrix(1);
}

/* move the part to its node in the last pose. the gate must pose before its
 * parts step */
static void hall_gate_part_follow(struct task *task)
{
    struct hall_gate_part_work *work = PART(task);

    pose_get_matrix(work->node, &scratch_matrix);
    njGetTranslation(&scratch_matrix, &work->pos);
}

/* lend the part a world object of model, at the part's pos and angles. put it
 * in the world with flags and return it, or null when none is free */
static NJS_OBJECT *hall_gate_part_add(struct task *task, NJS_MODEL *model,
                                      Sint32 flags)
{
    NJS_OBJECT *object;

    /* @hack spelling: object stored and tested in one expression. SHC then
     * tests R0 and copies it to R14 in the branch's delay slot (TST R0,R0 and
     * BT/S at 0x0c9139b4). as two statements, 3 of hall_gate_part_add's 61
     * words differ */
    if ((object = world_object_alloc()) != NULL) {
        object->evalflags = NJD_EVAL_BREAK;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->pos[0] = task->work->pos.x;
        object->pos[1] = task->work->pos.y;
        object->pos[2] = task->work->pos.z;
        object->ang[0] = task->work->ang.x;
        object->ang[1] = task->work->ang.y;
        object->ang[2] = task->work->ang.z;
        object->model = model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(flags, task, object);
    }
    return object;
}

/* move the part's object to the part. both of the task's struct task_force
 * records take the part's move this step as their speed. that speed carries
 * the players standing on the part */
static void hall_gate_part_move(struct task *task)
{
    struct hall_gate_part_work *work = PART(task);
    NJS_OBJECT *object = work->object;
    struct task_force *force = task->force;

    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    force[0].speed = work->pos;
    njSubVector(&force[0].speed, &work->last);
    force[1].speed = force[0].speed;
    work->last = work->pos;
    if (player_in_range(&work->pos, object->model->r + NEAR_MARGIN)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

/* create the switch the part carries, and take over its destructor. the
 * switch's first step puts its plate in the world without WORLD_MOVES. the
 * plate goes in again with PLATE_FLAGS, since the part moves it */
static void hall_gate_switch_create(struct task *task)
{
    struct hall_gate_part_work *work = PART(task);
    struct task *button =
        task_create_child(TASK_WORK | TASK_FORCE, hall_gate_switch_step, task);
    struct task_work *button_work = button->work;
    NJS_OBJECT *plate;

    button_work->scl.x = SWITCH_KIND_ON_ONLY;
    button_work->scl.y = SWITCH_ID;
    button_work->pos = work->pos;
    button_work->ang = work->ang;
    switch_step(button);
    work->button = button;
    work->button_destroy = button->destructor;
    button->destructor = hall_gate_switch_destroy;
    plate = BUTTON(button->work)->plate;
    world_remove(button, plate);
    world_add(PLATE_FLAGS, button, plate);
}

static void hall_gate_switch_step(struct task *task)
{
    /* @hack spelling: plate read through task->work ahead of work. SHC then
     * keeps work in R2 and plate in R4, and saves R11 to R13 (0x0c913b8e to
     * 0x0c913ba4). read through work after it, 42 of hall_gate_switch_step's
     * 60 words differ */
    NJS_OBJECT *plate = BUTTON(task->work)->plate;
    struct task_work *work = task->work;
    struct hall_gate_part_work *part = PART(task->parent);
    struct task_force *force = task->force;

    work->pos = part->pos;
    work->pos.y += SWITCH_RISE;
    plate->pos[0] = work->pos.x;
    plate->pos[1] = work->pos.y;
    plate->pos[2] = work->pos.z;
    /* @bug logic: the switch's force records carry no player with the
     * plate. both take part->pos less part->last as their speed (njSubVector's
     * JSR at 0x0c913be8). hall_gate_part_move has already set part->last to
     * part->pos (BSR at 0x0c913890) before the part steps its switch (JSR at
     * 0x0c9138a6), so the speed is always 0 */
    force[0].speed = part->pos;
    njSubVector(&force[0].speed, &part->last);
    force[1].speed = force[0].speed;
    switch_step(task);
}

static void hall_gate_switch_destroy(struct task *task)
{
    struct hall_gate_part_work *part = PART(task->parent);

    part->button_destroy(task);
    part->button_destroy = NULL;
    part->button = NULL;
}
