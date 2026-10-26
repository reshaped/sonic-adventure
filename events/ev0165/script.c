/* EV0165: Sonic takes the Crystal Ring */
#include <ninja.h>

#include "ev0165/script.h"
#include "ev0165/sonic_action_2.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 25
#define TEXT_2_LENGTH 62
#include "ev0165/script/text.inc"

/* @hack type: ev0165_sonic_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps each call's load of the 16 it passes as blend, as
 * the image's MOV #16,R2 at 0x0cb800e8. as one NJS_ACTION passed as
 * &ev0165_sonic_action_0, SHC holds the 16 in R10 and event_play_action in R9.
 * ev0165_script then saves R9 besides R10 to R14. event_camera_look_clear and
 * event_camera_follow_clear swap R12 and R11. script.c's code then ends at
 * 0x0cb80370, 4 bytes past the image's. 139 of 305 words differ */
extern NJS_ACTION ev0165_sonic_action_0[];
extern NJS_ACTION ev0165_sonic_action_1;

static struct event_path_point ev0165_light_points[472] = {
#include "ev0165/script/ev0165_light_points.inc"
};

static struct event_path ev0165_light_path = {472, ev0165_light_points};

static struct task *ev0165_light = NULL;

struct texture_pvm ev0165_textures[] = {{NULL, NULL}};

void ev0165_script(int phase)
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
        event_set_pos(event_get_player(0), -305.0F, 45.0F, 1541.0F);
        event_set_ang(event_get_player(0), 0xf55b, 0xfdc5, 0);
        event_wait(1);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 250, event_get_player(0), 8.0F, 0,
                           0x5c72, 0, 0, -0xffff, 0, 11.0F, 11.0F);
        event_play_action(event_get_player(0), ev0165_sonic_action_0,
                          &sonic_texlist, 0.2F, 0, 16);
        event_wait(45);
        ev0165_light =
            event_light_create(event_get_player(0)->work->pos.x - 11.764F,
                               event_get_player(0)->work->pos.y + 23.0F,
                               event_get_player(0)->work->pos.z + 6.254F);
        event_wait(1);
        event_set_flags(ev0165_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev0165_light);
        event_play_path(ev0165_light, &ev0165_light_path, 2.5F,
                        EVENT_FROM_HERE);
        event_voice_play(753);
        event_message_wait(0, TEXT_1);
        event_play_action(event_get_player(0), &ev0165_sonic_action_1,
                          &sonic_texlist, 0.3F, EVENT_REPEAT, 16);
        event_wait(20);
        event_message_wait(0, TEXT_2);
        event_wait(50);
        event_play_action(event_get_player(0), &ev0165_sonic_action_2,
                          &sonic_texlist, 0.3F, 0, 16);
        event_wait(30);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -80, 80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, -305.0F, 51.79F, 1550.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0xf8ad, 1, 0);
        event_wait(50);
        event_message_close();
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev0165_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_voice_stop();
        event_message_close();
        EVENT_KILL(ev0165_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
