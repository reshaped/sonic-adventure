/* the west rock in the station act, which breaks apart once player 0's
 * character has its west rock flag set */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/texlists.h"
#include "adv02/west_rock.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* the whole rock, which is also its own collision model */
extern NJS_OBJECT west_rock_object;
/* the broken rock's collision model, and the broken rock as it is drawn */
extern NJS_OBJECT west_rock_broken_ground_object;
extern NJS_OBJECT west_rock_broken_object;
/* the rock breaking apart */
/* @hack type: the action declared as an array, and read through ->. SHC then
 * loads its address and reads its motion 4 bytes past it (MOV.L @(4,R3),R2 at
 * 0x0c90c89e). declared as one, the code grows by 32 bytes, and measured with
 * its P row lengthened, 85 of west_rock_update's 380 words differ */
extern NJS_ACTION west_rock_action[];

/* the action's nodes, one for each piece the rock breaks into */
#define PIECES 14

/* the action frames the breaking rock plays a step */
#define FRAME_SPEED 0.5F
/* the collision world tests the rock only while player 0 or player 1 is within
 * NEAR */
#define NEAR 250.0F

/* the rock's flags in the collision world */
#define ROCK_WORLD_FLAGS (WORLD_LARGE | WORLD_MOVES)

/* the sound the rock makes as it starts to break apart, and again as its action
 * ends */
#define SE_BREAK 593

enum west_rock_mode { ROCK_INIT, ROCK_RUN };

/* the rock's states, in work->submode */
enum west_rock_state { ROCK_WHOLE, ROCK_BREAKING, ROCK_BROKEN, ROCK_STATES };

struct west_rock_vars {
    NJS_OBJECT *object; /* the rock's object in the collision world */
    Float frame; /* the frame of west_rock_action while the rock breaks */
};

#define VARS(work) ((struct west_rock_vars *)&(work)->vars)

static void west_rock_init(struct task *task);
static void west_rock_reset(struct task *task);
static void west_rock_update(struct task *task);
static void west_rock_draw(struct task *task);
static void west_rock_destroy(struct task *task);
static void west_rock_add_to_world(struct task *task, NJS_OBJECT *from,
                                   NJS_POINT3 *pos);
static void west_rock_remove(struct task *task);
static Bool west_rock_broken(void);

/* where the rock is drawn in each state. the collision world holds the whole
 * rock and the broken ground at their places too */
static NJS_POINT3 west_rock_places[ROCK_STATES] = {
    {-357.5F, 277.55F, 867.65F},
    {-353.76F, 101.42F, 893.69F},
    {-310.84F, 64.67F, 858.25F},
};

/* dust raised around the rock as it breaks apart, and off its pieces */
static struct dust_params west_rock_dust = {
    0.5F,
    0.1F,
    0.2F,
    0.2F,
    0.05F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static struct dust_params west_rock_piece_dust = {
    0.5F,
    0.1F,
    0.2F,
    0.2F,
    0.05F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
/* the chance a step that the breaking rock raises a puff around it, and that
 * each piece raises one off itself */
static Float west_rock_dust_chance = 0.01F;
static Float west_rock_piece_dust_chance = 0.02F;
/* where a piece's puff starts: its node's origin */
static NJS_POINT3 west_rock_piece_origin = {0.0F, 0.0F, 0.0F};

/* while the set editor holds the task, each step takes the rock out of the
 * collision world and resets it from west_rock_broken */
void west_rock_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case ROCK_INIT:
            west_rock_init(task);
            break;
        case ROCK_RUN:
            west_rock_draw(task);
            west_rock_update(task);
            break;
        }
    } else {
        switch (work->mode) {
        case ROCK_INIT:
            west_rock_init(task);
            break;
        case ROCK_RUN:
            west_rock_remove(task);
            west_rock_reset(task);
            west_rock_draw(task);
            west_rock_update(task);
            break;
        }
    }
}

static void west_rock_init(struct task *task)
{
    struct task_work *work = task->work;

    work->pos = west_rock_places[ROCK_WHOLE];
    task->draw = west_rock_draw;
    task->destructor = west_rock_destroy;
    west_rock_reset(task);
    work->mode = ROCK_RUN;
}

static void west_rock_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force = task->force;

    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    VARS(work)->frame = 0.0F;
    if (!west_rock_broken()) {
        west_rock_add_to_world(task, &west_rock_object,
                               &west_rock_places[ROCK_WHOLE]);
        work->submode = ROCK_WHOLE;
    } else {
        west_rock_add_to_world(task, &west_rock_broken_ground_object,
                               &west_rock_places[ROCK_BROKEN]);
        work->submode = ROCK_BROKEN;
    }
}

/* the rock breaks apart once west_rock_broken is TRUE, raising dust as its
 * action plays. the collision world keeps the whole rock until the action ends,
 * and then takes the broken ground. a broken rock is whole again as soon as
 * west_rock_broken is FALSE */
