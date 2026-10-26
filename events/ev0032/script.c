/* EV0032, Tails' story: Sonic finds Tails by his crashed plane. Tails shows him
 * the purple Chaos Emerald and invites him to his workshop */
#include <ninja.h>

#include "ev0032/emerald_action.h"
#include "ev0032/emerald_textures.h"
#include "ev0032/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 25
#define TEXT_3_LENGTH 47
#define TEXT_4_LENGTH 60
#define TEXT_5_LENGTH 48
#define TEXT_6_LENGTH 45
#define TEXT_7_LENGTH 21
#define TEXT_8_LENGTH 19
#define TEXT_9_LENGTH 42
#define TEXT_10_LENGTH 38
#define TEXT_11_LENGTH 50
#define TEXT_12_LENGTH 56
#define TEXT_13_LENGTH 63
#define TEXT_14_LENGTH 2
#define TEXT_15_LENGTH 6
#define TEXT_16_LENGTH 5
#define TEXT_17_LENGTH 3
#define TEXT_18_LENGTH 5
#define TEXT_19_LENGTH 3
#define TEXT_20_LENGTH 5
#define TEXT_21_LENGTH 7
#define TEXT_22_LENGTH 3
#define TEXT_23_LENGTH 5
#define TEXT_24_LENGTH 4
#define TEXT_25_LENGTH 4
#include "ev0032/script/text.inc"

extern NJS_ACTION sonic_walk_action;
extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_walk_action;

extern NJS_ACTION ev0032_sonic_action;
extern NJS_ACTION ev0032_tails_action;

static struct task *ev0032_emerald = NULL;

void ev0032_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_THESONIC);
        event_set_pos(event_get_player(0), -470.99F, 0.01F, 2023.55F);
        event_set_ang(event_get_player(0), 0, 0x640b, 0);
        event_play_action(event_get_player(0), &tails_walk_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_actor(&ev0032_emerald, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z,
                          event_get_player(0)->work->ang.x,
                          0x4000 - event_get_player(0)->work->ang.y,
                          event_get_player(0)->work->ang.z);
        event_create_player(2, sonic_step,
                            event_get_player(0)->work->pos.x + 20.0F,
                            event_get_player(0)->work->pos.y,
                            event_get_player(0)->work->pos.z - 10.0F,
                            event_get_player(0)->work->ang.x,
                            0x4000 - event_get_player(0)->work->ang.y,
                            event_get_player(0)->work->ang.z);
        event_wait(1);
        event_play_action(event_get_player(2), &sonic_walk_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd200, 0x8500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -439.7F, 100.41F, 1916.57F);
        event_wait(1);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_wait(1);
        event_run_to_speed(event_get_player(2), -450.67F, 0.01F, 1901.05F,
                           0.58F, 3.0F);
        event_run_to_speed(event_get_player(0), -460.17F, 1.01F, 1912.28F,
                           0.56F, 3.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xd200, 0x8500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -447.7F, 66.97F, 1931.85F);
        event_wait(90);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xb800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -495.04F, 12.73F, 1963.44F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0xfd00, 0xd400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -467.969F, 5.2386F,
                             1920.849F);
        event_wait(60);
        event_run_clear(event_get_player(0));
        event_run_clear(event_get_player(2));
        event_wait(1);
        event_run_to_speed(event_get_player(2), -450.67F, 0.01F, 1901.05F,
                           0.58F, 3.0F);
        event_run_to_speed(event_get_player(0), -457.17F, 1.01F, 1917.28F,
                           0.56F, 3.0F);
        event_clear_motions(event_get_player(2));
        event_clear_motions(event_get_player(0));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1184);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_clear();
        event_wait(30);
        event_set_ang(event_get_player(0), 0, 0x7200, 0);
        event_set_ang(event_get_player(2), 0, 0xea00, 0);
        event_look_clear(event_get_player(2));
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xb400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -485.13F, 7.67F, 1917.63F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xfd00, 0xc700, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -475.3F, 6.93F, 1915.92F);
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1185);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_voice_play(1186);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_message_close();
        event_run_to_speed(event_get_player(0), -457.17F, 1.01F, 1913.28F,
                           0.56F, 0.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xfd00, 0x9100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -465.538F, 7.67F, 1888.11F);
        event_run_wait(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x7200, 0);
        event_wait(40);
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1187);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_17);
        event_voice_play(1188);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(1);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 120, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -458.137F, 8.437F,
                             1883.174F);
        event_set_pos(ev0032_emerald, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev0032_emerald, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1189);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(1);
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(1190);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_look_clear();
        event_wait(1);
        event_sound_play(704, 0, 0, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe300, 0x7500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -451.03F, 21.11F, 1891.95F);
        event_play_action(event_get_player(0), &ev0032_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0032_emerald, &ev0032_emerald_action,
                          &ev0032_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0xe300, 0x7500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -455.47F, 6.74F, 1908.0F);
        event_wait(50);
        event_play_action(event_get_player(2), &ev0032_sonic_action,
                          &sonic_texlist, 0.3F, EVENT_REPEAT, 8);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_20);
        event_voice_play(1191);
        event_message_show(TEXT_8);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -458.277F, 5.85F, 1910.37F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -457.61F, 5.78F, 1909.64F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -458.277F, 5.85F, 1910.37F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -457.61F, 5.78F, 1909.64F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -458.277F, 5.85F, 1910.37F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -457.61F, 5.78F, 1909.64F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -458.277F, 5.85F, 1910.37F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xfe00, 0xe300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -462.28F, 6.27F, 1914.82F);
        event_wait(1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_orbit(EVENT_EASE_LINEAR, 240, event_get_player(0), 7.0F, 0,
                           0x8000, 0, 0, 0, 0, 10.0F, 10.0F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_clear_motions(event_get_player(2));
        event_set_pos(event_get_player(2), -445.67F, 0.01F, 1901.05F);
        event_talk_start(event_get_player(0), TEXT_21);
        event_voice_play(1192);
        event_message_show(TEXT_9);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_talk_start(event_get_player(0), TEXT_22);
        event_voice_play(1193);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(1);
        event_talk_start(event_get_player(0), TEXT_23);
        event_voice_play(1194);
        event_message_show(TEXT_11);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_clear();
        event_wait(60);
        event_talk_start(event_get_player(0), TEXT_24);
        event_voice_play(1195);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_look_clear();
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_camera_orbit(EVENT_EASE_LINEAR, 150, event_get_player(0), 7.0F, 0,
                           0x4000, 0, 0, -0x1000, 0, 10.0F, 40.0F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_run_to_speed(event_get_player(0), -450.29F, 0.01F, 1865.426F,
                           0.46F, 3.0F);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0032_emerald);
        event_talk_start(event_get_player(0), TEXT_25);
        event_voice_play(1196);
        event_message_show(TEXT_13);
        event_wait(10);
        event_wait(30);
        event_look_clear(event_get_player(0));
        event_run_to_speed(event_get_player(0), -416.5F, 0.01F, 1848.117F,
                           0.46F, 3.0F);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_kill_player(2);
        event_wait(40);
        event_camera_look_clear();
        event_wait(1);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), -416.5F, 0.01F, 1848.117F);
        event_set_ang(event_get_player(0), 0, 0x646d, 0);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0032_emerald);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0032_textures[] = {
    {"SONIC", &sonic_texlist},
    {"M_EM_PURPLE", &ev0032_emerald_texlist},
    {NULL, NULL},
};
