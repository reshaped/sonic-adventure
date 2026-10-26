/* Windy Valley stage task, and the sky task that sets each act's fog and draws
 * its sky */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/lens.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg02/debris.h"
#include "stg02/tornado.h"
#include "stg02/windy.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_TEXLIST windy_back_texlist;
extern NJS_OBJECT windy_back_object_0;
extern NJS_OBJECT windy_back_object_1;
extern NJS_OBJECT windy_back_object_2;
extern NJS_OBJECT windy_back_object_3;
extern NJS_OBJECT windy_back_object_4;
extern NJS_TEXLIST windy_back2_texlist;
extern NJS_OBJECT windy_back2_object_0;
extern NJS_OBJECT windy_back2_object_1;
extern NJS_OBJECT windy_back2_object_2;
extern NJS_TEXLIST windy_back3_texlist;
extern NJS_OBJECT windy_back3_object_0;
extern NJS_OBJECT windy_back3_object_1;
extern NJS_OBJECT windy_back3_object_2;
extern NJS_OBJECT windy_back3_object_3;

enum windy_mode {
    WINDY_INIT, /* start the current act's music and wind */
    WINDY_TORNADO, /* act 0, which ends once windy_tornado_done returns TRUE */
    WINDY_CLIMB, /* act 1, which ends once player 0 is above CLIMB_HEIGHT */
    WINDY_LAST, /* act 2, which lasts to the stage's end */
    WINDY_END
};

#define CLIMB_HEIGHT 2250.0F

/* the steps the music's task waits before the step that requests the act's
 * music */
#define MUSIC_DELAY 10

#define SE_WIND_ACT0 932
#define SE_WIND_ACT1 933
#define SE_WIND_ACT2 934

/* the sky task's mode is SKY_MODE(act, mode), for its act and one of these.
 * SKY_START loads the act's textures and sets its fog. SKY_DRAW draws the sky.
 * SKY_RESET releases the textures and sets the mode back to 0 */
#define SKY_MODES 3
enum sky_mode { SKY_START, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))

struct sky_vars {
    /* the constant material alpha act 0's sky draws with, an offset. it falls
     * from 0 as the storm gathers, and the sky fades into the black back
     * color */
    Float fade;
    Sint8 unk_04;
};

#define SKY_VARS(work) ((struct sky_vars *)&(work)->vars)

/* the sky task's work->submode in act 0. each storm mode starts once player 0
 * comes within its reach of the tornado's eye */
enum storm_mode { STORM_CLEAR, STORM_NEAR, STORM_NEARER, STORM_NEAREST };
#define STORM_NEAR_REACH 680.0F
#define STORM_NEARER_REACH 310.0F
#define STORM_NEAREST_REACH 170.0F

/* the far end of act 0's fog, as njGenerateFogTable3 takes it, before the
 * storm draws it in */
#define ACT0_FOG_FAR (-6500.0F)

/* in each storm mode, TASK_TIMER(work) counts how far the fog's far end has
 * closed in from ACT0_FOG_FAR. STORM_NEAR closes it NEAR_CLOSE a step, up to
 * NEAR_CLOSE_MAX, and STORM_NEARER NEARER_CLOSE a step, up to
 * NEARER_CLOSE_MAX. STORM_NEAREST opens it NEAREST_OPEN a step, back to
 * NEAREST_CLOSE_MIN */
#define NEAR_CLOSE 35
#define NEAR_CLOSE_MAX 3900
#define NEARER_CLOSE 45
#define NEARER_CLOSE_MAX 6280
#define NEAREST_OPEN 40
#define NEAREST_CLOSE_MIN 5500
/* in each storm mode, the sky's fade falls FADE_STEP a step, to FADE_MIN */
#define FADE_STEP 0.01F
#define FADE_MIN (-0.8F)

/* act 1's sky turns this much about y each step */
#define SKY_TURN 345

#define ZONE_ENTER_REACH 100.0F
#define ZONE_LEAVE_REACH 700.0F

