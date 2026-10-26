/* Casinopolis's stage task: each act's start, act requests and the tables'
 * lost balls. casino_set_texture picks the texture bank the module's objects
 * draw with */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "event/play.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg09/casino.h"
#include "stg09/obj_casino_textures.h"
#include "stg09/pinball.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_TEXLIST obj_casino8_texlist;

enum casino_bank { BANK_ACTS_01, BANK_ACTS_23 };

enum casino_mode {
    CASINO_INIT,
    CASINO_START,
    CASINO_ACT0,
    CASINO_ACT1,
    CASINO_ACT2,
    CASINO_ACT3
};

/* the height on each table below which player 0 loses a ball */
#define ACT2_FLOOR (-10.0F)
#define ACT3_FLOOR (-245.0F)
/* with this many rings or more, a table's last lost ball leaves for act 0,
 * not act 1 */
#define TABLE_PRIZE_RINGS 100

#define VIEW_ACTS_01 NJM_DEG_ANG(70)
#define VIEW_TABLES NJM_DEG_ANG(85)
#define TABLE_TILT NJM_DEG_ANG(5)

/* the steps with no event in control before an act's music starts */
#define MUSIC_DELAY 10
#define ROLL_DELAY 2

#define SE_BALL_LOST 249
#define SE_TABLE_START 252

#define ROLL_BUTTON PDD_DGT_TY

static void casino_roll_wait_step(struct task *task);

int casino_request = CASINO_REQUEST_NONE;
int casino_rings_deposited = 0;
static NJS_FOG_TABLE casino_fog = {
#include "stg09/casino/casino_fog.inc"
};

/* after work->tick steps with no event in control, request the music in
 * work->mode and end */
static void casino_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!event_in_control()) {
        if (--work->tick <= 0) {
            stream_reset();
            stream_request_music(work->mode);
            task_destroy(task);
        }
    }
}

static void casino_release_textures(int bank)
{
    if (bank == BANK_ACTS_01) {
        if (casino_flags & CASINO_HAS_CASINO9) {
            casino_flags &= ~CASINO_HAS_CASINO9;
            njReleaseTexture(&obj_casino9_texlist);
        }
        if (casino_flags & CASINO_HAS_CASINO8) {
            casino_flags &= ~CASINO_HAS_CASINO8;
            njReleaseTexture(&obj_casino8_texlist);
        }
    } else {
        if (casino_flags & CASINO_HAS_CASINO) {
            casino_flags &= ~CASINO_HAS_CASINO;
            njReleaseTexture(&obj_casino_texlist);
        }
        if (casino_flags & CASINO_HAS_CASINO2) {
            casino_flags &= ~CASINO_HAS_CASINO2;
            njReleaseTexture(&obj_casino2_texlist);
        }
    }
}

static void casino_load_textures(int bank)
{
    if (bank == BANK_ACTS_01) {
        if (!(casino_flags & CASINO_HAS_CASINO9)) {
            casino_flags |= CASINO_HAS_CASINO9;
            texture_load_pvm("OBJ_CASINO9", &obj_casino9_texlist);
        }
        if (!(casino_flags & CASINO_HAS_CASINO8)) {
            casino_flags |= CASINO_HAS_CASINO8;
            texture_load_pvm("OBJ_CASINO8", &obj_casino8_texlist);
        }
    } else {
        if (!(casino_flags & CASINO_HAS_CASINO)) {
            casino_flags |= CASINO_HAS_CASINO;
            texture_load_pvm("OBJ_CASINO", &obj_casino_texlist);
        }
        if (!(casino_flags & CASINO_HAS_CASINO2)) {
            casino_flags |= CASINO_HAS_CASINO2;
            texture_load_pvm("OBJ_CASINO2", &obj_casino2_texlist);
        }
    }
}

static void casino_enter_act2(struct task *task)
{
    if (stage_playing() && player_get_character(0) != CHARACTER_KNUCKLES &&
        casino_request == CASINO_REQUEST_ACT2) {
        stream_destroy();
        casino_release_textures(BANK_ACTS_01);
        world_change_act(2);
        placement_change_act(2);
        camera_change_act(2);
        stage_change_act(2);
        task->work->mode = CASINO_INIT;
        casino_request = CASINO_REQUEST_NONE;
        pinball_balls = PINBALL_BALLS;
    }
}

static void casino_enter_act3(struct task *task)
{
    if (stage_playing() && player_get_character(0) != CHARACTER_KNUCKLES &&
        casino_request == CASINO_REQUEST_ACT3) {
        stream_destroy();
        casino_release_textures(BANK_ACTS_01);
        world_change_act(3);
        placement_change_act(3);
        camera_change_act(3);
        stage_change_act(3);
        task->work->mode = CASINO_INIT;
        casino_request = CASINO_REQUEST_NONE;
        pinball_balls = PINBALL_BALLS;
    }
}

