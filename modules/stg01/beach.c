/* Emerald Coast's stage task and sky task. the stage task starts each act,
 * and the sky task draws each act's sky and lens flares */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "effect/lens.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg01/beach.h"
#include "stg01/cycle.h"
#include "stg01/footprints.h"
#include "stg01/orca_control.h"
#include "stg01/sea_surface.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_TEXLIST beach_sky_texlist;
extern NJS_OBJECT beach_sky_object_0;
extern NJS_OBJECT beach_sky_object_1;

/* the music task waits MUSIC_DELAY steps. it asks for the act's music on
 * the step after */
#define MUSIC_DELAY 10

/* the sounds of the waves and the surf. each act's start plays both, held */
#define SE_WAVES 926
#define SE_SURF 928

/* player 0 leaves act 0 for act 1 within EXIT_NEAR of act0_exit */
#define EXIT_NEAR 22.0F
/* act 1 draws its lens flare only while player 0 is within SUN_NEAR of
 * sun_point */
#define SUN_NEAR 1000.0F
/* act 2 fogs the view in place of its sky while player 0 is below FOG_Y. a
 * player must also be within FOG_NEAR of water_point */
#define FOG_NEAR 880.0F
#define FOG_Y (-70.0F)

/* sky modes, three for each act: start the act's sky, draw it, and reset it */
#define SKY_MODES 3
enum sky_mode { SKY_START, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))

/* the back color: BACK_COLOR for acts 0 and 1, and BACK_COLOR_ACT2 for act
 * 2 */
#define BACK_COLOR 0x3946
#define BACK_COLOR_ACT2 0x444cff

/* act 2's fog table. act 2's sky makes it again with njGenerateFogTable3
 * as it starts */
static NJS_FOG_TABLE beach_fog_table = {
#include "stg01/beach/beach_fog_table.inc"
};

static void beach_music_wait_step(struct task *task);
static void beach_act_start(struct task *task);
static void beach_end(struct task *task);
static void beach_sky_run(struct task *task);
static void beach_sky_draw(struct task *task);
static void beach_sky_destroy(struct task *task);
static void beach_act0_step(struct task *task);

void beach_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BEACH_INIT:
        beach_act_start(task);
        break;
    case BEACH_ACT0:
        beach_act0_step(task);
        break;
    case BEACH_ACT1:
    case BEACH_ACT2:
        break;
    case BEACH_END:
        beach_end(task);
        break;
    }
}

/* the music task. once work->timer counts down below 0, it resets the
 * streams and asks for the music in work->mode. then the task ends */
static void beach_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        stream_reset();
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* start the act the stage is in: the held sounds, the act's tasks and its
 * music. as Tails, starting act 1 also ends it as a clear. an act past 2
 * starts only the sounds, and takes the mode BEACH_ACT0 */
