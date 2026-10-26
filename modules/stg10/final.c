/* Final Egg stage task, the sky task that sets each act's fog, and a light
 * task */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/light.h"
#include "event/play.h"
#include "object/switch.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg10/color_door.h"
#include "stg10/final.h"
#include "stg10/gacha_robot.h"
#include "stg10/texture.h"
#include "stg10/zero_appear.h"
#include "system/task.h"
#include "system/util.h"

#define SE_ACT0 979
#define SE_ACT1 985

#define VOICE_E102_FIRST 450
#define VOICE_E102_SECOND 449
#define VOICE_SONIC 445

enum final_mode {
    FINAL_INIT,
    FINAL_RUN,
    FINAL_WAIT, /* Amy's act 0, which starts once no event has control */
    FINAL_END /* nothing in this file sets it */
};

enum final_sky_mode { SKY_INIT, SKY_RUN, SKY_END };

enum final_light_mode { LIGHT_INIT, LIGHT_RUN, LIGHT_END };

struct final_sky_vars {
    Uint8 act; /* the act the sky task last set the view for */
};

#define SKY_VARS(work) ((struct final_sky_vars *)&(work)->vars)

/* final_sonic_voice_step sets it TRUE at its first step, 120 steps before
 * the first voice. nothing clears it, so a retry doesn't play the voices
 * again */
static Bool final_voices_heard = FALSE;

static Float final_light_specular = 0.0F;
static Float final_light_diffuse = 1.0F;
static Float final_light_ambient = 0.2F;
static Float dat_0ca88bb0 = 1.0F; /* no code reads it */
static Float final_light_red = 0.9F;
static Float final_light_green = 1.0F;
static Float final_light_blue = 0.4F;

static Uint8 final_act_then; /* the act the stage task last started */
static Uint8 final_act_now; /* the stage's act this step */
static NJS_FOG_TABLE final_fog;
static NJS_LIGHT final_light;

static void final_init(struct task *task);
static void final_act0_start(struct task *task);
static void final_act1_start(struct task *task);
static void final_act2_start(struct task *task);
static void final_e102_voice_step(struct task *task);
static void final_sonic_voice_step(struct task *task);
static void final_view_reset(void);
static void final_light_destroy(struct task *task);
static void final_act0_fog(void);
static void final_act1_fog(void);
static void final_act2_fog(void);

void final_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FINAL_INIT:
        final_init(task);
        break;
    case FINAL_WAIT:
        if (!event_in_control()) {
            final_act0_start(task);
            work->mode = FINAL_RUN;
        }
        break;
    case FINAL_RUN:
        final_act_now = stage_get_stage_act();
        if (final_act_now != final_act_then) {
            final_act_then = final_act_now;
            switch (final_act_now) {
            case 0:
                light_load_act();
                final_act0_start(task);
                break;
            case 1:
                light_load_act();
                final_act1_start(task);
                break;
            case 2:
                light_load_act();
                final_act2_start(task);
                break;
            }
        }
        break;
    case FINAL_END:
        break;
    }
}

static void final_init(struct task *task)
{
    struct task_work *work = task->work;

    final_act_now = final_act_then = stage_get_stage_act();
    switch (final_act_now) {
    case 0:
        zero_appear_reset();
        if (get_character() == CHARACTER_AMY) {
            color_door_pick();
            work->mode = FINAL_WAIT;
        } else {
            final_act0_start(task);
            work->mode = FINAL_RUN;
        }
        break;
    case 1:
        final_act1_start(task);
        work->mode = FINAL_RUN;
        break;
    case 2:
        final_act2_start(task);
        work->mode = FINAL_RUN;
        break;
    default:
        work->mode = FINAL_RUN;
        switch_reset();
        break;
    }
}

static void final_act0_start(struct task *task)
{
    stream_reset();
    stream_request_and_save_music(MUSIC_FINALEG1);
    switch_reset();
    final_texture_start();
    sound_play_held(SE_ACT0, -1, 8, 0);
}

static void final_act1_start(struct task *task)
{
    stream_reset();
    stream_request_and_save_music(MUSIC_FINALEG2);
    switch_reset();
    final_texture_start();
    sound_play_held(SE_ACT1, -1, 8, 0);
}

static void final_act2_start(struct task *task)
{
    task_create(TASK_WORK, 3, gacha_robot_list_step);
    stream_reset();
    stream_request_and_save_music(MUSIC_FINALEG1);
    switch_reset();
    final_texture_start();
    world_land_blocks = -1; /* all bits: every block of the land is drawn */
    if (get_character() == CHARACTER_E102) {
        task_create(TASK_WORK, 3, final_e102_voice_step);
    }
    if (get_character() == CHARACTER_SONIC) {
        task_create(TASK_WORK, 3, final_sonic_voice_step);
    }
}

static void final_e102_voice_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        work->timer = 180;
        work->mode = 1;
        break;
    case 1:
        if (--TASK_TIMER(work) <= 0) {
            stream_request_voice(VOICE_E102_FIRST);
            work->timer = 240;
            work->mode = 2;
        }
        break;
    case 2:
        if (--TASK_TIMER(work) <= 0) {
            stream_request_voice(VOICE_E102_SECOND);
            work->timer = 240;
            work->mode = 3;
        }
        break;
    case 3:
        task_kill(task);
        break;
    }
}

