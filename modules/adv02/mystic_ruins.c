/* the Mystic Ruins' field and sky tasks. the field task creates each act's
 * tasks and its music, and the sky task sets each act's light and fog and draws
 * its sky */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/crowd.h"
#include "adv02/fog.h"
#include "adv02/handy_key.h"
#include "adv02/island_land.h"
#include "adv02/kill_switch.h"
#include "adv02/mystic_ruins.h"
#include "adv02/pyramid.h"
#include "adv02/runway.h"
#include "adv02/sand_entry.h"
#include "adv02/station_door.h"
#include "adv02/station_ladder.h"
#include "adv02/station_panel.h"
#include "adv02/texlists.h"
#include "adv02/west_rock.h"
#include "adv02/workshop.h"
#include "adv02/workshop_door.h"
#include "camera/camera.h"
#include "draw/light.h"
#include "draw/tree.h"
#include "event/play.h"
#include "main.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT sky_station_day_object;
extern NJS_OBJECT sky_station_evening_object;
extern NJS_OBJECT sky_station_night_object;
extern NJS_OBJECT sky_island_day_object;
extern NJS_OBJECT sky_island_evening_object;
extern NJS_OBJECT sky_island_night_object;
extern NJS_OBJECT sky_jungle_day_object;
extern NJS_OBJECT sky_jungle_evening_object;
extern NJS_OBJECT sky_jungle_night_object;
extern NJS_OBJECT station_sea_object;
/* the jungle's models whose UVs scroll, and the station's and the island's
 * objects whose textures cycle */
extern NJS_MODEL dat_0ca625ac;
extern NJS_MODEL dat_0ca629f8;
extern NJS_MODEL dat_0ca62c3c;
extern NJS_MODEL dat_0ca63240;
extern NJS_MODEL dat_0ca61fdc;
extern NJS_OBJECT dat_0c975cf4;
extern NJS_OBJECT dat_0c95a098;
extern NJS_OBJECT dat_0c95a340;
extern NJS_OBJECT dat_0c953e40;
extern NJS_OBJECT dat_0c954214;
extern NJS_OBJECT dat_0c9a8010;
extern NJS_OBJECT dat_0c9a4558;

/* the act light's turns about y and z. light_read_disc_sources reads them from
 * the light file, with the light sources. in the station, island and jungle
 * acts, mystic_ruins_load_light then sets them from light_turns for the time
 * of day */
extern Angle light_ry;
extern Angle light_rz;

/* the act each task started last. the field task keeps it in work->timer, as a
 * Uint16, and the sky task in work->tick */
#define FIELD_ACT(work) TASK_TIMER(work)
#define SKY_ACT(work) ((work)->tick)

/* slots for the act's tasks, which mystic_ruins_act_end destroys */
#define MYSTIC_RUINS_ACT_TASKS 10

enum mystic_ruins_mode {
    MYSTIC_RUINS_INIT,
    MYSTIC_RUINS_START,
    MYSTIC_RUINS_RUN,
    MYSTIC_RUINS_END
};

/* the music task's first count. the task takes 1 from it each step, and asks
 * for the act's music once it is below 0 */
#define MUSIC_DELAY 10

/* the steps key_give_step counts down before it creates the Ice Stone */
#define KEY_DELAY 5

/* the fog density main.c sets as the game starts, about 2^19: m 0xff and e
 * 0x12, as njGenerateFogDensity packs them. each njGenerateFogTable3 after it
 * sets the density again, from its far depth, before anything is drawn */
#define FOG_DENSITY 0xff12

/* a model whose UVs scroll_step moves: the first count of its first meshset's
 * UVs, along v, or along -v when backward */
struct uv_scroll {
    NJS_MODEL *model;
    int count;
    Bool backward;
};

#define SCROLLS 5

enum { SHOWN, PLACE }; /* cycle_vars.counts's indexes */

/* a texture cycle. each object's first material shows the textures from
 * FIRST_TEXTURE on, each for delay steps, and then starts over */
