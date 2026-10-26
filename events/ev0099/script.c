/* EV0099, Knuckles' story: on the Egg Carrier, Knuckles finds Sonic as Eggman
 * flees. Sonic gives chase and leaves Knuckles to face Chaos 6 */
#include <ninja.h>

#include "ev0099/eggmobile_action.h"
#include "ev0099/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "shared/chaos6.h"
#include "shared/eggmobile.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 26
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 31
#define TEXT_4_LENGTH 23
#define TEXT_5_LENGTH 22
#define TEXT_6_LENGTH 51
#define TEXT_7_LENGTH 58
#define TEXT_8_LENGTH 30
#define TEXT_9_LENGTH 41
#define TEXT_10_LENGTH 58
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 4
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 4
#define TEXT_17_LENGTH 3
#include "ev0099/script/text.inc"

/* @hack type: knuckles_texlist declared as an array, which script.c passes by
 * its name. SHC then keeps event_set_pos's address in R10 (0x0cb80026) and
 * loads event_set_ang's for each call, as at 0x0cb8009e. as one NJS_TEXLIST
 * passed as &knuckles_texlist, SHC holds event_set_ang's in R8 and
 * event_set_pos's in R9, script.c's code ends 8 bytes short of the image's, and
 * 1011 of 1103 words differ */
extern NJS_TEXLIST knuckles_texlist[];
/* @hack type: knuckles_stand_action declared as an array, which script.c passes
 * by its name. SHC then also keeps event_set_pos's address in R10 and loads
 * event_set_ang's for each call. as one NJS_ACTION passed as
 * &knuckles_stand_action, SHC holds event_set_ang's in R8 and event_set_pos's
 * in R9, script.c's code ends 8 bytes short of the image's, and 998 of 1103
 * words differ */
extern NJS_ACTION knuckles_stand_action[];
extern NJS_ACTION knuckles_run_action;
/* @hack type: sonic_texlist declared as an array, which script.c passes by its
 * name. SHC then keeps event_set_pos's address in R10 and event_play_action's
 * in R9 (0x0cb80026, 0x0cb80102). as one NJS_TEXLIST passed as &sonic_texlist,
 * R9 and R10 trade their callees, and 26 of 1103 words differ */
extern NJS_TEXLIST sonic_texlist[];
extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_jump_land_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_ACTION ev0099_knuckles_action_0;
extern NJS_ACTION ev0099_knuckles_action_1;

