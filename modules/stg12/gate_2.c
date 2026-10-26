/* O GATE2 and GATE13: gates that a handle raises */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/gate_2.h"
#include "stg12/handle.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT gate_2_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* the gate's unscaled half width and half height */
#define GATE_WIDTH 41.0F
#define GATE_HEIGHT 31.0F

/* each turn of the handle moves the gate's aim TURN_RISE. the gate moves
 * RISE_SPEED a step, and opens once OPEN_RISE above its rest */
#define TURN_RISE 2.8F
#define RISE_SPEED 0.2F
#define OPEN_RISE 6.0F
/* GATE13's handle counts, left and right, that open it */
#define COMBINATION_LEFT 1
#define COMBINATION_RIGHT 3

enum gate_2_mode { GATE_HANDLE, GATE_LOCKED, GATE_OPENING, GATE_OPEN };

/* right and left count the handle's turns the gate has answered */
struct gate_2_vars {
    Float bottom; /* the gate's rest height */
    int right;
    int left;
};

struct gate_2_motion {
    int key; /* the key of the handle it answers */
    Uint8 unk_04[48];
    Float top; /* the height the gate opens to */
    Float rise; /* how far each turn moves aim */
    Float aim; /* the height the gate moves toward */
};

#define VARS(work) ((struct gate_2_vars *)&(work)->vars)

static void gate_2_start(struct task *task);
static void gate_2_run(struct task *task);
static void gate_2_draw(struct task *task);
static void gate_2_destroy(struct task *task);

/* a box the gate's size, which gate_2_start resizes by the set entry's
 * scale */
static struct collision_shape gate_2_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, GATE_HEIGHT, 0.0F},
    GATE_WIDTH,
    GATE_HEIGHT,
    1.2F,
    {0},
    0,
    NULL,
};

/* O GATE2: each whole left turn of the handle raises the gate's aim, up to
 * its top. each right turn lowers it, down to its rest. once OPEN_RISE above
 * its rest, the gate opens fully */
void gate_2_step(struct task *task)
{
    struct task_work *work = task->work;

    gate_2_start(task);
    work->mode = GATE_HANDLE;
    task->step = gate_2_run;
    task->draw = gate_2_draw;
    task->destructor = gate_2_destroy;
}

/* GATE13: the gate opens fully once its handle's counts stand at
 * COMBINATION_LEFT turns left and COMBINATION_RIGHT right */
void gate_13_step(struct task *task)
{
    struct task_work *work = task->work;

    gate_2_start(task);
    work->mode = GATE_LOCKED;
    VARS(work)->right = 0;
    VARS(work)->left = 0;
    task->step = gate_2_run;
    task->draw = gate_2_draw;
    task->destructor = gate_2_destroy;
}

static void gate_2_start(struct task *task)
{
    struct task_work *work = task->work;
    struct gate_2_motion *motion = (struct gate_2_motion *)task->motion;
    Float width;
    Float height;
    Float y;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    width = (work->scl.x + 1.0F) * GATE_WIDTH;
    height = (work->scl.y + 1.0F) * GATE_HEIGHT;
    y = work->pos.y;
    motion->key = (int)njAbs(work->scl.z) % HANDLE_KEYS;
    handle_get_turns(motion->key, &VARS(work)->right, &VARS(work)->left);
    motion->rise = TURN_RISE;
    motion->aim = y;
    motion->top = (height * 2.0F) + y;
    VARS(work)->bottom = y;
    if (!placement_editing(task)) {
        collision_init(task, &gate_2_shape, COLLISION_OBJECT, 1, 1);
        work->collision->shapes->radius = width;
        work->collision->shapes->size_y = height + 2.0F;
        work->collision->shapes->center.y = height;
        work->collision->radius =
            njSqrt((width * width) + (height * height)) + 20.0F;
    }
}

static void gate_2_run(struct task *task)
{
    struct task_work *work = task->work;
    struct gate_2_motion *motion = (struct gate_2_motion *)task->motion;
    int right;
    int left;
    int key;
    Float y;
    Float aim;

    if (placement_editing(task)) {
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> SCALE  X");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> SCALE  Y");
        key = (int)work->scl.z;
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> Key ID:%d", key);
    } else {
        switch (work->mode) {
        case GATE_HANDLE:
            y = work->pos.y;
            aim = motion->aim;
            handle_get_turns(motion->key, &right, &left);
            if (left > VARS(work)->left) {
                VARS(work)->left++;
                aim += motion->rise;
                if (aim >= motion->top) {
                    aim = motion->top;
                }
                motion->aim = aim;
            }
            if (right > VARS(work)->right) {
                VARS(work)->right++;
                aim -= motion->rise;
                if (aim <= VARS(work)->bottom) {
                    aim = VARS(work)->bottom;
                }
                motion->aim = aim;
            }
            if (y < aim) {
                y += RISE_SPEED;
                if (y > aim) {
                    y = aim;
                }
            }
            if (y >= aim) {
                y -= RISE_SPEED;
                if (y < aim) {
                    y = aim;
                }
            }
            if (y - VARS(work)->bottom >= OPEN_RISE) {
                work->mode = GATE_OPENING;
            }
            work->pos.y = y;
            break;
        case GATE_LOCKED:
            handle_get_turns(motion->key, &right, &left);
            if (left == COMBINATION_LEFT && right == COMBINATION_RIGHT) {
                work->mode = GATE_OPENING;
            }
            break;
        case GATE_OPENING:
            work->pos.y += RISE_SPEED;
            if (work->pos.y >= motion->top) {
                work->pos.y = motion->top;
                work->mode = GATE_OPEN;
            }
            break;
        case GATE_OPEN:
            break;
        }
        /* @hack spelling: task->work read again, MOV.L @(32,R12),R4 at
         * 0x0c90504c in the call's delay slot; through work, 1 of 180 words
         * differ */
        collision_register(task->work);
    }
    gate_2_draw(task);
}

static void gate_2_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F, 1.0F);
        draw_tree(&gate_2_object);
        njPopMatrix(1);
    }
}

static void gate_2_destroy(struct task *task)
{
}
