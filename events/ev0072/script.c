/* EV0072, the end of Amy's story: her bird flies off with its two siblings, and
 * Amy vows to do what she can */
#include <ninja.h>

#include "ev0072/amy_action_8.h"
#include "ev0072/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 40
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 28
#define TEXT_7_LENGTH 34
#define TEXT_8_LENGTH 50
#define TEXT_9_LENGTH 48
#define TEXT_10_LENGTH 85
#define TEXT_11_LENGTH 65
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 5
#define TEXT_14_LENGTH 3
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 12
#define TEXT_17_LENGTH 4
#define TEXT_18_LENGTH 11
#define TEXT_19_LENGTH 14
#define TEXT_20_LENGTH 17
#include "ev0072/script/text.inc"

extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_walk_action;
extern NJS_ACTION amy_trot_action;
extern NJS_ACTION amy_clear_action;

extern NJS_ACTION ev0072_amy_action_0;
extern NJS_ACTION ev0072_amy_action_1;
extern NJS_ACTION ev0072_amy_action_2;
extern NJS_ACTION ev0072_amy_action_3;
extern NJS_ACTION ev0072_amy_action_4;
extern NJS_ACTION ev0072_amy_action_5;
extern NJS_ACTION ev0072_amy_action_6;
extern NJS_ACTION ev0072_amy_action_7;

extern struct event_path ev0072_bird_path;

/* the bird's two siblings, ev0072_wing_p and ev0072_wing_t, fly up and away
 * from Amy along these paths */
static struct event_path_point ev0072_wing_p_points[11] = {
#include "ev0072/script/ev0072_wing_p_points.inc"
};

static struct event_path ev0072_wing_p_path = {11, ev0072_wing_p_points};

static struct event_path_point ev0072_wing_t_points[11] = {
#include "ev0072/script/ev0072_wing_t_points.inc"
};

static struct event_path ev0072_wing_t_path = {11, ev0072_wing_t_points};

/* Amy's bird, as a task for each of its two models. ev0072_bird shows the
 * bird first, and ev0072_bird_ver4 shows it next until the script kills that
 * task. ev0072_bird then shows it again. the task not showing the bird
 * stands at least 30 below the bird's starting height */
static struct task *ev0072_bird = NULL;
static struct task *ev0072_bird_ver4 = NULL;
/* the bird's two siblings, named for their PVMs, WING_P and WING_T */
static struct task *ev0072_wing_p = NULL;
static struct task *ev0072_wing_t = NULL;
/* the fade to black that hides the cut to Amy at another place */
static struct task *ev0072_fade = NULL;

/* @hack spelling: the test and the call as one expression, as
 * src/event/play.h's EVENT_KILL has them. SHC then loads event_set_flags's
 * address into R11 at 0x0cb80242. as ifs, R11 takes event_set_shadow's address
 * instead, and 41 of ev0072_script's 2520 words differ */
#define SET_FADE_SPEED(speed)                                                  \
    (ev0072_fade != NULL && (event_fade_set_speed(ev0072_fade, speed), 0))

