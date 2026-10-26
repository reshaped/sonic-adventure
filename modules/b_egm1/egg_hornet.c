/* the Egg Hornet. it comes in as the camera swings round to it, then flies,
 * fires and dives. three hits beat it, and it wrecks. Tikal's hint names its
 * weak point once, from HINT_TIME steps after the boss starts. the hint
 * needs the stage to have counted a miss, a retry or a restart since it
 * loaded. it never comes once the boss has been hit */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/barrier.h"
#include "b_egm1/camera.h"
#include "b_egm1/egg_hornet.h"
#include "b_egm1/empty.h"
#include "b_egm1/flight.h"
#include "b_egm1/ground.h"
#include "b_egm1/jets.h"
#include "b_egm1/shapes.h"
#include "b_egm1/texlists.h"
#include "b_egm1/util.h"
#include "b_egm1/wreck.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "event/script.h"
#include "hud/life_gauge.h"
#include "object/tikal.h"
#include "player/player.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 52
#include "b_egm1/egg_hornet/text.inc"

/* the Egg Mobile's model, which the boss draws once it has burst */
extern NJS_OBJECT eggmobile_object;

#define SE_HUM 457
#define SE_HIT 463
#define VOICE_HURT 174
#define VOICE_ONE_LEFT 178
#define VOICE_HINT 434

#define HEALTH 3
/* the steps after a hit in which another hit costs no health. the boss
 * flickers through them */
#define HURT_STEPS 60
/* the steps from the boss's start before the hint can show */
#define HINT_TIME 5400
/* the height below which player 0 has fallen from the arena */
#define FALL_Y 90.0F
/* the y of Eggman's seat, from the boss's origin */
#define SEAT_Y (-0.7300005F)
/* the x of the Egg Mobile, from the boss's origin, once the boss has burst */
#define EGGMOBILE_X 38.16F

/* the hint task's steps left to wait, an unsigned count in vars.l[0] */
#define HINT_WAIT(work) (*(Uint32 *)&(work)->vars.l[0])
/* the steps left of the boss's hurt, from HURT_STEPS down to 0 */
#define HURT_LEFT(work) ((work)->vars.l[1])

static const struct script_page hint_pages[2] = {
    {TEXT_1, 250},
    {NULL, 0},
};
/* player 0's speed as a hit knocks it back, along player 0's own axes.
 * player_turn_to_world turns it to the world's */
static const NJS_VECTOR recoil = {-2.0F, 2.0F, 0.0F};

/* nothing reads it */
static Uint8 dat_0c905ba0[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0, 0, 0xff};
static Angle tilt_speed = NJM_DEG_ANG(24); /* tilt_phase's change each step */
/* sway's change each step, once it has run a quarter turn */
static Angle sway_speed = NJM_DEG_ANG(4);
/* the most the rock tilts the boss */
static Angle tilt_scale = NJM_DEG_ANG(12);
static Sint32 hum_volume = 120;
/* nothing reads them */
static Uint32 dat_0c905bb8 = 0x3f800000;
static Uint32 dat_0c905bbc = 0x5b0;
/* set while Tikal's hint shows. egg_hornet_voice holds Eggman's lines back
 * while it is set */
static Sint32 hint_showing = 0;
/* set once the hint has shown or the boss has been hit. init leaves it as it
 * is */
static Sint32 hint_spent = 0;

void egg_hornet_voice(int voice)
{
    if (hint_showing == 0) {
        stream_request_voice(voice);
    }
}

static void hint_wait_step(struct task *task)
{
    if (--HINT_WAIT(task->work) <= 0) {
        task_destroy(task);
    }
}

static void hint_wait_destroy(struct task *task)
{
    hint_showing = 0;
}

/* show Tikal's hint, with a task that holds Eggman's lines back while its
 * page shows. with no task to be had, no hint shows */
