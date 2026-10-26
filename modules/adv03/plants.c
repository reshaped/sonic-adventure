/* O SHIDA, a fern, and O SOTETSU, a cycad. a plant that stands still sways
 * through its action once whenever another collision record starts or stops
 * touching it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/plants.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_ACTION shida_action;
extern NJS_ACTION sotetsu_action;
extern NJS_OBJECT plants_shadow_object;

/* flags of work->flags. PLANT_SWAYING lasts from a sway's step through the
 * draw after it, which then plays the action. PLANT_TOUCHED is set while
 * another record touches the plant, and PLANT_CHANGED for the step a touch
 * starts or ends */
#define PLANT_SWAYING 0x200
#define PLANT_TOUCHED 0x400
#define PLANT_CHANGED 0x800

/* the sound a sway starts with */
#define SE_SWAY 0x255
/* the radius draw_in_view tests a plant's position by */
#define VIEW_RADIUS 30.0F

/* a sway plays its action one frame each step, from frame 0 until the frame
 * reaches SHIDA_FRAMES or SOTETSU_FRAMES */
#define SHIDA_FRAMES 60.0F
#define SOTETSU_FRAMES 15.0F

enum plant_mode { PLANT_INIT, PLANT_WAIT, PLANT_SWAY };

struct plant_vars {
    Uint8 unk_00[8];
    Float frame;
};

#define VARS(work) ((struct plant_vars *)&(work)->vars)

/* a ball that pushes no group, and that no group's shapes can push */
static struct collision_shape plant_shape[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     10.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void shida_destroy(struct task *task);
static void shida_draw(struct task *task);
static void sotetsu_destroy(struct task *task);
static void sotetsu_draw(struct task *task);
static void plant_touch(struct task_work *work);

void shida_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        shida_draw(task);
        return;
    }
    plant_touch(work);
    switch (work->mode) {
    case PLANT_INIT:
        collision_init(task, plant_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = shida_destroy;
        task->draw = shida_draw;
        work->mode = PLANT_WAIT;
        break;
    case PLANT_WAIT:
        if (work->flags & PLANT_CHANGED) {
            work->mode = PLANT_SWAY;
            VARS(work)->frame = 0.0F;
            sound_play(SE_SWAY, 0, 0, 0);
        }
        break;
    case PLANT_SWAY:
        work->flags |= PLANT_SWAYING;
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= SHIDA_FRAMES) {
            VARS(work)->frame = 0.0F;
            work->mode = PLANT_WAIT;
        }
        break;
    }
    shida_draw(task);
    work->flags &= ~PLANT_SWAYING;
    collision_register(work);
}

static void shida_destroy(struct task *task)
{
}

static void shida_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        NJS_ACTION *action = &shida_action;

        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
        if (work->flags & PLANT_SWAYING) {
            draw_action(action, VARS(work)->frame);
        } else {
            draw_tree(action->object);
        }
        draw_tree(&plants_shadow_object);
        njPopMatrix(1);
    }
}

void sotetsu_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        sotetsu_draw(task);
        return;
    }
    plant_touch(work);
    switch (work->mode) {
    case PLANT_INIT:
        collision_init(task, plant_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = sotetsu_destroy;
        task->draw = sotetsu_draw;
        work->mode = PLANT_WAIT;
        break;
    case PLANT_WAIT:
        if (work->flags & PLANT_CHANGED) {
            work->mode = PLANT_SWAY;
            VARS(work)->frame = 0.0F;
            sound_play(SE_SWAY, 0, 0, 0);
        }
        break;
    case PLANT_SWAY:
        work->flags |= PLANT_SWAYING;
        VARS(work)->frame += 1.0F;
        if (VARS(work)->frame >= SOTETSU_FRAMES) {
            VARS(work)->frame = 0.0F;
            work->mode = PLANT_WAIT;
        }
        break;
    }
    sotetsu_draw(task);
    work->flags &= ~PLANT_SWAYING;
    collision_register(work);
}

static void sotetsu_destroy(struct task *task)
{
}

static void sotetsu_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        NJS_ACTION *action = &sotetsu_action;

        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
        if (work->flags & PLANT_SWAYING) {
            draw_action(action, VARS(work)->frame);
        } else {
            draw_tree(action->object);
        }
        draw_tree(&plants_shadow_object);
        njPopMatrix(1);
    }
}

/* clear PLANT_CHANGED, then set it for the step another record starts or
 * stops touching the plant. without a collision record, PLANT_TOUCHED stays
 * as it was */
static void plant_touch(struct task_work *work)
{
    work->flags &= ~PLANT_CHANGED;
    if (work->collision != NULL) {
        if (work->collision->flags & COLLISION_HIT) {
            if (!(work->flags & PLANT_TOUCHED)) {
                work->flags |= PLANT_CHANGED;
            }
            work->flags |= PLANT_TOUCHED;
        } else {
            if (work->flags & PLANT_TOUCHED) {
                work->flags |= PLANT_CHANGED;
            }
            work->flags &= ~PLANT_TOUCHED;
        }
    }
}
