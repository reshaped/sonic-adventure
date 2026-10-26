/* AokiSwitch, a floor switch that turns one of 32 numbered flags on and off.
 * it repeats object/switch.c's switch, but only player 0 standing on it
 * presses it. its flags are its own: only AokiSwitches read them */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/aoki_switch.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MATERIAL aoki_switch_button_materials[];
extern NJS_MODEL aoki_switch_button_model;
extern NJS_MODEL aoki_switch_base_model;
extern NJS_MODEL aoki_switch_plate_model;

/* the first of the button's two lamp materials */
#define LAMP_MATERIAL 3

#define SWITCH_COUNT 32

/* the bits of a switch flag */
#define SWITCH_ON 0x1
#define SWITCH_CHANGED 0x2 /* since a timed switch last looked */

/* how a switch behaves, from its scl.x */
enum switch_kind {
    SWITCH_TOGGLE, /* each press turns it the other way */
    SWITCH_PUSH, /* on while it's pressed */
    SWITCH_TIMED, /* a press turns it on for scl.z steps */
    SWITCH_ON_ONLY, /* a press turns it on, for good */
    SWITCH_UNDEFINED /* this kind and every one past it toggle */
};

enum switch_mode { MODE_START, MODE_RUN };

/* the steps a switch stays down after a press */
#define PRESS_STEPS 15

/* the bounds, in steps, within which a timed switch's scl.z sets its time */
#define TIMER_SHORTEST 5
#define TIMER_LONGEST 30000.0F

/* a timed switch's lamp waits 1 step to blink, and more while its time left
 * exceeds BLINK_FAST. the extra is that excess times BLINK_LIT while lit, or
 * times BLINK_DARK while dark, rounded down. in its last BLINK_FAST steps,
 * the lamp turns every other step */
#define BLINK_FAST 40
#define BLINK_LIT 0.06F
#define BLINK_DARK 0.04F

/* the world tests the plate while a player is within PLATE_REACH. the switch
 * draws while one is within DRAW_REACH */
#define PLATE_REACH 30.0F
#define DRAW_REACH 410.0F

/* how far the button stands up while the switch is off */
#define BUTTON_UP 0.6F

/* a switch's bytes in work->vars. besides them, work->submode says whether
 * its lamp is lit. work->tick holds the steps it stays down, and work->timer
 * the steps a timed switch has left */
struct switch_vars {
    Sint8 held; /* a toggle was down the step before too */
    Uint8 glow; /* the lamp's brightness */
    Sint16 blink; /* steps until a timed switch's lamp blinks */
    Sint16 kind;
    Sint16 id; /* the switch flag it turns on and off */
    NJS_OBJECT *plate; /* what a player stands on, in the world */
};

#define VARS(work) ((struct switch_vars *)&(work)->vars)

static void aoki_switch_init(struct task *task);
static void aoki_switch_setup(struct task *task);
static void aoki_switch_run(struct task *task);
static void aoki_switch_draw(struct task *task);
static void aoki_switch_destroy(struct task *task);
static void aoki_switch_toggle(struct task *task);
static void aoki_switch_push(struct task *task);
static void aoki_switch_timed(struct task *task);
static void aoki_switch_start_timer(struct task_work *work);
static void aoki_switch_next_blink(struct task_work *work);
static void aoki_switch_on_only(struct task *task);
static void aoki_switch_light(struct task_work *work, Bool on);
static void aoki_switch_fade(struct task_work *work);
static void aoki_switch_task_set(struct task *task, Bool on);
static Bool aoki_switch_task_on(struct task *task);
static void aoki_switch_hold_down(struct task *task);
static void aoki_switch_set(Uint32 id, Bool on);
static Bool aoki_switch_is_on(Uint32 id);
static Bool aoki_switch_changed(Uint32 id);

/* each kind's name, as the set editor shows it, in the game's spelling */
static char *aoki_switch_kind_names[] = {"TOGGLE TYPE", "PUSH TYPE",
                                         "OFF AFTER A WHILE", "TURN ON ONLY",
                                         "UNDEFIND TYPE"};

static Uint8 aoki_switch_flags[SWITCH_COUNT];

