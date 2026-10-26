/* the escalator O ESCALATOR: nine stairs that run up its x axis. a set file's
 * scale x below 0 runs them down instead. it carries a player that touches it
 * along world x and up. a down escalator carries the player back and down. the
 * carry ignores the escalator's yaw, so a turned escalator would carry the
 * player off its stairs' line. SET0602S.BIN and SET0602K.BIN place each of
 * their four unturned */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg06/escalator.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION escalator_stair_action;
extern NJS_OBJECT escalator_down_object;
extern NJS_OBJECT escalator_up_object;

/* the square of the distance from the camera or player 0 within which the
 * escalator stays */
#define ESCALATOR_RANGE (1010.0F * 1010.0F)
/* how far its shape reaches from its position */
#define ESCALATOR_REACH 100.0F
/* how far it carries a player each step: along world x, and up */
#define CARRY_ALONG 0.9F
#define CARRY_UP 0.5F

enum escalator_mode { ESCALATOR_INIT, ESCALATOR_NORMAL, ESCALATOR_END };
enum escalator_way { ESCALATOR_DOWN, ESCALATOR_UP };

static void escalator_stair_step(struct task *task);

/* names for five modes, which nothing reads. the first three are
 * escalator_mode's; FREE and HOLD are none of its */
static char *escalator_mode_names[5] = {"INIT", "NORMAL", "END", "FREE",
                                        "HOLD"};

/* a box along the stairs, tilted to their slope */
static struct collision_shape escalator_shapes[1] = {
    {SHAPE_RECTANGLE2,
     0,
     PUSH_ALL,
     {40.0F, 22.5F, 0.0F},
     51.5F,
     5.0F,
     50.0F,
     {0, 0, 0x14a8},
     0,
     NULL},
};

/* a stair's spec, at x along the escalator and y up it. a down stair is turned
 * half a turn about y. an up stair isn't, and its work's flags are 1, which no
 * code here reads. NO_STAIR, a spec with a null step, ends a table of specs */
#define DOWN_STAIR(x, y)                                                       \
    {escalator_stair_step, TASK_WORK, 0, {0, 0x8000, 0}, {(x), (y), 0.0F}, 0}
#define UP_STAIR(x, y)                                                         \
    {escalator_stair_step, TASK_WORK, 1, {0, 0, 0}, {(x), (y), 0.0F}, 0}
#define NO_STAIR {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0}

static struct task_spec escalator_down_stairs[10] = {
    DOWN_STAIR(0.0F, 0.0F),   DOWN_STAIR(10.0F, 5.55F),
    DOWN_STAIR(20.0F, 11.1F), DOWN_STAIR(30.0F, 16.65F),
    DOWN_STAIR(40.0F, 22.2F), DOWN_STAIR(50.0F, 27.75F),
    DOWN_STAIR(60.0F, 33.3F), DOWN_STAIR(70.0F, 38.85F),
    DOWN_STAIR(80.0F, 44.4F), NO_STAIR,
};
static struct task_spec escalator_up_stairs[10] = {
    UP_STAIR(0.0F, 0.0F),   UP_STAIR(10.0F, 5.55F),
    UP_STAIR(20.0F, 11.1F), UP_STAIR(30.0F, 16.65F),
    UP_STAIR(40.0F, 22.2F), UP_STAIR(50.0F, 27.75F),
    UP_STAIR(60.0F, 33.3F), UP_STAIR(70.0F, 38.85F),
    UP_STAIR(80.0F, 44.4F), NO_STAIR,
};

/* draw the stair's action at its frame. unless the set editor holds the stair,
 * step the frame on, back to 0 after the last. task->draw runs this too, so the
 * action plays while the game is paused */
static void escalator_stair_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(ACTION(work)->action, ACTION(work)->frame);
    njPopMatrix(1);
    if (!placement_editing(task)) {
        /* @hack spelling: the frame stepped in the test; stepped ahead of it,
         * 13 of 62 words differ and the function ends 2 bytes late */
        if ((ACTION(work)->frame += 1.0F) >=
            (Float)ACTION(work)->action->motion->nbFrame) {
            ACTION(work)->frame = 0.0F;
        }
    }
}

static void escalator_stair_destroy(struct task *task)
{
    task_kill(task);
}

static void escalator_destroy(struct task *task)
{
    task_destroy_children(task);
    task_kill(task);
}

static void escalator_stair_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case ESCALATOR_INIT:
        work->mode = ESCALATOR_NORMAL;
        ACTION(work)->action = &escalator_stair_action;
        task->destructor = escalator_stair_destroy;
        task->draw = escalator_stair_draw;
        break;
    case ESCALATOR_NORMAL:
        escalator_stair_draw(task);
        break;
    case ESCALATOR_END:
        break;
    }
}

/* each step a player touches its shape, the escalator clears the player's accel
 * and slip. it moves the player CARRY_ALONG along world x and CARRY_UP up. a
 * down escalator moves the player back and down. it asks the player for
 * PLAYER_MODE_STILL, then for PLAYER_MODE_RESET, which replaces the first
 * request */
void escalator_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player *record;
    Sint8 player;
    Angle ang;
    Float along;
    Float up;

    if (placement_try_leave_by(task, ESCALATOR_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y + 0x8000, ang);
        texture_set_stage();
        njEasyDrawObject(work->scl.x < 0.0F ? &escalator_down_object
                                            : &escalator_up_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case ESCALATOR_INIT:
            work->mode = ESCALATOR_NORMAL;
            task->destructor = escalator_destroy;
            collision_init(task, escalator_shapes, COLLISION_OBJECT, 1, 1);
            task->work->collision->radius = ESCALATOR_REACH;
            if (work->scl.x < 0.0F) {
                task_create_children(escalator_down_stairs, task);
                work->submode = ESCALATOR_DOWN;
            } else {
                task_create_children(escalator_up_stairs, task);
                work->submode = ESCALATOR_UP;
            }
            break;
        case ESCALATOR_NORMAL:
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                player = PLAYER_VARS(work->collision->hit->owner->work)->player;
                player_set_mode(player, PLAYER_MODE_STILL);
                player_motion[player]->accel.x = 0.0F;
                player_motion[player]->accel.y = 0.0F;
                player_motion[player]->accel.z = 0.0F;
                record = player_get(player);
                record->accel.x = 0.0F;
                record->accel.y = 0.0F;
                record->accel.z = 0.0F;
                player_motion[player]->slip = 0.0F;
                /* @hack spelling: the carry read from locals, which the image
                 * loads before the switch; as CARRY_ALONG and CARRY_UP, 14 of
                 * 228 words differ */
                along = CARRY_ALONG;
                up = CARRY_UP;
                switch (work->submode) {
                case ESCALATOR_DOWN:
                    player_work[player]->pos.x -= along;
                    player_work[player]->pos.y -= up;
                    break;
                case ESCALATOR_UP:
                    /* the last case ends without a break: with one, 6 of 228
                     * words differ */
                    player_work[player]->pos.x += along;
                    player_work[player]->pos.y += up;
                }
                player_set_mode(player, PLAYER_MODE_RESET);
            }
            task_step_children(task);
            collision_register(work);
            break;
        case ESCALATOR_END:
            break;
        }
    }
}
