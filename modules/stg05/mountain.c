/* Red Mountain's stage task and sky task, the clouds below, act 2's updraft
 * and Knuckles' tutorial */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "camera/camera.h"
#include "collision/ground.h"
#include "collision/mesh.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/light.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/lens.h"
#include "event/play.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "progress/recorder.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg05/lava.h"
#include "stg05/mountain.h"
#include "stg05/rock_burst.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#include "stg05/mountain/text.inc"

extern NJS_TEXLIST obj_mountain_texlist;
extern NJS_OBJECT knuckles_wall_object[1];
extern NJS_OBJECT mountain_sky_0_object;
extern NJS_OBJECT mountain_sky_1_object;
extern NJS_OBJECT mountain_sky_2_object;
extern NJS_OBJECT mountain_sky_3_object;
extern NJS_OBJECT cloud_object;
extern NJS_COLOR cloud_uvs[];
extern NJS_OBJECT dat_0ca754a8;
extern NJS_TEXLIST dat_0caaeea0;
extern NJS_ACTION dat_0caaed9c;
/* @hack spelling: stage_clears' entries for Sonic and Tails as bytes of their
 * own, which fun_0c913a20 loads through R3 and R2 (0x0c913a34, 0x0c913a36);
 * read from the array, the two trade and 5 of 144 words differ */
extern Sint8 sonic_mountain_clear;
extern Sint8 tails_mountain_clear;

static const struct angles mountain_no_slope = {0, 0, 0};

static NJS_TEXNAME knuckles_tutorial_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
static NJS_TEXLIST knuckles_tutorial_texlist = {
#include "stg05/mountain/knuckles_tutorial_texlist.inc"
};
/* three light specs, which no code reads */
static struct light_spec dat_0cadb0f4[3] = {
#include "stg05/mountain/dat_0cadb0f4.inc"
};
struct task_work *mountain_work = NULL;
static NJS_TEXANIM knuckles_tutorial_frames[3] = {
#include "stg05/mountain/knuckles_tutorial_frames.inc"
};
/* the yaw the d-pad gives fun_0c913a20's carrier, and the carrier's action
 * frame */
static Sint32 carrier_ang = 0;
static Float carrier_frame = 0.0F;
/* the updraft's strength, from 0 to UPDRAFT_FULL */
static Sint32 updraft_level = 0;

/* TRUE from the tutorial's start until its prompts have slid out */
static Sint32 tutorial_on;
/* what cloud_step adds to the v of each of cloud_uvs, for the near layer and
 * the far one. they fall by 1 and 2 a step, and gain CLOUD_WRAP once below
 * 0 */
static Sint32 cloud_scroll_near;
static Sint32 cloud_scroll_far;
static NJS_COLOR cloud_uvs_near[32];
static NJS_COLOR cloud_uvs_far[32];
static NJS_FOG_TABLE mountain_fog;

enum mountain_mode {
    MOUNTAIN_START = 0,
    MOUNTAIN_RUN = 1,
    MOUNTAIN_RESUME = 5,
    MOUNTAIN_END = 6
};

/* the wind that plays from the start of acts 0 and 2 until stopped. act 2's
 * updraft plays it too */
#define SE_WIND 946
/* the wind that plays from the start of act 1 until stopped */
#define SE_ACT1_WIND 948
/* act 2's updraft's second wind. it plays while player 0's pos.y - 1 is above
 * UPDRAFT_SOUND_ABOVE. it also plays whenever player 0 is out of the updraft */
#define SE_UPDRAFT 950
#define UPDRAFT_SOUND_ABOVE 75.0F

/* steps before the act's track starts */
#define MUSIC_DELAY 3

/* in act 0, Sonic within 20 of this point goes on to act 1 */
#define ACT0_EXIT_X (-3667.0F)
#define ACT0_EXIT_Y 0.0F
#define ACT0_EXIT_Z (-2319.0F)
#define ACT0_EXIT_NEAR (20.0F * 20.0F)

#define NEAR_MARGIN 16.0F

#define ACT1_SKY_CAMERA 750.0F
#define SKY_BOUNDS 5.0F
#define ACT1_CLOUD_CAMERA 800.0F
#define CLOUD_WRAP 8192
#define CLOUD_TILE 2000.0F

/* where fun_0c913a20 draws its carrier, and the carrier's scale */
#define CARRIER_X (-3808.0F)
#define CARRIER_Y 1800.0F
#define CARRIER_Z (-2258.0F)
#define CARRIER_SCALE 0.2F

/* the top of act 2's updraft, and the clearance it needs over the ground */
#define UPDRAFT_TOP 150.0F
#define UPDRAFT_CLEAR 100.0F
/* updraft_level climbs by 1 each step in the updraft, to UPDRAFT_FULL. it
 * falls by UPDRAFT_FADE each step out of it, to 0 */
#define UPDRAFT_FULL 90
#define UPDRAFT_FADE 4
/* the lift for each level of updraft_level, and the push for each unit of
 * height below UPDRAFT_TOP */
#define UPDRAFT_LIFT 0.0075F
#define UPDRAFT_PUSH 0.002F