struct cycle_vars {
    NJS_OBJECT **objects;
    Sint8 delay;
    Sint8 textures; /* the textures in the cycle */
    Uint8 unk_06[2];
    /* counts[SHOWN] is the steps the current texture has shown, and
     * counts[PLACE] its place in the cycle, from 0 */
    Sint8 counts[2];
};

/* the cycle's first texture id, in work->timer */
#define FIRST_TEXTURE(work) TASK_TIMER(work)
#define CYCLE(work) ((struct cycle_vars *)&(work)->vars)

/* the scroll's offset along v, from 0 to 255, in work->timer */
#define SCROLL(work) TASK_TIMER(work)

enum scroll_mode { SCROLL_INIT, SCROLL_RUN };

static void mystic_ruins_act_start(struct task *task);
static void mystic_ruins_act_end(struct task *task);
static void mystic_ruins_destroy(struct task *task);
static void key_give_step(struct task *task);
static void held_restore(void);
static void mystic_ruins_sky_destroy(struct task *task);
static void mystic_ruins_sky_act_start(struct task *task);
static void mystic_ruins_sky_act_step(struct task *task);
static void mystic_ruins_sky_act_end(struct task *task);
static void mystic_ruins_sky_draw(struct task *task);
static void mystic_ruins_load_light(struct task *task);
static void mystic_ruins_music_step(struct task *task);
static void scroll_destroy(struct task *task);
static void scroll_step(struct task *task);
static void station_cycle_init(struct task *task);
static void station_cycle_2_init(struct task *task);
static void island_cycle_init(struct task *task);
static void cycle_step(struct task *task);
static void fog_set_color(void);

/* sky models by act, then by time of day: day, evening and night. the base act
 * has no sky */
static NJS_OBJECT *mystic_ruins_sky_models[3][3] = {
    {&sky_station_day_object, &sky_station_evening_object,
     &sky_station_night_object},
    {&sky_island_day_object, &sky_island_evening_object,
     &sky_island_night_object},
    {&sky_jungle_day_object, &sky_jungle_evening_object,
     &sky_jungle_night_object},
};
/* sky texture lists by act. no code reads them: mystic_ruins_sky_draw names
 * each act's list itself */
static NJS_TEXLIST *mystic_ruins_sky_texlists[3] = {
    &mr_sky00_texlist,
    &mr_sky01_texlist,
    &mr_sky02_texlist,
};
/* three zero words no code reads */
static int dat_0ca9aad8[3] = {0, 0, 0};

/* the light's turns about y and z, by act, then by time of day */
static Angle light_turns[3][3][2] = {
#include "adv02/mystic_ruins/light_turns.inc"
};

/* the jungle water models whose UVs scroll */
static struct uv_scroll uv_scrolls[SCROLLS] = {
#include "adv02/mystic_ruins/uv_scrolls.inc"
};

/* each texture cycle's objects, ending at a null */
static NJS_OBJECT *station_cycle_objects[3] = {&dat_0c975cf4,
                                               &station_sea_object, NULL};
static NJS_OBJECT *station_cycle_2_objects[5] = {
    &dat_0c95a098, &dat_0c95a340, &dat_0c953e40, &dat_0c954214, NULL};
static NJS_OBJECT *island_cycle_objects[3] = {&dat_0c9a8010, &dat_0c9a4558,
                                              NULL};

static struct task *mystic_ruins_act_tasks[MYSTIC_RUINS_ACT_TASKS];

/* #pragma section HEAD puts the two task steps in PHEAD, which opens the
 * module, where 1ST_READ.BIN's stage_steps and sky_steps name them */
#pragma section HEAD
void mystic_ruins_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case MYSTIC_RUINS_INIT:
        mystic_ruins_messages_load();
        FIELD_ACT(work) = (Uint8)stage_get_stage_act();
        task->destructor = mystic_ruins_destroy;
        held_restore();
        camera_set_start_mode(CAMERA_MODE_KNUCKLES);
        work->mode = MYSTIC_RUINS_START;
        break;
    case MYSTIC_RUINS_START:
        mystic_ruins_act_start(task);
        work->mode = MYSTIC_RUINS_RUN;
        break;
    case MYSTIC_RUINS_RUN:
        if ((Uint8)stage_get_stage_act() == FIELD_ACT(work)) {
            return;
        }
        work->mode = MYSTIC_RUINS_END;
        break;
    case MYSTIC_RUINS_END:
        mystic_ruins_act_end(task);
        work->mode = MYSTIC_RUINS_START;
        break;
    }
}

