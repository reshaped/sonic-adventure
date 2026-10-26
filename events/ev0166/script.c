/* EV0166: Sonic takes the Light Shoes */
#include <ninja.h>

#include "ev0166/script.h"
#include "ev0166/sonic_action_2.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 23
#define TEXT_2_LENGTH 50
#define TEXT_3_LENGTH 70
#define TEXT_4_LENGTH 64
#define TEXT_5_LENGTH 62
#include "ev0166/script/text.inc"

extern NJS_ACTION ev0166_sonic_action_0;
extern NJS_ACTION ev0166_sonic_action_1;

static struct event_path_point ev0166_light_points[771] = {
#include "ev0166/script/ev0166_light_points.inc"
};

static struct event_path ev0166_light_path = {771, ev0166_light_points};

static struct task *ev0166_first_light = NULL;
static struct task *ev0166_second_light = NULL;

void ev0166_script(int phase)
{
    /* @hack redundant: a discarded read of event_camera_look_clear keeps the
     * image's MOV.L of event_camera_look_clear into R9 at 0x0cb80024, and of
     * event_camera_follow_clear into R10 at 0x0cb80026. without it, the two
     * functions swap R9 and R10. 8 of 481 words differ */
    (void)event_camera_look_clear;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_set_pos(event_get_player(0), 505.0F, -93.1F, 800.0F);
        event_set_ang(event_get_player(0), 0, 0x8050, 0);
        event_wait(1);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 4.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 500, event_get_player(0), 8.0F, 0,
                           0xd555, 0, 0, -0x1871b, 0, 25.0F, 25.0F);
        ev0166_first_light =
            event_light_create(event_get_player(0)->work->pos.x + 12.864F,
                               event_get_player(0)->work->pos.y + 25.0F,
                               event_get_player(0)->work->pos.z - 5.767F);
        event_wait(1);
        event_set_flags(ev0166_first_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev0166_first_light);
        event_play_path(ev0166_first_light, &ev0166_light_path, 3.0F,
                        EVENT_FROM_HERE);
        event_voice_play(751);
        event_message_wait(0, TEXT_1);
        event_play_action(event_get_player(0), &ev0166_sonic_action_0,
                          &sonic_texlist, 0.1F, 0, 16);
        event_wait(30);
        event_message_wait(0, TEXT_2);
        event_wait(20);
        event_sound_stop(0);
        event_play_action(event_get_player(0), &ev0166_sonic_action_1,
                          &sonic_texlist, 0.6F, EVENT_REPEAT, 16);
        event_wait(80);
        event_message_wait(0, TEXT_3);
        event_wait(80);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_clear_paths(ev0166_first_light);
        EVENT_KILL(ev0166_first_light);
        event_wait(60);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 4.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 150, event_get_player(0), 5.0F, 0,
                           0x2aab, 0, 0, -0x9554, 0, 15.0F, 15.0F);
        event_message_wait(0, TEXT_4);
        event_wait(10);
        ev0166_second_light =
            event_light_create(event_get_player(0)->work->pos.x + 12.864F,
                               event_get_player(0)->work->pos.y + 25.0F,
                               event_get_player(0)->work->pos.z - 5.767F);
        event_wait(1);
        event_set_flags(ev0166_second_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev0166_second_light);
        event_play_path(ev0166_second_light, &ev0166_light_path, 3.0F,
                        EVENT_FROM_HERE);
        event_wait(1);
        event_wait(100);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 510.0F, -89.4F, 812.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x2d1, 0x9a7, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 495.2F, -90.9F, 812.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0xad1, 0xf318, 0);
        event_wait(50);
        event_sound_fade_volume(0, -120, 120);
        event_message_wait(0, TEXT_5);
        event_play_action(event_get_player(0), &ev0166_sonic_action_2,
                          &sonic_texlist, 0.2F, 0, 16);
        event_wait(80);
        event_message_close();
        event_wait(20);
        break;
    case EVENT_END:
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 495.2F, -90.9F, 812.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xad1, 0xf318, 0);
        event_wait(1);
        event_sound_stop_all();
        event_clear_paths(ev0166_first_light);
        event_clear_paths(ev0166_second_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_voice_stop();
        event_message_close();
        EVENT_KILL(ev0166_first_light);
        EVENT_KILL(ev0166_second_light);
        event_camera_release();
        event_pads_on();
        /* a second clear of the camera's follow and look. it changes nothing:
         * no call since the first sets either */
        event_camera_follow_clear();
        event_camera_look_clear();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0166_textures[] = {{NULL, NULL}};