static void hint_show(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 1, hint_wait_step)) != NULL) {
        task->destructor = hint_wait_destroy;
        HINT_WAIT(task->work) = hint_pages[0].steps;
        hint_showing = 1;
        tikal_hint_create((struct script_page *)hint_pages, VOICE_HINT);
    }
}

static void egg_hornet_hint(struct task *task, struct task_work *work,
                            struct egg_hornet *record)
{
    if (stage_get_retries() > 0 && TASK_TIMER(work) >= HINT_TIME &&
        !record->hinted && !hint_spent) {
        hint_spent = 1;
        record->hinted = TRUE;
        hint_show();
    }
}

/* play the boss's hum at its position, until it wrecks */
static void egg_hornet_hum(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;

    if (work->mode == EGG_HORNET_WRECK) {
        record->humming = FALSE;
    }
    /* @hack spelling: humming compared with TRUE, which SHC tests with
     * CMP/EQ #1 at 0x0c900284. humming is only ever set to TRUE or FALSE, so a
     * plain test of it is the same; as one, 8 of 36 words differ */
    if (record->humming == TRUE) {
        sound_play_at_for(SE_HUM, (int)task, 1, hum_volume, work->pos.x,
                          work->pos.y, work->pos.z, 1);
    }
}

static void egg_hornet_step(struct task *task);
static void egg_hornet_draw(struct task *task);
static void egg_hornet_destroy(struct task *task);
static void egg_hornet_init(struct task *task);

void egg_hornet_place(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;

    njTranslateV(NULL, &work->pos);
    if (egg_hornet_diving(task)) {
        njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    } else {
        matrix_turn_zxy(NULL, &work->ang);
    }
    TURN(njRotateX, record->tilt);
    TURN(njRotateZ, record->bank);
    njScale(NULL, 2.0F, 2.0F, 2.0F);
}

void egg_hornet_create(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK | TASK_MOTION, 1, egg_hornet_step)) !=
        NULL) {
        /* @bug memory: init leaves task->extra null when motion_create fails,
         * and the step and the draw still run. the step loads the record
         * (0x0c900738) and passes it to motion_step unchecked (0x0c900764) */
        egg_hornet_init(task);
        task->step = egg_hornet_step;
        task->draw = egg_hornet_draw;
        task->destructor = egg_hornet_destroy;
    }
}

/* set up the record, the boss's start and its jets, as egg_hornet_create
 * says. with no room for the record, do nothing */
static void egg_hornet_init(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion = motion_create(work, sizeof(struct egg_hornet));
    struct egg_hornet *record;
    struct task_work *player;
    /* @hack type: owner as an array of one, which SHC keeps 4 bytes into the
     * frame, above player (MOV.L R10,@(4,R15) at 0x0c900490); as a struct task
     * pointer passed by its address, 49 of 126 words differ */
    struct task *owner[1];

    if (motion != NULL) {
        task->extra = motion;
        record = task->extra;
        boss_work = work;
        work->pos.x = 832.0F;
        work->pos.y = 185.0F;
        work->pos.z = 1000.0F;
        work->mode = EGG_HORNET_WAIT;
        HURT_LEFT(work) = 0;
        motion->wanted = ACTION_IDLE;
        motion->current = ACTION_IDLE;
        motion->actions = egg_hornet_actions;
        record->health = HEALTH;
        record->volley_limit = 1;
        record->sway = 0;
        record->tilt = record->tilt_phase = 0;
        record->bank = record->bank_amount = 0;
        record->burst = FALSE;
        record->humming = TRUE;
        record->fell = FALSE;
        egg_hornet_shapes_init(task);
        egg_hornet_barrier_init(&record->barrier);
        egg_hornet_flight_reset(task, work);
        player = player_work[0];
        work->ang.y = egg_hornet_face_player(work);
        player->ang.y = -work->ang.y;
        work->ang.z = 0;
        work->ang.x = 0;
        owner[0] = task;
        egg_hornet_jets_create(owner);
        camera_start_swing(work, 25.0F, 55.0F, 180);
        camera_override_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
        egg_hornet_camera_snap();
        motion_step(work, motion);
        record->hinted = FALSE;
        hint_showing = 0;
        work->timer = 0;
        record->timer = 0;
    }
}

