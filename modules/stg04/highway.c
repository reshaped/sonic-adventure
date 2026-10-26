/* Speed Highway's stage, act and sky tasks. the stage task makes the act
 * task. the act task starts each act's held sound, and makes the task that
 * asks for its music. at the ends of acts 0 and 1, it moves player 0 on to
 * the next act. the sky task sets each act's fog and light and draws its
 * sky */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/light.h"
#include "draw/model.h"
#include "effect/wind.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/race.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg04/highway.h"
#include "stg04/traffic.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the skies. act 0 draws bg and bg01, act 1 bg and bg02, and act 2 bg03 */
extern NJS_TEXLIST highway_bg_texlist;
extern NJS_TEXLIST highway_bg01_texlist;
extern NJS_TEXLIST highway_bg02_texlist;
extern NJS_TEXLIST highway_bg03_texlist;
extern NJS_OBJECT highway_bg_object;
extern NJS_OBJECT highway_bg01_object;
extern NJS_OBJECT highway_bg02_object;
extern NJS_OBJECT highway_bg03_object;

/* the stage task's work->mode once act 2 has loaded the cars' textures */
#define CARS_LOADED 3

/* the act task's modes, in its work->mode */
enum highway_mode {
    HIGHWAY_INIT,
    HIGHWAY_ACT0,
    HIGHWAY_ROOF, /* act 1's start, on the roof */
    HIGHWAY_EDGE, /* until player 0 is over the edge and falling */
    HIGHWAY_WALL, /* down the wall to the street */
    HIGHWAY_STREET,
    HIGHWAY_EXIT, /* on to act 2 */
    HIGHWAY_ACT2,
    HIGHWAY_END
};

/* the stage task and its act task each hold the other, or NULL once the
 * other goes */
struct highway_vars {
    struct task *other;
};

#define VARS(work) ((struct highway_vars *)&(work)->vars)

/* in the act task's work->flags, once act 1's music is asked for */
#define EDGE_MUSIC 0x8

/* the steps highway_music_wait_step waits before it asks for an act's
 * music. act 1's waits EDGE_MUSIC_DELAY */
#define MUSIC_DELAY 10
#define EDGE_MUSIC_DELAY 5

#define ACT0_EXIT_NEAR 60.0F

/* act 0's view angle is VIEW_NARROW, 70 degrees, widened by player 0's
 * speed ahead. above WIDEN_FROM, the widening is VIEW_WIDER times the square
 * root of (speed - WIDEN_FROM) / WIDEN_SPAN. at WIDEN_FULL, the widening
 * jumps from under 4 degrees to the whole VIEW_WIDER, about 20. the view
 * approaches the angle by VIEW_TURN a step */
#define VIEW_NARROW 0x31c7
#define VIEW_WIDER 3640.0F
#define VIEW_TURN 0x200
#define WIDEN_FROM (2.3F * 0.6F)
#define WIDEN_FULL (WIDEN_FROM * 5.0F)
#define WIDEN_SPAN 148.62F

/* act 1's run from the roof down the wall. once player 0's x reaches
 * ROOF_EDGE_X, highway_roof takes player 0's pad. EDGE_WAIT steps into
 * HIGHWAY_EDGE, player 0 runs toward EDGE_RUN_X, EDGE_RUN_Z and over the
 * edge. player 0 runs only if it is on the ground at that step. once its y
 * falls to EDGE_MUSIC_Y, the act's music is asked for. at EDGE_RUN_END_Y,
 * the run ends. at WALL_TOP_Y, player 0 falls down the wall with its pad
 * back. it stands again at WALL_FOOT_Y */
#define ROOF_EDGE_X 0.0F
#define EDGE_WAIT 300
#define EDGE_RUN_X 20.0F
#define EDGE_RUN_Z 0.0F
#define EDGE_MUSIC_Y 25.0F
#define EDGE_RUN_END_Y (-130.0F)
#define WALL_TOP_Y (-150.0F)
#define WALL_FOOT_Y (-18800.0F)

/* once player 0 reaches STREET_END_X, the act task moves on to
 * HIGHWAY_EXIT. act 1 ends once player 0 reaches ACT1_EXIT_X */
#define STREET_END_X 230.0F
#define ACT1_EXIT_X 280.0F

/* each act's held sound: SE_WIND in acts 0 and 1, whose wind blows, and
 * SE_ACT2 in act 2 */
#define SE_WIND 943
#define SE_ACT2 941

/* the wind_set level of acts 0 and 1 */
#define WIND_LEVEL 3