#define SECOND_BLOCK_REACH 525.0F

/* the sky's fog table. each act's sky generates it again as it starts. act 0's
 * storm generates it again each step */
NJS_FOG_TABLE windy_fog_table = {
#include "stg02/windy/windy_fog_table.inc"
};

/* TRUE once a player comes within ZONE_ENTER_REACH of zone_entry in act 2. it
 * goes back to FALSE once neither player is within ZONE_LEAVE_REACH of
 * zone_center */
static Bool windy_zone_entered = FALSE;

NJS_COLOR windy_fog_color;
Uint32 windy_fog_shown;

static void windy_act_start(struct task *task);
static void windy_end(struct task *task);
static void windy_sky_run(struct task *task);
static void windy_sky_draw(struct task *task);
static void windy_sky_destroy(struct task *task);
static void windy_storm(struct task *task);
static void windy_keep_gravity(struct task *task);
static void windy_music_wait_step(struct task *task);
static void windy_act0_step(struct task *task);
static void windy_act1_step(struct task *task);
static void windy_act2_land_blocks(void);

void windy_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case WINDY_INIT:
        windy_act_start(task);
        break;
    case WINDY_TORNADO:
        windy_act0_step(task);
        break;
    case WINDY_CLIMB:
        windy_act1_step(task);
        break;
    case WINDY_LAST:
        break;
    case WINDY_END:
        windy_end(task);
        break;
    }
}

static void windy_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;
    Uint8 act = stage_get_stage_act();

    switch (act) {
    case 0:
        work->mode = WINDY_TORNADO;
        music = task_create(TASK_WORK, 0, windy_music_wait_step);
        music->work->mode = MUSIC_WNDYVLY1;
        music->work->timer = MUSIC_DELAY;
        sound_play_held(SE_WIND_ACT0, -1, 8, 0);
        break;
    case 1:
        music = task_create(TASK_WORK, 0, windy_music_wait_step);
        music->work->mode = MUSIC_WNDYVLY2;
        music->work->timer = MUSIC_DELAY;
        work->mode = WINDY_CLIMB;
        sound_play_held(SE_WIND_ACT1, -1, 8, 0);
        break;
    case 2:
        music = task_create(TASK_WORK, 0, windy_music_wait_step);
        music->work->mode = MUSIC_WNDYVLY3;
        music->work->timer = MUSIC_DELAY;
        work->mode = WINDY_LAST;
        sound_play_held(SE_WIND_ACT2, -1, 8, 0);
        break;
    }
}

static void windy_end(struct task *task)
{
}

void windy_sky_step(struct task *task)
{
    task->step = windy_sky_run;
    task->draw = windy_sky_draw;
    task->destructor = windy_sky_destroy;
}

/* kill the task outside Windy Valley. once the act differs from the sky's, a
 * sky in SKY_DRAW moves to its SKY_RESET. a sky in SKY_START moves to the new
 * act's SKY_START. the sky then runs its mode */
static void windy_sky_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = (Uint8)stage;

    stage &= STAGE_MASK;
    if (stage != STAGE_ACT(STAGE_WINDY, 0)) {
        task_kill(task);
        return;
    }
    if (work->mode / SKY_MODES != act) {
        if (work->mode % SKY_MODES == SKY_DRAW) {
            ++work->mode;
        } else if (work->mode % SKY_MODES == SKY_START) {
            work->mode = SKY_MODE(act, SKY_START);
        }
    }
    windy_sky_draw(task);
}

/* run the sky's mode. windy_sky_run calls it once a step. it is also the task's
 * draw, which runs on paused frames. so the sky turns and the storm gathers
 * while the game is paused */
