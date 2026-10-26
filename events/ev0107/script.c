/* EV0107: the Egg Carrier announces that the emergency is over and its
 * monorail runs again */
#include "ev0107/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 63
#include "ev0107/script/text.inc"

void ev0107_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
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

struct texture_pvm ev0107_textures[] = {{NULL, NULL}};
