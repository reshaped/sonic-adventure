/* EV0131: Eggman can't believe that Amy has cleared his torture game */
#include "ev0131/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 35
#include "ev0131/script/text.inc"

struct texture_pvm ev0131_textures[] = {{NULL, NULL}};

void ev0131_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGCARER1);
        event_voice_play(572);
        event_message_show(TEXT_1);
        event_voice_wait();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
