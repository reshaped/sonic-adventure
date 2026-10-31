/* O CONV_BELT conveyor belt that carries what rides it, at a speed and length
 * its set entry gives */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg10/conveyor_belt.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT conveyor_belt_object;
/* the belt's three meshsets */
extern NJS_MESHSET conveyor_belt_meshsets[];
/* the moving surface's 44 UVs */
extern NJS_COLOR conveyor_belt_uv[];
/* the belt's top, which carries its riders, and its body below */
extern NJS_OBJECT conveyor_belt_top_object;
extern NJS_OBJECT conveyor_belt_body_object;

/* the belt is destroyed beyond 500 units of the camera and player 0 */
#define RANGE (500.0F * 500.0F)

/* scale z, the belt's length, is held from SCALE_MIN to SCALE_MAX */
#define SCALE_MIN 0.5F
#define SCALE_MAX 2.0F

/* each unit of angle x moves the top's riders UNITS_PER_STEP units a step,
 * times scale z. it also scrolls the surface a 256th of its texture a step.
 * the texture repeats each 256 UNITS_PER_STEP, 20.24 units, times scale z.
 * the surface thus moves as fast as its riders */
#define UNITS_PER_STEP 0.0790514F

/* the moving surface's meshset, whose UVs scroll */
#define SURFACE 2
/* the UVs the scroll copies: the surface's 44, then 20 past them */
#define SCROLLED_UVS 64

/* the top and the body are tested only while a player is within REACH of
 * the belt. REACH is 128 units for each unit of scale z, plus 20 */
#define REACH(scale_z) (((scale_z) * 128.0F) + 20.0F)

/* sound effect of the motor, played where the belt is */
#define SE_BELT 288

enum belt_mode { BELT_INIT, BELT_RUN };

struct belt_vars {
    NJS_OBJECT *object; /* the belt's top or the body, in the collision world */
};

#define VARS(work) ((struct belt_vars *)&(work)->vars)

static void conveyor_belt_init(struct task *task);
static void conveyor_belt_reset(struct task *task);
static void conveyor_belt_add(struct task *task);
static void conveyor_belt_run(struct task *task);
static void conveyor_belt_draw(struct task *task);
static void conveyor_belt_destroy(struct task *task);
static void conveyor_belt_remove(struct task *task);
static void conveyor_belt_body_init(struct task *task);
static void conveyor_belt_body_step(struct task *task);
static void conveyor_belt_body_destroy(struct task *task);

/* the speed the top carries its riders at, before the belt's turn.
 * conveyor_belt_add sets z from angle x and scale z */
static NJS_VECTOR conveyor_belt_speed = {0.0F, 0.0F, 0.0F};

/* the UVs the surface draws with, scrolled */
static NJS_COLOR conveyor_belt_scrolled[SCROLLED_UVS];

void conveyor_belt_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case BELT_INIT:
            conveyor_belt_init(task);
            break;
        case BELT_RUN:
            conveyor_belt_draw(task);
            conveyor_belt_run(task);
            break;
        }
    } else {
        switch (work->mode) {
        case BELT_INIT:
            conveyor_belt_init(task);
            break;
        case BELT_RUN:
            conveyor_belt_remove(task);
            conveyor_belt_reset(task);
            conveyor_belt_draw(task);
            conveyor_belt_run(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> BELT SPEED");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "-> SCALE Z");
            break;
        }
    }
}

static void conveyor_belt_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = conveyor_belt_draw;
    task->destructor = conveyor_belt_destroy;
    conveyor_belt_reset(task);
    work->timer = 0;
    work->mode = BELT_RUN;
}

/* hold the length from SCALE_MIN to SCALE_MAX. then add the top to the
 * collision world, and make the body */
static void conveyor_belt_reset(struct task *task)
{
    struct task_work *work = task->work;

    if (work->scl.z > SCALE_MAX) {
        work->scl.z = SCALE_MAX;
    } else if (work->scl.z < SCALE_MIN) {
        work->scl.z = SCALE_MIN;
    }
    conveyor_belt_add(task);
    task_create_child(TASK_WORK | TASK_FORCE, conveyor_belt_body_init, task);
    task_step_children(task);
}

