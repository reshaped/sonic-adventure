/* EV016E: E-102 takes the Type Three Laser */
#include <ninja.h>

#include "ev016e/e102_action_2.h"
#include "ev016e/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 23
#define TEXT_2_LENGTH 58
#include "ev016e/script/text.inc"

extern NJS_ACTION e102_slide_stand_action;

/* @hack type: ev016e_e102_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps the load of the 16 at each call that passes it as
 * blend, as the image's MOV #16,R2 at 0x0cb800ae. as one NJS_ACTION passed as
 * &ev016e_e102_action_0, SHC holds the 16 in R8. ev016e_script then saves R8
 * besides R9 to R14. 143 of 334 words differ */
extern NJS_ACTION ev016e_e102_action_0[];
extern NJS_ACTION ev016e_e102_action_1;

static struct event_path_point ev016e_light_points[662] = {
#include "ev016e/script/ev016e_light_points.inc"
};

static struct event_path ev016e_light_path = {662, ev016e_light_points};

static struct task *ev016e_light = NULL;

void ev016e_script(int phase)
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
        event_set_pos(event_get_player(0), 25.3F, 56.73F, -1.2F);
        event_set_ang(event_get_player(0), 0, 0xb643, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), ev016e_e102_action_0,
                          &e102_texlist, 0.3F, 0, 16);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 14.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 250, event_get_player(0), 10.0F,
                           0, 0x10e39, 0, 0, -0x471b, 0, 30.0F, 30.0F);
        event_play_action(event_get_player(0), &ev016e_e102_action_1,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 8);
        event_voice_play(412);
        event_message_wait(0, TEXT_1);
        ev016e_light =
            event_light_create(event_get_player(0)->work->pos.x + 6.803F,
                               event_get_player(0)->work->pos.y + 35.9997F,
                               event_get_player(0)->work->pos.z + 30.68F);
        event_wait(1);
        event_set_flags(ev016e_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev016e_light);
        event_play_path(ev016e_light, &ev016e_light_path, 3.0F,
                        EVENT_FROM_HERE);
        event_wait(35);
        event_message_wait(0, TEXT_2);
        event_wait(20);
        event_play_action(event_get_player(0), &ev016e_e102_action_2,
                          &e102_texlist, 0.4F, 0, 16);
        event_wait(35);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 7.22F, 68.51F, -2.54F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x610, 0xbcfe, 0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(40);
        event_message_close();
        event_sound_fade_volume(0, -120, 120);
        event_wait(70);
        event_clear_paths(ev016e_light);
        EVENT_KILL(ev016e_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev016e_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        EVENT_KILL(ev016e_light);
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

struct texture_pvm ev016e_textures[] = {{NULL, NULL}};
