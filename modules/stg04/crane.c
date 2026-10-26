/* O CRANE crane that carries a platform up its arm and back down */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/target.h"
#include "sound/sound.h"
#include "stg04/crane.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT crane_object;
extern NJS_OBJECT crane_platform_object;
extern NJS_ACTION crane_platform_action;

/* the crane's range for placement_try_leave_by, squared */
#define CRANE_RANGE (1010.0F * 1010.0F)
/* the collision world tests the crane or the platform only while a player is
 * within CRANE_NEAR of it */
#define CRANE_NEAR 570.0F

#define CRANE_WORLD_FLAGS (WORLD_LARGE | WORLD_TURNED)
#define PLATFORM_WORLD_FLAGS (WORLD_MEDIUM | WORLD_TURNED | WORLD_MOVES)

/* the platform climbs until it is more than PLATFORM_HEIGHT above the crane.
 * it moves PLATFORM_SPEED a step along its heading, and PLATFORM_RISE up. at
 * the top, it waits for player 0 to stand on it for more than PLATFORM_WAIT
 * steps */
#define PLATFORM_HEIGHT 100.0F
#define PLATFORM_SPEED 1.225F
#define PLATFORM_RISE 0.25F
#define PLATFORM_WAIT 120

/* the box above the platform's floor in which player 0 counts as standing on
 * it: its center's height above the platform, and its half sizes */
#define PLATFORM_MIDDLE 20.0F
#define PLATFORM_HALF_X 20.0F
#define PLATFORM_HALF_Y 10.0F
#define PLATFORM_HALF_Z 20.0F

/* the platform's action advances PLATFORM_FRAME_STEP of a frame each step.
 * the platform is a target of radius PLATFORM_TARGET, which a falling player
 * steers onto */
#define PLATFORM_FRAME_STEP 0.2F
#define PLATFORM_TARGET 15.0F

/* sound effects of the platform moving and stopping */
#define SE_PLATFORM_MOVE 103
#define SE_PLATFORM_STOP 104

/* indices of the platform's lamp materials: the first glows while the platform
 * moves, the second while it is still */
#define LAMP_MOVING 3
#define LAMP_STILL 4

#define WHOLE_TURN NJM_DEG_ANG(360)

enum crane_mode {
    CRANE_INIT,
    CRANE_RUN,
    CRANE_GONE /* this file never sets it */
};
enum platform_mode {
    PLATFORM_INIT,
    PLATFORM_BOTTOM,
    PLATFORM_CLIMB,
    PLATFORM_TOP,
    PLATFORM_DESCEND,
    PLATFORM_LANDED,
    PLATFORM_GONE /* this file never sets it */
};

struct crane_vars {
    /* the task's copy of its model in the collision world */
    NJS_OBJECT *object;
    /* the task's own model: the crane's object, or the platform's action */
    union {
        NJS_OBJECT *object;
        NJS_ACTION *action;
    } model;
};

#define VARS(work) ((struct crane_vars *)&(work)->vars)
/* the platform's work->tick is TRUE while it moves, and work->scl.x holds its
 * action's frame */
#define MOVING(work) ((work)->tick)
#define FRAME(work) ((work)->scl.x)

static void crane_platform_step(struct task *task);
static void crane_platform_create(struct task *task);

/* keep the task's copy in the collision world at its position, while the set
 * editor is closed */
static void crane_follow(struct task_work *work)
{
    NJS_OBJECT *object;

    if (!edit_mode) {
        object = VARS(work)->object;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        if (player_in_range(&work->pos, CRANE_NEAR)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
}

static void crane_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_model(VARS(work)->object->model);
    njPopMatrix(1);
}

static void crane_remove(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && VARS(work)->object != NULL) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

static void crane_destroy(struct task *task)
{
    /* @hack spelling: crane, a copy of task, which SHC keeps in R14 across
     * task_destroy_children, MOV R4,R14 at 0x0c900a5c; through task, which
     * then goes to the stack, the function runs 2 bytes shorter and 9 of 9
     * words differ */
    struct task *crane = task;

    task_destroy_children(crane);
    crane_remove(crane);
}

void crane_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave_by(task, CRANE_RANGE)) {
        return;
    }
    if (edit_mode) {
        work->ang.z = 0;
        work->ang.x = 0;
        /* @bug logic: each step with the set editor open pops a matrix it
         * didn't push, by the call at 0x0c900a96, then pushes and pops its
         * own. a stack deeper than its base matrix is left one shallower.
         * from the base alone, njPopMatrix pops nothing */
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateY, work->ang.y);
        texture_set_stage();
        draw_tree(&crane_object);
        draw_tree(&crane_platform_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case CRANE_INIT:
            VARS(work)->model.object = &crane_object;
            task->destructor = crane_destroy;
            task->draw = crane_draw;
            work->ang.z = 0;
            work->ang.x = 0;
            if (!edit_mode) {
                VARS(work)->object = object = world_object_alloc();
                object->evalflags = VARS(work)->model.object->evalflags &
                                    ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
                object->scl[0] = object->scl[1] = object->scl[2] = 1.0F;
                object->ang[0] = work->ang.x;
                object->ang[1] = work->ang.y;
                object->ang[2] = work->ang.z;
                object->pos[0] = work->pos.x;
                object->pos[1] = work->pos.y;
                object->pos[2] = work->pos.z;
                object->model = VARS(work)->model.object->model;
                object->child = VARS(work)->model.object->child;
                object->sibling = VARS(work)->model.object->sibling;
                world_add(CRANE_WORLD_FLAGS, task, object);
            } else {
                VARS(work)->object = VARS(work)->model.object;
            }
            crane_platform_create(task);
            work->mode = CRANE_RUN;
            break;
        case CRANE_RUN:
            crane_follow(work);
            crane_draw(task);
            if (!edit_mode) {
                task_step_children(task);
            }
            break;
        case CRANE_GONE:
        default:
            crane_remove(task);
            break;
        }
    }
}