/* @hack spelling: the press tested through an inline function. the pragma
 * expands it in each caller. the caller sets TRUE or FALSE in a register and
 * tests that (0x0c90b2b4 to 0x0c90b302 in aoki_switch_toggle); tested in
 * place as task->work->tick, summed over its four callers, 160 of 201 words
 * differ */
#pragma inline(switch_pressed)
/* TRUE while the switch is down */
static Bool switch_pressed(struct task *task)
{
    if (task->work->tick) {
        return TRUE;
    }
    return FALSE;
}

void aoki_switch_step(struct task *task)
{
    struct task_work *work = task->work;

    /* while the set editor holds the switch, it doesn't run. each step
     * removes its plate, sets the switch up again from its set entry and
     * draws it. it also labels the entry's fields */
    if (placement_editing(task)) {
        switch (work->mode) {
        case MODE_START:
            aoki_switch_init(task);
            break;
        case MODE_RUN:
            aoki_switch_destroy(task);
            aoki_switch_setup(task);
            aoki_switch_draw(task);
            break;
        }
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- KIND:%s",
                    aoki_switch_kind_names[VARS(work)->kind]);
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- ID");
        if (VARS(work)->kind == SWITCH_TIMED) {
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- TIMER");
        }
    } else {
        switch (work->mode) {
        case MODE_START:
            aoki_switch_init(task);
            break;
        case MODE_RUN:
            aoki_switch_draw(task);
            aoki_switch_run(task);
            break;
        }
    }
}

static void aoki_switch_init(struct task *task)
{
    struct task_work *work = task->work;

    task->destructor = aoki_switch_destroy;
    task->draw = aoki_switch_draw;
    aoki_switch_setup(task);
    work->mode = MODE_RUN;
}

/* read the kind and id from the set entry's scale, and put the switch at
 * rest. add its plate to the world where it stands */
static void aoki_switch_setup(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force;
    NJS_OBJECT *plate;
    Uint32 kind;
    Uint16 id;

    kind = work->scl.x;
    if (kind > SWITCH_UNDEFINED) {
        kind = SWITCH_UNDEFINED;
    }
    VARS(work)->kind = kind;
    /* @hack type: scl.y converted through Uint32, which tests it against
     * 2147483648.0F before the FTRC (FCMP/GT at 0x0c90b03c); converted
     * straight to id, 76 of 99 words differ */
    id = (Uint32)work->scl.y;
    if (id >= SWITCH_COUNT) {
        id = SWITCH_COUNT - 1;
    }
    VARS(work)->id = id;
    work->tick = 0;
    work->timer = 0;
    VARS(work)->held = 0;
    work->submode = 0;
    VARS(work)->glow = 0;
    force = task->force;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
    plate = world_object_alloc();
    VARS(work)->plate = plate;
    plate->pos[0] = work->pos.x;
    plate->pos[1] = work->pos.y;
    plate->pos[2] = work->pos.z;
    plate->ang[0] = work->ang.x;
    plate->ang[1] = work->ang.y;
    plate->ang[2] = work->ang.z;
    plate->scl[0] = 1.0F;
    plate->scl[1] = 1.0F;
    plate->scl[2] = 1.0F;
    plate->model = &aoki_switch_plate_model;
    world_add(WORLD_SMALL, task, plate);
}

/* unless the set editor holds the switch, run the kind's rules, the press
 * and the lamp's fade */