static void casino_leave_act1(struct task *task)
{
    if (stage_playing() && player_get_character(0) != CHARACTER_KNUCKLES &&
        casino_request == CASINO_REQUEST_ACT0) {
        stream_destroy();
        casino_release_textures(BANK_ACTS_23);
        world_change_act(-1);
        placement_change_act(-1);
        camera_change_act(-1);
        stage_change_act(-1);
        task->work->mode = CASINO_INIT;
        casino_request = CASINO_REQUEST_NONE;
        casino_entrance = CASINO_FROM_ACT1;
    }
}

/* a fall below ACT2_FLOOR loses a ball. the last ball lost leaves for act 1,
 * or for act 0 with TABLE_PRIZE_RINGS rings or more. any other lost ball puts
 * player 0 back at the table's start. while the stage isn't in play, the
 * balls refill to PINBALL_BALLS */
static void casino_act2_lose_ball(struct task *task)
{
    NJS_VECTOR *speed;
    struct angles *ang;

    if (!stage_playing()) {
        pinball_balls = PINBALL_BALLS;
        return;
    }
    if (player_work[0]->pos.y < ACT2_FLOOR) {
        sound_play(SE_BALL_LOST, 0, 0, 0);
        if (--pinball_balls == 0) {
            stream_destroy();
            casino_release_textures(BANK_ACTS_23);
            camera_restore();
            player_set_mode(0, PLAYER_MODE_RESET);
            if ((Sint16)ring_count() < TABLE_PRIZE_RINGS) {
                world_change_act(-1);
                placement_change_act(-1);
                camera_change_act(-1);
                stage_change_act(-1);
            } else {
                world_change_act(-2);
                placement_change_act(-2);
                camera_change_act(-2);
                stage_change_act(-2);
                casino_entrance = CASINO_FROM_TABLE;
            }
            task->work->mode = CASINO_INIT;
        } else {
            speed = &player_get(0)->speed;
            ang = &player_work[0]->ang;
            player_set_position(0, 137.0F, 35.0F, 64.0F);
            ang->y = 0;
            ang->x = ang->z = 0;
            speed->x = speed->y = speed->z = 0.0F;
        }
    }
}

/* as casino_act2_lose_ball, below ACT3_FLOOR. the last ball lost also drops
 * the idea piece carried and zeroes pinball_idea_draws. any lost ball empties
 * the hand dealt so far, and a ball put back rolls up again */
static void casino_act3_lose_ball(struct task *task)
{
    NJS_VECTOR *speed;
    struct angles *ang;

    if (!stage_playing()) {
        pinball_balls = PINBALL_BALLS;
        return;
    }
    if (player_work[0]->pos.y < ACT3_FLOOR) {
        sound_play(SE_BALL_LOST, 0, 0, 0);
        if (--pinball_balls == 0) {
            stream_destroy();
            casino_release_textures(BANK_ACTS_23);
            camera_restore();
            player_set_mode(0, PLAYER_MODE_RESET);
            if ((Sint16)ring_count() < TABLE_PRIZE_RINGS) {
                world_change_act(-2);
                placement_change_act(-2);
                camera_change_act(-2);
                stage_change_act(-2);
            } else {
                world_change_act(-3);
                placement_change_act(-3);
                camera_change_act(-3);
                stage_change_act(-3);
                casino_entrance = CASINO_FROM_TABLE;
            }
            task->work->mode = CASINO_INIT;
            casino_flags &= ~CASINO_IDEA_CARRIED;
            pinball_idea_draws = 0;
        } else {
            speed = &player_get(0)->speed;
            ang = &player_work[0]->ang;
            player_set_position(0, -430.0F, -205.0F, 100.0F);
            task_create(TASK_WORK, 3, casino_roll_wait_step)->work->tick =
                ROLL_DELAY;
            ang->y = 0;
            ang->x = ang->z = 0;
            speed->x = speed->y = speed->z = 0.0F;
        }
        pinball_cards_dealt = 0;
    }
}

static void casino_destroy(struct task *task)
{
    stream_destroy();
    camera_set_perspective(VIEW_ACTS_01);
    world_tilt_gravity(0, 0);
    casino_release_textures(BANK_ACTS_01);
    casino_release_textures(BANK_ACTS_23);
    njFogDisable();
}

/* count work->tick down each step. the step that finds it at 0 sets player
 * 0 rolling and ends the task. in act 0 or 1 the task ends at once */
static void casino_roll_wait_step(struct task *task)
{
    Sint16 act = (Uint8)stage_get_stage_act();

    if (act == 0 || act == 1) {
        task_kill(task);
    } else if (task->work->tick-- == 0) {
        player_set_mode(0, PLAYER_MODE_SLOPE_ROLL);
        task_kill(task);
    }
}

