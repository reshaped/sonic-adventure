/* EV016D: Amy takes the Warrior Feather */
#include <ninja.h>

#include "ev016d/amy_action_2.h"
#include "ev016d/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 46
#define TEXT_2_LENGTH 60
#include "ev016d/script/text.inc"

extern NJS_ACTION ev016d_amy_action_0;
/* @hack type: ev016d_amy_action_1 declared as an array, which script.c passes
 * by its name. SHC then keeps the image's MOV.L of event_camera_look_clear into
 * R12 at 0x0cb8001a, and of event_camera_follow_clear into R9 at 0x0cb8001c. as
 * one NJS_ACTION passed as &ev016d_amy_action_1, event_camera_look_clear takes
 * R9 and event_camera_follow_clear R12. 6 of 322 words differ */
extern NJS_ACTION ev016d_amy_action_1[];

static struct event_path_point ev016d_light_points[483] = {
#include "ev016d/script/ev016d_light_points.inc"
};

static struct event_path ev016d_light_path = {483, ev016d_light_points};

static struct task *ev016d_light = NULL;

void ev016d_script(int phase)
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
        event_set_pos(event_get_player(0), 0.55F, 12.79F, 0.15F);
        event_set_ang(event_get_player(0), 0, 0xf47a, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev016d_amy_action_0,
                          &amy_texlist, 0.2F, 0, 8);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 4.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 310, event_get_player(0), 8.0F, 0,
                           0x14000, 0, 0, -0x71b, 0, 16.0F, 16.0F);
        event_play_action(event_get_player(0), ev016d_amy_action_1,
                          &amy_texlist, 0.2F, EVENT_REPEAT, 16);
        event_voice_play(477);
        event_message_wait(0, TEXT_1);
        ev016d_light =
            event_light_create(event_get_player(0)->work->pos.x + 9.535F,
                               event_get_player(0)->work->pos.y + 27.0F,
                               event_get_player(0)->work->pos.z + 13.89F);
        event_wait(1);
        event_set_flags(ev016d_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev016d_light);
        event_play_path(ev016d_light, &ev016d_light_path, 2.5F,
                        EVENT_FROM_HERE);
        event_wait(115);
        event_message_wait(0, TEXT_2);
        event_play_action(event_get_player(0), &ev016d_amy_action_2,
                          &amy_texlist, 0.4F, 0, 16);
        event_wait(35);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -0.82F, 19.97F, 7.72F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xf79e, 0xf899, 0);
        event_wait(80);
        event_message_close();
        event_wait(10);
        event_clear_paths(ev016d_light);
        EVENT_KILL(ev016d_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev016d_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        EVENT_KILL(ev016d_light);
        event_camera_release();
        event_pads_on();
        /* a second event_camera_release and event_pads_on. they change
         * nothing: the event doesn't have the camera after the first release,
         * and the pads already drive the players */
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev016d_textures[] = {{NULL, NULL}};
