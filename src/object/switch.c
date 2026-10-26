/* switches: floor buttons that turn numbered flags on and off, which other
 * objects read */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/model.h"
#include "enemy/enemy.h"
#include "object/models.h"
#include "object/switch.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the number of switches, and of their flags */
#define SWITCH_COUNT 32

/* the bits of a switch's flag */
#define SWITCH_ON 0x1
/* switch_set sets it, and a timed switch clears it once it follows the
 * change */
#define SWITCH_CHANGED 0x2

/* how a switch behaves, from its scl.x */
enum switch_kind {
    SWITCH_TOGGLE, /* each press turns it the other way */
    SWITCH_PUSH, /* on while it's pressed */
    SWITCH_TIMED, /* a press turns it on for scl.z steps */
    SWITCH_ON_ONLY, /* a press turns it on, for good */
    SWITCH_UNDEFINED /* this kind and every one past it toggle */
};

enum switch_mode { MODE_START, MODE_RUN };

/* steps a switch stays down after a press */
#define PRESS_STEPS 18
/* a timed switch's time, in steps: scl.z when it lies between TIMER_LEAST
 * and TIMER_MOST, else TIMER_LEAST */
#define TIMER_LEAST 5
#define TIMER_MOST 30000
/* in the last BLINK_FAST steps of a timed switch's time, its lamp flips
 * every other step */
#define BLINK_FAST 40
/* how far across from the switch's middle a press by feet reaches */
#define PRESS_RADIUS 7.0F

/* the world flags of the switch's plate, a small object players stand on
 * as if it were level */
#define PLATE_FLAGS (WORLD_SMALL | WORLD_NO_SLOPE)

/* the world tests the plate while player 0 or 1 is within TEST_RANGE. the
 * switch draws only while one is within DRAW_RANGE */
#define TEST_RANGE 70.0F
#define DRAW_RANGE 410.0F

/* a switch's bytes in work->vars. work->submode holds whether its lamp is
 * lit, work->tick the steps it stays down, and work->timer the steps a
 * timed switch has left */
struct switch_vars {
    Sint8 held; /* the toggle was down last step, so a press flips it once */
    Uint8 glow; /* the lamp's brightness, 255 when lit */
    Sint16 blink; /* steps until a timed switch's lamp blinks */
    Sint16 kind;
    Sint16 id; /* the switch flag it turns on and off */
    NJS_OBJECT *plate; /* what a player stands on, in the world */
};

#define VARS(work) ((struct switch_vars *)&(work)->vars)

static void switch_init(struct task *task);
static void switch_setup(struct task *task);
static void switch_run(struct task *task);
static void switch_draw(struct task *task);
static void switch_destroy(struct task *task);
static void switch_toggle(struct task *task);
static void switch_push(struct task *task);
static void switch_timed(struct task *task);
static void switch_start_timer(struct task_work *work);
static void switch_next_blink(struct task_work *work);
static void switch_on_only(struct task *task);
static void switch_light(struct task_work *work, Bool on);
static void switch_fade(struct task_work *work);
static void switch_task_set(struct task *task, Bool on);
static Bool switch_task_on(struct task *task);
static void switch_hold_down(struct task *task);
static Bool switch_changed(Uint32 id);

/* each kind's name, as the set editor shows it, in the game's spelling */
static char *switch_kind_names[] = {"TOGGLE TYPE", "PUSH TYPE",
                                    "OFF AFTER A WHILE", "TURN ON ONLY",
                                    "UNDEFIND TYPE"};

/* the switch's shape, for E-102 to lock on to and for attacks to hit */
/* clang-format off */
static struct collision_shape switch_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
    {0.0F, 1.0F, 0.0F}, 6.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

/* each switch's flag, by id */
static Uint8 switch_flags[SWITCH_COUNT];

/* @hack spelling: the press tested through an inline function, whose TRUE
 * or FALSE each caller builds in a register and tests again (MOV #1,R4 and
 * TST R4,R4 at 0x8c06b176 and 0x8c06b17a); tested in place as work->tick,
 * summed over its four callers, 168 of 201 words differ */
