/* Ice Cap's stage task, and its sky task, which sets each act's light and
 * fog */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "draw/light.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/lens.h"
#include "effect/snow.h"
#include "event/event.h"
#include "object/placement.h"
#include "player/fishing.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/sonic.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg08/act_joins.h"
#include "stg08/gates.h"
#include "stg08/icecap.h"
#include "stg08/land3.h"
#include "stg08/sky_object_2.h"
#include "stg08/sky_textures.h"
#include "stg08/snow_slide.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT icecap_sky_object_0;
extern NJS_OBJECT icecap_sky_object_1;

/* the stage task's modes. ICECAP_BOARD_WAIT to ICECAP_BOARD_GO are act 2's
 * opening, when player 0 comes from act 1 */
enum icecap_mode {
    ICECAP_START,
    ICECAP_ACT0, /* until player 0 reaches icecap_act0_exit */
    ICECAP_ACT1, /* until player 0 reaches icecap_act1_exit */
    ICECAP_BOARD_WAIT, /* until the board's stand stores ICECAP_BOARD_TAKEN */
    ICECAP_BOARD_WALK, /* one step, which starts player 0's first walk */
    ICECAP_BOARD_STOP, /* until that walk ends */
    ICECAP_BOARD_LOOK, /* the camera looks around, for OPENING_STEPS steps */
    ICECAP_BOARD_GO, /* until player 0's walk to the board ends */
    ICECAP_ACT2, /* the board run, until player 0 reaches its end */
    ICECAP_END,
    ICECAP_ACT3,
    ICECAP_IDLE,
    ICECAP_FALLEN /* player 0 has fallen out of the stage */
};

struct icecap_vars {
    Uint32 look_steps; /* the steps spent in ICECAP_BOARD_LOOK */
};

#define VARS(work) ((struct icecap_vars *)&(work)->vars)

/* steps before the act's track starts */
#define MUSIC_DELAY 10

/* the held sounds of acts 0 and 2, and of acts 1 and 3. SE_BOARD_OPENING
 * plays in the board run's opening */
#define SE_ACT0_HELD 973
#define SE_ACT1_HELD 971
#define SE_BOARD_OPENING 1122

/* player 0 goes on from act 0 or act 1 within ACT0_EXIT_NEAR or
 * ACT1_EXIT_NEAR of its exit. the board run ends within ACT2_END_NEAR of its
 * end */
#define ACT0_EXIT_NEAR 35.0F
#define ACT1_EXIT_NEAR 40.0F
#define ACT2_END_NEAR 200.0F
/* player 0 below this height in act 0 starts a fall out of the stage */
#define ACT0_FLOOR (-475.0F)

/* the board run's opening holds ICECAP_BOARD_LOOK until OPENING_STEPS steps
 * have passed. on step OPENING_LOOK, player 0 looks at icecap_look_point, and
 * the camera turns to the player. the opening's sound and music start on
 * OPENING_VOICE. on OPENING_FAR, the camera moves to look far off, near the
 * point player 0 looks at */
#define OPENING_STEPS 180
#define OPENING_LOOK 30
#define OPENING_VOICE 40
#define OPENING_FAR 80

/* the board run's view angle is VIEW_NARROW, 70 degrees, widened by up to
 * VIEW_WIDER, 30 more. the widening is VIEW_WIDER times the square root of
 * player 0's speed over BOARD_FAST, held to 1. the view approaches the angle
 * by VIEW_TURN a step */
#define VIEW_NARROW 0x31c7
#define VIEW_WIDER 5461.0F
#define VIEW_TURN 0x200
#define BOARD_FAST (4.6F * 1.7F)

/* player 0 on land within JUMP_NEAR of the jump point is sent flying. both
 * players then keep at least BOOST_SPEED for BOOST_STEPS + 2 steps */
#define JUMP_NEAR 30.0F
#define BOOST_SPEED 4.0F
#define BOOST_STEPS 100

/* a player breathes out every BREATH_STEPS steps, or twice as often while
 * faster than its sprint speed */
#define BREATH_STEPS 180

struct icecap_breath_vars {
    Uint8 unk_00[4];
    Sint16 wait[2]; /* the steps to each player's next breath */
};

#define BREATH(work) ((struct icecap_breath_vars *)&(work)->vars)

/* the boost task's TASK_EXTRA record */
struct icecap_boost {
    Uint32 steps; /* the steps it has held the players' speed */
};

