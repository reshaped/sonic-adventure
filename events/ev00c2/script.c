/* EV00C2, E-102's story: Amy stops E-102's fight with Sonic, and Tails flies
 * her off the falling Egg Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_lift.h"
#include "ev00c2/bird_textures_1.h"
#include "ev00c2/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/tails.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/dust_ring.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 23
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 52
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 30
#define TEXT_7_LENGTH 52
#define TEXT_8_LENGTH 56
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 26
#define TEXT_11_LENGTH 44
#define TEXT_12_LENGTH 56
#define TEXT_13_LENGTH 43
#define TEXT_14_LENGTH 67
#define TEXT_15_LENGTH 28
#define TEXT_16_LENGTH 41
#define TEXT_17_LENGTH 40
#define TEXT_18_LENGTH 51
#define TEXT_19_LENGTH 17
#define TEXT_20_LENGTH 42
#define TEXT_21_LENGTH 3
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 4
#define TEXT_24_LENGTH 2
#define TEXT_25_LENGTH 6
#define TEXT_26_LENGTH 114
#define TEXT_27_LENGTH 4
#define TEXT_28_LENGTH 2
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 7
#define TEXT_31_LENGTH 5
#define TEXT_32_LENGTH 5
#define TEXT_33_LENGTH 4
#define TEXT_34_LENGTH 3
#define TEXT_35_LENGTH 3
#include "ev00c2/script/text.inc"

/* @hack redundant: n behind a test of a float constant. SHC folds the test,
 * but handles n as a value it computes, not as a constant; without it, 1130
 * of ev00c2_script's 3708 words differ */
/* NOLINTNEXTLINE(clang-diagnostic-literal-conversion) */
#define LATE(n) (2.0F ? (n) : (n))

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_run_action;
extern NJS_ACTION tails_sprint_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_water_skate_action;
extern NJS_ACTION amy_trot_action;
extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev00c2_sonic_action_0;
extern NJS_ACTION ev00c2_sonic_action_1;
extern NJS_ACTION ev00c2_sonic_action_2;
extern NJS_ACTION ev00c2_sonic_action_3;
extern NJS_ACTION ev00c2_sonic_action_4;
extern NJS_ACTION ev00c2_sonic_action_5;
extern NJS_ACTION ev00c2_tails_action_0;
extern NJS_ACTION ev00c2_tails_action_1;
extern NJS_ACTION ev00c2_amy_action_0;
extern NJS_ACTION ev00c2_amy_action_1;
extern NJS_ACTION ev00c2_amy_action_2;
extern NJS_ACTION ev00c2_amy_action_3;
extern NJS_ACTION ev00c2_amy_action_4;
extern NJS_ACTION ev00c2_amy_action_5;
extern NJS_ACTION ev00c2_amy_action_6;
extern NJS_ACTION ev00c2_e102_action_0;
extern NJS_ACTION ev00c2_e102_action_1;
extern NJS_ACTION ev00c2_bird_action;

static struct event_path_point ev00c2_tails_takeoff_points[240] = {
#include "ev00c2/script/ev00c2_tails_takeoff_points.inc"
};

static struct event_path ev00c2_tails_takeoff = {240,
                                                 ev00c2_tails_takeoff_points};

static struct event_path_point ev00c2_e102_takeoff_points[200] = {
#include "ev00c2/script/ev00c2_e102_takeoff_points.inc"
};

static struct event_path ev00c2_e102_takeoff = {200,
                                                ev00c2_e102_takeoff_points};

static struct event_path_point ev00c2_tails_flight_points[200] = {
#include "ev00c2/script/ev00c2_tails_flight_points.inc"
};

static struct event_path ev00c2_tails_flight = {200,
                                                ev00c2_tails_flight_points};

static struct task *ev00c2_amy_actor = NULL;
static struct task *ev00c2_bird = NULL;
static struct task *ev00c2_fade = NULL;
static struct task *ev00c2_carry = NULL;