static void west_rock_update(struct task *task)
{
    struct task_work *work = task->work;
    Bool changed = FALSE;
    int i;

    switch (work->submode) {
    case ROCK_WHOLE:
        if (west_rock_broken()) {
            sound_play(SE_BREAK, 0, 0, 0);
            work->submode = ROCK_BREAKING;
            changed = TRUE;
        }
        break;
    case ROCK_BREAKING:
        if (rand() * RAND_SCALE < west_rock_dust_chance) {
            west_rock_dust.scale = 4.0F;
            west_rock_dust.pos.x =
                west_rock_places[ROCK_BREAKING].x +
                (((rand() * RAND_SCALE * 2.0F) - 1.0F) * 30.0F);
            west_rock_dust.pos.y =
                west_rock_places[ROCK_BREAKING].y +
                (((rand() * RAND_SCALE * 2.0F) - 1.0F) * 50.0F);
            west_rock_dust.pos.z =
                west_rock_places[ROCK_BREAKING].z +
                (((rand() * RAND_SCALE * 2.0F) - 1.0F) * 30.0F);
            west_rock_dust.speed.x = 0.0F;
            west_rock_dust.speed.z = 0.0F;
            west_rock_dust.speed.y = 1.0F;
            dust_create_from(&west_rock_dust);
        }
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &west_rock_places[ROCK_BREAKING]);
        pose_action(NULL, west_rock_action, VARS(work)->frame, PIECES - 1,
                    NULL);
        west_rock_piece_dust.scale = 4.0F;
        for (i = 0; i < PIECES; i++) {
            if (rand() * RAND_SCALE < west_rock_piece_dust_chance) {
                pose_get_matrix(i, NULL);
                njCalcPoint(NULL, &west_rock_piece_origin,
                            &west_rock_piece_dust.pos);
                west_rock_piece_dust.speed.x = 0.0F;
                west_rock_piece_dust.speed.z = 0.0F;
                west_rock_piece_dust.speed.y = 1.0F;
                dust_create_from(&west_rock_piece_dust);
            }
        }
        njPopMatrix(1);
        VARS(work)->frame += FRAME_SPEED;
        if (VARS(work)->frame >= west_rock_action->motion->nbFrame - 1.0F) {
            sound_play(SE_BREAK, 0, 0, 0);
            work->submode = ROCK_BROKEN;
            changed = TRUE;
        }
        break;
    case ROCK_BROKEN:
        if (!west_rock_broken()) {
            work->submode = ROCK_WHOLE;
            changed = TRUE;
        }
        break;
    }
    if (changed) {
        switch (work->submode) {
        case ROCK_WHOLE:
            west_rock_remove(task);
            west_rock_add_to_world(task, &west_rock_object,
                                   &west_rock_places[ROCK_WHOLE]);
            break;
        case ROCK_BREAKING:
            VARS(work)->frame = 0.0F;
            break;
        case ROCK_BROKEN:
            west_rock_remove(task);
            west_rock_add_to_world(task, &west_rock_broken_ground_object,
                                   &west_rock_places[ROCK_BROKEN]);
            break;
        }
    }
    if (player_in_range(&work->pos, NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void west_rock_draw(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->submode) {
    case ROCK_WHOLE:
        njPushMatrix(NULL);
        njTranslateV(NULL, &west_rock_places[ROCK_WHOLE]);
        njSetTexture(&mrobj_texlist);
        draw_tree(&west_rock_object);
        njPopMatrix(1);
        break;
    case ROCK_BREAKING:
        njPushMatrix(NULL);
        njTranslateV(NULL, &west_rock_places[ROCK_BREAKING]);
        njSetTexture(&mrobj_texlist);
        draw_action(west_rock_action, VARS(work)->frame);
        njPopMatrix(1);
        break;
    case ROCK_BROKEN:
        njPushMatrix(NULL);
        njTranslateV(NULL, &west_rock_places[ROCK_BROKEN]);
        njSetTexture(&mrobj_texlist);
        draw_tree(&west_rock_broken_object);
        njPopMatrix(1);
        break;
    }
}

static void west_rock_destroy(struct task *task)
{
    west_rock_remove(task);
    /* @hack redundant: (void)task after the call, which keeps the call a BSR
     * with the function's own return after it (0x0c90cae6); without it, SHC
     * jumps to west_rock_remove, and 6 of 6 words differ */
    (void)task;
}

/* put an object with from's model in the collision world at pos, unturned and
 * unscaled, as the rock's object */
static void west_rock_add_to_world(struct task *task, NJS_OBJECT *from,
                                   NJS_POINT3 *pos)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = world_object_alloc();

    VARS(work)->object = object;
    object->evalflags = from->evalflags;
    object->scl[0] = 1.0F;
    object->scl[1] = 1.0F;
    object->scl[2] = 1.0F;
    object->ang[0] = 0;
    object->ang[1] = 0;
    object->ang[2] = 0;
    object->pos[0] = pos->x;
    object->pos[1] = pos->y;
    object->pos[2] = pos->z;
    object->model = from->model;
    object->child = NULL;
    object->sibling = NULL;
    world_add(ROCK_WORLD_FLAGS, task, object);
}

static void west_rock_remove(struct task *task)
{
    struct task_work *work = task->work;

    world_remove(task, VARS(work)->object);
    world_object_free(VARS(work)->object);
}

/* TRUE while player 0's character has its west rock flag set, else FALSE. Amy,
 * Eggman and Tikal have no such flag */
static Bool west_rock_broken(void)
{
    switch (player_get_character(0)) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_MR_WEST_ROCK];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_MR_WEST_ROCK];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_MR_WEST_ROCK];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_MR_WEST_ROCK];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_MR_WEST_ROCK];
    case CHARACTER_AMY:
    default:
        return FALSE;
    }
}
