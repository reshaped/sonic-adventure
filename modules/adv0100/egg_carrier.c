/* the Egg Carrier outside's field task: each act's start, step and sounds,
 * and act 6, the transformation */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/back_fire.h"
#include "adv0100/egg_carrier.h"
#include "adv0100/egg_lift.h"
#include "adv0100/gate.h"
#include "adv0100/people.h"
#include "adv0100/sky.h"
#include "adv0100/station.h"
#include "adv0100/transform_textures.h"
#include "camera/camera.h"
#include "draw/draw.h"
#include "draw/light.h"
#include "event/play.h"
#include "object/switch.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"

extern NJS_ACTION egg_carrier_change_action;
extern NJS_CACTION egg_carrier_change_camera;

#define FIELD_ACT(work) TASK_TIMER(work) /* the act the task last started */
/* TRUE once act 6 has asked for its fade to the next act */
#define CHANGE_ASKED(work) ((work)->submode)

enum field_mode { FIELD_INIT, FIELD_START, FIELD_RUN, FIELD_END };

/* steps into the act before the music starts */
#define MUSIC_DELAY 10

/* steps act 6 runs before it fades to the next act */
#define CHANGE_STEPS 400

/* action frames of the transformation, of its reversion, which plays it
 * backward, and of their camera */
#define CHANGE_SPEED 0.5F /* the action frames each plays in a step */
#define CHANGE_END 228.5F /* the frame the transformation stops at */
#define CHANGE_BACK_START 229.0F /* the frame the reversion starts from */
#define CAMERA_END 239.0F /* the frame the camera stops at */

/* the near clip for the transformation's draw and after it. CHANGE_FAR_CLIP
 * is the far clip for the draw. after the draw, change_step sets the far clip
 * to FAR_CLIP, whatever it was before */
#define NEAR_CLIP (-3.0F)
#define CHANGE_FAR_CLIP (-10000.0F)
#define FAR_CLIP (-3000.0F)

/* the transformation's or the reversion's first sound, and its later ones */
#define SE_CHANGE_FIRST 706
#define SE_CHANGE_NEXT 701

/* each act's sound, which act_sound refreshes each step */
#define SE_DECK 1060 /* acts 0 to 2, while the Egg Carrier flies */
#define SE_DECK_SUNK 1064 /* acts 0 to 2, once it has sunk */
/* acts 3 and 4: the captain's room, and the act with the doorway into it */
#define SE_INSIDE 1062
#define SE_CHANGING 1063 /* act 6 */

/* how far across captain_door player 0 may stand, and how far above or below
 * it, to go through it */
#define DOORWAY_RADIUS 50.0F
#define DOORWAY_HEIGHT 20.0F
/* the entrance of act 3 that captain_door leads to */
#define CAPTAIN_ENTRANCE 1

enum doorway_mode { DOORWAY_WAIT, DOORWAY_TAKEN };
enum change_mode { CHANGE_INIT, CHANGE_BACK, CHANGE_FORWARD };

struct frame_sound {
    Float frame;
    int se; /* played on reaching frame */
};

/* the transformation's action frame, and its camera's */
static Float change_frame = 0.0F;
static Float change_camera_frame = 0.0F;
static int dat_0cae6d60 = 0; /* never read */
/* steps left before act 6 fades to the next act */
static int change_wait = 0;
static int dat_0cae6d68 = 0; /* never read */
/* the doorway in act 4 that leads to act 3, the captain's room */
static NJS_POINT3 captain_door = {-80.0F, 80.0F, 0.0F};
/* each character's flag that is set while the Egg Carrier stands transformed.
 * Eggman and Tikal have none: for their -1, change_flag_set and
 * change_flag_toggle write event_flags[-1] */
static int change_flags[CHARACTER_COUNT] = {
    FLAG_SONIC_EC_TRANSFORM,
    -1,
    FLAG_TAILS_EC_TRANSFORM,
    FLAG_KNUCKLES_EC_TRANSFORM,
    -1,
    FLAG_AMY_EC_TRANSFORM,
    FLAG_E102_EC_TRANSFORM,
    FLAG_BIG_EC_TRANSFORM,
};
/* sounds by action frame: change_sounds for the transformation, from frame 0
 * up, and change_back_sounds for the reversion, down from CHANGE_BACK_START.
 * each table ends at a frame its play never reaches, so change_step reads no
 * further */
