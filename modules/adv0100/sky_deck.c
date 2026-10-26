/* O SKYDECK, the hatch on the way to Sky Deck. it opens for Sonic and Tails
 * in act 1, and for Knuckles in act 5 */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/sky_deck.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_ACTION sky_deck_action;
extern NJS_OBJECT sky_deck_solid_object;

#define SOLID_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

#define SE_HATCH_OPEN 561

/* the action's frame at which the hatch stands open, and the frames a step it
 * opens and shuts by */
#define HATCH_OPEN_FRAME 28.99F
#define OPEN_SPEED 0.5F
#define SHUT_SPEED 0.8F

/* sky_deck_open asks for player 0 within OPEN_RANGE of the hatch. the
 * collision world tests the hatch while player 0 is within TESTED_RANGE */
#define OPEN_RANGE 50.0F
#define TESTED_RANGE 40.0F

struct sky_deck_vars {
    NJS_OBJECT *solid; /* in the collision world */
    NJS_OBJECT *from; /* the object whose model solid takes */
    Float frame; /* the action's frame: 0 shut, HATCH_OPEN_FRAME open */
};

#define VARS(work) ((struct sky_deck_vars *)&(work)->vars)

enum sky_deck_mode {
    HATCH_INIT,
    HATCH_SHUT,
    HATCH_OPEN,
    HATCH_OPENING,
    HATCH_SHUTTING
};

static void sky_deck_solid_init(struct task *task, struct task_work *work);
static void sky_deck_draw(struct task *task);
static void sky_deck_destroy(struct task *task);
static void sky_deck_draw_work(struct task_work *work);
static Bool sky_deck_open(struct task_work *work);

/* open the hatch, with SE_HATCH_OPEN, once sky_deck_open is TRUE, and shut it
 * once it is FALSE. the step tests sky_deck_open only while the hatch stands
 * open or shut, so a hatch that starts to move goes all the way */
void egg_carrier_sky_deck_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case HATCH_INIT:
        /* the set entry's x angle's low bit, which nothing reads */
        work->submode = work->ang.x & 1;
        work->ang.x = 0;
        sky_deck_solid_init(task, work);
        task->destructor = sky_deck_destroy;
        task->draw = sky_deck_draw;
        work->mode = HATCH_SHUT;
        break;
    case HATCH_SHUT:
        if (sky_deck_open(work)) {
            sound_play(SE_HATCH_OPEN, 0, 0, 0);
            work->mode = HATCH_OPENING;
        }
        break;
    case HATCH_OPEN:
        if (!sky_deck_open(work)) {
            work->mode = HATCH_SHUTTING;
        }
        break;
    case HATCH_OPENING:
        VARS(work)->frame += OPEN_SPEED;
        if (VARS(work)->frame < HATCH_OPEN_FRAME) {
            break;
        }
        VARS(work)->frame = HATCH_OPEN_FRAME;
        work->mode = HATCH_OPEN;
        break;
    case HATCH_SHUTTING:
        /* @hack spelling: the speed negated and added, -0.8 in the pool at
         * 0x0c92d04c and an FADD at 0x0c92cff8; subtracted, 1 of 117 words
         * differ */
        VARS(work)->frame += -SHUT_SPEED;
        if (VARS(work)->frame > 0.0F) {
            break;
        }
        VARS(work)->frame = 0.0F;
        work->mode = HATCH_SHUT;
        break;
    }
    work->flags &= ~WORLD_TESTED;
    if (njDistanceP2P(&work->pos, &player_work[0]->pos) < TESTED_RANGE) {
        work->flags |= WORLD_TESTED;
    }
    sky_deck_draw_work(work);
}

/* add a copy of the surface to the collision world at the hatch's position and
 * y angle */
static void sky_deck_solid_init(struct task *task, struct task_work *work)
{
    NJS_OBJECT *solid;

    VARS(work)->from = &sky_deck_solid_object;
    VARS(work)->solid = solid = world_object_alloc();
    solid->evalflags = NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = 0;
    solid->ang[1] = work->ang.y;
    solid->ang[2] = 0;
    solid->pos[0] = work->pos.x;
    solid->pos[1] = work->pos.y;
    solid->pos[2] = work->pos.z;
    solid->model = VARS(work)->from->model;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(SOLID_FLAGS, task, solid);
}

static void sky_deck_draw(struct task *task)
{
    struct task_work *work = task->work;

    sky_deck_draw_work(work);
}

static void sky_deck_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

static void sky_deck_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_action(&sky_deck_action, VARS(work)->frame);
    njPopMatrix(1);
}

/* TRUE while player 0 is within OPEN_RANGE of the hatch and may go on to Sky
 * Deck. Sonic and Tails may go in act 1, and Knuckles in act 5 */
static Bool sky_deck_open(struct task_work *work)
{
    int character = get_character();

    if (njDistanceP2P(&player_work[0]->pos, &work->pos) > OPEN_RANGE) {
        return FALSE;
    }
    switch ((Uint8)stage_get_stage_act()) {
    case 1:
        if (character == CHARACTER_SONIC || character == CHARACTER_TAILS) {
            return TRUE;
        }
        break;
    case 5:
        if (character == CHARACTER_KNUCKLES) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}
