/* EV0089, Knuckles' story: Knuckles is troubled that Sonic is after the Master
 * Emerald too. he decides to go on searching alone */
#include <ninja.h>

#include "ev0089/knuckles_action.h"
#include "ev0089/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 69
#define TEXT_2_LENGTH 43
#define TEXT_3_LENGTH 3
#define TEXT_4_LENGTH 3
#include "ev0089/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

void ev0089_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), 80.38F, 10.52F, 770.86F);
        event_set_ang(event_get_player(0), 0, 0xf4d2, 0);
        event_run_to_speed(event_get_player(0), 113.78F, 3.14F, 716.32F, 0.43F,
                           0.06F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 134.03F, 1.97F, 709.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c19, 0x5ec5, 0);
        event_wait(10);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0089_knuckles_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1589);
        event_message_wait(0, TEXT_1);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 134.03F, 1.97F, 709.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0e19, 0x4cc5, 0);
        event_wait(50);
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 114.03F, 3.63F, 701.07F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b25, 0x84ac, 0);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1590);
        event_message_wait(0, TEXT_2);
        event_wait(70);
        event_message_close();
        event_wait(10);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_set_pos(event_get_player(0), 113.78F, 3.14F, 716.32F);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0089_textures[] = {{NULL, NULL}};
