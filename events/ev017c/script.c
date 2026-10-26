/* EV017C: the west rock breaks apart and the island door opens in the Mystic
 * Ruins, in E-102's story */
#include "ev017c/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

void ev017c_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0x6600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -173.35F, 85.6F, 691.45F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0xf000, 0x3e00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, -121.0F, 207.77F, 901.94F);
        event_wait(130);
        stream_request_music(MUSIC_ITEM1);
        event_wait(85);
        break;
    case EVENT_END:
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf100, 0x6f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 651.0F, 94.0F, -188.1F);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev017c_textures[] = {{NULL, NULL}};