/* Knuckles' tutorial holds him within TUTORIAL_REACH of this point */
#define TUTORIAL_X (-3147.0F)
#define TUTORIAL_Z (-1398.0F)
#define TUTORIAL_REACH 60.0F
/* a glide of more than TUTORIAL_GLIDE steps in a row ends the tutorial. one
 * cut short shows the second prompt's other frame for TUTORIAL_CUT draws */
#define TUTORIAL_GLIDE 60
#define TUTORIAL_CUT 60
/* the prompts slide in from, and out to, this far either side of the
 * screen's middle */
#define TUTORIAL_OFF 576.0F

enum tutorial_mode {
    TUTORIAL_START,
    TUTORIAL_IN,
    TUTORIAL_SHOWN,
    TUTORIAL_OUT,
    TUTORIAL_DONE
};

/* sky modes, three for each act: start the act's sky, draw it, and reset it */
#define SKY_MODES 3
enum sky_mode { SKY_START, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))
/* the start mode of act 33, which the stage lacks. the sky task's next step
 * makes it the current act's start */
#define SKY_RESTART 99

/* the vars of fun_0c913bc6's and fun_0c913c6e's tasks. they hold
 * dir_light's direction, as a pitch from x toward y, then a yaw about y */
struct light_vars {
    Uint8 unk_00[4];
    Sint16 pitch;
    Sint16 yaw;
};

#define LIGHT(work) ((struct light_vars *)&(work)->vars)
/* nearly 60 degrees down */
#define LIGHT_DOWN (-10920)
/* a button's turn of the light each call: alone, with L held, and with R
 * held */
#define LIGHT_STEP 16
#define LIGHT_FINE 1
#define LIGHT_COARSE 256

static void mountain_start_act(struct task *task);
static void mountain_end(struct task *task);
static void mountain_next_act(struct task *task);
static void knuckles_tutorial_step(struct task *task);
static void knuckles_tutorial_draw(struct task *task);
static int solid_start(struct task *task);
static void solid_destroy(struct task *task);
static void solid_step(struct task *task);
static void solid_draw(struct task *task);
static void mountain_idle_step(struct task *task);
static void mountain_sky_draw(struct task *task);
static void mountain_sky_destroy(struct task *task);
static void knuckles_wall_step(struct task *task);
static void knuckles_wall_set(struct task *task);
static void knuckles_wall_destroy(struct task *task);
static void knuckles_wall_add(struct task *task);
static void knuckles_wall_move(struct task *task);
static void cloud_step(struct task *task);
static void cloud_draw(struct task *task);
static void cloud_draw_layer(struct task *task, NJS_OBJECT *object);
static void cloud_destroy(struct task *task);
static void mountain_updraft(void);

/* a task that starts the track in work->mode and then ends, once
 * work->timer counts down below 0. the count holds while an event is in
 * control. Tails gets no track */
static void mountain_music_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!event_in_control()) {
        --work->timer;
        if (work->timer < 0) {
            stream_reset();
            if (get_character() != CHARACTER_TAILS) {
                stream_request_music(work->mode);
            }
            task_destroy(task);
        }
    }
}

/* with Tails, the stage ends cleared on the task's first step */
void mountain_step(struct task *task)
{
    struct task_work *work = task->work;
    Float px;
    Float py;
    Float pz;
    Float distance;
    struct task_work *player;

    switch (work->mode) {
    case MOUNTAIN_START:
        mountain_start_act(task);
        if (get_character() == CHARACTER_TAILS) {
            stage_end(STAGE_END_CLEAR);
        }
        break;
    case MOUNTAIN_RUN:
        task_step_children(task);
        if (get_character() == CHARACTER_SONIC &&
            (stage_get_stage_act() & 0xff) == 0) {
            player = player_work[0];
            px = player->pos.x;
            py = player->pos.y;
            pz = player->pos.z;
            distance = vector_length_squared(ACT0_EXIT_X - px, ACT0_EXIT_Y - py,
                                             ACT0_EXIT_Z - pz);
            if (distance < ACT0_EXIT_NEAR) {
                mountain_next_act(task);
            }
        }
        break;
    case MOUNTAIN_RESUME:
        work->mode = MOUNTAIN_RUN;
        break;
    case MOUNTAIN_END:
        /* @hack redundant: a select whose condition is constant, which keeps
         * SHC from expanding mountain_end here; called plainly, it is
         * expanded and its function dropped, and 16 of 91 words differ */
        0 ? (void)0 : mountain_end(task);
        break;
    }
}

/* in act 2, unless the recorder is playing or recording, start Knuckles'
 * tutorial as a child of task. it hides the hud */
void knuckles_tutorial_create(struct task *task)
{
    /* @hack redundant: child set and never read; without it, the function is
     * 4 bytes longer and 9 of 30 words differ */
    struct task *child; /* NOLINT(clang-diagnostic-unused-but-set-variable) */

    tutorial_on = FALSE;
    if ((stage_get_stage_act() & 0xff) == 2) {
        if (recording_kind == RECORDING_OFF) {
            tutorial_on = TRUE;
            hud_hidden = -1;
            child = task_create_child(TASK_WORK, knuckles_tutorial_step, task);
        }
    }
}

/* hold player 0 within TUTORIAL_REACH, and slide the two prompts in. after a
 * long enough glide, chime and slide them out. pos.x and pos.z hold the
 * prompts' offsets from the screen's middle, and scl.x and scl.z their
 * speeds. tick counts down the draws left of the second prompt's other
 * frame */