#pragma section

/* create the act's tasks in mystic_ruins_act_tasks, and the music task, which
 * takes the act's music in work->mode. player 0 starts each act still */
static void mystic_ruins_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;
    int i;

    for (i = 0; i < MYSTIC_RUINS_ACT_TASKS; i++) {
        mystic_ruins_act_tasks[i] = NULL;
    }
    FIELD_ACT(work) = (Uint8)stage_get_stage_act();
    player_set_mode(0, PLAYER_MODE_RESET);
    player_set_speed(0, 0.0F, 0.0F, 0.0F);
    switch (FIELD_ACT(work)) {
    case MYSTIC_RUINS_STATION:
        mystic_ruins_act_tasks[0] = task_create(TASK_WORK, 3, runway_step);
        mystic_ruins_act_tasks[1] =
            task_create(TASK_WORK, 3, station_door_step);
        mystic_ruins_act_tasks[2] =
            task_create(TASK_WORK | TASK_FORCE, 3, west_rock_step);
        mystic_ruins_act_tasks[3] = task_create(TASK_WORK, 3, workshop_step);
        mystic_ruins_act_tasks[4] =
            task_create(TASK_WORK, 3, station_panel_step);
        mystic_ruins_act_tasks[5] =
            task_create(TASK_WORK, 3, workshop_door_step);
        mystic_ruins_act_tasks[6] =
            task_create(TASK_WORK, 3, station_ladder_step);
        mystic_ruins_act_tasks[7] =
            task_create(TASK_WORK, 3, station_cycle_init);
        mystic_ruins_act_tasks[8] =
            task_create(TASK_WORK, 3, station_cycle_2_init);
        music = task_create(TASK_WORK, 0, mystic_ruins_music_step);
        music->work->mode = MUSIC_MSTCLN;
        music->work->timer = MUSIC_DELAY;
        break;
    case MYSTIC_RUINS_ISLAND:
        mystic_ruins_act_tasks[0] =
            task_create(TASK_WORK | TASK_FORCE, 3, island_land_step);
        mystic_ruins_act_tasks[1] =
            task_create(TASK_WORK, 3, island_cycle_init);
        music = task_create(TASK_WORK, 0, mystic_ruins_music_step);
        music->work->mode = MUSIC_MSTCLN;
        music->work->timer = MUSIC_DELAY;
        break;
    case MYSTIC_RUINS_JUNGLE:
        mystic_ruins_act_tasks[0] =
            task_create(TASK_WORK | TASK_FORCE, 3, pyramid_step);
        mystic_ruins_act_tasks[1] = task_create(TASK_WORK, 3, sand_entry_step);
        mystic_ruins_act_tasks[2] = task_create(TASK_WORK, 3, scroll_step);
        music = task_create(TASK_WORK, 0, mystic_ruins_music_step);
        music->work->mode = MUSIC_MSTCLN;
        music->work->timer = MUSIC_DELAY;
        break;
    case MYSTIC_RUINS_BASE:
        music = task_create(TASK_WORK, 0, mystic_ruins_music_step);
        music->work->mode = MUSIC_FINALEG1;
        music->work->timer = MUSIC_DELAY;
        break;
    }
}

static void mystic_ruins_act_end(struct task *task)
{
    int i;

    for (i = 0; i < MYSTIC_RUINS_ACT_TASKS; i++) {
        if (mystic_ruins_act_tasks[i] != NULL) {
            task_destroy(mystic_ruins_act_tasks[i]);
            mystic_ruins_act_tasks[i] = NULL;
        }
    }
}

static void mystic_ruins_destroy(struct task *task)
{
    mystic_ruins_act_end(task);
}

/* count work->timer down to 0, a step at a time, then put a new Ice Stone in
 * player 0's hands and end the task */
