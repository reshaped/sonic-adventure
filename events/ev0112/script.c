/* EV0112, Tails' story: the Egg Carrier has changed shape, and Eggman dares
 * Sonic to get through the Sky Deck */
#include <ninja.h>

#include "ev0112/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 50
#define TEXT_2_LENGTH 48
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 52
#define TEXT_5_LENGTH 68
#define TEXT_6_LENGTH 43
#define TEXT_7_LENGTH 19
#define TEXT_8_LENGTH 53
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 1
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 1
#define TEXT_15_LENGTH 1
#include "ev0112/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_stand_action;

extern NJS_ACTION ev0112_sonic_action_0;
extern NJS_ACTION ev0112_sonic_action_1;
extern NJS_ACTION ev0112_tails_action;

struct texture_pvm ev0112_textures[] = {{NULL, NULL}};

void ev0112_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_take_control();
        event_wait(4);
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_create_player(2, sonic_step, 6.56F, 659.17F, 40.52F, 0, 0, 0);
        stream_request_music(MUSIC_EGCARER1);
        event_set_pos(event_get_player(0), -3.11F, 659.2F, 29.84F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(1);
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at(event_get_player(2), 6.56F, 700.17F, 50.52F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.17F, 661.82F, 37.33F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x13d7, 0x0296, 0x002f);
        event_talk_start(event_get_player(0), TEXT_9);
        event_voice_play(2067);
        event_message_show(TEXT_1);
        event_wait(60);
        event_look_at(event_get_player(0), 6.56F, 659.17F, 40.52F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 2.18F, 661.09F, 59.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0ed7, 0x0296, 0x002f);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 3.0F,
                           0.0F);
        event_wait(60);
        event_talk_start(event_get_player(2), TEXT_10);
        event_wait(60);
        event_look_at(event_get_player(2), 6.56F, 700.17F, 50.52F);
        event_talk_start(event_get_player(2), TEXT_11);
        event_voice_play(2068);
        event_message_wait(60, TEXT_2);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(2069);
        event_message_wait(60, TEXT_3);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -287.82F, 527.06F, -2888.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff4c, 0x88ec, 0x0d00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 318.11F, 937.39F,
                             -887.25F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0xff4c, 0x88ec, 0x0d00);
        event_voice_play(2070);
        event_message_wait(60, TEXT_4);
        event_voice_play(2071);
        event_message_wait(60, TEXT_5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 318.11F, 937.39F, -887.25F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff4c, 0x88ec, 0x0d00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 240, -521.0F, 887.57F,
                             -554.83F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 240, 0xff4c, 0x88ec, 0);
        event_voice_play(2072);
        event_message_wait(60, TEXT_6);
        event_voice_play(2073);
        event_message_wait(60, TEXT_7);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.18F, 661.09F, 59.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0ed7, 0x0296, 0x002f);
        event_play_action(event_get_player(0), &ev0112_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(event_get_player(0), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 15);
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(2074);
        event_message_show(TEXT_8);
        event_look_clear(event_get_player(2));
        event_play_action(event_get_player(2), &ev0112_sonic_action_0,
                          &sonic_texlist, 0.5F, 0, 16);
        event_play_action(event_get_player(2), &ev0112_sonic_action_1,
                          &sonic_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 3.0F,
                           0.0F);
        event_talk_start(event_get_player(0), TEXT_14);
        event_wait(60);
        event_look_clear(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -8.9F, 660.7F, 20.77F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0ed7, 0x8896, 0x002f);
        event_voice_wait();
        event_clear_motions(event_get_player(2));
        event_talk_stop(event_get_player(2));
        event_look_at(event_get_player(2), 6.56F, 700.17F, 50.52F);
        event_message_close();
        event_wait(50);
        /* @hack redundant: a wait that never runs. SHC writes no code for it,
         * but counts its 0x296, the y angle of three event_camera_set_ang
         * calls, when it allocates registers. the 0x296 keeps
         * event_camera_set_pos's address in R10, event_camera_set_ang's in R11
         * and event_voice_play's in R12 (0x0cb800e0, 0x0cb800fa, 0x0cb80110).
         * it also keeps 6.56F in FR15 and 1.0F in FR14 (0x0cb80064,
         * 0x0cb800b6). event_wait(1) in its place builds as no wait does.
         * without it, the three addresses rotate, the two floats trade, and 39
         * of 536 words differ */
        if (0) {
            event_wait(0x296);
        }
        break;
    case EVENT_END:
        event_reset_player(0);
        event_kill_player(2);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
