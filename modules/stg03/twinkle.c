/* Twinkle Park's stage task and sky task. act 0 is the coaster ride, and act 1
 * the park and its castle. act 2 is the castle's hall of mirrors, which only
 * Amy reaches from act 1 */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/light.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg03/cart.h"
#include "stg03/cycle.h"
#include "stg03/mirror.h"
#include "stg03/sky_textures.h"
#include "stg03/twinkle.h"
#include "system/task.h"
#include "system/util.h"

/* player 0 passes a gate while within GATE_RANGE of it. the gate back from
 * the mirrors, mirrors_return_gate, uses RETURN_RANGE instead */
#define GATE_RANGE 50.0F
#define RETURN_RANGE 36.06F
/* Sonic starting act 1 within ARRIVAL_RANGE units of act1_arrival has come off
 * the coaster */
#define ARRIVAL_RANGE 100.0F

/* the collision world's near radius during the coaster ride */
#define COASTER_NEAR 2000.0F

/* the coaster's held sound in act 0 */
#define SE_COASTER 937

/* steps the music task waits; it requests the act's music on the step
 * after */
#define MUSIC_DELAY 10

enum twinkle_mode {
    TWINKLE_INIT,
    TWINKLE_COASTER,
    TWINKLE_ACT1_FAR, /* act 1 before player 0 passes act1_gate */
    TWINKLE_ACT1_NEAR, /* and after, with the far clip brought in */
    TWINKLE_MIRRORS,
    TWINKLE_DONE
};

/* where Sonic starts act 1 when he comes off the coaster */
static const NJS_POINT3 act1_arrival = {520.0F, 975.0F, 1620.0F};
/* the gates. coaster_gate, at the coaster's end, leads to act 1, and act1_gate
 * brings act 1's far clip in. mirrors_gate leads Amy to act 2, and
 * mirrors_return_gate back */
static const NJS_POINT3 coaster_gate = {-6550.0F, -6720.0F, 23320.0F};
static const NJS_POINT3 act1_gate = {-170.0F, 0.0F, -1000.0F};
static const NJS_POINT3 mirrors_gate = {80.0F, 0.0F, -300.0F};
static const NJS_POINT3 mirrors_return_gate = {350.0F, 100.0F, 550.0F};

/* the fog tables of the coaster and the park, and of the hall of mirrors.
 * twinkle_set_fog generates each again before it sets it */
static NJS_FOG_TABLE twinkle_fog = {
#include "stg03/twinkle/twinkle_fog.inc"
};
static NJS_FOG_TABLE mirrors_fog = {
#include "stg03/twinkle/mirrors_fog.inc"
};

/* the hall's mirrors: one alone, then two pairs that face each other */
static struct twinkle_mirror hall_mirrors[5] = {
    {{-80.0F, -115.0F, -485.0F},
     {-80.0F, 285.0F, 430.0F},
     {-1.0F, 0.0F, 0.0F},
     0},
    {{280.0F, -5.0F, -74.5F}, {850.0F, 315.0F, -74.5F}, {0.0F, 0.0F, -1.0F}, 1},
    {{280.0F, -5.0F, -124.5F},
     {850.0F, 315.0F, -124.5F},
     {0.0F, 0.0F, 1.0F},
     -1},
    {{100.0F, -40.0F, -550.0F},
     {670.0F, 280.0F, -550.0F},
     {0.0F, 0.0F, -1.0F},
     1},
    {{100.0F, -40.0F, -600.0F},
     {670.0F, 280.0F, -600.0F},
     {0.0F, 0.0F, 1.0F},
     -1},
};

static void twinkle_act_start(struct task *task);
static void twinkle_set_fog(Uint8 mode);
static void coaster_step(struct task *task);
static void act1_far_step(struct task *task);
static void act1_near_step(struct task *task);
static void mirrors_step(struct task *task);

static void twinkle_destroy(struct task *task)
{
    world_reset_near_radius();
}

