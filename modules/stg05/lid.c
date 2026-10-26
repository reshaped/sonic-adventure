/* O Futa, a lid whose two halves slide apart once something touches it,
 * unless the character is Knuckles */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg05/lid.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT lid_half_0_object;
extern NJS_OBJECT lid_half_1_object;

/* the lid's modes, which each half takes from the lid while shut. a half
 * that has slid all the way sets itself and the lid LID_OPEN. no step sets
 * LID_MODE_4 */
enum lid_mode { LID_INIT, LID_SHUT, LID_OPENING, LID_OPEN, LID_MODE_4 };

/* the object a half draws, and a texlist the halves never read. a half's task
 * spec passes the pair in the half's vars */
struct lid_model {
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
};

/* work->submode holds the half's index, which its task spec gives as
 * work->flags. work->tick is set once the half has set itself up while the
 * set editor holds the lid */
struct lid_half_vars {
    union {
        struct lid_model *model; /* as the task spec passes it */
        NJS_OBJECT *object; /* the model's object, once the half starts */
    } draws;
    Float distance; /* what the half has left to take from its z */
    NJS_TEXLIST *texlist;
};

#define HALF(work) ((struct lid_half_vars *)&(work)->vars)

/* a half is open once it is this near the end of its slide */
#define LID_NEAR_END 0.01F

static void lid_half_draw(struct task *task);
static void lid_half_step(struct task *task);
static void lid_destroy(struct task *task);

/* the sphere whose touch opens the lid */
static struct collision_shape lid_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    60.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* @hack type: an array of one, not the single shape it is. as one shape,
 * lid_half_step keeps a 1 in R12 through the step and saves it, runs 6 bytes
 * longer, and 117 of 120 words differ */
static struct collision_shape lid_half_shape[1] = {{
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {6.5F, -1.5F, 0.0F},
    6.5F,
    3.0F,
    11.5F,
    {0, 0, 0},
    0,
    NULL,
}};

static struct lid_model lid_models[2] = {
    {&lid_half_0_object, &obj_mountain_texlist},
    {&lid_half_1_object, &obj_mountain_texlist},
};

/* the halves, the second turned half a turn to face the first. each spec's
 * flags give its half's index */
static struct task_spec lid_halves[3] = {
    {lid_half_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&lid_models[0]},
    {lid_half_step,
     TASK_WORK,
     1,
     {0, 0x8000, 0},
     {0.0F, 0.0F, 0.0F},
     (Sint32)&lid_models[1]},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* lid_slides[i] is what half i takes from its z each step.
 * lid_slides[i + 2] is what it takes in all */
static Float lid_slides[4] = {-0.1F, 0.1F, -18.0F, 18.0F};

static void lid_half_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_model(HALF(work)->draws.object->model);
        njPopMatrix(1);
    }
}

/* while the set editor holds the lid, a half follows it. otherwise the half
 * takes the lid's mode while shut, and slides once that is LID_OPENING */
static void lid_half_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lid_model *model;

    if (placement_editing(task->parent)) {
        /* @hack spelling: a switch of one case, which tests the tick with
         * EXTU.B and CMP/EQ #0 (0x0c90f742, 0x0c90f744); as an if, it tests it
         * with TST, the function is 2 bytes shorter and 110 of 120 words differ
         */
        switch (work->tick) {
        case 0:
            work->submode = work->flags;
            model = HALF(work)->draws.model;
            HALF(work)->texlist = &obj_mountain_texlist;
            HALF(work)->draws.object = model->object;
            work->tick = 1;
            break;
        }
        work->pos = task->parent->work->pos;
        work->ang.y = task->parent->work->ang.y;
        lid_half_draw(task);
    } else {
        switch (work->mode) {
        case LID_INIT:
            work->submode = work->flags;
            HALF(work)->distance = lid_slides[work->submode + 2];
            model = HALF(work)->draws.model;
            HALF(work)->texlist = &obj_mountain_texlist;
            HALF(work)->draws.object = model->object;
            task->draw = lid_half_draw;
            collision_init(task, lid_half_shape, COLLISION_OBJECT, 1, 1);
            work->mode = LID_SHUT;
            break;
        case LID_SHUT:
            work->mode = task->parent->work->mode;
            break;
        case LID_OPENING:
            work->pos.z -= lid_slides[work->submode];
            HALF(work)->distance -= lid_slides[work->submode];
            if (_builtin_fabsf(HALF(work)->distance) <= LID_NEAR_END) {
                work->mode = task->parent->work->mode = LID_OPEN;
            }
            break;
        /* @hack redundant: a case and break for LID_OPEN apart from the
         * default's, which keeps the switch's tests' branches (0x0c90f782 to
         * 0x0c90f78a); without it, the function is 4 bytes shorter and 75 of
         * 120 words differ */
        case LID_OPEN:
            break;
        default:
            break;
        }
        lid_half_draw(task);
        collision_register(work);
    }
}

static void lid_destroy(struct task *task)
{
}

/* unless the set editor holds the lid, a touch opens it, but not with
 * Knuckles as the character. the touch that opens it turns the sphere off */
void lid_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case LID_INIT:
        task->destructor = lid_destroy;
        task_create_children(lid_halves, task);
        collision_init(task, &lid_shape, COLLISION_OBJECT, 1, 1);
        work->mode = LID_SHUT;
        /* fall through */
    case LID_SHUT:
        if (!placement_editing(task)) {
            if (work->collision->flags & COLLISION_HIT) {
                if (get_character() != CHARACTER_KNUCKLES) {
                    work->mode = LID_OPENING;
                    work->collision->shapes->flags |= SHAPE_OFF;
                }
            }
        }
        break;
    case LID_OPENING:
    case LID_OPEN:
    case LID_MODE_4:
        break;
    }
    task_step_children(task);
    collision_register(work);
}
