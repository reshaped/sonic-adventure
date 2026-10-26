/* EV016C: Amy takes the Long Hammer */
#include <ninja.h>

#include "ev016c/amy_action_2.h"
#include "ev016c/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 23
#define TEXT_2_LENGTH 58
#include "ev016c/script/text.inc"

extern NJS_ACTION ev016c_amy_action_0;
/* @hack type: ev016c_amy_action_1 declared as an array, which script.c passes
 * by its name. SHC then keeps the image's MOV.L of event_play_action into R8 at
 * 0x0cb800bc. as one NJS_ACTION passed as &ev016c_amy_action_1, SHC keeps
 * event_play_action in R10. event_camera_look_clear and event_clear_paths swap
 * R11 and R12. event_camera_follow_clear moves from R9 to R8, and
 * amy_set_hammer_scale from R10 to R9. 18 of 347 words differ */
extern NJS_ACTION ev016c_amy_action_1[];

static struct event_path_point ev016c_light_points[483] = {
#include "ev016c/script/ev016c_light_points.inc"
};

static struct event_path ev016c_light_path = {483, ev016c_light_points};

static struct task *ev016c_light = NULL;

void ev016c_script(int phase)
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
        event_set_pos(event_get_player(0), 0.55F, 12.79F, 0.15F);
        event_set_ang(event_get_player(0), 0, 0xf47a, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev016c_amy_action_0,
                          &amy_texlist, 0.2F, 0, 16);
        amy_set_hammer_scale(0, 1.26F);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 4.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 200, event_get_player(0), 8.0F, 0,
                           0xf8e4, 0, 0, 0x1555, 0, 16.0F, 16.0F);
        event_play_action(event_get_player(0), ev016c_amy_action_1,
                          &amy_texlist, 0.2F, EVENT_REPEAT, 8);
        event_voice_play(413);
        event_message_wait(0, TEXT_1);
        ev016c_light =
            event_light_create(event_get_player(0)->work->pos.x + 7.535F,
                               event_get_player(0)->work->pos.y + 27.0F,
                               event_get_player(0)->work->pos.z + 13.89F);
        event_wait(1);
        event_set_flags(ev016c_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev016c_light);
        event_play_path(ev016c_light, &ev016c_light_path, 2.5F,
                        EVENT_FROM_HERE);
        event_wait(35);
        event_sound_stop(0);
        event_message_wait(0, TEXT_2);
        event_wait(80);
        event_play_action(event_get_player(0), &ev016c_amy_action_2,
                          &amy_texlist, 0.4F, 0, 8);
        event_wait(5);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -80, 80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -1.21F, 19.51F, 10.13F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xf73a, 0xf8e5, 0);
        event_wait(50);
        amy_set_hammer_scale(0, 0.0F);
        event_wait(60);
        event_message_close();
        event_wait(10);
        event_clear_paths(ev016c_light);
        EVENT_KILL(ev016c_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        amy_set_hammer_scale(0, 0.0F);
        event_clear_paths(ev016c_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        EVENT_KILL(ev016c_light);
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

struct texture_pvm ev016c_textures[] = {{NULL, NULL}};