void ev0072_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0072.mlt");
        event_sound_init(3);
        event_wait(1);
        event_reset_player(0);
        ev0072_fade =
            event_fade_create(EVENT_FADE_STILL, 0.0F, 0.0F, 0.0F, 0.0F);
        event_set_pos(event_get_player(0), -78.7F, 752.3F, -361.3F);
        event_set_ang(event_get_player(0), 0x00f8, 0xadb1, 0x02ff);
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          0.8F, EVENT_REPEAT, 8);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_10);
        event_look_at(event_get_player(0), -142.5F, 749.5F, -380.0F);
        event_place_actor(&ev0072_bird, -137.5F, 747.9F, -381.0F, 0x00f8,
                          0xadb1, 0x02ff);
        event_place_actor(&ev0072_bird_ver4, -137.5F, 747.9F, -381.0F, 0x00f8,
                          0xadb1, 0x02ff);
        event_place_actor(&ev0072_wing_p, -134.0F, 751.8F, -385.0F, 0x00f8,
                          0xac01, 0x02ff);
        event_place_actor(&ev0072_wing_t, -136.3F, 750.66F, -376.5F, 0x00f8,
                          0xadb1, 0x02ff);
        event_wait(1);
        event_set_pos(ev0072_bird, -137.5F, 748.2F, -381.0F);
        event_set_ang(ev0072_bird, 0, 0xe000, 0x4000);
        event_set_flags(ev0072_bird, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0072_bird, 0.5F);
        event_play_action(ev0072_bird, &ev0072_bird_action,
                          &ev0072_bird_texlist, 0.02F, EVENT_REPEAT, 16);
        event_set_pos(ev0072_bird_ver4, -137.5F, 710.0F, -381.0F);
        event_set_ang(ev0072_bird_ver4, 0x40f8, 0x6db1, 0x02ff);
        event_set_shadow(ev0072_bird_ver4, 0.01F);
        event_set_flags(ev0072_bird_ver4, EVENT_IGNORE_GROUND);
        event_play_action(ev0072_bird_ver4, &ev0072_bird_ver4_action_1,
                          &ev0072_bird_ver4_texlist, 0.1F, EVENT_REPEAT, 0);
        event_set_pos(ev0072_wing_p, -135.7F, 749.6F, -382.3F);
        event_set_ang(ev0072_wing_p, 0x00f8, 0xd000, 0x02ff);
        event_set_flags(ev0072_wing_p, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0072_wing_p, 0.5F);
        event_play_action(ev0072_wing_p, &ev0072_wing_action,
                          &ev0072_wing_p_texlist, 0.9F, EVENT_REPEAT, 16);
        event_set_pos(ev0072_wing_t, -136.6F, 750.66F, -376.1F);
        event_set_ang(ev0072_wing_t, 0, 0x86b1, 0x05ff);
        event_set_flags(ev0072_wing_t, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0072_wing_t, 0.5F);
        event_play_action(ev0072_wing_t, &ev0072_wing_action,
                          &ev0072_wing_t_texlist, 0.9F, EVENT_REPEAT, 16);
        /* the same action again, queued behind the first, so the first ends
         * after one pass. both ends of the 16-draw blend into the copy are the
         * motion's frame 0, so ev0072_wing_t holds that pose for the 16 draws.
         * the copy then repeats */
        event_play_action(ev0072_wing_t, &ev0072_wing_action,
                          &ev0072_wing_t_texlist, 0.9F, EVENT_REPEAT, 16);
        event_talk_start(event_get_player(0), TEXT_11);
        stream_request_music(MUSIC_EVTBGM00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -140.9F, 748.0F, -382.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -141.0F, 748.0F, -381.0F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_run_to_speed(event_get_player(0), -137.5F, 747.0F, -380.0F, 0.05F,
                           0.01F);
        event_wait(90);
        event_camera_look_clear();
        event_wait(5);
        event_run_clear(event_get_player(0));
        event_set_pos(event_get_player(0), -133.3F, 747.3F, -379.85F);
        event_set_ang(event_get_player(0), 0, 0xd029, 0x0329);
        event_set_pos(ev0072_wing_p, -137.5F, 751.3F, -383.3F);
        event_set_ang(ev0072_wing_p, 0x01f8, 0x2200, 0x02ff);
        event_set_pos(ev0072_wing_t, -134.6F, 755.4F, -376.2F);
        event_set_ang(ev0072_wing_t, 0, 0x8a00, 0x02ff);
        event_play_action(event_get_player(0), &ev0072_amy_action_2,
                          &amy_texlist, 0.9F, EVENT_REPEAT, 16);
        event_set_pos(ev0072_bird, -137.5F, 717.9F, -381.0F);
        event_set_ang(ev0072_bird, 0, 0xd000, 0x02ff);
        event_set_pos(ev0072_bird_ver4, -134.5F, 747.0F, -376.5F);
        event_set_ang(ev0072_bird_ver4, 0xc500, 0xea00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -150.0F, 761.1F, -384.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x117a, 0xb429, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -147.7F, 750.6F, -383.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x047a, 0xb429, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(1520);
        event_message_show(TEXT_1);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 130, 0x1c72);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -139.3F, 755.4F, -375.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe884, 0xe729, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(1521);
        event_message_show(TEXT_2);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_clear_motions(ev0072_bird_ver4);
        event_play_action(ev0072_bird_ver4, &ev0072_bird_ver4_action_2,
                          &ev0072_bird_ver4_texlist, 0.6F, 0, 0);
        event_wait(50);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_clear_motions(ev0072_bird_ver4);
        event_play_action(ev0072_bird_ver4, &ev0072_bird_ver4_action_0,
                          &ev0072_bird_ver4_texlist, 0.9F, EVENT_REPEAT, 0);
        event_wait(1);
        event_set_pos(ev0072_bird_ver4, -135.4F, 752.3F, -380.1F);
        event_set_ang(ev0072_bird_ver4, 0, 0xcab1, 0);
        event_move(ev0072_bird_ver4, -135.4F, 752.3F, -380.1F, -192.4F, 762.4F,
                   -383.6F, 160);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -143.0F, 752.3F, -385.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x017a, 0xa529, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, -144.9F, 753.2F, -385.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0xff7a, 0xa729, 0);
        event_set_ang(event_get_player(0), 0, 0xc429, 0x0329);
        event_set_pos(ev0072_wing_p, -133.2F, 752.7F, -384.6F);
        event_set_ang(ev0072_wing_p, 0x01f8, 0xc000, 0x02ff);
        event_play_action(ev0072_wing_p, &ev0072_wing_action,
                          &ev0072_wing_p_texlist, 3.4F, EVENT_REPEAT, 16);
        event_set_pos(ev0072_wing_t, -134.6F, 755.4F, -376.2F);
        event_set_ang(ev0072_wing_t, 0, 0xc300, 0x02ff);
        event_play_action(ev0072_wing_t, &ev0072_wing_action,
                          &ev0072_wing_t_texlist, 3.4F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(0), &ev0072_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(0), ev0072_bird_ver4, 0.0F, 0.0F,
                           0.0F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 35, -159.5F, 747.5F, -395.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 45, 0x0f7a, 0x9929, 0x0500);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -127.6F, 749.5F, -367.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x067a, 0x1f29, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -125.4F, 749.4F, -369.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x067a, 0x2c29, 0);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -181.7F, 759.9F, -384.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x067a, 0x4429, 0);
        event_camera_follow(ev0072_bird_ver4);
        event_move(ev0072_bird_ver4, -192.4F, 762.4F, -383.6F, -261.2F, 774.76F,
                   -382.96F, 60);
        event_wait(75);
        event_camera_follow_clear();
        stream_stop_music();
        event_move_turn(ev0072_bird_ver4, -261.2F, 774.76F, -382.96F, -273.9F,
                        745.0F, -382.96F, 0, 0x8ab1, 0, 0x5000, 0x8fb1, 0x3000,
                        65);
        event_wait(65);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -141.7F, 750.5F, -381.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c7a, 0xb929, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 10, -139.8F, 751.1F, -380.7F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_14);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_wait(20);
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 1);
        event_wait(10);
        event_run_to_speed(event_get_player(0), -179.4F, 744.5F, -378.0F, 0.4F,
                           0.06F);
        event_wait(10);
        event_set_pos(event_get_player(0), -179.4F, 744.5F, -378.0F);
        event_set_ang(event_get_player(0), 0, 0xbf29, 0x0329);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -191.9F, 755.0F, -380.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf4f8, 0xb08e, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 20, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -172.5F, 750.5F, -382.4F);
        event_wait(40);
        stream_request_music(MUSIC_EVTBGM03);
        event_camera_look_clear();
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &ev0072_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -166.3F, 744.93F, -382.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x14f8, 0x498e, 0xfd00);
        event_kill_task(&ev0072_bird_ver4);
        event_play_path(ev0072_bird, &ev0072_bird_path, 0.075F, 0);
        event_play_action(ev0072_bird, &ev0072_bird_action,
                          &ev0072_bird_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -160.8F, 746.5F, -378.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x10c2, 0x4110, 0);
        event_camera_look_clear();
        event_play_path(ev0072_wing_p, &ev0072_wing_p_path, 0.09F, 0);
        event_play_path(ev0072_wing_t, &ev0072_wing_t_path, 0.09F, 0);
        event_wait(60);
        event_wait(35);
        event_clear_paths(ev0072_bird);
        event_clear_paths(ev0072_wing_p);
        event_clear_paths(ev0072_wing_t);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -182.76F, 752.7F, -379.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe8d8, 0xb25f, 0xfc80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -182.65F, 753.5F, -378.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xe1d0, 0xba5f, 0xfc80);
        event_wait(100);
        event_camera_look_clear();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -179.4F, 744.5F, -378.0F);
        event_set_ang(event_get_player(0), 0, 0xb000, 0x0329);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -201.61F, 768.8F, -371.42F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe0e3, 0xd2eb, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -184.61F, 762.7F, -370.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xd991, 0xeaed, 0);
        event_play_action(event_get_player(0), &ev0072_amy_action_4,
                          &amy_texlist, 1.3F, EVENT_REPEAT, 8);
        event_wait(40);
        event_set_ang(ev0072_bird, 0xfe00, 0x8000, 0);
        event_move(ev0072_bird, -178.2F, 755.5F, -371.3F, -188.8F, 753.2F,
                   -417.0F, 40);
        event_play_action(ev0072_bird, &ev0072_bird_action,
                          &ev0072_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_set_ang(ev0072_wing_p, 0, 0x8000, 0);
        event_move(ev0072_wing_p, -182.2F, 754.5F, -371.0F, -185.8F, 750.2F,
                   -417.0F, 45);
        event_set_ang(ev0072_wing_t, 0, 0x8600, 0);
        event_move(ev0072_wing_t, -174.2F, 753.5F, -372.8F, -191.8F, 748.2F,
                   -417.0F, 45);
        event_wait(15);
        event_run_clear(ev0072_bird);
        event_run_clear(ev0072_wing_p);
        event_run_clear(ev0072_wing_t);
        event_set_pos(ev0072_wing_p, -133.2F, 852.7F, -384.6F);
        event_set_ang(ev0072_wing_p, 0x01f8, 0xc000, 0x02ff);
        event_set_pos(ev0072_wing_t, -134.6F, 855.4F, -376.2F);
        event_set_ang(ev0072_wing_t, 0, 0xc300, 0x02ff);
        event_set_pos(ev0072_bird, -135.4F, 852.3F, -380.1F);
        event_set_ang(ev0072_bird, 0, 0xcab1, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -169.9F, 749.1F, -371.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xfff1, 0x354b, 0);
        event_play_action(event_get_player(0), &ev0072_amy_action_5,
                          &amy_texlist, 1.3F, 0, 8);
        event_play_action(event_get_player(0), &ev0072_amy_action_6,
                          &amy_texlist, 1.3F, EVENT_REPEAT, 8);
        event_wait(25);
        event_set_ang(ev0072_bird, 0xfe00, 0x8000, 0);
        event_move(ev0072_bird, -178.2F, 759.5F, -371.3F, -177.8F, 750.2F,
                   -371.0F, 60);
        event_play_action(ev0072_bird, &ev0072_bird_action,
                          &ev0072_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_15);
        event_voice_play(1522);
        event_message_show(TEXT_3);
        event_set_ang(ev0072_wing_p, 0, 0x7800, 0);
        event_move(ev0072_wing_p, -182.2F, 759.5F, -371.0F, -180.8F, 749.3F,
                   -374.0F, 60);
        event_set_ang(ev0072_wing_t, 0, 0x8600, 0);
        event_move(ev0072_wing_t, -174.2F, 759.5F, -372.8F, -175.5F, 748.0F,
                   -371.0F, 60);
        event_wait(100);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -168.9F, 749.1F, -371.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xfff1, 0x372a, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1523);
        event_message_show(TEXT_4);
        event_wait(100);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 60, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, -176.4F, 800.0F, -361.7F);
        event_wait(20);
        event_play_action(event_get_player(0), &ev0072_amy_action_7,
                          &amy_texlist, 1.3F, 0, 8);
        event_play_action(event_get_player(0), &ev0072_amy_action_8,
                          &amy_texlist, 1.3F, EVENT_REPEAT, 8);
        event_set_ang(ev0072_bird, -0x1000, 0x8000, 0);
        event_move(ev0072_bird, -177.8F, 750.2F, -372.0F, -177.8F, 794.2F,
                   -363.0F, 145);
        event_wait(10);
        event_set_ang(ev0072_wing_p, 0, 0x8000, 0);
        event_move(ev0072_wing_p, -180.8F, 749.3F, -372.6F, -185.0F, 788.0F,
                   -367.0F, 140);
        event_set_ang(ev0072_wing_t, 0, 0x7b00, 0);
        event_move(ev0072_wing_t, -175.5F, 748.0F, -371.0F, -169.0F, 788.0F,
                   -365.0F, 140);
        event_wait(145);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -177.5F, 782.2F, -371.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1a5f, 0x5a9b, 0x0bd0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -174.8F, 789.4F, -368.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x1a5f, 0x639b, 0x0dd0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -165.8F, 784.2F, -368.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x195f, 0x4d9b, 0x0dd0);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -164.2F, 783.0F, -369.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x195f, 0x4d9b, 0x0dd0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -173.8F, 798.2F, -364.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xca5f, 0x169b, 0x0200);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, -175.0F, 799.8F, -363.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0xcc5f, 0x1c9b, 0x09d0);
        event_wait(38);
        event_reset_player(0);
        event_set_pos(event_get_player(0), -179.4F, 744.5F, -378.0F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0x0329);
        event_move_turn(ev0072_bird, -247.0F, 811.0F, -373.0F, -359.0F, 830.0F,
                        -360.0F, 0, 0x8000, 0x1000, 0, 0x8000, 0x1000, 280);
        event_move_turn(ev0072_wing_p, -247.0F, 800.0F, -366.0F, -353.0F,
                        820.0F, -350.0F, 0, 0x8000, 0xf000, 0, 0x8000, 0xf000,
                        280);
        event_move_turn(ev0072_wing_t, -247.0F, 800.0F, -380.0F, -353.0F,
                        820.0F, -370.0F, 0, 0x8000, 0xf000, 0, 0x8000, 0xf000,
                        280);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -170.28F, 746.2F, -371.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1107, 0x36f1, 0x0030);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, -168.6F, 745.0F, -385.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0x1307, 0x4df1, 0x0330);
        event_play_action(event_get_player(0), &ev0072_amy_action_0,
                          &amy_texlist, 0.9F, EVENT_REPEAT, 4);
        event_wait(170);
        event_play_action(event_get_player(0), &ev0072_amy_action_1,
                          &amy_texlist, 0.9F, EVENT_REPEAT, 4);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -185.2F, 750.8F, -382.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0607, 0xa4f1, 0xff30);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 130, 0x0107, 0xa4f1, 0xff30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1524);
        event_message_show(TEXT_5);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -184.4F, 750.8F, -381.6F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1525);
        event_message_wait(80, TEXT_6);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        SET_FADE_SPEED(0.02F);
        event_wait(70);
        event_clear_motions(event_get_player(0));
        event_wait(10);
        SET_FADE_SPEED(-0.03F);
        event_look_clear(event_get_player(0));
        stream_request_music(MUSIC_THEAMY);
        event_set_pos(event_get_player(0), 234.0F, 569.0F, -639.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 242.2F, 587.8F, -655.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1507, 0x6c21, 0xff30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 238.2F, 571.4F, -647.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x1507, 0x6d21, 0xff30);
        event_wait(60);
        event_play_action(event_get_player(0), &amy_walk_action, &amy_texlist,
                          0.8F, EVENT_REPEAT, 0);
        event_wait(2);
        event_run_to_speed(event_get_player(0), 234.4F, 571.0F, -724.0F, 0.4F,
                           0.02F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 238.2F, 574.4F, -647.2F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 20, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(1526);
        event_message_show(TEXT_7);
        event_voice_wait();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 226.2F, 569.8F, -731.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1000, 0x7f21, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, 244.5F, 572.8F, -731.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x1000, 0x7221, 0xff00);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(1527);
        event_message_wait(120, TEXT_8);
        event_camera_follow_clear();
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_run_clear(event_get_player(0));
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_set_pos(event_get_player(0), 304.0F, 569.0F, -723.0F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 302.9F, 573.0F, -712.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0xf921, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 310.9F, 575.8F, -714.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xf800, 0x1921, 0xff00);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_voice_play(1528);
        event_message_show(TEXT_9);
        event_wait(10);
        event_play_action(event_get_player(0), &amy_clear_action, &amy_texlist,
                          0.9F, 0, 16);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, 312.9F, 575.0F, -726.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0x0900, 0x5021, 0xff00);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 315.7F, 572.5F, -727.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0a00, 0x5021, 0x0800);
        event_wait(15);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 4);
        event_talk_stop(event_get_player(0));
        event_voice_wait();
        event_wait(90);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_voice_stop();
        event_clear_task(event_get_player(0));
        event_kill_task(&ev0072_bird);
        event_kill_task(&ev0072_bird_ver4);
        event_kill_task(&ev0072_wing_p);
        event_kill_task(&ev0072_wing_t);
        if (ev0072_fade != NULL) {
            task_kill(ev0072_fade);
            ev0072_fade = NULL;
        }
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0072_textures[] = {
    {"VER2_WING", &ev0072_bird_texlist},
    {"VER4_WING", &ev0072_bird_ver4_texlist},
    {"WING_P", &ev0072_wing_p_texlist},
    {"WING_T", &ev0072_wing_t_texlist},
    {NULL, NULL},
};
