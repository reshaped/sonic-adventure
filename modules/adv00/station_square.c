/* Station Square acts, and the field and sky tasks */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/casino_ornament.h"
#include "adv00/crowd.h"
#include "adv00/elevators.h"
#include "adv00/land_anim.h"
#include "adv00/land_time.h"
#include "adv00/sky_models.h"
#include "adv00/station_square.h"
#include "adv00/texlists.h"
#include "adv00/traffic.h"
#include "camera/camera.h"
#include "draw/light.h"
#include "draw/tree.h"
#include "event/play.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

/* work->timer holds the act the field or sky task last started */
#define STATION_SQUARE_ACT(work) TASK_TIMER(work)

enum station_square_mode {
    STATION_SQUARE_INIT,
    STATION_SQUARE_START,
    STATION_SQUARE_RUN,
    STATION_SQUARE_END
};

/* the time of day the sky task last started under, which picks its model */
#define SKY_TIME(work) ((work)->submode)

/* the steps the music task counts down before it requests the music */
#define MUSIC_DELAY 10

/* held sounds of the sewer and hotel acts, which play until stopped */
#define SE_GESUI 0x223
#define SE_HOTEL 0x41b

/* the light files each time of day reads, by the act their names give:
 * SL_Q4B.bin and PL_Q4B.bin by day */
#define LIGHTS_DAY 4
#define LIGHTS_EVENING 1
#define LIGHTS_NIGHT 3

static void station_square_act_start(struct task *task);
static void station_square_destroy(struct task *task);
static void station_square_sky_destroy(struct task *task);
static void station_square_sky_act_start(struct task *task);
static void station_square_sky_draw(struct task *task);
static void music_step(struct task *task);

static void station_square_act_step(struct task *task)
{
    /* @hack redundant: work, read and never used, which leaves MOV R13,R4 where
     * station_square_step expands the call (0x0c900066). without it, 25 of
     * 58 words differ */
    struct task_work *work = task->work;
}

static void station_square_sky_act_end(struct task *task)
{
    /* @hack redundant: work, read and never used, which leaves MOV R13,R4 where
     * station_square_sky_step expands the call (0x0c900140). without it, 9
     * of 86 words differ */
    struct task_work *work = task->work;
}

/* under section HEAD, SHC writes this step and the sky's to section PHEAD,
 * which the module's link places first. 1ST_READ.BIN's stage_steps and
 * sky_steps name the two by their addresses there */
#pragma section HEAD
void station_square_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STATION_SQUARE_INIT:
        STATION_SQUARE_ACT(work) = (Uint8)stage_get_stage_act();
        task->destructor = station_square_destroy;
        station_square_messages_load();
        camera_set_start_mode(CAMERA_MODE_KNUCKLES);
        player_set_held_kind(0, 0);
        work->mode = STATION_SQUARE_START;
        break;
    case STATION_SQUARE_START:
        station_square_act_start(task);
        work->mode = STATION_SQUARE_RUN;
        break;
    case STATION_SQUARE_RUN:
        if ((Uint8)stage_get_stage_act() != STATION_SQUARE_ACT(work)) {
            work->mode = STATION_SQUARE_END;
        } else {
            station_square_act_step(task);
        }
        break;
    case STATION_SQUARE_END:
        work->mode = STATION_SQUARE_START;
        break;
    }
}
#pragma section

static void station_square_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;

    STATION_SQUARE_ACT(work) = (Uint8)stage_get_stage_act();
    player_set_mode(0, PLAYER_MODE_RESET);
    player_set_speed(0, 0.0F, 0.0F, 0.0F);
    if (STATION_SQUARE_ACT(work) != STATION_SQUARE_GESUI) {
        music = task_create(TASK_WORK, 0, music_step);
        music->work->mode = MUSIC_S_SQUARE;
        music->work->timer = MUSIC_DELAY;
    }
    land_time_set();
    sound_kill_all();
    switch (STATION_SQUARE_ACT(work)) {
    case STATION_SQUARE_OFFICE:
        traffic_start(0);
        break;
    case STATION_SQUARE_CASINO:
        task_create(TASK_WORK, 3, casino_ornament_init);
        break;
    case STATION_SQUARE_GESUI:
        land_anim_start(LAND_ANIM_GESUI);
        sound_play_held(SE_GESUI, -1, 8, 0);
        break;
    case STATION_SQUARE_STATION:
        traffic_start(1);
        land_anim_start(LAND_ANIM_STATION_A);
        land_anim_start(LAND_ANIM_STATION_B);
        land_anim_start(LAND_ANIM_STATION_C);
        break;
    case STATION_SQUARE_HOTEL:
        land_anim_start(LAND_ANIM_HOTEL_A);
        land_anim_start(LAND_ANIM_HOTEL_B);
        land_anim_start(LAND_ANIM_HOTEL_C);
        sound_play_held(SE_HOTEL, -1, 8, 0);
        break;
    case STATION_SQUARE_TWINKLE:
        twinkle_lights_start();
        break;
    }
}

