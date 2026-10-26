/* EV0104: Sonic is through the Sky Deck, and Tails points him up to the
 * bridge */
#include <ninja.h>

#include "ev0104/script.h"
#include "ev0104/sonic_action.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/floor.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 27
#define TEXT_2_LENGTH 54
#define TEXT_3_LENGTH 3
#define TEXT_4_LENGTH 3
#define TEXT_5_LENGTH 1
#include "ev0104/script/text.inc"

extern NJS_ACTION sonic_stand_action;

/* the floor under Tails, until Sonic speaks */
static struct task *ev0104_floor = NULL;

void ev0104_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_create_player(2, tails_step, -151.22F, 175.0F, -173.36F, 0,
                            0x4000, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), -151.22F, 175.0F, -173.36F);
        event_set_ang(event_get_player(2), 0, 0x4000, 0);
        ev0104_floor = event_floor_create(event_get_player(2), 174.8F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 3.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -125.0F, 95.67F, -142.71F);
        event_set_pos(event_get_player(0), -137.79F, 160.0F, -163.36F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_wait(80);
        event_look_at(event_get_player(0), -170.22F, 175.0F, -143.36F);
        event_wait(20);
        event_look_at(event_get_player(0), -136.22F, 80.0F, -191.36F);
        EVENT_KILL(ev0104_floor);
        event_voice_play(2076);
        event_message_show(TEXT_1);
        event_play_action(event_get_player(0), &ev0104_sonic_action,
                          &sonic_texlist, 0.75F, 0, 30);
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 20);
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -137.13F, 86.78F, -176.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf6b9, 0x4ac5, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -126.66F, 89.49F, -180.33F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0xf6b9, 0x4ec5, 0);
        event_voice_play(2077);
        event_talk_start(event_get_player(2), TEXT_4);
        event_message_show(TEXT_2);
        event_wait(30);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 4.0F,
                           0.0F);
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_5);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -142.2F, 95.24F, -154.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0xeab9, 0xeac5, 0);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(20);
        break;
    case EVENT_END:
        EVENT_KILL(ev0104_floor);
        event_reset_player(0);
        event_kill_player(2);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0104_textures[] = {{NULL, NULL}};