/* act 3 draws its land blocks by where its center is. the center is the
 * camera while it follows Big's lure in the water, and player 0 otherwise.
 * the blocks change above ACT3_HIGH and below ACT3_LOW, and at x ACT3_SIDE, or
 * ACT3_LOW_SIDE below ACT3_LOW */
#define ACT3_HIGH 210.0F
#define ACT3_LOW 0.0F
#define ACT3_SIDE 1666.0F
#define ACT3_LOW_SIDE 1612.0F

/* sky modes, three for each act: start the act's sky, draw it, and reset it */
#define SKY_MODES 3
enum sky_mode { SKY_START, SKY_DRAW, SKY_RESET };
#define SKY_MODE(act, mode) (((act) * SKY_MODES) + (mode))

/* depths below a height of 0, at which act 2's sky and fog change as
 * player 0 descends. the sky draws only while he is shallower than
 * FOG_CLOSING or deeper than FOG_OPENED. the fog's near distance is
 * FOG_OPEN_NEAR down to FOG_CLOSING, and closes in to FOG_CLOSED_NEAR by
 * FOG_CLOSED. it holds there down to FOG_OPENING, then jumps out to 2600.
 * it closes in again to FOG_OPEN_NEAR by FOG_OPENED, and holds there */
#define FOG_CLOSING 4000.0F
#define FOG_CLOSED 7000.0F
#define FOG_OPENING 15500.0F
#define FOG_OPENED 18500.0F
/* the fog's near distance while it is open and while it is closed. while
 * it holds, its far distance is FOG_BAND past its near one. while it
 * moves, its near distance shrinks by FOG_SLOPE for each unit player 0
 * descends. its far distance is then FOG_BAND nearer than its near one */
#define FOG_OPEN_NEAR 1600.0F
#define FOG_CLOSED_NEAR 600.0F
#define FOG_BAND 500.0F
#define FOG_SLOPE (1.0F / 3.0F)

/* the sky task's light, a point light, which acts 0, 1 and 3 make */
static struct light_spec icecap_light_spec = {
#include "stg08/icecap/icecap_light_spec.inc"
};

/* a breath's puff. each breath writes its growth, pos and speed here before
 * snow_create_from makes it */
static struct dust_params icecap_breath = {
    0.01F,
    0.002F,
    0.05F,
    0.005F,
    -0.0005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.8F, 0.4F, 0.4F, 0.4F},
};

/* made again as acts 0 and 2 start, and at each draw of act 2's sky */
static NJS_FOG_TABLE icecap_fog_table = {
#include "stg08/icecap/icecap_fog_table.inc"
};

static NJS_LIGHT icecap_sky_light;

static void icecap_breath_step(struct task *task);
static void icecap_land_set_solid(Sint32 blocks);
static void icecap_act_start(struct task *task);
static void icecap_act0_step(struct task *task);
static void icecap_act1_step(struct task *task);
static void icecap_act2_step(struct task *task);
static void icecap_board_start_step(struct task *task);
/* @hack type: icecap_idle declared without a prototype, which keeps the call
 * to it at 0x0c90f076; prototyped, SHC drops the call, and 22 of
 * icecap_step's 368 words differ */
static void icecap_idle(); /* NOLINT(clang-diagnostic-strict-prototypes) */
static void icecap_sky_run(struct task *task);
static void icecap_sky_draw(struct task *task);
static void icecap_sky_destroy(struct task *task);

/* a task that does nothing for work->timer steps, and ends on the next */
static void icecap_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        task_destroy(task);
    }
}

/* show each player's breath once its wait has run out and its animation's
 * frame is below 1. a player under water shows none */
