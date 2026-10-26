/* EV0086, Knuckles' story: back from the past, Knuckles finds himself in
 * Station Square */
#include <ninja.h>

#include "ev0086/knuckles_action.h"
#include "ev0086/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 16
#define TEXT_2_LENGTH 40
#define TEXT_3_LENGTH 27
#define TEXT_4_LENGTH 1
#define TEXT_5_LENGTH 2
#define TEXT_6_LENGTH 1
#include "ev0086/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

/* the white flash the event opens with */
static struct task *ev0086_flash = NULL;

void ev0086_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_sound_init(1);
        event_reset_player(0);
        event_take_control();
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        ev0086_flash = event_fade_create(1.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_wait(1);
        event_set_pos(event_get_player(0), -564.26F, -2.5F, 957.18F);
        event_set_ang(event_get_player(0), 0, 0x2000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -529.21F, 5.53F, 991.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0x2100, 0);
        event_wait(30);
        /* the flash clears over the next 60 steps */
        if (ev0086_flash != NULL) {
            event_fade_set_speed(ev0086_flash, -1.0F / 60.0F);
        }
        event_wait(30);
        event_sound_stop(0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0100, 0x2100, 0);
        event_wait(30);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, -548.04F, 6.16F, 973.18F);
        event_wait(30);
        EVENT_KILL(ev0086_flash);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1561);
        event_message_wait(60, TEXT_1);
        event_wait(30);
        event_look_at(event_get_player(0), -507.69F, 16.52F, 951.6F);
        event_wait(10);
        event_look_at(event_get_player(0), -528.2F, 16.52F, 1001.11F);
        event_wait(10);
        event_look_at(event_get_player(0), -564.25F, 16.52F, 1014.02F);
        event_wait(10);
        event_look_at(event_get_player(0), -528.2F, 16.52F, 1001.11F);
        event_wait(10);
        event_look_at(event_get_player(0), -507.69F, 16.52F, 951.6F);
        event_wait(10);
        event_look_at(event_get_player(0), -528.2F, 16.52F, 1001.11F);
        event_wait(70);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1562);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0086_knuckles_action,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1563);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(60);
        break;
    case EVENT_END:
        /* EVENT_PLAY gives Knuckles this place and heading too. EVENT_END
         * also runs after a skip, so he ends here either way */
        event_set_pos(event_get_player(0), -564.26F, -2.5F, 957.18F);
        event_set_ang(event_get_player(0), 0, 0x2000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -570.13F, 8.154F, 933.287F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd00, 0x8800, 0);
        event_reset_player(0);
        event_sound_stop_all();
        if (ev0086_flash != NULL) {
            task_kill(ev0086_flash);
            ev0086_flash = NULL;
        }
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0086_textures[] = {{NULL, NULL}};
