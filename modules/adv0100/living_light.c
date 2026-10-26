/* O LIVINGLIGHT standing lamp in the living room */
#include <ninja.h>

#include "adv0100/living_light.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT living_light_object;

enum living_light_mode { LIGHT_INIT, LIGHT_RUN };

static void living_light_init(struct task *task);
static void living_light_draw(struct task *task);
static void living_light_destroy(struct task *task);

/* a capsule for the stand and two cylinders for the shade's rings */
static struct collision_shape living_light_shapes[] = {
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 17.8F, 0.0F},
     7.0F,
     17.8F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 32.0F, 0.0F},
     14.35F,
     2.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 29.7F, 0.0F},
     20.5F,
     1.25F,
     0.0F,
     {0},
     0,
     NULL},
};

/* register the lamp's shapes for this step's collision checks */
static void living_light_register(struct task *task)
{
    collision_register(task->work);
}

/* draw and register the lamp. while the set editor holds the task, also label
 * the editor's angle rows as rotations */
void egg_carrier_living_light_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case LIGHT_INIT:
            living_light_init(task);
            break;
        case LIGHT_RUN:
            living_light_draw(task);
            living_light_register(task);
            break;
        }
    } else {
        switch (work->mode) {
        case LIGHT_INIT:
            living_light_init(task);
            break;
        case LIGHT_RUN:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATE X");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATE Z");
            living_light_draw(task);
            living_light_register(task);
            break;
        }
    }
}

static void living_light_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = living_light_draw;
    task->destructor = living_light_destroy;
    collision_init(task, living_light_shapes, COLLISION_OBJECT, 1, 3);
    work->mode = LIGHT_RUN;
}

static void living_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&living_light_object);
    njPopMatrix(1);
}

static void living_light_destroy(struct task *task)
{
}
