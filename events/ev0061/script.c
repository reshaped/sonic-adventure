/* EV0061, Amy's story: a bird fleeing Zero crashes into Amy, and she vows to
 * protect it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/traffic.h"
#include "enemy/zero.h"
#include "ev0061/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/move.h"
#include "shared/shapes.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 37
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 46
#define TEXT_5_LENGTH 28
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 17
#define TEXT_8_LENGTH 32
#define TEXT_9_LENGTH 49
#define TEXT_10_LENGTH 21
#define TEXT_11_LENGTH 60
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 10
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 7
#define TEXT_21_LENGTH 7
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 1
#define TEXT_25_LENGTH 3
#define TEXT_26_LENGTH 4
#define TEXT_27_LENGTH 3
#define TEXT_28_LENGTH 54
#define TEXT_29_LENGTH 1
#define TEXT_30_LENGTH 2
#define TEXT_31_LENGTH 3
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 5
#define TEXT_34_LENGTH 1
#define TEXT_35_LENGTH 2
#define TEXT_36_LENGTH 2
#define TEXT_37_LENGTH 3
#define TEXT_38_LENGTH 3
#define TEXT_39_LENGTH 12
#include "ev0061/script/text.inc"

extern NJS_ACTION ev0061_amy_action_0;
extern NJS_ACTION ev0061_amy_action_1;
extern NJS_ACTION ev0061_amy_action_2;
extern NJS_ACTION ev0061_amy_action_3;
extern NJS_ACTION ev0061_amy_action_4;
extern NJS_ACTION ev0061_amy_action_5;
extern NJS_ACTION ev0061_amy_action_6;
extern NJS_ACTION ev0061_amy_action_7;
extern NJS_ACTION ev0061_amy_action_8;
extern NJS_ACTION ev0061_amy_action_9;
extern NJS_ACTION ev0061_amy_action_10;
extern NJS_ACTION ev0061_amy_action_11;
extern NJS_ACTION ev0061_amy_action_12;

/* @hack type: single NJS_ACTIONs declared as arrays. SHC then keeps Amy's first
 * x, 134.0, in FR13 (0x0cb80094) and the angle 0x8000 in R8 (0x0cb800a8). as
 * single objects passed by address, FR12 takes 134.0, and R8 takes
 * event_set_ang's address. ev0061_script then runs 48 bytes short, and 1983 of
 * its 2186 words differ */
extern NJS_ACTION ev0061_ver2_wing_action_0[];
extern NJS_ACTION ev0061_ver2_wing_action_1[];
extern NJS_ACTION ev0061_ver2_wing_action_2[];
extern NJS_ACTION ev0061_ver2_wing_action_3[];
extern NJS_ACTION ev0061_ver2_wing_action_4[];
extern NJS_ACTION ev0061_ver2_wing_action_5[];
extern NJS_ACTION ev0061_ver2_wing_action_6[];
extern NJS_ACTION ev0061_ver2_wing_action_7[];
extern NJS_ACTION ev0061_ver3_wing_action[];

/* the bird's texlists, defined below ev0061_textures, where the image places
 * them */
static NJS_TEXLIST ev0061_ver1_wing_texlist;
static NJS_TEXLIST ev0061_ver2_wing_texlist;
static NJS_TEXLIST ev0061_ver3_wing_texlist;

static struct event_camera_key ev0061_camera_keys[106] = {
#include "ev0061/script/ev0061_camera_keys.inc"
};

static struct event_camera_path ev0061_camera_path = {
    {0}, 105.0F, ev0061_camera_keys};

static struct task *ev0061_bird = NULL;

