/* EV0102: the Egg Carrier has changed shape, and Eggman dares Sonic to get
 * through the Sky Deck */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0102/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
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
#include "ev0102/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_stand_action;

extern NJS_ACTION ev0102_sonic_action_0;
extern NJS_ACTION ev0102_sonic_action_1;
extern NJS_ACTION ev0102_tails_action;

void ev0102_script(int phase)
{
    /* @hack spelling: event_camera_set_pos called through a local. SHC then
     * keeps its address in R12 and event_camera_set_ang's in R11 (0x0cb800e0,
     * 0x0cb800fa). called by name, event_camera_set_pos takes R10 and
     * event_camera_set_ang R12, and 39 of 530 words differ */
    void (*set_pos)(enum event_ease ease, int frames, Float x, Float y,
                    Float z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_take_control();
        event_wait(4);
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_create_player(2, tails_step, -3.11F, 659.2F, 29.84F, 0, 0, 0);
        stream_request_music(MUSIC_EGCARER1);
        event_set_pos(event_get_player(0), 6.56F, 659.17F, 40.52F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at(event_get_player(0), 6.56F, 700.17F, 50.52F);
        set_pos = event_camera_set_pos;
        set_pos(EVENT_EASE_SMOOTH, 0, -3.17F, 661.82F, 37.33F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x13d7, 0x0296, 0x002f);
        event_talk_start(event_get_player(2), TEXT_9);
        event_voice_play(2067);
        event_message_show(TEXT_1);
        event_wait(60);
        event_look_at(event_get_player(2), 6.56F, 659.17F, 40.52F);
        set_pos(EVENT_EASE_SMOOTH, 90, 2.18F, 661.09F, 59.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0ed7, 0x0296, 0x002f);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 3.0F,
                           0.0F);
        event_wait(60);
        event_talk_start(event_get_player(0), TEXT_10);
        event_wait(60);
        event_look_at(event_get_player(0), 6.56F, 700.17F, 50.52F);
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(2068);
        event_message_wait(60, TEXT_2);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(2), TEXT_12);
        event_voice_play(2069);
        event_message_wait(60, TEXT_3);
        event_talk_stop(event_get_player(2));
        set_pos(EVENT_EASE_SMOOTH, 0, -287.82F, 527.06F, -2888.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff4c, 0x88ec, 0x0d00);
        set_pos(EVENT_EASE_SMOOTH, 180, 318.11F, 937.39F, -887.25F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0xff4c, 0x88ec, 0x0d00);
        event_voice_play(2070);
        event_message_wait(60, TEXT_4);
        event_voice_play(2071);
        event_message_wait(60, TEXT_5);
        set_pos(EVENT_EASE_SMOOTH, 0, 318.11F, 937.39F, -887.25F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff4c, 0x88ec, 0x0d00);
        set_pos(EVENT_EASE_SMOOTH, 240, -521.0F, 887.57F, -554.83F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 240, 0xff4c, 0x88ec, 0);
        event_voice_play(2072);
        event_message_wait(60, TEXT_6);
        event_voice_play(2073);
        event_message_wait(60, TEXT_7);
        event_look_clear(event_get_player(2));
        set_pos(EVENT_EASE_SMOOTH, 0, 2.18F, 661.09F, 59.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0ed7, 0x0296, 0x002f);
        event_play_action(event_get_player(2), &ev0102_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(event_get_player(2), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 15);
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(2074);
        event_message_show(TEXT_8);
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &ev0102_sonic_action_0,
                          &sonic_texlist, 0.5F, 0, 16);
        event_play_action(event_get_player(0), &ev0102_sonic_action_1,
                          &sonic_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 3.0F,
                           0.0F);
        event_talk_start(event_get_player(2), TEXT_14);
        event_wait(60);
        event_look_clear(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_wait();
        set_pos(EVENT_EASE_LINEAR, 80, 14.0F, 661.0F, 27.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0dd7, 0x7596, 0);
        event_talk_stop(event_get_player(0));
        event_look_at(event_get_player(0), 6.56F, 700.17F, 50.52F);
        event_message_close();
        event_wait(60);
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

struct texture_pvm ev0102_textures[] = {{NULL, NULL}};