static void key_give_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *key;

    if (work->timer == 0) {
        key = task_create(TASK_MOTION | TASK_WORK, 3, handy_key_step);
        key->work->scl.x = KEY_ICE_STONE;
        player_hand_task(0, key);
        task_kill(task);
    } else {
        work->timer--;
    }
}

/* as the field starts, give back what player 0's held kind says it carries. an
 * Ice Stone comes back in its hands by key_give_step, KEY_DELAY steps later.
 * the Wind Stone's, the statues' and the kill switch's held kinds stay, and any
 * other is cleared */
static void held_restore(void)
{
    struct task *task;
    int held = player_get_held_kind(0);

    switch (held) {
    case HELD_ICE_STONE:
        task = task_create(TASK_WORK, 3, key_give_step);
        task->work->timer = KEY_DELAY;
        break;
    case HELD_WINDY_STONE:
    case HELD_GOLD_STATUE:
    case HELD_SILVER_STATUE:
    case HELD_KILL_SWITCH:
        break;
    default:
        player_set_held_kind(0, 0);
        break;
    }
}

#pragma section HEAD
void mystic_ruins_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case MYSTIC_RUINS_INIT:
        task->destructor = mystic_ruins_sky_destroy;
        task->draw = mystic_ruins_sky_draw;
        work->mode = MYSTIC_RUINS_START;
        break;
    case MYSTIC_RUINS_START:
        mystic_ruins_sky_act_start(task);
        work->mode = MYSTIC_RUINS_RUN;
        break;
    case MYSTIC_RUINS_RUN:
        if ((Uint8)stage_get_stage_act() != SKY_ACT(work)) {
            work->mode = MYSTIC_RUINS_END;
            return;
        }
        mystic_ruins_sky_act_step(task);
        mystic_ruins_sky_draw(task);
        break;
    case MYSTIC_RUINS_END:
        mystic_ruins_sky_act_end(task);
        work->mode = MYSTIC_RUINS_START;
        break;
    }
}
#pragma section

static void mystic_ruins_sky_destroy(struct task *task)
{
    mystic_ruins_sky_act_end(task);
}

/* set the act's light, clip planes and fog for the time of day. the far clip
 * is 2000 units out, or 1000 in the base. the jungle's fog is
 * jungle_fog_init's. elsewhere, fog grows from none at 500 units out, or 5000
 * in the base, to whole at 8000. the base keeps the fog color it had */
static void mystic_ruins_sky_act_start(struct task *task)
{
    struct task_work *work = task->work;

    SKY_ACT(work) = stage_get_stage_act();
    story_get_time();
    mystic_ruins_load_light(task);
    switch (SKY_ACT(work)) {
    case MYSTIC_RUINS_STATION:
        njClipZ(-3.0F, -2000.0F);
        fog_set_color();
        njSetFogDensity(FOG_DENSITY);
        njGenerateFogTable3(fog_table, -500.0F, -8000.0F);
        njSetFogTable(fog_table);
        break;
    case MYSTIC_RUINS_ISLAND:
        njClipZ(-3.0F, -2000.0F);
        fog_set_color();
        njSetFogDensity(FOG_DENSITY);
        njGenerateFogTable3(fog_table, -500.0F, -8000.0F);
        njSetFogTable(fog_table);
        break;
    case MYSTIC_RUINS_JUNGLE:
        jungle_fog_init();
        njClipZ(-3.0F, -2000.0F);
        break;
    case MYSTIC_RUINS_BASE:
        njClipZ(-3.0F, -1000.0F);
        njSetFogDensity(FOG_DENSITY);
        njGenerateFogTable3(fog_table, -5000.0F, -8000.0F);
        njSetFogTable(fog_table);
        break;
    }
    work->vars.b[0] = 0;
}

