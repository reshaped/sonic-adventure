/* O PIER, the pier where the boat or the raft waits once the Egg Carrier has
 * sunk */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/boats.h"
#include "adv0100/egg_carrier.h"
#include "adv0100/pier.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "main.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT pier_object;
extern NJS_OBJECT pier_solid_object;

/* each doorway's cylinder: player 0 is in it within DOOR_RADIUS across and
 * DOOR_HEIGHT above or below its point */
#define DOOR_RADIUS 10.0F
#define DOOR_HEIGHT 30.0F

/* DARK_STEPS is how long the black holds. AWAY_STEPS is how long the
 * doorways wait after the pier starts or player 0 crosses */
#define DARK_STEPS 20
#define AWAY_STEPS 10

/* the fade's change each step */
#define FADE_SPEED 0.02F

struct pier_vars {
    Uint8 unk_00[8];
    /* how much of the scene shows through the black, from 0 to 1. the
     * black's alpha is 1 - fade */
    Float fade;
};

#define VARS(work) ((struct pier_vars *)&(work)->vars)

/* the vars of the solid, the pier's child task */
struct pier_solid_vars {
    NJS_OBJECT *solid; /* in the collision world */
    NJS_OBJECT *from; /* the object whose model and evalflags it takes */
};

#define SOLID(work) ((struct pier_solid_vars *)&(work)->vars)

/* the doorway player 0 stepped into, which work->submode keeps. the upper
 * one is on the deck, and the lower one on the pier */
enum pier_door { DOOR_NONE, DOOR_UPPER, DOOR_LOWER };

enum pier_mode {
    PIER_INIT,
    /* wait AWAY_STEPS steps, then until player 0 is out of both doorways */
    PIER_AWAY,
    /* wait for player 0 to step into a doorway */
    PIER_WAIT,
    /* hold the black DARK_STEPS steps, then move player 0 through */
    PIER_DARK,
    PIER_FADE_IN,
    PIER_FADE_OUT
};

enum pier_solid_mode { SOLID_INIT, SOLID_RUN };

static void pier_draw_work(struct task_work *work);
static void pier_draw_fade(Float fade);
static int pier_doorway(Float side);
static void pier_go_through(Float side, Sint8 door);
static void pier_solid_destroy(struct task *task);
static void pier_solid_step(struct task *task);

/* the doorways' points. pier_doorway sets x by the pier's side */
static const NJS_POINT3 pier_upper_door = {0.0F, 570.0F, -723.0F};
static const NJS_POINT3 pier_lower_door = {0.0F, 438.0F, -723.0F};

/* do nothing until the Egg Carrier has sunk. then, outside the set editor,
 * create the solid, and the boat on the +x side or the raft on the other.
 * when player 0 steps into a doorway, fade to black, put player 0 beside the
 * other doorway and fade back. player 0's pad is off until the fade ends */
void egg_carrier_pier_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *made;

    if (!egg_carrier_sunk) {
        return;
    }
    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case PIER_INIT:
        if (!placement_editing(task)) {
            child = task_create_child(TASK_WORK | TASK_FORCE, pier_solid_step,
                                      task);
            if (child != NULL) {
                made = child->work;
                SOLID(made)->from = &pier_solid_object;
                made->ang.y = work->ang.y;
                made->pos = work->pos;
            }
            if (work->pos.x > 0.0F) {
                child =
                    task_create_child(TASK_WORK, egg_carrier_boat_step, task);
                if (child != NULL) {
                    made = child->work;
                    made->pos.x = 587.0F;
                    made->pos.y = 414.0F;
                    made->pos.z = -702.0F;
                    made->ang.y = 0x4000;
                }
            } else {
                child =
                    task_create_child(TASK_WORK, egg_carrier_raft_step, task);
                if (child != NULL) {
                    made = child->work;
                    made->pos.x = -596.0F;
                    made->pos.y = 416.0F;
                    made->pos.z = -744.0F;
                    made->ang.y = 0x4000;
                }
            }
        }
        work->mode = PIER_AWAY;
        work->timer = AWAY_STEPS;
        break;
    case PIER_AWAY:
        if (work->timer == 0) {
            if (!pier_doorway(work->pos.x)) {
                work->mode = PIER_WAIT;
            }
        } else {
            work->timer--;
        }
        break;
    case PIER_WAIT:
        switch (pier_doorway(work->pos.x)) {
        case DOOR_UPPER:
            work->submode = DOOR_UPPER;
            pad_disable_player(0);
            VARS(work)->fade = 1.0F;
            work->mode = PIER_FADE_OUT;
            break;
        case DOOR_LOWER:
            /* no break ends the last case, so each case keeps its own copy of
             * the code the two share (0x0c92c24e, 0x0c92c262). with one,
             * DOOR_UPPER's case branches into DOOR_LOWER's code, the function
             * runs 32 bytes short and 100 of 238 words differ */
            work->submode = DOOR_LOWER;
            pad_disable_player(0);
            VARS(work)->fade = 1.0F;
            work->mode = PIER_FADE_OUT;
        }
        break;
    case PIER_FADE_OUT:
        VARS(work)->fade -= FADE_SPEED;
        if (VARS(work)->fade <= 0.0F) {
            VARS(work)->fade = 0.0F;
            work->timer = DARK_STEPS;
            work->mode = PIER_DARK;
        }
        pier_draw_fade(VARS(work)->fade);
        break;
    case PIER_DARK:
        if (work->timer == 0) {
            pier_go_through(work->pos.x, work->submode);
            work->mode = PIER_FADE_IN;
        } else {
            work->timer--;
        }
        pier_draw_fade(VARS(work)->fade);
        break;
    case PIER_FADE_IN:
        VARS(work)->fade += FADE_SPEED;
        if (VARS(work)->fade >= 1.0F) {
            VARS(work)->fade = 1.0F;
            pad_enable_player(0);
            work->mode = PIER_AWAY;
            work->timer = AWAY_STEPS;
        }
        pier_draw_fade(VARS(work)->fade);
        break;
    }
    pier_draw_work(work);
    if (!placement_editing(task)) {
        task_step_children(task);
    }
}

