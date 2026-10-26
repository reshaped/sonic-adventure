/* the Perfect Chaos fight's stage and sky tasks */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos7/boss.h"
#include "b_chaos7/chaos7.h"
#include "b_chaos7/fight.h"
#include "b_chaos7/fight_pvms.h"
#include "b_chaos7/ray.h"
#include "b_chaos7/textures.h"
#include "camera/camera.h"
#include "chaos/start.h"
#include "collision/world.h"
#include "draw/light.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT sea_object;
extern NJS_OBJECT sea_floor_object;

/* a music task's timer starts at MUSIC_DELAY. the task asks for its music
 * once the timer falls below 0, on its 61st step */
#define MUSIC_DELAY 60

#define SE_SEA 0x409

#define FLOOR_TIME 600
#define FLOOR_GRACE 60
#define FLOOR_SINK 0.1F
#define FLOOR_DEPTH (-10.0F)
#define FLOOR_REACH 80.0F

/* the vars of the stage task and of super_wait_step's task. steps counts
 * each task's steps: the stage task's in each opening mode, and the other's
 * since it began */
struct chaos7_vars {
    Uint8 unk_00[4];
    Sint32 steps;
};

struct floor_vars {
    NJS_OBJECT *world; /* the floor's copy in the collision world */
    /* the floor's object while its first step makes the copy, then the steps
     * it has counted outside events */
    union {
        NJS_OBJECT *object;
        Sint32 steps;
    } held;
};

struct sky_vars {
    NJS_OBJECT *object;
};

#define VARS(work) ((struct chaos7_vars *)&(work)->vars)
#define FLOOR_VARS(work) ((struct floor_vars *)&(work)->vars)
#define SKY_VARS(work) ((struct sky_vars *)&(work)->vars)

enum chaos7_mode {
    CHAOS7_START,
    CHAOS7_FIGHT,
    CHAOS7_IDLE,
    CHAOS7_OPENING,
    CHAOS7_OPENING_END,
    CHAOS7_STILL,
    CHAOS7_END
};

enum floor_mode { FLOOR_INIT, FLOOR_HOLD, FLOOR_SINKING, FLOOR_GONE };

/* three modes for each act: set the act's light and fog, draw the sky, and
 * reset */
#define SKY_MODES 3
enum sky_mode { SKY_LIGHT, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))

/* the sea's first and last textures in the stage's texlist. the sky's
 * work->tick steps through them, one a step */
#define SEA_FIRST 98
#define SEA_LAST 111
/* the sea bobs SEA_BOB_HEIGHT either way of y 0. its height follows the sine
 * of the sky's work->ang.x, which gains SEA_BOB_TURN a step */
#define SEA_BOB_HEIGHT 5.0F
#define SEA_BOB_TURN 0x100
/* an act whose modes chaos7_sky_step's switch has no case for */
#define SKY_NO_ACT 4

static const char breath_2nd_pvm[] = "CHAOS7_0BREATH2ND";
static const NJS_POINT3 sea_pos = {0.0F, 0.0F, 0.0F};

/* the sky's fog table, generated again by njGenerateFogTable3 as each act's sky
 * starts */
static NJS_FOG_TABLE chaos7_fog = {
#include "b_chaos7/chaos7/chaos7_fog.inc"
};

/* chaos7_boss_start clears it, and nothing reads it */
static Uint8 dat_0ca6b4c8;

static void chaos7_fight_start(struct task *task);
static void sea_floor_step(struct task *task);
static void chaos7_boss_start(struct task *task);

/* on this task's second step, turn player 0 super and destroy the task */
static void super_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    if (++VARS(work)->steps > 1) {
        player_set_mode(0, PLAYER_MODE_SUPER);
        task_destroy(task);
    }
}

/* count work->timer down. once it falls below 0, ask for the music
 * work->mode names, and destroy the task */
static void chaos7_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_and_save_music(work->mode);
        task_destroy(task);
    }
}

/* release chaos7_breath_texlist, load chaos7_fight_pvms, and start the fight.
 * player 0 turns super, and a music task asks for the first part's music */
