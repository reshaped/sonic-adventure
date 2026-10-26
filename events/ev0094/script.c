/* EV0094, Knuckles' story: Knuckles slips aboard the Egg Carrier and sets out
 * to find the Master Emerald */
#include <ninja.h>

#include "ev0094/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 45
#define TEXT_3_LENGTH 6
#include "ev0094/script/text.inc"

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_run_action;

void ev0094_script(int phase)
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
        event_set_pos(event_get_player(0), -246.6F, 733.52F, 1145.28F);
        event_set_ang(event_get_player(0), 0, 0x6714, 0);
        event_wait(1);
        stream_request_music(MUSIC_EGCARER1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -212.31F, 738.56F, 1004.03F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          &knuckles_texlist, 2.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -147.03F, 733.51F, 1012.42F,
                           2.0F, 0.06F);
        event_wait(120);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -150.91F, 738.19F, 1002.91F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0823, 0x965f, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, -141.79F, 738.79F,
                             1002.24F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 240, 0x0323, 0x735f, 0);
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1629);
        event_message_wait(70, TEXT_1);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1630);
        event_message_wait(90, TEXT_2);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        break;
    case EVENT_END:
        /* put Knuckles where his run ends. when the event is skipped, he
         * never gets there */
        event_reset_player(0);
        event_set_pos(event_get_player(0), -146.97F, 733.52F, 1012.34F);
        event_set_ang(event_get_player(0), 0, 0x65d0, 0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0094_textures[] = {{NULL, NULL}};
