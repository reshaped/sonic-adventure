/* EV00DE: Chaos 4 takes the emeralds Froggy and Eggman hold, becomes Chaos 6
 * and swallows Froggy, and Sonic comes too late */
#include <ninja.h>

#include "ev00de/emerald_sky_textures.h"
#include "ev00de/frog_action_2.h"
#include "ev00de/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_models.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/chaos4.h"
#include "shared/chaos6.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/frog.h"
#include "shared/move.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 49
#define TEXT_2_LENGTH 52
#define TEXT_3_LENGTH 28
#define TEXT_4_LENGTH 36
#define TEXT_5_LENGTH 48
#define TEXT_6_LENGTH 61
#define TEXT_7_LENGTH 16
#define TEXT_8_LENGTH 35
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 20
#define TEXT_11_LENGTH 20
#define TEXT_12_LENGTH 20
#define TEXT_13_LENGTH 28
#define TEXT_14_LENGTH 25
#define TEXT_15_LENGTH 53
#define TEXT_16_LENGTH 46
#define TEXT_17_LENGTH 22
#define TEXT_18_LENGTH 36
#define TEXT_19_LENGTH 32
#define TEXT_20_LENGTH 21
#define TEXT_21_LENGTH 44
#include "ev00de/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_sprint_action;
extern NJS_ACTION sonic_skid_action;

extern NJS_TEXLIST event_eggmobile_texlist;

extern NJS_ACTION ev00de_big_action_0;
extern NJS_ACTION ev00de_big_action_1;
extern NJS_ACTION ev00de_big_action_2;
extern NJS_ACTION ev00de_sonic_action_0;
extern NJS_ACTION ev00de_sonic_action_1;
extern NJS_ACTION ev00de_frog_action_0;
extern NJS_ACTION ev00de_frog_action_1;
extern NJS_ACTION ev00de_eggmobile_action_0;
extern NJS_ACTION ev00de_eggmobile_action_1;
extern NJS_ACTION ev00de_emerald_action;
extern NJS_TEXLIST ev00de_emerald_yellow_texlist;

static struct event_path_point ev00de_emerald_yellow_points[8] = {
#include "ev00de/script/ev00de_emerald_yellow_points.inc"
};

static struct event_path ev00de_emerald_yellow_path = {
    8, ev00de_emerald_yellow_points};

static struct event_path_point ev00de_emerald_sky_points[13] = {
#include "ev00de/script/ev00de_emerald_sky_points.inc"
};

static struct event_path ev00de_emerald_sky_path = {13,
                                                    ev00de_emerald_sky_points};

static struct task *ev00de_frog = NULL;
static struct task *ev00de_emerald_yellow = NULL;
static struct task *ev00de_emerald_sky = NULL;
/* the slot holds a white fade as Chaos 4 becomes Chaos 6, then a blue one,
 * then a white flash */
static struct task *ev00de_flash = NULL;