static void pier_draw_work(struct task_work *work)
{
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&pier_object);
    njPopMatrix(1);
}

/* on a frame's first step, cover the screen in black of alpha 1 - fade */
static void pier_draw_fade(Float fade)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    Uint32 color;

    if (frame_step != 0) {
        return;
    }
    quad.p = points;
    quad.col = colors;
    quad.tex = NULL;
    quad.num = 4;
    color = (1.0F - fade) * 255.0F;
    color <<= 24;
    points[0].x = 0.0F;
    points[0].y = 0.0F;
    colors[0].color = color;
    points[1].x = 0.0F;
    points[1].y = SCREEN_HEIGHT;
    colors[1].color = color;
    points[2].x = SCREEN_WIDTH;
    points[2].y = SCREEN_HEIGHT;
    colors[2].color = color;
    points[3].x = SCREEN_WIDTH;
    points[3].y = 0.0F;
    colors[3].color = color;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawPolygon2D(&quad, 4, -10.0F, NJD_FILL | NJD_TRANSPARENT);
}

/* the doorway player 0 stands in, or DOOR_NONE. side's sign picks the pair
 * on that side of x */
static int pier_doorway(Float side)
{
    NJS_POINT3 upper;
    NJS_POINT3 lower;

    upper = pier_upper_door;
    lower = pier_lower_door;
    if (side > 0.0F) {
        upper.x = 326.0F;
        lower.x = 515.0F;
    } else {
        upper.x = -326.0F;
        lower.x = -515.0F;
    }
    if (player_in_cylinder(&upper, DOOR_RADIUS, DOOR_HEIGHT) == 1) {
        return DOOR_UPPER;
    }
    if (player_in_cylinder(&lower, DOOR_RADIUS, DOOR_HEIGHT) == 1) {
        return DOOR_LOWER;
    }
    return DOOR_NONE;
}

/* put player 0 beside the doorway other than door, outside its cylinder.
 * side's sign picks the side of x */
static void pier_go_through(Float side, Sint8 door)
{
    Float offset;
    Float x;
    Float y;
    Float z;

    if (side > 0.0F) {
        side = 1.0F;
        offset = 22.0F;
    } else {
        side = -1.0F;
        offset = -22.0F;
    }
    if (door == DOOR_UPPER) {
        x = side * 548.0F;
        y = 426.0F;
        z = offset - 723.0F;
    } else {
        x = side * 306.0F;
        y = 570.0F;
        z = -722.0F;
    }
    player_set_position(0, x, y, z);
}

static void pier_solid_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (SOLID(work)->solid != NULL) {
        world_remove(task, SOLID(work)->solid);
        world_object_free(SOLID(work)->solid);
    }
}

/* the solid's step: add it to the collision world. the world tests it while
 * player 0 is below it and within 400, or above it and within 60 */
static void pier_solid_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *solid;

    switch (work->mode) {
    case SOLID_INIT:
        SOLID(work)->solid = solid = world_object_alloc();
        solid->evalflags = SOLID(work)->from->evalflags | NJD_EVAL_BREAK;
        solid->scl[0] = 1.0F;
        solid->scl[1] = 1.0F;
        solid->scl[2] = 1.0F;
        solid->ang[0] = 0;
        solid->ang[1] = work->ang.y;
        solid->ang[2] = 0;
        solid->pos[0] = work->pos.x;
        solid->pos[1] = work->pos.y;
        solid->pos[2] = work->pos.z;
        solid->model = SOLID(work)->from->model;
        solid->child = NULL;
        solid->sibling = NULL;
        world_add(WORLD_SOLID, task, solid);
        work->mode = SOLID_RUN;
        task->destructor = pier_solid_destroy;
        break;
    case SOLID_RUN:
        break;
    }
    work->flags &= ~WORLD_TESTED;
    if (player_work[0]->pos.y < work->pos.y) {
        if (player_in_range(&work->pos, 400.0F) == 1) {
            work->flags |= WORLD_TESTED;
        }
    } else {
        if (player_in_range(&work->pos, 60.0F) == 1) {
            work->flags |= WORLD_TESTED;
        }
    }
}
