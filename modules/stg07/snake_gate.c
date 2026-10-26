/* the snake gate: HebiGate, a solid gate. it opens by playing its action
 * backward once player 0 nears it along x */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/snake_gate.h"
#include "system/task.h"
#include "system/texture.h"

/* snake_gate_object is the gate as the world tests it. the gate draws its
 * action, whose object is another model */
extern NJS_OBJECT snake_gate_object;
extern NJS_ACTION snake_gate_action;

struct snake_gate_vars {
    NJS_OBJECT *world; /* its copy of snake_gate_object in the world */
    Float frame; /* the action's frame */
    Sint32 state; /* a snake_gate_state */
};

#define VARS(work) ((struct snake_gate_vars *)&(work)->vars)

enum snake_gate_state { GATE_WAIT, GATE_MOVE, GATE_DONE };

/* the gate opens once player 0's x exceeds the gate's x less GATE_RANGE */
#define GATE_RANGE 400.0F
/* the world tests the gate only while player 0 or 1 is near it. near is
 * within GATE_REACH past the radius of snake_gate_object's model */
#define GATE_REACH 20.0F

/* the action plays from GATE_SHUT_FRAME down to GATE_OPEN_FRAME, a frame
 * each step */
#define GATE_SHUT_FRAME 99.0F
#define GATE_OPEN_FRAME 1.0F

#define SE_RUMBLE 199

/* no code reads this shape. it is here because the image holds it */
/* clang-format off */
static struct collision_shape snake_gate_shape = {
    SHAPE_RECTANGLE | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
    PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS), {0.0F, 50.0F, 0.0F},
    1.0F, 50.0F, 50.0F, {0}, 0, NULL,
};
/* clang-format on */

static void snake_gate_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
    task_destroy_children(task);
}

static void snake_gate_draw(struct task *task)
{
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &task->work->pos);
    draw_action(&snake_gate_action, VARS(task->work)->frame);
    njPopMatrix(1);
    /* @hack redundant: a return where the function returns anyway, which keeps
     * task at @R15 by ADD #-4,R15 and MOV.L R4,@R15 (0x0c9029ac,
     * 0x0c9029b2). without it, SHC pushes task with MOV.L R4,@-R15, and 26
     * of snake_gate_draw's 27 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void snake_gate_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *player = &player_work[0]->pos;
    NJS_OBJECT *world = VARS(work)->world;

    if (placement_try_leave(task)) {
        return;
    }
    if (player_in_range(&work->pos, world->model->r + GATE_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    task_step_children(task);
    switch (VARS(work)->state) {
    case GATE_WAIT:
        if (player->x > work->pos.x - GATE_RANGE) {
            VARS(work)->state++;
            sound_play_at_for(SE_RUMBLE, (int)work, 1, 96, work->pos.x,
                              work->pos.y, work->pos.z, 2);
        }
        break;
    case GATE_MOVE:
        VARS(work)->frame -= 1.0F;
        if (VARS(work)->frame < GATE_OPEN_FRAME) {
            VARS(work)->frame = GATE_OPEN_FRAME;
            VARS(work)->state++;
        }
        break;
    }
    snake_gate_draw(task);
}

void snake_gate_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    world = VARS(work)->world =
        object_add_world(&snake_gate_object, task, WORLD_SOLID);
    world->scl[0] = 1.0F;
    world->scl[1] = 1.0F;
    world->scl[2] = 1.0F;
    VARS(work)->frame = GATE_SHUT_FRAME;
    VARS(work)->state = GATE_WAIT;
    task->destructor = snake_gate_destroy;
    task->step = snake_gate_run;
    task->draw = snake_gate_draw;
}
