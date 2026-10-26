/* EV0062, Amy's story: Amy finds Sonic and asks him to guard the bird, which is
 * being chased */
#include <ninja.h>

#include "ev0062/amy_action.h"
#include "ev0062/script.h"
#include "ev0062/ver2_wing_action.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 15
#define TEXT_4_LENGTH 15
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 26
#define TEXT_7_LENGTH 70
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 34
#define TEXT_10_LENGTH 19
#define TEXT_11_LENGTH 30
#define TEXT_12_LENGTH 1
#define TEXT_13_LENGTH 1
#define TEXT_14_LENGTH 1
#define TEXT_15_LENGTH 1
#define TEXT_16_LENGTH 3
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 4
#define TEXT_19_LENGTH 1
#include "ev0062/script/text.inc"

extern NJS_ACTION sonic_walk_action;
extern NJS_ACTION amy_stop_action;
extern NJS_ACTION ev0062_sonic_action_0;

/* @hack type: single NJS_ACTIONs declared as arrays. SHC then keeps
 * event_talk_stop's address in R8 (0x0cb80488) and event_clear_motions's in
 * R9 (0x0cb801e0). it keeps event_camera_set_pos's address in R10
 * (0x0cb80026) and &ev0062_bird in R11 (0x0cb80138). it keeps 4.8 in FR12
 * (0x0cb8018e) and 0.4 in FR13 (0x0cb80192). ev0062_sonic_action_0, above,
 * builds the same as a single object only while ev0062_sonic_action_1 is an
 * array: with both single, 113 of ev0062_script's 1647 words differ. as
 * single objects passed by address, R8 and R9, R10 and R11, and FR12 and
 * FR13 trade values, and 88 of the 1647 differ */
extern NJS_ACTION sonic_stand_action[];
extern NJS_ACTION amy_stand_action[];
extern NJS_ACTION amy_walk_action[];
extern NJS_ACTION amy_trot_action[];
extern NJS_ACTION ev0062_sonic_action_1[];
extern NJS_ACTION ev0062_sonic_action_2[];
extern NJS_ACTION ev0062_sonic_action_3[];

/* port 2's keys as Sonic walks off, one a step. the stick is pushed most of the
 * way for 228 steps, then let go for the last 3 */
static const struct event_pad_key ev0062_walk_keys[231] = {
#include "ev0062/script/ev0062_walk_keys.inc"
};

static const struct event_pad_recording ev0062_walk = {
    ev0062_walk_keys, sizeof(ev0062_walk_keys) / sizeof(ev0062_walk_keys[0])};

static struct task *ev0062_bird = NULL;