static void windy_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float twice;
    Float thrice;
    NJS_POINT3 sun;

    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (work->mode) {
    case SKY_MODE(0, SKY_START):
        njSetBackColor(0, 0, 0);
        texture_load_pvm("WINDY_BACK", &windy_back_texlist);
        njSetFogColor(0xffffffff);
        /* act 0 leaves setting this table to 1ST_READ.BIN's windy_set_fog, at
         * the end of each frame */
        njGenerateFogTable3(windy_fog_table, -10.0F, ACT0_FOG_FAR);
        SKY_VARS(work)->unk_04 = 0;
        windy_fog_color.argb.a = 0xff;
        windy_fog_color.argb.r = 0xff;
        windy_fog_color.argb.g = 0xff;
        windy_fog_color.argb.b = 0xff;
        windy_fog_shown = windy_fog_color.color;
        njFogEnable();
        SKY_VARS(work)->fade = 0.0F;
        work->mode = SKY_MODE(0, SKY_DRAW);
        /* no break: SKY_START falls into SKY_DRAW, so the sky draws on this
         * step too */
    case SKY_MODE(0, SKY_DRAW):
        /* the storm gathers for every character but E-102 */
        if (get_character() != CHARACTER_E102) {
            windy_storm(task);
        }
        njSetTexture(&windy_back_texlist);
        njFogDisable();
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        draw_set_material(SKY_VARS(work)->fade, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        draw_tree(&windy_back_object_0);
        draw_tree(&windy_back_object_1);
        draw_tree(&windy_back_object_2);
        draw_tree(&windy_back_object_3);
        draw_tree(&windy_back_object_4);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njFogEnable();
        draw_attr_restore();
        break;
    case SKY_MODE(0, SKY_RESET):
        windy_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(1, SKY_START):
        njSetBackColor(0xffffff, 0xffffff, 0xffffff);
        texture_load_pvm("WINDY_BACK2", &windy_back2_texlist);
        njSetFogColor(0xffffffff);
        njGenerateFogTable3(windy_fog_table, -10.0F, -1200.0F);
        njSetFogTable(windy_fog_table);
        njFogEnable();
        work->mode = SKY_MODE(1, SKY_DRAW);
        /* no break: SKY_START falls into SKY_DRAW, so the sky draws on this
         * step too */
    case SKY_MODE(1, SKY_DRAW):
        /* before the sky's first layer, the matrix turns by work->ang.y.
         * before the second it turns by twice that, and before the third by
         * three times */
        njSetTexture(&windy_back2_texlist);
        njFogDisable();
        work->ang.y += SKY_TURN;
        njPushMatrix(NULL);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&windy_back2_object_0);
        /* @hack spelling: each product compared with 0.0F, where TURN_VIA
         * tests it bare. the comparison makes the turn read the product back
         * from the frame, by the FMOV.S at 0x0c90048a and 0x0c9004b4; without
         * them, 45 of 394 words differ */
        ((twice = work->ang.y * 2.0F) != 0.0F) ? njRotateY(NULL, twice)
                                               : (void)0;
        draw_tree(&windy_back2_object_1);
        ((thrice = work->ang.y * 3.0F) != 0.0F) ? njRotateY(NULL, thrice)
                                                : (void)0;
        draw_tree(&windy_back2_object_2);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_MODE(1, SKY_RESET):
        windy_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(2, SKY_START):
        njSetBackColor(0, 0, 0);
        texture_load_pvm("WINDY_BACK3", &windy_back3_texlist);
        njSetFogColor(0xffffffff);
        njGenerateFogTable3(windy_fog_table, -10.0F, -3000.0F);
        njSetFogTable(windy_fog_table);
        work->mode = SKY_MODE(2, SKY_DRAW);
        break;
    case SKY_MODE(2, SKY_DRAW):
        /* act 2 picks its land blocks, draws the sun's flare and keeps
         * gravity straight down */
        windy_act2_land_blocks();
        sun.x = 1096.0F;
        sun.y = 2000.0F;
        sun.z = -1251.0F;
        lens_draw(&sun);
        njSetTexture(&windy_back3_texlist);
        njFogDisable();
        work->ang.y += 4;
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, camera_work->pos.y,
                    camera_work->pos.z);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&windy_back3_object_0);
        draw_tree(&windy_back3_object_1);
        draw_tree(&windy_back3_object_2);
        draw_tree(&windy_back3_object_3);
        njPopMatrix(1);
        windy_keep_gravity(task);
        break;
    case SKY_MODE(2, SKY_RESET):
        windy_sky_destroy(task);
        work->mode = 0;
        break;
    default:
        task_kill(task);
        break;
    }
    draw_control_restore();
}