/* hold Amy's actor at Tails' pos plus the task's pos, and at his ang.y
 * plus the task's ang.y. Tails is player 3. nothing checks that the actor
 * or Tails still exists, so both must outlive the task */
static void ev00c2_carry_step(struct task *task)
{
    struct task_work *tails = event_get_player(3)->work;
    struct task_work *amy = ev00c2_amy_actor->work;
    struct task_work *work = task->work;

    amy->pos.x = tails->pos.x + work->pos.x;
    amy->pos.y = tails->pos.y + work->pos.y;
    amy->pos.z = tails->pos.z + work->pos.z;
    amy->ang.y = tails->ang.y + work->ang.y;
}

/* make the task that holds Amy's actor 5 above Tails. its ang.y is 0,
 * player 0's ang.y less itself, so the actor takes Tails' ang.y */
static void ev00c2_carry_create(void)
{
    struct task_work *e102;
    struct task_work *e102_again;
    struct task_work *work;

    ev00c2_carry = task_create(TASK_WORK, 5, ev00c2_carry_step);
    e102 = event_get_player(0)->work;
    e102_again = event_get_player(0)->work;
    work = ev00c2_carry->work;
    work->pos.x = 0.0F;
    work->pos.y = 5.0F;
    work->pos.z = 0.0F;
    work->ang.y = e102_again->ang.y - e102->ang.y;
}

