/* EV0170: Big takes the Power Rod */
#include <ninja.h>

#include "ev0170/big_action_2.h"
#include "ev0170/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 38
#include "ev0170/script/text.inc"

extern NJS_ACTION ev0170_big_action_0;
extern NJS_ACTION ev0170_big_action_1;

static struct event_path_point ev0170_light_points[662] = {
#include "ev0170/script/ev0170_light_points.inc"
};

static struct event_path ev0170_light_path = {662, ev0170_light_points};

static struct task *ev0170_light = NULL;

void ev0170_script(int phase)
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
        event_set_pos(event_get_player(0), 1305.05F, 120.42F, -705.03F);
        event_set_ang(event_get_player(0), 0, 0xe9e1, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0170_big_action_0,
                          &big_texlist, 0.2F, 0, 16);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_play_action(event_get_player(0), &ev0170_big_action_1,
                          &big_texlist, 0.5F, EVENT_REPEAT, 8);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 10.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 200, event_get_player(0), 15.0F,
                           0, 0x31c7, 0, 0, -0x11554, 0, 25.0F, 25.0F);
        event_voice_play(479);
        event_message_wait(0, TEXT_1);
        ev0170_light =
            event_light_create(event_get_player(0)->work->pos.x + 6.803F,
                               event_get_player(0)->work->pos.y + 32.9997F,
                               event_get_player(0)->work->pos.z + 30.68F);
        event_wait(1);
        event_set_flags(ev0170_light, EVENT_IGNORE_GROUND);
        event_play_path(ev0170_light, &ev0170_light_path, 4.0F,
                        EVENT_FROM_HERE);
        event_wait(10);
        event_message_wait(0, TEXT_2);
        event_wait(45);
        event_sound_stop(0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 1296.67F, 135.81F,
                             -690.51F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xf7f6, 0xeaac, 0);
        event_play_action(event_get_player(0), &ev0170_big_action_2,
                          &big_texlist, 0.3F, 0, 16);
        event_wait(40);
        event_message_close();
        event_sound_fade_volume(0, -80, 80);
        event_wait(60);
        event_clear_paths(ev0170_light);
        EVENT_KILL(ev0170_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev0170_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        EVENT_KILL(ev0170_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0170_textures[] = {{NULL, NULL}};