static void crane_platform_draw(struct task *task)
{
    struct task_work *work = task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    njTranslate(NULL, 0.0F, 0.0F, -30.0F);
    draw_action(VARS(work)->model.action, FRAME(work));
    njPopMatrix(1);
}

/* TRUE while player 0 is on the ground inside the box above the platform's
 * floor */
static Bool crane_platform_holds(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR to;
    NJS_VECTOR local;

    to.x = player_work[0]->pos.x - work->pos.x;
    to.y = player_work[0]->pos.y - work->pos.y - PLATFORM_MIDDLE;
    to.z = player_work[0]->pos.z - work->pos.z;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, WHOLE_TURN - work->ang.y);
    njCalcVector(NULL, &to, &local);
    njPopMatrix(1);
    if ((player_work[0]->flags & PLAYER_ON_LAND) &&
        njAbs(local.x) < PLATFORM_HALF_X && njAbs(local.y) < PLATFORM_HALF_Y &&
        njAbs(local.z) < PLATFORM_HALF_Z) {
        return TRUE;
    }
    return FALSE;
}

/* show the lamp of the platform's state, moving or still, and hide the other.
 * the lamp shown ignores the light, so it glows */
static void crane_platform_lamps(struct task_work *work)
{
    NJS_OBJECT *lamps;
    NJS_MATERIAL *still;
    NJS_MATERIAL *moving;
    NJS_OBJECT *still_node;
    NJS_OBJECT *moving_node;

    lamps = VARS(work)->model.action->object->child;
    still = lamps->model->mats;
    moving = &still[LAMP_MOVING];
    still += LAMP_STILL;
    moving_node = lamps->child;
    still_node = lamps->child->sibling;
    if (MOVING(work) == TRUE) {
        moving->attrflags |= NJD_FLAG_IGNORE_LIGHT;
        moving_node->evalflags &= ~NJD_EVAL_HIDE;
        still->attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        /* @hack spelling: the store written out, which keeps still_node in R5
         * and the still material in R6 (0x0c900d82 to 0x0c900d9c); as |=, the
         * two swap and 16 of 47 words differ */
        still_node->evalflags = still_node->evalflags | NJD_EVAL_HIDE;
    } else {
        moving->attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        moving_node->evalflags |= NJD_EVAL_HIDE;
        still->attrflags |= NJD_FLAG_IGNORE_LIGHT;
        /* @hack spelling: the same for the clear; as &=, 16 of 47 words
         * differ */
        still_node->evalflags = still_node->evalflags & ~NJD_EVAL_HIDE;
    }
}

/* player 0 standing on the platform at the foot of the arm sends it up. at the
 * top, player 0 stepping off doesn't reset the count of steps on it. back down,
 * it climbs again as soon as player 0 stands on it, even after a ride down */
