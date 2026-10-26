/* the door the object list names O MOUNTAIN DOOR: a gate on the way to Red
 * Mountain, which the monkey cages open */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/monkey_cage.h"
#include "adv02/mountain_door.h"
#include "adv02/texlists.h"
#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the door's collision model, and the action that shuts the door */
extern NJS_OBJECT mountain_door_object;
/* @hack type: declared an array, not the one NJS_ACTION it is, so SHC pools
 * its address, at 0x0c907a60 and 0x0c907b4c, and reads motion through it, as
 * MOV.L @(4,R2),R3 at 0x0c907958 and MOV.L @(4,R4),R3 at 0x0c907aca; declared
 * as one, 3 of mountain_door_reset's 47 words, 36 of mountain_door_update's
 * 160 and 2 pool words differ */
extern NJS_ACTION mountain_door_action[];

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)
/* the collision world tests the door only while a player is this near */
#define NEAR 80.0F

#define RADIUS 60.0F /* the collision record's radius */

#define FRAME_SPEED 0.8F /* the action frames the door swings each step */
/* Knuckles has come through the first door once he is within 70 units of it,
 * at a z below PAST in the door's frame. THROUGH is the 70 squared */
#define THROUGH (70.0F * 70.0F)
#define PAST (-5.0F)

#define DOOR_FLAGS (WORLD_MEDIUM | WORLD_UNCLIMBABLE | WORLD_SOLID)

#define SE_DOOR 589

enum mountain_door_mode { MOUNTAIN_DOOR_INIT, MOUNTAIN_DOOR_RUN };

/* the door's kinds, in work->tick, read from its x angle */
enum mountain_door_kind { DOOR_FIRST, DOOR_SECOND, DOOR_KINDS };

/* the door's states, in work->submode */
enum mountain_door_state { DOOR_OPENING, DOOR_OPEN, DOOR_SHUTTING, DOOR_SHUT };

struct mountain_door_vars {
    Float frame;
    Uint8 unk_04[4];
    NJS_OBJECT *object; /* the door's copy in the collision world */
};

#define VARS(work) ((struct mountain_door_vars *)&(work)->vars)
#define KIND(work) ((work)->tick)
/* the action's last frame, with the door shut */
#define SHUT_FRAME (mountain_door_action->motion->nbFrame - 1.0F)

static void mountain_door_init(struct task *task);
static void mountain_door_reset(struct task *task);
static void mountain_door_add_to_world(struct task *task);
static void mountain_door_update(struct task *task);
static void mountain_door_draw(struct task *task);
static void mountain_door_destroy(struct task *task);
static Bool mountain_door_opened(struct task_work *work);

/* a box round the door, which pushes every group and which no group can push.
 * mountain_door_update registers it in every state but DOOR_OPEN */
static struct collision_shape mountain_door_shape[1] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 30.0F, 0.0F},
     20.0F,
     32.0F,
     10.0F,
     {0},
     0,
     NULL},
};

/* while the set editor holds the door, each step rebuilds it and labels the
 * editor's fields. the field it labels MOTION SPEED(*10), scale x, is never
 * read */
void mountain_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case MOUNTAIN_DOOR_INIT:
            mountain_door_init(task);
            break;
        case MOUNTAIN_DOOR_RUN:
            mountain_door_draw(task);
            mountain_door_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case MOUNTAIN_DOOR_INIT:
            mountain_door_init(task);
            break;
        case MOUNTAIN_DOOR_RUN:
            mountain_door_destroy(task);
            mountain_door_reset(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ID(0-1)");
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> MOTION SPEED(*10)");
            mountain_door_draw(task);
            mountain_door_update(task);
            break;
        }
    }
}

static void mountain_door_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = mountain_door_draw;
    task->destructor = mountain_door_destroy;
    collision_init(task, mountain_door_shape, COLLISION_OBJECT, 1, 1);
    work->collision->radius = RADIUS;
    VARS(work)->frame = 0.0F;
    mountain_door_reset(task);
    work->mode = MOUNTAIN_DOOR_RUN;
}

/* put the door in the collision world and read its kind from its x angle,
 * clamped to the door kinds. the door starts open or shut, as
 * mountain_door_opened says */
static void mountain_door_reset(struct task *task)
{
    struct task_work *work = task->work;

    mountain_door_add_to_world(task);
    KIND(work) = work->ang.x;
    /* @bug logic: work->tick is a Uint8, so the test below DOOR_FIRST,
     * CMP/PZ at 0x0c907930 after EXTU.B, never holds. an x angle of -1, whose
     * low byte is 255, gives DOOR_SECOND rather than DOOR_FIRST */
    if (KIND(work) < DOOR_FIRST) {
        KIND(work) = DOOR_FIRST;
    } else if (KIND(work) >= DOOR_KINDS) {
        KIND(work) = DOOR_SECOND;
    }
    if (mountain_door_opened(work)) {
        VARS(work)->frame = 0.0F;
        work->submode = DOOR_OPEN;
    } else {
        VARS(work)->frame = SHUT_FRAME;
        work->submode = DOOR_SHUT;
    }
}

