/* EV00BE, E-102's story: E-102 walks a little way and stops. he says it
 * seems to be the wrong room */
#include <ninja.h>

#include "ev00be/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 28
#include "ev00be/script/text.inc"

extern NJS_ACTION e102_walk_action;
extern NJS_ACTION e102_slide_stand_action;

void ev00be_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_THEE102);
        event_set_pos(event_get_player(0), 71.5F, 0.0F, 192.7F);
        event_set_ang(event_get_player(0), 0, 0xa851, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 38.5F, 7.0F, 190.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0xb200, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_walk_action, &e102_texlist,
                          0.7F, EVENT_REPEAT, 1);
        event_run_to_speed(event_get_player(0), 29.2F, 0.0F, 157.8F, 0.8F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 38.5F, 8.5F, 190.3F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 15.0F, 0.0F, 0x0100);
        event_run_wait(event_get_player(0));
        event_camera_look_clear();
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 0.6F, EVENT_REPEAT, 16);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 13.0F, 3.7F, 141.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x9f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, 13.0F, 11.7F, 141.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x9f00, 0);
        event_message_wait(100, TEXT_1);
        event_message_clear();
        event_wait(30);
        event_voice_play(1760);
        event_message_wait(80, TEXT_2);
        event_wait(90);
        event_message_clear();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00be_textures[] = {{NULL, NULL}};
