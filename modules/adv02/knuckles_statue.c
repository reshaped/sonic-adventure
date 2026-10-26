/* the statue the object list names O KNUCKLES STUTUE, as the disc spells it.
 * it stands in the jungle, and moves aside once FLAG_SONIC_MR_ENTRANCE_RUIN is
 * set */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/knuckles_statue.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the action that moves the statue aside, and the statue's collision models,
 * standing and aside */
extern NJS_ACTION knuckles_statue_action;
extern NJS_OBJECT knuckles_statue_object;
/* @hack type: declared an array, not the one NJS_OBJECT it is, so SHC pools
 * its address at 0x0c90b9d0 and reads model through it, MOV.L @(4,R3),R1 at
 * 0x0c90b96e; declared as one, 7 of knuckles_statue_step's 110 words and a
 * pool word differ */
extern NJS_OBJECT knuckles_statue_aside_object[];

#define FRAME_SPEED 0.2F /* the action frames the statue advances each step */
/* the action's last frame, with the statue aside */
#define ASIDE_FRAME 53.99F

/* the collision world tests the statue only while a player is this near */
#define NEAR 100.0F

#define STATUE_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

enum knuckles_statue_mode {
    STATUE_INIT,
    STATUE_STANDING,
    STATUE_MOVING,
    STATUE_ASIDE
};

struct knuckles_statue_vars {
    NJS_OBJECT *object; /* the statue's copy in the collision world */
    NJS_OBJECT *from; /* what knuckles_statue_add_to_world copies */
    Float frame;
};

#define VARS(work) ((struct knuckles_statue_vars *)&(work)->vars)

static void knuckles_statue_draw(struct task *task);
static void knuckles_statue_destroy(struct task *task);
static void knuckles_statue_add_to_world(struct task *task);

/* the statue tests FLAG_SONIC_MR_ENTRANCE_RUIN, Sonic's flag, whoever plays.
 * it moves aside for any character once Sonic has set the flag, and it starts
 * aside only for Sonic */
void knuckles_statue_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case STATUE_INIT:
            if (get_character() == CHARACTER_SONIC &&
                event_flag_check(FLAG_SONIC_MR_ENTRANCE_RUIN)) {
                VARS(work)->from = knuckles_statue_aside_object;
                VARS(work)->frame = ASIDE_FRAME;
                work->mode = STATUE_ASIDE;
            } else {
                VARS(work)->from = &knuckles_statue_object;
                VARS(work)->frame = 0.0F;
                work->mode = STATUE_STANDING;
            }
            knuckles_statue_add_to_world(task);
            task->draw = knuckles_statue_draw;
            task->destructor = knuckles_statue_destroy;
            /* every statue starts in STATUE_STANDING: this replaces the mode
             * either branch set. a statue that starts aside already holds
             * ASIDE_FRAME, and its next two steps take it through
             * STATUE_MOVING back to STATUE_ASIDE */
            work->mode = STATUE_STANDING;
            break;
        case STATUE_STANDING:
            if (event_flag_check(FLAG_SONIC_MR_ENTRANCE_RUIN)) {
                work->mode = STATUE_MOVING;
            }
            break;
        case STATUE_MOVING:
            VARS(work)->frame += FRAME_SPEED;
            if (VARS(work)->frame >= ASIDE_FRAME) {
                VARS(work)->frame = ASIDE_FRAME;
                work->mode = STATUE_ASIDE;
                VARS(work)->object->model = knuckles_statue_aside_object->model;
            }
            break;
        case STATUE_ASIDE:
        default:
            break;
        }
        work->flags &= ~WORLD_TESTED;
        if (player_in_range(&work->pos, NEAR) > 0) {
            work->flags |= WORLD_TESTED;
        }
        knuckles_statue_draw(task);
    }
}

static void knuckles_statue_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&knuckles_statue_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void knuckles_statue_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->object != NULL) {
        world_remove(task, VARS(work)->object);
        world_object_free(VARS(work)->object);
    }
}

static void knuckles_statue_add_to_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *from = VARS(work)->from;
    NJS_OBJECT *object;

    VARS(work)->object = object = world_object_alloc();
    object->evalflags = NJD_EVAL_BREAK;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = work->pos.x;
    object->pos[1] = work->pos.y;
    object->pos[2] = work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(STATUE_FLAGS, task, object);
}