static void knuckles_tutorial_step(struct task *task)
{
    struct task_work *player = player_work[0];
    struct task_work *work = task->work;
    Float x;
    Float z;
    Float reach;
    Angle ang;

    if (tutorial_on) {
        x = player->pos.x - TUTORIAL_X;
        z = player->pos.z - TUTORIAL_Z;
        ang = (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
        reach = njSqrt((x * x) + (z * z));
        if (reach > TUTORIAL_REACH) {
            reach = TUTORIAL_REACH;
        }
        x = njCos(ang) * reach;
        z = njSin(ang) * reach;
        player->pos.x = x + TUTORIAL_X;
        player->pos.z = z + TUTORIAL_Z;
        switch (work->submode) {
        case TUTORIAL_START:
            work->pos.x = TUTORIAL_OFF;
            work->pos.z = -TUTORIAL_OFF;
            work->scl.x = 0.0F;
            work->scl.z = 0.0F;
            work->tick = 0;
            work->timer = 0;
            work->submode = TUTORIAL_IN;
            break;
        case TUTORIAL_IN:
            work->scl.x -= 2.0F;
            work->pos.x += work->scl.x;
            if (work->pos.x < 0.0F) {
                work->pos.x = 0.0F;
            }
            work->scl.z += 2.0F;
            work->pos.z += work->scl.z;
            if (work->pos.z > 0.0F) {
                work->pos.z = 0.0F;
            }
            if (work->pos.x == 0.0F && work->pos.z == 0.0F) {
                work->submode = TUTORIAL_SHOWN;
            }
            break;
        case TUTORIAL_SHOWN:
            break;
        case TUTORIAL_OUT:
            if (work->scl.x < 0.0F) {
                work->scl.x = 0.0F;
            }
            if (work->scl.z > 0.0F) {
                work->scl.z = 0.0F;
            }
            work->scl.x += 4.0F;
            work->pos.x += work->scl.x;
            if (work->pos.x > TUTORIAL_OFF) {
                work->pos.x = TUTORIAL_OFF;
            }
            work->scl.z -= 4.0F;
            work->pos.z += work->scl.z;
            if (work->pos.z < -TUTORIAL_OFF) {
                work->pos.z = -TUTORIAL_OFF;
            }
            if (work->pos.x == TUTORIAL_OFF && work->pos.z == -TUTORIAL_OFF) {
                work->submode = TUTORIAL_DONE;
            }
            break;
        case TUTORIAL_DONE:
            tutorial_on = FALSE;
            hud_hidden = 0;
            break;
        }
        if (work->submode <= TUTORIAL_SHOWN) {
            if (work->timer == 0) {
                if (player->mode == KNUCKLES_GLIDE) {
                    work->timer = 1;
                }
            } else if (player->mode != KNUCKLES_GLIDE) {
                work->timer = 0;
                work->tick = TUTORIAL_CUT;
            } else {
                ++work->timer;
                if (TASK_TIMER(work) > TUTORIAL_GLIDE) {
                    sound_play(SE_NOTICE, 0, 0, 0);
                    work->submode = TUTORIAL_OUT;
                }
            }
        }
        knuckles_tutorial_draw(task);
    }
}

static void knuckles_tutorial_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE sprite;
    NJS_ARGB material;
    Int frame;

    if (frame_step) {
        return;
    }
    njSetTexture(&knuckles_tutorial_texlist);
    sprite.tlist = &knuckles_tutorial_texlist;
    sprite.tanim = knuckles_tutorial_frames;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = work->pos.x + 320.0F;
    sprite.p.y = 96.0F;
    material.a = 0.6F;
    material.r = 1.0F;
    material.g = 1.0F;
    material.b = 1.0F;
    njSetConstantMaterial(&material);
    njDrawSprite2D(&sprite, 0, -11.0F, NJD_SPRITE_ALPHA);
    sprite.p.x = work->pos.z + 320.0F;
    sprite.p.y = 384.0F;
    if (work->tick) {
        --work->tick;
        frame = 2;
    } else {
        frame = 1;
    }
    njDrawSprite2D(&sprite, frame, -11.0F, NJD_SPRITE_ALPHA);
    material.a = 0.0F;
    material.r = 0.0F;
    material.g = 0.0F;
    material.b = 0.0F;
    njSetConstantMaterial(&material);
}

/* point mountain_work at the stage task's work, and start the act. each act
 * sets its fog and wind, and creates its tasks and its track's task */