static void icecap_breath_step(struct task *task)
{
    struct task_work *work = task->work;
    struct player *record;
    struct task_work *player;
    struct dust_params *params;
    NJS_VECTOR speed;
    NJS_POINT3 pos;
    int i;

    for (i = 0; i < 2; ++i) {
        record = player_get(i);
        if (record == NULL || (record->shadow.above_flags & WORLD_WATER)) {
            continue;
        } else if (BREATH(work)->wait[i] > 0) {
            --BREATH(work)->wait[i];
            if (record->speed.x > record->consts.sprint_speed) {
                --BREATH(work)->wait[i];
            }
        }
        /* @hack spelling: the empty arm for an animation at frame 1 or on,
         * which puts the waits' address in R3 (0x0c90ea52 to 0x0c90ea58); as
         * one test of frame < 1.0F, R2 takes it and 3 of 221 words differ */
        else if (record->anim.frame >= 1.0F) {
        } else {
            BREATH(work)->wait[i] = BREATH_STEPS;
            params = &icecap_breath;
            player = player_work[i];
            switch (PLAYER_VARS(player)->character) {
            case CHARACTER_TAILS:
                speed.x = 0.03F;
                speed.y = 0.0F;
                speed.z = 0.0F;
                pos.x = 1.5F;
                pos.y = record->consts.eye_height;
                pos.z = 0.0F;
                params->growth = 0.0025F;
                break;
            case CHARACTER_BIG:
                speed.x = 0.06F;
                speed.y = 0.0F;
                speed.z = 0.0F;
                pos.x = 2.8F;
                pos.y = record->consts.eye_height - 1.0F;
                pos.z = 0.0F;
                params->growth = 0.004F;
                break;
            case CHARACTER_SONIC:
            default:
                speed.x = 0.02F;
                speed.y = 0.0F;
                speed.z = 0.0F;
                pos.x = record->weld_point4.x + (2.0F * record->weld_axis4.x);
                pos.y = record->weld_point4.y + (2.0F * record->weld_axis4.y) +
                        record->consts.center_y;
                pos.z = record->weld_point4.z + (2.0F * record->weld_axis4.z);
                params->growth = 0.003F;
                break;
            }
            player_turn_to_world(player, &speed);
            player_turn_to_world(player, &pos);
            njAddVector(&pos, &player->pos);
            params->pos = pos;
            params->speed = speed;
            snow_create_from(params);
        }
    }
}

/* make each of act 3's land entries solid only while blocks holds one of its
 * land blocks. an entry of size WORLD_HUGE is left as it is */
static void icecap_land_set_solid(Sint32 blocks)
{
    Sint32 flags;
    struct land_entry *entry = icecap_act3_land.entries;
    int count = icecap_act3_land.count;

    for (--count; count >= 0; --count, ++entry) {
        flags = entry->flags;
        if (flags & WORLD_SIZE) {
            if (entry->blocks & blocks) {
                entry->flags = flags | WORLD_SOLID;
            } else {
                entry->flags = flags & ~WORLD_SOLID;
            }
        }
    }
}

