/* an event's claw, drawn from the five nodes each event defines */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "shared/claw.h"
#include "shared/claw_textures.h"
#include "system/task.h"
#include "system/util.h"

/* how far each arm turns from shut to open */
#define CLAW_OPEN_MAX 0x3000

struct claw_vars {
    Uint32 open; /* how far each arm has turned from shut */
};

#define VARS(work) ((struct claw_vars *)&(work)->vars)

/* the claw's TASK_MOTION record, which task_create zeroes */
struct claw_motion {
    Float steps; /* the steps a move lasts */
    NJS_VECTOR speed; /* added to its place each step of a move */
    Uint8 unk_10[24];
    Uint32 open_rate; /* how far its arms turn each step as it opens */
    Uint32 close_rate; /* how far its arms turn each step as it closes */
    Uint8 unk_30[4];
    Float timer; /* the steps its moves have run, which nothing resets */
    Uint8 unk_38[8];
};

#define CLAW(task) ((struct claw_motion *)(task)->motion)

/* the claw's nodes, by their place in event_claw_nodes */
enum claw_node {
    CLAW_SPRING,
    CLAW_BALL, /* under the spring */
    CLAW_HUB,
    CLAW_ARM_PLUS, /* the arm the draw turns on the hub by open about z */
    CLAW_ARM_MINUS, /* the arm it turns by -open */
    CLAW_NODES
};

/* each event that links claw.c defines these */
extern NJS_OBJECT *event_claw_nodes[CLAW_NODES];

static void event_claw_destroy(struct task *task)
{
}

static void event_claw_draw(struct task *task)
{
    njSetTexture(&event_claw_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &task->work->pos);
    njTranslate(NULL, event_claw_nodes[CLAW_SPRING]->pos[0],
                event_claw_nodes[CLAW_SPRING]->pos[1],
                event_claw_nodes[CLAW_SPRING]->pos[2]);
    TURN(njRotateZ, task->work->ang.z + event_claw_nodes[CLAW_SPRING]->ang[2]);
    TURN(njRotateX, task->work->ang.x + event_claw_nodes[CLAW_SPRING]->ang[0]);
    TURN(njRotateY, task->work->ang.y + event_claw_nodes[CLAW_SPRING]->ang[1]);
    njPushMatrix(NULL);
    njScale(NULL, 0.5F, 2.0F, 0.5F);
    draw_model(event_claw_nodes[CLAW_SPRING]->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, event_claw_nodes[CLAW_BALL]->pos[0],
                event_claw_nodes[CLAW_BALL]->pos[1],
                event_claw_nodes[CLAW_BALL]->pos[2]);
    TURN(njRotateZ, event_claw_nodes[CLAW_BALL]->ang[2]);
    TURN(njRotateX, event_claw_nodes[CLAW_BALL]->ang[0]);
    TURN(njRotateY, event_claw_nodes[CLAW_BALL]->ang[1]);
    njScale(NULL, 0.5F, 0.5F, 0.5F);
    draw_model(event_claw_nodes[CLAW_BALL]->model);
    njPopMatrix(1);
    /* the hub's model isn't drawn: its arms turn on it */
    njPushMatrix(NULL);
    njTranslate(NULL, event_claw_nodes[CLAW_HUB]->pos[0],
                event_claw_nodes[CLAW_HUB]->pos[1],
                event_claw_nodes[CLAW_HUB]->pos[2]);
    TURN(njRotateZ, event_claw_nodes[CLAW_HUB]->ang[2]);
    TURN(njRotateX, event_claw_nodes[CLAW_HUB]->ang[0]);
    TURN(njRotateY, event_claw_nodes[CLAW_HUB]->ang[1]);
    njScale(NULL, 0.5F, 0.5F, 0.5F);
    njPushMatrix(NULL);
    njTranslate(NULL, event_claw_nodes[CLAW_ARM_PLUS]->pos[0],
                event_claw_nodes[CLAW_ARM_PLUS]->pos[1],
                event_claw_nodes[CLAW_ARM_PLUS]->pos[2]);
    TURN(njRotateZ,
         event_claw_nodes[CLAW_ARM_PLUS]->ang[2] + VARS(task->work)->open);
    TURN(njRotateX, event_claw_nodes[CLAW_ARM_PLUS]->ang[0]);
    TURN(njRotateY, event_claw_nodes[CLAW_ARM_PLUS]->ang[1]);
    draw_model(event_claw_nodes[CLAW_ARM_PLUS]->model);
    njPopMatrix(1);
    njTranslate(NULL, event_claw_nodes[CLAW_ARM_MINUS]->pos[0],
                event_claw_nodes[CLAW_ARM_MINUS]->pos[1],
                event_claw_nodes[CLAW_ARM_MINUS]->pos[2]);
    TURN(njRotateZ,
         event_claw_nodes[CLAW_ARM_MINUS]->ang[2] - VARS(task->work)->open);
    TURN(njRotateX, event_claw_nodes[CLAW_ARM_MINUS]->ang[0]);
    TURN(njRotateY, event_claw_nodes[CLAW_ARM_MINUS]->ang[1]);
    draw_model(event_claw_nodes[CLAW_ARM_MINUS]->model);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void event_claw_step(struct task *task)
{
    struct task_work *work = task->work;
    struct claw_motion *motion = CLAW(task);

    switch (work->mode) {
    case CLAW_STILL:
        break;
    case CLAW_OPEN:
        VARS(work)->open += motion->open_rate;
        if (VARS(work)->open >= CLAW_OPEN_MAX) {
            VARS(work)->open = CLAW_OPEN_MAX;
            work->mode = CLAW_STILL;
        }
        break;
    case CLAW_CLOSE:
        VARS(work)->open -= motion->close_rate;
        VARS(work)->open &= 0xffff;
        /* @hack spelling: open <= 0, which for an unsigned open is open == 0,
         * keeps the image's MOV #0,R2 and CMP/HI R2,R0 (0x0cb8776c and
         * 0x0cb87772 in EV0008); without it, 47 of 73 words differ */
        if (VARS(work)->open <= 0) {
            VARS(work)->open = 0;
            work->mode = CLAW_STILL;
        }
        break;
    case CLAW_MOVE:
        work->pos.x += motion->speed.x;
        work->pos.y += motion->speed.y;
        work->pos.z += motion->speed.z;
        if (++motion->timer >= motion->steps) {
            work->mode = CLAW_STILL;
        }
        break;
    case CLAW_LOCKED:
        break;
    }
    event_claw_draw(task);
}

struct task *event_claw_create(Float x, Float y, Float z, Angle ang_x,
                               Angle ang_y, Angle ang_z)
{
    struct task *task =
        task_create(TASK_MOTION | TASK_WORK, 3, event_claw_step);

    if (task != NULL) {
        task->work->pos.x = x;
        task->work->pos.y = y;
        task->work->pos.z = z;
        task->work->ang.x = ang_x;
        task->work->ang.y = ang_y;
        task->work->ang.z = ang_z;
        task->work->timer = 0;
        VARS(task->work)->open = 0;
        CLAW(task)->open_rate = 0x20;
        CLAW(task)->close_rate = 0x200;
        task->draw = event_claw_draw;
        task->destructor = event_claw_destroy;
    }
    return task;
}

void event_claw_set_mode(struct task *task, enum claw_mode mode)
{
    if (task != NULL && task->work->mode == CLAW_STILL) {
        task->work->mode = mode;
    }
}

void event_claw_set_rates(struct task *task, int open_rate, int close_rate)
{
    if (task != NULL) {
        CLAW(task)->open_rate = open_rate;
        CLAW(task)->close_rate = close_rate;
    }
}
