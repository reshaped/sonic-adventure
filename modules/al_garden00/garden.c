/* the Station Square garden: the stage's task */
#include <ninja.h>

#include "al_garden00/door.h"
#include "al_garden00/garden.h"
#include "al_main/alife.h"
#include "al_main/camera.h"
#include "al_main/card.h"
#include "al_main/chao.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "debug/editor.h"
#include "draw/light.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"

extern NJS_MATERIAL water_material;

/* water_material's texture cycles from 0 to WATER_LAST, a texture every
 * third step */
#define WATER_LAST 9

/* the field of view the start gives camera_set_perspective, which
 * garden_camera_step soon replaces with the zoom's: 30, 49 or 68 degrees */
#define GARDEN_PERSPECTIVE NJM_DEG_ANG(80)
#define PERSPECTIVE NJM_DEG_ANG(70) /* the one the destructor puts back */

/* player 0 leaves for the race once within EXIT_RANGE of (0, 0, -250) */
#define EXIT_RANGE 80.0F
#define CEILING 62.0F /* the step holds player 0's y at or below this */

enum garden_mode { GARDEN_INIT, GARDEN_WAIT, GARDEN_RUN };

static void station_square_garden_start(struct task *task);
static void station_square_garden_destroy(struct task *task);
static void station_square_garden_light_load(struct task *task);

void station_square_garden_step(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode != 0) {
        camera_restore();
    }
    switch (work->mode) {
    case GARDEN_INIT:
        alife_create();
        station_square_garden_start(task);
        task->destructor = station_square_garden_destroy;
        station_square_garden_light_load(task);
        work->mode++;
        work->timer = 0;
        break;
    case GARDEN_WAIT:
        if (TASK_TIMER(work)++ > 3) {
            camera_override_high(garden_camera_step, 1, CAMERA_LOOK_AT);
            work->mode++;
        }
        break;
    case GARDEN_RUN:
        if (work->tick++ > 1) {
            water_material.attr_texId++;
            if (water_material.attr_texId > WATER_LAST) {
                water_material.attr_texId = 0;
            }
            work->tick = 0;
        }
        {
            NJS_POINT3 exit = {0.0F, 0.0F, -250.0F};

            if (njDistanceP2P(&player_work[0]->pos, &exit) < EXIT_RANGE) {
                garden_flags_save();
                alife_save();
                stage_fade_to(STAGE_ALIFE_RACE, 0);
                task_kill(task);
            }
        }
        if (player_work[0]->pos.y > CEILING) {
            player_work[0]->pos.y = CEILING;
        }
        break;
    }
}

static void station_square_garden_start(struct task *task)
{
    chao_gardens_init();
    alife_chao_create_garden(GARDEN_STATION_SQUARE);
    garden_flags_load();
    camera_set_perspective(GARDEN_PERSPECTIVE);
    stream_reset();
    stream_request_music(MUSIC_CHAO);
}

static void station_square_garden_destroy(struct task *task)
{
    camera_set_perspective(PERSPECTIVE);
    camera_restore();
    njFogDisable();
    stream_destroy();
}

/* set the work's submode to the time of day, and read that time's light
 * sources and palettes, which are act 0's for every time. a time other than
 * TIME_DAY, TIME_EVENING and TIME_NIGHT sets a red, green and blue
 * background instead */
static void station_square_garden_light_load(struct task *task)
{
    struct task_work *work = task->work;

    work->submode = story_get_time();
    /* @hack spelling: the same reads in a case of each time's own, which SHC
     * keeps as one arm behind a CMP/EQ for each time (0x0cb8019c to
     * 0x0cb801a4); with the three labels on one case, 35 of 39 words differ */
    switch (work->submode) {
    case TIME_DAY:
        light_read_disc_sources(STAGE_ALIFE_SS, 0);
        light_read_disc_palettes(STAGE_ALIFE_SS, 0);
        break;
    case TIME_EVENING:
        light_read_disc_sources(STAGE_ALIFE_SS, 0);
        light_read_disc_palettes(STAGE_ALIFE_SS, 0);
        break;
    case TIME_NIGHT:
        light_read_disc_sources(STAGE_ALIFE_SS, 0);
        light_read_disc_palettes(STAGE_ALIFE_SS, 0);
        break;
    default:
        njSetBackColor(0xff0000, 0xff00, 0xff);
        break;
    }
}