#pragma inline(switch_pressed)
/* TRUE while the switch is down, else FALSE */
static Bool switch_pressed(struct task *task)
{
    if (task->work->tick) {
        return TRUE;
    }
    return FALSE;
}

void switch_step(struct task *task)
{
    struct task_work *work = task->work;

    /* in the set editor's hands, the switch only draws. it sets itself up
     * again from its set entry each step, and labels the entry's fields */
    if (placement_editing(task)) {
        switch (work->mode) {
        case MODE_START:
            switch_init(task);
            break;
        case MODE_RUN:
            switch_destroy(task);
            switch_setup(task);
            switch_draw(task);
            break;
        /* @hack redundant: the default's break, a jump past the cases (BRA at
         * 0x8c06ac9c), which keeps the first print's string in R2 and the
         * next in R3 (0x8c06acb6, 0x8c06acc0); without it, the two trade and
         * 6 of 85 words differ */
        default:
            break;
        }
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<- ANGLE X");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<- ANGLE Y");
        debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "<- ANGLE Z");
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- KIND:%s",
                    switch_kind_names[VARS(work)->kind]);
        debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- ID");
        if (VARS(work)->kind == SWITCH_TIMED) {
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- TIMER");
        }
    } else {
        switch (work->mode) {
        case MODE_START:
            switch_init(task);
            break;
        case MODE_RUN:
            switch_draw(task);
            switch_run(task);
            break;
        }
    }
}

static void switch_init(struct task *task)
{
    struct task_work *work = task->work;

    task->destructor = switch_destroy;
    task->draw = switch_draw;
    enemy_alloc_record(task, work);
    collision_init(task, &switch_shape, COLLISION_ITEM, 1, 1);
    work->collision->flags &= ~COLLISION_TARGET;
    work->collision->radius = 50.0F;
    switch_setup(task);
    work->mode = MODE_RUN;
}

/* read the kind from scl.x and the id from scl.y, each held to its range.
 * put the switch at rest, and add its plate to the world 2 below it */
static void switch_setup(struct task *task)
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
     * 2147483648.0F ahead of the FTRC (FCMP/GT at 0x8c06ae1c); converted
     * straight to id's Uint16, 81 of 102 words differ */
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
    plate->pos[1] = work->pos.y - 2.0F;
    plate->pos[2] = work->pos.z;
    plate->ang[0] = work->ang.x;
    plate->ang[1] = work->ang.y;
    plate->ang[2] = work->ang.z;
    plate->scl[0] = 1.0F;
    plate->scl[1] = 1.0F;
    plate->scl[2] = 1.0F;
    plate->model = &switch_plate_model;
    world_add(PLATE_FLAGS, task, plate);
}

/* run the kind's rules, the press and the lamp's fade, unless the set
 * editor holds the switch. then let the world test the plate while a
 * player is within TEST_RANGE */
static void switch_run(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (VARS(work)->kind) {
        case SWITCH_PUSH:
            switch_push(task);
            break;
        case SWITCH_TIMED:
            switch_timed(task);
            break;
        case SWITCH_ON_ONLY:
            switch_on_only(task);
            break;
        case SWITCH_TOGGLE:
        default:
            switch_toggle(task);
            break;
        }
        switch_hold_down(task);
        switch_fade(work);
    }
    if (player_in_range(&work->pos, TEST_RANGE)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    collision_register(work);
}

/* draw the base, then the button, pressed in further while the switch is
 * on. the button's materials 3 and 4 are its lamp, at glow's brightness */