/* release the sky's textures, in the modes that have loaded them. act 2's sky
 * releases them only in SKY_RESET */
static void windy_sky_destroy(struct task *task)
{
    switch (task->work->mode) {
    /* @hack redundant: the cases with only a break, SKY_START in each act and
     * act 2's SKY_DRAW. SHC tests each of modes 0, 3, 6 and 7 with a CMP/EQ
     * and a BT to the exit (0x0c90063a, 0x0c900646, 0x0c900652, 0x0c900656);
     * without them, 30 of 38 words differ */
    case SKY_MODE(0, SKY_START):
        break;
    case SKY_MODE(0, SKY_DRAW):
    case SKY_MODE(0, SKY_RESET):
        njReleaseTexture(&windy_back_texlist);
        break;
    case SKY_MODE(1, SKY_START):
        break;
    case SKY_MODE(1, SKY_DRAW):
    case SKY_MODE(1, SKY_RESET):
        njReleaseTexture(&windy_back2_texlist);
        break;
    case SKY_MODE(2, SKY_START):
    case SKY_MODE(2, SKY_DRAW):
        break;
    case SKY_MODE(2, SKY_RESET):
        njReleaseTexture(&windy_back3_texlist);
        break;
    default:
        return;
    }
}

/* gather the storm as player 0 nears the tornado's eye. the storm's modes only
 * advance. STORM_CLEAR keeps the sky and the fog clear. in the other modes, the
 * sky fades, and the fog's far end closes in. each channel of the fog's color
 * falls by 1 to 1. it falls every step in STORM_NEAR, and every other step
 * after it.
 * STORM_NEARER also starts the haze about the camera.
 *
 * the fog's table takes closing, the timer read before its clamp. so the far
 * end passes each bound by up to one step's change */
static void windy_storm(struct task *task)
{
    static const NJS_POINT3 storm_tornado_eye = {3254.0F, -421.0F, -1665.0F};
    struct task_work *work = task->work;
    struct task *player;
    NJS_POINT3 eye;
    Float distance;
    Float closing;

    eye = storm_tornado_eye;
    if ((player = player_get_task(0)) == NULL) {
        return;
    }
    distance = njDistanceP2P(&player->work->pos, &eye);
    switch (work->submode) {
    case STORM_CLEAR:
        if (distance < STORM_NEAR_REACH) {
            work->submode = STORM_NEAR;
            work->timer = 0;
        }
        SKY_VARS(work)->fade = 0.0F;
        windy_fog_color.argb.a = 0xff;
        windy_fog_color.argb.r = 0xff;
        windy_fog_color.argb.g = 0xff;
        windy_fog_color.argb.b = 0xff;
        windy_fog_shown = windy_fog_color.color;
        return;
    case STORM_NEAR:
        work->tick++;
        if (work->tick >= 1) {
            work->tick = 0;
            windy_fog_color.argb.a--;
            windy_fog_color.argb.r--;
            windy_fog_color.argb.g--;
            windy_fog_color.argb.b--;
            windy_fog_shown = windy_fog_color.color;
        }
        SKY_VARS(work)->fade -= FADE_STEP;
        TASK_TIMER(work) += NEAR_CLOSE;
        closing = TASK_TIMER(work);
        if (closing > NEAR_CLOSE_MAX) {
            TASK_TIMER(work) = NEAR_CLOSE_MAX;
        }
        if (distance < STORM_NEARER_REACH) {
            task_create(TASK_WORK, 2, windy_haze_step);
            work->submode = STORM_NEARER;
        }
        break;
    case STORM_NEARER:
        SKY_VARS(work)->fade -= FADE_STEP;
        work->tick++;
        if (work->tick > 1) {
            work->tick = 0;
            windy_fog_color.argb.a--;
            windy_fog_color.argb.r--;
            windy_fog_color.argb.g--;
            windy_fog_color.argb.b--;
            windy_fog_shown = windy_fog_color.color;
        }
        TASK_TIMER(work) += NEARER_CLOSE;
        closing = TASK_TIMER(work);
        if (closing > NEARER_CLOSE_MAX) {
            TASK_TIMER(work) = NEARER_CLOSE_MAX;
        }
        if (distance < STORM_NEAREST_REACH) {
            work->submode = STORM_NEAREST;
        }
        break;
    case STORM_NEAREST:
        SKY_VARS(work)->fade -= FADE_STEP;
        work->tick++;
        if (work->tick > 1) {
            work->tick = 0;
            windy_fog_color.argb.a--;
            windy_fog_color.argb.r--;
            windy_fog_color.argb.g--;
            windy_fog_color.argb.b--;
            windy_fog_shown = windy_fog_color.color;
        }
        TASK_TIMER(work) -= NEAREST_OPEN;
        closing = TASK_TIMER(work);
        if (closing < NEAREST_CLOSE_MIN) {
            TASK_TIMER(work) = NEAREST_CLOSE_MIN;
        }
        break;
    }
    if (windy_fog_color.argb.a == 0) {
        windy_fog_color.argb.a = 1;
        windy_fog_color.argb.r = 1;
        windy_fog_color.argb.g = 1;
        windy_fog_color.argb.b = 1;
        windy_fog_shown = windy_fog_color.color;
    }
    if (work->submode != STORM_CLEAR) {
        njGenerateFogTable3(windy_fog_table, -10.0F, ACT0_FOG_FAR + closing);
    }
    if (SKY_VARS(work)->fade < FADE_MIN) {
        SKY_VARS(work)->fade = FADE_MIN;
    }
}