static void mountain_start_act(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    NJS_POINT3 pos;
    Uint16 stage;

    mountain_work = work;
    cloud_scroll_near = cloud_scroll_far = 0;
    stage = stage_get_stage_act();
    switch (stage & 0xff) {
    case 0:
        work->mode = MOUNTAIN_RUN;
        rock_burst_create();
        task_create_child(TASK_WORK, cloud_step, task);
        njSetFogColor(0xffffffff);
        njGenerateFogTable3(mountain_fog, -550.0F, -9600.0F);
        njSetFogTable(mountain_fog);
        njFogEnable();
        task_create(TASK_WORK, 3, lava_anim_step);
        pos.x = 0.0F;
        pos.y = 10000.0F;
        pos.z = 0.0F;
        lens_create(&pos);
        child = task_create_child(TASK_WORK, mountain_music_step, task);
        child->work->mode = MUSIC_REDMNTN1;
        child->work->timer = MUSIC_DELAY;
        sound_play_held(SE_WIND, -1, 8, 0);
        break;
    case 1:
        work->mode = MOUNTAIN_RUN;
        task_create_child(TASK_WORK, cloud_step, task);
        njSetFogColor(0xff000000);
        njGenerateFogTable3(mountain_fog, -850.0F, -1200.0F);
        njSetFogTable(mountain_fog);
        njFogEnable();
        task_create(TASK_WORK, 3, lava_anim_step);
        child = task_create_child(TASK_WORK, mountain_music_step, task);
        child->work->mode = MUSIC_REDMNTN2;
        child->work->timer = MUSIC_DELAY;
        sound_play_held(SE_ACT1_WIND, -1, 8, 0);
        task_create_child(0, mountain_idle_step, task);
        break;
    case 2:
        work->mode = MOUNTAIN_RUN;
        if (get_character() == CHARACTER_KNUCKLES) {
            task_create_child(TASK_WORK | TASK_FORCE, knuckles_wall_step, task);
        }
        task_create_child(TASK_WORK, cloud_step, task);
        njSetFogColor(0xffffffff);
        njGenerateFogTable3(mountain_fog, -500.0F, -5000.0F);
        njSetFogTable(mountain_fog);
        njFogEnable();
        task_create(TASK_WORK, 3, lava_anim_step);
        pos.x = 0.0F;
        pos.y = 10000.0F;
        pos.z = 0.0F;
        lens_create(&pos);
        child = task_create_child(TASK_WORK, mountain_music_step, task);
        child->work->mode = MUSIC_REDMNTN1;
        child->work->timer = MUSIC_DELAY;
        sound_play_held(SE_WIND, -1, 8, 0);
        break;
    default:
        work->mode = MOUNTAIN_RUN;
        break;
    }
}

static void mountain_end(struct task *task)
{
    mountain_work = NULL;
}

/* go on from act 0 to act 1 under the same stage task, whose children it
 * replaces */
/* @hack redundant: register on task, which keeps it in R14 across the calls
 * (0x0c913320); without it, it is kept in the frame, the function is 2 bytes
 * shorter and 31 of 31 words differ */
static void mountain_next_act(register struct task *task)
{
    stream_destroy();
    world_change_act(1);
    placement_change_act(1);
    camera_change_act(1);
    stage_change_act(1);
    stage_place_player(player_work[0]);
    light_load_act();
    task_destroy_children(task);
    mountain_start_act(task);
}

/* start dat_0ca754a8 as a solid model. it draws in the stage slot's texture
 * list, or in obj_mountain_texlist while that slot is empty */
void fun_0c91335e(struct task *task)
{
    struct task_work *work = task->work;

    WORLD_MODEL(work)->model = &dat_0ca754a8;
    WORLD_MODEL(work)->texlist = &obj_mountain_texlist;
    solid_start(task);
}

/* unless the set editor holds the task, add a solid copy of the model to the
 * world. its radius picks its world size. then make solid_step and solid_draw
 * the task's. returns 0 */
static int solid_start(struct task *task)
{
    struct task_work *work;
    NJS_OBJECT *world;
    Uint32 flags;

    work = task->work;
    if (!placement_editing(task)) {
        WORLD_MODEL(work)->world = world = world_object_alloc();
        world->evalflags = WORLD_MODEL(work)->model->evalflags &
                           ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = work->ang.x;
        world->ang[1] = work->ang.y;
        world->ang[2] = work->ang.z;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = WORLD_MODEL(work)->model->model;
        world->child = WORLD_MODEL(work)->model->child;
        world->sibling = WORLD_MODEL(work)->model->sibling;
        flags = WORLD_HUGE | WORLD_SOLID;
        /* @hack spelling: flags set before the tests, which leaves the first
         * arm empty; set in that arm, the function is 2 bytes longer and 42 of
         * 97 words differ */
        if (world->model->r > 20.0F) {
        } else if (world->model->r > 10.0F) {
            flags = WORLD_LARGE | WORLD_SOLID;
        } else if (world->model->r > 3.0F) {
            flags = WORLD_MEDIUM | WORLD_SOLID;
        } else {
            flags = WORLD_SMALL | WORLD_SOLID;
        }
        world_add(flags, task, world);
    }
    task->destructor = solid_destroy;
    task->step = solid_step;
    task->draw = solid_draw;
    return 0;
}

static void solid_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}

/* the world tests the copy only while a player is within its radius plus
 * NEAR_MARGIN */
