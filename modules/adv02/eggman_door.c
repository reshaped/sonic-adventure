/* the door the object list names O EGGMAN DOOR. it opens as player 0 comes
 * near, for the characters its kind admits */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/eggman_door.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION eggman_door_action;

#define NEAR 50.0F /* the door opens while player 0 is this near */
#define OPEN_FRAME 13.99F /* the action's frame when fully open */
/* the action frames the door moves through each step, opening and shutting */
#define OPEN_SPEED 1.0F
#define SHUT_SPEED (-1.0F)

#define SE_DOOR 586

#define EVENT_E102_PASS 177

enum eggman_door_mode {
    DOOR_INIT,
    DOOR_OPENING,
    DOOR_CLOSING,
    DOOR_SHUT,
    DOOR_OPEN
};

/* the door's kinds, which say whom it opens for. DOOR_INIT moves the kind
 * from the set entry's x angle to work->submode */
enum eggman_door_kind {
    /* Sonic and Amy, and E-102 once FLAG_E102_MR_FREE_PASS is set */
    DOOR_FOR_SONIC_AMY,
    DOOR_FOR_E102, /* E-102 once EVENT_E102_PASS has played */
    DOOR_LOCKED, /* no one */
    /* Sonic, Amy and E-102. the door is there only once the character's
     * FLAG_*_MR_APPEAR_FINAL_EGG is set */
    DOOR_HIDDEN
};

struct eggman_door_vars {
    Uint8 unk_00[8];
    Float frame;
};

#define VARS(work) ((struct eggman_door_vars *)&(work)->vars)

static void eggman_door_destroy(struct task *task);
static void eggman_door_draw(struct task *task);
static void eggman_door_draw_work(struct task_work *work);
static Bool eggman_door_opens(struct task_work *work);
static Bool eggman_door_present(struct task_work *work);

/* a box across the shut doorway, a box at each side of the open door, and a
 * capsule across the top of the doorway */
enum { SHUT_SHAPE, LEFT_SHAPE, RIGHT_SHAPE, TOP_SHAPE };
static struct collision_shape eggman_door_shapes[4] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 20.0F, 2.0F},
     20.2F,
     20.2F,
     2.2F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-17.5F, 20.0F, 2.0F},
     2.7F,
     20.2F,
     2.2F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {17.5F, 20.0F, 2.0F},
     2.7F,
     20.2F,
     2.2F,
     {0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 32.0F, 4.0F},
     6.0F,
     11.0F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
};

/* eggman_door_present tests work->submode before DOOR_INIT moves the kind
 * there. a new task's submode is 0, DOOR_FOR_SONIC_AMY, which passes, so
 * DOOR_INIT always runs */
void eggman_door_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!eggman_door_present(work)) {
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        VARS(work)->frame = 0.0F;
        work->submode = work->ang.x;
        work->ang.x = 0;
        collision_init(task, eggman_door_shapes, COLLISION_OBJECT, 1, 4);
        task->destructor = eggman_door_destroy;
        task->draw = eggman_door_draw;
        work->mode = DOOR_SHUT;
        break;
    case DOOR_SHUT:
        if (player_in_range(&work->pos, NEAR) == 1 && eggman_door_opens(work)) {
            work->collision->shapes[SHUT_SHAPE].flags |= SHAPE_OFF;
            work->collision->shapes[LEFT_SHAPE].flags &= ~SHAPE_OFF;
            work->collision->shapes[RIGHT_SHAPE].flags &= ~SHAPE_OFF;
            sound_play(SE_DOOR, 0, 0, 0);
            work->mode = DOOR_OPENING;
        }
        break;
    case DOOR_OPEN:
        if (player_in_range(&work->pos, NEAR) != 1) {
            sound_play(SE_DOOR, 0, 0, 0);
            work->mode = DOOR_CLOSING;
        }
        break;
    case DOOR_OPENING:
        VARS(work)->frame += OPEN_SPEED;
        if (VARS(work)->frame >= OPEN_FRAME) {
            VARS(work)->frame = OPEN_FRAME;
            work->mode = DOOR_OPEN;
        }
        break;
    case DOOR_CLOSING:
        VARS(work)->frame += SHUT_SPEED;
        if (VARS(work)->frame <= 0.0F) {
            VARS(work)->frame = 0.0F;
            work->collision->shapes[SHUT_SHAPE].flags &= ~SHAPE_OFF;
            work->collision->shapes[LEFT_SHAPE].flags |= SHAPE_OFF;
            work->collision->shapes[RIGHT_SHAPE].flags |= SHAPE_OFF;
            work->mode = DOOR_SHUT;
        }
        break;
    }
    eggman_door_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void eggman_door_destroy(struct task *task)
{
}

static void eggman_door_draw(struct task *task)
{
    if (eggman_door_present(task->work)) {
        eggman_door_draw_work(task->work);
    }
}

static void eggman_door_draw_work(struct task_work *work)
{
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(&eggman_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

static Bool eggman_door_opens(struct task_work *work)
{
    int character = get_character();

    switch (work->submode) {
    case DOOR_FOR_SONIC_AMY:
        switch (character) {
        case CHARACTER_SONIC:
        case CHARACTER_AMY:
            return TRUE;
        case CHARACTER_E102:
            return event_flag_check(FLAG_E102_MR_FREE_PASS);
        }
        break;
    case DOOR_FOR_E102:
        return character == CHARACTER_E102 && event_was_played(EVENT_E102_PASS);
    case DOOR_LOCKED:
        break;
    case DOOR_HIDDEN:
        switch (character) {
        case CHARACTER_SONIC:
        case CHARACTER_AMY:
        case CHARACTER_E102:
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static Bool eggman_door_present(struct task_work *work)
{
    int character = get_character();

    switch (work->submode) {
    case DOOR_FOR_SONIC_AMY:
    case DOOR_FOR_E102:
    case DOOR_LOCKED:
        return TRUE;
    case DOOR_HIDDEN:
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
        break;
    }
    return FALSE;
}
