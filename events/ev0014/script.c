/* EV0014, Sonic's story: Sonic has lost Amy. he wonders whether Eggman's robot
 * went after her, and sets off to find her */
#include <ninja.h>

#include "ev0014/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 33
#define TEXT_2_LENGTH 48
#define TEXT_3_LENGTH 27
#define TEXT_4_LENGTH 3
#define TEXT_5_LENGTH 2
#define TEXT_6_LENGTH 1
#define TEXT_7_LENGTH 1
#include "ev0014/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;

extern NJS_ACTION ev0014_sonic_action_0;
extern NJS_ACTION ev0014_sonic_action_1;

void ev0014_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), 798.95F, 50.01F, 1771.19F);
        event_set_ang(event_get_player(0), 0, 0xc345, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 717.83F, 87.25F, 1755.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xeec5, 0xb626, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 708.99F, 67.92F, 1748.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xf9c5, 0xad26, 0);
        event_run_to_speed(event_get_player(0), 756.76F, 50.0F, 1769.88F, 1.75F,
                           0.06F);
        event_wait(60);
        event_play_action(event_get_player(0), &ev0014_sonic_action_1,
                          &sonic_texlist, 0.15F, EVENT_REPEAT, 8);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 739.2F, 58.09F, 1788.09F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfbc5, 0xdc26, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 270, 741.32F, 55.09F, 1760.51F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 270, 0xfdc5, 0xa83a, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1051);
        event_message_wait(0, TEXT_1);
        event_wait(80);
        event_voice_play(1052);
        event_talk_start(event_get_player(0), TEXT_5);
        event_message_wait(0, TEXT_2);
        event_wait(25);
        event_play_action(event_get_player(0), &ev0014_sonic_action_0,
                          &sonic_texlist, 0.2F, 0, 4);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_wait(30);
        event_voice_play(1053);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_message_wait(0, TEXT_3);
        event_wait(5);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 0.5F, 0, 16);
        event_wait(40);
        event_message_close();
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), 756.76F, 50.0F, 1769.88F);
        event_set_ang(event_get_player(0), 0, 0xc345, 0);
        event_wait(1);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0014_textures[] = {{NULL, NULL}};