static void solid_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world = WORLD_MODEL(work)->world;

    if (world != NULL) {
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->ang[0] = work->ang.x;
        world->ang[1] = work->ang.y;
        world->ang[2] = work->ang.z;
        if (player_in_range(&work->pos, world->model->r + NEAR_MARGIN)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    if (task->destructor == solid_destroy) {
        solid_draw(task);
    }
}

static void solid_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (!texture_set_stage()) {
        njSetTexture(WORLD_MODEL(work)->texlist);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(WORLD_MODEL(work)->model);
    njPopMatrix(1);
}

static void mountain_idle_step(struct task *task)
{
}

/* the sky task's mode is the act times SKY_MODES plus a sky_mode. a start
 * mode of an act other than the current one becomes the current act's
 * start. a draw mode of another act moves on to that act's reset. while the
 * stage is other than Red Mountain, each step also moves the mode on by one.
 * act 1's sky starts on a black back color, and act 2's runs the updraft */
void mountain_sky_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = stage & 0xff;

    stage &= STAGE_MASK;
    if (work->mode / SKY_MODES != act) {
        if (work->mode % SKY_MODES == SKY_DRAW) {
            ++work->mode;
        } else if (work->mode % SKY_MODES == SKY_START) {
            work->mode = act * SKY_MODES;
        }
    }
    if (stage != STAGE_MOUNTAIN << 8) {
        ++work->mode;
        act = 4;
    }
    switch (work->mode) {
    case SKY_MODE(0, SKY_START):
        task->draw = mountain_sky_draw;
        task->destructor = mountain_sky_destroy;
        ++work->mode;
        /* fall through */
    case SKY_MODE(0, SKY_DRAW):
        task_step_children(task);
        mountain_sky_draw(task);
        break;
    case SKY_MODE(0, SKY_RESET):
        work->mode = act * SKY_MODES;
        task_destroy_children(task);
        break;
    case SKY_MODE(1, SKY_START):
        task->draw = mountain_sky_draw;
        task->destructor = mountain_sky_destroy;
        njSetBackColor(0, 0, 0);
        ++work->mode;
        /* fall through */
    case SKY_MODE(1, SKY_DRAW):
        task_step_children(task);
        mountain_sky_draw(task);
        break;
    case SKY_MODE(1, SKY_RESET):
        work->mode = SKY_RESTART;
        break;
    case SKY_MODE(2, SKY_START):
        task->draw = mountain_sky_draw;
        task->destructor = mountain_sky_destroy;
        work->mode = SKY_MODE(2, SKY_DRAW);
        /* fall through */
    case SKY_MODE(2, SKY_DRAW):
        mountain_updraft();
        mountain_sky_draw(task);
        break;
    case SKY_MODE(2, SKY_RESET):
        work->mode = SKY_RESTART;
        break;
    }
}

void fun_0c9137a0(struct task *task)
{
}

/* draw the sky's four models at the act's place, without fog. their constant
 * material is the sky task's scl.y. the first model's center and radius are
 * SKY_BOUNDS times as far and as big while they draw. act 1 draws the sky only
 * while the camera is above ACT1_SKY_CAMERA. a call that draws nothing asks
 * for an fx level of 0 */
static void mountain_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    int act = stage_get_stage_act() & 0xff;
    Float y;
    Float x;
    Float z;
    Float light;
    Float fade;
    Float center_x;
    Float radius;
    Float center_z;
    Float center_y;
    NJS_POINT3 pos;
    NJS_ARGB material;
    NJS_OBJECT *object;
    NJS_OBJECT *part;

    switch (act) {
    case 1:
        x = -500.0F;
        y = 700.0F;
        z = 3000.0F;
        break;
    case 2:
        x = z = 0.0F;
        y = 100.0F;
        break;
    default:
        x = y = z = 0.0F;
        break;
    }
    if ((act != 1 || camera_work->pos.y > ACT1_SKY_CAMERA) && !frame_step) {
        draw_control_save();
        draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
        njFogDisable();
        njClipZ(-3.0F, -50000.0F);
        light = 1.0F;
        /* @hack redundant: fade, from pos.y before the call at 0x0c913974
         * first sets pos (0x0c913892), and never read; without it, the
         * function is 34 bytes shorter and 163 of 206 words differ */
        fade = _builtin_fabsf(pos.y - 570.0F);
        fade = light - (fade * 0.01F);
        if (fade < 0.0F) {
            fade = 0.0F;
        }
        material.a = 0.0F;
        light *= work->scl.y;
        material.r = material.g = material.b = light;
        njSetConstantMaterial(&material);
        njSetTexture(&obj_mountain_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, x, y, z);
        object = &mountain_sky_0_object;
        center_x = object->model->center.x;
        center_y = object->model->center.y;
        center_z = object->model->center.z;
        radius = object->model->r;
        object->model->center.x = center_x * SKY_BOUNDS;
        object->model->center.y = center_y * SKY_BOUNDS;
        object->model->center.z = center_z * SKY_BOUNDS;
        object->model->r = radius * SKY_BOUNDS;
        draw_model(object->model);
        /* @hack spelling: each model through part, which puts each object in
         * the pool (0x0c913928 on); from the objects themselves, the pool holds
         * the members' addresses and 8 of 206 words differ */
        draw_model((part = &mountain_sky_1_object)->model);
        draw_model((part = &mountain_sky_2_object)->model);
        draw_model((part = &mountain_sky_3_object)->model);
        object->model->center.x = center_x;
        object->model->center.y = center_y;
        object->model->center.z = center_z;
        object->model->r = radius;
        njPopMatrix(1);
        njClipZ(-3.0F, -5000.0F);
        njFogEnable();
        player_get_history(0, 0, &pos, NULL);
        draw_control_restore();
    } else {
        sound_request_fx(0);
    }
}

