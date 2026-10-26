/* EV0110, Tails' story: Sonic marvels at the Egg Carrier, and Tails hurries him
 * on to find Amy */
#include <ninja.h>

#include "ev0110/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 48
#define TEXT_2_LENGTH 51
#define TEXT_3_LENGTH 42
#define TEXT_4_LENGTH 22
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 1
#define TEXT_7_LENGTH 1
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 2
#include "ev0110/script/text.inc"

extern NJS_ACTION sonic_stand_action;

extern NJS_ACTION ev0110_sonic_action;
extern NJS_ACTION ev0110_tails_action;

void ev0110_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        /* here Tails is player 0 and Sonic player 2, each where the other
         * stands in EV0100.PRS */
        event_create_player(2, sonic_step, 12.8F, 699.25F, -1093.5F, 0, 0xf8aa,
                            0);
        event_wait(1);
        stream_request_music(MUSIC_EGCARER1);
        event_set_pos(event_get_player(0), 23.39F, 699.25F, -1083.27F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 29.86F, 700.06F, -1065.27F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b0b, 0x0f8e, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -10.33F, 705.81F,
                             -1066.96F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x00ff, 0xe376, 0);
        event_talk_start(event_get_player(2), TEXT_4);
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(2057);
        event_message_show(TEXT_1);
        event_wait(120);
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(0), &ev0110_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 111.0F, 1129.0F, -1873.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xebb4, 0x7ef7, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -170.0F, 1129.0F, -1880.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xebb4, 0x7ff7, 0);
        event_voice_play(2058);
        event_message_show(TEXT_2);
        event_wait(80);
        event_message_close();
        event_message_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -10.33F, 705.81F, -1066.96F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x00ff, 0xe376, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 29.86F, 700.06F,
                             -1065.27F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 210, 0x0b0b, 0x0f8e, 0);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 3.0F,
                           0.0F);
        event_talk_start(event_get_player(0), TEXT_6);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 3.0F,
                           0.0F);
        event_wait(30);
        event_talk_start(event_get_player(2), TEXT_7);
        event_look_clear(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_8);
        event_voice_play(2059);
        event_message_show(TEXT_3);
        event_wait(30);
        event_look_clear(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_9);
        event_wait(40);
        event_play_action(event_get_player(2), &ev0110_sonic_action,
                          &sonic_texlist, 0.7F, 0, 20);
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 5);
        event_wait(40);
        event_talk_stop(event_get_player(2));
        event_message_clear();
        event_voice_wait();
        break;
    case EVENT_END:
        event_clear_task(event_get_player(0));
        event_kill_player(2);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0110_textures[] = {{NULL, NULL}};