static struct frame_sound change_sounds[] = {
    {0.0F, SE_CHANGE_FIRST},  {40.0F, SE_CHANGE_NEXT},
    {120.0F, SE_CHANGE_NEXT}, {130.0F, SE_CHANGE_NEXT},
    {192.0F, SE_CHANGE_NEXT}, {1000.0F, SE_CHANGE_NEXT},
};
static struct frame_sound change_back_sounds[] = {
    {228.0F, SE_CHANGE_FIRST}, {192.0F, SE_CHANGE_NEXT},
    {130.0F, SE_CHANGE_NEXT},  {120.0F, SE_CHANGE_NEXT},
    {40.0F, SE_CHANGE_NEXT},   {-1000.0F, SE_CHANGE_NEXT},
};

Sint8 captain_switch_was;
struct task *field_act_tasks[FIELD_ACT_TASKS];
Bool egg_carrier_sunk;

static void act_start(struct task *task);
static void act_step(struct task *task);
static void act_end(struct task *task);
static void field_destroy(struct task *task);
static void music_step(struct task *task);
static void captain_door_step(struct task *task);
static Bool deck_open(void);
static void change_flag_toggle(void);
static void change_flag_set(int changed);
static void change_step(struct task *task);
static void act_sound(struct task *task);

/* 1ST_READ.BIN's stage_steps names the field task step at its address in the
 * module's head, right after the sky task step. under section HEAD, SHC
 * writes the function to section PHEAD, which the module's link places
 * first */
#pragma section HEAD
void egg_carrier_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    switch (work->mode) {
    case FIELD_INIT:
        FIELD_ACT(work) = (Uint8)stage_get_stage_act();
        task->destructor = field_destroy;
        camera_set_start_mode(CAMERA_MODE_KNUCKLES);
        egg_carrier_people_init();
        egg_carrier_sunk = egg_carrier_has_sunk();
        for (i = 0; i < FIELD_ACT_TASKS; i++) {
            field_act_tasks[i] = NULL;
        }
        work->mode = FIELD_START;
        break;
    case FIELD_START:
        act_start(task);
        work->mode = FIELD_RUN;
        break;
    case FIELD_RUN:
        if ((Uint8)stage_get_stage_act() != FIELD_ACT(work)) {
            act_end(task);
            act_start(task);
        } else {
            act_step(task);
            act_sound(task);
        }
        break;
    case FIELD_END:
        act_end(task);
        act_start(task);
        work->mode = FIELD_START;
        break;
    }
}
#pragma section

static void act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;
    int i;

    FIELD_ACT(work) = (Uint8)stage_get_stage_act();
    player_set_mode(0, PLAYER_MODE_RESET);
    player_set_speed(0, 0.0F, 0.0F, 0.0F);
    if (FIELD_ACT(work) != 6) {
        music = task_create(TASK_WORK, 0, music_step);
        music->work->mode = MUSIC_EGCARER1;
        music->work->timer = MUSIC_DELAY;
    } else {
        music = task_create(TASK_WORK, 0, music_step);
        music->work->mode = MUSIC_TITL_EGG;
        music->work->timer = MUSIC_DELAY;
    }
    switch (FIELD_ACT(work)) {
    case 0:
        event_flags[FLAG_MONORAIL_CALLED] = 0;
        change_flag_set(0);
        i = 0;
        if (!egg_carrier_sunk) {
            field_act_tasks[i++] =
                task_create(TASK_WORK, 3, egg_carrier_back_fire_step);
            field_act_tasks[i] =
                task_create(TASK_WORK, 3, egg_carrier_clouds_step);
        }
        if (deck_open()) {
            event_flags[FLAG_LIFT] = LIFT_READY_NOW;
            event_flags[FLAG_GATE] = GATE_OPEN_NOW;
        } else {
            event_flags[FLAG_LIFT] = LIFT_NO_COMMAND;
            event_flags[FLAG_GATE] = GATE_SHUT_NOW;
        }
        if (!egg_carrier_sunk) {
            light_read_disc_sources(STAGE_EGG_CARRIER, 0);
            light_read_disc_palettes(STAGE_EGG_CARRIER, 0);
        } else {
            light_read_disc_sources(STAGE_BEACH, 0);
            light_read_disc_palettes(STAGE_BEACH, 0);
        }
        break;
    case 1:
        event_flags[FLAG_MONORAIL_CALLED] = 0;
        change_flag_set(1);
        i = 0;
        if (!egg_carrier_sunk) {
            field_act_tasks[i++] =
                task_create(TASK_WORK, 3, egg_carrier_back_fire_step);
            field_act_tasks[i] =
                task_create(TASK_WORK, 3, egg_carrier_clouds_step);
        }
        if (!egg_carrier_sunk) {
            light_read_disc_sources(STAGE_EGG_CARRIER, 0);
            light_read_disc_palettes(STAGE_EGG_CARRIER, 0);
        } else {
            light_read_disc_sources(STAGE_BEACH, 0);
            light_read_disc_palettes(STAGE_BEACH, 0);
        }
        break;
    case 2:
        change_flag_set(1);
        i = 0;
        if (!egg_carrier_sunk) {
            field_act_tasks[i++] =
                task_create(TASK_WORK, 3, egg_carrier_back_fire_step);
            field_act_tasks[i] =
                task_create(TASK_WORK, 3, egg_carrier_clouds_step);
        }
        if (deck_open()) {
            event_flags[FLAG_LIFT] = LIFT_READY_NOW;
            event_flags[FLAG_GATE] = GATE_OPEN_NOW;
        } else {
            event_flags[FLAG_LIFT] = LIFT_NO_COMMAND;
            event_flags[FLAG_GATE] = GATE_SHUT_NOW;
        }
        if (!egg_carrier_sunk) {
            light_read_disc_sources(STAGE_EGG_CARRIER, 0);
            light_read_disc_palettes(STAGE_EGG_CARRIER, 0);
        } else {
            light_read_disc_sources(STAGE_BEACH, 0);
            light_read_disc_palettes(STAGE_BEACH, 0);
        }
        break;
    case 3:
        captain_switch_was = 0;
        switch_set(0, FALSE);
        break;
    case 4:
        field_act_tasks[0] = task_create(TASK_WORK, 3, captain_door_step);
        break;
    case 5:
        break;
    case 6:
        field_act_tasks[0] = task_create(TASK_WORK, 3, change_step);
        CHANGE_ASKED(work) = FALSE;
        change_wait = CHANGE_STEPS;
        break;
    }
}