static void aoki_switch_run(struct task *task)
{
    struct task_work *work = task->work;

    if (player_in_range(&work->pos, PLATE_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (!placement_editing(task)) {
        switch (VARS(work)->kind) {
        case SWITCH_PUSH:
            aoki_switch_push(task);
            break;
        case SWITCH_TIMED:
            aoki_switch_timed(task);
            break;
        case SWITCH_ON_ONLY:
            aoki_switch_on_only(task);
            break;
        case SWITCH_TOGGLE:
        default:
            aoki_switch_toggle(task);
            break;
        }
        aoki_switch_hold_down(task);
        aoki_switch_fade(work);
    }
}

/* draw the base, then the button and its lamp */
static void aoki_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    int glow;

    /* @hack spelling: the range compared with 0, which SHC tests with
     * CMP/EQ #0 (0x0c90b1b4); as a !, 1 of 104 words differ */
    if (player_in_range(&work->pos, DRAW_REACH) == 0) {
        return;
    }
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njEasyDrawModel(&aoki_switch_base_model);
    if (aoki_switch_task_on(task)) {
        njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    } else {
        njTranslate(NULL, 0.0F, BUTTON_UP, 0.0F);
    }
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_ENABLE_ALPHA |
                NJD_CONTROL_3D_OFFSET_MATERIAL | NJD_CONTROL_3D_DEPTH_QUEUE |
                NJD_CONTROL_3D_MODEL_CLIP);
    glow = VARS(work)->glow;
    aoki_switch_button_materials[LAMP_MATERIAL].attrflags |=
        NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA | NJD_FLAG_IGNORE_SPECULAR;
    aoki_switch_button_materials[LAMP_MATERIAL].diffuse.argb.b = glow;
    aoki_switch_button_materials[LAMP_MATERIAL].diffuse.argb.g = glow;
    aoki_switch_button_materials[LAMP_MATERIAL].diffuse.argb.r = glow;
    aoki_switch_button_materials[LAMP_MATERIAL].diffuse.argb.a = 200;
    aoki_switch_button_materials[LAMP_MATERIAL + 1].attrflags |=
        NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA | NJD_FLAG_IGNORE_SPECULAR;
    aoki_switch_button_materials[LAMP_MATERIAL + 1].diffuse.argb.b = glow;
    aoki_switch_button_materials[LAMP_MATERIAL + 1].diffuse.argb.g = glow;
    aoki_switch_button_materials[LAMP_MATERIAL + 1].diffuse.argb.r = glow;
    aoki_switch_button_materials[LAMP_MATERIAL + 1].diffuse.argb.a = 200;
    njDrawModel(&aoki_switch_button_model);
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_ENABLE_ALPHA |
                NJD_CONTROL_3D_OFFSET_MATERIAL | NJD_CONTROL_3D_DEPTH_QUEUE |
                NJD_CONTROL_3D_MODEL_CLIP |
                NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    njPopMatrix(1);
}

static void aoki_switch_destroy(struct task *task)
{
    NJS_OBJECT *plate = VARS(task->work)->plate;

    world_remove(task, plate);
    world_object_free(plate);
}

static void aoki_switch_toggle(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_pressed(task)) {
        if (!VARS(work)->held) {
            /* @hack spelling: the flag flipped as 1 minus its state, which
             * SHC writes as NEG and ADD #1 (0x0c90b310, 0x0c90b314).
             * aoki_switch_task_on returns only TRUE or FALSE, so 1 minus
             * the state is its logical not. as a !, 19 of 37 words differ */
            aoki_switch_task_set(task, 1 - aoki_switch_task_on(task));
        }
        VARS(work)->held = 1;
    } else {
        VARS(work)->held = 0;
    }
    aoki_switch_light(work, aoki_switch_task_on(task));
}

static void aoki_switch_push(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_pressed(task)) {
        aoki_switch_task_set(task, TRUE);
        aoki_switch_light(work, TRUE);
    } else {
        aoki_switch_task_set(task, FALSE);
        aoki_switch_light(work, FALSE);
    }
}

/* on for its time. its lamp blinks faster as the time runs out, with a
 * sound at each blink. on a step that finds its flag changed, by any switch
 * of its id, it follows the flag */
static void aoki_switch_timed(struct task *task)
{
    struct task_work *work = task->work;

    if (aoki_switch_changed(VARS(work)->id)) {
        aoki_switch_flags[VARS(work)->id] &= ~SWITCH_CHANGED;
        /* @hack spelling: the flag's state compared with TRUE, which SHC
         * tests with CMP/EQ #1 (0x0c90b3a8). aoki_switch_task_on returns only
         * TRUE or FALSE. as a plain test, 2 of 112 words differ */
        if (aoki_switch_task_on(task) == TRUE) {
            aoki_switch_light(work, TRUE);
            aoki_switch_start_timer(work);
            aoki_switch_next_blink(work);
        } else {
            aoki_switch_light(work, FALSE);
            VARS(work)->blink = 0;
        }
    }
    if (switch_pressed(task)) {
        aoki_switch_task_set(task, TRUE);
        aoki_switch_light(work, TRUE);
        aoki_switch_start_timer(work);
        aoki_switch_next_blink(work);
        /* @hack spelling: the flag's state compared with TRUE, which SHC
         * tests with CMP/EQ #1 (0x0c90b424). aoki_switch_task_on returns only
         * TRUE or FALSE. as a plain test, 2 of 112 words differ */
    } else if (aoki_switch_task_on(task) == TRUE) {
        if (VARS(work)->blink) {
            VARS(work)->blink--;
        } else {
            aoki_switch_light(work, 1 - work->submode);
            aoki_switch_next_blink(work);
            sound_play(SE_SWITCH, 0, 0, 0);
        }
        if (TASK_TIMER(work) > 0) {
            work->timer--;
            if (work->timer == 0) {
                aoki_switch_task_set(task, FALSE);
                aoki_switch_light(work, FALSE);
            }
        }
    } else {
        aoki_switch_light(work, FALSE);
    }
}