/* act 1 draws its sky only while player 0 is above ACT1_SKY_LOW */
#define ACT1_SKY_LOW (-10400.0F)

/* the sky task's work->mode, as SKY_MODE(act, mode). each act has three:
 * start its sky, draw it, and reset it */
#define SKY_MODES 3
enum sky_mode { SKY_START, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))

/* the sky's fog table, generated again by njGenerateFogTable3 as each act's
 * sky starts */
static NJS_FOG_TABLE highway_fog_table = {
#include "stg04/highway/highway_fog_table.inc"
};

static void highway_destroy(struct task *task);
static void highway_acts_destroy(struct task *task);
static void highway_acts_step(struct task *task);
static void highway_act2_land_blocks(struct task *task);
static void highway_music_wait_step(struct task *task);
static void highway_act_start(struct task *task);
static void highway_act0_exit(struct task *task);
static void highway_act0_widen(struct task *task);
static void highway_roof(struct task *task);
static void highway_edge(struct task *task);
static void highway_wall(struct task *task);
static void highway_street(struct task *task);
static void highway_exit(struct task *task);
static void highway_end(struct task *task);
static void highway_sky_run(struct task *task);
static void highway_sky_draw(struct task *task);
static void highway_sky_destroy(struct task *task);

/* the stage task's step. it makes the act task at any step that holds none.
 * it holds none at its first step, and once the act task goes. its first
 * step in act 2 ends the music, destroying the streams' task, and loads the
 * cars' textures */
void highway_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *acts = VARS(work)->other;

    if (!acts) {
        acts = task_create(TASK_WORK, 0, highway_acts_step);
        VARS(work)->other = acts;
        VARS(acts->work)->other = task;
        task->destructor = highway_destroy;
    }
    if ((Uint8)stage_get_stage_act() == 2 && work->mode != CARS_LOADED) {
        work->mode = CARS_LOADED;
        stream_destroy();
        texture_load_pvm("HIGHWAY_CAR", &highway_car_texlist);
    }
}

static void highway_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct task *acts = VARS(work)->other;

    if (acts) {
        VARS(acts->work)->other = NULL;
        task_destroy(acts);
    }
    if (work->mode == CARS_LOADED) {
        njReleaseTexture(&highway_car_texlist);
    }
}

static void highway_acts_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct task *stage = VARS(work)->other;

    if (stage) {
        VARS(stage->work)->other = NULL;
    }
}

static void highway_acts_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case HIGHWAY_INIT:
        highway_act_start(task);
        break;
    case HIGHWAY_ACT0:
        highway_act0_widen(task);
        highway_act0_exit(task);
        break;
    case HIGHWAY_ROOF:
        highway_roof(task);
        break;
    case HIGHWAY_EDGE:
        highway_edge(task);
        break;
    case HIGHWAY_WALL:
        highway_wall(task);
        break;
    case HIGHWAY_STREET:
        highway_street(task);
        break;
    case HIGHWAY_EXIT:
        highway_exit(task);
        break;
    case HIGHWAY_ACT2:
        highway_act2_land_blocks(task);
        break;
    case HIGHWAY_END:
        highway_end(task);
        break;
    }
}

/* the land blocks act 2 draws, by where the camera is */
static void highway_act2_land_blocks(struct task *task)
{
    if (camera_work->pos.x < -1500.0F) {
        world_land_blocks = 0x06;
    } else if (camera_work->pos.x < -600.0F) {
        world_land_blocks = 0x37;
    } else if (camera_work->pos.z < -1180.0F) {
        world_land_blocks = 0x15;
    } else if (camera_work->pos.z < -1000.0F) {
        world_land_blocks = 0x3d;
    } else if (camera_work->pos.z < -570.0F) {
        world_land_blocks = 0x2d;
    } else {
        world_land_blocks = 0x29;
    }
}

/* the music task's step. it counts work->timer down. once the count falls
 * below 0, it forgets every request to the streams and starts their task.
 * it then asks for the music in work->mode, and destroys itself */
static void highway_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* start the act the stage is in, with its mode and held sound. acts 0 and 1
 * also set the wind, and acts 0 and 2 ask for their music. act 1's music
 * waits for highway_edge. in Tails' race, act 0 also makes the task that
 * loads Eggman's textures */
