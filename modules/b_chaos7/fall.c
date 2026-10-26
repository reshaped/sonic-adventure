/* O FALL 0: the Perfect Chaos fight's waterfall, which pours and splashes */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/fall.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/splash.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT fall_object;

/* a fall destroys itself once neither the camera nor player 0 is within 4010.
 * the range is squared, as placement_try_leave_by takes it */
#define FALL_RANGE (4010.0F * 4010.0F)

/* the flow scrolls the v of the first FALL_UVS UVs of the model's first mesh
 * set. the scroll goes back to 0 once it reaches FALL_SCROLL_WRAP */
#define FALL_UVS 80
#define FALL_SCROLL_WRAP 256

/* the set entry's fields. FALL_SPEED is the flow's speed, in UV units a step.
 * the fall splashes each time its timer passes FALL_INTERVAL. the splashes
 * land FALL_REACH from pos, scaled with the width. the model is drawn as far
 * back along z. FALL_HEIGHT and FALL_WIDTH are the fall's scales less 1 */
#define FALL_SPEED(work) ((work)->ang.x)
#define FALL_INTERVAL(work) ((work)->ang.z)
#define FALL_REACH(work) ((work)->scl.x)
#define FALL_HEIGHT(work) ((work)->scl.y)
#define FALL_WIDTH(work) ((work)->scl.z)

struct fall_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    /* @hack type: the flow's speed and scroll as an array, whose address SHC
     * takes into R5 to read both (MOV R14,R5 at 0x0c91c12a); without it, SHC
     * reads two fields off the work, fall_draw_flowing runs 2 bytes short, and
     * 77 of 82 words differ */
    Sint16 flow[2];
};

/* the indexes of flow's speed, which FALL_SPEED gives, and of the scroll so
 * far */
#define FLOW_SPEED 0
#define FLOW_SCROLL 1

#define VARS(work) ((struct fall_vars *)&(work)->vars)

/* the fall's row of fall_kinds, which chaos7_fall_0_step sets before
 * fall_step runs */
#define FALL_KIND(work) ((work)->tick)

/* nothing sets FALL_IDLE */
enum fall_mode { FALL_INIT, FALL_POURING, FALL_IDLE };

struct fall_kind {
    NJS_OBJECT *object;
    Uint8 unk_04[12];
};

static const NJS_VECTOR fall_splash_speed = {0.0F, 1.0F, 0.0F};

static struct fall_kind fall_kinds[1] = {
    {&fall_object, {0}},
};

/* the drops' params. each drop keeps them, not a copy, for its life.
 * fall_splash sets pos and speed before it makes one */
static struct dust_params fall_splash_params = {
    3.0F,
    0.038F,
    0.19F,
    0.115F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {-1.0F, 0.4F, 1.0F, 0.7F},
};

static void fall_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float width = FALL_WIDTH(work) + 1.0F;
    Float height = FALL_HEIGHT(work) + 1.0F;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, width, height, width);
    njTranslate(NULL, 0.0F, 0.0F, -FALL_REACH(work));
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

/* advance the flow's scroll, shift the UVs by it for the draw, then shift them
 * back */
static void fall_draw_flowing(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 i;

    if ((VARS(work)->flow[FLOW_SCROLL] += VARS(work)->flow[FLOW_SPEED]) >=
        FALL_SCROLL_WRAP) {
        VARS(work)->flow[FLOW_SCROLL] = 0;
    }
    for (i = 0; i < FALL_UVS; ++i) {
        VARS(work)->object->model->meshsets[0].vertuv[i].tex.v -=
            VARS(work)->flow[FLOW_SCROLL];
    }
    fall_draw(task);
    for (i = 0; i < FALL_UVS; ++i) {
        VARS(work)->object->model->meshsets[0].vertuv[i].tex.v +=
            VARS(work)->flow[FLOW_SCROLL];
    }
}

/* make a drop on the water at a random place on a half circle round pos. the
 * fall faces that half. the circle's radius is FALL_REACH, scaled with the
 * fall's width */
static void fall_splash(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float reach;
    Angle ang;

    speed = fall_splash_speed;
    reach = FALL_REACH(work) * (FALL_WIDTH(work) + 1.0F);
    ang = (Angle)(rand() * 0.99999999F) + work->ang.y + NJM_DEG_ANG(90);
    pos.x = work->pos.x + (njSin(ang) * reach);
    pos.y = 0.0F;
    pos.z = work->pos.z + (njCos(ang) * reach);
    fall_splash_params.pos = pos;
    fall_splash_params.speed = speed;
    splash_create_from(&fall_splash_params);
}

static void fall_destroy(struct task *task)
{
    task_kill(task);
}

static void fall_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 kind = FALL_KIND(work);

    if (placement_try_leave_by(task, FALL_RANGE)) {
        return;
    }
    if (edit_mode) {
        VARS(work)->object = fall_kinds[kind].object;
        VARS(work)->flow[FLOW_SPEED] = FALL_SPEED(work);
        fall_draw_flowing(task);
        if (++TASK_TIMER(work) > FALL_INTERVAL(work)) {
            work->timer = 0;
            fall_splash(task);
        }
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> FLOW SPEED");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "> Y ROTATION");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "> SPLASH INTERVAL");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> SPLASH RADIUS");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "> Y SCALE + 1.0F");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "> XZ SCALE + 1.0F");
        }
    } else {
        switch (work->mode) {
        case FALL_INIT:
            work->mode = FALL_POURING;
            VARS(work)->flow[FLOW_SPEED] = FALL_SPEED(work);
            VARS(work)->object = fall_kinds[kind].object;
            task->destructor = fall_destroy;
            task->draw = fall_draw;
            break;
        case FALL_POURING:
            fall_draw_flowing(task);
            if (TASK_TIMER(work)++ > FALL_INTERVAL(work)) {
                work->timer = 0;
                fall_splash(task);
            }
            break;
        case FALL_IDLE:
            break;
        }
    }
}

void chaos7_fall_0_step(struct task *task)
{
    FALL_KIND(task->work) = 0;
    fall_step(task);
}