void icecap_step(struct task *task)
{
    static const NJS_POINT3 icecap_look_point = {343.0F, 663.0F, 38.0F};
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_POINT3 *center;

    switch (work->mode) {
    case ICECAP_START:
        icecap_act_start(task);
        break;
    case ICECAP_ACT0:
        icecap_act0_step(task);
        break;
    case ICECAP_ACT1:
        icecap_act1_step(task);
        break;
    case ICECAP_BOARD_WAIT:
        if (work->flags & ICECAP_BOARD_TAKEN) {
            work->flags = 0;
            work->mode = ICECAP_BOARD_WALK;
            pad_disable_player(0);
            player_clear_speed(player_motion[0], player_get(0));
        }
        break;
    case ICECAP_BOARD_WALK:
        player_event_run(0, 81.37F, -66.29F);
        task_create(TASK_WORK, 3, icecap_gate_combo_step);
        task_create(TASK_WORK, 3, snow_slide_step);
        pos.x = 0.0F;
        pos.y = 10000.0F;
        pos.z = 0.0F;
        lens_create(&pos);
        work->mode = ICECAP_BOARD_STOP;
        break;
    case ICECAP_BOARD_STOP:
        if (player_work[0]->event->action == EVENT_ACTION_DONE) {
            work->mode = ICECAP_BOARD_LOOK;
            player_set_heading(0, 0x3000);
            player_event_stop(0);
            VARS(work)->look_steps = 0;
        }
        vibration_start(0, 5, 15, 0);
        break;
    case ICECAP_BOARD_LOOK:
        if (VARS(work)->look_steps++ >= OPENING_STEPS) {
            VARS(work)->look_steps = 0;
            work->mode = ICECAP_BOARD_GO;
            player_event_run(0, 15.75F, -79.57F);
            camera_restore();
            camera_override_view.target.x = 15.0F;
            camera_override_view.target.y = 259.0F;
            camera_override_view.target.z = -79.0F;
            camera_override_view.pos.x = -30.0F;
            camera_override_view.pos.y = 216.0F;
            camera_override_view.pos.z = -104.0F;
            camera_override(camera_step_fixed, CAMERA_ADJUST_NORMAL,
                            CAMERA_LOOK_AT);
            camera_set_perspective(VIEW_NARROW);
            snow_slide_release();
        } else if (VARS(work)->look_steps == OPENING_LOOK) {
            pos = icecap_look_point;
            player_look_at_point(0, &pos, 240);
            camera_override_view.target.x = player_work[0]->pos.x;
            camera_override_view.target.y = player_work[0]->pos.y + 7.5F;
            camera_override_view.target.z = player_work[0]->pos.z;
            camera_override_view.pos.x = 100.0F;
            camera_override_view.pos.y = 385.0F;
            camera_override_view.pos.z = -54.0F;
            camera_override(camera_step_fixed, CAMERA_ADJUST_NORMAL,
                            CAMERA_LOOK_AT);
        } else if (VARS(work)->look_steps == OPENING_VOICE) {
            player_mouth_set_script(0, 8);
            sound_play(SE_BOARD_OPENING, 0, 0, 0);
            stream_request_music(MUSIC_ICECAP3);
        } else if (VARS(work)->look_steps == OPENING_FAR) {
            camera_override_view.target.x = 371.0F;
            camera_override_view.target.y = 592.0F;
            camera_override_view.target.z = 36.0F;
            camera_override_view.pos.x = 73.0F;
            camera_override_view.pos.y = 402.0F;
            camera_override_view.pos.z = -74.0F;
        }
        vibration_start(0, 5, 15, 0);
        break;
    case ICECAP_BOARD_GO:
        if (player_work[0]->event->action == EVENT_ACTION_DONE) {
            work->mode = ICECAP_ACT2;
            player_event_end(0);
            pad_enable_player(0);
            player_set_mode(0, PLAYER_MODE_BOARD);
            camera_override(camera_step_board, CAMERA_ADJUST_THREE1,
                            CAMERA_LOOK_AT);
        }
        vibration_start(0, 5, 15, 0);
        break;
    case ICECAP_ACT2:
        icecap_act2_step(task);
        break;
    case ICECAP_END:
        break;
    case ICECAP_ACT3:
        if (fishing_flags & FISHING_CAMERA) {
            center = &camera_work->pos;
        } else {
            center = &player_work[0]->pos;
        }
        if (center->y > ACT3_HIGH) {
            if (center->x < ACT3_SIDE) {
                world_land_blocks = 0x62;
            } else {
                world_land_blocks = 0x78;
            }
        } else if (center->y < ACT3_LOW) {
            if (center->x < ACT3_LOW_SIDE) {
                world_land_blocks = 0x06;
            } else {
                world_land_blocks = 0x1c;
            }
        } else {
            if (center->x < ACT3_SIDE) {
                world_land_blocks = 0x06;
            } else {
                world_land_blocks = 0x18;
            }
        }
        icecap_land_set_solid(world_land_blocks);
        break;
    case ICECAP_IDLE:
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        icecap_idle(task);
        break;
    }
}

static void icecap_slide_release_destroy(struct task *task)
{
    snow_slide_release();
}

/* release the snow slides as the task ends, on its second step.
 * snow_slide_step clears the release as it starts, so the release has to
 * come after that */
static void icecap_slide_release_step(struct task *task)
{
    task->destructor = icecap_slide_release_destroy;
    task_kill(task);
}

/* ask for the music in work->mode once work->tick counts down to 0, then end
 * the task */
