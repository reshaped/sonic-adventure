/* the ruin's stage task and sky task. the stage task starts each act's
 * music. at the ends of acts 0 and 1, it moves player 0 on to the next
 * act */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/light.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg07/mirror.h"
#include "stg07/obj_ruin_textures.h"
#include "stg07/ruin.h"
#include "stg07/textures.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_MODEL ruin_sky_dome_model;
extern NJS_MODEL ruin_sky_floor_model;
extern NJS_FOG_TABLE ruin_fog;

/* player 0 leaves act 0 once within ACT0_END_RANGE of its end, and act 1
 * once within ACT1_END_RANGE */
#define ACT0_END_RANGE 40.0F
#define ACT1_END_RANGE 50.0F

/* the steps ruin_music_wait_step waits before it asks for the act's
 * music */
#define MUSIC_DELAY 10

/* a bit of the stage task's work flags, which act 2's start clears. no code
 * in STG07.BIN sets it */
#define RUIN_UNK_8000 0x8000

/* acts 0 and 2's fog, white from FOG_NEAR to FOG_FAR, as z values */
#define FOG_NEAR (-10.0F)
#define FOG_FAR (-1600.0F)

/* act 1's sky draws no dome near two places. near is within COVERED_0_REACH of
 * one, or COVERED_1_REACH of the other */
#define COVERED_0_REACH 860.0F
#define COVERED_1_REACH 5300.0F

/* the stage task's work mode. RUIN_INIT starts the act's mode and music.
 * no code sets RUIN_UNUSED */
enum ruin_mode { RUIN_INIT, RUIN_ACT0, RUIN_ACT1, RUIN_ACT2, RUIN_UNUSED };

static void ruin_sky_run(struct task *task);
static void ruin_sky_draw(struct task *task);
static void ruin_sky_destroy(struct task *task);

/* a task whose tick counts down to 0. it then resets the streams, asks for
 * the music its mode names, and ends */
static void ruin_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    if (--work->tick <= 0) {
        stream_reset();
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* at act 0's end, change the act to 1 and place player 0 in it.
 * ruin_act_start then starts the act */
static void ruin_act0_step(struct task *task)
{
    NJS_POINT3 end = {6111.0F, -2445.0F, 1333.0F};

    if (player_in_range(&end, ACT0_END_RANGE) == 1) {
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player_work[0]);
        task->work->mode = RUIN_INIT;
    }
}

/* the same at act 1's end, which changes the act to 2 */
static void ruin_act1_step(struct task *task)
{
    NJS_POINT3 end = {6441.0F, -2421.0F, 1162.0F};

    if (njDistanceP2P(&end, &player_work[0]->pos) < ACT1_END_RANGE) {
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player_work[0]);
        task->work->mode = RUIN_INIT;
    }
}

/* set the act's mode, and ask for its music MUSIC_DELAY steps on. Knuckles
 * hears act 2's music in act 1 */
static void ruin_act_start(struct task *task)
{
    struct task *music = task_create(TASK_WORK, 0, ruin_music_wait_step);

    music->work->tick = MUSIC_DELAY;
    switch ((Uint8)stage_get_stage_act()) {
    case 0:
        task->work->mode = RUIN_ACT0;
        music->work->mode = MUSIC_LSTWRLD1;
        break;
    case 1:
        task->work->mode = RUIN_ACT1;
        if (get_character() == CHARACTER_KNUCKLES) {
            music->work->mode = MUSIC_LSTWRLD3;
        } else {
            music->work->mode = MUSIC_LSTWRLD2;
        }
        break;
    case 2:
        task->work->flags &= ~RUIN_UNK_8000;
        task->work->mode = RUIN_ACT2;
        music->work->mode = MUSIC_LSTWRLD3;
        break;
    }
    /* @hack redundant: a return where the function returns anyway, which ends
     * the cases on one shared store of the music's mode (MOV.B R3,@R2 at
     * 0x0c90baca). without it, 35 of 61 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

void ruin_step(struct task *task)
{
    switch (task->work->mode) {
    case RUIN_INIT:
        ruin_act_start(task);
        break;
    case RUIN_ACT0:
        ruin_act0_step(task);
        break;
    case RUIN_ACT1:
        ruin_act1_step(task);
        break;
    case RUIN_ACT2:
    case RUIN_UNUSED:
        break;
    }
}

void ruin_sky_step(struct task *task)
{
    task->step = ruin_sky_run;
    task->draw = ruin_sky_draw;
    task->destructor = ruin_sky_destroy;
}

/* the sky task's work mode, SKY_MODES to an act, as SKY_MODE(act, mode)
 * gives it. SKY_LIGHT sets up the act's light and fog, and moves on to
 * SKY_DRAW. SKY_RESET sets the mode to 0, and in act 1 releases the sky's
 * textures */
#define SKY_MODES 3
enum sky_mode { SKY_LIGHT, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))

/* the sky task ends once the stage isn't the ruin. while the mode belongs to
 * another act, SKY_DRAW moves on to SKY_RESET. another act's SKY_LIGHT becomes
 * the current act's. SKY_RESET leaves mode 0, act 0's SKY_LIGHT, so the next
 * step sets up the current act */
static void ruin_sky_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = (Uint8)stage;

    stage &= STAGE_MASK;
    if (stage != STAGE_ACT(STAGE_RUIN, 0)) {
        task_kill(task);
        return;
    }
    if (work->mode / SKY_MODES != act) {
        if (work->mode % SKY_MODES == SKY_DRAW) {
            ++work->mode;
        } else if (work->mode % SKY_MODES == SKY_LIGHT) {
            work->mode = SKY_MODE(act, SKY_LIGHT);
        }
    }
    ruin_sky_draw(task);
}