/* start the fight once the camera has swung round. turn the boss's flying
 * shapes on, show its life gauge, and let the pads drive the game again */
static void egg_hornet_wait(struct task_work *work)
{
    if (camera_swing_done) {
        work->mode = EGG_HORNET_FIGHT;
        egg_hornet_shapes_enable(work, SHAPES_FLYING);
        life_gauge_create(0, 0, HEALTH);
        camera_override_mode(CAMERA_MODE_CHAOS_STD, CAMERA_ADJUST_NONE);
        pad_enable();
    }
}

/* take a hit on the boss, which knocks player 0 back. a hit while the boss
 * isn't hurt costs it a health, and adds a volley to each round. losing its
 * last health beats it */
static void egg_hornet_hurt(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;
    NJS_VECTOR speed;
    struct task_work *player;

    /* @hack redundant: a test of player 0's COLLISION_DAMAGED whose result
     * nothing reads (TST R4,R0 at 0x0c9005b4); without it, 98 of 102 words
     * differ */
    if (player_work[0]->flags & COLLISION_DAMAGED) {
    }
    if (work->flags & COLLISION_DAMAGED) {
        if (record->health > 0) {
            speed = recoil;
            player = player_work[0];
            work->flags &= ~COLLISION_DAMAGED;
            player_turn_to_world(player, &speed);
            player_set_speed(0, speed.x, speed.y, speed.z);
            hint_spent = 1;
            if (!HURT_LEFT(work)) {
                sound_play(SE_HIT, 0, 0, 0);
                HURT_LEFT(work) = HURT_STEPS;
                record->volley_limit++;
                if (--record->health <= 0) {
                    egg_hornet_jets_set_mode(JETS_OFF);
                    wreck_smoke_create(&work->pos);
                    work->mode = EGG_HORNET_BEATEN;
                }
                if (record->health == 1) {
                    egg_hornet_voice(VOICE_ONE_LEFT);
                } else {
                    egg_hornet_voice(VOICE_HURT);
                }
            }
        }
    }
}

/* down player 0 the first time it falls below FALL_Y, from the fight on */
static void egg_hornet_down_fallen_player(struct task_work *work,
                                          struct egg_hornet *record)
{
    struct task_work *player;

    if (work->mode >= EGG_HORNET_FIGHT && !record->fell) {
        player = player_work[0];
        if (player->pos.y < FALL_Y) {
            player_down(0);
            record->fell = TRUE;
        }
    }
}

