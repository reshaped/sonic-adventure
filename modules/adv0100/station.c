/* O STATION, a monorail station, whose button calls the Egg Carrier's
 * monorail. modules/adv0130/station.c builds this file again for the inside,
 * with that field's button textures */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/station.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: the four objects the stand draws declared as arrays, so SHC
 * pools each one's address and reads its model at @(4,R4), as at
 * 0x0c92cbbc; declared as objects, summed over station_draw_work and
 * station_scroll, 46 of 119 words differ */
extern NJS_OBJECT station_object[];
extern NJS_OBJECT station_button_object[];
extern NJS_OBJECT station_panel_object[];
extern NJS_OBJECT station_glow_object[];
extern NJS_OBJECT station_solid_object;
extern NJS_OBJECT station_button_solid_object;

/* the stand counts the steps from player 0's first stand on the button, and
 * the button goes in on the step its count reads PRESS_STEPS. it goes in and
 * comes out along half a sine whose angle turns PRESS_TURN a step */
#define PRESS_STEPS 20
#define PRESS_TURN 0x800

#define SE_PRESS 568

/* the button's texture ids, lit and dark, in the field's texture list.
 * ADV0130's list holds them elsewhere, so modules/adv0130/station.c defines
 * these first */
#ifndef BUTTON_LIT
#define BUTTON_LIT 123
#define BUTTON_DARK 124
#endif

/* in the stand's work->flags. STATION_LIT lights the button, and is clear
 * only while the monorail is called. STATION_GLOW fades the panel in, and is
 * set while the station is open and hasn't called the monorail. the button's
 * task sets STATION_STOOD_ON once player 0 stands on the button */
#define STATION_LIT 0x200
#define STATION_GLOW 0x400
#define STATION_STOOD_ON 0x800

#define SOLID_FLAGS (WORLD_MEDIUM | WORLD_SOLID)

/* the collision world tests the stand while a player is within
 * TESTED_RANGE */
#define TESTED_RANGE 50.0F

/* the vars of the stand and of the button */
struct station_vars {
    NJS_OBJECT *solid; /* in the collision world */
    union {
        NJS_OBJECT *object; /* the object whose model solid takes */
        /* the glow's scroll along u, in the stand's vars once its surface
         * is in the world */
        Sint16 scroll;
    } from;
    Float press; /* how far the button is in: 0 out, down to -1 */
};

#define VARS(work) ((struct station_vars *)&(work)->vars)
/* the alpha of the constant material the panel and the glow are drawn
 * with, kept in the stand's work->scl.x. at -1 they are hidden, and at 0
 * they show as their own materials draw them. it moves by FADE_SPEED a
 * step */
#define FADE(work) ((work)->scl.x)
#define FADE_SPEED 0.02F

/* the glow's scroll along u each step */
#define GLOW_SCROLL 4

/* in the stand's work->mode */
enum station_mode {
    STATION_INIT,
    STATION_READY, /* open: the panel glows, and the button waits */
    STATION_LOCKED, /* until the character's story opens the station */
    STATION_CALLED, /* until the monorail clears FLAG_MONORAIL_CALLED */
    STATION_PRESSED /* the button goes in and comes out */
};

/* in the button's work->mode */
enum button_mode { BUTTON_INIT, BUTTON_RUN };

static void station_destroy(struct task *task);
static void station_draw(struct task *task);
static void station_draw_work(struct task_work *work);
static void station_solid_init(struct task *task, struct task_work *work);
static void station_scroll(struct task_work *work);
static void station_button_step(struct task *task);
static void station_button_destroy(struct task *task);
static void station_button_solid_init(struct task *task,
                                      struct task_work *work);
static Bool station_open(void);

/* once station_open is TRUE, fade the panel in and count toward the press.
 * the press sends the button in and out with SE_PRESS, then sets
 * FLAG_MONORAIL_CALLED. the button and the panel stay dark until the
 * monorail clears the flag */