void ev0099_script(int phase)
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
        sound_load_file("e_0009.mlt");
        stream_request_music(MUSIC_EGCARER1);
        event_set_pos(event_get_player(0), -79.19F, 743.22F, -549.67F);
        event_set_ang(event_get_player(0), 0, 0xa9f4, 0xfccf);
        event_create_player(2, sonic_step, -44.9F, 745.65F, -231.87F, 0x02ce,
                            0x725e, 0x0119);
        event_eggmobile_create(0.0F, 780.0F, -386.04F, 0, 0x4000, 0);
        event_chaos6_hold(16.0F, 560.0F, -30.0F, 0, 0, 0, 0x038e);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), knuckles_stand_action,
                          knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), &ev0099_eggmobile_action,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_eggmobile_set_bob(0x0100, 0.5F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 161.73F, 833.99F, -87.22F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf494, 0x1a0c, 0xfa00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 141.27F, 837.52F, -73.28F);
        event_wait(90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -80.66F, 752.11F, -184.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x01ca, 0xeb29, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -88.49F, 751.4F, -170.81F);
        sound_play_at_for(1227, EVENT_SOUND_ID, 1, 30, 0.0F, 780.0F, -386.04F,
                          180);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -21.03F, 780.5F, -342.65F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x07ca, 0xe8ad, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, -27.75F, 780.5F, -346.99F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x07ca, 0xe5a5, 0);
        event_set_pos(event_get_player(0), 139.57F, 743.65F, -265.68F);
        event_set_ang(event_get_player(0), 0x01cb, 0xbff7, 0xfd71);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          knuckles_texlist, 2.5F, EVENT_REPEAT, 0);
        event_voice_play(1119);
        event_message_show(TEXT_1);
        event_play_action(event_eggmobile_get_task(), &ev0099_eggmobile_action,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(100);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -45.56F, 747.81F, -254.17F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x02ca, 0x8431, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 45, -67.74F, 748.2F, -238.33F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 45, 0x02ca, 0xb831, 0);
        event_wait(45);
        event_run_to_speed(event_get_player(0), 21.97F, 747.63F, -253.9F, 1.75F,
                           0.06F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, -62.6F, 748.2F, -264.83F);
        event_voice_play(1649);
        event_message_show(TEXT_2);
        event_wait(30);
        event_message_close();
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), knuckles_stand_action,
                          knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0x0317, 0x8772, 0);
        event_look_at(event_get_player(0), 0.0F, 803.0F, -387.04F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_stand_action,
                          sonic_texlist, 0.8F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           10.0F, 0.0F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.16F, 750.65F, -257.31F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x02ca, 0xb931, 0);
        event_wait(45);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x11c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 65.22F, 763.05F, -288.32F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf8ca, 0x5831, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 79.09F, 766.04F, -297.68F);
        event_talk_start(event_get_player(2), TEXT_11);
        event_wait(10);
        event_voice_play(1650);
        event_message_show(TEXT_3);
        event_wait(70);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 18.65F, 778.13F, -373.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f94, 0x2e0c, 0xfd00);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(5);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 18.65F, 778.13F, -373.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f94, 0x2e0c, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 69.36F, 771.59F, -342.66F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0d94, 0x2c0c, 0xfd00);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_voice_play(1124);
        event_eggmobile_fly_turn(0.0F, 785.0F, -386.04F, 0, -0x4000, 0, 50, 0);
        event_message_show(TEXT_4);
        event_wait(25);
        event_run_to_speed(event_get_player(2), -217.33F, 748.34F, -350.01F,
                           1.75F, 0.06F);
        event_wait(15);
        event_look_at(event_get_player(0), -50.33F, 748.34F, -350.01F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, 73.58F, 769.95F, -340.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 35, 0x0d94, 0x2d0c, 0xfd00);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_eggmobile_fly_turn(-300.0F, 755.0F, -400.0F, 0, 0, 0, 90, 1);
        sound_play_at(1231, 0, 0, 120, 0.0F, 780.0F, -386.04F);
        event_wait(15);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 6.53F, 754.66F, -268.14F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0194, 0x390c, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 45, 44.75F, 753.63F, -254.47F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_trot_action,
                          sonic_texlist, 2.75F, EVENT_REPEAT, 0);
        event_voice_play(1123);
        event_message_wait(20, TEXT_5);
        event_message_close();
        event_wait(10);
        event_kill_player(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 15.56F, 749.26F, -262.51F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0894, 0xa80c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 360, 12.79F, 749.26F, -258.35F);
        event_message_show(TEXT_6);
        event_voice_play(1652);
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(70);
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(50);
        event_talk_stop(event_get_player(0));
        event_wait(15);
        event_eggmobile_kill();
        event_message_show(TEXT_7);
        event_voice_play(1653);
        event_talk_start(event_get_player(0), TEXT_13);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_chaos6_set_pos(5.4F, 750.0F, -400.0F);
        event_chaos6_set_ang(0, 0x4000, 0);
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_wait(15);
        stream_stop_music();
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), 0.0F, 803.0F, -387.04F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 8.67F, 751.87F, -235.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0994, 0xf80c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 6.9F, 748.38F, -220.75F);
        stream_request_music(MUSIC_EVTBGM01);
        event_message_show(TEXT_8);
        event_voice_play(1654);
        event_talk_start(event_get_player(0), TEXT_14);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_message_show(TEXT_9);
        event_voice_play(1655);
        event_talk_start(event_get_player(0), TEXT_15);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_play_action(event_get_player(0), &ev0099_knuckles_action_0,
                          knuckles_texlist, 1.0F, 0, 2);
        event_play_action(event_get_player(0), &ev0099_knuckles_action_1,
                          knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_message_show(TEXT_10);
        event_voice_play(1656);
        event_talk_start(event_get_player(0), TEXT_16);
        event_wait(70);
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_17);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(60);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_kill_player(2);
        event_eggmobile_kill();
        event_chaos6_release();
        event_set_pos(event_get_player(0), 0.0F, 744.0F, -568.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0099_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"SONIC", sonic_texlist},
    {NULL, NULL},
};