/* set the force the top carries its riders with: the speed, turned the way
 * the belt faces. out of the set editor, add the top to the collision world */
static void conveyor_belt_add(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *top;
    NJS_OBJECT *object;
    Angle ang;

    conveyor_belt_speed.z = work->scl.z * work->ang.x * UNITS_PER_STEP;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njCalcVector(NULL, &conveyor_belt_speed, &force->speed);
    njPopMatrix(1);
    if (!placement_editing(task)) {
        top = &conveyor_belt_top_object;
        VARS(work)->object = object = world_object_alloc();
        object->evalflags = top->evalflags;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = work->scl.z;
        object->ang[0] = 0;
        object->ang[1] = work->ang.y;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = top->model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(0, task, object);
    }
}

/* have the collision world test the top only while a player is within
 * REACH. move the scroll, which work->timer holds in 256ths of the texture,
 * and keep the motor's sound playing */
static void conveyor_belt_run(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, REACH(work->scl.z))) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    work->timer -= work->ang.x;
    work->timer &= 0xff;
    sound_play_at_for(SE_BELT, (int)task, 1, 0, work->pos.x, work->pos.y,
                      work->pos.z, 2);
}

static void conveyor_belt_draw(struct task *task)
{
    struct task_work *work = task->work;
    Sint16 *from;
    Sint16 *to;
    Angle ang;
    int i;

    if (frame_step == 0) {
        conveyor_belt_meshsets[SURFACE].vertuv = conveyor_belt_scrolled;
        /* @hack spelling: Sint16 cursors walk the u and v halves of successive
         * NJS_COLOR records, for the image's @Rn+ loads and stores. as stores
         * to the records' members, the object's code runs 32 bytes long and 72
         * of 73 words differ */
        to = &conveyor_belt_scrolled->tex.u;
        from = &conveyor_belt_uv->tex.u;
        /* @bug memory: the scroll copies SCROLLED_UVS UVs (MOV #64,R6
         * at 0x0c902952) from conveyor_belt_uv, which holds 44. the last 20
         * read the three meshsets that follow it and 8 bytes past them, and
         * nothing draws their copies */
#ifdef GCC
        /* repairs the read the tag names: GCC copies conveyor_belt_uv's 44 */
        for (i = 0; i < 44; i++) {
#else
        for (i = 0; i < SCROLLED_UVS; i++) {
#endif
            *to++ = *from++;
            *to++ = *from++ + work->timer;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, 1.0F, 1.0F, work->scl.z);
        texture_set_stage();
        draw_tree(&conveyor_belt_object);
        njPopMatrix(1);
    }
}

static void conveyor_belt_destroy(struct task *task)
{
    sound_stop_id((int)task);
    conveyor_belt_remove(task);
}

/* take the top out of the collision world, free it, and destroy the body */
static void conveyor_belt_remove(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
    task_destroy_children(task);
}

/* the body carries nothing: its force is 0. out of the set editor, add it to
 * the collision world, stretched along z as the belt is. then hand over to its
 * step */
static void conveyor_belt_body_init(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;
    NJS_OBJECT *body;
    NJS_OBJECT *object;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    if (!placement_editing(task)) {
        body = &conveyor_belt_body_object;
        VARS(work)->object = object = world_object_alloc();
        object->evalflags = body->evalflags;
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = task->parent->work->scl.z;
        object->ang[0] = 0;
        object->ang[1] = work->ang.y;
        object->ang[2] = 0;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = body->model;
        object->child = NULL;
        object->sibling = NULL;
        world_add(0, task, object);
    }
    task->step = conveyor_belt_body_step;
    task->draw = NULL;
    task->destructor = conveyor_belt_body_destroy;
}

static void conveyor_belt_body_step(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, REACH(task->parent->work->scl.z))) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void conveyor_belt_body_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}