static void icecap_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    if (--work->tick <= 0) {
        stream_reset();
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* start the act's music, held sound and tasks, and the players' breath. for
 * Sonic or Tails it loads both characters' board motions, whatever the act */
static void icecap_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *music;
    struct task_work *made;
    NJS_POINT3 pos;
    Uint8 act;

    music = task_create(TASK_WORK, 0, icecap_music_wait_step)->work;
    music->tick = MUSIC_DELAY;
    if (get_character() == CHARACTER_SONIC ||
        get_character() == CHARACTER_TAILS) {
        loader_read_prs("s_sbmot.PRS", SONIC_BOARD_AREA);
        loader_read_prs("m_sbmot.PRS", TAILS_BOARD_AREA);
    }
    made = task_create(TASK_WORK, 3, icecap_wait_step)->work;
    /* nothing reads this mode: icecap_wait_step reads only the timer */
    made->mode = ICECAP_IDLE;
    made->timer = 10;
    task_create(TASK_WORK, 2, icecap_breath_step);
    act = stage_get_stage_act();
    if (act == 1) {
        if (get_character() != CHARACTER_BIG) {
            made =
                task_create(TASK_WORK | TASK_FORCE, 3, icecap_act12_join_step)
                    ->work;
            made->pos.x = 1075.0F;
            made->pos.y = 336.0F;
            made->pos.z = 336.0F;
        }
        work->mode = ICECAP_ACT1;
        stream_reset();
        music->mode = MUSIC_ICECAP2;
        sound_play_held(SE_ACT1_HELD, -1, 8, 0);
        return;
    }
    /* @hack redundant: the else after the return, which loads
     * camera_set_perspective's address into R3 at 0x0c90f22c; without it, R2
     * takes it and 2 of 186 words differ */
    else {
        if (act == 2) {
            task_create(0, 0, icecap_board_start_step);
            camera_set_perspective(VIEW_NARROW);
            task_create(TASK_WORK, 3, icecap_gate_combo_step);
            task_create(TASK_WORK, 3, snow_slide_step);
            made =
                task_create(TASK_WORK | TASK_FORCE, 3, icecap_act23_join_step)
                    ->work;
            made->pos.x = 410.0F;
            made->pos.y = 365.0F;
            made->pos.z = -105.0F;
            task_create(0, 2, icecap_slide_release_step);
            pos.x = 0.0F;
            pos.y = 10000.0F;
            pos.z = 0.0F;
            lens_create(&pos);
            camera_override(camera_step_board, CAMERA_ADJUST_THREE1,
                            CAMERA_LOOK_AT);
            work->mode = ICECAP_ACT2;
            stream_reset();
            music->mode = MUSIC_ICECAP3;
            sound_play_held(SE_ACT0_HELD, -1, 8, 0);
            return;
        }
        if (act == 0) {
            work->mode = ICECAP_ACT0;
            music->mode = MUSIC_ICECAP1;
            sound_play_held(SE_ACT0_HELD, -1, 8, 0);
        } else {
            work->mode = ICECAP_ACT3;
            music->mode = MUSIC_ICECAP2;
            sound_play_held(SE_ACT1_HELD, -1, 8, 0);
        }
    }
}

static void icecap_act0_step(struct task *task)
{
    static const NJS_POINT3 icecap_act0_exit = {1082.0F, 336.0F, 356.0F};
    struct task_work *work = task->work;
    NJS_POINT3 exit;
    struct task_work *player;

    exit = icecap_act0_exit;
    if (njDistanceP2P(&exit, &player_work[0]->pos) < ACT0_EXIT_NEAR) {
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        work->mode = ICECAP_ACT1;
        stream_reset();
        stream_request_music(MUSIC_ICECAP2);
        sound_play_held(SE_ACT1_HELD, -1, 8, 0);
    }
    if ((player = player_work[0]) != NULL) {
        if (player->pos.y < ACT0_FLOOR &&
            !(player_get(0)->effects & PLAYER_FALLING)) {
            player_fall(0);
            work->mode = ICECAP_FALLEN;
        }
    }
}

static void icecap_act1_step(struct task *task)
{
    static const NJS_POINT3 icecap_act1_exit = {481.0F, 366.0F, -202.0F};
    struct task_work *work = task->work;
    struct task_work *board;
    NJS_POINT3 exit;

    exit = icecap_act1_exit;
    if (njDistanceP2P(&exit, &player_work[0]->pos) < ACT1_EXIT_NEAR) {
        stream_destroy();
        world_change_act(1);
        placement_change_act(1);
        camera_change_act(1);
        stage_change_act(1);
        work->mode = ICECAP_BOARD_WAIT;
        sound_play_held(SE_ACT0_HELD, -1, 8, 0);
        board =
            task_create(TASK_MOTION | TASK_WORK, 2, sonic_board_start)->work;
        board->pos.x = 16.44F;
        board->pos.y = 267.85F;
        board->pos.z = -79.45F;
        board->ang.x = 0;
        board->ang.y = 0;
        board->ang.z = 0;
        board->flags |= BOARD_STILL;
        stream_reset();
    }
}

static void icecap_board_boost_step(struct task *task)
{
    struct player *record = player_get(0);
    struct player *partner = player_get(1);

    if (record->speed.x < BOOST_SPEED) {
        record->speed.x = BOOST_SPEED;
    }
    if (partner != NULL) {
        if (partner->speed.x < BOOST_SPEED) {
            partner->speed.x = BOOST_SPEED;
        }
    }
    if (((struct icecap_boost *)task->extra)->steps++ > BOOST_STEPS) {
        task_kill(task);
    }
}