static void beach_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *made;
    Uint16 stage;

    sound_play_held(SE_WAVES, -1, 8, 0);
    sound_play_held(SE_SURF, -1, 8, 0);
    stage = stage_get_stage_act();
    if ((Uint8)stage == 0) {
        orca_control_start();
        if ((made = task_create(TASK_WORK, 3, footprints_start)) != NULL) {
            made->work->scl.x = 0.0F;
        }
        task_create(TASK_WORK | TASK_EXTRA, 3, beach_act0_sand_step);
        task_create(TASK_WORK | TASK_EXTRA, 3, beach_act0_wave_step);
        made = task_create(TASK_WORK, 0, beach_music_wait_step);
        made->work->mode = MUSIC_ECOAST1;
        made->work->timer = MUSIC_DELAY;
        work->mode = BEACH_ACT0;
    } else if ((Uint8)stage == 1) {
        if ((made = task_create(TASK_WORK, 3, footprints_start)) != NULL) {
            made->work->scl.x = 0.0F;
        }
        beach_sea_surface_create(BEACH_ACT1);
        task_create(TASK_WORK | TASK_EXTRA, 1, beach_act1_sand_step);
        task_create(TASK_WORK | TASK_EXTRA, 1, beach_act1_wave_step);
        task_create(TASK_WORK | TASK_EXTRA, 1, beach_act1_water_step);
        made = task_create(TASK_WORK, 0, beach_music_wait_step);
        made->work->mode = MUSIC_ECOAST2;
        made->work->timer = MUSIC_DELAY;
        work->mode = BEACH_ACT1;
        if (get_character() == CHARACTER_TAILS) {
            stage_end(STAGE_END_CLEAR);
        }
    } else if ((Uint8)stage == 2) {
        if ((made = task_create(TASK_WORK, 3, footprints_start)) != NULL) {
            made->work->scl.x = 0.0F;
        }
        beach_sea_surface_create(BEACH_ACT2);
        task_create(TASK_WORK | TASK_EXTRA, 1, beach_act2_sand_step);
        task_create(TASK_WORK | TASK_EXTRA, 1, beach_act2_wave_step);
        task_create(TASK_WORK | TASK_EXTRA, 1, beach_act2_water_step);
        made = task_create(TASK_WORK, 0, beach_music_wait_step);
        made->work->mode = MUSIC_ECOAST3;
        made->work->timer = MUSIC_DELAY;
        work->mode = BEACH_ACT2;
    } else {
        /* @hack redundant: the jump to the next statement, which puts
         * BEACH_ACT0 in R2 for its store (MOV #1,R2 at 0x0c90021e); without it,
         * 2 of 168 words differ */
        goto other_act;
    other_act:
        work->mode = BEACH_ACT0;
    }
}

static void beach_end(struct task *task)
{
    Uint16 act = (Uint8)stage_get_stage_act();

    if (act == 0) {
        orca_control_end();
    }
    if (act == 2) {
        stage_end(STAGE_END_CLEAR);
    }
}

void beach_sky_step(struct task *task)
{
    task->step = beach_sky_run;
    task->draw = beach_sky_draw;
    task->destructor = beach_sky_destroy;
}

/* end the sky task and its children off Emerald Coast. once the act
 * changes, a drawing sky goes to its reset. the reset returns it to
 * SKY_MODE(0, SKY_START), act 0's start. a sky at any act's start goes to
 * the new act's start */
static void beach_sky_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = (Uint8)stage;

    stage &= STAGE_MASK;
    if (stage != STAGE_ACT(STAGE_BEACH, 0)) {
        task_kill(task);
        task_destroy_children(task);
        return;
    }
    if (work->mode / SKY_MODES != act) {
        if (work->mode % SKY_MODES == SKY_DRAW) {
            ++work->mode;
        } else if (work->mode % SKY_MODES == SKY_START) {
            work->mode = SKY_MODE(act, SKY_START);
        }
    }
    beach_sky_draw(task);
}

/* each act's sky, about the camera's x and z, and its lens flares. act 2's
 * draw also steps the task's children. FOG_NEAR and FOG_Y say where it fogs
 * the view in place of the sky */