/* start the timer at scl.z steps, or at TIMER_SHORTEST unless scl.z lies
 * between TIMER_SHORTEST and TIMER_LONGEST */
static void aoki_switch_start_timer(struct task_work *work)
{
    if (work->scl.z > TIMER_SHORTEST && work->scl.z < TIMER_LONGEST) {
        work->timer = (int)work->scl.z;
    } else {
        work->timer = TIMER_SHORTEST;
    }
}

/* set the steps to the next blink, fewer as the time runs out. they are
 * more while lit than while dark */
static void aoki_switch_next_blink(struct task_work *work)
{
    int steps = TASK_TIMER(work) - BLINK_FAST;

    if (steps > 0) {
        if (work->submode) {
            VARS(work)->blink = (int)(steps * BLINK_LIT) + 1;
        } else {
            VARS(work)->blink = (int)(steps * BLINK_DARK) + 1;
        }
    } else {
        VARS(work)->blink = 1;
    }
}

static void aoki_switch_on_only(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_pressed(task)) {
        aoki_switch_task_set(task, TRUE);
    }
    aoki_switch_light(work, aoki_switch_task_on(task));
}

/* light the lamp at its brightest, or let it fade */
static void aoki_switch_light(struct task_work *work, Bool on)
{
    if (on) {
        work->submode = 1;
        VARS(work)->glow = 255;
    } else {
        work->submode = 0;
    }
}

static void aoki_switch_fade(struct task_work *work)
{
    if (!work->submode) {
        if (VARS(work)->glow >= 50) {
            VARS(work)->glow -= 30;
        } else {
            VARS(work)->glow = 20;
        }
    }
}

/* turn the switch's flag on or off, with its sound when it changes */
static void aoki_switch_task_set(struct task *task, Bool on)
{
    if (on) {
        if (!aoki_switch_is_on(VARS(task->work)->id)) {
            sound_play(SE_SWITCH, 0, 0, 0);
        }
        aoki_switch_set(VARS(task->work)->id, TRUE);
    } else {
        if (aoki_switch_is_on(VARS(task->work)->id)) {
            sound_play(SE_SWITCH, 0, 0, 0);
        }
        aoki_switch_set(VARS(task->work)->id, FALSE);
    }
}

static Bool aoki_switch_task_on(struct task *task)
{
    return aoki_switch_is_on(VARS(task->work)->id);
}

/* while player 0 stands on the switch, hold it down for PRESS_STEPS more
 * steps */
static void aoki_switch_hold_down(struct task *task)
{
    struct task_work *work = task->work;

    if (player_stands_on(0, task)) {
        work->tick = PRESS_STEPS;
    } else if (work->tick) {
        work->tick--;
    }
}

static void aoki_switch_set(Uint32 id, Bool on)
{
    if (on) {
        aoki_switch_flags[id] |= SWITCH_ON;
    } else {
        aoki_switch_flags[id] &= ~SWITCH_ON;
    }
    aoki_switch_flags[id] |= SWITCH_CHANGED;
}

static Bool aoki_switch_is_on(Uint32 id)
{
    if (aoki_switch_flags[id] & SWITCH_ON) {
        return TRUE;
    }
    return FALSE;
}

static Bool aoki_switch_changed(Uint32 id)
{
    if (aoki_switch_flags[id] & SWITCH_CHANGED) {
        return TRUE;
    }
    return FALSE;
}