static void icecap_act2_step(struct task *task)
{
    struct player *record = player_get(0);
    NJS_POINT3 point;
    NJS_VECTOR speed;
    Angle ang[3];
    Float fast;
    Angle view;
    NJS_POINT3 *pos;

    if (player_work[0]->flags & PLAYER_ON_LAND) {
        point.x = 132.0F;
        point.y = 375.0F;
        point.z = -42.0F;
        if (njDistanceP2P(&player_work[0]->pos, &point) < JUMP_NEAR) {
            speed.x = 4.0F;
            speed.y = 0.4F;
            speed.z = 0.0F;
            ang[0] = ang[2] = 0;
            ang[1] = 0xb200;
            /* @hack type: the array's address taken whole and cast, which
             * has SHC store the angles at R15's offsets and form the address
             * for the call alone (0x0c90f5c6, 0x0c90f5da); by its plain
             * name, the address goes to R14 and 107 of 135 words differ */
            player_send_flying(0, &speed, (const Angle *)&ang, 60);
            task_create(TASK_EXTRA, 2, icecap_board_boost_step);
        }
    }
    pos = &player_work[0]->pos;
    point.x = -3798.0F;
    point.y = -23289.0F;
    point.z = -5526.0F;
    if (njDistanceP2P(pos, &point) <= ACT2_END_NEAR) {
        player_set_mode(0, PLAYER_MODE_RESET);
        camera_restore();
        task->work->mode = ICECAP_END;
    }
    fast = record->speed.x;
    if (fast >= BOARD_FAST) {
        fast = 1.0F;
    } else {
        fast *= 1.0F / BOARD_FAST;
    }
    view = VIEW_NARROW + (int)(njSqrt(fast) * VIEW_WIDER);
    camera_set_perspective(
        angle_approach(camera_get_perspective(), view, VIEW_TURN));
}

static void icecap_board_start_step(struct task *task)
{
    player_set_mode(0, PLAYER_MODE_BOARD);
    if (get_character() == CHARACTER_SONIC) {
        task_create(TASK_MOTION | TASK_WORK, 2, sonic_board_start);
    }
    if (get_character() == CHARACTER_TAILS) {
        task_create(TASK_MOTION | TASK_WORK, 2, tails_board_start);
    }
    task_kill(task);
}

static void icecap_idle() /* NOLINT(clang-diagnostic-strict-prototypes) */
{
}

void icecap_sky_step(struct task *task)
{
    task->step = icecap_sky_run;
    task->draw = icecap_sky_draw;
    task->destructor = icecap_sky_destroy;
}

/* end the sky task off Ice Cap. once the act changes, a drawing sky goes to
 * its reset. the reset returns it to mode 0, act 0's start. a sky at a start
 * goes to the new act's start */
