/* EV0096, Knuckles' story: Knuckles arrives in the past a third time, by night.
 * the white screen clears around him, and he wonders what awaits him this
 * time */
#include <ninja.h>

#include "ev0096/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 17
#define TEXT_2_LENGTH 29
#define TEXT_3_LENGTH 18
#define TEXT_4_LENGTH 3
#define TEXT_5_LENGTH 4
#define TEXT_6_LENGTH 2
#include "ev0096/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

extern NJS_ACTION ev0096_knuckles_action_0;
extern NJS_ACTION ev0096_knuckles_action_1;

static struct task *ev0096_fade = NULL;

void ev0096_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_sound_init(1);
        /* a speed of 1 takes the alpha from 0.1 to 1 on the first step, so
         * the screen is white at once */
        ev0096_fade = event_fade_create(1.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_wait(1);
        event_camera_take();
        event_wait(1);
        event_camera_take();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.8F, -69.95F, 1256.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d00, 0x8e00, 0);
        event_set_pos(event_get_player(0), -3.0F, -71.0F, 1265.5F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_reset_player(0);
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 0.2F, 0, 0);
        event_talk_start(event_get_player(0), TEXT_3);
        event_wait(40);
        /* the white clears over the next 60 steps */
        /* @hack spelling: the test and the call as one expression. SHC then
         * spares R8: the function saves R9 to R14 alone (0x0cb80008 to
         * 0x0cb80016). as an if, SHC saves R8 too, event_reset_player's address
         * moves from R10 to R12, and 180 of 361 words differ */
        ev0096_fade != NULL &&
            (event_fade_set_speed(ev0096_fade, -1.0F / 60.0F), 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -4.9565F, -69.67F,
                             1258.172F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x1600, 0x8e00, 0);
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_4);
        event_wait(50);
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1638);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_look_at(event_get_player(0), -9.7F, -66.0F, 1256.0F);
        event_wait(5);
        event_look_at(event_get_player(0), 2.4F, -66.0F, 1252.0F);
        event_wait(20);
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1639);
        event_message_show(TEXT_2);
        event_play_action(event_get_player(0), &ev0096_knuckles_action_0,
                          &knuckles_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev0096_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(50);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 7.0F, -68.6F, 1273.72F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0a00, 0x0800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 3.0F, -68.6F, 1274.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0a00, 0x0800, 0);
        event_wait(60);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -3.0F, -71.0F, 1265.5F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        EVENT_KILL(ev0096_fade);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0096_textures[] = {{NULL, NULL}};
