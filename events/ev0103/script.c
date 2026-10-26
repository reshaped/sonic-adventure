/* EV0103: Sonic nears the Sky Deck that Eggman spoke of */
#include "ev0103/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 42
#define TEXT_2_LENGTH 4
#include "ev0103/script/text.inc"

void ev0103_script(int phase)
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
        event_voice_play(2075);
        event_message_show(TEXT_1);
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

struct texture_pvm ev0103_textures[] = {{NULL, NULL}};