static void icecap_sky_run(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage = stage_get_stage_act();
    Uint16 act = (Uint8)stage;

    stage &= STAGE_MASK;
    if (stage != STAGE_ACT(STAGE_ICECAP, 0)) {
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
    icecap_sky_draw(task);
}

/* acts 0 and 2 draw the sky's three objects about the camera, unfogged. act 2
 * turns its third by 0xc000, and draws it only while player 0 is shallower
 * than FOG_CLOSING. acts 0, 1 and 3 step the light, the task's child */
static void icecap_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float near;
    /* @hack spelling: -1.0F through a local, which puts it first in the
     * pool, at 0x0c90f8a8; as the constant at each use, it comes after the
     * fog functions' addresses and 19 of 427 words differ */
    Float down;
    /* @hack spelling: njSetFogColor called through a local, which keeps its
     * address in R9 (0x0c90f87a); called directly, 171 of 427 words
     * differ */
    void (*set_fog_color)(Uint32 color);
    /* @hack spelling: njSetFogTable called through a local, which keeps its
     * address in R8 and njSetFogColor's in R9 (0x0c90f87a, 0x0c90f87c);
     * called directly, the two trade, the registers after them shift, and 23
     * of 427 words differ */
    void (*set_fog_table)(NJS_FOG_TABLE fog);

    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    down = -1.0F;
    set_fog_table = njSetFogTable;
    set_fog_color = njSetFogColor;
    switch (work->mode) {
    case SKY_MODE(0, SKY_START):
        njSetBackColor(0xcccccc, 0xdddddd, 0xeeeeee);
        texture_load_pvm("bg_icecap", &icecap_sky_texlist);
        njSetDepthQueue(-1700.0F, -1900.0F);
        njClipZ(-3.0F, -1700.0F);
        /* @hack redundant: a test of set_fog_color that never runs, which keeps
         * njSetFogColor's address in R9, njSetFogTable's in R8 and
         * njGenerateFogTable3's in R11 (0x0c90f86e to 0x0c90f87c); without it,
         * they move and 19 of 427 words differ */
        if (0) {
            if (set_fog_color) {
            }
        }
        /* @hack redundant: a test of draw_tree's address that never runs, which
         * keeps it in R10, with njSetFogColor's in R9 and njSetFogTable's in R8
         * (0x0c90f87a to 0x0c90f87e); without it, the three rotate and 19 of
         * 427 words differ */
        if (0) {
            /* NOLINTNEXTLINE(clang-diagnostic-pointer-bool-conversion) */
            if (draw_tree) {
            }
        }
        set_fog_color(0xffffffff);
        njGenerateFogTable3(icecap_fog_table, -800.0F, -1400.0F);
        set_fog_table(icecap_fog_table);
        njFogEnable();
        njSetLightIntensity(&dir_light, 1.0F, 1.0F, 0.36F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 1.0F);
        njSetLightDirection(&dir_light, 0.0F, down, 0.0F);
        light_create(task, &icecap_sky_light, &icecap_light_spec);
        work->mode = SKY_MODE(0, SKY_DRAW);
        /* falls through, so the sky draws on the step it starts */
    case SKY_MODE(0, SKY_DRAW):
        task_step_children(task);
        njFogDisable();
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        njSetTexture(&icecap_sky_texlist);
        draw_tree(&icecap_sky_object_0);
        draw_tree(&icecap_sky_object_1);
        draw_tree(&icecap_sky_object_2);
        njPopMatrix(1);
        set_fog_table(icecap_fog_table);
        njFogEnable();
        break;
    case SKY_MODE(0, SKY_RESET):
        icecap_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(1, SKY_START):
        njSetBackColor(0x001040, 0x401074, 0x807474);
        set_fog_color(0x60);
        njSetLightIntensity(&dir_light, 1.0F, 1.0F, 0.22F);
        njSetLightColor(&dir_light, 0.59F, 0.75F, 0.77F);
        njSetLightDirection(&dir_light, 0.0F, down, 0.0F);
        light_create(task, &icecap_sky_light, &icecap_light_spec);
        work->mode = SKY_MODE(1, SKY_DRAW);
        /* falls through, so the light, the task's child, steps from the step
         * it is made */
    case SKY_MODE(1, SKY_DRAW):
        task_step_children(task);
        break;
    case SKY_MODE(1, SKY_RESET):
        icecap_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(2, SKY_START):
        njSetBackColor(0xcccccc, 0xdddddd, 0xeeeeee);
        texture_load_pvm("bg_icecap", &icecap_sky_texlist);
        njClipZ(-3.0F, -1600.0F);
        set_fog_color(0xffffffff);
        njGenerateFogTable3(icecap_fog_table, -1200.0F, -1600.0F);
        set_fog_table(icecap_fog_table);
        njFogEnable();
        njSetLightIntensity(&dir_light, 1.0F, 1.0F, 0.68F);
        njSetLightColor(&dir_light, 1.0F, 1.0F, 1.0F);
        njSetLightDirection(&dir_light, 1.0F, 0.0F, 0.0F);
        work->mode = SKY_MODE(2, SKY_DRAW);
        /* falls through, so the sky draws on the step it starts */
    case SKY_MODE(2, SKY_DRAW):
        njFogDisable();
        if (player_work[0]->pos.y > -FOG_CLOSING ||
            player_work[0]->pos.y < -FOG_OPENED) {
            njPushMatrix(NULL);
            njTranslateV(NULL, &camera_work->pos);
            njSetTexture(&icecap_sky_texlist);
            draw_tree(&icecap_sky_object_0);
            draw_tree(&icecap_sky_object_1);
            if (player_work[0]->pos.y > -FOG_CLOSING) {
                njRotateY(NULL, 0xc000);
                draw_tree(&icecap_sky_object_2);
            }
            njPopMatrix(1);
        }
        if (player_work[0]->pos.y < -FOG_CLOSING &&
            player_work[0]->pos.y > -FOG_CLOSED) {
            /* @hack redundant: a test of player_work[0] that never runs, which
             * with the next one and the jump below keeps 1/3 in FR5 and the
             * second arm's pos.y in FR6 (0x0c90fbca, 0x0c90fbf2); without it,
             * the two trade and 5 of 427 words differ */
            if (0) {
                if (player_work[0]) {
                }
            }
            /* @hack redundant: the same test again, which with the one above
             * keeps 1/3 in FR5 and the second arm's pos.y in FR6 (0x0c90fbca,
             * 0x0c90fbf2); without it, the two trade and 5 of 427 words
             * differ */
            if (0) {
                if (player_work[0]) {
                }
            }
            /* @hack redundant: a goto to the next statement, which loads
             * player_work[0] into R2 at 0x0c90fbde; without it, R3 takes it
             * and 2 of 427 words differ */
            goto upper;
        upper:
            near = ((-player_work[0]->pos.y - FOG_CLOSING) * FOG_SLOPE) -
                   FOG_OPEN_NEAR;
            njGenerateFogTable3(icecap_fog_table, near, near + FOG_BAND);
        } else if (player_work[0]->pos.y < -FOG_OPENING &&
                   player_work[0]->pos.y > -FOG_OPENED) {
            near = ((-player_work[0]->pos.y - FOG_OPENED) * FOG_SLOPE) -
                   FOG_OPEN_NEAR;
            njGenerateFogTable3(icecap_fog_table, near, near + FOG_BAND);
        } else if (player_work[0]->pos.y >= -FOG_CLOSING) {
            njGenerateFogTable3(icecap_fog_table, -FOG_OPEN_NEAR,
                                -(FOG_OPEN_NEAR + FOG_BAND));
        } else if (player_work[0]->pos.y <= -FOG_OPENED) {
            njGenerateFogTable3(icecap_fog_table, -FOG_OPEN_NEAR,
                                -(FOG_OPEN_NEAR + FOG_BAND));
        } else {
            njGenerateFogTable3(icecap_fog_table, -FOG_CLOSED_NEAR,
                                -(FOG_CLOSED_NEAR + FOG_BAND));
        }
        set_fog_table(icecap_fog_table);
        njFogEnable();
        break;
    case SKY_MODE(2, SKY_RESET):
        icecap_sky_destroy(task);
        work->mode = 0;
        break;
    case SKY_MODE(3, SKY_START):
        njSetBackColor(0, 0, 0);
        set_fog_color(0x60);
        njSetLightIntensity(&dir_light, 1.0F, 1.0F, 0.22F);
        njSetLightColor(&dir_light, 0.59F, 0.75F, 0.77F);
        njSetLightDirection(&dir_light, 0.0F, down, 0.0F);
        light_create(task, &icecap_sky_light, &icecap_light_spec);
        work->mode = SKY_MODE(3, SKY_DRAW);
        /* falls through, so the light, the task's child, steps from the step
         * it is made */
    case SKY_MODE(3, SKY_DRAW):
        task_step_children(task);
        break;
    case SKY_MODE(3, SKY_RESET):
        icecap_sky_destroy(task);
        work->mode = 0;
        break;
    default:
        /* @bug memory: a mode past 11 would hand the task itself to heap_free
         * (JSR at 0x0c90fd66). tasks come from task_init's pool, not from
         * heap_alloc. Ice Cap's four acts give modes 0 to 11. icecap_sky_run
         * sets a mode past them only for an act of 4 or more */
        heap_free(task);
        break;
    }
    draw_control_restore();
}

