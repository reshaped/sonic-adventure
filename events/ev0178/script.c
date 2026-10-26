/* EV0178: the Ice Stone appears in Station Square in Big's story */
#include "ev0178/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

void ev0178_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_set_pos(event_get_player(0), 682.0F, 0.0F, 1523.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe300, 0xa500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 640.3F, 42.3F, 1572.1F);
        stream_request_music(MUSIC_ITEM1);
        event_wait(85);
        break;
    case EVENT_END:
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf200, 0xaa00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 298.0F, 29.3F, 1572.3F);
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), 353.0F, 0.0F, 1605.0F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0178_textures[] = {{NULL, NULL}};