static void highway_act_start(struct task *task)
{
    struct task_work *work = task->work;
    Uint8 act = stage_get_stage_act();
    struct task *music;

    task->destructor = highway_acts_destroy;
    /* @hack redundant: a never-run store of 10 to the timer, by which SHC
     * weighs work's register and 10's: work takes R14 and 10 R11 (0x0c90bc54,
     * 0x0c90bc72); without it, 46 of 98 words differ */
    if (0) {
        work->timer = MUSIC_DELAY;
    }
    if (act == 0 && get_character() == CHARACTER_TAILS) {
        task_create(TASK_WORK, TASK_LEVEL_FIRST, race_eggman_texture_step);
    } else if (act == 1) {
        work->mode = HIGHWAY_ROOF;
        wind_set(WIND_LEVEL, NULL);
        sound_play_held(SE_WIND, -1, 8, 0);
        return;
    } else if (act == 2) {
        work->mode = HIGHWAY_ACT2;
        sound_play_held(SE_ACT2, -1, 8, 0);
        music = task_create(TASK_WORK, 0, highway_music_wait_step);
        music->work->mode = MUSIC_HIGHWAY3;
        music->work->timer = MUSIC_DELAY;
        return;
    }
    /* @hack redundant: two never-run calls of task_create, by which SHC weighs
     * its register: task_create's address takes R13 and sound_play_held's R10
     * (0x0c90bc64, 0x0c90bc62), and act R12 and 10 R11 (0x0c90bc6a,
     * 0x0c90bc72); without it, 13 of 98 words differ */
    if (0) {
        task_create(TASK_WORK, 0, highway_music_wait_step);
        task_create(TASK_WORK, 0, highway_music_wait_step);
    }
    work->mode = HIGHWAY_ACT0;
    wind_set(WIND_LEVEL, NULL);
    sound_play_held(SE_WIND, -1, 8, 0);
    music = task_create(TASK_WORK, 0, highway_music_wait_step);
    music->work->mode = MUSIC_HIGHWAY1;
    music->work->timer = MUSIC_DELAY;
}

/* once player 0 comes within ACT0_EXIT_NEAR of act 0's exit, move it on to
 * act 1's start */
static void highway_act0_exit(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 exit = {4002.0F, -1500.0F, 4750.0F};

    if (!player) {
        return;
    }
    if (njDistanceP2P(&exit, &player->pos) < ACT0_EXIT_NEAR) {
        work->mode = HIGHWAY_ROOF;
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player);
        sound_play_held(SE_WIND, -1, 8, 0);
    }
}

static void highway_act0_widen(struct task *task)
{
    struct player *player = player_get(0);
    Float amount;
    int ang;

    if (player) {
        amount = player->speed.x;
        if (amount >= WIDEN_FULL) {
            amount = 1.0F;
        } else if (amount > WIDEN_FROM) {
            amount -= WIDEN_FROM;
            amount *= 1.0F / WIDEN_SPAN;
        } else {
            amount = 0.0F;
        }
        ang = (int)(njSqrt(amount) * VIEW_WIDER) + VIEW_NARROW;
        camera_set_perspective(
            angle_approach(camera_get_perspective(), ang, VIEW_TURN));
    }
}

static void highway_roof(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    if (player && player->pos.x >= ROOF_EDGE_X) {
        work->mode = HIGHWAY_EDGE;
        pad_disable_player(0);
        work->timer = 0;
    }
}

static void highway_edge(struct task *task)
{
    struct task_work *player = player_work[0];
    /* @hack redundant: register on work, which keeps it in R14 and player in
     * R13, at 0x0c90bf2e and 0x0c90bf24; without it, the two trade, and 15 of
     * 81 words differ */
    register struct task_work *work = task->work;
    struct task *music;

    if (player) {
        if (TASK_TIMER(work)++ == EDGE_WAIT &&
            (player->flags & PLAYER_ON_GROUND)) {
            player_event_run(0, EDGE_RUN_X, EDGE_RUN_Z);
        }
        if (player->pos.y <= EDGE_MUSIC_Y && !(work->flags & EDGE_MUSIC)) {
            work->flags |= EDGE_MUSIC;
            music = task_create(TASK_WORK, 0, highway_music_wait_step);
            music->work->mode = MUSIC_HIGHWAY2;
            music->work->timer = EDGE_MUSIC_DELAY;
        }
        if (player->pos.y <= WALL_TOP_Y) {
            player_set_mode(0, PLAYER_MODE_WALL_FALL);
            pad_enable_player(0);
            work->mode = HIGHWAY_WALL;
        } else if (player->pos.y <= EDGE_RUN_END_Y) {
            player_event_end(0);
        }
    }
}

/* at WALL_FOOT_Y, player 0 stands again and steps off the wall along x,
 * and the wind stops */
