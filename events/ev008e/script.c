/* EV008E, Knuckles' story: Knuckles arrives in the past a second time. the
 * white screen clears around him. he asks what is going on */
#include <ninja.h>

#include "ev008e/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 17
#define TEXT_2_LENGTH 31
#define TEXT_3_LENGTH 3
#include "ev008e/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

void ev008e_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_wait(1);
        /* in_steps 0 turns the screen white in one step, as white.h says.
         * the climb's step, 255.0F / 0, is infinity, which FTRC saturates to
         * 0x7fffffff (0x0cb80342). the Uint8 step keeps its low byte, 255 */
        event_white_start_hold(0, 30, 60);
        event_set_pos(event_get_player(0), 0.18F, -47.05F, 1393.05F);
        event_set_ang(event_get_player(0), 0, 0x8917, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -4.51F, -41.16F, 1384.88F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0x9cdc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, -6.15F, -41.16F, 1386.08F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0x0400, 0x9cdc, 0);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_wait(7);
        event_voice_play(1612);
        event_message_wait(0, TEXT_1);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.35F, -36.92F, 1351.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0xffdc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 7.13F, -41.63F, 1415.03F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0300, 0xffdc, 0);
        event_voice_play(1613);
        event_message_wait(0, TEXT_2);
        event_wait(100);
        event_message_close();
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        event_white_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev008e_textures[] = {{NULL, NULL}};