void twinkle_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case TWINKLE_INIT:
        twinkle_act_start(task);
        task->destructor = twinkle_destroy;
        break;
    case TWINKLE_COASTER:
        coaster_step(task);
        break;
    case TWINKLE_ACT1_FAR:
        act1_far_step(task);
        break;
    case TWINKLE_ACT1_NEAR:
        act1_near_step(task);
        break;
    case TWINKLE_MIRRORS:
        mirrors_step(task);
        break;
    case TWINKLE_DONE:
        twinkle_destroy(task);
        break;
    }
}

/* the music task's step. after work->timer steps, it forgets the pending
 * music and voice, asks for work->mode's music and ends the task */
static void twinkle_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    --work->timer;
    if (work->timer < 0) {
        stream_reset();
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* set the fog's color and table for mode, a twinkle_mode, and turn fog on */
static void twinkle_set_fog(Uint8 mode)
{
    switch (mode) {
    case TWINKLE_COASTER:
    case TWINKLE_ACT1_FAR:
        njSetFogColor(0);
        njGenerateFogTable3(twinkle_fog, -2000.0F, -3000.0F);
        njSetFogTable(twinkle_fog);
        break;
    case TWINKLE_ACT1_NEAR:
        njSetFogColor(0x100030);
        njGenerateFogTable3(twinkle_fog, -400.0F, -1000.0F);
        njSetFogTable(twinkle_fog);
        break;
    case TWINKLE_MIRRORS:
        njSetFogColor(0);
        njGenerateFogTable3(mirrors_fog, 0.0F, -600.0F);
        njSetFogTable(mirrors_fog);
    }
    njFogEnable();
}

/* start the act: its mode, fog and music task. act 0 also starts the carts and
 * the coaster's held sound, and act 1 the texture cycle. Sonic starting act 1
 * within ARRIVAL_RANGE of act1_arrival starts before act1_gate, with Tails
 * kept from following. Sonic anywhere else, Amy and Big start act 1 past
 * act1_gate, with the far clip at 1000. act 2 sets up the hall's mirrors and
 * player 0's reflection */
static void twinkle_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;
    NJS_POINT3 point;
    Uint8 act = stage_get_stage_act();

    switch (act) {
    case 0:
        work->mode = TWINKLE_COASTER;
        cart_setup();
        world_set_near_radius(COASTER_NEAR);
        music = task_create(TASK_WORK, 0, twinkle_music_wait_step);
        music->work->mode = MUSIC_TWNKLPK1;
        music->work->timer = MUSIC_DELAY;
        sound_play_held(SE_COASTER, -1, 8, 0);
        break;
    case 1:
        switch (player_get_character(0)) {
        case CHARACTER_SONIC:
            point = act1_arrival;
            if (njDistanceP2P(&point, &player_work[0]->pos) < ARRIVAL_RANGE) {
                work->mode = TWINKLE_ACT1_FAR;
                npc_disable();
            } else {
                work->mode = TWINKLE_ACT1_NEAR;
                njClipZ(-3.0F, -1000.0F);
                npc_enable();
            }
            break;
        case CHARACTER_AMY:
            work->mode = TWINKLE_ACT1_NEAR;
            njClipZ(-3.0F, -1000.0F);
            break;
        case CHARACTER_BIG:
            work->mode = TWINKLE_ACT1_NEAR;
            njClipZ(-3.0F, -1000.0F);
        }
        task_create(TASK_WORK, 1, twinkle_cycle_step);
        music = task_create(TASK_WORK, 0, twinkle_music_wait_step);
        music->work->mode = MUSIC_TWNKLPK2;
        music->work->timer = MUSIC_DELAY;
        break;
    case 2:
        work->mode = TWINKLE_MIRRORS;
        twinkle_mirrors_clear();
        twinkle_mirrors_add(5, hall_mirrors);
        twinkle_reflect_player(task);
        music = task_create(TASK_WORK, 0, twinkle_music_wait_step);
        music->work->mode = MUSIC_TWNKLPK3;
        music->work->timer = MUSIC_DELAY;
        break;
    }
    twinkle_set_fog(work->mode);
}

