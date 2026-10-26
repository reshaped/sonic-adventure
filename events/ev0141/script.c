/* EV0141: E-102 learns that an E-Series robot is in the Hot Shelter, and
 * unlocks the shelter */
#include "ev0141/script.h"
#include "event/play.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 32
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 54
#define TEXT_5_LENGTH 47
#include "ev0141/script/text.inc"

struct texture_pvm ev0141_textures[] = {{NULL, NULL}};

void ev0141_script(int phase)
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
                                  0.0F, 15.0F, 0.0F, 0);
        event_camera_set_pos_heading(EVENT_EASE_LINEAR, 960,
                                     event_get_player(0), 10.0F, 3.0F, 15.0F);
        event_voice_play(1783);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_message_show(TEXT_2);
        event_wait(120);
        event_voice_play(1784);
        event_message_show(TEXT_3);
        event_voice_play(1785);
        event_message_show(TEXT_4);
        event_voice_play(1786);
        event_message_show(TEXT_5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60,
                             event_get_player(0)->work->pos.x,
                             event_get_player(0)->work->pos.y + 20.0F,
                             event_get_player(0)->work->pos.z - 40.0F);
        event_camera_look_at(EVENT_EASE_SMOOTH, 1, 0.0F, 733.0F, 536.0F, 0);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        break;
    case EVENT_END:
        /* the view EVENT_PLAY ends on, set again so that a skipped event
         * ends on it too */
        event_camera_set_pos(EVENT_EASE_LINEAR, 1,
                             event_get_player(0)->work->pos.x,
                             event_get_player(0)->work->pos.y + 20.0F,
                             event_get_player(0)->work->pos.z - 40.0F);
        event_camera_look_at(EVENT_EASE_SMOOTH, 1, 0.0F, 733.0F, 536.0F, 0);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