static void mountain_sky_destroy(struct task *task)
{
}

/* draw dat_0caaed9c in MOUNTAIN_CARRIER's textures, at CARRIER_SCALE of its
 * size. it shows for Sonic or Tails until that character clears the
 * stage. it draws only in mode 0, but plays on and turns in any mode. the
 * d-pad's left and right on pad 0 turn it */
void fun_0c913a20(int mode)
{
    if (get_character() != CHARACTER_SONIC) {
        goto tails;
    }
    if (sonic_mountain_clear == 0) {
        goto draw;
    } else {
    tails:
        if (get_character() != CHARACTER_TAILS) {
            return;
        } else {
            if (tails_mountain_clear != 0) {
                return;
            }
        }
    }
draw:
    if (pad_records[0].on & PDD_DGT_KL) {
        carrier_ang += 136;
    }
    if (pad_records[0].on & PDD_DGT_KR) {
        carrier_ang -= 136;
    }
    carrier_frame += 1.0F;
    if (carrier_frame >= 30.0F) {
        carrier_frame = 0.0F;
    }
    njClipZ(-3.0F, -5000.0F);
    njFogDisable();
    switch (mode) {
    case 0:
        if (!draw_in_view(CARRIER_X, CARRIER_Y, CARRIER_Z, 1000.0F)) {
            return;
        }
        njSetTexture(&dat_0caaeea0);
        njPushMatrix(NULL);
        njTranslate(NULL, CARRIER_X, CARRIER_Y, CARRIER_Z);
        TURN(njRotateY, carrier_ang);
        draw_divide_light(CARRIER_SCALE);
        njScale(NULL, CARRIER_SCALE, CARRIER_SCALE, CARRIER_SCALE);
        draw_action(&dat_0caaed9c, carrier_frame);
        draw_reset_light();
        njPopMatrix(1);
        break;
    }
    njClipZ(-3.0F, -5000.0F);
    njFogEnable();
}

/* point dir_light along x, pitched down by LIGHT_DOWN */
void fun_0c913bc6(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR dir;
    NJS_VECTOR light;
    Angle ang;

    switch (work->mode) {
    case 0:
        LIGHT(work)->pitch = LIGHT_DOWN;
        LIGHT(work)->yaw = 0;
        work->mode = 1;
        /* fall through */
    case 1:
    default:
        break;
    }
    light.x = njCos(LIGHT(work)->pitch);
    light.y = njSin(LIGHT(work)->pitch);
    light.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, LIGHT(work)->yaw, ang);
    njCalcVector(NULL, &light, &dir);
    njPopMatrix(1);
    njSetLightDirection(&dir_light, dir.x, dir.y, dir.z);
}

/* point dir_light as the second pad turns it, from along -x. X and A raise
 * and lower its pitch, and Y and B its yaw. with L and R both held, a button
 * moves it LIGHT_FINE and LIGHT_COARSE together */
void fun_0c913c6e(struct task *task)
{
    struct task_work *work = task->work;
    NJS_VECTOR dir;
    NJS_VECTOR light;
    Angle ang;

    switch (work->mode) {
    case 0:
        LIGHT(work)->pitch = 0;
        LIGHT(work)->yaw = -32768;
        work->mode = 1;
        /* fall through */
    case 1:
        if (per_info[1]->on & PDD_DGT_TX) {
            if (per_info[1]->on & PDD_DGT_TL) {
                LIGHT(work)->pitch += LIGHT_FINE;
            }
            if (per_info[1]->on & PDD_DGT_TR) {
                LIGHT(work)->pitch += LIGHT_COARSE;
            }
            if (!((per_info[1]->on & PDD_DGT_TL) |
                  (per_info[1]->on & PDD_DGT_TR))) {
                LIGHT(work)->pitch += LIGHT_STEP;
            }
        }
        if (per_info[1]->on & PDD_DGT_TA) {
            if (per_info[1]->on & PDD_DGT_TL) {
                LIGHT(work)->pitch -= LIGHT_FINE;
            }
            if (per_info[1]->on & PDD_DGT_TR) {
                LIGHT(work)->pitch -= LIGHT_COARSE;
            }
            if (!((per_info[1]->on & PDD_DGT_TL) |
                  (per_info[1]->on & PDD_DGT_TR))) {
                LIGHT(work)->pitch -= LIGHT_STEP;
            }
        }
        if (per_info[1]->on & PDD_DGT_TY) {
            if (per_info[1]->on & PDD_DGT_TL) {
                LIGHT(work)->yaw += LIGHT_FINE;
            }
            if (per_info[1]->on & PDD_DGT_TR) {
                LIGHT(work)->yaw += LIGHT_COARSE;
            }
            if (!((per_info[1]->on & PDD_DGT_TL) |
                  (per_info[1]->on & PDD_DGT_TR))) {
                LIGHT(work)->yaw += LIGHT_STEP;
            }
        }
        if (per_info[1]->on & PDD_DGT_TB) {
            if (per_info[1]->on & PDD_DGT_TL) {
                LIGHT(work)->yaw -= LIGHT_FINE;
            }
            if (per_info[1]->on & PDD_DGT_TR) {
                LIGHT(work)->yaw -= LIGHT_COARSE;
            }
            if (!((per_info[1]->on & PDD_DGT_TL) |
                  (per_info[1]->on & PDD_DGT_TR))) {
                LIGHT(work)->yaw -= LIGHT_STEP;
            }
        }
        break;
    }
    light.x = njCos(LIGHT(work)->pitch);
    light.y = njSin(LIGHT(work)->pitch);
    light.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateY, LIGHT(work)->yaw, ang);
    njCalcVector(NULL, &light, &dir);
    njPopMatrix(1);
    njSetLightDirection(&dir_light, dir.x, dir.y, dir.z);
}