/* once player 0 is within GATE_RANGE of coaster_gate, change to act 1. play
 * starts before act1_gate, with Tails kept from following */
static void coaster_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;
    NJS_POINT3 point;

    point = coaster_gate;
    if (njDistanceP2P(&point, &player_work[0]->pos) < GATE_RANGE) {
        work->mode = TWINKLE_ACT1_FAR;
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        stage_place_player(player_work[0]);
        world_reset_near_radius();
        npc_disable();
        task_create(TASK_WORK, 1, twinkle_cycle_step);
        music = task_create(TASK_WORK, 0, twinkle_music_wait_step);
        music->work->mode = MUSIC_TWNKLPK2;
        music->work->timer = MUSIC_DELAY;
        twinkle_set_fog(work->mode);
    }
}

/* set world_land_blocks to act 1's four land blocks, less those the camera's
 * position rules out. most of the rules depend on work->mode */
static void act1_land_blocks(struct task_work *work)
{
    NJS_POINT3 camera;

    camera = camera_work->pos;
    world_land_blocks = 0xf;
    switch (work->mode) {
    case TWINKLE_ACT1_FAR:
        if (camera.z > 1000.0F) {
            world_land_blocks &= ~0x1;
        }
        if (camera.z < -200.0F) {
            world_land_blocks &= ~(0x2 | 0x4);
        }
        break;
    case TWINKLE_ACT1_NEAR:
        if (camera.z < -500.0F) {
            world_land_blocks &= ~0x2;
        }
        if (camera.y < 200.0F) {
            world_land_blocks &= ~0x4;
        }
        break;
    }
    if (camera.z > 250.0F) {
        world_land_blocks &= ~0x8;
    }
}

/* set act 1's land blocks. once player 0 is within GATE_RANGE of act1_gate,
 * change to TWINKLE_ACT1_NEAR. that lets Tails follow, sets its fog and brings
 * the far clip in to 1000 */
static void act1_far_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 point;

    point = act1_gate;
    act1_land_blocks(work);
    if (njDistanceP2P(&point, &player_work[0]->pos) < GATE_RANGE) {
        work->mode = TWINKLE_ACT1_NEAR;
        npc_enable();
        twinkle_set_fog(work->mode);
        njClipZ(-3.0F, -1000.0F);
    }
}

/* set act 1's land blocks. once player 0 is Amy and within GATE_RANGE of
 * mirrors_gate, change to act 2. that sets up the hall's mirrors and player
 * 0's reflection */
static void act1_near_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;
    NJS_POINT3 point;

    point = mirrors_gate;
    act1_land_blocks(work);
    if (player_get_character(0) == CHARACTER_AMY &&
        njDistanceP2P(&point, &player_work[0]->pos) < GATE_RANGE) {
        work->mode = TWINKLE_MIRRORS;
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        twinkle_set_fog(work->mode);
        twinkle_mirrors_clear();
        twinkle_mirrors_add(5, hall_mirrors);
        twinkle_reflect_player(task);
        music = task_create(TASK_WORK, 0, twinkle_music_wait_step);
        music->work->mode = MUSIC_TWNKLPK3;
        music->work->timer = MUSIC_DELAY;
    }
}

/* once player 0 is within RETURN_RANGE of mirrors_return_gate, go back to act
 * 1 in TWINKLE_ACT1_NEAR. the mirrors are cleared, and the far clip set to
 * 600 */
static void mirrors_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;
    NJS_POINT3 point;

    point = mirrors_return_gate;
    if (njDistanceP2P(&point, &player_work[0]->pos) < RETURN_RANGE) {
        work->mode = TWINKLE_ACT1_NEAR;
        stream_destroy();
        world_change_act(-1);
        placement_change_act(-1);
        camera_change_act(-1);
        stage_change_act(-1);
        twinkle_mirrors_clear();
        twinkle_set_fog(work->mode);
        njClipZ(-3.0F, -600.0F);
        music = task_create(TASK_WORK, 0, twinkle_music_wait_step);
        music->work->mode = MUSIC_TWNKLPK2;
        music->work->timer = MUSIC_DELAY;
    }
}

