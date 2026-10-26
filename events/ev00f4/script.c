/* EV00F4: at Mystic Ruins' station, Tails runs up to tell Sonic that Angel
 * Island is falling again */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00f4/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/floor.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 48
#define TEXT_3_LENGTH 31
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 51
#define TEXT_6_LENGTH 29
#define TEXT_7_LENGTH 50
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 30
#define TEXT_10_LENGTH 48
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 15
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 2
#define TEXT_15_LENGTH 3
#define TEXT_16_LENGTH 2
#define TEXT_17_LENGTH 3
#define TEXT_18_LENGTH 2
#define TEXT_19_LENGTH 4
#include "ev00f4/script/text.inc"

extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_skid_action;

extern NJS_ACTION ev00f4_sonic_action_0;
extern NJS_ACTION ev00f4_sonic_action_1;
extern NJS_ACTION ev00f4_sonic_action_2;
extern NJS_ACTION ev00f4_sonic_action_3;
extern NJS_ACTION ev00f4_sonic_action_4;
extern NJS_ACTION ev00f4_sonic_action_5;
extern NJS_ACTION ev00f4_tails_action_0;
extern NJS_ACTION ev00f4_tails_action_1;

/* no code uses this word. it keeps ev00f4_floor at its address in the
 * image */
static Uint32 dat_0cb80be0 = 0;
static struct task *ev00f4_floor = NULL;

void ev00f4_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_MSTCLN);
        event_set_pos(event_get_player(0), -53.8F, 36.4F, 545.83F);
        event_set_ang(event_get_player(0), 0x098c, 0x44cf, 0xf9a5);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00f4_sonic_action_2,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        ev00f4_floor = event_floor_create(event_get_player(0), 36.4F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -100.0F, 212.555F, 500.78F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x9000, 0);
        event_create_player(2, tails_step, -51.24F, 73.7676F, 431.1F, 0x0893,
                            0x0eec, 0xfe1e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -100.0F, 57.555F, 500.78F);
        event_wait(200);
        event_talk_start(event_get_player(0), TEXT_11);
        event_play_action(event_get_player(2), &tails_run_action,
                          &tails_texlist, 1.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -95.0F, 57.458F, 498.93F);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -52.43F, 40.78F, 534.5666F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0100, 0x7c00, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -52.6F, 40.83F, 536.58F);
        event_play_action(event_get_player(0), &ev00f4_sonic_action_3,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_play(1905);
        event_message_show(TEXT_1);
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(50);
        event_play_action(event_get_player(0), &ev00f4_sonic_action_2,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(1906);
        event_message_show(TEXT_2);
        event_wait(50);
        event_wait(15);
        event_message_close();
        event_wait(15);
        event_message_show(TEXT_3);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(1908);
        event_message_show(TEXT_4);
        event_play_action(event_get_player(0), &ev00f4_sonic_action_4,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_wait(60);
        event_run_to_speed(event_get_player(2), -55.95F, 43.125F, 522.8F, 0.4F,
                           0.6F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -42.63F, 34.0F, 556.61F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1500, 0x2b00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -42.66F, 34.0F, 556.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x1500, 0x1600, 0);
        event_wait(70);
        event_voice_play(1909);
        event_talk_start(event_get_player(2), TEXT_15);
        event_message_show(TEXT_5);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 250, -41.0F, 34.0F, 555.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 250, 0x1500, 0x1c00, 0);
        event_play_action(event_get_player(2), &tails_skid_action,
                          &tails_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(2), &ev00f4_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(2), &ev00f4_tails_action_1,
                          &tails_texlist, 2.0F, 0, 4);
        event_play_action(event_get_player(2), &ev00f4_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 16);
        event_message_close();
        event_voice_play(1910);
        event_talk_start(event_get_player(2), TEXT_16);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -50.3F, 56.8657F, 518.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xde75, 0x70b7, 0xf757);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -48.0F, 52.6725F, 515.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xeb75, 0x69b7, 0xfc00);
        event_wait(5);
        event_voice_play(1911);
        event_talk_start(event_get_player(2), TEXT_17);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(0), &ev00f4_sonic_action_5,
                          &sonic_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev00f4_sonic_action_1,
                          &sonic_texlist, 0.8F, 0, 4);
        event_play_action(event_get_player(0), &ev00f4_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_voice_play(1912);
        event_talk_start(event_get_player(0), TEXT_18);
        event_message_show(TEXT_8);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_voice_play(1913);
        event_talk_start(event_get_player(0), TEXT_19);
        event_message_show(TEXT_9);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -43.3F, 38.8F, 549.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f00, 0x2600, 0xfa00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -44.78F, 39.535F,
                             548.675F);
        event_voice_play(1914);
        event_talk_start(event_get_player(2), TEXT_17);
        event_message_show(TEXT_10);
        event_wait(60);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(30);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev00f4_floor);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f4_textures[] = {
    {"MILES", &tails_texlist},
    {NULL, NULL},
};