/* in act 3, switch 0 turning on starts act 6, the transformation. once the
 * Egg Carrier has sunk, it toggles the character's transform flag instead.
 * act 6 runs CHANGE_STEPS steps, then toggles the flag and fades to an act
 * chosen by the act before it, as stage_get_previous_stage_act gives it.
 * after act 0 the fade is to act 1, or act 2 for Knuckles. after act 1 or 2
 * it is to act 0. either way the Egg Carrier takes its other shape. after
 * any other act the fade is back to it, at entrance 0 after act 3 */
static void act_step(struct task *task)
{
    struct task_work *work = task->work;
    int value;

    switch (FIELD_ACT(work)) {
    case 3:
        value = switch_is_on(0);
        /* @hack spelling: value compared with TRUE, CMP/EQ #1 at 0x0c92e1a4.
         * switch_is_on returns only TRUE or FALSE, so a plain test of value is
         * the same; as one, 13 of 95 words differ */
        if (!captain_switch_was && value == TRUE) {
            if (!egg_carrier_sunk) {
                stage_fade_to(STAGE_EGG_CARRIER, 6);
            } else {
                change_flag_toggle();
            }
        }
        captain_switch_was = value;
        break;
    case 4:
        break;
    case 6:
        if (!CHANGE_ASKED(work)) {
            if (change_wait == 0) {
                value = (Uint8)stage_get_previous_stage_act();
                switch (value) {
                case 0:
                    if (get_character() == CHARACTER_KNUCKLES) {
                        value = 2;
                    } else {
                        value = 1;
                    }
                    break;
                case 1:
                case 2:
                    value = 0;
                    break;
                case 3:
                    stage_set_entrance(0);
                    break;
                }
                CHANGE_ASKED(work) = TRUE;
                stage_fade_to(STAGE_EGG_CARRIER, value);
                change_flag_toggle();
            } else {
                --change_wait;
            }
        }
        break;
    }
}

static void act_end(struct task *task)
{
    int i;

    for (i = 0; i < FIELD_ACT_TASKS; ++i) {
        if (field_act_tasks[i] != NULL) {
            task_destroy(field_act_tasks[i]);
            field_act_tasks[i] = NULL;
        }
    }
}

static void field_destroy(struct task *task)
{
    act_end(task);
}

/* once work->timer has run out, request work->mode's music unless an event
 * plays, and destroy the task */
static void music_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        if (event_check_music()) {
            stream_reset();
            stream_request_and_save_music(work->mode);
        }
        task_destroy(task);
    }
}

