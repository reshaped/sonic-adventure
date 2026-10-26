/* EV0130: Eggman mocks Amy for trying to escape and dares her to clear his
 * torture game */
#include "ev0130/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 60
#define TEXT_2_LENGTH 47
#include "ev0130/script/text.inc"

struct texture_pvm ev0130_textures[] = {{NULL, NULL}};

void ev0130_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -69.0F, 20.0F, 31.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0xe600, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -6.8F, 20.0F, 78.0F);
        stream_request_music(MUSIC_EGCARER1);
        event_voice_play(568);
        event_message_show(TEXT_1);
        event_wait(5);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 27.6F, 31.6F, 21.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf700, 0x1a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -23.0F, 31.6F, 58.7F);
        event_voice_play(570);
        event_message_show(TEXT_2);
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