/* in the jungle, update the fog for the time of day and the camera */
static void mystic_ruins_sky_act_step(struct task *task)
{
    struct task_work *work = task->work;

    if (SKY_ACT(work) == MYSTIC_RUINS_JUNGLE) {
        jungle_fog_update(story_get_time());
    }
    /* @hack redundant: a switch on the act whose cases do nothing, which SHC
     * keeps as four CMP/EQ, from 0x0c902c8a to 0x0c902c96; without it, the call
     * to jungle_fog_update is a tail call, and all 25 of
     * mystic_ruins_sky_act_step's 25 words differ */
    switch (SKY_ACT(work)) {
    case MYSTIC_RUINS_STATION:
        break;
    case MYSTIC_RUINS_ISLAND:
        break;
    case MYSTIC_RUINS_JUNGLE:
        break;
    case MYSTIC_RUINS_BASE:
        break;
    }
}

static void mystic_ruins_sky_act_end(struct task *task)
{
    struct task_work *work = task->work;

    /* @hack redundant: a switch on the act whose cases do nothing, which SHC
     * keeps as four CMP/EQ, from 0x0c902ce6 to 0x0c902cf2; without it, SHC
     * gives mystic_ruins_sky_step other registers, and 12 of
     * mystic_ruins_sky_act_end's 12 words and 39 of mystic_ruins_sky_step's 56
     * differ */
    switch (SKY_ACT(work)) {
    case MYSTIC_RUINS_STATION:
        break;
    case MYSTIC_RUINS_ISLAND:
        break;
    case MYSTIC_RUINS_JUNGLE:
        break;
    case MYSTIC_RUINS_BASE:
        break;
    }
}

/* draw the act's sky for the time of day, with no fog and no clip check,
 * centered on the camera in x and z. the station also draws its sea, at the
 * sea's own place. the jungle's sky is drawn twice its size, and the base has
 * none */
static void mystic_ruins_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    int time = story_get_time();

    switch (SKY_ACT(work)) {
    case MYSTIC_RUINS_STATION:
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njSetTexture(&mr_sky00_texlist);
        draw_tree(mystic_ruins_sky_models[SKY_ACT(work)][time]);
        njPopMatrix(1);
        njSetTexture(&adv_mr00_texlist);
        draw_tree(&station_sea_object);
        njFogEnable();
        draw_control_restore();
        break;
    case MYSTIC_RUINS_ISLAND:
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njSetTexture(&mr_sky01_texlist);
        draw_tree(mystic_ruins_sky_models[SKY_ACT(work)][time]);
        njPopMatrix(1);
        njFogEnable();
        draw_control_restore();
        break;
    case MYSTIC_RUINS_JUNGLE:
        draw_control_save();
        draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
        njScale(NULL, 2.0F, 2.0F, 2.0F);
        njSetTexture(&mr_sky02_texlist);
        draw_tree(mystic_ruins_sky_models[SKY_ACT(work)][time]);
        njPopMatrix(1);
        njFogEnable();
        draw_control_restore();
        break;
    }
}

/* read the light sources and palettes, and set the light's turns from
 * light_turns. light_read_disc_sources and light_read_disc_palettes number
 * the files by act. the base reads its own, and keeps the turns its sources
 * file gives. the other acts share a file for each time of day, numbered as
 * story_get_time gives it */
static void mystic_ruins_load_light(struct task *task)
{
    struct task_work *work = task->work;
    int act = SKY_ACT(work);
    int time = story_get_time();

    if (act != MYSTIC_RUINS_BASE) {
        switch (time) {
        case TIME_DAY:
            light_read_disc_sources(STAGE_MYSTIC_RUINS, TIME_DAY);
            light_read_disc_palettes(STAGE_MYSTIC_RUINS, TIME_DAY);
            break;
        case TIME_EVENING:
            light_read_disc_sources(STAGE_MYSTIC_RUINS, TIME_EVENING);
            light_read_disc_palettes(STAGE_MYSTIC_RUINS, TIME_EVENING);
            break;
        case TIME_NIGHT:
            light_read_disc_sources(STAGE_MYSTIC_RUINS, TIME_NIGHT);
            light_read_disc_palettes(STAGE_MYSTIC_RUINS, TIME_NIGHT);
            break;
        }
        light_ry = light_turns[act][time][0];
        light_rz = light_turns[act][time][1];
    } else {
        light_read_disc_sources(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_BASE);
        light_read_disc_palettes(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_BASE);
    }
}

