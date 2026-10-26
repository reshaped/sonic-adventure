/* O CASINO DOOR the casino's double doors, which swing open once the casino
 * button is pushed */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/casino_door.h"
#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the frame, with the panels as its child and the child's sibling */
extern NJS_OBJECT casino_door_object;

#define DOOR_SPEED 0.02F
#define PANEL_TURN 2731.0F /* each panel's turn at full swing (15 degrees) */
#define SE_DOOR 530

/* by night the doors play SE_NIGHT_OPEN while they are unlocked and player 0
 * is within OPEN_HEARD. otherwise they play SE_NIGHT while player 0 is within
 * HEARD. both distances are in x and z, squared */
#define SE_NIGHT_OPEN 1049
#define SE_NIGHT 1047
#define OPEN_HEARD (50.0F * 50.0F)
#define HEARD (180.0F * 180.0F)

struct casino_door_vars {
    Float swing; /* 0 shut, 1 open */
    Uint8 unk_04[4];
    Float speed; /* swing a step, before casino_door_swing eases it */
};

#define VARS(work) ((struct casino_door_vars *)&(work)->vars)

enum casino_door_mode { DOOR_SHUT = 0, DOOR_SWINGING = 1, DOOR_OPEN = 4 };

/* the jambs and open panels lie out along x or along z */
enum casino_door_shape { DOORS_SHUT, JAMB_X, JAMB_Z, PANEL_X, PANEL_Z };

static void casino_door_step(struct task *task);
static void casino_door_draw(struct task *task);

/* clang-format off */
static struct collision_shape casino_door_shapes[] = {
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {8.0F, 12.0F, 8.0F},
     23.0F, 12.0F, 1.0F, {0, 0x2000, 0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {25.0F, 12.0F, -10.0F},
     5.0F, 12.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-10.0F, 12.0F, 25.0F},
     5.0F, 12.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {20.0F, 9.5F, -10.0F},
     5.0F, 9.5F, 0.5F, {0}, 0, NULL},
    {SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL, {-10.0F, 9.5F, 20.0F},
     0.5F, 9.5F, 5.0F, {0}, 0, NULL},
};
/* clang-format on */

/* TRUE once the casino button is pushed for Sonic, Tails, Knuckles or Amy */
static Bool casino_door_unlocked(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_SS_ENTRANCE_CASINO];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_SS_ENTRANCE_CASINO];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_SS_ENTRANCE_CASINO];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_SS_ENTRANCE_CASINO];
    case CHARACTER_E102:
    case CHARACTER_BIG:
        break;
    }
    return FALSE;
}

/* turn on the shape across the doorway while the doors are shut, or the open
 * panels' shapes while they swing or stand open. the jambs' shapes stay on.
 * then register the record */
static void casino_door_collide(struct task_work *work)
{
    struct collision_shape *shapes = work->collision->shapes;

    if (work->mode == DOOR_SHUT) {
        shapes[DOORS_SHUT].flags &= ~SHAPE_OFF;
        shapes[JAMB_X].flags &= ~SHAPE_OFF;
        shapes[JAMB_Z].flags &= ~SHAPE_OFF;
        shapes[PANEL_X].flags |= SHAPE_OFF;
        shapes[PANEL_Z].flags |= SHAPE_OFF;
    } else if (work->mode == DOOR_SWINGING || work->mode == DOOR_OPEN) {
        shapes[DOORS_SHUT].flags |= SHAPE_OFF;
        shapes[JAMB_X].flags &= ~SHAPE_OFF;
        shapes[JAMB_Z].flags &= ~SHAPE_OFF;
        shapes[PANEL_X].flags &= ~SHAPE_OFF;
        shapes[PANEL_Z].flags &= ~SHAPE_OFF;
    }
    collision_register(work);
}

/* advance swing by speed times a factor of |swing|: 5(|swing| + 0.1) below
 * 0.3, 2 below 0.7, and 1.4 / |swing| from there. the factor rises from 0.5
 * to 2, then eases off to 1.4 at open */
