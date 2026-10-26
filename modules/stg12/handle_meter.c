/* O KAITENMETER: a gauge of the whole turns each way under a handle's key */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "stg12/handle.h"
#include "stg12/handle_meter.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT handle_meter_object;
extern NJS_OBJECT handle_meter_right_object;
extern NJS_OBJECT handle_meter_left_object;

/* destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

enum handle_meter_mode { METER_INIT, METER_RUN };

/* left and right are the counts the bars show */
struct handle_meter_vars {
    Uint8 left;
    Uint8 right;
    Uint8 unk_02[6];
    /* @hack type: key in an array of one, so SHC adds 16 to the work's
     * address to reach it, as ADD #16,R3 at 0x0c9018b6 does; as a Sint8,
     * summed over handle_meter_init and handle_meter_read, 18 of 50 words
     * differ */
    Sint8 key[1];
};

#define VARS(work) ((struct handle_meter_vars *)&(work)->vars)

static void handle_meter_init(struct task *task);
static void handle_meter_draw(struct task *task);
static void handle_meter_destroy(struct task *task);
static void handle_meter_read(struct task_work *work);

static struct collision_shape handle_meter_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    17.0F,
    1.5F,
    0.0F,
    {0},
    0,
    NULL,
};

/* a bar's x scale, by the count it shows. the table holds counts to 5. each
 * handle under a meter's key must count at most 5 turns each way */
static Float handle_meter_lengths[] = {0.0F, 9.0F, 18.0F, 27.0F, 36.0F, 45.0F};

/* @hack spelling: the registration in a helper, which SHC expands at both of
 * handle_meter_step's calls. the step then saves only R14 and R13 (0x0c901800,
 * 0x0c901804). with collision_register(work) written in the step, it saves R12
 * too, the object's code runs 32 bytes long, and 70 of the step's 74 words
 * differ */
static void handle_meter_register(struct task *task)
{
    struct task_work *work = task->work;

    collision_register(work);
}

void handle_meter_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case METER_INIT:
            handle_meter_init(task);
            break;
        case METER_RUN:
            handle_meter_draw(task);
            handle_meter_read(work);
            handle_meter_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case METER_INIT:
            handle_meter_init(task);
            break;
        case METER_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            handle_meter_draw(task);
            handle_meter_register(task);
            break;
        }
    }
}

static void handle_meter_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = handle_meter_draw;
    task->destructor = handle_meter_destroy;
    collision_init(task, &handle_meter_shape, COLLISION_OBJECT, 1, 1);
    VARS(work)->left = 0;
    VARS(work)->right = 0;
    VARS(work)->key[0] = (int)work->scl.x & (HANDLE_KEYS - 1);
    work->mode = METER_RUN;
}

static void handle_meter_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&handle_meter_object);
        njPushMatrix(NULL);
        njScale(NULL, handle_meter_lengths[VARS(work)->left], 1.0F, 1.0F);
        draw_tree(&handle_meter_left_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njScale(NULL, handle_meter_lengths[VARS(work)->right], 1.0F, 1.0F);
        draw_tree(&handle_meter_right_object);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void handle_meter_destroy(struct task *task)
{
}

static void handle_meter_read(struct task_work *work)
{
    int right;
    int left;

    handle_get_turns(VARS(work)->key[0], &right, &left);
    VARS(work)->right = right;
    VARS(work)->left = left;
}