void ev00de_script(int phase)
{
    /* @hack spelling: every voice is played through voice_play, a local. SHC
     * then keeps event_voice_play's address in R8 (0x0cb80600). called
     * directly, R8 holds event_message_show's, and 87 of 3054 words differ */
    void (*voice_play)(int voice);
    struct task_work *work;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00de.mlt");
        event_sound_init(4);
        event_reset_player(0);
        event_create_player(5, sonic_step, 185.0F, 766.0F, -199.0F, 0xf800,
                            0x2281, 0);
        event_place_actor(&ev00de_emerald_yellow, 973.3F, 126.5F, 823.5F,
                          0xfff0, 0x19e0, 0x0054);
        event_place_actor(&ev00de_emerald_sky, 973.3F, 126.5F, 823.5F, 0xfff0,
                          0x19e0, 0x0054);
        event_place_task(&ev00de_frog, event_frog_create, 51.2F, 748.5F,
                         -463.0F, 0, 0xa900, 0);
        event_wait(1);
        event_set_pos(ev00de_emerald_yellow, 4.8F, 740.5F, -440.6F);
        event_set_ang(ev00de_emerald_yellow, 0x0600, 0x9ff3, 0);
        event_set_pos(ev00de_emerald_sky, 2.8F, 740.5F, -440.6F);
        event_set_ang(ev00de_emerald_sky, 0x0600, 0x9ff3, 0);
        event_set_pos(event_get_player(0), 62.5F, 747.5F, -456.6F);
        event_set_ang(event_get_player(0), 0x0600, 0x9ff3, 0);
        event_set_pos(event_get_player(5), -169.7F, 594.0F, -809.4F);
        event_set_ang(event_get_player(5), 0x0300, 0x0100, 0);
        event_set_flags(ev00de_frog, EVENT_IGNORE_GROUND);
        event_set_flags(ev00de_emerald_yellow, EVENT_IGNORE_GROUND);
        event_set_flags(ev00de_emerald_sky, EVENT_IGNORE_GROUND);
        event_play_action(ev00de_frog, &ev00de_frog_action_1,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00de_emerald_yellow, &ev00de_emerald_action,
                          &ev00de_emerald_yellow_texlist, 1.0F, EVENT_REPEAT,
                          1);
        event_play_action(ev00de_emerald_sky, &ev00de_emerald_action,
                          &ev00de_emerald_sky_texlist, 1.0F, EVENT_REPEAT, 1);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 1);
        event_set_shadow(ev00de_frog, 0.6F);
        event_eggmobile_create(77.4F, 825.8F, -429.1F, 0, 0, 0);
        event_wait(3);
        event_chaos4_create(5.3F, 745.0F, -500.7F, 0, 0x6800, 0, 80);
        event_chaos6_hold(-321.0F, 592.5F, -289.85F, 0, 0, 0, 100);
        event_wait(1);
        event_chaos4_set_speed(1.0F);
        event_chaos6_melt();
        event_set_pos(event_get_player(0), 62.5F, 747.5F, -456.6F);
        event_set_ang(event_get_player(0), 0x0900, 0xa000, 0);
        stream_request_music(MUSIC_EVTBGM04);
        event_sound_start(1, 1227, 1800);
        event_sound_fade_volume(1, 20, 1);
        event_sound_fade_pan(1, 20, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -40.45F, 756.0F, -470.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff54, 0xb0e5, 0xffb2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, -34.18F, 758.17F,
                             -495.36F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 110, 0x0154, 0xbae5, 0xffb2);
        event_wait(125);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -22.32F, 751.1F, -518.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c29, 0x84a9, 0xffe0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 15.93F, 766.0F, -529.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 55, 0x0429, 0x6aa9, 0xfee0);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 34.2F, 775.4F, -495.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x1029, 0x4da8, 0xfee0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 85, 28.45F, 769.93F, -480.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 85, 0x1e29, 0x19a8, 0xfbe0);
        event_wait(25);
        event_camera_set_fov(EVENT_EASE_LINEAR, 140, 0x29f5);
        event_wait(85);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 83.2F, 749.68F, -491.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f29, 0x4ba8, 0xfee0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 88.13F, 749.44F, -466.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1329, 0x3da8, 0xfee0);
        event_wait(110);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 61.0F, 756.0F, -487.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b29, 0x81a9, 0xffe0);
        voice_play = event_voice_play;
        voice_play(1867);
        event_message_show(TEXT_1);
        event_eggmobile_fly_turn(11.0F, 765.0F, -411.0F, 0, -0x5600, 0, 100, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0b29, 0x74c2, 0xffe0);
        event_wait(120);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_voice_wait();
        event_message_clear();
        event_set_pos(event_get_player(0), 62.5F, 747.5F, -456.6F);
        event_set_ang(event_get_player(0), 0x0600, 0x92f3, 0);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 27.8F, 778.6F, -477.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x01f3, 0);
        voice_play(1868);
        event_message_show(TEXT_2);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 25, 0x0400, 0x10f3, 0);
        event_wait(23);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 8, 27.0F, 778.8F, -479.4F);
        event_wait(40);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 39.5F, 765.5F, -479.96F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 42.6F, 757.6F, -475.3F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, ev00de_frog, 0.0F, 0.0F,
                                  0.0F, 0);
        event_wait(40);
        event_message_close();
        event_set_pos(ev00de_emerald_sky, 18.5F, 781.0F, -415.4F);
        event_set_ang(ev00de_emerald_sky, 0, 0, 0);
        event_play_action(event_eggmobile_get_task(),
                          &ev00de_eggmobile_action_1, &event_eggmobile_texlist,
                          1.1F, EVENT_REPEAT, 0);
        voice_play(1869);
        event_message_show(TEXT_3);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 14.3F, 781.92F, -414.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xedb3, 0x6031, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 17, 21.7F, 785.36F, -419.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 17, 0xedb3, 0x6531, 0);
        event_wait(60);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 56.468F, 747.63F, -460.75F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x056a, 0x2474, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x146a, 0x2474, 0x0800);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1dde);
        event_wait(170);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 51.9F, 753.3F, -480.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf66a, 0x7c74, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 51.4F, 752.0F, -475.0F);
        event_play_action(ev00de_frog, &ev00de_frog_action_1,
                          &event_frog_texlist, 1.8F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(ev00de_frog, &ev00de_frog_action_1,
                          &event_frog_texlist, 3.3F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(ev00de_frog, &ev00de_frog_action_1,
                          &event_frog_texlist, 4.0F, EVENT_REPEAT, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 48.7F, 748.63F, -455.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb6a, 0xeed9, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 44.37F, 749.52F, -459.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xfb6a, 0xdad9, 0);
        event_wait(25);
        event_frog_set_shape(1);
        event_wait(55);
        event_frog_set_shape(6);
        event_play_action(ev00de_frog, &ev00de_frog_action_1,
                          &event_frog_texlist, 0.9F, EVENT_REPEAT, 0);
        event_wait(10);
        event_set_pos(ev00de_emerald_yellow, 51.2F, 748.5F, -463.0F);
        event_set_ang(ev00de_emerald_yellow, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 41.63F, 747.5F, -465.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0e6a, 0xbcd9, 0);
        event_play_action(ev00de_frog, &ev00de_frog_action_1,
                          &event_frog_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(40);
        event_frog_set_shape(0);
        event_wait(20);
        /* @hack spelling: the third argument reads the emerald's work through
         * work, a local. SHC then keeps -456.6 in FR12 for the player's
         * event_set_pos calls (0x0cb80136). read in place, FR12 holds 62.5,
         * and 25 of 3054 words differ */
        event_rays_create(
            ev00de_emerald_yellow->work->pos.x,
            ev00de_emerald_yellow->work->pos.y,
            (work = ev00de_emerald_yellow->work, work->pos.z - 0.5F), 2, 22,
            0.2F, 0.7F, 0xffffff64, 0x00ffff1e);
        event_move(ev00de_emerald_yellow, 51.2F, 748.35F, -463.0F, 49.11F,
                   748.35F, -466.0F, 110);
        event_wait(140);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 47.4F, 749.33F, -490.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0600, 0x88fb, 0x0080);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 35.66F, 750.1F, -497.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0600, 0x88fb, 0x0080);
        event_wait(20);
        event_rays_kill();
        event_wait(50);
        event_play_path(ev00de_emerald_yellow, &ev00de_emerald_yellow_path,
                        0.08F, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 20, ev00de_emerald_yellow,
                                  0.0F, 0.0F, 0.0F, 0);
        event_wait(40);
        event_play_path(ev00de_emerald_sky, &ev00de_emerald_sky_path, 0.25F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 55.8F, 745.5F, -501.4F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 20, ev00de_emerald_sky,
                                  0.0F, 0.0F, 0.0F, 0);
        event_sound_start(1, 1223, 1800);
        event_sound_fade_volume(1, 110, 1);
        event_sound_fade_pan(1, -30, 1);
        event_wait(30);
        stream_request_music(MUSIC_EVTBGM01);
        event_rays_create(11.0F, 780.0F, -497.2F, 3, 10, 0.4F, 2.0F, 0xffffff64,
                          0x00ffff1e);
        event_wait(15);
        event_rays_kill();
        event_rays_create(14.77F, 777.07F, -494.97F, 3, 11, 0.5F, 2.6F,
                          0xff1e1eff, 0x008282ff);
        event_wait(70);
        ev00de_flash = event_fade_create(0.00002F, 0.3F, 1.0F, 1.0F, 1.0F);
        event_kill_task(&ev00de_emerald_yellow);
        event_kill_task(&ev00de_emerald_sky);
        event_chaos4_kill();
        event_chaos6_set_pos(-5.3F, 745.0F, -500.7F);
        event_chaos6_set_ang(0, 0x6800, 0);
        event_chaos6_form();
        event_play_action(event_eggmobile_get_task(),
                          &ev00de_eggmobile_action_0, &event_eggmobile_texlist,
                          1.1F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev00de_big_action_2,
                          &big_texlist, 1.2F, EVENT_REPEAT, 0);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 23.7F, 751.1F, -470.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0x12d1, 0x0018);
        event_camera_set_pos(EVENT_EASE_LINEAR, 25, 26.7F, 784.6F, -464.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0xea00, 0x12d1, 0x0018);
        event_rays_kill();
        event_wait(30);
        if (ev00de_flash != NULL) {
            event_fade_set_speed(ev00de_flash, -0.01F);
        }
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 51.3F, 757.8F, -436.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0, 0x18d1, 0x0018);
        event_wait(80);
        if (ev00de_flash != NULL) {
            event_fade_set_speed(ev00de_flash, -0.00000001F);
        }
        event_sound_stop(1);
        event_play_action(event_get_player(0), &ev00de_big_action_2,
                          &big_texlist, 1.4F, EVENT_REPEAT, 0);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 15.0F, 783.8F, -420.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf200, 0x6d0c, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 14.6F, 782.8F, -417.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xf254, 0x6d0c, 0);
        voice_play(1871);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_message_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 56.8F, 750.2F, -473.15F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd50, 0x7e0c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 8, 50.8F, 750.2F, -473.45F);
        event_wait(17);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 15.0F, 778.93F, -416.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff50, 0x680c, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0550, 0x680c, 0);
        voice_play(1870);
        event_message_show(TEXT_5);
        event_play_action(ev00de_frog, &ev00de_frog_action_1,
                          &event_frog_texlist, 1.5F, EVENT_REPEAT, 0);
        event_wait(85);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 58.6F, 758.0F, -470.53F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0d50, 0x8f0c, 0);
        event_voice_wait();
        event_camera_look_clear();
        event_message_show(TEXT_6);
        voice_play(1872);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 5.9F, 753.5F, -451.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1154, 0x220c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 28.3F, 752.7F, -470.16F);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 13.7F, 788.5F, -419.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe354, 0x770c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 13.0F, 786.0F, -416.8F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 52.7F, 753.5F, -471.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1154, 0x9a0c, 0xfb00);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 32.2F, 795.8F, -474.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xea54, 0x270c, 0xfb00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 38.73F, 771.8F, -471.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0xf854, 0x270c, 0);
        event_wait(40);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.4F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 56.9F, 750.0F, -463.11F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0056, 0x2d0c, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0454, 0x2d0c, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 100, 0x24fa);
        event_wait(120);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 80.87F, 763.0F, -460.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf6cc, 0x40da, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 80.6F, 763.1F, -466.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xf7c8, 0x3ada, 0x0200);
        event_play_action(ev00de_frog, &ev00de_frog_action_2,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev00de_frog, 53.0F, 750.0F, -461.2F, 2.8F, 773.0F, -486.0F,
                   130);
        event_run_wait(ev00de_frog);
        event_wait(120);
        event_play_action(ev00de_frog, &ev00de_frog_action_0,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        EVENT_KILL(ev00de_flash);
        ev00de_flash = event_fade_create(0.00002F, 0.3F, 0.6F, 0.6F, 1.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 8.87F, 773.7F, -486.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0x3629, 0x0400);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 9.57F, 771.6F, -481.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x123c, 0x3000, 0x0400);
        event_wait(40);
        event_frog_set_shape(2);
        event_sound_play(6, 60, 0, 0);
        event_wait(60);
        event_sound_play(6, 90, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, 6.8F, 771.6F, -482.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x053c, 0x3029, 0x0400);
        event_wait(50);
        event_message_clear();
        event_message_close();
        EVENT_KILL(ev00de_flash);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -64.17F, 761.7F, -464.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x103c, 0xd229, 0x0400);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -82.3F, 757.7F, -506.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x163c, 0xc929, 0x0400);
        event_rays_create(-35.4F, 791.2F, -523.9F, 4, 10, 0.5F, 2.6F,
                          0xffdcdcb4, 0x00b4b428);
        event_wait(20);
        event_sound_start(2, 1224, 1800);
        event_sound_fade_volume(2, 128, 1);
        event_sound_fade_pan(2, 20, 1);
        ev00de_flash = event_fade_create(0.015F, 0.3F, 1.0F, 1.0F, 0.95F);
        event_wait(60);
        if (ev00de_flash != NULL) {
            event_fade_set_speed(ev00de_flash, -0.05F);
        }
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -70.17F, 864.7F, -497.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe03c, 0xd729, 0x0400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -28.17F, 820.7F, -445.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x083c, 0x0229, 0x0400);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -7.65F, 849.7F, -412.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0xe83c, 0x0329, 0xfd00);
        event_sound_stop(2);
        event_sound_play(1226, 128, -10, 0);
        event_look_clear(event_get_player(0));
        event_set_pos(event_get_player(5), 171.6F, 747.0F, -397.4F);
        event_set_ang(event_get_player(5), 0, 0x9600, 0);
        event_set_pos(event_get_player(0), 66.1F, 759.5F, -458.3F);
        event_set_ang(event_get_player(0), 0x0900, 0x9af3, 0);
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(5), &sonic_sprint_action,
                          &sonic_texlist, 1.2F, EVENT_REPEAT, 0);
        event_wait(5);
        event_wait(40);
        event_sound_play(1225, 128, -10, 0);
        event_wait(40);
        event_rays_kill();
        event_run_to_speed(event_get_player(5), 70.6F, 747.0F, -477.0F, 1.6F,
                           0.1F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 39.5F, 756.7F, -469.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0xb129, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0500, 0xbb29, 0);
        stream_request_music(MUSIC_THESONIC);
        event_run_wait(event_get_player(5));
        event_sound_play(18, 100, 0, 0);
        event_set_ang(event_get_player(5), 0xf800, 0xb500, 0);
        event_play_action(event_get_player(5), &sonic_skid_action,
                          &sonic_texlist, 1.4F, 0, 16);
        event_wait_motions(event_get_player(5));
        event_play_action(event_get_player(5), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_17);
        voice_play(1104);
        event_message_show(TEXT_7);
        event_voice_wait();
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 80.5F, 747.7F, -457.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1303, 0x2c27, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 113.6F, 747.5F, -460.55F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0c03, 0x3727, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 90, 0x25b0);
        event_wait(1);
        voice_play(1107);
        event_message_show(TEXT_8);
        event_wait(100);
        event_voice_wait();
        voice_play(1800);
        event_message_show(TEXT_9);
        event_voice_wait();
        event_look_clear(event_get_player(0));
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 24, 95.6F, 752.7F, -456.55F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 24, 0x0a03, 0x4c27, 0);
        voice_play(1105);
        event_message_show(TEXT_10);
        event_play_action(event_get_player(5), &ev00de_sonic_action_0,
                          &sonic_texlist, 0.7F, 0, 16);
        event_wait(10);
        event_run_to_speed(event_get_player(5), 68.6F, 747.0F, -477.0F, 0.8F,
                           0.06F);
        event_wait_motions(event_get_player(5));
        event_play_action(event_get_player(5), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_set_ang(event_get_player(5), 0, 0xc000, 0);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 30.96F, 777.7F, -411.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x02dd, 0x480c, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 27.4F, 778.12F, -416.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 20, 0x05dd, 0x500c, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        voice_play(1106);
        event_message_show(TEXT_11);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 34, 19.9F, 782.4F, -420.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 34, 0xfadd, 0x620c, 0);
        event_wait(40);
        EVENT_KILL(ev00de_flash);
        voice_play(1874);
        event_message_show(TEXT_12);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 89.4F, 762.0F, -470.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0884, 0x4351, 0);
        event_set_pos(event_get_player(0), 62.5F, 747.5F, -456.6F);
        event_set_ang(event_get_player(0), 0xfcd0, 0xa0ba, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x0884, 0x3e51, 0);
        event_wait(70);
        event_set_pos(ev00de_frog, -18.7F, 775.6F, -488.0F);
        event_set_ang(ev00de_frog, 0, 0x5700, 0);
        event_set_ang(event_get_player(5), 0, 0x9600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 8.9F, 775.5F, -446.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0084, 0x1a51, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 2.9F, 775.5F, -454.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0084, 0x1851, 0);
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_18);
        voice_play(1875);
        event_message_show(TEXT_13);
        event_voice_wait();
        event_set_ang(event_get_player(0), 0, 0xac00, 0);
        event_set_ang(event_get_player(5), 0, 0xa400, 0);
        event_look_at_task(event_get_player(5), event_get_player(0), 0.0F, 8.0F,
                           0.0F);
        voice_play(1108);
        event_message_show(TEXT_14);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 42.0F, 767.6F, -457.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf184, 0xc551, 0);
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_19);
        event_wait(5);
        event_wait(5);
        event_play_action(event_get_player(0), &ev00de_big_action_0,
                          &big_texlist, 0.9F, 0, 8);
        event_play_action(event_get_player(0), &ev00de_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_message_clear();
        event_message_close();
        event_look_clear(event_get_player(5));
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_20);
        event_set_ang(event_get_player(5), 0, 0xa400, 0);
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(5), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_21);
        voice_play(1109);
        event_message_show(TEXT_15);
        event_play_action(event_get_player(5), &ev00de_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 1);
        event_play_action(event_get_player(5), &ev00de_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_look_clear(event_get_player(0));
        event_wait(140);
        event_talk_stop(event_get_player(5));
        event_message_close();
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(5), &sonic_sprint_action,
                          &sonic_texlist, 1.4F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 69.1F, 755.6F, -429.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0584, 0x1b51, 0);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 90.2F, 755.6F, -446.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0984, 0x2751, 0);
        event_run_to_speed(event_get_player(5), -89.5F, 766.5F, -520.3F, 0.46F,
                           0.06F);
        event_wait(80);
        event_set_pos(event_get_player(0), 62.5F, 747.5F, -456.6F);
        event_set_ang(event_get_player(0), 0x0900, 0xa000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 40.0F, 760.3F, -512.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc1d, 0x999a, 0xffb2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 27.6F, 752.2F, -498.47F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0190, 0x9c5a, 0xffb0);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 49.4F, 764.3F, -456.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf41d, 0xca9a, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 55.4F, 765.54F, -449.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0xf81d, 0xbc9a, 0);
        voice_play(1876);
        event_message_show(TEXT_16);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 42.96F, 748.3F, -457.72F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x123a, 0xbe62, 0xffb2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 42.3F, 760.0F, -467.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xfd3a, 0xaf62, 0xffb2);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 49.8F, 761.3F, -464.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x023a, 0xaf62, 0xffb2);
        event_wait(60);
        event_sound_play(1225, 128, -10, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 113.0F, 756.7F, -438.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0d3a, 0x2f62, 0xffb2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, 114.5F, 749.1F, -449.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x0f3a, 0x2f62, 0xffb2);
        event_wait(160);
        event_sound_stop(1);
        event_voice_wait();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_message_clear();
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_voice_stop();
        event_chaos4_kill();
        event_kill_player(5);
        event_kill_player(3);
        event_kill_player(4);
        event_kill_task(&ev00de_emerald_yellow);
        event_kill_task(&ev00de_emerald_sky);
        event_kill_task(&ev00de_frog);
        event_eggmobile_kill();
        event_rays_kill();
        EVENT_KILL(ev00de_flash);
        event_chaos6_release();
        /* event_kill_task has set ev00de_emerald_yellow and ev00de_emerald_sky
         * to null, so these calls clear nothing */
        event_clear_paths(ev00de_emerald_yellow);
        event_clear_paths(ev00de_emerald_sky);
        event_sound_stop_all();
        event_wait(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00de_textures[] = {
    {"M_EM_YELLOW", &ev00de_emerald_yellow_texlist},
    {"M_EM_SKY", &ev00de_emerald_sky_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"CHAOS4_COMMON", &event_chaos4_texlist},
    {"SHAPE_FROG", &event_frog_texlist},
    {NULL, NULL},
};