static void switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    int glow;

    /* @hack spelling: player_in_range's result compared with 0, CMP/EQ #0
     * at 0x8c06af9a; as a !, SHC tests it with TST, and 1 of 144 words
     * differ */
    if (player_in_range(&work->pos, DRAW_RANGE) == 0) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 12.0F)) {
        return;
    }
    texture_set_regular();
    draw_set_palette(DRAW_PLAYER_PALETTE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njDrawModel(&switch_base_model);
    draw_set_palette(0);
    if (switch_task_on(task)) {
        njTranslate(NULL, 0.0F, -1.2F, 0.0F);
    } else {
        njTranslate(NULL, 0.0F, -0.3F, 0.0F);
    }
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_ENABLE_ALPHA |
                NJD_CONTROL_3D_OFFSET_MATERIAL | NJD_CONTROL_3D_DEPTH_QUEUE |
                NJD_CONTROL_3D_MODEL_CLIP);
    glow = VARS(work)->glow;
    switch_button_materials[3].attrflags |=
        NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_IGNORE_SPECULAR;
    switch_button_materials[3].diffuse.argb.b = glow;
    switch_button_materials[3].diffuse.argb.g = glow;
    switch_button_materials[3].diffuse.argb.r = glow;
    switch_button_materials[3].diffuse.argb.a = 100;
    switch_button_materials[4].attrflags |=
        NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_IGNORE_SPECULAR;
    switch_button_materials[4].diffuse.argb.b = glow;
    switch_button_materials[4].diffuse.argb.g = glow;
    switch_button_materials[4].diffuse.argb.r = glow;
    switch_button_materials[4].diffuse.argb.a = 100;
    njDrawModel(&switch_button_model);
    draw_control_restore();
    njPopMatrix(1);
}

static void switch_destroy(struct task *task)
{
    NJS_OBJECT *plate = VARS(task->work)->plate;

    world_remove(task, plate);
    world_object_free(plate);
}

static void switch_toggle(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_pressed(task)) {
        if (!VARS(work)->held) {
            /* @hack spelling: the flag flipped as 1 less it, NEG and ADD #1
             * at 0x8c06b188 and 0x8c06b18c. switch_task_on returns only TRUE
             * or FALSE, for which 1 - x is !x; as a !, SHC uses MOVT, and 19
             * of 37 words differ */
            switch_task_set(task, 1 - switch_task_on(task));
        }
        VARS(work)->held = 1;
    } else {
        VARS(work)->held = 0;
    }
    switch_light(work, switch_task_on(task));
}

static void switch_push(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_pressed(task)) {
        switch_task_set(task, TRUE);
        switch_light(work, TRUE);
    } else {
        switch_task_set(task, FALSE);
        switch_light(work, FALSE);
    }
}

/* a press turns the switch on for its time. meanwhile the lamp blinks
 * faster as the time runs out, with a sound at each blink. when anything
 * else changes the flag, the switch follows */
static void switch_timed(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_changed(VARS(work)->id)) {
        switch_flags[VARS(work)->id] &= ~SWITCH_CHANGED;
        /* @hack spelling: the flag compared with TRUE, CMP/EQ #1 at
         * 0x8c06b208. switch_task_on returns only TRUE or FALSE; as a plain
         * test, TST, and 2 of 112 words differ */
        if (switch_task_on(task) == TRUE) {
            switch_light(work, TRUE);
            switch_start_timer(work);
            switch_next_blink(work);
        } else {
            switch_light(work, FALSE);
            VARS(work)->blink = 0;
        }
    }
    /* @hack spelling: the flag compared with TRUE in the else if, CMP/EQ #1
     * at 0x8c06b284. switch_task_on returns only TRUE or FALSE; as a plain
     * test, TST, and 2 of 112 words differ */
    if (switch_pressed(task)) {
        switch_task_set(task, TRUE);
        switch_light(work, TRUE);
        switch_start_timer(work);
        switch_next_blink(work);
    } else if (switch_task_on(task) == TRUE) {
        if (VARS(work)->blink) {
            VARS(work)->blink--;
        } else {
            /* @hack spelling: the lamp flipped as 1 less submode, NEG and
             * ADD #1 at 0x8c06b298 and 0x8c06b29a. submode holds only
             * switch_setup's 0 and switch_light's 0 or 1, for which 1 - x is
             * !x; as a !, 47 of 112 words differ */
            switch_light(work, 1 - work->submode);
            switch_next_blink(work);
            sound_play(SE_SWITCH, 0, 0, 0);
        }
        if (TASK_TIMER(work) > 0) {
            work->timer--;
            if (work->timer == 0) {
                switch_task_set(task, FALSE);
                switch_light(work, FALSE);
            }
        }
    } else {
        switch_light(work, FALSE);
    }
}

