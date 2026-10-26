/* O ELEVATOR button panel that sends the elevator between its two floors */
#include <ninja.h>

#include "adv0100/elevator.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* its first button is its child, and the second is that child's sibling */
extern NJS_OBJECT elevator_object;

#define PRESS_DEPTH (-4.0F) /* how far a press sinks the buttons */
#define PRESS_SPEED 0.5F /* how far the buttons move in a step */
#define TRAVEL_STEPS 240 /* the steps the elevator takes between floors */

/* in work->flags, each set from the press until the elevator arrives */
#define LIT_1 0x200 /* the first button, lit for travel from floor 1 */
#define LIT_0 0x100 /* the second button, lit for travel from floor 0 */

#define PRESS(work) ((work)->scl.x) /* the buttons' depth, to PRESS_DEPTH */
/* the floor the elevator stands at, or travels from: 0 or 1 */
#define FLOOR(work) ((work)->submode)

enum elevator_mode {
    ELEVATOR_INIT,
    ELEVATOR_WAIT,
    ELEVATOR_PRESSING,
    ELEVATOR_RELEASING,
    ELEVATOR_TRAVEL,
    ELEVATOR_EDITING /* while the set editor runs, the panel is only drawn */
};

static void elevator_draw(struct task *task);
static void elevator_destroy(struct task *task);
static void elevator_draw_work(struct task_work *work);

static struct collision_shape elevator_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL,
     {0.0F, 15.0F, 0.25F},
     10.0F,
     15.0F,
     0.25F,
     {0, 0, 0},
     0,
     NULL},
};

void egg_carrier_elevator_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case ELEVATOR_INIT:
        if (!placement_editing(task)) {
            work->ang.x = 0;
            work->ang.z = 0;
            work->scl.x = 0.0F;
            work->scl.z = 0.0F;
            collision_init(task, elevator_shapes, COLLISION_OBJECT, 1, 1);
            if (stage_get_entrance() == 0) {
                FLOOR(work) = 0;
                event_flag_clear(FLAG_ELEVATOR_AT_1);
                event_flag_set(FLAG_ELEVATOR_AT_0);
            } else {
                FLOOR(work) = 1;
                event_flag_set(FLAG_ELEVATOR_AT_1);
                event_flag_clear(FLAG_ELEVATOR_AT_0);
            }
        }
        task->destructor = elevator_destroy;
        task->draw = elevator_draw;
        if (placement_editing(task)) {
            work->mode = ELEVATOR_EDITING;
        } else {
            work->mode = ELEVATOR_WAIT;
        }
        break;
    case ELEVATOR_WAIT:
        if (work->collision->flags & COLLISION_HIT) {
            event_flags[FLAG_ELEVATOR_AT_1] = 0;
            event_flags[FLAG_ELEVATOR_AT_0] = 0;
            work->mode = ELEVATOR_PRESSING;
        }
        break;
    case ELEVATOR_PRESSING:
        /* @hack spelling: -PRESS_SPEED added, a constant of its own, which
         * SHC loads through R1 and FPUL (0x0c925bc4) as ELEVATOR_RELEASING
         * loads 0.5 (0x0c925bf6); with -=, SHC loads 0.5 once into FR4 ahead
         * of the switch, and 138 of 221 words differ */
        PRESS(work) += -PRESS_SPEED;
        if (PRESS(work) > PRESS_DEPTH) {
            break;
        }
        PRESS(work) = PRESS_DEPTH;
        if (FLOOR(work) == 1) {
            work->flags |= LIT_1;
        } else {
            work->flags |= LIT_0;
        }
        work->mode = ELEVATOR_RELEASING;
        break;
    case ELEVATOR_RELEASING:
        PRESS(work) += PRESS_SPEED;
        if (PRESS(work) < 0.0F) {
            break;
        }
        PRESS(work) = 0.0F;
        if (FLOOR(work) == 1) {
            work->flags |= LIT_1;
        } else {
            work->flags |= LIT_0;
        }
        work->timer = 0;
        work->mode = ELEVATOR_TRAVEL;
        break;
    case ELEVATOR_TRAVEL:
        ++work->timer;
        if (TASK_TIMER(work) < TRAVEL_STEPS) {
            break;
        }
        work->flags &= ~LIT_1;
        work->flags &= ~LIT_0;
        if (FLOOR(work) == 1) {
            FLOOR(work) = 0;
            event_flag_clear(FLAG_ELEVATOR_AT_1);
            event_flag_set(FLAG_ELEVATOR_AT_0);
        } else {
            FLOOR(work) = 1;
            event_flag_set(FLAG_ELEVATOR_AT_1);
            event_flag_clear(FLAG_ELEVATOR_AT_0);
        }
        work->mode = ELEVATOR_WAIT;
        break;
    }
    elevator_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void elevator_draw(struct task *task)
{
    struct task_work *work = task->work;

    elevator_draw_work(work);
}

static void elevator_destroy(struct task *task)
{
}

static void elevator_draw_work(struct task_work *work)
{
    NJS_OBJECT *object = &elevator_object;
    Angle ang;

    texture_set_stage();
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    njScale(NULL, 0.5F, 0.5F, 0.5F);
    draw_model(object->model);
    object = object->child;
    if (work->flags & LIT_1) {
        draw_set_material(1.0F, 0.6F, 0.6F, 0.0F);
    } else {
        draw_set_material(1.0F, -0.3F, -0.3F, -0.3F);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    njTranslate(NULL, 0.0F, 0.0F, PRESS(work));
    draw_model(object->model);
    njPopMatrix(1);
    object = object->sibling;
    if (work->flags & LIT_0) {
        draw_set_material(1.0F, 0.0F, 0.6F, 0.6F);
    } else {
        draw_set_material(1.0F, -0.3F, -0.3F, -0.3F);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    njTranslate(NULL, 0.0F, 0.0F, PRESS(work));
    draw_model(object->model);
    njPopMatrix(1);
    njPopMatrix(1);
}
