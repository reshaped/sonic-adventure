/* EV016A: Knuckles takes the Fighting Gloves */
#include <ninja.h>

#include "ev016a/knuckles_action_2.h"
#include "ev016a/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 31
#define TEXT_2_LENGTH 50
#define TEXT_3_LENGTH 37
#define TEXT_4_LENGTH 62
#include "ev016a/script/text.inc"

/* @hack type: ev016a_knuckles_action_0 declared as an array, which script.c
 * passes by its name. SHC then keeps the image's MOV.L of event_play_action
 * into R9 at 0x0cb800ac, and its load of event_camera_look_clear at each call.
 * as one NJS_ACTION passed as &ev016a_knuckles_action_0, SHC loads
 * event_play_action at each call and keeps event_camera_look_clear in R8.
 * event_camera_follow_clear moves from R8 to R11, and event_clear_paths from
 * R11 to R9. script.c's code then ends at 0x0cb803e8, 4 bytes short of the
 * image's. 313 of 347 words differ */
extern NJS_ACTION ev016a_knuckles_action_0[];
extern NJS_ACTION ev016a_knuckles_action_1;

static struct event_path_point ev016a_light_points[483] = {
#include "ev016a/script/ev016a_light_points.inc"
};

static struct event_path ev016a_light_path = {483, ev016a_light_points};

static struct task *ev016a_light = NULL;

void ev016a_script(int phase)
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
        event_set_pos(event_get_player(0), 1367.87F, 417.0F, -40.0F);
        event_set_ang(event_get_player(0), 0x54, 0x9f8a, 0);
        event_wait(1);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), ev016a_knuckles_action_0,
                          &knuckles_texlist, 0.1F, 0, 16);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 390, event_get_player(0), 8.0F, 0,
                           0x10e39, 0, 0, -0x638d, 0, 16.0F, 16.0F);
        event_voice_play(815);
        event_message_wait(0, TEXT_1);
        ev016a_light =
            event_light_create(event_get_player(0)->work->pos.x + 7.535F,
                               event_get_player(0)->work->pos.y + 27.0F,
                               event_get_player(0)->work->pos.z + 13.89F);
        event_wait(1);
        event_set_flags(ev016a_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev016a_light);
        event_play_path(ev016a_light, &ev016a_light_path, 1.5F,
                        EVENT_FROM_HERE);
        event_wait(35);
        event_play_action(event_get_player(0), &ev016a_knuckles_action_1,
                          &knuckles_texlist, 0.2F, EVENT_REPEAT, 16);
        event_sound_stop(0);
        event_voice_play(816);
        event_message_wait(0, TEXT_2);
        event_wait(70);
        event_play_action(event_get_player(0), &ev016a_knuckles_action_2,
                          &knuckles_texlist, 0.3F, 0, 16);
        event_voice_play(817);
        event_message_wait(0, TEXT_3);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_voice_play(818);
        event_message_wait(0, TEXT_4);
        event_wait(40);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 1360.77F, 424.38F, -48.49F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xfaf2, 0x9c5f, 0);
        event_wait(30);
        event_sound_fade_volume(0, -120, 120);
        event_wait(70);
        event_message_close();
        event_wait(10);
        event_clear_paths(ev016a_light);
        EVENT_KILL(ev016a_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev016a_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_voice_stop();
        event_message_close();
        EVENT_KILL(ev016a_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev016a_textures[] = {{NULL, NULL}};
