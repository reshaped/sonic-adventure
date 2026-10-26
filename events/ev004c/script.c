/* EV004C, Tails' story: Amy stands up for E-102, and Tails flies her off
 * the sinking Egg Carrier */
#include <ninja.h>

#include "ev004c/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/e102.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/dust_ring.h"
#include "shared/fade.h"
#include "shared/lightning.h"
#include "shared/shapes.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 26
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 45
#define TEXT_5_LENGTH 26
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 47
#define TEXT_8_LENGTH 32
#define TEXT_9_LENGTH 33
#define TEXT_10_LENGTH 57
#define TEXT_11_LENGTH 13
#define TEXT_12_LENGTH 72
#define TEXT_13_LENGTH 2
#define TEXT_14_LENGTH 1
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 3
#define TEXT_17_LENGTH 1
#define TEXT_18_LENGTH 5
#define TEXT_19_LENGTH 5
#define TEXT_20_LENGTH 3
#define TEXT_21_LENGTH 5
#define TEXT_22_LENGTH 5
#include "ev004c/script/text.inc"

/* how far above Tails ev004c_carry holds Amy */
#define CARRY_HEIGHT 5.0F

extern NJS_ACTION sonic_run_action;
extern NJS_OBJECT tails_fly_object;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_sprint_action;
extern NJS_ACTION tails_skid_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_water_skate_action;
extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev004c_e102_action_0;
extern NJS_ACTION ev004c_e102_action_1;
extern NJS_ACTION ev004c_e102_action_2;
extern NJS_ACTION ev004c_e102_action_3;
extern NJS_ACTION ev004c_e102_action_4;
extern NJS_ACTION ev004c_tails_action_0;
extern NJS_ACTION ev004c_tails_action_1;
extern NJS_ACTION ev004c_amy_action_0;
extern NJS_ACTION ev004c_amy_action_1;
extern NJS_ACTION ev004c_amy_action_2;
extern NJS_ACTION ev004c_amy_action_3;
extern NJS_ACTION ev004c_sonic_action_0;
extern NJS_ACTION ev004c_sonic_action_1;
extern NJS_ACTION ev004c_sonic_action_2;

static struct event_path_point ev004c_tails_takeoff_points[240] = {
#include "ev004c/script/ev004c_tails_takeoff_points.inc"
};

static struct event_path ev004c_tails_takeoff = {240,
                                                 ev004c_tails_takeoff_points};

static struct event_path_point ev004c_e102_takeoff_points[200] = {
#include "ev004c/script/ev004c_e102_takeoff_points.inc"
};

static struct event_path ev004c_e102_takeoff = {200,
                                                ev004c_e102_takeoff_points};

static struct event_path_point ev004c_tails_flight_points[200] = {
#include "ev004c/script/ev004c_tails_flight_points.inc"
};

static struct event_path ev004c_tails_flight = {200,
                                                ev004c_tails_flight_points};

static struct task *ev004c_fade = NULL;
static struct task *ev004c_carry = NULL;

/* hold Amy, player 2, at Tails' pos plus the task's pos, and at his ang.y.
 * Tails is player 0. nothing checks that either still exists, so both must
 * outlive the task */
static void ev004c_carry_step(struct task *task)
{
    struct task_work *tails = event_get_player(0)->work;
    struct task_work *amy = event_get_player(2)->work;
    struct task_work *work = task->work;

    amy->pos.x = tails->pos.x + work->pos.x;
    amy->pos.y = tails->pos.y + work->pos.y;
    amy->pos.z = tails->pos.z + work->pos.z;
    amy->ang.y = tails->ang.y;
}

/* create ev004c_carry. it keeps Amy's ang.y less Tails' in its own ang.y,
 * which ev004c_carry_step never reads */
static void ev004c_carry_create(void)
{
    struct task_work *tails;
    struct task_work *amy;
    struct task_work *work;

    ev004c_carry = task_create(TASK_WORK, 5, ev004c_carry_step);
    tails = event_get_player(0)->work;
    amy = event_get_player(2)->work;
    work = ev004c_carry->work;
    work->pos.x = 0.0F;
    work->pos.y = CARRY_HEIGHT;
    work->pos.z = 0.0F;
    work->ang.y = amy->ang.y - tails->ang.y;
}