void ev00c2_script(int phase)
{
    /* @hack spelling: &ev00c2_e102_action_0 through a local, which R9 holds
     * from 0x0cb80506 for all five plays; named at each play, the function runs
     * 4 bytes short and 3023 of 3708 words differ */
    NJS_ACTION *e102_action;
    /* @hack spelling: Amy's y, 1525.9, through a local. FR12 holds it from
     * 0x0cb80540 for all five uses, two of them the camera's; as the literal at
     * each use, the function runs 4 bytes short and 3192 of 3708 words
     * differ */
    Float amy_y;
    Angle pitch;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_001B.mlt");
        event_sound_init(5);
        event_reset_player(0);
        /* the screen stays clear until the last 100 steps. there
         * event_fade_set_speed makes the speed 0.016, which turns it black
         * within 63 steps */
        ev00c2_fade =
            event_fade_create(EVENT_FADE_STILL, 0.0F, 0.0F, 0.0F, 0.0F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_wait(1);
        event_create_player(4, sonic_step, -3.1F, 1525.7F, 3314.34F, 0, 0, 0);
        event_wait(1);
        event_set_pos(event_get_player(4), -3.1F, 1525.7F, 3314.34F);
        event_set_ang(event_get_player(4), 0, 0, 0);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev00c2_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), -3.1F, 1525.7F, 3354.34F);
        event_set_ang(event_get_player(0), event_get_player(4)->work->ang.x,
                      -0x4000 - event_get_player(4)->work->ang.y,
                      event_get_player(4)->work->ang.z);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_create_player(2, amy_step,
                            event_get_player(0)->work->pos.x - 40.0F,
                            event_get_player(0)->work->pos.y,
                            event_get_player(0)->work->pos.z - 66.0F,
                            event_get_player(0)->work->ang.x,
                            0xd700 - event_get_player(0)->work->ang.y,
                            event_get_player(0)->work->ang.z);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00c2_amy_action_2,
                          &amy_texlist, 0.8F, EVENT_REPEAT, 16);
        event_place_actor(&ev00c2_bird, -13.1F, 1025.7F, 3314.34F, 0, 0, 0);
        event_wait(1);
        event_set_flags(ev00c2_bird, EVENT_IGNORE_GROUND);
        event_create_player(3, tails_step,
                            event_get_player(4)->work->pos.x - 15.0F,
                            event_get_player(4)->work->pos.y,
                            event_get_player(4)->work->pos.z - 17.0F,
                            event_get_player(4)->work->ang.x,
                            0x5000 - event_get_player(4)->work->ang.y,
                            event_get_player(4)->work->ang.z);
        event_wait(1);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev00c2_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        stream_request_music(MUSIC_THEE102);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe00, 0x1100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -17.8F, 1527.6F, 3384.8F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, 16.5F, 1527.6F, 3368.9F);
        event_wait(80);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xe00, 0x1700, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 5.8F, 1536.8F, 3343.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x86d, 0x5d54, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 31.7F, 1531.4F, 3346.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0xa6d, 0x3850, 0xfe00);
        event_wait(60);
        e102_action = &ev00c2_e102_action_0;
        event_play_action(event_get_player(0), e102_action, &e102_texlist, 0.9F,
                          EVENT_REPEAT, 30);
        event_wait(140);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           10.0F, 30.0F);
        amy_y = 1525.9F;
        event_set_pos(event_get_player(2), -39.7F, amy_y, 3313.8F);
        event_set_ang(event_get_player(2), 0, 0x400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -41.7F, 1531.13F, 3322.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x958, 0x11ff, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00c2_amy_action_2,
                          &amy_texlist, 0.8F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, -37.26F, 1531.1F, 3320.0F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_21);
        event_voice_play(0x6e9);
        event_message_wait(60, TEXT_1);
        event_talk_stop(event_get_player(2));
        stream_stop_music();
        event_set_pos(event_get_player(0), -3.1F, 1525.7F, 3354.34F);
        event_set_pos(event_get_player(4), -3.1F, 1525.7F, 3314.34F);
        event_set_ang(event_get_player(4), 0, 0, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           16.0F, 0.0F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_trot_action, &amy_texlist,
                          1.8F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(2), -3.55F, 1525.7F, 3339.02F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -21.4F, 1535.01F, 3301.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x400, 0x7700, 0x700);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 6.5F, 1533.3F, 3310.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x100, 0x7500, 0x700);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        stream_stop_music();
        event_run_wait(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(2), event_get_player(4)->work->ang.x,
                      0x4000 - event_get_player(4)->work->ang.y,
                      event_get_player(4)->work->ang.z);
        event_voice_play(0x6ea);
        event_message_show(TEXT_2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, 2.5F, 1532.71F, 3324.17F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 130, 0x700, 0x7700, 0x700);
        event_wait(100);
        event_message_close();
        stream_request_music(MUSIC_EVTBGM00);
        event_wait(35);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 15.86F, 1532.9F, 3342.16F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x600, 0x4e00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 20.51F, 1532.1F, 3340.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x600, 0x4e00, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00c2_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_22);
        event_voice_play(0x6eb);
        event_message_show(TEXT_3);
        event_wait(20);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 48);
        event_wait(80);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.4F, 1536.3F, 3344.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe200, 0x100, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -3.4F, 1536.1F, 3344.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xe200, 0x100, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_23);
        event_voice_play(0x6ec);
        event_message_show(TEXT_4);
        event_wait(80);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 7.55F, 1532.7F, 3329.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x600, 0x6a00, 0);
        event_play_action(event_get_player(0), e102_action, &e102_texlist, 1.5F,
                          EVENT_REPEAT, 0);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), e102_action, &e102_texlist, 2.1F,
                          EVENT_REPEAT, 0);
        event_wait(70);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), e102_action, &e102_texlist, 1.5F,
                          EVENT_REPEAT, 0);
        event_wait(70);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), e102_action, &e102_texlist, 0.7F,
                          EVENT_REPEAT, 0);
        event_wait(90);
        event_play_action(event_get_player(0), &ev00c2_e102_action_1,
                          &e102_texlist, 0.7F, EVENT_REPEAT, 20);
        event_wait(60);
        event_set_pos(event_get_player(3),
                      event_get_player(4)->work->pos.x - 40.0F,
                      event_get_player(4)->work->pos.y,
                      event_get_player(4)->work->pos.z - 10.0F);
        event_set_ang(event_get_player(3), event_get_player(4)->work->ang.x,
                      0x7000 - event_get_player(4)->work->ang.y,
                      event_get_player(4)->work->ang.z);
        event_look_clear(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_set_pos(event_get_player(4), -3.1F, 1525.7F, 3320.34F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, 5.7F, 1529.5F, 3312.53F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x600, 0x6f00, 0);
        event_play_action(event_get_player(4), &ev00c2_sonic_action_3,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_wait(40);
        event_talk_stop(event_get_player(4));
        /* E-102's mouth starts on TEXT_24. the event_talk_stop calls before
         * and after it name Sonic, player 4. no call in the script stops
         * E-102's mouth */
        event_talk_start(event_get_player(0), TEXT_24);
        event_voice_play(0x6ed);
        event_message_wait(50, TEXT_5);
        event_talk_stop(event_get_player(4));
        event_message_close();
        event_play_action(event_get_player(4), &ev00c2_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(4));
        event_talk_start(event_get_player(4), TEXT_25);
        event_wait(1);
        event_voice_play(0x6ee);
        event_message_wait(60, TEXT_6);
        event_talk_stop(event_get_player(4));
        event_wait(60);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_set_pos(event_get_player(4), event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 25.0F);
        event_set_ang(event_get_player(4), 0, 0, 0);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev00c2_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        /* @hack redundant: an empty block that never runs. it leaves 140 in
         * R8 from 0x0cb8051c to 0x0cb80c36; without it, R8 holds 0x700 and
         * then 0x600 over that stretch, and 795 of 3708 words differ */
        if (0) {
        }
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x2889);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 26.4F, 1542.8F, 3328.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf000, 0x6241, 0x100);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 19.89F, 1543.4F, 3318.63F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_sound_start(0, 0x4c7, 1800);
        stream_stop_music();
        event_play_action(event_get_player(4), &ev00c2_sonic_action_4,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(2), &ev00c2_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        stream_request_music(MUSIC_EGCARER1);
        vibration_start(0, 7, 50, 3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfd00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfd00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfd00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfd00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfd00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfd00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfd00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_play_action(event_get_player(4), &ev00c2_sonic_action_4,
                          &sonic_texlist, 0.25F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev00c2_amy_action_1,
                          &amy_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfa00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfa00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfa00, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_sound_fade_volume(0, -70, 21);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf500, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf600, 0x6241, 0x500);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -31.56F, 1531.1F, 3317.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe80, 0x1f00, 0);
        event_play_action(event_get_player(0), &ev00c2_e102_action_1,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 30);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x2889);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, -32.7F, 1531.0F, 3315.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_26);
        event_voice_play(0x44a);
        event_message_show(TEXT_7);
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfd80, 0x1f00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe80, 0x1f00, 0);
        event_wait(3);
        event_play_action(event_get_player(4), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_message_close();
        event_wait(5);
        event_set_pos(event_get_player(2), -3.55F, 1525.7F, 3339.02F);
        event_set_ang(event_get_player(2), 0, 0xa000, 0);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(4), 0.0F, 4.0F,
                           0.0F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -45.9F, 1528.76F, 3287.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x380, 0x9900, 0x300);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -48.39F, 1529.0F, 3288.78F);
        event_play_action(event_get_player(4), &ev00c2_sonic_action_1,
                          &sonic_texlist, 0.2F, 0, 4);
        event_wait(20);
        event_talk_stop(event_get_player(4));
        event_talk_start(event_get_player(4), TEXT_27);
        event_voice_play(0x44b);
        event_message_wait(70, TEXT_8);
        event_message_close();
        event_talk_stop(event_get_player(4));
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_28);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(4), 0.0F, 4.0F,
                           0.0F);
        event_voice_play(0x5c0);
        event_message_wait(40, TEXT_9);
        event_talk_stop(event_get_player(2));
        event_look_clear(event_get_player(4));
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(4), event_get_player(2), 0.0F, 4.0F,
                           0.0F);
        event_look_at_task(event_get_player(2), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 11.4F, 1546.36F, 3359.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xee80, 0x2200, 0x300);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 13.5F, 1542.7F, 3360.65F);
        event_play_action(event_get_player(4), &ev00c2_sonic_action_0,
                          &sonic_texlist, 0.4F, 0, 8);
        event_play_action(event_get_player(4), &ev00c2_sonic_action_2,
                          &sonic_texlist, 0.4F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(4));
        event_talk_start(event_get_player(4), TEXT_29);
        event_voice_play(0x44d);
        event_message_wait(60, TEXT_10);
        event_message_wait(60, TEXT_11);
        event_message_close();
        event_talk_stop(event_get_player(4));
        event_look_clear(event_get_player(4));
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &sonic_run_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 10);
        event_run_to(event_get_player(4),
                     event_get_player(4)->work->pos.x - 80.0F,
                     event_get_player(4)->work->pos.y,
                     event_get_player(4)->work->pos.z - 240.0F);
        event_look_at_task(event_get_player(2), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0xf780, 0x1c00, 0);
        event_wait(85);
        event_set_pos(event_get_player(2), -3.55F, 1525.7F, 3339.02F);
        event_set_ang(event_get_player(2), 0, 0xa000, 0);
        event_look_clear(event_get_player(2));
        event_play_action(ev00c2_bird, &ev00c2_bird_action,
                          &ev00c2_bird_texlist_1, 0.7F, EVENT_REPEAT, 0);
        event_set_pos(ev00c2_bird, event_get_player(2)->work->pos.x + 1.0F,
                      event_get_player(2)->work->pos.y + 1.0F,
                      event_get_player(2)->work->pos.z - 0.6F);
        /* @bug logic: gives the bird angles from Amy's position, not her
         * angles, here and in the matching call below. each is her x, her y
         * plus 36864.0F (0x9000) or her z, converted to an Angle (FTRC at
         * 0x0cb81828 to 0x0cb81852 and 0x0cb8202c to 0x0cb82056, before the
         * JSRs at 0x0cb8185a and 0x0cb8205e) */
        event_set_ang(ev00c2_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 36864.0F,
                      event_get_player(2)->work->pos.z);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_30);
        event_voice_play(0x5c1);
        event_message_show(TEXT_12);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 0.4F, 1528.6F, 3327.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x800, 0x764d, 0x300);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00c2_amy_action_5,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_set_ang(event_get_player(2), 0, 0x9000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 3.4F, 1531.6F, 3328.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x800, 0x734d, 0x300);
        event_wait(100);
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 3.55F, 1532.0F, 3328.0F);
        event_wait(60);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 13.9F, 1528.7F, 3335.23F);
        /* the camera's pitch, 0xc00, which the image makes by shifting the
         * 0x300 it holds in R8 left 2 (SHLL2 at 0x0cb81994) */
        pitch = LATE(0x300) * 4;
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, pitch, 0x5800, 0);
        event_set_pos(event_get_player(2), -3.55F, amy_y, 3330.34F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_set_pos(ev00c2_bird, event_get_player(2)->work->pos.x + 3.0F,
                      event_get_player(2)->work->pos.y + 1.0F,
                      event_get_player(2)->work->pos.z + 4.3F);
        event_set_ang(ev00c2_bird, 0xf650, 0, 0);
        event_follow(ev00c2_bird, 0.0F, 0.9F, 1.5F, 200, event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00c2_amy_action_4,
                          &amy_texlist, 1.3F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), -3.55F, amy_y, 3339.34F, 1.0F,
                           0.06F);
        event_run_wait(event_get_player(2));
        event_play_action(event_get_player(2), &ev00c2_amy_action_5,
                          &amy_texlist, 1.32F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, 21.9F, amy_y, 3329.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 170, pitch, 0x5b00, 0);
        event_run_clear(event_get_player(4));
        event_clear_motions(event_get_player(4));
        event_set_pos(event_get_player(4), 1.58F, 1532.8F, 3011.0F);
        event_set_ang(event_get_player(4), 0, 0, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           16.0F, 0.0F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_wait(1);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_31);
        event_message_show(TEXT_13);
        event_voice_play(0x5c2);
        event_voice_wait();
        event_wait(1);
        event_voice_play(0x5c3);
        event_voice_wait();
        event_wait(70);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_32);
        event_voice_play(0x5c4);
        event_message_wait(100, TEXT_14);
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 19.6F, 1526.77F, 3331.66F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, pitch, 0x5b00, 0);
        event_voice_play(0x6ef);
        event_message_wait(75, TEXT_15);
        event_message_close();
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.2F, 1537.2F, 3344.86F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe300, 0x200, 0x200);
        /* @hack redundant: an empty block that never runs. SHC pops the
         * stack arguments of the calls above it where the block stands, 116
         * bytes at 0x0cb81cf2; without it, SHC keeps 0x96 in R8
         * (0x0cb804a2), the function runs 4 bytes long and 2825 of 3708
         * words differ */
        if (0) {
        }
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -3.29F, 1536.6F, 3344.11F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_33);
        event_voice_play(0x5c5);
        event_message_show(TEXT_16);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -3.23F, 1533.8F, 3345.49F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xf000, 0x200, 0x200);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_34);
        event_voice_play(0x5c6);
        event_message_wait(65, TEXT_17);
        event_voice_wait();
        event_voice_play(0x5c7);
        event_message_show(TEXT_18);
        event_wait(65);
        event_talk_stop(event_get_player(2));
        event_camera_set_ang(EVENT_EASE_SMOOTH, 25, 0xf500, 0x1600, 0);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(3), 0.0F, 4.0F,
                           0.0F);
        event_look_clear(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(2), 0.0F, 4.0F,
                           0.0F);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_35);
        event_voice_play(0x5c8);
        event_message_show(TEXT_19);
        event_wait(60);
        event_talk_stop(event_get_player(3));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -19.35F, 1528.7F, 3314.89F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xa00, 0x9200, 0);
        event_wait(30);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           16.0F, 0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -24.33F, amy_y, 3304.3F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00c2_amy_action_6,
                          &amy_texlist, 2.5F, EVENT_REPEAT, 0);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(3), 0.0F,
                           16.0F, 0.0F);
        event_run_to(event_get_player(2), -33.4F, 1530.9F, 3312.5F);
        event_wait(1);
        event_set_pos(ev00c2_bird, event_get_player(2)->work->pos.x + 1.0F,
                      event_get_player(2)->work->pos.y + 1.0F,
                      event_get_player(2)->work->pos.z);
        event_set_ang(ev00c2_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 36864.0F,
                      event_get_player(2)->work->pos.z);
        event_follow(ev00c2_bird, 1.0F, 1.0F, 0.6F, 300, event_get_player(2));
        event_run_wait(event_get_player(2));
        event_message_close();
        /* the image loads 23.1 before it calls event_get_player, and holds it
         * on the stack across the call (0x0cb8208c) */
        event_set_pos(event_get_player(0), LATE(23.1F), 1525.7F, 3354.34F);
        event_flags[FLAG_LIFT] = LIFT_LOW_NOW;
        /* from here Amy's actor stands in for her. event_set_pos below puts
         * player 2 at y 1634.7, over 100 above the actor */
        event_place_actor(&ev00c2_amy_actor, -1.95F, 1528.7F, 3370.0F, 0,
                          0x8000, 0);
        event_wait(1);
        event_clear_motions(ev00c2_amy_actor);
        event_play_action(ev00c2_amy_actor, &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_flags(ev00c2_amy_actor, EVENT_IGNORE_GROUND);
        event_set_radius(ev00c2_amy_actor, 0.0F);
        event_set_pos(ev00c2_amy_actor, -6.235F, 1531.0F, 3371.6F);
        event_set_ang(ev00c2_amy_actor, 0, 0x9800, 0);
        event_set_pos(event_get_player(2), 0.0F, 1634.7F, 3175.0F);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_follow(ev00c2_bird, -0.3F, 1.0F, 0.6F, 200, ev00c2_amy_actor);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 7.92F, 1527.7758F, 3348.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x7b7, 0x2ede, 0x200);
        event_look_clear(event_get_player(3));
        event_look_clear(event_get_player(4));
        event_play_action(event_get_player(4), &sonic_run_action,
                          &sonic_texlist, 1.6F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(3), &tails_sprint_action,
                          &tails_texlist, 1.3F, EVENT_REPEAT, 8);
        event_wait(8);
        event_run_to_speed(event_get_player(4), -44.6F, 1530.46F, 3026.0F, 1.6F,
                           0.0F);
        event_run_to_speed(event_get_player(3), -2.0754F, 1530.46F, 3358.678F,
                           1.3F, 0.0F);
        event_look_clear(event_get_player(3));
        event_run_wait(event_get_player(3));
        event_set_pos(event_get_player(3), -3.82F, 1525.7F, 3319.0F);
        event_set_pos(ev00c2_amy_actor, -1.95F, 1531.0F, 3368.0F);
        event_set_ang(ev00c2_amy_actor, 0, 0x8000, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -6.0F, 1552.56F, 3298.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfbb7, 0x80de, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, event_get_player(3),
                                  0.0F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -1.227F, 1533.0F, 3350.3F);
        event_set_flags(event_get_player(3), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_dust_ring_create(event_get_player(0)->work->pos.x,
                               event_get_player(0)->work->pos.y,
                               event_get_player(0)->work->pos.z);
        event_play_path(event_get_player(3), &ev00c2_tails_takeoff, 0.6F,
                        EVENT_FROM_HERE);
        event_play_path(event_get_player(3), &ev00c2_tails_flight, 1.0F,
                        EVENT_FROM_HERE);
        event_wait(40);
        event_play_action(event_get_player(3), &ev00c2_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00c2_amy_actor, &ev00c2_amy_action_3, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(11);
        ev00c2_carry_create();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 53.86F, 1526.25F, 3352.35F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(3),
                                  0.0F, 4.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 111, 45.86F, 1527.25F,
                             3328.17F);
        event_wait(20);
        event_camera_look_clear();
        event_wait(50);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 41, 0x107e, 0x590d, 0x500);
        event_wait(30);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(11);
        event_camera_look_clear();
        event_voice_play(0x5c9);
        event_message_show(TEXT_20);
        event_wait(50);
        event_play_action(event_get_player(0), &e102_water_skate_action,
                          &e102_texlist, 1.4F, EVENT_REPEAT, 16);
        event_wait(11);
        event_dust_ring_set_mode(EVENT_DUST_RING_ON);
        event_wait(45);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x382e);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 53.86F, 1526.25F, 3352.35F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 10.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, 51.86F, 1527.25F, 3341.58F);
        event_play_path(event_get_player(0), &ev00c2_e102_takeoff, 0.7F,
                        EVENT_FROM_HERE);
        event_wait(40);
        event_dust_ring_set_mode(EVENT_DUST_RING_WAIT);
        if (ev00c2_fade != NULL) {
            event_fade_set_speed(ev00c2_fade, 0.016F);
        }
        event_wait(100);
        event_sound_stop(0);
        break;
    case EVENT_END:
        event_move_stop_all();
        if (ev00c2_carry != NULL) {
            task_kill(ev00c2_carry);
            ev00c2_carry = NULL;
        }
        event_set_flags(event_get_player(0), EVENT_GROUND);
        event_kill_task(&ev00c2_amy_actor);
        event_clear_paths(event_get_player(3));
        event_clear_paths(event_get_player(0));
        event_kill_task(&ev00c2_bird);
        event_kill_player(2);
        event_kill_player(3);
        event_kill_player(4);
        event_dust_ring_kill();
        event_sound_stop_all();
        event_voice_stop();
        if (ev00c2_fade != NULL) {
            task_kill(ev00c2_fade);
            ev00c2_fade = NULL;
        }
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00c2_textures[] = {
    {"VER1_WING", &ev00c2_bird_texlist_1},
    {"AMY", &amy_texlist},
    {"MILES", &tails_texlist},
    {"SONIC", &sonic_texlist},
    {"VER2_WING", &ev00c2_bird_texlist_2},
    {NULL, NULL},
};