/* keep act 2's gravity straight down while player 0 exists. the zone's tests
 * set and clear windy_zone_entered. each branch tilts gravity by 0, so the zone
 * changes nothing */
static void windy_keep_gravity(struct task *task)
{
    static const NJS_POINT3 zone_entry = {5321.0F, -8692.0F, 880.0F};
    static const NJS_POINT3 zone_center = {5321.0F, -9884.0F, 847.0F};
    NJS_POINT3 entry;
    NJS_POINT3 center;
    struct task_work *player = player_work[0];

    entry = zone_entry;
    center = zone_center;
    if (player != NULL) {
        world_tilt_gravity(0, 0);
        if (player_in_range(&entry, ZONE_ENTER_REACH) ||
            windy_zone_entered == TRUE) {
            windy_zone_entered = TRUE;
        } else if (!windy_zone_entered) {
            world_tilt_gravity(0, 0);
        }
        if (windy_zone_entered == TRUE) {
            if (!player_in_range(&center, ZONE_LEAVE_REACH)) {
                world_tilt_gravity(0, 0);
                windy_zone_entered = FALSE;
            }
        }
    }
}

/* the music's task. work->mode holds the music to request, and work->timer
 * the steps left to wait */
static void windy_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        task_create(0, 1, stream_init_step);
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

static void windy_act0_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    if (player != NULL && windy_tornado_done()) {
        work->mode = WINDY_INIT;
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player);
    }
}

static void windy_act1_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    if (player != NULL && player->pos.y > CLIMB_HEIGHT) {
        work->mode = WINDY_INIT;
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player);
    }
}

/* draw the second land block alone while player 0 is within
 * SECOND_BLOCK_REACH of second_block_point. otherwise draw the first alone */
static void windy_act2_land_blocks(void)
{
    static const NJS_POINT3 second_block_point = {4136.0F, -4460.0F, -1620.0F};
    NJS_POINT3 point;

    point = second_block_point;
    if (player_in_range(&point, SECOND_BLOCK_REACH) == 1) {
        world_land_blocks = 0x2;
    } else {
        world_land_blocks = 0x1;
    }
}