static void beach_sky_draw(struct task *task)
{
    static const NJS_POINT3 sun_point = {6235.0F, 0.0F, -2275.0F};
    static const NJS_POINT3 water_point = {6152.0F, -85.0F, 2383.0F};
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 sun;
    NJS_POINT3 point;

    sun = sun_point;
    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (work->mode) {
    case SKY_MODE(0, SKY_START):
        njSetBackColor(BACK_COLOR, BACK_COLOR, BACK_COLOR);
        njSetFogColor(0);
        njFogDisable();
        work->mode = SKY_MODE(0, SKY_DRAW);
        /* falls through, so the sky draws on the step it starts */
    case SKY_MODE(0, SKY_DRAW):
        point.x = 10000.0F;
        point.y = 4000.0F;
        point.z = -590.0F;
        lens_draw(&point);
        point.x = -1500.0F;
        point.y = 3000.0F;
        point.z = 4000.0F;
        lens_draw(&point);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njSetTexture(&beach_sky_texlist);
        draw_tree(&beach_sky_object_0);
        draw_tree(&beach_sky_object_1);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_MODE(0, SKY_RESET):
        beach_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(1, SKY_START):
        njSetBackColor(BACK_COLOR, BACK_COLOR, BACK_COLOR);
        if (!event_flag_check(FLAG_SONIC_CLEAR_BEACH)) {
            texture_load_character(CHARACTER_TAILS);
        }
        njFogDisable();
        work->mode = SKY_MODE(1, SKY_DRAW);
        /* falls through, so the sky draws on the step it starts */
    case SKY_MODE(1, SKY_DRAW):
        if (njDistanceP2P(&player->pos, &sun) < SUN_NEAR) {
            point.x = 17000.0F;
            point.y = 3500.0F;
            point.z = -2000.0F;
            lens_draw(&point);
        }
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njSetTexture(&beach_sky_texlist);
        draw_tree(&beach_sky_object_0);
        draw_tree(&beach_sky_object_1);
        njPopMatrix(1);
        break;
    case SKY_MODE(1, SKY_RESET):
        beach_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(2, SKY_START):
        njSetFogColor(0xffffffff);
        njGenerateFogTable3(beach_fog_table, -10.0F, -1600.0F);
        njSetFogTable(beach_fog_table);
        njSetBackColor(BACK_COLOR_ACT2, BACK_COLOR_ACT2, BACK_COLOR_ACT2);
        njSetDepthQueue(-1700.0F, -1900.0F);
        njClipZ(-3.0F, -2000.0F);
        work->mode = SKY_MODE(2, SKY_DRAW);
        njFogDisable();
        break;
    case SKY_MODE(2, SKY_DRAW):
        point.x = 6158.0F;
        point.y = 3500.0F;
        point.z = 2385.0F;
        lens_draw(&point);
        task_step_children(task);
        njFogDisable();
        point = water_point;
        if (player_in_range(&point, FOG_NEAR) &&
            player_work[0]->pos.y < FOG_Y) {
            njFogEnable();
        } else {
            njPushMatrix(NULL);
            njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
            njSetTexture(&beach_sky_texlist);
            draw_tree(&beach_sky_object_0);
            draw_tree(&beach_sky_object_1);
            njPopMatrix(1);
        }
        break;
    case SKY_MODE(2, SKY_RESET):
        beach_sky_destroy(task);
        work->mode = 0;
        break;
    default:
        /* @bug memory: a mode past 8 hands heap_free the task itself (JSR at
         * 0x0c9006f2), though tasks come from task_init's pool, not from
         * heap_alloc. SKY_MODE gives acts 0 to 2 the modes 0 to 8.
         * beach_sky_run sets a mode past them only for an act of 3 or more */
        heap_free(task);
        break;
    }
    draw_control_restore();
}

/* act 1's and act 2's skies, drawing or resetting, remove the sea surface.
 * act 1's also releases Tails' textures, which its start loaded, and act
 * 2's turns the fog off */
static void beach_sky_destroy(struct task *task)
{
    switch (task->work->mode) {
    /* @hack redundant: the cases with only a break, act 0's modes and
     * SKY_START in acts 1 and 2. SHC tests each of modes 0 to 3 and 6 and
     * branches to the exit (0x0c900718 to 0x0c900746); without them, 53 of 56
     * words differ */
    case SKY_MODE(0, SKY_START):
    case SKY_MODE(0, SKY_DRAW):
    case SKY_MODE(0, SKY_RESET):
    case SKY_MODE(1, SKY_START):
    case SKY_MODE(2, SKY_START):
        break;
    case SKY_MODE(1, SKY_DRAW):
    case SKY_MODE(1, SKY_RESET):
        beach_sea_surface_remove();
        if (!event_flag_check(FLAG_SONIC_CLEAR_BEACH)) {
            texture_release_character(CHARACTER_TAILS);
        }
        break;
    case SKY_MODE(2, SKY_DRAW):
    case SKY_MODE(2, SKY_RESET):
        beach_sea_surface_remove();
        njFogDisable();
        break;
    default:
        return;
    }
}

static void beach_act0_step(struct task *task)
{
    static const NJS_POINT3 act0_exit = {5746.0F, 406.0F, 655.0F};
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 exit;

    exit = act0_exit;
    if (player == NULL) {
        return;
    }
    if (njDistanceP2P(&exit, &player->pos) < EXIT_NEAR) {
        work->mode = BEACH_INIT;
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player);
    }
}