void ev0062_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_0012.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_create_player(2, sonic_step, -623.2F, 5.6F, 1079.0F, 0, 0xc000,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(0), -525.5F, 0.02F, 1340.9F);
        event_set_ang(event_get_player(0), 0, 0x8068, 0);
        event_set_pos(event_get_player(2), -534.2F, 0.01F, 1270.0F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_place_actor(&ev0062_bird, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z, 0, 0x8000, 0);
        event_wait(1);
        event_set_ang(ev0062_bird, 0, 0x8000, 0);
        event_set_flags(ev0062_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev0062_bird, ev0062_ver2_wing_action,
                          &ev0062_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), amy_walk_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -525.5F, 0.02F, 1300.9F, 0.5F,
                           3.0F);
        event_follow(ev0062_bird, 4.8F, 7.5F, 0.4F, 100, event_get_player(0));
        event_camera_follow(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -537.23F, 7.84F, 1349.59F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0xda00, 0);
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(2), &sonic_walk_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), -534.2F, 0.0F, 1204.0F, 0.2F,
                           0.06F);
        event_wait(60);
        event_camera_follow_clear();
        stream_request_music(MUSIC_THESONIC);
        event_set_pos(event_get_player(0), -599.58F, 0.0F, 1112.49F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_pos(ev0062_bird, event_get_player(0)->work->pos.x + 4.8F,
                      event_get_player(0)->work->pos.y + 7.5F,
                      event_get_player(0)->work->pos.z + 0.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd00, 0x0400, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -527.0F, 11.04F, 1322.4F);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 30, 0x071c);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0);
        event_wait(50);
        event_camera_look_clear();
        event_wait(10);
        event_set_pos(event_get_player(0), -525.5F, 0.02F, 1300.9F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_set_pos(ev0062_bird, event_get_player(0)->work->pos.x + 4.8F,
                      event_get_player(0)->work->pos.y + 7.5F,
                      event_get_player(0)->work->pos.z + 0.4F);
        event_play_action(event_get_player(0), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -507.93F, 6.08F, 1277.97F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0x6400, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -520.62F, 6.57F, 1293.42F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(1392);
        event_message_show(TEXT_1);
        event_wait(50);
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_look_at(event_get_player(0), -520.62F, 6.57F, 1293.42F);
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(1393);
        event_message_wait(60, TEXT_2);
        event_talk_stop(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 7.0F,
                           0.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_trot_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_run_to_speed(event_get_player(0), -528.2F, 0.0F, 1240.0F, 0.3F,
                           3.0F);
        event_follow(ev0062_bird, 4.8F, 7.5F, 0.4F, 230, event_get_player(0));
        event_message_close();
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(1394);
        event_message_show(TEXT_3);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -512.67F, 6.13F, 1315.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0x1d00, 0);
        event_camera_follow(event_get_player(0));
        event_wait(70);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_play_action(event_get_player(2), &ev0062_sonic_action_0,
                          &sonic_texlist, 0.25F, 0, 4);
        event_wait(60);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -529.41F, 4.9F, 1256.32F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x8800, 0);
        event_set_pos(event_get_player(0), -525.5F, 0.02F, 1280.9F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_set_pos(ev0062_bird, event_get_player(0)->work->pos.x + 4.8F,
                      event_get_player(0)->work->pos.y + 7.5F,
                      event_get_player(0)->work->pos.z + 0.4F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_trot_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_run_to_speed(event_get_player(0), -534.2F, 0.0F, 1221.0F, 0.5F,
                           3.0F);
        event_follow(ev0062_bird, 4.8F, 7.5F, 0.4F, 140, event_get_player(0));
        event_wait(1);
        event_camera_follow(event_get_player(0));
        event_wait(50);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_set_ang(event_get_player(2), 0, 0xfe90, 0);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -514.04F, 7.7F, 1216.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0xf955, 0x39c3, 0);
        event_wait(50);
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(1043);
        event_message_show(TEXT_4);
        event_wait(12);
        event_clear_motions(event_get_player(0));
        event_wait(1);
        event_play_action(event_get_player(0), &amy_stop_action, &amy_texlist,
                          1.0F, 0, 8);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 7.0F,
                           0.0F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1395);
        event_message_show(TEXT_5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -533.1F, 5.7F, 1208.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0255, 0xffc3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 18, -532.3F, 2.7F, 1213.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 18, 0x0c55, 0x05c3, 0);
        event_wait(1);
        event_sound_play(1223, 0, 0, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), ev0062_sonic_action_3,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 16);
        event_move(event_get_player(2), -534.2F, 0.0F, 1204.0F, -534.2F, 0.0F,
                   1194.0F, 40);
        event_wait(42);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), ev0062_sonic_action_1,
                          &sonic_texlist, 1.25F, EVENT_REPEAT, 4);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -516.85F, 4.41F, 1187.33F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0255, 0x59c3, 0);
        event_wait(40);
        event_set_pos(ev0062_bird, event_get_player(0)->work->pos.x + 4.8F,
                      event_get_player(0)->work->pos.y + 7.5F,
                      event_get_player(0)->work->pos.z + 0.4F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -518.28F, 4.58F, 1190.76F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0255, 0x53c3, 0);
        event_wait(10);
        event_run_to_speed(event_get_player(0), -534.2F, 0.0F, 1215.0F, 0.5F,
                           3.0F);
        event_follow(ev0062_bird, 4.8F, 7.5F, 0.4F, 100, event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1396);
        event_message_wait(60, TEXT_6);
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -532.52F, 6.21F, 1206.87F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a02, 0x76f0, 0);
        event_set_pos(ev0062_bird, event_get_player(0)->work->pos.x - 0.3F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0062_bird);
        event_play_action(ev0062_bird, &ev0062_ver1_wing_action,
                          &ev0062_ver1_wing_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev0062_amy_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(15);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0xf902, 0x76f0, 0);
        event_play_action(event_get_player(2), sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_clear(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1397);
        event_message_show(TEXT_7);
        event_wait(120);
        event_message_close();
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 7.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -509.2F, 11.52F, 1213.24F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf955, 0x33c3, 0);
        event_follow(ev0062_bird, 4.8F, 7.5F, 0.4F, 999999,
                     event_get_player(0));
        event_clear_motions(ev0062_bird);
        event_play_action(ev0062_bird, ev0062_ver2_wing_action,
                          &ev0062_ver2_wing_texlist, 1.0F, EVENT_REPEAT, 8);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, -513.91F, 10.71F, 1211.78F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1398);
        event_play_action(event_get_player(2), ev0062_sonic_action_2,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 4);
        event_message_show(TEXT_8);
        event_wait(60);
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1399);
        event_message_show(TEXT_9);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_walk_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(0));
        event_camera_follow(event_get_player(0));
        event_wait(8);
        event_run_to_speed(event_get_player(0), -534.0F, 0.0F, 1209.0F, 0.5F,
                           1.0F);
        event_play_action(event_get_player(2), ev0062_sonic_action_2,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 8);
        event_camera_follow_clear();
        event_run_wait(event_get_player(0));
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -524.29F, 7.26F, 1217.85F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf955, 0x1bc3, 0);
        event_clear_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 600, -510.0F, 11.01F, 1235.47F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_19);
        event_voice_play(1400);
        event_message_show(TEXT_10);
        event_wait(30);
        event_play_action(event_get_player(2), ev0062_sonic_action_3,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_move(event_get_player(2), -534.2F, 0.0F, 1194.0F, -534.2F, 0.0F,
                   1184.0F, 30);
        event_wait(30);
        event_talk_stop(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_pad(2, &ev0062_walk);
        event_wait(1);
        event_message_close();
        event_wait(40);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_trot_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_follow(event_get_player(0));
        event_run_to_speed(event_get_player(0), -537.54F, 0.0F, 1127.25F, 0.6F,
                           0.0F);
        event_wait(10);
        event_voice_play(1401);
        event_message_show(TEXT_11);
        event_wait(70);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 7.0F,
                           0.0F);
        event_message_close();
        event_camera_follow_clear();
        event_wait(40);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_set_pos(event_get_player(0), -537.54F, 0.0F, 1127.25F);
        event_set_ang(event_get_player(0), 0, 0x81be, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -546.825F, 19.75F,
                             1079.097F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0x87c3, 0);
        event_kill_player(2);
        event_kill_task(&ev0062_bird);
        event_reset_player(0);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0062_textures[] = {
    {"SONIC", &sonic_texlist},
    {"VER1_WING", &ev0062_ver1_wing_texlist},
    {"VER2_WING", &ev0062_ver2_wing_texlist},
    {NULL, NULL},
};
