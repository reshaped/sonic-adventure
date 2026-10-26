/* EV0121: Knuckles wonders what happened to the Egg Carrier */
#include "ev0121/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 6
#include "ev0121/script/text.inc"

struct texture_pvm ev0121_textures[] = {{NULL, NULL}};

void ev0121_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGCARER1);
        event_camera_set_pos_heading(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                     20.0F, 4.0F, 20.0F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 4.0F, 0.0F, 0);
        event_camera_set_pos_heading(EVENT_EASE_LINEAR, 120,
                                     event_get_player(0), 20.0F, 4.0F, 6.0F);
        event_voice_play(1632);
        event_talk_start(event_get_player(0), TEXT_2);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        break;
    case EVENT_END:
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