void ev0061_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_take_control();
        event_wait(1);
        event_camera_take();
        event_pads_off();
        event_reset_player(0);
        sound_load_file("e_0061.mlt");
        event_sound_init(5);
        event_flags[FLAG_TRAFFIC] = FALSE;
        event_set_pos(event_get_player(0), 134.0F, 0.0F, 675.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_reset_player(0);
        event_play_action(event_get_player(0), &ev0061_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_24);
        event_sound_start(0, 1223, 1800);
        event_place_actor(&ev0061_bird, -100.0F, 5.5F, -5.5F, 0, 0, 0);
        event_set_flags(ev0061_bird, EVENT_IGNORE_GROUND);
        event_create_player(6, zero_event_init, 384.7F, 5.0F, 412.0F, 0, 0x8000,
                            0);
        event_reset_player(6);
        stream_request_music(MUSIC_EVTBGM01);
        event_wait(1);
        event_set_pos(event_get_player(0), 134.0F, 0.0F, 675.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 134.0F, 34.8F, 654.776F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd800, 0x8000, 0);
        event_voice_play(1380);
        event_talk_start(event_get_player(0), TEXT_25);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_sound_play(1224, 0, 0, 0);
        event_wait(15);
        event_look_at(event_get_player(0), 132.8F, 5.5F, 662.0F);
        event_play_action(event_get_player(0), &ev0061_amy_action_0,
                          &amy_texlist, 0.3F, EVENT_REPEAT, 1);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_26);
        event_wait(3);
        event_voice_play(1381);
        event_message_show(TEXT_2);
        event_wait(17);
        event_message_close();
        event_sound_stop(0);
        event_look_at(event_get_player(0), 134.0F, 5.5F, 631.75F);
        event_sound_start(1, 1224, 1800);
        event_camera_set_path(&ev0061_camera_path, 0.9F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 100, 0x11c7);
        event_wait(24);
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_wait(50);
        event_talk_start(event_get_player(0), TEXT_27);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), 134.0F, 8.847F, 670.64F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 134.0F, 8.847F, 670.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf1db, 0x8000, 0);
        event_wait(11);
        event_white_start_hold(2, 65, 45);
        event_sound_stop(1);
        event_sound_play(1229, 0, 0, 0);
        event_voice_play(1382);
        event_message_show(TEXT_3);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_28);
        event_wait(21);
        event_set_pos(event_get_player(0), 128.0F, 0.0F, 693.1F);
        event_message_close();
        event_set_pos(ev0061_bird, 128.0F, 0.0F, 693.1F);
        event_set_ang(ev0061_bird, 0, 0x8000, 0);
        event_clear_motions(ev0061_bird);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x271c);
        event_wait(1);
        event_wait(5);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0061_amy_action_1,
                          &amy_texlist, 0.7F, 0, 8);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_0,
                          &ev0061_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 130.0F, 2.74F, 673.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff00, 0x7f00, 0);
        event_talk_start(event_get_player(0), TEXT_29);
        event_wait(30);
        event_play_action(event_get_player(0), &ev0061_amy_action_2,
                          &amy_texlist, 0.8F, 0, 0);
        event_talk_start(event_get_player(0), TEXT_30);
        event_wait(7);
        event_voice_play(1383);
        event_message_show(TEXT_4);
        event_play_action(event_get_player(0), &ev0061_amy_action_3,
                          &amy_texlist, 0.8F, EVENT_REPEAT, 0);
        event_wait(6);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 142.5F, 7.68F, 679.242F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xf400, 0x6200, 0x0500);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_31);
        event_wait(1);
        event_voice_play(1384);
        event_message_show(TEXT_5);
        event_wait(59);
        event_set_pos(event_get_player(6), 127.475F, 12.0F, 850.0F);
        event_set_ang(event_get_player(6), 0, 0x7d00, 0);
        event_wait(20);
        event_message_close();
        vibration_start(0, 4, 50, 5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf500, 0x6200, 0x0500);
        event_wait(2);
        event_look_at(event_get_player(0), 127.0F, 13.0F, 680.1F);
        event_talk_start(event_get_player(0), TEXT_32);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0, 0x6200, 0x0500);
        event_sound_play(1227, 0, 0, 0);
        stream_stop_music();
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf500, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xfe00, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf400, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xff00, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf500, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf100, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf500, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf000, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf400, 0x6200, 0x0500);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf100, 0x6200, 0x0500);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 133.38F, 3.07F, 683.36F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe00, 0x7600, 0);
        event_talk_stop(event_get_player(0));
        event_wait(5);
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &ev0061_amy_action_5,
                          &amy_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 128.8F, 9.7F, 839.856F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1b00, 0x7800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 131.1F, 0.378F, 825.35F);
        event_wait(70);
        stream_request_music(MUSIC_EGGROBO);
        event_wait(25);
        event_move(event_get_player(6), 127.475F, 12.0F, 850.0F, 130.0F, 12.0F,
                   770.0F, 30);
        event_set_ang(event_get_player(0), 0, 0xee00, 0);
        event_set_ang(ev0061_bird, 0, 0xee00, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0061_bird);
        event_play_action(event_get_player(0), &ev0061_amy_action_6,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_1,
                          &ev0061_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(6), 0.0F,
                           -2.0F, 0.0F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 126.8F, 5.5F, 701.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0600, 0xf100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 125.0F, 5.5F, 700.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0600, 0xf100, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_33);
        event_wait(20);
        event_voice_play(1385);
        event_message_show(TEXT_6);
        event_set_pos(event_get_player(6), 127.475F, 12.0F, 850.0F);
        event_set_ang(event_get_player(6), 0, 0x7d00, 0);
        event_wait(10);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 128.83F, 12.3F, 666.86F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0100, 0x7e00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 128.83F, 2.16F, 668.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0b00, 0x7e00, 0);
        event_move(event_get_player(6), 127.475F, 12.0F, 850.0F, 134.0F, 12.0F,
                   550.0F, 170);
        event_wait(10);
        event_play_action(event_get_player(0), &ev0061_amy_action_8,
                          &amy_texlist, 1.9F, 0, 12);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_3,
                          &ev0061_ver2_wing_texlist, 1.9F, 0, 12);
        event_wait(15);
        event_move(event_get_player(0), 128.0F, 0.0F, 693.1F, 128.0F, 0.0F,
                   688.0F, 17);
        event_follow(ev0061_bird, 0.0F, 0.0F, 0.0F, 17, event_get_player(0));
        event_wait(5);
        event_play_action(event_get_player(0), &ev0061_amy_action_6,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_1,
                          &ev0061_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(5);
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(6));
        event_wait(25);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_34);
        event_clear_motions(ev0061_bird);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0061_amy_action_7,
                          &amy_texlist, 0.7F, 0, 0);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_2,
                          &ev0061_ver2_wing_texlist, 0.7F, 0, 0);
        event_wait(10);
        event_voice_play(1386);
        event_message_show(TEXT_3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 10, 122.847F, 2.16F, 667.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 10, 0x0b00, 0x7700, 0);
        event_wait(9);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 119.9F, 2.16F, 667.0F);
        event_message_close();
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 123.07F, 2.95F, 709.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0500, 0x0500, 0x0100);
        event_look_clear(event_get_player(0));
        event_set_pos(ev0061_bird, 120.0F, 0.0F, 693.1F);
        event_set_pos(event_get_player(0), 120.0F, 0.0F, 693.1F);
        event_set_ang(event_get_player(0), 0, 0x7600, 0);
        event_set_ang(ev0061_bird, 0, 0x7600, 0);
        event_play_action(event_get_player(0), &ev0061_amy_action_6,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_1,
                          &ev0061_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(6));
        event_run_to(event_get_player(6), 121.0F, 12.0F, 584.0F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 125, 123.057F, 2.9457F,
                             712.52F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 125, 0x0500, 0x0500, 0x0100);
        event_play_action(event_get_player(0), &ev0061_amy_action_8,
                          &amy_texlist, 1.1F, EVENT_REPEAT, 12);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_3,
                          &ev0061_ver2_wing_texlist, 1.1F, EVENT_REPEAT, 12);
        event_move(event_get_player(0), 120.0F, 0.0F, 693.1F, 119.0F, 0.0F,
                   700.4F, 95);
        event_follow(ev0061_bird, 0.0F, 0.0F, 0.0F, 95, event_get_player(0));
        event_wait(96);
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0061_bird);
        event_play_action(event_get_player(0), &ev0061_amy_action_9,
                          &amy_texlist, 1.8F, EVENT_REPEAT, 4);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_4,
                          &ev0061_ver2_wing_texlist, 1.8F, EVENT_REPEAT, 4);
        event_wait(4);
        event_camera_follow(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_ang(ev0061_bird, 0, 0, 0);
        event_move(event_get_player(0), 119.59F, 0.0F, 696.4F, 118.8F, 0.0F,
                   790.0F, 92);
        event_follow(ev0061_bird, 0.0F, 0.0F, 0.0F, 92, event_get_player(0));
        event_run_to_speed(event_get_player(6), 121.0F, 0.0F, 710.0F, 0.7F,
                           0.7F);
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_35);
        event_voice_play(1387);
        event_message_show(TEXT_7);
        event_wait(10);
        event_look_at_task(event_get_player(0), event_get_player(6), 0.0F, 8.0F,
                           0.0F);
        event_wait(25);
        event_look_clear(event_get_player(0));
        event_wait(15);
        event_message_close();
        event_set_pos(ev0061_bird, 118.8F, 0.0F, 728.8F);
        event_set_pos(event_get_player(0), 118.8F, 0.0F, 728.8F);
        event_move(event_get_player(0), 118.8F, 0.0F, 728.8F, 118.8F, 0.0F,
                   768.0F, 40);
        event_follow(ev0061_bird, 0.0F, 0.0F, 0.0F, 40, event_get_player(0));
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 181.73F, 16.7F, 769.475F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff00, 0x4000, 0);
        event_wait(30);
        event_set_ang(ev0061_bird, 0, -0x3500, 0);
        event_run_to_speed(event_get_player(0), 72.15F, 0.0F, 770.6F, 1.3F,
                           2.0F);
        event_follow(ev0061_bird, 0.0F, 0.0F, 0.0F, 150, event_get_player(0));
        event_wait(5);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 181.73F, 16.7F, 769.475F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff00, 0x4000, 0);
        event_wait(47);
        event_set_ang(ev0061_bird, 0, 0x8a00, 0);
        event_run_to_speed(event_get_player(0), 73.0F, 0.0F, 752.7F, 1.0F,
                           1.5F);
        event_follow(ev0061_bird, 0.0F, 0.0F, 0.0F, 45, event_get_player(0));
        event_wait(75);
        event_set_pos(event_get_player(6), 125.0F, 20.0F, 700.1F);
        event_wait(5);
        event_run_to(event_get_player(6), 125.0F, 20.0F, 730.0F);
        event_set_pos(ev0061_bird, 80.0F, 1.0F, 763.0F);
        event_set_pos(event_get_player(0), 80.0F, 1.0F, 763.0F);
        event_set_ang(event_get_player(0), 0, -0x4000, 0);
        event_set_ang(ev0061_bird, 0, -0x4000, 0);
        event_play_action(event_get_player(0), &ev0061_amy_action_10,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_5,
                          &ev0061_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 43.0775F, 26.03F, 759.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf400, 0xcf00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 70.024F, 12.24F, 737.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0500, 0xc000, 0);
        event_wait(30);
        event_run_to(event_get_player(6), 115.0F, 20.0F, 740.1F);
        event_wait(70);
        event_run_to_speed(event_get_player(6), 125.0F, 0.0F, 900.0F, 0.4F,
                           0.4F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 71.556F, 5.9F, 762.66F);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 70.3F, 8.26F, 758.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xff00, 0xa700, 0);
        event_wait(110);
        stream_stop_music();
        event_kill_player(6);
        event_wait(20);
        event_look_at(event_get_player(0), 83.27F, 5.35F, 752.146F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_36);
        event_voice_play(1388);
        event_message_wait(60, TEXT_8);
        event_wait(40);
        event_play_action(event_get_player(0), &ev0061_amy_action_11,
                          &amy_texlist, 1.0F, 0, 0);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_6,
                          &ev0061_ver2_wing_texlist, 1.0F, 0, 0);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 70.67F, 4.21F, 755.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0x0200, 0xa300, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_37);
        event_voice_play(1389);
        event_message_show(TEXT_9);
        event_play_action(event_get_player(0), &ev0061_amy_action_12,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0061_bird, ev0061_ver2_wing_action_7,
                          &ev0061_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(120);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_38);
        event_voice_play(1390);
        event_message_wait(60, TEXT_10);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 132.5F, 3.53F, 751.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0x6700, 0);
        event_set_pos(ev0061_bird, 88.0F, 0.5F, 768.0F);
        event_set_pos(event_get_player(0), 88.0F, 0.5F, 768.0F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_set_ang(ev0061_bird, 0, 0x4000, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0061_bird);
        event_play_action(event_get_player(0), &ev0061_amy_action_4,
                          &amy_texlist, 0.9F, EVENT_REPEAT, 0);
        event_play_action(ev0061_bird, ev0061_ver3_wing_action,
                          &ev0061_ver3_wing_texlist, 0.9F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 130.0F, 0.5F, 768.0F, 0.23F,
                           1.5F);
        event_follow(ev0061_bird, 0.0F, 0.0F, 0.0F, 185, event_get_player(0));
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 136.66F, 4.5F, 765.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x0b00, 0x5000, 0);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_39);
        event_voice_play(1391);
        event_message_show(TEXT_11);
        event_wait(80);
        event_play_action(event_get_player(0), &ev0061_amy_action_4,
                          &amy_texlist, 0.1F, EVENT_REPEAT, 0);
        event_play_action(ev0061_bird, ev0061_ver3_wing_action,
                          &ev0061_ver3_wing_texlist, 0.1F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 121.15F, 4.95F, 761.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0xb100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 120.1F, 4.95F, 768.0F);
        event_wait(40);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(6);
        event_kill_task(&ev0061_bird);
        event_shapes_clear_firm_players(event_get_player(0));
        event_white_kill();
        event_flags[FLAG_TRAFFIC] = TRUE;
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0061_textures[] = {
    {"VER1_WING", &ev0061_ver1_wing_texlist},
    {"VER2_WING", &ev0061_ver2_wing_texlist},
    {"VER3_WING", &ev0061_ver3_wing_texlist},
    {"EGGROB", &zero_texlist},
    {NULL, NULL},
};

static NJS_TEXNAME ev0061_ver1_wing_textures[12] = {
    {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
    {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0},
    {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
};

static NJS_TEXLIST ev0061_ver1_wing_texlist = {
#include "ev0061/script/ev0061_ver1_wing_texlist.inc"
};

static NJS_TEXNAME ev0061_ver2_wing_textures[12] = {
    {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
    {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0},
    {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
};

static NJS_TEXLIST ev0061_ver2_wing_texlist = {
#include "ev0061/script/ev0061_ver2_wing_texlist.inc"
};

static NJS_TEXNAME ev0061_ver3_wing_textures[12] = {
    {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
    {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0},
    {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
};

static NJS_TEXLIST ev0061_ver3_wing_texlist = {
#include "ev0061/script/ev0061_ver3_wing_texlist.inc"
};