/* the sky task's work mode is SKY_MODE(act, mode): an act, and one of the
 * act's SKY_MODES sky_modes. SKY_LIGHT sets up the act's light, then moves on
 * to SKY_DRAW. SKY_RESET sets the work mode to 0 */
#define SKY_MODES 3
enum sky_mode { SKY_LIGHT, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))

/* at SKY_DRAW, step the task's children. acts 0 and 1 then draw the sky round
 * the camera, unfogged. act 2, the mirrors, has no sky. a mode past act
 * 2's kills the task. it does nothing while camera_work is null */
static void twinkle_sky_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (camera_work == NULL) {
        return;
    }
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (work->mode) {
    case SKY_MODE(0, SKY_LIGHT):
        njSetBackColor(0, 0, 0);
        njSetLightIntensity(&dir_light, 0.8F, 0.9F, 0.2F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 0.95F);
        njSetLightDirection(&dir_light, 0.5F, -0.866025F, 0.0F);
        work->mode = SKY_MODE(0, SKY_DRAW);
        /* falls through into SKY_DRAW, which draws the sky on this step too */
    case SKY_MODE(0, SKY_DRAW):
        task_step_children(task);
        njFogDisable();
        njSetTexture(&bg_shareobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        draw_tree(&twinkle_sky_object);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_MODE(0, SKY_RESET):
        work->mode = 0;
        break;
    case SKY_MODE(1, SKY_LIGHT):
        njSetBackColor(0, 0, 0);
        njSetLightIntensity(&dir_light, 0.8F, 0.9F, 0.2F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 0.95F);
        njSetLightDirection(&dir_light, 0.5F, -0.866025F, 0.0F);
        work->mode = SKY_MODE(1, SKY_DRAW);
        /* falls through into SKY_DRAW, which draws the sky on this step too */
    case SKY_MODE(1, SKY_DRAW):
        task_step_children(task);
        njFogDisable();
        njSetTexture(&bg_shareobj_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        draw_tree(&twinkle_sky_object);
        njPopMatrix(1);
        njFogEnable();
        break;
    case SKY_MODE(1, SKY_RESET):
        work->mode = 0;
        break;
    case SKY_MODE(2, SKY_LIGHT):
        njSetBackColor(0, 0, 0);
        njSetLightIntensity(&dir_light, 0.8F, 0.9F, 0.2F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 0.95F);
        njSetLightDirection(&dir_light, 0.5F, -0.866025F, 0.0F);
        work->mode = SKY_MODE(2, SKY_DRAW);
        /* falls through into SKY_DRAW, which steps the children on this step
         * too */
    case SKY_MODE(2, SKY_DRAW):
        task_step_children(task);
        break;
    case SKY_MODE(2, SKY_RESET):
        work->mode = 0;
        break;
    default:
        task_kill(task);
        break;
    }
    draw_control_restore();
}

static void twinkle_sky_destroy(struct task *task)
{
}

/* the sky task ends once the stage isn't Twinkle Park. a work mode of another
 * act moves on toward the current act's. its SKY_LIGHT becomes the current
 * act's SKY_LIGHT. its SKY_DRAW becomes its SKY_RESET. the draw then sets mode
 * 0, act 0's SKY_LIGHT, which the next step makes the current act's */
static void twinkle_sky_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = (Uint8)stage;

    stage &= STAGE_MASK;
    if (stage != STAGE_ACT(STAGE_TWINKLE, 0)) {
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
    twinkle_sky_draw(task);
}

void twinkle_sky_step(struct task *task)
{
    task->step = twinkle_sky_run;
    task->draw = twinkle_sky_draw;
    task->destructor = twinkle_sky_destroy;
}
