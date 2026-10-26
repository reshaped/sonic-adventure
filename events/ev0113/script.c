/* EV0113: Tails sees the Sky Deck ahead */
#include "ev0113/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 31
#define TEXT_2_LENGTH 4
#include "ev0113/script/text.inc"

struct texture_pvm ev0113_textures[] = {{NULL, NULL}};

void ev0113_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_look_at(event_get_player(0), 0.0F, 747.0F, 420.0F);
        event_talk_start(event_get_player(0), TEXT_2);
        event_voice_play(2078);
        event_message_wait(60, TEXT_1);
        event_voice_wait();
        event_message_clear();
        event_look_clear(event_get_player(0));
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
