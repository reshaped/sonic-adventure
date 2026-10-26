/* EV0140: Eggman calls E-102 to battle stations over the speakers, and sends
 * him to the armory for the Booster */
#include "ev0140/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 38
#define TEXT_2_LENGTH 29
#define TEXT_3_LENGTH 50
#include "ev0140/script/text.inc"

struct texture_pvm ev0140_textures[] = {{NULL, NULL}};

void ev0140_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGCARER1);
        event_voice_play(1761);
        event_message_show(TEXT_1);
        event_voice_play(1762);
        event_message_show(TEXT_2);
        event_voice_play(1763);
        event_message_show(TEXT_3);
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
