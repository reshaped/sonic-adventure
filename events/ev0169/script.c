/* EV0169: Tails takes the Rhythm Brooch */
#include <ninja.h>

#include "ev0169/tails_action_2.h"
#include "ev0169/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 23
#define TEXT_2_LENGTH 64
#include "ev0169/script/text.inc"

extern NJS_ACTION ev0169_tails_action_0;
extern NJS_ACTION ev0169_tails_action_1;

static struct event_path_point ev0169_light_points[472] = {
#include "ev0169/script/ev0169_light_points.inc"
};

static struct event_path ev0169_light_path = {472, ev0169_light_points};

static struct task *ev0169_light = NULL;

void ev0169_script(int phase)
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
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_set_pos(event_get_player(0), -789.0F, 100.25F, 904.0F);
        event_set_ang(event_get_player(0), 0, 0xf89a, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0169_tails_action_0,
                          &tails_texlist, 0.1F, 0, 16);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 3.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 230, event_get_player(0), 6.0F, 0,
                           0x4000, 0, 0, -0x1071b, 0, 20.6F, 20.6F);
        event_voice_play(474);
        event_message_wait(0, TEXT_1);
        event_play_action(event_get_player(0), &ev0169_tails_action_1,
                          &tails_texlist, 0.2F, EVENT_REPEAT, 8);
        ev0169_light =
            event_light_create(event_get_player(0)->work->pos.x - 11.764F,
                               event_get_player(0)->work->pos.y + 23.0F,
                               event_get_player(0)->work->pos.z + 6.254F);
        event_wait(1);
        event_set_flags(ev0169_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev0169_light);
        event_play_path(ev0169_light, &ev0169_light_path, 3.0F,
                        EVENT_FROM_HERE);
        event_wait(35);
        event_message_wait(0, TEXT_2);
        event_wait(40);
        event_play_action(event_get_player(0), &ev0169_tails_action_2,
                          &tails_texlist, 0.6F, 0, 8);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -791.02F, 105.97F, 915.49F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xfb15, 0xf8e5, 0);
        event_wait(30);
        event_sound_fade_volume(0, -80, 80);
        event_wait(40);
        event_message_close();
        event_clear_paths(ev0169_light);
        EVENT_KILL(ev0169_light);
        event_wait(10);
        break;
    case EVENT_END:
        /* @hack redundant: calls of event_camera_follow_clear and
         * event_camera_look_clear that never run keep the image's MOV.L of
         * event_camera_follow_clear into R12 at 0x0cb8001a. without them,
         * event_camera_follow_clear moves from R12 to R10, event_clear_paths
         * from R10 to R9, and event_play_action from R9 to R12. 13 of 316 words
         * differ */
        if (0) {
            event_camera_follow_clear();
            event_camera_look_clear();
        }
        event_sound_stop_all();
        event_clear_paths(ev0169_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        EVENT_KILL(ev0169_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0169_textures[] = {{NULL, NULL}};