void ev004c_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_001B.mlt");
        event_sound_init(5);
        event_set_pos(event_get_player(0), -2.6F, 1525.718F, 3270.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_set_flags(event_get_player(0), EVENT_GROUND);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_create_player(4, e102_step, event_get_player(0)->work->pos.x,
                            event_get_player(0)->work->pos.y,
                            event_get_player(0)->work->pos.z + 122.0F,
                            event_get_player(0)->work->ang.x,
                            -0x4000 - event_get_player(0)->work->ang.y,
                            event_get_player(0)->work->ang.z);
        event_create_player(3, sonic_step, -91.2F, 1533.6F, 3342.4F, 0, 0x1cde,
                            0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_ang(event_get_player(4), event_get_player(0)->work->ang.x,
                      -0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_play_action(event_get_player(0), &ev004c_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(4), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(3), &ev004c_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(
            2, amy_step, event_get_player(4)->work->pos.x - 120.0F,
            event_get_player(4)->work->pos.y,
            event_get_player(4)->work->pos.z + 10.0F,
            event_get_player(4)->work->ang.x, -event_get_player(4)->work->ang.y,
            event_get_player(4)->work->ang.z);
        event_lightning_create(event_get_player(4)->work->pos.x,
                               event_get_player(4)->work->pos.y,
                               event_get_player(4)->work->pos.z, 4);
        stream_request_music(MUSIC_EGCARER1);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe000, 0x7f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.5F, 1555.8F, 3243.22F);
        event_wait(1);
        event_set_ang(event_get_player(4), event_get_player(0)->work->ang.x,
                      -0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(9);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 85, 0x890, 0x7f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 85, -2.256F, 1533.934F,
                             3233.24F);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev004c_e102_action_1,
                          &e102_texlist, 1.1F, EVENT_REPEAT, 0);
        event_sound_start(1, 0x4c8, 1800);
        event_wait(50);
        event_play_action(event_get_player(4), &ev004c_e102_action_2,
                          &e102_texlist, 1.1F, 0, 16);
        event_wait(20);
        event_sound_start(1, 0x4c8, 1800);
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf100, 0xb00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 10.655F, 1543.9F, 3403.8F);
        event_play_action(event_get_player(4), &ev004c_e102_action_3,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_sound_play(0x4c9, 0, 0, 0);
        event_wait(5);
        event_run_to_speed(event_get_player(2), -1.6547F, 1526.0F, 3373.7F,
                           1.0F, 0.0F);
        event_wait(80);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_run_action,
                          &tails_texlist, 2.5F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -1.338F, 1526.0F, 3340.0F, 1.5F,
                           0.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0xfca8, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -1.563F, 1533.0F,
                             3379.3777F);
        event_run_wait(event_get_player(2));
        event_turn_by(event_get_player(2), 0, 0x8200, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 4.0F,
                           0.0F);
        event_wait(1);
        event_play_action(event_get_player(2), &ev004c_amy_action_0,
                          &amy_texlist, 0.8F, 0, 0);
        event_sound_stop(1);
        event_lightning_kill();
        event_voice_play(0x50b);
        event_message_show(TEXT_1);
        stream_stop_music();
        event_wait(15);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x5a8, 0x7100, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.35F, 1528.176F, 3329.58F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 4.99F, 1528.73F, 3332.9F);
        event_play_action(event_get_player(0), &tails_skid_action,
                          &tails_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004c_tails_action_0,
                          &tails_texlist, 0.8F, EVENT_REPEAT, 8);
        event_wait(15);
        event_voice_play(0x50c);
        event_talk_start(event_get_player(0), TEXT_13);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(2), &ev004c_amy_action_0,
                          &amy_texlist, 0.8F, 0, 16);
        event_voice_play(0x50e);
        event_talk_start(event_get_player(2), TEXT_14);
        event_message_show(TEXT_3);
        event_wait_motions(event_get_player(2));
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(2), &ev004c_amy_action_2,
                          &amy_texlist, 1.2F, EVENT_REPEAT, 16);
        event_voice_play(0x50f);
        event_talk_start(event_get_player(2), TEXT_15);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_voice_play(0x510);
        event_talk_start(event_get_player(2), TEXT_16);
        event_message_show(TEXT_5);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x8a8, 0xf600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.74F, 1526.3F, 3353.79F);
        event_voice_play(0x511);
        event_talk_start(event_get_player(0), TEXT_17);
        event_message_show(TEXT_6);
        event_wait(10);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 70, event_get_player(0),
                                  0.0F, 7.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, 5.0F, 1529.77F, 3345.88F);
        event_set_pos(event_get_player(3), -44.0F, 1528.342F, 3344.2742F);
        event_wait(55);
        event_message_close();
        event_wait(15);
        event_play_action(event_get_player(0), &tails_stand_action,
                          &tails_texlist, 0.5F, EVENT_REPEAT, 24);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F,
                           -55.0F, 0.0F);
        event_wait(50);
        event_voice_play(0x512);
        event_talk_start(event_get_player(0), TEXT_18);
        event_message_show(TEXT_7);
        event_wait(5);
        event_look_clear(event_get_player(0));
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_voice_play(0x513);
        event_talk_start(event_get_player(0), TEXT_19);
        event_message_show(TEXT_8);
        event_camera_look_clear();
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_sound_start(0, 0x4c7, 1800);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xdb7, 0x22de, 0);
        event_wait(3);
        vibration_start(0, 4, 50, 10);
        stream_request_music(MUSIC_EVTBGM01);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1b7, 0x22de, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x11b7, 0x22de, 0);
        event_wait(3);
        event_play_action(event_get_player(0), &ev004c_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1b7, 0x22de, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x11b7, 0x22de, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1b7, 0x22de, 0);
        event_wait(3);
        event_play_action(event_get_player(4), &ev004c_e102_action_4,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(2), &ev004c_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -13.45F, 1536.9F, 3325.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf6b7, 0x8fde, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf0b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfdb7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf0b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfdb7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf0b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfdb7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf0b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfbb7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf2b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfbb7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf2b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xfbb7, 0x8fde, 0);
        event_wait(3);
        event_sound_fade_volume(0, -30, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf4b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf8b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf4b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf8b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf4b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf8b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf4b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf8b7, 0x8fde, 0);
        event_wait(8);
        event_sound_fade_volume(0, -70, 21);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf5b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf3b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf5b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf3b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf5b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf3b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0xf5b7, 0x8fde, 0);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -8.272F, 1527.888F, 3351.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xab7, 0xe6de, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 15, 0x271c);
        event_voice_play(0x514);
        event_talk_start(event_get_player(0), TEXT_20);
        event_message_show(TEXT_9);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_look_at_task(event_get_player(0), event_get_player(3), 0.0F, 3.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 7.92F, 1527.7758F, 3348.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x7b7, 0x2ede, 0x200);
        event_wait(1);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_flags(event_get_player(2), EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(2), -6.235F, 1531.0F, 3371.6F);
        event_set_ang(event_get_player(2), 0, 0x9800, 0);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(4), &ev004c_e102_action_3,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(3), &ev004c_sonic_action_2,
                          &sonic_texlist, 1.2F, EVENT_REPEAT, 8);
        event_wait(20);
        event_voice_play(0x515);
        event_talk_start(event_get_player(3), TEXT_21);
        event_message_show(TEXT_10);
        event_wait(70);
        event_clear_motions(event_get_player(3));
        event_run_to_speed(event_get_player(3), -37.34F, 1525.7F, 3349.16F,
                           0.8F, 4.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 11.29F, 1531.14F, 3331.29F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0xffb7, 0x5dde, 0x200);
        event_run_wait(event_get_player(3));
        event_voice_play(0x44c);
        event_talk_start(event_get_player(2), TEXT_17);
        event_message_show(TEXT_11);
        event_wait(1);
        event_voice_wait();
        event_play_action(event_get_player(3), &ev004c_sonic_action_0,
                          &sonic_texlist, 0.9F, 0, 8);
        event_play_action(event_get_player(3), &ev004c_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_voice_play(0x516);
        event_talk_start(event_get_player(3), TEXT_22);
        event_message_show(TEXT_12);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_play_action(event_get_player(4), &ev004c_e102_action_0,
                          &e102_texlist, 1.0F, 0, 0);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 4.0F,
                           0.0F);
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 4.0F,
                           0.0F);
        event_set_ang(event_get_player(2), 0, 0x79de, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 7.92F, 1527.7758F, 3348.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x7b7, 0x2ede, 0x200);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(3));
        event_play_action(event_get_player(3), &sonic_run_action,
                          &sonic_texlist, 1.7F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(0), &tails_sprint_action,
                          &tails_texlist, 1.3F, EVENT_REPEAT, 8);
        event_wait(8);
        event_run_to_speed(event_get_player(3), -44.6F, 1530.46F, 3026.0F, 1.6F,
                           0.0F);
        event_run_to_speed(event_get_player(0), -2.0754F, 1530.46F, 3358.678F,
                           1.3F, 0.0F);
        event_look_clear(event_get_player(0));
        event_run_wait(event_get_player(0));
        event_set_pos(event_get_player(0), -3.82F, 1525.7F, 3319.0F);
        event_set_pos(event_get_player(2), -1.95F, 1531.0F, 3368.0F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -6.0F, 1552.56F, 3298.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfbb7, 0x80de, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, event_get_player(0),
                                  0.0F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -1.227F, 1533.0F, 3350.3F);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(4), EVENT_IGNORE_GROUND);
        event_dust_ring_create(event_get_player(4)->work->pos.x,
                               event_get_player(4)->work->pos.y,
                               event_get_player(4)->work->pos.z);
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(2));
        event_play_path(event_get_player(0), &ev004c_tails_takeoff, 0.6F,
                        EVENT_FROM_HERE);
        event_play_path(event_get_player(0), &ev004c_tails_flight, 1.3F,
                        EVENT_FROM_HERE);
        event_wait(40);
        /* @hack spelling: the motion as (&ev004c_tails_action_1)->motion. SHC
         * then loads the action's address and reads the motion 4 bytes past
         * it, as the image does (MOV.L @(4,R1),R3 at 0x0cb812ea). as
         * ev004c_tails_action_1.motion, SHC loads the field's own address
         * from the pool, ev004c_script is 4 bytes shorter than the image's,
         * and 337 of its 2111 words differ */
        event_play_motion(event_get_player(0), &tails_fly_object,
                          (&ev004c_tails_action_1)->motion, &tails_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev004c_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(8);
        TAILS_FLAGS(player_work[0]) |= TAILS_TAIL_SPIN;
        ev004c_carry_create();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -52.5267F, 1622.05F,
                             3364.56F);
        event_wait(20);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, -93.03F, 1635.66F, 3375.8F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30, event_get_player(0),
                                  0.0F, 4.0F, 0.0F, 0);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 37, -142.16F, 1628.347F,
                             3386.73F);
        event_wait(25);
        event_play_action(event_get_player(4), &e102_water_skate_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(11);
        event_dust_ring_set_mode(EVENT_DUST_RING_ON);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -142.65F, 1623.64F,
                             3364.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0xede6, 0xb53a, 0);
        event_wait(15);
        event_play_path(event_get_player(4), &ev004c_e102_takeoff, 0.7F,
                        EVENT_FROM_HERE);
        event_wait(35);
        event_dust_ring_set_mode(EVENT_DUST_RING_WAIT);
        event_wait(75);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x382e);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -215.96F, 1616.454F,
                             3405.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfce6, 0x2b3a, 0x100);
        event_kill_player(4);
        event_sound_stop(0);
        event_wait(140);
        ev004c_fade = event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(60);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_STILL;
        if (ev004c_carry != NULL) {
            task_kill(ev004c_carry);
            ev004c_carry = NULL;
        }
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_clear_paths(event_get_player(0));
        event_clear_paths(event_get_player(2));
        event_clear_paths(event_get_player(4));
        event_set_pos(event_get_player(0), -2.6F, 1525.718F, 3270.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(3);
        event_kill_player(4);
        event_dust_ring_kill();
        event_lightning_kill();
        /* @hack spelling: the kill and the clear as one statement. SHC then
         * reads ev004c_fade again to pass it to task_kill, as the image does
         * (MOV.L @R3,R4 at 0x0cb8160c). as two statements, SHC passes
         * task_kill the value it tested, and 10 of 2111 words differ */
        if (ev004c_fade != NULL) {
            task_kill(ev004c_fade), ev004c_fade = NULL;
        }
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev004c_textures[] = {
    {"AMY", &amy_texlist},
    {"E102", &e102_texlist},
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