static void chaos7_fight_start(struct task *task)
{
    struct task_work *work = task->work;
    struct texture_pvm *pvm;
    struct task *music;

    njReleaseTexture(&chaos7_breath_texlist);
    for (pvm = chaos7_fight_pvms; pvm->texlist != NULL; ++pvm) {
        texture_load_pvm(pvm->name, pvm->texlist);
    }
    task_create_child(TASK_WORK | TASK_FORCE, super_wait_step, task);
    task_create(TASK_WORK, 0, chaos7_fight_step);
    music = task_create(TASK_WORK, 0, chaos7_music_wait_step);
    music->work->mode = MUSIC_CHAOS_P1;
    music->work->timer = MUSIC_DELAY;
    chaos7_won = 0;
    work->mode = CHAOS7_FIGHT;
}

/* start act 0: its clip, the sea floor, the sea's sound and the boss. other
 * acts start nothing here */
static void chaos7_act_start(struct task *task)
{
    Uint16 stage_act = stage_get_stage_act();

    switch (stage_act & 0xff) {
    case 0:
        njClipZ(-3.0F, -5000.0F);
        chaos7_part_2 = 0;
        task_create(TASK_WORK | TASK_FORCE, 0, sea_floor_step);
        sound_play_held(SE_SEA, -1, 8, 0);
        chaos7_boss_start(task);
    }
}

static void chaos7_boss_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *boss;

    chaos_start();
    boss = task_create_child(TASK_WORK | TASK_FORCE, chaos7_boss_step, task);
    chaos7_place_boss(boss, 0);
    dat_0ca6b4c8 = 0;
    work->mode = CHAOS7_OPENING;
}

/* step the opening's tasks, and keep player 0 at y 0 or above. from this
 * mode's second step, once no event has control, go on to
 * CHAOS7_OPENING_END */
static void chaos7_opening_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos;

    task_step_children(task);
    pos = &player_work[0]->pos;
    pos->y = pos->y > 0.0F ? pos->y : 0.0F;
    ++VARS(work)->steps;
    if (event_in_control() != TRUE && VARS(work)->steps > 1) {
        VARS(work)->steps = 0;
        work->mode = CHAOS7_OPENING_END;
    }
}

/* as chaos7_opening_step. from this mode's second step, once no event has
 * control, destroy the opening's tasks and start the fight */
static void chaos7_opening_end_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos;

    task_step_children(task);
    pos = &player_work[0]->pos;
    pos->y = pos->y > 0.0F ? pos->y : 0.0F;
    ++VARS(work)->steps;
    if (event_in_control() != TRUE && VARS(work)->steps > 1) {
        task_destroy_children(task);
        chaos7_fight_start(task);
    }
}

void chaos7_music_stop(void)
{
    stream_destroy();
}

void chaos7_restart(void)
{
    struct task *task;

    /* @hack redundant: a call to stage_get_stage_act whose result nothing
     * reads, which the image makes (JSR at 0x0c91505e). the call only reads the
     * stage and act, so it changes nothing; without it, 13 of 16 words
     * differ */
    stage_get_stage_act();
    task = stage_task;
    stream_destroy();
    task_destroy_children(task);
    chaos7_act_start(task);
}

void chaos7_part_2_start(void)
{
    struct task *music;

    stream_destroy();
    light_read_disc_sources(STAGE_CHAOS7, 1);
    light_read_disc_palettes(STAGE_CHAOS7, 1);
    njReleaseTexture(&chaos7_breath_texlist);
    texture_load_pvm((char *)breath_2nd_pvm, &chaos7_breath_texlist);
    music = task_create(TASK_WORK, 0, chaos7_music_wait_step);
    music->work->mode = MUSIC_CHAOS_P2;
    music->work->timer = MUSIC_DELAY;
}

void chaos7_part_2_end(void)
{
    stream_destroy();
    light_read_disc_sources(STAGE_CHAOS7, 0);
    light_read_disc_palettes(STAGE_CHAOS7, 0);
}

/* move the floor and its copy under player 0, at the floor's height in
 * work->scl.y. the collision world tests the copy while a player is within
 * FLOOR_REACH */