/* put a copy of mountain_door_object into the collision world, moved by the
 * door's position and turned to its y angle. the task's force is zeroed
 * first, so a player standing on the copy isn't moved */
static void mountain_door_add_to_world(struct task *task)
{
    struct task_force *force = task->force;
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    NJS_OBJECT *from;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    from = &mountain_door_object;
    object = world_object_alloc();
    VARS(work)->object = object;
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = work->ang.y;
    object->ang[2] = 0;
    object->pos[0] = from->pos[0] + work->pos.x;
    object->pos[1] = from->pos[1] + work->pos.y;
    object->pos[2] = from->pos[2] + work->pos.z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(DOOR_FLAGS, task, object);
}

/* swing the door open or shut, as mountain_door_opened says. once the second
 * door has shut, set the Red Mountain entrance flag of the character played,
 * if that is Sonic, Knuckles or E-102 */
static void mountain_door_update(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->submode) {
    case DOOR_OPENING:
        VARS(work)->frame -= FRAME_SPEED;
        if (VARS(work)->frame < 0.0F) {
            VARS(work)->frame = 0.0F;
            work->submode = DOOR_OPEN;
        }
        collision_register(work);
        if (!mountain_door_opened(work)) {
            work->submode = DOOR_SHUTTING;
        }
        break;
    case DOOR_OPEN:
        if (!mountain_door_opened(work)) {
            sound_play(SE_DOOR, 0, 0, 0);
            work->submode = DOOR_SHUTTING;
        }
        break;
    case DOOR_SHUTTING:
        VARS(work)->frame += FRAME_SPEED;
        if (VARS(work)->frame >= SHUT_FRAME) {
            VARS(work)->frame = SHUT_FRAME;
            work->submode = DOOR_SHUT;
            if (KIND(work) == DOOR_SECOND) {
                switch (get_character()) {
                case CHARACTER_SONIC:
                    event_flag_set(FLAG_SONIC_MR_ENTRANCE_MOUNTAIN);
                    break;
                case CHARACTER_KNUCKLES:
                    event_flag_set(FLAG_KNUCKLES_MR_ENTRANCE_MOUNTAIN);
                    break;
                case CHARACTER_E102:
                    event_flag_set(FLAG_E102_MR_ENTRANCE_MOUNTAIN);
                    break;
                }
            }
        }
        if (mountain_door_opened(work)) {
            work->submode = DOOR_OPENING;
        }
        collision_register(work);
        break;
    case DOOR_SHUT:
        if (mountain_door_opened(work)) {
            work->submode = DOOR_OPENING;
            sound_play(SE_DOOR, 0, 0, 0);
        }
        collision_register(work);
        break;
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void mountain_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&mrobj_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(mountain_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

static void mountain_door_destroy(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

/* TRUE while the door should stand open, else FALSE. the first door stands
 * open for every character but Knuckles. for him it stands open until he
 * comes through it, which sets FLAG_KNUCKLES_MR_MONKEY_DOOR_ENTERED. it then
 * stands open only while the first cage's flag is set. the second door
 * stands open while the second cage's flag is set */
static Bool mountain_door_opened(struct task_work *work)
{
    Bool open;
    NJS_POINT3 player;
    NJS_POINT3 local;
    Float x;
    Float y;
    Float z;
    Angle ang;

    switch (KIND(work)) {
    case DOOR_FIRST:
        if (player_get_character(0) == CHARACTER_KNUCKLES) {
            if (event_flags[FLAG_KNUCKLES_MR_MONKEY_DOOR_ENTERED] == 0) {
                player = player_work[0]->pos;
                njPushMatrix(&_unit_matrix_);
                njTranslateV(NULL, &work->pos);
                TURN_VIA(njRotateY, work->ang.y, ang);
                njInvertMatrix(NULL);
                njCalcPoint(NULL, &player, &local);
                njPopMatrix(1);
                x = local.x;
                y = local.y;
                z = local.z;
                if ((x * x) + (y * y) + (z * z) < THROUGH) {
                    if (z < PAST) {
                        event_flags[FLAG_KNUCKLES_MR_MONKEY_DOOR_ENTERED] =
                            TRUE;
                        open = FALSE;
                    } else {
                        open = TRUE;
                    }
                } else {
                    open = TRUE;
                }
            } else {
                open = monkey_cage_flag(0);
            }
        } else {
            open = TRUE;
        }
        break;
    case DOOR_SECOND:
        open = monkey_cage_flag(1);
        break;
    }
    return open;
}