static void casino_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *music =
        task_create(TASK_WORK, 0, casino_music_wait_step)->work;
    NJS_VECTOR *speed = &player_get(0)->speed;
    struct angles *ang = &player_work[0]->ang;

    music->tick = MUSIC_DELAY;
    speed->x = speed->y = speed->z = 0.0F;
    ang->x = ang->z = 0;
    stream_destroy();
    switch ((Uint8)stage_get_stage_act()) {
    case 0:
        /* the palette is zeros when PL_9MB.BIN won't open */
        if (loader_read_file("PL_9MB.BIN", MUDDY_PALETTE)) {
            memset(MUDDY_PALETTE, 0, PALETTE_SIZE);
        }
        casino_load_textures(BANK_ACTS_01);
        camera_set_perspective(VIEW_ACTS_01);
        world_tilt_gravity(0, 0);
        if (casino_entrance == CASINO_FROM_START) {
            player_set_position(0, 75.0F, -201.0F, 270.0F);
            player_work[0]->ang.y = 0;
        } else if (casino_entrance != CASINO_FROM_ACT1) {
            player_set_position(0, 238.0F, 201.0F, -101.0F);
            player_work[0]->ang.y = 0;
        } else {
            player_set_position(0, -236.0F, -201.0F, -555.0F);
            player_work[0]->ang.y = 0;
        }
        ang->y = 0;
        music->mode = MUSIC_CASINO1;
        work->mode = CASINO_ACT0;
        njFogDisable();
        njSetFogColor(0);
        njGenerateFogTable3(casino_fog, -450.0F, -850.0F);
        njSetFogTable(casino_fog);
        njFogEnable();
        break;
    case 1:
        if (loader_read_file("PL_9MB.BIN", MUDDY_PALETTE)) {
            memset(MUDDY_PALETTE, 0, PALETTE_SIZE);
        }
        casino_load_textures(BANK_ACTS_01);
        camera_set_perspective(VIEW_ACTS_01);
        world_tilt_gravity(0, 0);
        player_set_position(0, 19.0F, -1694.87F, 2849.74F);
        ang->y = 0;
        music->mode = MUSIC_CASINO2;
        work->mode = CASINO_ACT1;
        njFogDisable();
        njSetFogColor(0xff707000);
        njGenerateFogTable3(casino_fog, -30.0F, -2000.0F);
        njSetFogTable(casino_fog);
        njFogEnable();
        break;
    case 2:
        casino_load_textures(BANK_ACTS_23);
        camera_set_perspective(VIEW_TABLES);
        world_tilt_gravity(-TABLE_TILT, 0);
        task_create(TASK_WORK, 3, casino_roll_wait_step)->work->tick =
            ROLL_DELAY;
        player_set_position(0, 137.0F, 35.0F, 64.0F);
        ang->y = 0;
        music->mode = MUSIC_CASINO3;
        work->mode = CASINO_ACT2;
        sound_play(SE_TABLE_START, 0, 0, 0);
        njFogDisable();
        njSetFogColor(0xffff00ff);
        njGenerateFogTable3(casino_fog, -5000.0F, -7000.0F);
        njSetFogTable(casino_fog);
        njFogEnable();
        break;
    case 3:
        casino_load_textures(BANK_ACTS_23);
        camera_set_perspective(VIEW_TABLES);
        world_tilt_gravity(-TABLE_TILT, 0);
        if (get_character() == CHARACTER_SONIC) {
            task_create(TASK_WORK, 3, casino_roll_wait_step)->work->tick =
                ROLL_DELAY;
        }
        player_set_position(0, -430.0F, -205.0F, 100.0F);
        ang->y = 0;
        music->mode = MUSIC_CASINO4;
        work->mode = CASINO_ACT3;
        sound_play(SE_TABLE_START, 0, 0, 0);
        njFogDisable();
        njSetFogColor(0xffff00ff);
        njGenerateFogTable3(casino_fog, -5000.0F, -7000.0F);
        njSetFogTable(casino_fog);
        njFogEnable();
        break;
    }
    task->destructor = casino_destroy;
}

void casino_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CASINO_INIT:
        work->mode = CASINO_START;
        break;
    case CASINO_START:
        casino_act_start(task);
        break;
    case CASINO_ACT0:
        casino_enter_act2(task);
        casino_enter_act3(task);
        break;
    case CASINO_ACT1:
        casino_leave_act1(task);
        break;
    case CASINO_ACT2:
        if (!(player_work[0]->flags & (PLAYER_ON_PATH | PLAYER_BALL))) {
            if (per_info[0]->press & ROLL_BUTTON) {
                player_set_mode(0, PLAYER_MODE_SLOPE_ROLL);
            }
        }
        casino_act2_lose_ball(task);
        break;
    case CASINO_ACT3:
        if (!(player_work[0]->flags & (PLAYER_ON_PATH | PLAYER_BALL))) {
            if (per_info[0]->press & ROLL_BUTTON) {
                player_set_mode(0, PLAYER_MODE_SLOPE_ROLL);
            }
        }
        casino_act3_lose_ball(task);
        if (player_work[0]->pos.y > 0.0F) {
            world_tilt_gravity(TABLE_TILT, 0);
        } else {
            world_tilt_gravity(-TABLE_TILT, 0);
        }
        break;
    }
}

void casino_set_texture(int acts)
{
    if (acts == CASINO_TEXTURES_ACTS_12) {
        njSetTexture(&obj_casino9_texlist);
    } else if (acts == CASINO_TEXTURES_ACTS_34) {
        njSetTexture(&obj_casino_texlist);
    } else {
        for (;;) {
        }
    }
}
