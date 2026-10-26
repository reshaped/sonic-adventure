/* EV0150, Big's story: the Egg Carrier announces that the emergency is over and
 * its monorail runs again */
#include "ev0150/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 63
#include "ev0150/script/text.inc"

struct texture_pvm ev0150_textures[] = {{NULL, NULL}};

void ev0150_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGCARER1);
        event_voice_play(562);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_message_clear();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
