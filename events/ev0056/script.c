/* EV0056, Tails' story: Tails and Sonic find Eggman holds two Chaos Emeralds to
 * their one */
#include <ninja.h>

#include "ev0056/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 33
#define TEXT_2_LENGTH 48
#define TEXT_3_LENGTH 22
#define TEXT_4_LENGTH 56
#define TEXT_5_LENGTH 29
#define TEXT_6_LENGTH 18
#define TEXT_7_LENGTH 2
#define TEXT_8_LENGTH 2
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 2
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 3
#include "ev0056/script/text.inc"

extern NJS_ACTION sonic_stand_action;

extern NJS_ACTION ev0056_sonic_action_0;
extern NJS_ACTION ev0056_sonic_action_1;
extern NJS_ACTION ev0056_sonic_action_2;
extern NJS_ACTION ev0056_sonic_action_3;
extern NJS_ACTION ev0056_tails_action_0;
extern NJS_ACTION ev0056_tails_action_1;
extern NJS_ACTION ev0056_tails_action_2;
extern NJS_ACTION ev0056_tails_action_3;
extern NJS_ACTION ev0056_tails_action_4;
extern NJS_ACTION ev0056_tails_action_5;

static struct task *ev0056_fade = NULL;

void ev0056_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -526.0F, -2.0F, 941.0F);
        event_set_ang(event_get_player(0), 0, 0xf200, 0);
        event_reset_player(0);
        event_create_player(2, sonic_step, -535.4F, -2.0F, 944.5F, 0, 0xfa26,
                            0);
        ev0056_fade = event_fade_create(1.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_wait(1);
        event_play_action(event_get_player(0), &ev0056_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_6);
        event_play_action(event_get_player(2), &ev0056_sonic_action_1,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(2), TEXT_6);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -520.753F, 6.2F, 951.515F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe8e6, 0x16b1, 0);
        if (ev0056_fade != NULL) {
            event_fade_set_speed(ev0056_fade, -1.0F / 60.0F);
        }
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -520.58F, 10.5F, 959.357F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xe9e6, 0x13b1, 0);
        event_wait(100);
        event_talk_start(event_get_player(2), TEXT_7);
        event_wait(5);
        event_talk_start(event_get_player(0), TEXT_7);
        event_wait(27);
        event_play_action(event_get_player(2), &ev0056_sonic_action_2,
                          &sonic_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(5);
        event_play_action(event_get_player(0), &ev0056_tails_action_2,
                          &tails_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(0));
        event_set_pos(event_get_player(0), -527.3F, -2.0F, 944.7F);
        event_play_action(event_get_player(0), &ev0056_tails_action_3,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -519.9F, 0.5177F, 951.47F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0ce6, 0x27b1, 0xff00);
        event_play_action(event_get_player(0), &ev0056_tails_action_4,
                          &tails_texlist, 1.0F, 0, 8);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 3.0F,
                           0.0F);
        event_play_action(event_get_player(0), &ev0056_tails_action_5,
                          &tails_texlist, 1.3F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 3.0F,
                           0.0F);
        event_talk_stop(event_get_player(0));
        event_voice_play(1215);
        event_talk_start(event_get_player(0), TEXT_8);
        event_message_show(TEXT_1);
        event_wait(60);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 350, -524.7646F, 0.4634F,
                             956.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 350, 0x0ce6, 0x12b1, 0xff00);
        event_play_action(event_get_player(2), &ev0056_sonic_action_3,
                          &sonic_texlist, 0.8F, 0, 8);
        event_wait_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0056_sonic_action_0,
                          &sonic_texlist, 0.3F, EVENT_REPEAT, 16);
        event_talk_start(event_get_player(2), TEXT_9);
        event_voice_play(1216);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(10);
        event_voice_play(1217);
        event_talk_start(event_get_player(2), TEXT_10);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(1218);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -523.9F, -2.555F, 959.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x10e6, 0x0cb1, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -526.1435F, 0.67F, 952.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x10e6, 0x0cb1, 0xff00);
        event_play_action(event_get_player(0), &ev0056_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_play(1219);
        event_talk_start(event_get_player(0), TEXT_12);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), -527.3F, -2.0F, 944.7F);
        event_set_ang(event_get_player(0), 0, 0xf200, 0);
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev0056_fade);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0056_textures[] = {
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
