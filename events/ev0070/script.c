/* EV0070, Amy's story: Amy, out of breath, hasn't found the bird's family. the
 * bird fled the Egg Carrier, so she means to look for them there */
#include <ninja.h>

#include "ev0070/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/move.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 52
#define TEXT_3_LENGTH 51
#define TEXT_4_LENGTH 53
#define TEXT_5_LENGTH 29
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 3
#define TEXT_8_LENGTH 1
#define TEXT_9_LENGTH 3
#define TEXT_10_LENGTH 4
#define TEXT_11_LENGTH 1
#include "ev0070/script/text.inc"

extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_trot_action;

extern NJS_ACTION ev0070_amy_action_0;
extern NJS_ACTION ev0070_amy_action_1;
extern NJS_ACTION ev0070_amy_action_2;
extern NJS_TEXLIST ev0070_bird_texlist;
extern NJS_ACTION ev0070_bird_action;

static struct task *ev0070_bird = NULL;

void ev0070_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_THEAMY);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 30.35F, 8.48F, -172.77F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf800, 0x3700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 15.87F, 10.03F, -152.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0xf600, 0x1800, 0);
        event_place_actor(&ev0070_bird, 3.9F, 0.21F, -165.84F, 0, 0x0035, 0);
        event_set_flags(ev0070_bird, EVENT_IGNORE_GROUND);
        event_set_pos(ev0070_bird, -1.78F, 13.0F, -180.51F);
        event_set_ang(ev0070_bird, 0, 0x17d8, 0);
        event_clear_motions(ev0070_bird);
        event_play_action(ev0070_bird, &ev0070_bird_action,
                          &ev0070_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev0070_bird, -1.78F, 13.0F, -180.51F, 2.52F, 7.0F, -167.72F,
                   60);
        event_set_pos(event_get_player(0), 6.19F, 0.19F, -179.35F);
        event_set_ang(event_get_player(0), 0, 0x0035, 0);
        event_run_to_speed(event_get_player(0), 6.83F, 0.21F, -165.26F, 1.0F,
                           0.06F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          1.0F, 0, 0);
        event_wait(40);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1505);
        event_message_wait(0, TEXT_1);
        event_play_action(event_get_player(0), &ev0070_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(140);
        event_voice_play(1506);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_message_wait(60, TEXT_2);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1.99F, 4.49F, -174.22F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0700, 0x9e00, 0);
        event_play_action(event_get_player(0), &ev0070_amy_action_1,
                          &amy_texlist, 1.0F, 0, 8);
        event_wait(30);
        event_play_action(event_get_player(0), &ev0070_amy_action_2,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_talk_start(event_get_player(0), TEXT_8);
        event_wait(45);
        event_talk_start(event_get_player(0), TEXT_9);
        event_voice_play(1507);
        event_message_wait(0, TEXT_3);
        event_wait(120);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_10);
        event_voice_play(1508);
        event_message_wait(0, TEXT_4);
        event_wait(100);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 8.88F, 0.69F, -147.09F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d00, 0x0600, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 7.07F, 4.77F, -159.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x0d00, 0x0600, 0);
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(1509);
        event_message_wait(0, TEXT_5);
        event_wait(50);
        event_message_close();
        event_wait(10);
        event_move_stop(ev0070_bird);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_move_stop(ev0070_bird);
        event_kill_task(&ev0070_bird);
        event_set_pos(event_get_player(0), 6.83F, 0.21F, -165.26F);
        event_set_ang(event_get_player(0), 0, 0x0035, 0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0070_textures[] = {
    {"VER2_WING", &ev0070_bird_texlist},
    {NULL, NULL},
};
