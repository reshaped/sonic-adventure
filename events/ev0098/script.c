/* EV0098, Knuckles' story: Knuckles comes back from the past and wonders if he
 * has gone mad */
#include <ninja.h>

#include "ev0098/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 35
#define TEXT_3_LENGTH 2
#define TEXT_4_LENGTH 2
#include "ev0098/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

static struct task *ev0098_fade = NULL;

void ev0098_script(int phase)
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
        event_sound_init(1);
        event_set_pos(event_get_player(0), -15.89F, -67.0F, 0.0F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        /* the screen starts white and clears by 0.0012 a step. 25 steps on,
         * event_fade_set_speed below raises that to 0.016 a step */
        ev0098_fade = event_fade_create(-0.0012F, 1.0F, 1.0F, 1.0F, 1.0F);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_wait(25);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.81F, -61.59F, -4.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x055c, 0x57cf, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 11.38F, -64.47F, -16.06F);
        if (ev0098_fade != NULL) {
            event_fade_set_speed(ev0098_fade, -0.016F);
        }
        event_wait(40);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(100);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -2.92F, -64.06F, 5.04F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x095c, 0x25cf, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -6.52F, -64.06F, 9.84F);
        event_voice_play(1647);
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_message_wait(40, TEXT_1);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_4);
        event_wait(15);
        event_voice_play(1648);
        event_message_wait(60, TEXT_2);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        EVENT_KILL(ev0098_fade);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0098_textures[] = {{NULL, NULL}};