void egg_carrier_station_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case STATION_INIT:
        station_solid_init(task, work);
        task_create_child(TASK_WORK | TASK_FORCE, station_button_step, task);
        work->ang.x = 0;
        work->ang.z = 0;
        FADE(work) = -1.0F;
        work->flags |= STATION_LIT;
        work->flags &= ~STATION_GLOW;
        VARS(work)->from.scroll = 0;
        task->destructor = station_destroy;
        task->draw = station_draw;
        work->mode = STATION_LOCKED;
        break;
    case STATION_LOCKED:
        if (station_open()) {
            work->mode = STATION_READY;
            work->flags |= STATION_GLOW;
        }
        break;
    case STATION_READY:
        /* @bug logic: STATION_STOOD_ON stays set once player 0 has stood
         * on the button, so its test, the TST at 0x0c92c9d0, resets the
         * count only until then. the button goes in whether player 0 stays
         * on it or not. a press leaves the count at 0x8015: PRESS_STEPS + 1,
         * and PRESS_TURN for each of 16 steps. the count runs on from there,
         * so the button goes in again only once it wraps round to
         * PRESS_STEPS */
        if (event_flags[FLAG_MONORAIL_CALLED] == 1) {
            work->flags &= ~STATION_LIT;
            work->flags &= ~STATION_GLOW;
            work->mode = STATION_CALLED;
        } else if (!(work->flags & STATION_STOOD_ON)) {
            TASK_TIMER(work) = 0;
        } else if (TASK_TIMER(work)++ == PRESS_STEPS) {
            sound_play(SE_PRESS, 0, 0, 0);
            work->mode = STATION_PRESSED;
        }
        break;
    case STATION_CALLED:
        if (event_flags[FLAG_MONORAIL_CALLED] == 0) {
            work->flags |= STATION_LIT;
            work->flags |= STATION_GLOW;
            work->mode = STATION_READY;
        }
        break;
    case STATION_PRESSED:
        TASK_TIMER(work) += PRESS_TURN;
        VARS(work)->press = -njSin(TASK_TIMER(work));
        if (TASK_TIMER(work) < 0x8000) {
            break;
        }
        VARS(work)->press = 0.0F;
        event_flags[FLAG_MONORAIL_CALLED] = 1;
        work->flags &= ~STATION_LIT;
        work->flags &= ~STATION_GLOW;
        work->mode = STATION_CALLED;
        break;
    }
    if (FADE(work) > -1.0F) {
        station_scroll(work);
    }
    if (work->flags & STATION_GLOW) {
        if (FADE(work) < 0.0F) {
            FADE(work) += FADE_SPEED;
        } else {
            FADE(work) = 0.0F;
        }
    } else {
        if (FADE(work) > -1.0F) {
            FADE(work) -= FADE_SPEED;
        } else {
            FADE(work) = -1.0F;
        }
    }
    work->flags &= ~WORLD_TESTED;
    if (player_in_range(&work->pos, TESTED_RANGE) != 0) {
        work->flags |= WORLD_TESTED;
    }
    task_step_children(task);
    station_draw_work(work);
}

static void station_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
    task_destroy_children(task);
}

static void station_draw(struct task *task)
{
    struct task_work *work = task->work;

    station_draw_work(work);
}

/* draw the stand, then the panel and the glow under FADE's constant
 * material, then the button, lit or dark and pressed in by press */
