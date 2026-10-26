/* EV0122: Knuckles senses the Master Emerald ahead */
#include "ev0122/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 32
#define TEXT_3_LENGTH 2
#define TEXT_4_LENGTH 7
#include "ev0122/script/text.inc"

struct texture_pvm ev0122_textures[] = {{NULL, NULL}};

void ev0122_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGCARER1);
        event_voice_play(1633);
        event_talk_start(event_get_player(0), TEXT_3);
        event_message_wait(60, TEXT_1);
        event_look_at(event_get_player(0), -80.0F, -67.0F, 0.0F);
        event_voice_play(1634);
        event_talk_start(event_get_player(0), TEXT_4);
        event_message_wait(120, TEXT_2);
        event_message_clear();
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_look_clear(event_get_player(0));
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