static void switch_start_timer(struct task_work *work)
{
    if (work->scl.z > TIMER_LEAST && work->scl.z < TIMER_MOST) {
        work->timer = (int)work->scl.z;
    } else {
        work->timer = TIMER_LEAST;
    }
}

/* set the steps the lamp waits before it next flips: fewer as the time
 * runs out, and more while lit than while dark */
static void switch_next_blink(struct task_work *work)
{
    int steps = TASK_TIMER(work) - BLINK_FAST;

    if (steps > 0) {
        if (work->submode) {
            VARS(work)->blink = (int)(steps * 0.06F) + 1;
        } else {
            VARS(work)->blink = (int)(steps * 0.04F) + 1;
        }
    } else {
        VARS(work)->blink = 1;
    }
}

static void switch_on_only(struct task *task)
{
    struct task_work *work = task->work;

    if (switch_pressed(task)) {
        switch_task_set(task, TRUE);
    }
    switch_light(work, switch_task_on(task));
}

/* light the lamp at its brightest, or let it fade */
static void switch_light(struct task_work *work, Bool on)
{
    if (on) {
        work->submode = 1;
        VARS(work)->glow = 255;
    } else {
        work->submode = 0;
    }
}

/* dim an unlit lamp by 30 a step, and hold it at 20 once below 50 */
static void switch_fade(struct task_work *work)
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
static void switch_task_set(struct task *task, Bool on)
{
    if (on) {
        if (!switch_is_on(VARS(task->work)->id)) {
            sound_play(SE_SWITCH, 0, 0, 0);
        }
        switch_set(VARS(task->work)->id, TRUE);
    } else {
        if (switch_is_on(VARS(task->work)->id)) {
            sound_play(SE_SWITCH, 0, 0, 0);
        }
        switch_set(VARS(task->work)->id, FALSE);
    }
}

static Bool switch_task_on(struct task *task)
{
    return switch_is_on(VARS(task->work)->id);
}

/* hold the switch down for PRESS_STEPS from each press, and count work->tick
 * down between presses. player 0 standing on the switch presses it while
 * the player nearest it is within PRESS_RADIUS of its middle. a hit that
 * sets COLLISION_DAMAGED presses it when the hitting record is a
 * COLLISION_ATTACK, as E-102's shot is, or belongs to Amy. a
 * COLLISION_ATTACK also releases the switch's lock-on */
static void switch_hold_down(struct task *task)
{
    struct task_work *work = task->work;
    Bool pressed = FALSE;
    NJS_POINT3 pos;
    NJS_POINT3 local;

    if (player_stands_on(0, task)) {
        pos = player_work[player_nearest(&work->pos)]->pos;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, work->ang.z);
        njInvertMatrix(NULL);
        njCalcPoint(NULL, &pos, &local);
        njPopMatrix(1);
        if ((local.x * local.x) + (local.z * local.z) <
            PRESS_RADIUS * PRESS_RADIUS) {
            pressed = TRUE;
        }
    }
    if (work->flags & COLLISION_DAMAGED) {
        if (work->collision->hit->kind == COLLISION_ATTACK) {
            lockon_release(task);
            pressed = TRUE;
        } else if (player_task_character(work->collision->hit->owner) ==
                   CHARACTER_AMY) {
            pressed = TRUE;
        }
    }
    if (pressed) {
        work->tick = PRESS_STEPS;
    } else if (work->tick) {
        work->tick--;
    }
}

void switch_set(Uint32 id, Bool on)
{
    if (on) {
        switch_flags[id] |= SWITCH_ON;
    } else {
        switch_flags[id] &= ~SWITCH_ON;
    }
    switch_flags[id] |= SWITCH_CHANGED;
}

Bool switch_is_on(Uint32 id)
{
    if (switch_flags[id] & SWITCH_ON) {
        return TRUE;
    }
    return FALSE;
}

static Bool switch_changed(Uint32 id)
{
    if (switch_flags[id] & SWITCH_CHANGED) {
        return TRUE;
    }
    return FALSE;
}

void switch_reset(void)
{
    int i;

    for (i = 0; i < SWITCH_COUNT; i++) {
        switch_flags[i] = 0;
    }
}