/* take 1 from work->timer each step. once it is below 0, reset the streams
 * and, while no event plays, start and keep the music work->mode names. then
 * destroy the task */
static void mystic_ruins_music_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        stream_reset();
        if (event_check_music()) {
            stream_reset();
            stream_request_and_save_music(work->mode);
        }
        task_destroy(task);
    }
}

/* take the scroll's offset back off every model's UVs, as the task ends */
static void scroll_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_scroll *scroll = uv_scrolls;
    int back = -SCROLL(work);
    NJS_COLOR *uv;
    int i;
    int j;

    for (i = 0; i < SCROLLS; i++, scroll++) {
        uv = scroll->model->meshsets->vertuv;
        for (j = 0; j < scroll->count; j++, uv++) {
            /* @bug logic: a backward model's UVs moved along -v, and this
             * moves them along -v again, so the UVs of dat_0ca61fdc, the one
             * backward model, end twice the offset from where they started.
             * ADD R13,R0 at 0x0c903046 adds the negated offset, from NEG
             * R0,R13 at 0x0c903032, to every UV, with no test of backward */
            uv->tex.v += back;
        }
    }
}

/* move each model's UVs one unit along v each step, or along -v when backward.
 * on the step the offset reaches 256, it goes back to 0, and the UVs move 256
 * back, a whole texture */
static void scroll_step(struct task *task)
{
    struct task_work *work = task->work;
    struct uv_scroll *scroll;
    NJS_COLOR *uv;
    int step;
    int i;
    int j;

    if (work->mode == SCROLL_INIT) {
        work->mode = SCROLL_RUN;
        task->destructor = scroll_destroy;
    }
    SCROLL(work)++;
    if (SCROLL(work) == 256) {
        SCROLL(work) = 0;
        step = -256;
    } else {
        step = 1;
    }
    scroll = uv_scrolls;
    for (i = 0; i < SCROLLS; i++, scroll++) {
        uv = scroll->model->meshsets->vertuv;
        for (j = 0; j < scroll->count; j++, uv++) {
            if (scroll->backward) {
                uv->tex.v -= step;
            } else {
                uv->tex.v += step;
            }
        }
    }
}

static void station_cycle_init(struct task *task)
{
    struct task_work *work = task->work;

    CYCLE(work)->objects = station_cycle_objects;
    CYCLE(work)->delay = 5;
    CYCLE(work)->textures = 10;
    FIRST_TEXTURE(work) = 130;
    task->step = cycle_step;
}

static void station_cycle_2_init(struct task *task)
{
    struct task_work *work = task->work;

    CYCLE(work)->objects = station_cycle_2_objects;
    CYCLE(work)->delay = 5;
    CYCLE(work)->textures = 15;
    FIRST_TEXTURE(work) = 140;
    task->step = cycle_step;
}

static void island_cycle_init(struct task *task)
{
    struct task_work *work = task->work;

    CYCLE(work)->objects = island_cycle_objects;
    CYCLE(work)->delay = 2;
    CYCLE(work)->textures = 14;
    FIRST_TEXTURE(work) = 76;
    task->step = cycle_step;
}

static void cycle_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT **objects;
    NJS_OBJECT *object;

    CYCLE(work)->counts[SHOWN]++;
    if (CYCLE(work)->counts[SHOWN] == CYCLE(work)->delay) {
        CYCLE(work)->counts[SHOWN] = 0;
        CYCLE(work)->counts[PLACE]++;
        if (CYCLE(work)->counts[PLACE] == CYCLE(work)->textures) {
            CYCLE(work)->counts[PLACE] = 0;
        }
    }
    objects = CYCLE(work)->objects;
    while ((object = *objects++) != NULL) {
        object->model->mats[0].attr_texId =
            FIRST_TEXTURE(work) + CYCLE(work)->counts[PLACE];
    }
}

static void fog_set_color(void)
{
    switch (story_get_time()) {
    case TIME_DAY:
        njSetFogColor(0xffa0a0a0);
        break;
    case TIME_EVENING:
        njSetFogColor(0xff907858);
        break;
    case TIME_NIGHT:
        njSetFogColor(0xff001058);
        break;
    }
}
