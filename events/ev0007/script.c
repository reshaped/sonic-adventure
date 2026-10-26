/* EV0007, Sonic's story: Eggman comes to Tails' workshop and demands the Chaos
 * Emerald. he then flies the Egg Hornet at Sonic and Tails */
#include <ninja.h>

#include "ev0007/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/eggmobile.h"
#include "shared/eggmobile_action.h"
#include "shared/hornet.h"
#include "shared/jets.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 30
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 67
#define TEXT_5_LENGTH 17
#define TEXT_6_LENGTH 71
#define TEXT_7_LENGTH 36
#define TEXT_8_LENGTH 39
#define TEXT_9_LENGTH 59
#define TEXT_10_LENGTH 23
#define TEXT_11_LENGTH 25
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 8
#include "ev0007/script/text.inc"

extern NJS_ACTION tails_sprint_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_ACTION ev0007_sonic_action;
extern NJS_ACTION ev0007_tails_action;

/* an actor with no motion, which isn't drawn. Sonic, Tails and the camera
 * look at it as the script moves it */
static struct task *ev0007_target = NULL;

void ev0007_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_0007.mlt");
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_init(4);
        event_reset_player(0);
        event_place_actor(&ev0007_target, 995.6F, 118.0F, 724.7F, 0, 0, 0);
        event_wait(1);
        event_set_flags(ev0007_target, EVENT_IGNORE_GROUND);
        event_set_pos(ev0007_target, 995.6F, 118.0F, 724.7F);
        event_set_ang(ev0007_target, 0, 0, 0);
        event_set_pos(event_get_player(0), 952.7F, 123.5F, 726.7F);
        event_set_ang(event_get_player(0), 0, 0x3000, 0);
        event_look_at_task(event_get_player(0), ev0007_target, 0.0F, 8.0F,
                           0.0F);
        event_create_player(2, tails_step, 947.3F, 123.5F, 705.77F, 0, 0x2d00,
                            0);
        event_wait(1);
        event_look_at_task(event_get_player(2), ev0007_target, 0.0F, 8.0F,
                           0.0F);
        event_set_pos(event_get_player(2), 946.3F, 123.5F, 698.77F);
        event_set_ang(event_get_player(2), 0, 0x3000, 0);
        event_eggmobile_create(1000.3F, 186.5F, 810.0F, 0, 0xd600, 0);
        event_wait(1);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), &event_eggmobile_action,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_hornet_create(1172.0F, 55.0F, 1120.7F, 0, 0x9700, 0);
        event_wait(1);
        event_hornet_set_speed(0.8F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 973.5F, 133.77F, 740.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0x1e00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 968.4F, 126.7F, 735.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0400, 0x2500, 0);
        event_voice_play(964);
        event_message_show(TEXT_1);
        event_play_action(event_get_player(0), &ev0007_sonic_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev0007_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(20);
        event_move(ev0007_target, 995.6F, 118.0F, 724.7F, 970.2F, 125.0F,
                   777.7F, 5);
        event_wait(33);
        event_set_pos(ev0007_target, 1227.6F, 7.0F, 651.7F);
        event_set_ang(ev0007_target, 0, 0, 0);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_look_at(event_get_player(0), 1000.3F, 136.5F, 810.0F);
        event_look_at(event_get_player(2), 1000.3F, 136.5F, 810.0F);
        stream_request_music(MUSIC_EGGMAN);
        event_sound_start(1, 1223, 1800);
        event_sound_fade_volume(1, -60, 1);
        event_wait(1);
        event_sound_fade_volume(1, -50, 80);
        event_wait(30);
        event_set_pos(event_get_player(0), 943.0F, 123.5F, 724.7F);
        event_set_ang(event_get_player(0), 0, 0x2000, 0);
        event_set_pos(event_get_player(2), 951.4F, 123.5F, 705.07F);
        event_set_ang(event_get_player(2), 0, 0x1000, 0);
        event_eggmobile_fly_to(1000.3F, 146.5F, 810.0F, 100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 936.3F, 126.2F, 716.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xa400, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 930.6F, 127.5F, 721.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0400, 0xa900, 0xfe00);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(965);
        event_message_wait(60, TEXT_2);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1039.4F, 128.3F, 837.17F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0fa0, 0x298e, 0x0710);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 1048.11F, 129.1F, 823.7F);
        event_sound_fade_volume(1, -40, 40);
        event_voice_play(966);
        event_message_wait(50, TEXT_3);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 992.4F, 159.6F, 810.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xeba0, 0xb88e, 0x0110);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0x01a0, 0xb88e, 0x0110);
        event_voice_play(967);
        event_message_show(TEXT_4);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 996.9F, 159.5F, 804.96F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x01a0, 0x948e, 0x0110);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 997.47F, 159.6F, 805.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0x04a0, 0x958e, 0x0110);
        event_wait(45);
        event_play_action(event_get_player(2), &tails_sprint_action,
                          &tails_texlist, 1.1F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 958.5F, 129.3F, 732.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x4300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 956.0F, 129.6F, 724.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0200, 0x4900, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(968);
        event_message_wait(70, TEXT_5);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 969.7F, 129.8F, 705.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0xfd00, 0x5700, 0);
        event_voice_play(969);
        event_message_wait(90, TEXT_6);
        event_run_to_speed(event_get_player(2), 953.6F, 123.5F, 709.2F, 0.9F,
                           1.0F);
        event_play_action(event_get_player(2), &ev0007_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(20);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(970);
        event_message_show(TEXT_7);
        event_wait(80);
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 927.6F, 131.7F, 688.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x088c, 0xa000, 0xfa00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, 912.3F, 127.3F, 715.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0x088c, 0xaa2c, 0xfa00);
        event_sound_fade_volume(1, -20, 60);
        event_eggmobile_fly_to(1034.9F, 198.5F, 869.7F, 150);
        event_voice_play(971);
        event_message_wait(60, TEXT_8);
        event_sound_fade_volume(1, -30, 120);
        event_voice_play(972);
        event_message_wait(120, TEXT_9);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 944.3F, 131.9F, 737.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf900, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x2aab);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_15);
        event_voice_play(973);
        event_message_wait(60, TEXT_10);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 912.3F, 127.3F, 715.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x088c, 0xaa2c, 0xfa00);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0x068c, 0xaa2c, 0xfa00);
        event_wait(5);
        event_voice_play(974);
        event_message_show(TEXT_11);
        event_sound_fade_volume(1, 0, 20);
        event_eggmobile_turn(0, 0x8000, 0, 20, 0);
        event_wait(20);
        event_sound_fade_volume(1, -20, 30);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_eggmobile_fly_turn(1213.6F, 160.0F, 1146.3F, 0, 0, 0, 60, 0);
        event_wait(63);
        event_sound_fade_volume(1, -90, 100);
        event_eggmobile_set_jets(EGGMOBILE_JETS_OFF);
        event_eggmobile_fly_turn(1216.0F, 105.3F, 1172.9F, 0, 0x5e00, 0, 60, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, 939.4F, 126.0F, 699.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x058d, 0x9226, 0xf800);
        event_wait(25);
        event_eggmobile_kill();
        event_wait(4);
        event_message_close();
        event_sound_stop(1);
        event_hornet_place(1272.57F, 55.0F, 1191.11F, 0, 0x8800, 0);
        event_wait(2);
        event_hornet_fly_to(1188.9567F, 198.0F, 1099.0181F);
        event_sound_start(2, 1224, 1800);
        event_sound_fade_volume(2, 45, 1);
        event_wait(1);
        event_sound_fade_volume(2, 60, 94);
        event_wait(94);
        event_hornet_place(1188.9567F, 198.0F, 1099.0181F, 0, 0x9e00, 0);
        event_wait(0);
        event_sound_fade_volume(2, 80, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1193.3F, 202.0F, 1075.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x090d, 0x7278, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 1200.0F, 204.0F, 1079.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0b0d, 0x7a78, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 1204.0F, 200.6F, 1076.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x050d, 0x8378, 0x0200);
        event_wait(61);
        event_set_pos(ev0007_target, 1190.3F, 197.5F, 1100.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1088.4F, 127.7F, 1033.3F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 65, 1076.4F, 135.7F, 1032.3F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, ev0007_target, 0.0F,
                                  0.0F, 0.0F, 0);
        event_wait(70);
        event_camera_look_clear();
        event_sound_start(3, 1225, 1800);
        event_sound_fade_volume(2, 90, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1220.5F, 178.0F, 1155.47F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0210, 0x0c70, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 1225.4F, 195.0F, 1154.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0210, 0x0c70, 0);
        event_sound_stop(1);
        event_wait(1);
        event_sound_fade_volume(2, 100, 10);
        event_hornet_jets_off();
        event_wait(10);
        event_sound_fade_volume(2, 80, 1);
        event_hornet_jets_boost();
        event_sound_fade_volume(2, 90, 10);
        event_wait(10);
        event_sound_fade_volume(2, 70, 110);
        event_hornet_jets_off();
        event_wait(10);
        event_sound_fade_volume(2, 110, 160);
        event_hornet_set_speed(6.0F);
        event_hornet_jets_hold();
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1088.4F, 127.7F, 1033.3F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, 1076.4F, 135.7F, 1032.3F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, ev0007_target, 0.0F,
                                  0.0F, 0.0F, 0);
        event_move(ev0007_target, 1190.3F, 197.5F, 1100.0F, 980.6F, 146.0F,
                   768.7F, 60);
        event_hornet_fly_to(980.6F, 146.0F, 768.7F);
        event_wait(20);
        event_hornet_jets_boost();
        event_wait(45);
        event_set_ang(event_get_player(0), 0, 0x0a00, 0);
        event_set_pos(event_get_player(2), 951.4F, 123.5F, 708.07F);
        event_set_ang(event_get_player(2), 0, 0xfb00, 0);
        event_hornet_jets_on();
        event_sound_fade_volume(2, 10, 30);
        event_sound_play(1226, 110, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 943.9F, 125.2F, 708.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0ea8, 0x9364, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, 925.7F, 124.8F, 719.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x0bab, 0xa36c, 0);
        event_wait(180);
        event_sound_stop(2);
        event_message_close();
        break;
    case EVENT_END:
        event_voice_stop();
        event_sound_stop_all();
        event_reset_player(0);
        event_move_stop_all();
        event_set_pos(event_get_player(0), 952.7F, 123.5F, 726.7F);
        event_set_ang(event_get_player(0), 0, 0x3e00, 0);
        event_kill_player(2);
        event_kill_task(&ev0007_target);
        event_eggmobile_kill();
        event_hornet_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0007_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"egm1jet", &event_jet_texlist},
    {"egm1jetb", &event_jet_glow_texlist},
    {"EV_EGGMOBLE1", &event_hornet_texlist},
    {"EV_EGGMAN_BODY", &event_eggman_body_texlist},
    {"MILES", &tails_texlist},
    {NULL, NULL},
};
