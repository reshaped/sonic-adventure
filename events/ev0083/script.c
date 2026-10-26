/* EV0083, Knuckles' story: Tikal's light appears beside Knuckles. he watches it
 * run its path. the screen fades to white */
#include <ninja.h>

#include "ev0083/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_ACTION ev0083_knuckles_action_0;
extern NJS_ACTION ev0083_knuckles_action_1;

/* the light's path, a point a step. the script makes the light at
 * (8.26, 16.994, -24.889) from Knuckles. it plays the path EVENT_FROM_HERE,
 * so each point is an offset from there */
static struct event_path_point ev0083_light_points[178] = {
#include "ev0083/script/ev0083_light_points.inc"
};

static struct event_path ev0083_light_path = {178, ev0083_light_points};

static struct task *ev0083_light = NULL;
/* the white fade that ends the event */
static struct task *ev0083_fade = NULL;

void ev0083_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), 227.61F, -199.95F, -203.73F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_wait(1);
        ev0083_light =
            event_light_create(event_get_player(0)->work->pos.x + 8.26F,
                               event_get_player(0)->work->pos.y + 16.994F,
                               event_get_player(0)->work->pos.z - 24.889F);
        event_set_flags(ev0083_light, EVENT_IGNORE_GROUND);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->pos.x + 22.46F,
                             event_get_player(0)->work->pos.y + 11.4F,
                             event_get_player(0)->work->pos.z + 0.71F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->ang.x - 0x0800,
                             0x3f00 - event_get_player(0)->work->ang.y,
                             event_get_player(0)->work->ang.z);
        event_sound_start(0, SE_TIKAL_CHIME, 260);
        event_play_path(ev0083_light, &ev0083_light_path, 1.0F,
                        EVENT_FROM_HERE);
        event_wait(50);
        event_look_at_task(event_get_player(0), ev0083_light, 0.0F, 0.0F, 0.0F);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->pos.x + 1.33F,
                             event_get_player(0)->work->pos.y + 7.6F,
                             event_get_player(0)->work->pos.z + 2.9F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, ev0083_light, 0.0F,
                                  0.0F, 0.0F, 0);
        event_set_ang(event_get_player(0), 0, 0x2000, 0);
        event_wait(29);
        event_camera_look_clear();
        event_wait(1);
        event_play_action(event_get_player(0), &ev0083_knuckles_action_0,
                          &knuckles_texlist, 0.5F, 0, 8);
        event_play_action(event_get_player(0), &ev0083_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->pos.x + 22.46F,
                             event_get_player(0)->work->pos.y + 11.4F,
                             event_get_player(0)->work->pos.z + 0.71F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->ang.x - 0x0800,
                             0x5f00 - event_get_player(0)->work->ang.y,
                             event_get_player(0)->work->ang.z);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 30, 0x1555);
        event_wait(30);
        ev0083_fade = event_fade_create(1.0F / 60.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_wait(89);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_look_clear(event_get_player(0));
        event_wait(1);
        break;
    case EVENT_END:
        event_reset_player(0);
        EVENT_KILL(ev0083_fade);
        event_sound_stop_all();
        event_clear_paths(ev0083_light);
        EVENT_KILL(ev0083_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0083_textures[] = {{NULL, NULL}};
