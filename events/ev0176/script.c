/* EV0176: the Ice Stone appears in Station Square. Sonic's story plays
 * EV0176.PRS. Tails' story plays EV0177.PRS, a copy of it */
#include "ev0176/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

void ev0176_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_set_pos(event_get_player(0), -528.0F, 3.1F, 1312.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xeb00, 0x7600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -677.5F, 33.0F, 1268.72F);
        stream_request_music(MUSIC_ITEM1);
        event_wait(85);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0176_textures[] = {{NULL, NULL}};
