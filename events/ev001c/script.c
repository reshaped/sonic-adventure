/* EV001C, Sonic's story: Sonic and Big find Chaos 6 with Froggy in it. Eggman
 * sets Chaos 6 on Sonic */
#include <ninja.h>

#include "ev001c/script.h"
#include "ev001c/sonic_action_0.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big.h"
#include "player/textures.h"
#include "shared/chaos6.h"
#include "shared/eggmobile.h"
#include "shared/frog.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 34
#define TEXT_3_LENGTH 19
#define TEXT_4_LENGTH 20
#define TEXT_5_LENGTH 19
#define TEXT_6_LENGTH 28
#define TEXT_7_LENGTH 25
#define TEXT_8_LENGTH 24
#define TEXT_9_LENGTH 33
#define TEXT_10_LENGTH 55
#define TEXT_11_LENGTH 37
#define TEXT_12_LENGTH 47
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 64
#define TEXT_15_LENGTH 39
#define TEXT_16_LENGTH 2
#define TEXT_17_LENGTH 5
#define TEXT_18_LENGTH 3
#include "ev001c/script/text.inc"

extern NJS_ACTION sonic_jump_land_action;

extern NJS_ACTION ev001c_frog_action;
extern NJS_ACTION ev001c_eggmobile_action_0;
extern NJS_ACTION ev001c_eggmobile_action_1;
extern NJS_ACTION ev001c_eggmobile_action_2;
extern NJS_ACTION ev001c_eggmobile_action_3;
extern NJS_ACTION ev001c_eggmobile_action_4;
extern NJS_ACTION ev001c_eggmobile_action_5;
extern NJS_ACTION ev001c_eggmobile_action_6;
extern NJS_TEXLIST event_eggmobile_texlist;

static struct task *ev001c_frog = NULL;