static void egg_hornet_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;

    if (TASK_TIMER(work) < 0xffff) {
        work->timer++;
    }
    egg_hornet_camera_follow(task);
    if (HURT_LEFT(work)) {
        HURT_LEFT(work)--;
    }
    /* while hurt, the boss draws in two steps of every four */
    if ((HURT_LEFT(work) & 3) <= 1) {
        egg_hornet_draw(task);
    }
    motion_step(work, &record->motion);
    switch (work->mode) {
    case EGG_HORNET_WAIT:
        pad_disable();
        egg_hornet_wait(work);
        break;
    case EGG_HORNET_FIGHT:
        egg_hornet_flight_step(task, work);
        egg_hornet_hurt(task);
        egg_hornet_hint(task, work, record);
        break;
    case EGG_HORNET_BEATEN:
        egg_hornet_flight_step(task, work);
        if (record->state == FLIGHT_START) {
            wreck_reset(task);
            work->mode = EGG_HORNET_WRECK;
        }
        break;
    case EGG_HORNET_WRECK:
        wreck_step(task, work);
        if (record->state == WRECK_DONE &&
            (player_get(0)->last_flags & PLAYER_ON_GROUND)) {
            work->mode = EGG_HORNET_CLEAR;
        }
        break;
    case EGG_HORNET_CLEAR:
        stage_clear();
        work->mode = EGG_HORNET_WRECKED;
        /* fall through: the wreck goes on bobbing this step */
    case EGG_HORNET_WRECKED:
        wreck_step(task, work);
        break;
    /* @hack redundant: a case for mode 6, which no code sets, whose test SHC
     * keeps (CMP/EQ #6,R0 at 0x0c900782); without it, 106 of 212 words
     * differ */
    case 6:
        break;
    }
    egg_hornet_barrier_set_owner(&record->barrier, task);
    egg_hornet_barrier_step(&record->barrier);
    egg_hornet_shadow(task);
    life_gauge_health = record->health;
    egg_hornet_shapes_turn_yzx(work, egg_hornet_diving(task));
    collision_register(work);
    /* sway runs its first quarter turn in two steps, and goes back to 0
     * past half a turn */
    if (record->sway > 0) {
        if (record->sway < 0x4000) {
            record->sway += 0x2001;
        } else {
            record->sway += sway_speed;
        }
        if (record->sway > 0x8000) {
            record->sway = 0;
        }
    }
    /* the boss rocks about x, as far as sway's sine lets it. a negative tilt
     * goes four tenths as far */
    record->tilt_phase += tilt_speed;
    record->tilt =
        (Angle)(njSin(record->tilt_phase) * njSin(record->sway) * tilt_scale);
    if (record->tilt < 0) {
        record->tilt = record->tilt * 4 / 10;
    }
    egg_hornet_hum(task);
    egg_hornet_down_fallen_player(work, record);
    fun_0c905020();
}

/* draw the boss as its motion poses it, then Eggman in his seat. Eggman
 * plays his own action for the boss's turns and its shudder, and sits still
 * through the rest */
static void egg_hornet_draw_whole(struct task *task)
{
    struct egg_hornet *record = task->extra;
    NJS_ACTION *action;

    if (record->motion.state != MOTION_BLENDING) {
        action = record->motion.actions[record->motion.current].action;
    } else {
        action = record->motion.blend;
    }
    njSetTexture(&egg_hornet_texlist);
    draw_action(action, record->motion.frame);
    njSetTexture(&egm1eggman_texlist);
    njTranslate(NULL, 0.0F, SEAT_Y, 0.0F);
    switch (record->motion.current) {
    case ACTION_TURN_1:
        draw_action(&eggman_turn_1_action, record->motion.frame);
        break;
    case ACTION_TURN_2:
        draw_action(&eggman_turn_2_action, record->motion.frame);
        break;
    case ACTION_SHUDDER:
        draw_action(&eggman_shudder_action, record->motion.frame);
        break;
    default:
        draw_tree(&eggman_object);
        break;
    }
}

/* draw what the burst leaves: the Egg Mobile, and Eggman sitting still */
static void egg_hornet_draw_burst(struct task *task)
{
    njPushMatrix(NULL);
    njTranslate(NULL, EGGMOBILE_X, 0.0F, 0.0F);
    njSetTexture(&egg_hornet_texlist);
    draw_tree(&eggmobile_object);
    njPopMatrix(1);
    njSetTexture(&egm1eggman_texlist);
    njTranslate(NULL, 0.0F, SEAT_Y, 0.0F);
    draw_tree(&eggman_object);
}

static void egg_hornet_draw(struct task *task)
{
    struct egg_hornet *record = task->extra;

    egg_hornet_barrier_draw(&record->barrier);
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njPushMatrix(NULL);
    egg_hornet_place(task);
    if (!record->burst) {
        egg_hornet_draw_whole(task);
    } else {
        egg_hornet_draw_burst(task);
    }
    njPopMatrix(1);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_control_restore();
}

/* free the motion's blend, which task_destroy leaves, and end the jets */
static void egg_hornet_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion = task->extra;

    motion->state = MOTION_FREE;
    motion_step(work, motion);
    egg_hornet_jets_kill();
}