static void crane_platform_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *crane = task->parent->work;
    struct task_force *force = task->force;
    NJS_OBJECT *object;

    switch (work->mode) {
    case PLATFORM_INIT:
        work->ang.z = 0;
        work->ang.x = 0;
        work->mode = PLATFORM_BOTTOM;
        VARS(work)->model.action = &crane_platform_action;
        MOVING(work) = FALSE;
        crane_platform_lamps(work);
        task->destructor = crane_remove;
        task->draw = crane_platform_draw;
        if (!edit_mode) {
            VARS(work)->object = object = world_object_alloc();
            object->evalflags = VARS(work)->model.action->object->evalflags &
                                ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
            object->scl[0] = object->scl[1] = object->scl[2] = 1.0F;
            object->ang[0] = work->ang.x;
            object->ang[1] = work->ang.y;
            object->ang[2] = work->ang.z;
            object->pos[0] = work->pos.x;
            object->pos[1] = work->pos.y;
            object->pos[2] = work->pos.z;
            object->model = VARS(work)->model.action->object->model;
            object->child = VARS(work)->model.action->object->child;
            /* @bug memory: the action read as an object, which reads
             * the sibling 0x30 bytes into the 8-byte NJS_ACTION, past its end
             * (MOV.L @(48,R2),R1 at 0x0c900eba). the copy's sibling is then
             * 0x00020004, the word the image holds at 0x0ca4021c */
            object->sibling = VARS(work)->model.object->sibling;
            world_add(PLATFORM_WORLD_FLAGS, task, object);
            force[0].speed.x = 0.0F;
            force[0].speed.y = 0.0F;
            force[0].speed.z = 0.0F;
        } else {
            VARS(work)->object = VARS(work)->model.action->object;
        }
        break;
    case PLATFORM_BOTTOM:
        crane_follow(work);
        crane_platform_draw(task);
        if (!edit_mode && crane_platform_holds(task)) {
            work->mode = PLATFORM_CLIMB;
            MOVING(work) = TRUE;
            crane_platform_lamps(work);
        }
        break;
    case PLATFORM_CLIMB:
        crane_follow(work);
        crane_platform_draw(task);
        if (work->pos.y - crane->pos.y > PLATFORM_HEIGHT) {
            work->mode = PLATFORM_TOP;
            MOVING(work) = FALSE;
            force[0].speed.x = 0.0F;
            force[0].speed.y = 0.0F;
            force[0].speed.z = 0.0F;
            crane_platform_lamps(work);
            sound_stop(SE_PLATFORM_MOVE);
            sound_play(SE_PLATFORM_STOP, 0, 0, 0);
        } else {
            /* @hack spelling: speed, a copy of PLATFORM_SPEED, which SHC
             * loads once into FR14 and keeps across njSin and njCos
             * (0x0c900f9e); without it, the function runs 32 bytes shorter
             * and 387 of 421 words differ */
            Float speed = PLATFORM_SPEED;

            force[0].speed.x = speed * njSin(work->ang.y);
            force[0].speed.y = PLATFORM_RISE;
            force[0].speed.z = speed * njCos(work->ang.y);
            work->pos.x += force[0].speed.x;
            work->pos.y += force[0].speed.y;
            work->pos.z += force[0].speed.z;
            sound_play_at_for(SE_PLATFORM_MOVE, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 2);
        }
        break;
    case PLATFORM_TOP:
        crane_follow(work);
        crane_platform_draw(task);
        if (crane_platform_holds(task)) {
            if (TASK_TIMER(work)++ > PLATFORM_WAIT) {
                work->mode = PLATFORM_DESCEND;
                MOVING(work) = TRUE;
                work->timer = 0;
                crane_platform_lamps(work);
            }
        }
        break;
    case PLATFORM_DESCEND:
        crane_follow(work);
        crane_platform_draw(task);
        if (work->pos.y - crane->pos.y <= 0.0F) {
            work->mode = PLATFORM_LANDED;
            MOVING(work) = FALSE;
            force[0].speed.x = 0.0F;
            force[0].speed.y = 0.0F;
            force[0].speed.z = 0.0F;
            crane_platform_lamps(work);
            sound_stop(SE_PLATFORM_MOVE);
            sound_play(SE_PLATFORM_STOP, 0, 0, 0);
        } else {
            /* @hack spelling: the same for the descent, loaded at 0x0c90108a;
             * without it, the function runs 32 bytes shorter and 384 of 421
             * words differ */
            Float speed = PLATFORM_SPEED;

            force[0].speed.x = -speed * njSin(work->ang.y);
            force[0].speed.y = -PLATFORM_RISE;
            force[0].speed.z = -speed * njCos(work->ang.y);
            work->pos.x += force[0].speed.x;
            work->pos.y += force[0].speed.y;
            work->pos.z += force[0].speed.z;
            sound_play_at_for(SE_PLATFORM_MOVE, (int)work, 1, 0, work->pos.x,
                              work->pos.y, work->pos.z, 2);
        }
        break;
    case PLATFORM_LANDED:
        crane_follow(work);
        crane_platform_draw(task);
        if (crane_platform_holds(task)) {
            work->mode = PLATFORM_BOTTOM;
            work->timer = 0;
        }
        break;
    case PLATFORM_GONE:
    default:
        crane_remove(task);
        break;
    }
    if ((FRAME(work) += PLATFORM_FRAME_STEP) >=
        VARS(work)->model.action->motion->nbFrame) {
        FRAME(work) = 0.0F;
    }
    force[1] = force[0];
    player_add_target(work->pos.x, work->pos.y, work->pos.z, PLATFORM_TARGET);
}

static void crane_platform_create(struct task *task)
{
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as two vectors. the image's ADD #-28,R15 reserves them
     * (0x0c9011cc); without them, 2 of 19 words differ */
    NJS_VECTOR unused[2];
    struct task *platform =
        task_create_child(TASK_WORK | TASK_FORCE, crane_platform_step, task);

    if (platform != NULL) {
        struct task_work *work = platform->work;

        work->ang.x = work->ang.z = 0;
    }
}