static void highway_wall(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    if (player && player->pos.y <= WALL_FOOT_Y) {
        player_set_mode(0, PLAYER_MODE_RESET);
        player_set_speed(0, 1.4F, -2.0F, player_motion[0]->speed.z);
        work->mode = HIGHWAY_STREET;
        wind_stop();
    }
}

static void highway_street(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    if (player && player->pos.x >= STREET_END_X) {
        work->mode = HIGHWAY_EXIT;
    }
}

static void highway_exit(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct task *music;

    if (player && player->pos.x >= ACT1_EXIT_X) {
        work->mode = HIGHWAY_ACT2;
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player);
        sound_play_held(SE_ACT2, -1, 8, 0);
        music = task_create(TASK_WORK, 0, highway_music_wait_step);
        music->work->mode = MUSIC_HIGHWAY3;
        music->work->timer = MUSIC_DELAY;
    }
}

static void highway_end(struct task *task)
{
}

void highway_sky_step(struct task *task)
{
    task->step = highway_sky_run;
    task->draw = highway_sky_draw;
    task->destructor = highway_sky_destroy;
}

/* the sky task's step. outside Speed Highway it kills the task. when the act
 * changes, a sky in the old act's SKY_DRAW moves to its SKY_RESET. there the
 * draw releases the textures and sets mode 0. from a SKY_START, the sky
 * moves to the new act's SKY_START */
static void highway_sky_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = (Uint8)stage;

    stage &= STAGE_MASK;
    if (stage != STAGE_ACT(STAGE_HIGHWAY, 0)) {
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
    highway_sky_draw(task);
}

/* the sky task's draw, which its step also calls. each act's SKY_START loads
 * its sky's textures and sets its fog, clip and light. SKY_DRAW draws the
 * sky around the camera with fog off */