void ev001c_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x271c);
        event_set_pos(event_get_player(0), 104.3F, 744.0F, -522.0F);
        event_set_ang(event_get_player(0), 0xf829, 0xc6a9, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.7F, EVENT_REPEAT, 16);
        event_create_player(2, big_step, 115.0F, 744.0F, -480.0F, 0, 0xb8a9, 0);
        event_place_task(&ev001c_frog, event_frog_create, 18.255F, 773.8F,
                         -524.37F, 0, 0x8000, 0);
        event_chaos6_hold(5.3F, 745.0F, -500.7F, 0, -0x500, 0, NJM_DEG_ANG(5));
        event_eggmobile_create(20.0F, 780.0F, -421.0F, 0, 0xa000, 0);
        event_wait(1);
        stream_request_music(MUSIC_EVTBGM01);
        event_set_flags(ev001c_frog, EVENT_IGNORE_GROUND);
        event_set_pos(ev001c_frog, 24.46F, 763.885F, -516.44F);
        event_set_ang(ev001c_frog, 0xf800, -0xa630, 0);
        event_play_action(ev001c_frog, &ev001c_frog_action, &event_frog_texlist,
                          0.2F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev001c_big_action_0,
                          &big_texlist, 0.7F, EVENT_REPEAT, 16);
        event_frog_set_shape(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 111.932F, 748.608F, -522.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f29, 0x42a9, 0xffe0);
        event_wait(10);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 138.3F, 744.746F,
                             -521.203F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0x0f29, 0x3da9, 0xffe0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 130, 0x31c7);
        /* @hack redundant: a wait of 130 that never runs. with it, SHC loads
         * 130 into R9 at 0x0cb80274, and 0x0f29 into R8 at 0x0cb8025a. it
         * also loads 744.0 into FR13 at 0x0cb8006c, and 0.8 into FR14 at
         * 0x0cb806b4. without it, each pair of registers trades its values,
         * and 19 of 1180 words differ */
        if (0) {
            event_wait(130);
        }
        event_wait(129);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 133.572F, 744.351F,
                             -516.46F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0f29, 0x3da9, 0xffe0);
        event_wait(49);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 134.15F, 747.171F,
                             -507.42F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 55, 0x0c29, 0x3aa9, 0xffe0);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 98.0F, 746.06F, -516.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1030, 0xe0b0, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 99.3F, 746.83F, -517.4F);
        event_voice_play(0x450);
        event_talk_start(event_get_player(0), TEXT_16);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 77.62F, 750.48F, -531.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1030, 0x41b0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 79.11F, 750.48F, -496.0F);
        event_wait(30);
        event_voice_play(0x453);
        event_talk_start(event_get_player(0), TEXT_17);
        event_message_show(TEXT_2);
        event_wait(50);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 98.74F, 746.0F, -515.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1030, 0xe0b0, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 97.26F, 746.15F, -516.7F);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_voice_play(0x451);
        event_message_show(TEXT_3);
        event_play_action(event_get_player(0), &ev001c_sonic_action_0,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_message_close();
        event_set_ang(event_get_player(0), 0xf829, 0xc2a9, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 80, 0x31c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 103.0F, 746.59F, -531.34F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x1030, 0x7ab0, 0xfc00);
        event_voice_play(0x452);
        event_message_show(TEXT_4);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 96.84F, 746.168F,
                             -539.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x0a29, 0x7da9, 0);
        event_voice_wait();
        event_run_to_speed(event_get_player(2), 110.0F, 746.0F, -482.43F, 0.6F,
                           0.0F);
        event_voice_play(0x752);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 44.82F, 757.47F, -531.28F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0a29, 0x59a9, 0xffe0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 70, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 39.34F, 759.2F, -527.28F);
        event_voice_play(0x753);
        event_message_show(TEXT_6);
        event_wait(60);
        event_message_close();
        event_clear_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 87.54F, 745.8F, -539.62F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0729, 0x97b0, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &ev001c_sonic_action_4,
                          &sonic_texlist, 0.6F, EVENT_REPEAT, 8);
        event_voice_play(0x454);
        event_message_wait(60, TEXT_7);
        event_play_action(event_get_player(2), &ev001c_big_action_1,
                          &big_texlist, 0.8F, 0, 8);
        event_play_action(event_get_player(2), &ev001c_big_action_2,
                          &big_texlist, 0.8F, EVENT_REPEAT, 0);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 91.77F, 745.6F, -543.84F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0729, 0x9640, 0);
        event_wait(20);
        event_play_action(event_get_player(0), &ev001c_sonic_action_5,
                          &sonic_texlist, 0.7F, 0, 8);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_play(0x456);
        event_talk_start(event_get_player(0), TEXT_18);
        event_message_show(TEXT_8);
        event_wait(1);
        event_voice_wait();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 40, 0x31c7);
        event_talk_stop(event_get_player(0));
        event_clear_motions(event_get_player(2));
        event_run_to_speed(event_get_player(2), 91.0F, 757.0F, -290.0F, 0.0F,
                           2.0F);
        event_message_close();
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 27.6F, 788.86F, -420.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c30, 0x43b0, 0x0200);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_1, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 0);
        /* @hack redundant: a call of event_play_action with 0.8 that never
         * runs. with it, SHC loads event_play_action's address into R12 at
         * 0x0cb800a2, and event_wait's into R11 at 0x0cb8011e. it also loads
         * 744.0 into FR13 at 0x0cb8006c, and 0.8 into FR14 at 0x0cb806b4.
         * without it, each pair of registers trades its values, and 61 of
         * 1180 words differ */
        if (0) {
            event_play_action(NULL, NULL, NULL, 0.8F, 0, 0);
        }
        event_voice_play(0x457);
        event_message_show(TEXT_3);
        event_kill_player(2);
        event_voice_wait();
        event_camera_set_fov(EVENT_EASE_LINEAR, 260, 0x293f);
        event_camera_set_pos(EVENT_EASE_LINEAR, 260, 38.7356F, 807.9F,
                             -390.446F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 260, 0xf068, 0x01b3, 0);
        event_voice_play(0x458);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, 0, 8);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_3, &event_eggmobile_texlist,
                          1.0F, 0, 8);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_3, &event_eggmobile_texlist,
                          1.0F, 0, 8);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_message_show(TEXT_9);
        event_wait(1);
        event_voice_wait();
        event_voice_play(0x459);
        event_message_show(TEXT_10);
        event_wait(1);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_6, &event_eggmobile_texlist,
                          1.0F, 0, 16);
        event_voice_wait();
        event_voice_play(0x45a);
        event_message_show(TEXT_11);
        event_wait(1);
        event_wait_motions(event_eggmobile_get_task());
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 73.756F, 754.0F, -515.62F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf729, 0xc8a9, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 92.668F, 744.0F, -514.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0x1329, 0xda49, 0x0200);
        event_camera_set_fov(EVENT_EASE_LINEAR, 40, 0x18e4);
        event_voice_play(0x45b);
        event_talk_start(event_get_player(0), TEXT_18);
        event_message_show(TEXT_12);
        event_play_action(event_get_player(0), &ev001c_sonic_action_1,
                          &sonic_texlist, 0.8F, 0, 4);
        event_play_action(event_get_player(0), &ev001c_sonic_action_2,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 4);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 93.25F, 744.0F, -513.8885F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x1329, 0xdea9, 0x0200);
        event_wait(45);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x271c);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 45.17F, 785.5F, -432.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0x5000, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 38.068F, 793.89F,
                             -418.567F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0, 0x4000, 0);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_6, &event_eggmobile_texlist,
                          0.4F, 0, 8);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_4, &event_eggmobile_texlist,
                          1.0F, 0, 8);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_0, &event_eggmobile_texlist,
                          1.3F, EVENT_REPEAT, 6);
        event_voice_play(0x45c);
        event_message_show(TEXT_13);
        event_voice_wait();
        event_voice_play(0x45d);
        event_message_show(TEXT_14);
        event_voice_wait();
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_5, &event_eggmobile_texlist,
                          1.0F, 0, 6);
        event_play_action(event_eggmobile_get_task(),
                          &ev001c_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_message_close();
        event_play_action(event_get_player(0), &ev001c_sonic_action_3,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 8);
        event_voice_play(0x45e);
        event_message_show(TEXT_15);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 119.05F, 744.29F, -519.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0x0f00, 0x4300, 0x0200);
        event_message_close();
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, 118.88F, 744.43F, -522.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x0f00, 0x4300, 0x0200);
        event_wait(70);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev001c_frog);
        event_chaos6_release();
        event_eggmobile_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev001c_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"SHAPE_FROG", &event_frog_texlist},
    {"BIG", &big_texlist},
    {NULL, NULL},
};
