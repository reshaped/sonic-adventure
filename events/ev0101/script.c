/* EV0101: Eggman sets the Egg Carrier changing into its true form */

/* EV0111.PRS has EV0101.PRS's bytes. Tails' story plays it where Sonic's
 * story plays EV0101.PRS */
#include "ev0101/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 27
#define TEXT_2_LENGTH 57
#define TEXT_3_LENGTH 17
#include "ev0101/script/text.inc"

void ev0101_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGGMBL23);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -537.96F, 623.22F,
                             -1462.91F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd80, 0x882d, 0xf400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -123.0F, 1000.0F, -706.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 300, 0xfd80, 0x882d, 0xfc00);
        event_voice_play(2060);
        event_message_wait(60, TEXT_1);
        event_voice_wait();
        event_wait(50);
        event_voice_play(2062);
        event_message_show(TEXT_2);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -64.62F, 766.69F, -134.79F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0480, 0x822d, 0xfc09);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 68.62F, 780.69F, -143.79F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0x0480, 0x822d, 0);
        event_wait(90);
        event_voice_play(2064);
        event_message_wait(60, TEXT_3);
        event_wait(120);
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

struct texture_pvm ev0101_textures[] = {{NULL, NULL}};