static void station_square_destroy(struct task *task)
{
}

#pragma section HEAD
void station_square_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case STATION_SQUARE_INIT:
        STATION_SQUARE_ACT(work) = (Uint8)stage_get_stage_act();
        njSetFogColor(0);
        njSetLightIntensity(&dir_light, 0.8F, 0.9F, 0.5F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 0.95F);
        njSetLightDirection(&dir_light, 0.5F, -0.866025F, 0.0F);
        njFogDisable();
        task->destructor = station_square_sky_destroy;
        task->draw = station_square_sky_draw;
        work->mode = STATION_SQUARE_START;
        break;
    case STATION_SQUARE_START:
        station_square_sky_act_start(task);
        work->mode = STATION_SQUARE_RUN;
        break;
    case STATION_SQUARE_RUN:
        if ((Uint8)stage_get_stage_act() != STATION_SQUARE_ACT(work)) {
            work->mode = STATION_SQUARE_END;
            /* @hack spelling: a return where a break would return as well,
             * which has this case share STATION_SQUARE_INIT's store of the mode
             * (BRA 0x0c900110 at 0x0c90012e). with a break, 18 of 86 words
             * differ */
            return;
        }
        station_square_sky_draw(task);
        break;
    case STATION_SQUARE_END:
        station_square_sky_act_end(task);
        work->mode = STATION_SQUARE_START;
        break;
    }
}
#pragma section

static void station_square_sky_destroy(struct task *task)
{
}

/* read the lights and palettes for the time of day from the disc, and set
 * the background's color. a time other than TIME_DAY, TIME_EVENING and
 * TIME_NIGHT sets a red, green and blue background, and kills the task */
static void station_square_sky_act_start(struct task *task)
{
    struct task_work *work = task->work;

    SKY_TIME(work) = story_get_time();
    STATION_SQUARE_ACT(work) = (Uint8)stage_get_stage_act();
    switch (SKY_TIME(work)) {
    case TIME_DAY:
        light_read_disc_sources(STAGE_STATION_SQUARE, LIGHTS_DAY);
        light_read_disc_palettes(STAGE_STATION_SQUARE, LIGHTS_DAY);
        njSetBackColor(0x18dcf0, 0x18dcf0, 0x18dcf0);
        break;
    case TIME_EVENING:
        light_read_disc_sources(STAGE_STATION_SQUARE, LIGHTS_EVENING);
        light_read_disc_palettes(STAGE_STATION_SQUARE, LIGHTS_EVENING);
        njSetBackColor(0x780000, 0x780000, 0x780000);
        break;
    case TIME_NIGHT:
        light_read_disc_sources(STAGE_STATION_SQUARE, LIGHTS_NIGHT);
        light_read_disc_palettes(STAGE_STATION_SQUARE, LIGHTS_NIGHT);
        njSetBackColor(30, 30, 30);
        break;
    default:
        njSetBackColor(0xff0000, 0xff00, 0xff);
        task_kill(task);
        break;
    }
}

static void station_square_sky_draw(struct task *task)
{
    struct task_work *work = task->work;

    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    njFogDisable();
    njSetTexture(&ss_bg_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
    draw_tree(station_square_sky_models[SKY_TIME(work)]);
    njPopMatrix(1);
    njFogEnable();
    draw_control_restore();
}

/* request the music work->mode names once work->timer runs out, unless an
 * event plays then. the task ends either way */
static void music_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        if (event_check_music()) {
            stream_reset();
            stream_request_and_save_music(work->mode);
        }
        task_destroy(task);
    }
}
