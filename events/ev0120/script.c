/* EV0120: the Egg Carrier shakes under Knuckles */
#include "ev0120/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 23
#define TEXT_2_LENGTH 5
#include "ev0120/script/text.inc"

void ev0120_script(int phase)
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
        /* the camera's aim jumps between 4 and 2 above Knuckles every 4
         * steps, which shakes the view */
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 4.0F, 0.0F, 0);
        event_wait(4);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 2.0F, 0.0F, 0);
        event_wait(4);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 4.0F, 0.0F, 0);
        event_wait(4);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 2.0F, 0.0F, 0);
        event_wait(4);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 4.0F, 0.0F, 0);
        event_wait(4);
        event_voice_play(1631);
        event_talk_start(event_get_player(0), TEXT_2);
        event_message_show(TEXT_1);
        event_camera_set_pos_heading(EVENT_EASE_LINEAR, 120,
                                     event_get_player(0), 20.0F, 4.0F, 6.0F);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_clear();
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

struct texture_pvm ev0120_textures[] = {{NULL, NULL}};