static void captain_door_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case DOORWAY_WAIT:
        /* @hack spelling: player_in_cylinder compared with 1, CMP/EQ #1 at
         * 0x0c92e368. player_in_cylinder returns only 1 or 0, so a plain test
         * of its result is the same; as one, 2 of 31 words differ */
        if (player_in_cylinder(&captain_door, DOORWAY_RADIUS, DOORWAY_HEIGHT) ==
            1) {
            stage_set_entrance(CAPTAIN_ENTRANCE);
            story_go_to(STAGE_EGG_CARRIER, 3);
            work->mode = DOORWAY_TAKEN;
        }
        break;
    case DOORWAY_TAKEN:
        break;
    }
}

/* TRUE once the character's story has opened the deck's gates and lift */
static Bool deck_open(void)
{
    switch (get_character()) {
    case CHARACTER_SONIC:
        return event_flags[FLAG_SONIC_EC_EGG_LIFT];
    case CHARACTER_TAILS:
        return event_flags[FLAG_TAILS_EC_EGG_LIFT];
    case CHARACTER_KNUCKLES:
        return event_flags[FLAG_KNUCKLES_EC_EGG_LIFT];
    case CHARACTER_AMY:
        return event_flags[FLAG_AMY_EC_EGG_LIFT];
    case CHARACTER_E102:
        return event_flags[FLAG_E102_EC_EGG_LIFT];
    case CHARACTER_BIG:
        return event_flags[FLAG_BIG_EC_EGG_LIFT];
    }
    return FALSE;
}

static void change_flag_toggle(void)
{
    int flag = change_flags[get_character()];

    event_flags[flag] = !event_flags[flag];
}

static void change_flag_set(int changed)
{
    int flag = change_flags[get_character()];

    event_flags[flag] = changed;
}

/* play act 6: the reversion if the character's transform flag is set as the
 * task starts, else the transformation. the camera's action plays with it,
 * and each frame's sound as the play passes it */
static void change_step(struct task *task)
{
    struct task_work *work = task->work;
    int changed;

    switch (work->mode) {
    case CHANGE_INIT:
        changed = event_flag_check(change_flags[get_character()]);
        if (changed) {
            change_frame = CHANGE_BACK_START;
            change_camera_frame = 0.0F;
            work->mode = CHANGE_BACK;
        } else {
            change_frame = 0.0F;
            change_camera_frame = 0.0F;
            work->mode = CHANGE_FORWARD;
        }
        break;
    case CHANGE_BACK:
        if (change_frame <= change_back_sounds[work->submode].frame) {
            sound_play(change_back_sounds[work->submode].se, 0, 0, 0);
            ++work->submode;
        }
        /* @hack spelling: -CHANGE_SPEED added, a constant of its own, which
         * SHC loads from the pool for the add (MOVA at 0x0c92e530); with -=,
         * SHC subtracts the 0.5 it holds in a register for the other adds,
         * the registers of 0.5, 239.0 and 0 rotate, and 28 of 167 words
         * differ */
        change_frame += -CHANGE_SPEED;
        if (change_frame <= 0.0F) {
            change_frame = 0.0F;
        }
        change_camera_frame += CHANGE_SPEED;
        if (change_camera_frame >= CAMERA_END) {
            change_camera_frame = CAMERA_END;
        }
        break;
    case CHANGE_FORWARD:
        if (change_frame >= change_sounds[work->submode].frame) {
            sound_play(change_sounds[work->submode].se, 0, 0, 0);
            ++work->submode;
        }
        change_frame += CHANGE_SPEED;
        if (change_frame >= CHANGE_END) {
            change_frame = CHANGE_END;
        }
        change_camera_frame += CHANGE_SPEED;
        if (change_camera_frame >= CAMERA_END) {
            change_camera_frame = CAMERA_END;
        }
        break;
    }
    njClipZ(NEAR_CLIP, CHANGE_FAR_CLIP);
    camera_play_action(&egg_carrier_change_camera, change_camera_frame);
    njSetTexture(&ec_transform_texlist);
    njPushMatrix(NULL);
    njRotateY(NULL, 0x8000);
    draw_action(&egg_carrier_change_action, change_frame);
    njPopMatrix(1);
    njClipZ(NEAR_CLIP, FAR_CLIP);
}

static void act_sound(struct task *task)
{
    struct task_work *work = task->work;

    switch (FIELD_ACT(work)) {
    case 0:
    case 1:
    case 2:
        if (!egg_carrier_sunk) {
            sound_play_for(SE_DECK, (int)work, 1, 0, 2);
        } else {
            sound_play_for(SE_DECK_SUNK, (int)work, 1, 0, 2);
        }
        break;
    case 3:
    case 4:
        sound_play_for(SE_INSIDE, (int)work, 1, 0, 2);
        break;
    case 5:
        break;
    case 6:
        sound_play_for(SE_CHANGING, (int)work, 1, 0, 2);
        break;
    }
}
