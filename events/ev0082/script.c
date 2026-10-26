/* EV0082, Knuckles' story: in Station Square, Knuckles resolves to gather the
 * Master Emerald's scattered pieces. he looks around and decides he can only
 * search at random */
#include <ninja.h>

#include "ev0082/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 71
#define TEXT_2_LENGTH 63
#define TEXT_3_LENGTH 31
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 17
#define TEXT_6_LENGTH 2
#include "ev0082/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

/* @hack type: ev0082_knuckles_action_0 declared as an array, which script.c
 * passes by its name. SHC then keeps the first look's z, 1551.04, in FR14 and
 * its y, 4.31, in FR15 (0x0cb80280, 0x0cb80284). as one NJS_ACTION passed as
 * &ev0082_knuckles_action_0, SHC holds them in FR12 and FR14 and the x, 53.03,
 * in FR15, and 94 of 670 words differ */
extern NJS_ACTION ev0082_knuckles_action_0[];
extern NJS_ACTION ev0082_knuckles_action_1;

void ev0082_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_S_SQUARE);
        event_set_pos(event_get_player(0), 63.29F, 0.05F, 1495.52F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0x9300, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 58.54F, 4.61F, 1488.2F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 53.27F, 1.98F, 1477.8F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0,
                             event_get_player(0)->work->pos.x + 10.0F,
                             event_get_player(0)->work->pos.y + 8.0F,
                             event_get_player(0)->work->pos.z + 23.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfb00, 0x1100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 480,
                             event_get_player(0)->work->pos.x + 5.46F,
                             event_get_player(0)->work->pos.y + 6.6F,
                             event_get_player(0)->work->pos.z + 12.8F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), ev0082_knuckles_action_0,
                          &knuckles_texlist, 0.5F, 0, 8);
        event_play_action(event_get_player(0), &ev0082_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1546);
        event_message_show(TEXT_1);
        event_wait(100);
        event_talk_stop(event_get_player(0));
        event_message_clear();
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(10);
        event_look_clear(event_get_player(0));
        event_wait(2);
        /* from here to TEXT_2, Knuckles looks from side to side, at a new
         * point every step or two */
        event_look_at(event_get_player(0), 53.03F, 4.31F, 1551.04F);
        event_wait(1);
        event_look_at(event_get_player(0), 44.92F, 4.31F, 1549.01F);
        event_wait(2);
        event_look_at(event_get_player(0), 32.34F, 7.26F, 1543.06F);
        event_wait(2);
        event_look_at(event_get_player(0), 44.92F, 6.52F, 1549.01F);
        event_wait(1);
        event_look_at(event_get_player(0), 53.03F, 5.78F, 1551.04F);
        event_wait(1);
        event_look_at(event_get_player(0), 64.34F, 5.04F, 1551.86F);
        event_wait(2);
        event_look_at(event_get_player(0), 75.2F, 4.31F, 1550.5F);
        event_wait(1);
        event_look_at(event_get_player(0), 83.21F, 4.31F, 1548.07F);
        event_wait(1);
        event_look_at(event_get_player(0), 93.16F, 4.31F, 1543.06F);
        event_wait(2);
        event_look_at(event_get_player(0), 83.21F, 4.31F, 1548.07F);
        event_wait(1);
        event_look_at(event_get_player(0), 75.2F, 4.31F, 1550.5F);
        event_wait(1);
        event_look_at(event_get_player(0), 64.34F, 4.31F, 1551.86F);
        event_wait(1);
        event_look_at(event_get_player(0), 53.03F, 4.31F, 1551.04F);
        event_wait(1);
        event_look_at(event_get_player(0), 44.92F, 4.31F, 1549.01F);
        event_wait(2);
        event_look_at(event_get_player(0), 32.34F, 4.31F, 1543.06F);
        event_wait(2);
        event_look_at(event_get_player(0), 44.92F, 4.31F, 1549.01F);
        event_wait(1);
        event_look_at(event_get_player(0), 53.03F, 4.31F, 1551.04F);
        event_wait(1);
        event_look_at(event_get_player(0), 64.34F, 4.31F, 1551.86F);
        event_wait(1);
        event_look_at(event_get_player(0), 75.2F, 4.31F, 1550.5F);
        event_wait(1);
        event_look_at(event_get_player(0), 83.21F, 4.31F, 1548.07F);
        event_wait(1);
        event_look_at(event_get_player(0), 93.16F, 4.31F, 1543.06F);
        event_wait(1);
        event_look_at(event_get_player(0), 83.21F, 4.31F, 1548.07F);
        event_wait(1);
        event_look_at(event_get_player(0), 75.2F, 4.31F, 1550.5F);
        event_wait(1);
        event_look_at(event_get_player(0), 64.34F, 4.31F, 1551.86F);
        event_wait(1);
        event_look_at(event_get_player(0), 53.03F, 4.31F, 1551.04F);
        event_wait(1);
        event_look_at(event_get_player(0), 44.92F, 4.31F, 1549.01F);
        event_wait(2);
        event_look_at(event_get_player(0), 32.34F, 4.31F, 1543.06F);
        event_wait(2);
        event_look_at(event_get_player(0), 44.92F, 4.31F, 1549.01F);
        event_wait(1);
        event_look_at(event_get_player(0), 53.03F, 4.31F, 1551.04F);
        event_wait(1);
        event_look_at(event_get_player(0), 64.34F, 4.31F, 1551.86F);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1547);
        event_message_wait(100, TEXT_2);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1548);
        event_message_wait(60, TEXT_3);
        event_talk_stop(event_get_player(0));
        event_wait(60);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), 63.29F, 0.05F, 1495.52F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0082_textures[] = {{NULL, NULL}};
