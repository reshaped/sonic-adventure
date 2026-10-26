/* EV00FF: before the last fight, the emeralds gather about Sonic, and he
 * becomes Super Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00ff/big_action.h"
#include "ev00ff/emerald_textures.h"
#include "ev00ff/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/big.h"
#include "player/knuckles.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/textures.h"
#include "shared/chaos7.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/highway.h"
#include "shared/move.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 58
#define TEXT_6_LENGTH 48
#define TEXT_7_LENGTH 51
#define TEXT_8_LENGTH 64
#define TEXT_9_LENGTH 67
#define TEXT_10_LENGTH 33
#define TEXT_11_LENGTH 5
#include "ev00ff/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION amy_run_action;
extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev00ff_sonic_action_0;
extern NJS_ACTION ev00ff_sonic_action_1;
extern NJS_ACTION ev00ff_sonic_action_2;
extern NJS_ACTION ev00ff_sonic_action_3;
extern NJS_ACTION ev00ff_sonic_action_4;
extern NJS_ACTION ev00ff_sonic_action_5;
extern NJS_ACTION ev00ff_amy_action_0;
extern NJS_ACTION ev00ff_amy_action_1;
extern NJS_ACTION ev00ff_tails_action;
extern NJS_ACTION ev00ff_knuckles_action;
extern NJS_TEXLIST ev00ff_bird_texlist;
extern NJS_ACTION ev00ff_bird_action;

/* Amy's bird */
static struct task *ev00ff_bird = NULL;
/* the seven emeralds, by number. Amy holds emerald 0, Tails emerald 1 and
 * Knuckles emeralds 2, 3, 5 and 6. emerald 4 shows only as the seven gather
 * about Sonic */
static struct task *ev00ff_emerald_0 = NULL;
static struct task *ev00ff_emerald_1 = NULL;
static struct task *ev00ff_emerald_2 = NULL;
static struct task *ev00ff_emerald_3 = NULL;
static struct task *ev00ff_emerald_4 = NULL;
static struct task *ev00ff_emerald_5 = NULL;
static struct task *ev00ff_emerald_6 = NULL;
/* one emerald, shown near Big, near Tails and then near Knuckles, who looks
 * at it */
static struct task *ev00ff_found_emerald = NULL;
static struct task *ev00ff_sonic_floor = NULL;
/* no code uses this word. it keeps ev00ff_amy_floor at its address in the
 * image */
static struct task *dat_0cb84338 = NULL;
static struct task *ev00ff_amy_floor = NULL;
static struct task *ev00ff_tails_floor = NULL;
static struct task *ev00ff_knuckles_floor = NULL;
static struct task *ev00ff_big_floor = NULL;
/* the pale yellow fade that hides Sonic's change into Super Sonic */
static struct task *ev00ff_fade = NULL;

