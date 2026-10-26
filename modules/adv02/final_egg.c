/* the Final Egg base in the jungle, which the object list names O FINALEGG,
 * and O FINALWAY, the solid way into it. both are there only once the
 * character's FLAG_*_MR_APPEAR_FINAL_EGG is set */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/final_egg.h"
#include "adv02/mystic_ruins.h"
#include "adv02/texlists.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_ACTION mystic_ruins_final_egg_action;
/* the base draws this action at HELD_FRAME, which nothing changes. its tree
 * stands where the way does */
extern NJS_ACTION mystic_ruins_final_egg_way_action;
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address at 0x0c9117b4 and reads model through it, MOV.L @(4,R4),R4 at
 * 0x0c911746; declared as one, 12 of mystic_ruins_final_way_step's 79 words
 * and a pool word differ */
extern NJS_OBJECT mystic_ruins_final_way_object[];

#define FRAME_SPEED 0.2F /* the frames the base's action advances each step */
#define FRAMES 59.0F /* the frame at which the base's action starts over */

#define WAY_FLAGS (WORLD_LARGE | WORLD_SOLID)
/* the collision world tests the way only while player 0 is this near */
#define NEAR 100.0F

/* the frames the base draws its two actions at, in work->scl.x and
 * work->scl.y. each starts as the set entry's scale gives it */
#define FRAME(work) ((work)->scl.x)
#define HELD_FRAME(work) ((work)->scl.y)

struct final_way_vars {
    NJS_OBJECT *object; /* the way's copy in the collision world */
};

#define VARS(work) ((struct final_way_vars *)&(work)->vars)

/* the modes of both steps */
enum final_egg_mode { FINAL_EGG_INIT, FINAL_EGG_RUN };

static void mystic_ruins_final_egg_draw(struct task *task);
static void mystic_ruins_final_egg_destroy(struct task *task);
static void mystic_ruins_final_way_draw(struct task *task);
static void mystic_ruins_final_way_destroy(struct task *task);
static void mystic_ruins_final_way_add_to_world(struct task *task);
static Bool mystic_ruins_final_egg_appeared(void);

void mystic_ruins_final_egg_step(struct task *task)
{
    struct task_work *work = task->work;

    if ((Uint8)stage_get_stage_act() == MYSTIC_RUINS_JUNGLE &&
        mystic_ruins_final_egg_appeared()) {
        switch (work->mode) {
        case FINAL_EGG_INIT:
            task->destructor = mystic_ruins_final_egg_destroy;
            task->draw = mystic_ruins_final_egg_draw;
            work->mode = FINAL_EGG_RUN;
            break;
        case FINAL_EGG_RUN:
            FRAME(work) += FRAME_SPEED;
            if (FRAME(work) >= FRAMES) {
                FRAME(work) = 0.0F;
            }
            break;
        }
        mystic_ruins_final_egg_draw(task);
    }
}

/* draw the base with fog off and the far clip 100000 away. then turn fog on
 * and put the far clip 2000 away, whatever the two were before */
static void mystic_ruins_final_egg_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njClipZ(-3.0F, -100000.0F);
    njFogDisable();
    njSetTexture(&mr_finalegg_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&mystic_ruins_final_egg_action, FRAME(work));
    draw_action(&mystic_ruins_final_egg_way_action, HELD_FRAME(work));
    njPopMatrix(1);
    njFogEnable();
    njClipZ(-3.0F, -2000.0F);
}

static void mystic_ruins_final_egg_destroy(struct task *task)
{
}

void mystic_ruins_final_way_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (!mystic_ruins_final_egg_appeared()) {
        return;
    }
    if (placement_editing(task)) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njSimpleDrawModel(mystic_ruins_final_way_object->model);
        njPopMatrix(1);
        return;
    }
    switch (work->mode) {
    case FINAL_EGG_INIT:
        mystic_ruins_final_way_add_to_world(task);
        task->destructor = mystic_ruins_final_way_destroy;
        task->draw = mystic_ruins_final_way_draw;
        work->mode = FINAL_EGG_RUN;
        break;
    case FINAL_EGG_RUN:
        break;
    }
    work->flags &= ~WORLD_TESTED;
    if (player_in_range(&work->pos, NEAR) == 1) {
        work->flags |= WORLD_TESTED;
    }
}

static void mystic_ruins_final_way_draw(struct task *task)
{
}

static void mystic_ruins_final_way_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object != NULL) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

static void mystic_ruins_final_way_add_to_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from = mystic_ruins_final_way_object;
    NJS_OBJECT *object;

    VARS(work)->object = object = world_object_alloc();
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = from->ang[0];
    object->ang[1] = from->ang[1];
    object->ang[2] = from->ang[2];
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(WAY_FLAGS, task, object);
}

static Bool mystic_ruins_final_egg_appeared(void)
{
    int character = get_character();

    switch (character) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_MR_APPEAR_FINAL_EGG];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_MR_APPEAR_FINAL_EGG];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_MR_APPEAR_FINAL_EGG];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_MR_APPEAR_FINAL_EGG];
    }
    return FALSE;
}
