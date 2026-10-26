/* EV0106: Sonic sees the Egg Carrier has to change again, and guesses its
 * switch is on the bridge */
#include "ev0106/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 54
#define TEXT_2_LENGTH 30
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 6
#include "ev0106/script/text.inc"

void ev0106_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_set_pos(event_get_player(0), 137.95F, 1519.77F, 2887.85F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 259.0F, 2036.0F, 3712.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf200, 0xff45, 0xf400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 151.44F, 1530.22F,
                             2864.57F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 240, event_get_player(0),
                                  0.0F, 10.0F, 0.0F, 0xf400);
        event_wait(200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 400, 151.44F, 1520.22F,
                             2864.57F);
        event_voice_play(2079);
        event_talk_start(event_get_player(0), TEXT_3);
        event_message_show(TEXT_1);
        event_wait(180);
        event_talk_stop(event_get_player(0));
        event_look_at(event_get_player(0), 102.0F, 1711.0F, 2870.0F);
        event_voice_play(2080);
        event_talk_start(event_get_player(0), TEXT_4);
        event_message_show(TEXT_2);
        event_wait(150);
        event_talk_stop(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_message_clear();
        break;
    case EVENT_END:
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0106_textures[] = {{NULL, NULL}};