/* unless the set editor holds the task, put knuckles_wall_object in the
 * world at the origin, solid. the stage task makes this task for Knuckles in
 * act 2. its draw only sets the model, so the wall is never drawn */
static void knuckles_wall_step(struct task *task)
{
    struct task_work *work = task->work;

    knuckles_wall_set(task);
    switch (work->mode) {
    case 0:
        work->pos.x = work->pos.y = work->pos.z = 0.0F;
        knuckles_wall_set(task);
        knuckles_wall_add(task);
        task->draw = knuckles_wall_set;
        task->destructor = knuckles_wall_destroy;
        work->mode = 1;
        break;
    case 1:
        knuckles_wall_set(task);
        knuckles_wall_move(task);
        break;
    }
}

static void knuckles_wall_set(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = knuckles_wall_object;

    WORLD_MODEL(work)->model = object;
}

static void knuckles_wall_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}

static void knuckles_wall_add(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world;

    if (!placement_editing(task)) {
        WORLD_MODEL(work)->world = world = world_object_alloc();
        world->evalflags = NJD_EVAL_BREAK;
        world->scl[0] = 1.0F;
        world->scl[1] = 1.0F;
        world->scl[2] = 1.0F;
        world->ang[0] = 0;
        world->ang[1] = 0;
        world->ang[2] = 0;
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        world->model = WORLD_MODEL(work)->model->model;
        world->child = WORLD_MODEL(work)->model->child;
        world->sibling = WORLD_MODEL(work)->model->sibling;
        world_add(WORLD_UNCLIMBABLE | WORLD_SOLID, task, world);
    }
}

static void knuckles_wall_move(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *world = WORLD_MODEL(work)->world;

    if (world != NULL) {
        world->pos[0] = work->pos.x;
        world->pos[1] = work->pos.y;
        world->pos[2] = work->pos.z;
        work->flags |= WORLD_TESTED;
    }
}

/* move each layer's scroll on. then copy the first four of cloud_uvs into the
 * layer's uvs, each v plus the scroll */
static void cloud_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_COLOR *from;
    NJS_COLOR *to;
    Sint16 tex_u;
    Sint16 tex_v;
    int i;

    switch (work->mode) {
    case 0:
        task->draw = cloud_draw;
        task->destructor = cloud_destroy;
        work->mode = 1;
        break;
    case 1:
        --cloud_scroll_near;
        if (cloud_scroll_near < 0) {
            cloud_scroll_near += CLOUD_WRAP;
        }
        from = cloud_uvs;
        to = cloud_uvs_near;
        for (i = 4; i > 0; --i) {
            tex_v = from->color;
            tex_u = from->color >> 16;
            ++from;
            tex_v += cloud_scroll_near;
            to->color = (((Uint32)tex_u << 16) & 0xffff0000) | (Uint16)tex_v;
            ++to;
        }
        cloud_scroll_far -= 2;
        if (cloud_scroll_far < 0) {
            cloud_scroll_far += CLOUD_WRAP;
        }
        from = cloud_uvs;
        to = cloud_uvs_far;
        for (i = 4; i > 0; --i) {
            tex_v = from->color;
            tex_u = from->color >> 16;
            ++from;
            tex_v += cloud_scroll_far;
            to->color = (((Uint32)tex_u << 16) & 0xffff0000) | (Uint16)tex_v;
            ++to;
        }
        cloud_draw(task);
        break;
    }
}

/* draw the near layer, then the far one below it, lower for Knuckles and
 * higher in act 1. act 1 draws them only while the camera is above
 * ACT1_CLOUD_CAMERA */
static void cloud_draw(struct task *task)
{
    struct task_work *work = task->work;
    int act;
    NJS_OBJECT *object;

    njFogDisable();
    act = stage_get_stage_act() & 0xff;
    if (act != 1 || camera_work->pos.y > ACT1_CLOUD_CAMERA) {
        object = &cloud_object;
        if (get_character() == CHARACTER_KNUCKLES) {
            work->pos.y = 0.0F;
        } else {
            work->pos.y = 330.0F;
        }
        if (act == 1) {
            work->pos.y = 700.0F;
        }
        object->model->meshsets->vertuv = cloud_uvs_near;
        cloud_draw_layer(task, object);
        if (get_character() == CHARACTER_KNUCKLES) {
            work->pos.y = -50.0F;
        } else {
            work->pos.y = 280.0F;
        }
        if (act == 1) {
            work->pos.y = 680.0F;
        }
        object->model->meshsets->vertuv = cloud_uvs_far;
        cloud_draw_layer(task, object);
    }
    njFogEnable();
}

