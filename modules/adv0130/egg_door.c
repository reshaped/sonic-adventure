/* O EGGDOOR, a door that slides open for the players its kind allows */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/egg_door.h"
#include "adv0130/tarai.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "event/play.h"
#include "object/placement.h"
#include "object/switch.h"
#include "player/player.h"
#include "progress/progress.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION egg_door_action;

/* the door opens for the nearest player at NEAR or nearer. it stays open
 * while the nearest player, of any character, is nearer than NEAR. it
 * stands open at OPEN_FRAME, just short of its action's last frame, 14. it
 * moves OPEN_SPEED action frames a step as it opens, and SHUT_SPEED as it
 * shuts */
#define NEAR 50.0F
#define OPEN_FRAME 13.99F
#define OPEN_SPEED 1.0F
#define SHUT_SPEED (-1.0F)

#define EVENT_AMY_FREED 191 /* EV00BF: E-102 frees Amy and the bird */
#define EVENT_WRONG_ROOM 190 /* EV00BE: E-102 finds the wrong room */

#define SWITCH_DOOR_0 0
#define SWITCH_DOOR_1 1

enum egg_door_mode {
    DOOR_INIT,
    DOOR_OPENING,
    DOOR_CLOSING,
    DOOR_SHUT,
    DOOR_OPEN
};

/* in work->submode, from the set entry's z angle */
enum egg_door_kind {
    DOOR_E102_AFTER, /* E-102, once EVENT_AMY_FREED has played */
    DOOR_FOR_ALL,
    DOOR_E102_OR_SWITCH, /* E-102, or anyone while SWITCH_DOOR_0 is on */
    DOOR_TARAI, /* anyone, once the washbasin puzzle is solved */
    /* E-102, Big, and Amy once she has the Warrior Feather */
    DOOR_E102_BIG_AMY,
    DOOR_E102_UNTIL, /* E-102, until EVENT_WRONG_ROOM has played */
    /* anyone while SWITCH_DOOR_1 is on, and otherwise E-102 alone, once his
     * story sets FLAG_E102_EC_HOT_SHELTER */
    DOOR_SWITCH_OR_E102
};

struct egg_door_vars {
    Uint8 unk_00[8];
    Float frame;
};

#define VARS(work) ((struct egg_door_vars *)&(work)->vars)

static void egg_door_draw(struct task *task);
static void egg_door_draw_work(struct task_work *work);
static Bool egg_door_opens(struct task_work *work);

/* shape 0 is a box across the shut doorway, and shapes 1 and 2 are boxes at
 * its sides. shape 3 is a capsule lying across the doorway near its top. the
 * step turns shape 0 off as the door starts to open, and on again once it is
 * shut. it turns the side boxes the other way. all four are on until the door
 * first opens */
static struct collision_shape egg_door_shapes[4] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 20.0F, 2.0F},
     20.2F,
     20.2F,
     5.2F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-17.5F, 20.0F, 2.0F},
     2.7F,
     20.2F,
     5.2F,
     {0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {17.5F, 20.0F, 2.0F},
     2.7F,
     20.2F,
     5.2F,
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

void egg_carrier_egg_door_step(struct task *task)
{
    struct task_work *work = task->work;
    int player;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case DOOR_INIT:
        VARS(work)->frame = 0.0F;
        work->submode = work->ang.z;
        work->ang.z = 0;
        if (!placement_editing(task)) {
            collision_init(task, egg_door_shapes, COLLISION_OBJECT, 1, 4);
        }
        task->draw = egg_door_draw;
        work->mode = DOOR_SHUT;
        break;
    case DOOR_SHUT:
        if (!egg_door_opens(work)) {
            break;
        }
        work->collision->shapes[0].flags |= SHAPE_OFF;
        work->collision->shapes[1].flags &= ~SHAPE_OFF;
        work->collision->shapes[2].flags &= ~SHAPE_OFF;
        work->mode = DOOR_OPENING;
        break;
    case DOOR_OPEN:
        player = player_nearest(&work->pos);
        if (njDistanceP2P(&player_work[player]->pos, &work->pos) < NEAR) {
            break;
        }
        work->mode = DOOR_CLOSING;
        break;
    case DOOR_OPENING:
        VARS(work)->frame += OPEN_SPEED;
        if (VARS(work)->frame < OPEN_FRAME) {
            break;
        }
        VARS(work)->frame = OPEN_FRAME;
        work->mode = DOOR_OPEN;
        break;
    case DOOR_CLOSING:
        VARS(work)->frame += SHUT_SPEED;
        if (VARS(work)->frame > 0.0F) {
            break;
        }
        VARS(work)->frame = 0.0F;
        work->collision->shapes[0].flags &= ~SHAPE_OFF;
        work->collision->shapes[1].flags |= SHAPE_OFF;
        work->collision->shapes[2].flags |= SHAPE_OFF;
        work->mode = DOOR_SHUT;
        break;
    }
    egg_door_draw_work(work);
    if (!placement_editing(task)) {
        collision_register(work);
    }
}

static void egg_door_draw(struct task *task)
{
    egg_door_draw_work(task->work);
}

static void egg_door_draw_work(struct task_work *work)
{
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(&egg_door_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* TRUE when the nearest player is within NEAR and the door's kind opens for
 * that player's character, else FALSE */
static Bool egg_door_opens(struct task_work *work)
{
    int player = player_nearest(&work->pos);
    int character;

    if (njDistanceP2P(&player_work[player]->pos, &work->pos) > NEAR) {
        return FALSE;
    }
    character = player_get_character(player);
    switch (work->submode) {
    case DOOR_E102_AFTER:
        if (character == CHARACTER_E102) {
            return event_was_played(EVENT_AMY_FREED);
        }
        return FALSE;
    case DOOR_FOR_ALL:
        break;
    case DOOR_E102_OR_SWITCH:
        if (character != CHARACTER_E102 && !switch_is_on(SWITCH_DOOR_0)) {
            return FALSE;
        }
        break;
    case DOOR_TARAI:
        if (event_flags[FLAG_TARAI_SOLVED]) {
            break;
        }
        return FALSE;
    case DOOR_E102_BIG_AMY:
        if (character != CHARACTER_E102 && character != CHARACTER_BIG &&
            (character != CHARACTER_AMY ||
             !event_flag_check(FLAG_AMY_WARRIOR_FEATHER))) {
            return FALSE;
        }
        break;
    case DOOR_E102_UNTIL:
        if (character != CHARACTER_E102) {
            return FALSE;
        }
        if (event_was_played(EVENT_WRONG_ROOM) == TRUE) {
            return FALSE;
        }
        break;
    case DOOR_SWITCH_OR_E102:
        if (switch_is_on(SWITCH_DOOR_1) == TRUE) {
            break;
        }
        if (character != CHARACTER_E102) {
            return FALSE;
        }
        return event_flag_check(FLAG_E102_EC_HOT_SHELTER);
    }
    return TRUE;
}