static void highway_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *camera;

    /* @hack spelling: camera assigned in the test, which holds it in R10
     * (0x0c90c23c); assigned first, it takes R11, and 16 of 426 words differ */
    if ((camera = camera_work) == NULL) {
        return;
    }
    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (work->mode) {
    case SKY_MODE(0, SKY_START):
        njSetBackColor(0, 0, 0);
        texture_load_pvm("bg_highway", &highway_bg_texlist);
        texture_load_pvm("bg_highway01", &highway_bg01_texlist);
        njSetFogColor(0);
        if (get_character() == CHARACTER_SONIC) {
            njClipZ(-3.0F, -2000.0F);
            njGenerateFogTable3(highway_fog_table, -1900.0F, -2400.0F);
        } else {
            njClipZ(-3.0F, -2500.0F);
            njGenerateFogTable3(highway_fog_table, -1850.0F, -2200.0F);
        }
        njSetFogTable(highway_fog_table);
        njFogDisable();
        njSetLightIntensity(&dir_light, 0.7F, 0.6F, 0.2F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 1.0F);
        njSetLightDirection(&dir_light, 0.5F, -0.866025F, 0.0F);
        work->mode = SKY_MODE(0, SKY_DRAW);
        /* falls through, so the sky draws from the step it starts */
    case SKY_MODE(0, SKY_DRAW):
        njFogDisable();
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera->pos);
        njSetTexture(&highway_bg_texlist);
        /* @hack spelling: the sky's model through its address, which loads the
         * address and reads @(4,R4) (0x0c90c36a); as highway_bg_object.model,
         * SHC loads the model's address from the pool, and 4 of 426 words
         * differ */
        draw_model((&highway_bg_object)->model);
        njSetTexture(&highway_bg01_texlist);
        /* @hack spelling: as above (0x0c90c378); without it, 2 of 426 words
         * differ */
        draw_model((&highway_bg01_object)->model);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_MODE(0, SKY_RESET):
        highway_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(1, SKY_START):
        njSetBackColor(0, 0, 0);
        texture_load_pvm("bg_highway", &highway_bg_texlist);
        texture_load_pvm("bg_highway02", &highway_bg02_texlist);
        njSetFogColor(0x00300020);
        njGenerateFogTable3(highway_fog_table, -1000.0F, -2000.0F);
        njSetFogTable(highway_fog_table);
        njFogEnable();
        njClipZ(-3.0F, -2000.0F);
        njSetLightIntensity(&dir_light, 1.0F, 0.8F, 0.3F);
        njSetLightColor(&dir_light, 1.0F, 0.9F, 0.9F);
        njSetLightDirection(&dir_light, 1.0F, 0.0F, 0.0F);
        work->mode = SKY_MODE(1, SKY_DRAW);
        /* falls through, so the sky draws from the step it starts */
    case SKY_MODE(1, SKY_DRAW):
        task_step_children(task);
        njFogDisable();
        if (player_work[0]->pos.y > ACT1_SKY_LOW) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &camera->pos);
            njSetTexture(&highway_bg_texlist);
            /* @hack spelling: as act 0's (0x0c90c50a); without it, 2 of 426
             * words differ */
            draw_model((&highway_bg_object)->model);
            /* bg02 draws lowered from the camera by 10000 and by a fifth
             * of the camera's y */
            njTranslate(NULL, 0.0F, -(camera->pos.y * 0.2F) - 10000.0F, 0.0F);
            njSetTexture(&highway_bg02_texlist);
            /* @hack spelling: as act 0's (0x0c90c536); without it, 11 of 426
             * words differ */
            draw_model((&highway_bg02_object)->model);
            njPopMatrix(1);
        }
        njFogEnable();
        break;
    case SKY_MODE(1, SKY_RESET):
        highway_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(2, SKY_START):
        njSetBackColor(0x001040, 0x401074, 0x807474);
        njClipZ(-3.0F, -700.0F);
        njSetFogColor(0xff7fb2e5);
        njGenerateFogTable3(highway_fog_table, -600.0F, -1000.0F);
        njSetFogTable(highway_fog_table);
        njFogEnable();
        texture_load_pvm("bg_highway03", &highway_bg03_texlist);
        njSetLightIntensity(&dir_light, 0.65F, 0.8F, 0.35F);
        njSetLightColor(&dir_light, 1.0F, 0.9F, 0.9F);
        njSetLightDirection(&dir_light, 0.866025F, -0.5F, 0.0F);
        work->mode = SKY_MODE(2, SKY_DRAW);
        /* falls through, so the sky draws from the step it starts */
    case SKY_MODE(2, SKY_DRAW):
        njClipZ(-3.0F, -1100.0F);
        njSetTexture(&highway_bg03_texlist);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera->pos);
        /* @hack spelling: as act 0's (0x0c90c67a); without it, 4 of 426 words
         * differ */
        draw_model((&highway_bg03_object)->model);
        njPopMatrix(1);
        njFogEnable();
        njClipZ(-3.0F, -700.0F);
        break;
    case SKY_MODE(2, SKY_RESET):
        highway_sky_destroy(task);
        work->mode = 0;
        break;
    default:
        /* @bug memory: a mode past 8 would hand the task itself to heap_free
         * (JSR at 0x0c90c722). tasks come from task_init's pool, not from
         * heap_alloc. Speed Highway's three acts give modes 0 to 8.
         * highway_sky_run sets a mode past them only for an act of 3 or more */
        heap_free(task);
        break;
    }
    draw_control_restore();
}

/* release the sky textures in an act's SKY_DRAW or SKY_RESET, the modes
 * that hold them. in act 2's, also turn the fog off */
static void highway_sky_destroy(struct task *task)
{
    /* @hack spelling: njReleaseTexture and highway_bg_texlist through locals,
     * which SHC loads into R14 and R13 ahead of the switch (0x0c90c764,
     * 0x0c90c768); called and passed by name, the function is 4 bytes
     * shorter and 42 of 51 words differ */
    Sint32 (*release)(NJS_TEXLIST *texlist) = njReleaseTexture;
    NJS_TEXLIST *texlist = &highway_bg_texlist;

    switch (task->work->mode) {
    case SKY_MODE(0, SKY_DRAW):
    case SKY_MODE(0, SKY_RESET):
        release(texlist);
        release(&highway_bg01_texlist);
        break;
    case SKY_MODE(1, SKY_DRAW):
    case SKY_MODE(1, SKY_RESET):
        release(texlist);
        release(&highway_bg02_texlist);
        break;
    case SKY_MODE(2, SKY_DRAW):
    case SKY_MODE(2, SKY_RESET):
        release(&highway_bg03_texlist);
        njFogDisable();
        break;
    /* @hack redundant: a case for each act's start, with nothing to release,
     * whose tests the switch makes (CMP/EQ #0,R0, #3 and #6 at 0x0c90c76a,
     * 0x0c90c776, 0x0c90c782); without them, the function is 12 bytes
     * shorter and 46 of 51 words differ */
    case SKY_MODE(0, SKY_START):
    case SKY_MODE(1, SKY_START):
    case SKY_MODE(2, SKY_START):
        break;
    }
}