/* at an act's SKY_LIGHT, set up its light and fog, and start its textures'
 * task. at act 1's SKY_DRAW, draw the sky's dome round the camera. near
 * either of two places, the back stays black instead */
static void ruin_sky_draw(struct task *task)
{
    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (task->work->mode) {
    case SKY_MODE(0, SKY_LIGHT):
        njSetBackColor(0, 0, 0);
        njSetFogColor(0);
        njFogDisable();
        njSetLightIntensity(&dir_light, 0.8F, 0.4F, 0.05F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 1.0F);
        njSetLightDirection(&dir_light, 0.8F, -0.866025F, 0.0F);
        njSetFogColor(0xffffffff);
        njGenerateFogTable3(ruin_fog, FOG_NEAR, FOG_FAR);
        njSetFogTable(ruin_fog);
        njFogEnable();
        task_create(TASK_WORK, 1, ruin_act0_textures_step);
        task->work->mode = SKY_MODE(0, SKY_DRAW);
        /* falls through into SKY_DRAW, where act 0 draws no sky */
    case SKY_MODE(0, SKY_DRAW):
        break;
    case SKY_MODE(0, SKY_RESET):
        ruin_sky_destroy(task);
        task->work->mode = 0;
        break;
    case SKY_MODE(1, SKY_LIGHT):
        njSetBackColor(0, 0, 0);
        texture_load_pvm("BG_RUIN", &bg_ruin_texlist);
        njSetLightIntensity(&dir_light, 0.65F, 0.8F, 0.25F);
        njSetLightColor(&dir_light, 1.0F, 0.9F, 0.9F);
        njSetLightDirection(&dir_light, 0.866025F, -0.5F, 0.0F);
        task_create(TASK_WORK, 1, ruin_act1_textures_step);
        task_create(TASK_WORK, 1, ruin_act1_fog_step);
        task->work->mode = SKY_MODE(1, SKY_DRAW);
        /* falls through into SKY_DRAW, so the dome draws from this step on */
    case SKY_MODE(1, SKY_DRAW): {
        NJS_POINT3 covered0 = {176.0F, 116.0F, 551.0F};
        NJS_POINT3 covered1 = {4453.0F, -2101.0F, 1336.0F};
        struct task_work *camera;

        /* @hack spelling: camera assigned in the test, which SHC loads into R14
         * and tests there (TST R14,R14 at 0x0c90be3c). assigned in a
         * statement of its own, 19 of ruin_sky_draw's 333 words differ */
        if ((camera = camera_work) != NULL) {
            if (njDistanceP2P(&covered0, &camera->pos) < COVERED_0_REACH ||
                njDistanceP2P(&covered1, &camera->pos) < COVERED_1_REACH) {
                njSetBackColor(0, 0, 0);
            } else {
                njFogDisable();
                njPushMatrix(NULL);
                njTranslateV(NULL, &camera_work->pos);
                njSetTexture(&bg_ruin_texlist);
                njScale(NULL, 2.0F, 2.0F, 2.0F);
                njSimpleDrawModel(&ruin_sky_dome_model);
                njSimpleDrawModel(&ruin_sky_floor_model);
                njPopMatrix(1);
                njFogEnable();
            }
        }
    } break;
    case SKY_MODE(1, SKY_RESET):
        ruin_sky_destroy(task);
        task->work->mode = 0;
        break;
    case SKY_MODE(2, SKY_LIGHT):
        njSetBackColor(0x001040, 0x401074, 0x807474);
        njSetDepthQueue(-1700.0F, -1900.0F);
        njClipZ(-3.0F, -2000.0F);
        task_create(TASK_WORK, 1, ruin_act2_textures_step);
        njSetFogColor(0xffffffff);
        njGenerateFogTable3(ruin_fog, FOG_NEAR, FOG_FAR);
        njSetFogTable(ruin_fog);
        task->work->mode = SKY_MODE(2, SKY_DRAW);
        /* falls through into SKY_DRAW, where act 2 shows nothing */
    case SKY_MODE(2, SKY_DRAW):
        njFogDisable();
        njPushMatrix(NULL);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_MODE(2, SKY_RESET):
        ruin_sky_destroy(task);
        task->work->mode = 0;
        break;
    default:
        /* @bug memory: a mode past SKY_MODE(2, SKY_RESET) would pass
         * heap_free the task, which heap_alloc didn't give (JSR at
         * 0x0c90bfd0). ruin_sky_run sets such a mode only for an act of
         * 3 or more */
        heap_free(task);
        break;
    }
    draw_control_restore();
}

/* release BG_RUIN's textures while the mode is act 1's SKY_DRAW or
 * SKY_RESET */
static void ruin_sky_destroy(struct task *task)
{
    switch (task->work->mode) {
    /* @hack redundant: the cases with only a break, every mode's but act 1's
     * SKY_DRAW and SKY_RESET. SHC tests each of modes 0 to 3 and 6 to 8 with a
     * CMP/EQ and a BT to the exit (0x0c90c064 to 0x0c90c086); without them,
     * 26 of 28 words differ */
    case SKY_MODE(0, SKY_LIGHT):
    case SKY_MODE(0, SKY_DRAW):
    case SKY_MODE(0, SKY_RESET):
    case SKY_MODE(1, SKY_LIGHT):
        break;
    case SKY_MODE(1, SKY_DRAW):
    case SKY_MODE(1, SKY_RESET):
        njReleaseTexture(&bg_ruin_texlist);
        break;
    case SKY_MODE(2, SKY_LIGHT):
    case SKY_MODE(2, SKY_DRAW):
    case SKY_MODE(2, SKY_RESET):
        break;
    }
    /* @hack redundant: a return where the function returns anyway, which gives
     * a mode no case names an RTS of its own (0x0c90c088). without it, 1 of 28
     * words differs */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}
