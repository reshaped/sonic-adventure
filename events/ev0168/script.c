/* EV0168: Tails takes the Jet Anklet */
#include <ninja.h>

#include "ev0168/tails_action_2.h"
#include "ev0168/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 50
#include "ev0168/script/text.inc"

/* @hack type: ev0168_tails_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps each call's load of the 16 it passes as blend, as
 * the image's MOV #16,R3 at 0x0cb800f8. as one NJS_ACTION passed as
 * &ev0168_tails_action_0, SHC holds the 16 in R9 and event_play_action in R8.
 * ev0168_script then saves R8 besides R9 to R14. 144 of 318 words differ */
extern NJS_ACTION ev0168_tails_action_0[];
extern NJS_ACTION ev0168_tails_action_1;

static struct event_path_point ev0168_light_points[472] = {
#include "ev0168/script/ev0168_light_points.inc"
};

static struct event_path ev0168_light_path = {472, ev0168_light_points};

static struct task *ev0168_light = NULL;

void ev0168_script(int phase)
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
        event_set_pos(event_get_player(0), 697.92F, 47.99F, 1255.25F);
        event_set_ang(event_get_player(0), 0, 0xf3fc, 0);
        event_wait(1);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 3.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 250, event_get_player(0), 6.0F, 0,
                           -0xf8e3, 0, 0, 0, 0, 10.6F, 10.6F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), ev0168_tails_action_0,
                          &tails_texlist, 0.1F, 0, 16);
        event_wait(35);
        event_voice_play(752);
        event_message_wait(0, TEXT_1);
        ev0168_light =
            event_light_create(event_get_player(0)->work->pos.x - 11.764F,
                               event_get_player(0)->work->pos.y + 23.0F,
                               event_get_player(0)->work->pos.z + 6.254F);
        event_wait(1);
        event_set_flags(ev0168_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev0168_light);
        event_play_path(ev0168_light, &ev0168_light_path, 3.5F,
                        EVENT_FROM_HERE);
        event_play_action(event_get_player(0), &ev0168_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(10);
        event_message_wait(0, TEXT_2);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_wait(30);
        event_play_action(event_get_player(0), &ev0168_tails_action_2,
                          &tails_texlist, 0.6F, 0, 16);
        event_wait(30);
        event_message_close();
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, 697.92F, 53.07F, 1261.96F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x491, 0, 0);
        event_wait(1);
        event_sound_fade_volume(0, -80, 80);
        event_wait(70);
        event_clear_paths(ev0168_light);
        EVENT_KILL(ev0168_light);
        event_wait(10);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev0168_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        EVENT_KILL(ev0168_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0168_textures[] = {{NULL, NULL}};