static void sea_floor_follow(struct task_work *work)
{
    NJS_OBJECT *world = FLOOR_VARS(work)->world;

    work->pos.x = player_work[0]->pos.x;
    work->pos.y = work->scl.y;
    work->pos.z = player_work[0]->pos.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    if (player_in_range(&work->pos, FLOOR_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void sea_floor_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (FLOOR_VARS(work)->world != NULL) {
        world_remove(task, FLOOR_VARS(work)->world);
        world_object_free(FLOOR_VARS(work)->world);
    }
    task_kill(task);
}

/* the floor that holds player 0 up, at its height in work->scl.y. it stays
 * where it starts until it has counted FLOOR_TIME steps outside events, and
 * then follows him. FLOOR_GRACE + 1 such steps later, it tests whether he is
 * super, and again every second such step. once a test finds he isn't, the
 * floor sinks FLOOR_SINK a step. below FLOOR_DEPTH, player 0 goes down. while
 * chaos7_won is set, the floor neither tests him nor sinks. in any act but act
 * 0, it kills its placement */
static void sea_floor_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;
    struct player *record;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CHAOS7, 0)) {
        work->mode = FLOOR_GONE;
    }
    switch (work->mode) {
    case FLOOR_INIT:
        work->mode = FLOOR_HOLD;
        work->pos = player_work[0]->pos;
        FLOOR_VARS(work)->held.object = &sea_floor_object;
        task->destructor = sea_floor_destroy;
        FLOOR_VARS(work)->world = world = world_object_alloc();
        world->evalflags = FLOOR_VARS(work)->held.object->evalflags &
                           ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
        world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
        world->ang[0] = world->ang[1] = world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        world->pos[1] = 0.0F;
        world->pos[2] = work->pos.z;
        world->model = FLOOR_VARS(work)->held.object->model;
        world->child = NULL;
        world->sibling = NULL;
        world_add(WORLD_LARGE | WORLD_MOVES, task, world);
        FLOOR_VARS(work)->held.steps = 0;
        /* falls through, so the floor counts from its first step */
    case FLOOR_HOLD:
        if (event_in_control() == TRUE) {
            break;
        }
        FLOOR_VARS(work)->held.steps++;
        if (FLOOR_VARS(work)->held.steps < FLOOR_TIME) {
            break;
        }
        if (chaos7_won == 0) {
            if (TASK_TIMER(work)++ > FLOOR_GRACE) {
                record = player_get(0);
                if (!(record->upgrades & PLAYER_SONIC_SUPER)) {
                    work->mode = FLOOR_SINKING;
                }
                work->timer = FLOOR_GRACE;
            }
        }
        sea_floor_follow(work);
        break;
    case FLOOR_SINKING:
        if (chaos7_won == 0) {
            work->scl.y -= FLOOR_SINK;
            if (work->scl.y < FLOOR_DEPTH) {
                player_down(0);
            }
        }
        sea_floor_follow(work);
        break;
    case FLOOR_GONE:
        placement_kill(task);
        break;
    }
}

/* start the act and run the opening, then run the fight. through both, keep
 * player 0 at y 0 or above. it and the sky's step lead the module, where
 * 1ST_READ.BIN's stage_steps and sky_steps reach them */
#pragma section HEAD
void chaos7_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos;

    switch (work->mode) {
    case CHAOS7_START:
        chaos7_act_start(task);
        break;
    case CHAOS7_OPENING:
        chaos7_opening_step(task);
        break;
    case CHAOS7_OPENING_END:
        chaos7_opening_end_step(task);
        break;
    case CHAOS7_IDLE:
        break;
    case CHAOS7_FIGHT:
        task_step_children(task);
        pos = &player_work[0]->pos;
        pos->y = pos->y > 0.0F ? pos->y : 0.0F;
        break;
    case CHAOS7_STILL:
        break;
    case CHAOS7_END:
    default:
        chaos7_music_stop();
        break;
    }
}

#pragma section

/* draw the sky round the camera without fog, then the sea, bobbing, in the
 * texture work->tick names */