static void final_sonic_voice_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        if (final_voices_heard) {
            work->mode = 4;
        } else {
            work->timer = 120;
            work->mode = 1;
            final_voices_heard = TRUE;
        }
        break;
    case 1:
        if (--TASK_TIMER(work) <= 0) {
            stream_request_voice(VOICE_SONIC);
            work->timer = 420;
            work->mode = 2;
        }
        break;
    case 2:
        if (--TASK_TIMER(work) <= 0) {
            stream_request_voice(VOICE_SONIC);
            work->timer = 420;
            work->mode = 3;
        }
        break;
    case 3:
        if (--TASK_TIMER(work) <= 0) {
            stream_request_voice(VOICE_SONIC);
            work->timer = 420;
            work->mode = 4;
        }
        break;
    case 4:
        task_kill(task);
        break;
    }
}

void final_sky_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 act = stage_get_stage_act();

    /* @hack spelling: the act masked in a statement of its own, which copies
     * the call's result to R4 and masks it from there (MOV R0,R4 and EXTU.B
     * R4,R11 at 0x0c90042a and 0x0c90042e). masked as it is declared, SHC
     * masks R0 itself, and 50 of 153 words differ */
    act &= 0xff;
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (work->mode) {
    case SKY_INIT:
        switch (act) {
        case 0:
            final_view_reset();
            final_act0_fog();
            njClipZ(-3.0F, -1400.0F);
            break;
        case 1:
            final_view_reset();
            final_act1_fog();
            njClipZ(-3.0F, -1000.0F);
            break;
        case 2:
            final_view_reset();
            final_act2_fog();
            njClipZ(-3.0F, -600.0F);
            break;
        case 3:
        case 4:
        case 5:
            final_view_reset();
            break;
        }
        SKY_VARS(work)->act = act;
        work->mode = SKY_RUN;
        break;
    case SKY_RUN:
        if (act != SKY_VARS(work)->act) {
            SKY_VARS(work)->act = act;
            switch (act) {
            case 0:
                final_view_reset();
                final_act0_fog();
                njClipZ(-3.0F, -1400.0F);
                break;
            case 1:
                final_view_reset();
                final_act1_fog();
                njClipZ(-3.0F, -1000.0F);
                break;
            case 2:
                final_view_reset();
                final_act2_fog();
                njClipZ(-3.0F, -600.0F);
                break;
            case 3:
            case 4:
            case 5:
                final_view_reset();
                break;
            }
        }
        break;
    case SKY_END:
        /* @hack redundant: an empty case for each of acts 0 to 5. SHC writes
         * them as CMP/EQ #0 to #5 (0x0c900562 to 0x0c900576), and each of their
         * branches leads to the task_kill below; without it, 52 of 153 words
         * differ */
        switch (act) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        }
        task_kill(task);
        break;
    default:
        task_kill(task);
        break;
    }
    task_step_children(task);
    draw_control_restore();
}

static void final_view_reset(void)
{
    njSetDepthQueue(-100.0F, -500.0F);
    camera_set_perspective(NJM_DEG_ANG(60));
    njSetBackColor(0, 0, 0);
}

void final_light_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case LIGHT_INIT:
        njCreateLight(&final_light, NJD_DIR_LIGHT);
        njSetLightIntensity(&final_light, final_light_specular,
                            final_light_diffuse, final_light_ambient);
        njSetLightColor(&final_light, final_light_red, final_light_green,
                        final_light_blue);
        njSetLightDirection(&final_light, 0.866025F, -0.6F, 0.0F);
        njLightOn(&final_light);
        task->destructor = final_light_destroy;
        work->mode = LIGHT_RUN;
        break;
    case LIGHT_RUN:
        njSetLightIntensity(&dir_light, final_light_specular,
                            final_light_diffuse, final_light_ambient);
        njSetLightColor(&dir_light, final_light_red, final_light_green,
                        final_light_blue);
        njSetLightDirection(&dir_light, 0.866025F, -0.6F, 0.0F);
        break;
    case LIGHT_END:
        task_kill(task);
        break;
    }
}

static void final_light_destroy(struct task *task)
{
    njDeleteLight(&final_light);
}

static void final_act0_fog(void)
{
    njSetFogColor(0xff000000);
    njGenerateFogTable3(final_fog, -800.0F, -1200.0F);
    njSetFogTable(final_fog);
    njFogEnable();
}

static void final_act1_fog(void)
{
    njSetFogColor(0xff000000);
    njGenerateFogTable3(final_fog, -400.0F, -800.0F);
    njSetFogTable(final_fog);
    njFogEnable();
}

static void final_act2_fog(void)
{
    njSetFogColor(0xff000000);
    njGenerateFogTable3(final_fog, -400.0F, -600.0F);
    njSetFogTable(final_fog);
    njFogEnable();
}