static void casino_door_swing(struct task_work *work)
{
    Float swing = njAbs(VARS(work)->swing);

    /* @hack spelling: each sum written out, which loads each constant with
     * MOVA (0x0c90d34e, 0x0c90d35c, 0x0c90d376, 0x0c90d386). with +=, SHC
     * moves each through FPUL from a MOV.L, and 38 of 47 words differ */
    if (swing < 0.3F) {
        VARS(work)->swing =
            VARS(work)->swing + (VARS(work)->speed * (swing + 0.1F) * 5.0F);
    } else if (swing < 0.7F) {
        VARS(work)->swing = VARS(work)->swing + (VARS(work)->speed * 2.0F);
    } else {
        VARS(work)->swing =
            VARS(work)->swing + (VARS(work)->speed * 1.4F / swing);
    }
}

void casino_door_init(struct task *task)
{
    struct task_work *work = task->work;

    if ((Uint8)stage_get_stage_act() != STATION_SQUARE_CASINO) {
        njPrintC(NJM_LOCATION(10, 8), "ACT CASINO ONLY !");
        return;
    }
    if (!placement_try_leave(task)) {
        work->ang.x = 0;
        work->ang.z = 0;
        VARS(work)->speed = 0.0F;
        work->submode = 0;
        work->tick = 0;
        if (!placement_editing(task)) {
            if (casino_door_unlocked()) {
                VARS(work)->swing = 1.0F;
                work->mode = DOOR_OPEN;
            } else {
                work->mode = DOOR_SHUT;
                VARS(work)->swing = 0.0F;
            }
            collision_init(task, casino_door_shapes, COLLISION_OBJECT, 1, 5);
        }
        task->step = casino_door_step;
        task->draw = casino_door_draw;
    }
}

static void casino_door_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    Float x;
    Float z;
    Float distance;

    if (placement_try_leave(task)) {
        return;
    }
    if (story_get_time() == TIME_NIGHT) {
        player_get_history(0, 0, &pos, NULL);
        x = work->pos.x - pos.x;
        z = work->pos.z - pos.z;
        distance = (x * x) + (z * z);
        if (distance < OPEN_HEARD && casino_door_unlocked()) {
            sound_play_for(SE_NIGHT_OPEN, (int)task, 1, 0, 2);
        } else if (distance < HEARD) {
            sound_play_for(SE_NIGHT, (int)task, 1, 0, 2);
        }
    }
    switch (work->mode) {
    case DOOR_SHUT:
        if (casino_door_unlocked()) {
            work->mode = DOOR_SWINGING;
            VARS(work)->speed = DOOR_SPEED;
            sound_play(SE_DOOR, 0, 0, 0);
        }
        break;
    case DOOR_SWINGING:
        casino_door_swing(work);
        if (VARS(work)->swing > 1.0F) {
            VARS(work)->swing = 1.0F;
            work->mode = DOOR_OPEN;
            work->timer = 60; /* no reader of the door's timer is known */
        }
        break;
    case DOOR_OPEN:
        break;
    }
    if (!placement_editing(task)) {
        casino_door_collide(work);
    }
    casino_door_draw(task);
}

static void casino_door_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle turn = (Angle)(VARS(work)->swing * PANEL_TURN);
    Angle ang;
    NJS_OBJECT *panel;

    njSetTexture(&ss_casino_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(casino_door_object.model);
    panel = casino_door_object.child;
    njPushMatrix(NULL);
    njTranslate(NULL, panel->pos[0], panel->pos[1], panel->pos[2]);
    TURN(njRotateY, turn);
    draw_model(panel->model);
    njPopMatrix(1);
    panel = panel->sibling;
    njTranslate(NULL, panel->pos[0], panel->pos[1], panel->pos[2]);
    TURN_VIA(njRotateY, -turn, ang);
    draw_model(panel->model);
    njPopMatrix(1);
}