static void chaos7_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_OBJECT *sea;

    njFogDisable();
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
    draw_tree(SKY_VARS(work)->object);
    njPopMatrix(1);
    njFogEnable();
    pos = sea_pos;
    sea = &sea_object;
    pos.y = njSin(work->ang.x) * SEA_BOB_HEIGHT;
    sea->model->mats[0].attr_texId = work->tick;
    njPushMatrix(NULL);
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_ATTR |
                       NJD_CONTROL_3D_CONSTANT_MATERIAL |
                       NJD_CONTROL_3D_OFFSET_MATERIAL);
    njTranslateV(NULL, &pos);
    draw_tree(sea);
    draw_control_restore();
    njPopMatrix(1);
}

static void chaos7_sky_destroy(struct task *task)
{
    stream_destroy();
}

#pragma section HEAD
/* when the act changes, a drawing sky moves to its reset, which starts the new
 * act's modes. off the stage, the mode advances every step. a reset there sends
 * the sky to SKY_NO_ACT's modes, which do nothing */
void chaos7_sky_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = (Uint8)stage;

    stage &= STAGE_MASK;
    if (work->mode / SKY_MODES != act) {
        if (work->mode % SKY_MODES == SKY_DRAW) {
            ++work->mode;
        } else if (work->mode % SKY_MODES == SKY_LIGHT) {
            work->mode = SKY_MODE(act, SKY_LIGHT);
        }
    }
    if (stage != STAGE_ACT(STAGE_CHAOS7, 0)) {
        ++work->mode;
        act = SKY_NO_ACT;
    }
    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (work->mode) {
    case SKY_MODE(0, SKY_LIGHT):
        work->mode = SKY_MODE(0, SKY_DRAW);
        njSetBackColor(0xff467882, 0xff467882, 0xff467882);
        njSetFogColor(0xff19cec9);
        njGenerateFogTable3(chaos7_fog, -3500.0F, -4000.0F);
        njSetFogTable(chaos7_fog);
        njFogEnable();
        njSetLightIntensity(&dir_light, 0.8F, 0.9F, 0.2F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 0.95F);
        njSetLightDirection(&dir_light, 0.5F, -0.866025F, 0.0F);
        SKY_VARS(work)->object = &chaos7_sky_object;
        task->draw = chaos7_sky_draw;
        task->destructor = chaos7_sky_destroy;
        work->tick = SEA_FIRST;
        /* falls through, so the sky is drawn from this step on */
    case SKY_MODE(0, SKY_DRAW):
        work->ang.x += SEA_BOB_TURN;
        if (++work->tick > SEA_LAST) {
            work->tick = SEA_FIRST;
        }
        task_step_children(task);
        chaos7_sky_draw(task);
        break;
    case SKY_MODE(0, SKY_RESET):
        work->mode = act * SKY_MODES;
        break;
    case SKY_MODE(1, SKY_LIGHT):
        work->mode = SKY_MODE(1, SKY_DRAW);
        njSetBackColor(0xff000000, 0xff000000, 0xff000000);
        njSetFogColor(0);
        njGenerateFogTable3(chaos7_fog, -300.0F, -5000.0F);
        njSetFogTable(chaos7_fog);
        njFogEnable();
        njSetLightIntensity(&dir_light, 0.0F, 1.0F, 0.0F);
        njSetLightColor(&dir_light, 0.1F, 0.81F, 0.79F);
        njSetLightDirection(&dir_light, -0.425386F, -0.8191F, -0.384867F);
        SKY_VARS(work)->object = &chaos7_sky_object;
        task->draw = chaos7_sky_draw;
        task->destructor = chaos7_sky_destroy;
        /* falls through, so the sky is drawn from this step on */
    case SKY_MODE(1, SKY_DRAW):
        task_step_children(task);
        work->ang.x += SEA_BOB_TURN;
        if (++work->tick > SEA_LAST) {
            work->tick = SEA_FIRST;
        }
        chaos7_sky_draw(task);
        break;
    case SKY_MODE(1, SKY_RESET):
        work->mode = act * SKY_MODES;
        break;
    default:
        break;
    }
    draw_control_restore();
}
#pragma section