/* draw a layer at the height in pos.y, as three by three tiles CLOUD_TILE
 * across. the middle tile sits on the multiples of CLOUD_TILE nearest the
 * camera. scl.y keeps how near the camera's height is to the layer's, from 0
 * to 0.8. scl.y sets the layer's shade */
static void cloud_draw_layer(struct task *task, NJS_OBJECT *object)
{
    struct task_work *work = task->work;
    NJS_ARGB material;
    NJS_POINT3 pos;
    Float x;
    Float z;
    Float tile_x;
    Float tile_z;
    Float fade;
    int i;
    int j;

    if (frame_step) {
        return;
    }
    pos = camera_work->pos;
    fade = _builtin_fabsf(pos.y - work->pos.y);
    fade *= 0.025F;
    work->scl.y = 1.0F - fade;
    if (work->scl.y < 0.0F) {
        work->scl.y = 0.0F;
    }
    if (work->scl.y > 0.8F) {
        work->scl.y = 0.8F;
    }
    njSetTexture(&obj_mountain_texlist);
    material.a = material.r = material.g = material.b =
        -(work->scl.y * 0.8F) - 0.2F;
    njSetConstantMaterial(&material);
    x = camera_work->pos.x;
    z = camera_work->pos.z;
    if (x < 0.0F) {
        x -= CLOUD_TILE;
    }
    if (z < 0.0F) {
        z -= CLOUD_TILE;
    }
    x += CLOUD_TILE / 2;
    z += CLOUD_TILE / 2;
    tile_x = njRoundOff(x * (1.0F / CLOUD_TILE)) * CLOUD_TILE;
    tile_z = njRoundOff(z * (1.0F / CLOUD_TILE)) * CLOUD_TILE;
    for (j = -1; j < 2; ++j) {
        for (i = -1; i < 2; ++i) {
            njPushMatrix(NULL);
            njTranslate(NULL, tile_x + (i * CLOUD_TILE), work->pos.y,
                        tile_z + (j * CLOUD_TILE));
            draw_tree(object);
            njPopMatrix(1);
        }
    }
    material.a = material.r = material.g = material.b = 0.0F;
    njSetConstantMaterial(&material);
}

static void cloud_destroy(struct task *task)
{
}

/* act 2's updraft, which adds to player 0's accel, what its speed gains this
 * step. player 0 is in the updraft while pos.y - 1 is at most UPDRAFT_TOP.
 * pos.y + 2.5 must also be UPDRAFT_CLEAR or more above the ground. while
 * pos.y - 1 is below 0, updraft_level starts again from 0.
 *
 * the lift is UPDRAFT_LIFT times updraft_level. it is a quarter of that unless
 * accel.y is negative. accel.y gains the lift, and is then held to the lift at
 * most. the push is UPDRAFT_PUSH times how far pos.y - 1 is below UPDRAFT_TOP.
 * out of the updraft, the push is 0. accel.x, which is ahead, gains the push
 * and is then held to it at most. the winds' volume follows updraft_level */
static void mountain_updraft(void)
{
    struct task_work *player = player_work[0];
    Float x = player->pos.x;
    Float y = player->pos.y + 2.5F;
    Float z = player->pos.z;
    struct angles slope;
    int near_floor = 0;
    Float floor;
    Float sound_y;
    Float lift;
    Float push;

    slope = mountain_no_slope;
    floor = ground_below(x, y, z, &slope);
    y -= floor;
    if (y < UPDRAFT_CLEAR) {
        near_floor = 1;
    }
    y = player_work[0]->pos.y - 1.0F;
    if (y < 0.0F) {
        updraft_level = 0;
    }
    if (y > UPDRAFT_TOP || near_floor) {
        updraft_level -= UPDRAFT_FADE;
        if (updraft_level < 0) {
            updraft_level = 0;
        }
        y = UPDRAFT_TOP;
    } else {
        ++updraft_level;
        if (updraft_level > UPDRAFT_FULL) {
            updraft_level = UPDRAFT_FULL;
        }
    }
    sound_y = 1.0F;
    if (y > UPDRAFT_SOUND_ABOVE) {
        sound_play_at_for(SE_UPDRAFT, 555, 1, updraft_level - 64, x, sound_y, z,
                          4);
    }
    sound_play_at_for(SE_WIND, 666, 1, updraft_level - 64, x, sound_y, z, 4);
    lift = updraft_level * UPDRAFT_LIFT;
    y = UPDRAFT_TOP - y;
    if (y < 0.0F) {
        y = 0.0F;
    }
    push = y * UPDRAFT_PUSH;
    /* @hack redundant: an add of 0 to accel.z, which keeps the image's FADD
     * FR6,FR2 at 0x0c9145c2; without it, 64 of 184 words differ */
    player_motion[0]->accel.z += 0.0F;
    if (!(player_motion[0]->accel.y < 0.0F)) {
        lift *= 0.25F;
    }
    player_motion[0]->accel.y += lift;
    if (player_motion[0]->accel.y > lift) {
        player_motion[0]->accel.y = lift;
    }
    player_motion[0]->accel.x += push;
    if (player_motion[0]->accel.x > push) {
        player_motion[0]->accel.x = push;
    }
}
