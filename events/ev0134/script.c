/* EV0134: back on the Egg Carrier, Amy wonders if her bird's family is still
 * aboard, and hears a voice from above */
#include <ninja.h>

#include "ev0134/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 43
#define TEXT_2_LENGTH 54
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 37
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 5
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 3
#define TEXT_9_LENGTH 1
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 1
#include "ev0134/script/text.inc"

extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev0134_amy_action;

struct texture_pvm ev0134_textures[] = {{NULL, NULL}};

void ev0134_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGCARER2);
        event_camera_set_pos_heading(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                     -10.0F, 3.0F, 15.0F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 8.0F, 0.0F, 0);
        event_camera_set_pos_heading(EVENT_EASE_LINEAR, 960,
                                     event_get_player(0), 10.0F, 3.0F, 15.0F);
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1510);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_voice_play(1511);
        event_message_show(TEXT_2);
        event_voice_wait();
        event_wait(80);
        event_talk_start(event_get_player(0), TEXT_8);
        event_wait(50);
        event_voice_play(1512);
        event_message_show(TEXT_3);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0134_amy_action, &amy_texlist,
                          0.6F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 30);
        event_wait(60);
        event_clear_motions(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_9);
        event_talk_start(event_get_player(0), TEXT_10);
        event_voice_play(1513);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(1514);
        event_message_show(TEXT_5);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_clear();
        event_clear_motions(event_get_player(0));
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
