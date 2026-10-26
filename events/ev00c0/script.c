/* EV00C0, E-102's story: Eggman calls E-102 to the Egg Carrier's rear
 * deck */
#include <ninja.h>

#include "ev00c0/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 36
#define TEXT_2_LENGTH 46
#define TEXT_3_LENGTH 9
#include "ev00c0/script/text.inc"

extern NJS_ACTION e102_stand_action;

void ev00c0_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        stream_request_music(MUSIC_EGCARER1);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 0.4F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), 20.5F, 1.0F, -53.0F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 39.4F, 3.8F, -56.16F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x03ab, 0x441b, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 40.05F, 10.8F, -56.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x14ab, 0x441b, 0x0100);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 38.76F, 26.0F, -46.24F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe8ab, 0x2e1b, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 33.57F, 42.42F, -22.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0xe3ab, 0x0f1b, 0x0100);
        event_message_show(TEXT_1);
        event_voice_play(1764);
        event_voice_wait();
        event_wait(60);
        event_message_show(TEXT_2);
        event_voice_play(1765);
        event_voice_wait();
        event_wait(130);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 40.4F, 9.8F, -66.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x01ab, 0x571b, 0x0100);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 20, 0x03ab, 0x571b, 0x0100);
        event_message_show(TEXT_3);
        event_voice_play(1766);
        event_voice_wait();
        event_wait(80);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_voice_stop();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00c0_textures[] = {{NULL, NULL}};
