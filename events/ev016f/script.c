/* EV016F: E-102 takes the Booster */
#include <ninja.h>

#include "ev016f/e102_action_2.h"
#include "ev016f/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 64
#include "ev016f/script/text.inc"

extern NJS_ACTION e102_slide_stand_action;

/* @hack type: ev016f_e102_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps the image's MOV.L of event_camera_look_clear into
 * R9 at 0x0cb8001c. as one NJS_ACTION passed as &ev016f_e102_action_0, SHC
 * keeps event_camera_look_clear in R10. event_camera_follow_clear moves from
 * R12 to R11, event_clear_paths from R11 to R9, and event_play_action from R10
 * to R12. 16 of 336 words differ */
extern NJS_ACTION ev016f_e102_action_0[];
/* @hack type: ev016f_e102_action_1 declared as an array, which script.c passes
 * by its name. SHC then keeps the image's MOV.L of event_play_action into R10
 * at 0x0cb800a2. as one NJS_ACTION passed as &ev016f_e102_action_1, SHC keeps
 * event_play_action in R12. event_camera_follow_clear moves from R12 to R11,
 * and event_clear_paths from R11 to R10. 13 of 336 words differ */
extern NJS_ACTION ev016f_e102_action_1[];

static struct event_path_point ev016f_light_points[662] = {
#include "ev016f/script/ev016f_light_points.inc"
};

static struct event_path ev016f_light_path = {662, ev016f_light_points};

static struct task *ev016f_light = NULL;

void ev016f_script(int phase)
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
        event_set_pos(event_get_player(0), -73.17F, 1.0F, -36.76F);
        event_set_ang(event_get_player(0), 0, 0x0b79, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), ev016f_e102_action_0,
                          &e102_texlist, 0.2F, 0, 8);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 13.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 250, event_get_player(0), 18.0F,
                           0x071c, 0x638e, 0, 0, -0xf8e3, 0, 28.0F, 28.0F);
        event_play_action(event_get_player(0), ev016f_e102_action_1,
                          &e102_texlist, 0.4F, EVENT_REPEAT, 16);
        event_voice_play(411);
        event_message_wait(0, TEXT_1);
        ev016f_light =
            event_light_create(event_get_player(0)->work->pos.x + 6.803F,
                               event_get_player(0)->work->pos.y + 35.9997F,
                               event_get_player(0)->work->pos.z + 30.68F);
        event_wait(1);
        event_set_flags(ev016f_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev016f_light);
        event_play_path(ev016f_light, &ev016f_light_path, 3.0F,
                        EVENT_FROM_HERE);
        event_wait(25);
        event_message_wait(0, TEXT_2);
        event_wait(10);
        event_play_action(event_get_player(0), &ev016f_e102_action_2,
                          &e102_texlist, 0.4F, 0, 8);
        event_wait(40);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(40);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -70.0F, 17.41F, -18.85F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xf982, 0x071d, 0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_wait(35);
        event_sound_fade_volume(0, -120, 120);
        event_wait(80);
        event_message_close();
        event_wait(10);
        event_clear_paths(ev016f_light);
        EVENT_KILL(ev016f_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev016f_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        EVENT_KILL(ev016f_light);
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

struct texture_pvm ev016f_textures[] = {{NULL, NULL}};