/* only act 2's sky, drawing or resetting, turns the fog off */
static void icecap_sky_destroy(struct task *task)
{
    /* @hack redundant: the empty cases, for acts 0, 1 and 3 and each act's
     * start, with which SHC jumps through a table of 12 offsets (BRAF at
     * 0x0c90fdf2); with act 2's cases alone, icecap.c's code runs 128 bytes
     * short and 17 of 20 words differ */
    switch (task->work->mode) {
    case SKY_MODE(0, SKY_DRAW):
    case SKY_MODE(0, SKY_RESET):
        break;
    case SKY_MODE(1, SKY_DRAW):
    case SKY_MODE(1, SKY_RESET):
    case SKY_MODE(3, SKY_DRAW):
    case SKY_MODE(3, SKY_RESET):
        break;
    case SKY_MODE(2, SKY_DRAW):
    case SKY_MODE(2, SKY_RESET):
        njFogDisable();
        break;
    case SKY_MODE(0, SKY_START):
    case SKY_MODE(1, SKY_START):
    case SKY_MODE(2, SKY_START):
    case SKY_MODE(3, SKY_START):
    default:
        break;
    }
    /* @hack redundant: a return where the function ends, which gives the
     * switch's empty cases an RTS each (0x0c90fe10, 0x0c90fe14); without it,
     * they branch to the function's end, and 2 of 20 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}