static void station_draw_work(struct task_work *work)
{
    NJS_MATERIAL *button;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(station_object->model);
    if (FADE(work) > -0.99F) {
        draw_set_material(FADE(work), 0.0F, 0.0F, 0.0F);
        draw_model(station_panel_object->model);
        draw_model(station_glow_object->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    button = station_button_object->model->mats;
    if (work->flags & STATION_LIT) {
        button->attr_texId = BUTTON_LIT;
    } else {
        button->attr_texId = BUTTON_DARK;
    }
    njTranslate(NULL, 0.0F, VARS(work)->press, 0.0F);
    draw_model(station_button_object->model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

/* add a copy of the stand's surface to the collision world at the stand */
static void station_solid_init(struct task *task, struct task_work *work)
{
    NJS_OBJECT *solid;

    VARS(work)->from.object = &station_solid_object;
    VARS(work)->solid = solid = world_object_alloc();
    solid->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = 0;
    solid->ang[1] = work->ang.y;
    solid->ang[2] = 0;
    solid->pos[0] = work->pos.x;
    solid->pos[1] = work->pos.y;
    solid->pos[2] = work->pos.z;
    solid->model = VARS(work)->from.object->model;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(SOLID_FLAGS, task, solid);
}

/* scroll the glow's texture along u by GLOW_SCROLL, wrapping at 255 */
static void station_scroll(struct task_work *work)
{
    NJS_TEX *uv = &station_glow_object->model->meshsets->vertuv->tex;
    int scroll = VARS(work)->from.scroll;

    uv[1].u = scroll;
    uv[3].u = scroll;
    uv[0].u = scroll + 251;
    uv[2].u = scroll + 251;
    VARS(work)->from.scroll += GLOW_SCROLL;
    if (VARS(work)->from.scroll > 255) {
        VARS(work)->from.scroll -= 510;
    }
}

/* the button's step. put the button's surface in the collision world, and set
 * STATION_STOOD_ON in the stand's flags while player 0 stands on the button.
 * copy WORLD_TESTED from the stand's flags */
static void station_button_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *stand = task->parent->work;

    switch (work->mode) {
    case BUTTON_INIT:
        station_button_solid_init(task, work);
        task->destructor = station_button_destroy;
        work->mode = BUTTON_RUN;
        break;
    case BUTTON_RUN:
        if (player_stands_on(0, task)) {
            stand->flags |= STATION_STOOD_ON;
        }
        break;
    }
    work->flags &= ~WORLD_TESTED;
    if (stand->flags & WORLD_TESTED) {
        work->flags |= WORLD_TESTED;
    }
}

static void station_button_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (VARS(work)->solid != NULL) {
        world_remove(task, VARS(work)->solid);
        world_object_free(VARS(work)->solid);
    }
}

/* add a copy of the button's surface to the collision world at the stand */
static void station_button_solid_init(struct task *task, struct task_work *work)
{
    struct task_work *stand = task->parent->work;
    NJS_OBJECT *solid;

    VARS(work)->from.object = &station_button_solid_object;
    VARS(work)->solid = solid = world_object_alloc();
    solid->evalflags = NJD_EVAL_UNIT_SCL | NJD_EVAL_BREAK;
    solid->scl[0] = 1.0F;
    solid->scl[1] = 1.0F;
    solid->scl[2] = 1.0F;
    solid->ang[0] = 0;
    solid->ang[1] = stand->ang.y;
    solid->ang[2] = 0;
    solid->pos[0] = stand->pos.x;
    solid->pos[1] = stand->pos.y;
    solid->pos[2] = stand->pos.z;
    solid->model = VARS(work)->from.object->model;
    solid->child = NULL;
    solid->sibling = NULL;
    world_add(SOLID_FLAGS, task, solid);
}

/* TRUE once the character's story has set its FLAG_<CHARACTER>_EC_MONORAIL.
 * FALSE for Eggman and Tikal */
static Bool station_open(void)
{
    switch (get_character()) {
    case CHARACTER_SONIC:
        return event_flag_check(FLAG_SONIC_EC_MONORAIL);
    case CHARACTER_TAILS:
        return event_flag_check(FLAG_TAILS_EC_MONORAIL);
    case CHARACTER_KNUCKLES:
        return event_flag_check(FLAG_KNUCKLES_EC_MONORAIL);
    case CHARACTER_AMY:
        return event_flag_check(FLAG_AMY_EC_MONORAIL);
    case CHARACTER_E102:
        return event_flag_check(FLAG_E102_EC_MONORAIL);
    case CHARACTER_BIG:
        return event_flag_check(FLAG_BIG_EC_MONORAIL);
    }
    return FALSE;
}