void ev00ff_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_sound_init(4);
        sound_load_file("e_00ff.mlt");
        event_highway_create();
        event_set_pos(event_get_player(0), 806.63F, 150.0F, -251.4F);
        event_set_ang(event_get_player(0), 0, 0x2000, 0);
        event_chaos7_hold(270.0F, 0.0F, -8500.0F, 0, 0xa000, 0, 0x038e);
        event_create_player(3, amy_step, 736.23F, 150.0F, -391.75F, 0, 0x3000,
                            0);
        event_create_player(4, tails_step, 867.89F, 150.0F, -404.22F, 0, 0x9000,
                            0);
        event_create_player(5, knuckles_step, 901.04F, 150.0F, -418.0F, 0,
                            0x8b00, 0);
        event_create_player(6, big_step, 705.68F, 150.0F, -372.72F, 0, 0x4000,
                            0);
        event_place_actor(&ev00ff_bird, 746.23F, 150.0F, -391.75F, 0, 0x3000,
                          0);
        event_place_actor(&ev00ff_emerald_0, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00ff_emerald_1, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00ff_emerald_2, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00ff_emerald_3, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00ff_emerald_4, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00ff_emerald_5, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00ff_emerald_6, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00ff_found_emerald, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_wait(1);
        event_set_pos(event_get_player(6), 705.68F, 150.0F, -372.72F);
        event_set_pos(event_get_player(4), 756.09F, 160.0F, -394.66F);
        event_set_pos(event_get_player(5), 687.97F, 160.0F, -537.43F);
        event_set_pos(event_get_player(3), 736.23F, 150.0F, -391.75F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00ff_sonic_action_1,
                          &sonic_texlist, 0.5F, 0, 4);
        ev00ff_sonic_floor = event_floor_create(event_get_player(0), 150.0F);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &amy_run_action, &amy_texlist,
                          2.0F, EVENT_REPEAT, 0);
        ev00ff_amy_floor = event_floor_create(event_get_player(3), 150.0F);
        event_clear_motions(event_get_player(4));
        ev00ff_tails_floor = event_floor_create(event_get_player(4), 150.0F);
        event_clear_motions(event_get_player(5));
        ev00ff_knuckles_floor = event_floor_create(event_get_player(5), 150.0F);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &ev00ff_big_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        ev00ff_big_floor = event_floor_create(event_get_player(6), 150.0F);
        event_set_flags(ev00ff_bird, EVENT_IGNORE_GROUND);
        event_set_pos(ev00ff_bird, 741.23F, 158.0F, -393.75F);
        event_set_ang(ev00ff_bird, 0, 0x1000, 0);
        event_clear_motions(ev00ff_bird);
        event_play_action(ev00ff_bird, &ev00ff_bird_action,
                          &ev00ff_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(ev00ff_emerald_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ff_emerald_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ff_emerald_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ff_emerald_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ff_emerald_4, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ff_emerald_5, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ff_emerald_6, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ff_found_emerald, EVENT_IGNORE_GROUND);
        event_set_pos(ev00ff_found_emerald, 735.0F, 150.0F, -372.0F);
        event_set_ang(ev00ff_found_emerald, 0x2000, 0x2000, 0x1000);
        event_play_action(ev00ff_found_emerald, &ev00ff_emerald_action,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 713.13F, 159.0F, -372.46F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0a00, 0x3f00, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 740.98F, 151.0F, -361.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0b00, 0x2900, 0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 748.43F, 152.71F, -380.44F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0300, 0xe6a7, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 744.48F, 151.9F, -369.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0100, 0xe2a7, 0);
        EVENT_KILL(ev00ff_big_floor);
        event_kill_player(6);
        event_set_pos(event_get_player(4), 756.09F, 160.0F, -394.66F);
        event_set_ang(event_get_player(4), 0, 0xf3e8, 0);
        event_set_pos(ev00ff_found_emerald, 750.0F, 150.0F, -373.39F);
        event_set_ang(ev00ff_found_emerald, 0x2000, 0x2000, 0x1000);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 693.39F, 153.3F, -523.79F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0500, 0x1a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 695.64F, 151.32F, -515.72F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0700, 0x1a00, 0);
        event_set_pos(event_get_player(4), 867.89F, 150.0F, -404.22F);
        event_set_ang(event_get_player(4), 0, 0x9000, 0);
        event_set_pos(ev00ff_found_emerald, 685.41F, 150.0F, -521.8F);
        event_set_ang(ev00ff_found_emerald, 0x2000, 0x2000, 0x1000);
        event_set_pos(event_get_player(5), 687.97F, 160.0F, -537.43F);
        event_set_ang(event_get_player(5), 0, 0xe6a8, 0);
        event_wait(45);
        event_look_at_task(event_get_player(5), ev00ff_found_emerald, 0.0F,
                           0.0F, 0.0F);
        event_wait(45);
        event_set_pos(event_get_player(5), 901.04F, 150.0F, -418.0F);
        event_set_ang(event_get_player(5), 0, 0x8b00, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 812.44F, 155.3F, -235.87F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0100, 0x1d00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 818.49F, 155.3F, -241.09F);
        event_look_clear(event_get_player(5));
        event_kill_task(&ev00ff_found_emerald);
        event_play_action(event_get_player(0), &ev00ff_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 16);
        event_move_turn(event_get_player(3), event_get_player(3)->work->pos.x,
                        event_get_player(3)->work->pos.y,
                        event_get_player(3)->work->pos.z, 799.81F, 150.0F,
                        -275.78F, event_get_player(3)->work->ang.x,
                        0x4000 - event_get_player(3)->work->ang.y,
                        event_get_player(3)->work->ang.z, 0, 0x3000, 0, 140);
        event_follow(ev00ff_bird, 5.0F, 8.0F, -2.0F, 140, event_get_player(3));
        event_voice_play(2003);
        event_message_show(TEXT_1);
        event_wait(45);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(0), event_get_player(3), 0.0F, 6.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 801.0F, 156.1F, -308.23F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(3),
                                  0.0F, 6.0F, 0.0F, 0);
        event_voice_play(2004);
        event_message_show(TEXT_2);
        event_wait(60);
        event_set_pos(event_get_player(3), 799.81F, 150.0F, -274.69F);
        event_set_ang(event_get_player(3), 0, 0x1000, 0);
        event_set_ang(ev00ff_bird, 0, 0x1000, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_wait(1);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 822.47F, 155.92F, -239.19F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe12, 0x1d5d, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 120, 0x18e4);
        event_set_pos(ev00ff_emerald_0, event_get_player(3)->work->pos.x + 0.9F,
                      event_get_player(3)->work->pos.y + 3.5F,
                      event_get_player(3)->work->pos.z + 1.9F);
        event_set_ang(ev00ff_emerald_0, 0, 0, 0);
        event_play_action(ev00ff_emerald_0, &ev00ff_emerald_action,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev00ff_amy_action_0,
                          &amy_texlist, 1.0F, 0, 0);
        event_wait(30);
        event_move(ev00ff_emerald_0, event_get_player(3)->work->pos.x + 0.9F,
                   event_get_player(3)->work->pos.y + 3.5F,
                   event_get_player(3)->work->pos.z + 1.9F,
                   event_get_player(3)->work->pos.x + 1.05F,
                   event_get_player(3)->work->pos.y + 4.8F,
                   event_get_player(3)->work->pos.z + 3.05F, 30);
        event_play_action(event_get_player(3), &ev00ff_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 32);
        event_talk_start(event_get_player(3), TEXT_11);
        event_wait(5);
        event_voice_play(2006);
        event_message_show(TEXT_3);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x18e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 812.97F, 156.34F, -305.01F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0012, 0x755d, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 801.09F, 156.34F, -305.17F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xff12, 0x855d, 0);
        event_wait(30);
        event_set_pos(event_get_player(4), 843.99F, 200.0F, -187.56F);
        event_wait(10);
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &ev00ff_sonic_action_0,
                          &sonic_texlist, 0.5F, 0, 8);
        event_wait(20);
        event_set_pos(event_get_player(5), 836.31F, 200.0F, -181.07F);
        event_wait(40);
        event_play_action(event_get_player(4), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(20);
        event_play_action(event_get_player(5), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 826.85F, 154.0F, -217.53F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0282, 0x9068, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 824.11F, 153.57F,
                             -223.95F);
        event_set_pos(ev00ff_emerald_1,
                      event_get_player(4)->work->pos.x - 0.85F,
                      event_get_player(4)->work->pos.y + 3.5F,
                      event_get_player(4)->work->pos.z - 2.0F);
        event_set_ang(ev00ff_emerald_1, 0, 0, 0);
        event_play_action(ev00ff_emerald_1, &ev00ff_emerald_action,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev00ff_emerald_2, event_get_player(5)->work->pos.x - 3.5F,
                      event_get_player(5)->work->pos.y + 4.8F,
                      event_get_player(5)->work->pos.z - 2.5F);
        event_set_ang(ev00ff_emerald_2, 0, 0, 0);
        event_play_action(ev00ff_emerald_2, &ev00ff_emerald_action,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev00ff_emerald_3, event_get_player(5)->work->pos.x - 2.5F,
                      event_get_player(5)->work->pos.y + 3.8F,
                      event_get_player(5)->work->pos.z - 2.5F);
        event_set_ang(ev00ff_emerald_3, 0, 0, 0);
        event_play_action(ev00ff_emerald_3, &ev00ff_emerald_action,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev00ff_emerald_5, event_get_player(5)->work->pos.x + 2.3F,
                      event_get_player(5)->work->pos.y + 4.2F,
                      event_get_player(5)->work->pos.z - 4.0F);
        event_set_ang(ev00ff_emerald_5, 0, 0, 0);
        event_play_action(ev00ff_emerald_5, &ev00ff_emerald_action,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev00ff_emerald_6, event_get_player(5)->work->pos.x + 1.5F,
                      event_get_player(5)->work->pos.y + 3.8F,
                      event_get_player(5)->work->pos.z - 3.0F);
        event_set_ang(ev00ff_emerald_6, 0, 0, 0);
        event_play_action(ev00ff_emerald_6, &ev00ff_emerald_action,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(4));
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(4), &ev00ff_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(5), &ev00ff_knuckles_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(2008);
        event_message_show(TEXT_4);
        event_chaos7_set_pos(150.0F, 0.0F, 50.0F);
        event_chaos7_set_ang(0, 0, 0);
        event_wait(30);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x38e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 886.43F, 157.4F, -352.91F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1a82, 0x5868, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, 1038.09F, 157.4F,
                             -130.75F);
        event_voice_play(2011);
        event_message_show(TEXT_5);
        event_wait(120);
        event_message_close();
        event_look_clear(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x1000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 848.45F, 154.12F, -202.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0482, 0x7768, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, 839.64F, 154.12F,
                             -204.47F);
        event_chaos7_set_pos(270.0F, 0.0F, -8500.0F);
        event_chaos7_set_ang(0, 0xa000, 0);
        event_talk_start(event_get_player(4), TEXT_11);
        event_wait(5);
        event_voice_play(2013);
        event_message_show(TEXT_6);
        event_voice_wait();
        event_talk_stop(event_get_player(4));
        event_wait(15);
        event_talk_start(event_get_player(5), TEXT_11);
        event_wait(5);
        event_voice_play(2015);
        event_message_show(TEXT_7);
        event_voice_wait();
        event_talk_stop(event_get_player(5));
        event_message_close();
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 812.04F, 156.64F, -244.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe82, 0x7768, 0xfb00);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 15, 0xfe82, 0x1c68, 0xfb00);
        event_wait(5);
        stream_request_music(MUSIC_SPRSONIC);
        event_sound_start(0, 1234, 1800);
        event_sound_fade_volume(0, 120, 30);
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 803.68F, 152.62F, -241.28F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1982, 0x0768, 0xf700);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 600, 815.87F, 150.31F,
                             -245.21F);
        event_set_pos(event_get_player(3), 799.81F, 150.0F, -295.78F);
        event_set_pos(ev00ff_bird, 804.81F, 158.0F, -297.78F);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_voice_play(2017);
        event_wait(20);
        event_voice_play(2018);
        event_wait(15);
        event_voice_play(2024);
        event_wait(20);
        event_voice_play(2019);
        event_wait(30);
        event_voice_play(2026);
        event_message_show(TEXT_8);
        event_wait(100);
        event_voice_play(2028);
        event_message_show(TEXT_9);
        event_wait(100);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 822.18F, 155.74F, -241.27F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0082, 0x2e68, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 835.57F, 153.57F, -237.3F);
        event_look_clear(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x1000, 0);
        event_set_pos(ev00ff_emerald_0, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z + 0.2F);
        event_set_pos(ev00ff_emerald_1, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 5.3F,
                      event_get_player(0)->work->pos.z + 0.2F);
        event_set_pos(ev00ff_emerald_2, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 5.3F,
                      event_get_player(0)->work->pos.z + 0.2F);
        event_set_pos(ev00ff_emerald_3, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 5.3F,
                      event_get_player(0)->work->pos.z + 0.2F);
        event_set_pos(ev00ff_emerald_4, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 5.3F,
                      event_get_player(0)->work->pos.z + 0.2F);
        event_set_pos(ev00ff_emerald_5, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 5.3F,
                      event_get_player(0)->work->pos.z + 0.2F);
        event_set_pos(ev00ff_emerald_6, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 5.3F,
                      event_get_player(0)->work->pos.z + 0.2F);
        event_set_ang(ev00ff_emerald_0, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_set_ang(ev00ff_emerald_1, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_set_ang(ev00ff_emerald_2, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_set_ang(ev00ff_emerald_3, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_set_ang(ev00ff_emerald_4, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_set_ang(ev00ff_emerald_5, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_set_ang(ev00ff_emerald_6, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_clear_motions(ev00ff_emerald_0);
        event_clear_motions(ev00ff_emerald_1);
        event_clear_motions(ev00ff_emerald_2);
        event_clear_motions(ev00ff_emerald_3);
        event_clear_motions(ev00ff_emerald_4);
        event_clear_motions(ev00ff_emerald_5);
        event_clear_motions(ev00ff_emerald_6);
        event_play_action(ev00ff_emerald_0, &ev00ff_gather_action_0,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00ff_emerald_1, &ev00ff_gather_action_1,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00ff_emerald_2, &ev00ff_gather_action_2,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00ff_emerald_3, &ev00ff_gather_action_3,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00ff_emerald_4, &ev00ff_gather_action_4,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00ff_emerald_5, &ev00ff_gather_action_5,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00ff_emerald_6, &ev00ff_gather_action_6,
                          &ev00ff_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00ff_sonic_action_3,
                          &sonic_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &ev00ff_sonic_action_4,
                          &sonic_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &ev00ff_sonic_action_5,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 32);
        event_wait(1);
        event_voice_play(2030);
        event_message_show(TEXT_10);
        event_wait(89);
        event_message_close();
        event_sound_start(1, 1230, 1800);
        event_sound_fade_volume(1, -120, 1);
        event_wait(1);
        event_sound_fade_volume(1, 120, 60);
        event_rays_create(event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y + 6.0F,
                          event_get_player(0)->work->pos.z, 3, 5, 0.2F, 4.0F,
                          0xb4ffff91, 0x00ffff32);
        event_wait(30);
        ev00ff_fade = event_fade_create(1.0F / 120.0F, 0.2F, 1.0F, 1.0F, 0.88F);
        event_wait(100);
        event_kill_task(&ev00ff_emerald_0);
        event_kill_task(&ev00ff_emerald_1);
        event_kill_task(&ev00ff_emerald_2);
        event_kill_task(&ev00ff_emerald_3);
        event_kill_task(&ev00ff_emerald_4);
        event_kill_task(&ev00ff_emerald_5);
        event_kill_task(&ev00ff_emerald_6);
        event_sound_stop(0);
        event_clear_motions(event_get_player(0));
        event_wait(1);
        player_set_mode(0, PLAYER_MODE_SUPER);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 795.62F, 152.76F, -246.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe00, 0xcf00, 0);
        event_set_ang(event_get_player(0), 0, 0xd000, 0);
        event_wait(60);
        event_rays_kill();
        if (ev00ff_fade != NULL) {
            event_fade_set_speed(ev00ff_fade, -1.0F / 60.0F);
        }
        event_sound_fade_volume(1, -120, 60);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 795.85F, 157.75F,
                             -246.83F);
        event_wait(15);
        event_sound_stop(1);
        EVENT_KILL(ev00ff_fade);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 812.22F, 152.86F, -238.42F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0x1300, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 812.11F, 157.85F, -238.64F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 797.67F, 157.38F, -247.86F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0xab00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 800.29F, 157.38F, -252.5F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 801.43F, 150.65F, -228.16F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 7.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 786.38F, 150.17F,
                             -261.15F);
        event_wait(120);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 788.43F, 175.8F, -254.21F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_sound_start(2, 1231, 1800);
        event_sound_fade_volume(2, 120, 1);
        event_move(event_get_player(0), 806.63F, 150.0F, -251.4F, 806.63F,
                   400.0F, -251.4F, 75);
        event_wait(50);
        event_sound_start(3, 1232, 1800);
        event_white_start_hold(30, 60, 30);
        event_wait(25);
        event_move_stop(event_get_player(0));
        event_sound_stop(2);
        event_wait(3);
        event_sound_stop(3);
        event_wait(2);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_move_stop_all();
        event_white_kill();
        EVENT_KILL(ev00ff_sonic_floor);
        EVENT_KILL(ev00ff_amy_floor);
        EVENT_KILL(ev00ff_tails_floor);
        EVENT_KILL(ev00ff_knuckles_floor);
        EVENT_KILL(ev00ff_big_floor);
        EVENT_KILL(ev00ff_fade);
        event_rays_kill();
        event_reset_player(0);
        event_kill_player(3);
        event_kill_player(4);
        event_kill_player(5);
        event_kill_player(6);
        event_kill_task(&ev00ff_bird);
        event_kill_task(&ev00ff_emerald_0);
        event_kill_task(&ev00ff_emerald_1);
        event_kill_task(&ev00ff_emerald_2);
        event_kill_task(&ev00ff_emerald_3);
        event_kill_task(&ev00ff_emerald_4);
        event_kill_task(&ev00ff_emerald_5);
        event_kill_task(&ev00ff_emerald_6);
        event_kill_task(&ev00ff_found_emerald);
        event_chaos7_release(TRUE);
        event_highway_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00ff_textures[] = {
    {"LAST1A_HIGHWAY_A", &event_highway_texlist},
    {"M_EM_BLACK", &ev00ff_emerald_texlist},
    {"MILES", &tails_texlist},
    {"KNUCKLES", &knuckles_texlist},
    {"WING_P", &ev00ff_bird_texlist},
    {"BIG", &big_texlist},
    {"AMY", &amy_texlist},
    {NULL, NULL},
};
