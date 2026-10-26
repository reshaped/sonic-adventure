/* EV00B8, E-102's story: Tikal's light circles E-102 beside Froggy, and
 * the screen turns white */
#include <ninja.h>

#include "ev00b8/frog_action.h"
#include "ev00b8/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_kaeru_textures.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_ACTION e102_hold_stand_action;

/* the light's path, as offsets from where it starts. it runs a little over
 * one turn around E-102 and ends between him and the camera */
static struct event_path_point ev00b8_light_points[71] = {
#include "ev00b8/script/ev00b8_light_points.inc"
};

static struct event_path ev00b8_light_path = {71, ev00b8_light_points};

static struct task *ev00b8_light = NULL;
static struct task *ev00b8_fade = NULL;
static struct task *ev00b8_frog = NULL;

void ev00b8_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_init(1);
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), 867.91F, 80.2F, 698.35F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        ev00b8_light =
            event_light_create(event_get_player(0)->work->pos.x - 20.2862F,
                               event_get_player(0)->work->pos.y + 18.0F,
                               event_get_player(0)->work->pos.z + 17.0F);
        event_set_flags(ev00b8_light, EVENT_IGNORE_GROUND);
        event_place_actor(
            &ev00b8_frog, event_get_player(0)->work->pos.x + 13.0F,
            event_get_player(0)->work->pos.y,
            event_get_player(0)->work->pos.z + 14.0F, 0, 0x8000, 0);
        event_set_flags(ev00b8_frog, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_ang(ev00b8_frog, 0, -0x4900, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00b8_frog);
        event_play_action(event_get_player(0), &e102_hold_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00b8_frog, &ev00b8_frog_action, &big_kaeru_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->pos.x,
                             event_get_player(0)->work->pos.y + 16.498F,
                             event_get_player(0)->work->pos.z + 28.821F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0, 0);
        event_play_path(ev00b8_light, &ev00b8_light_path, 0.6F,
                        EVENT_FROM_HERE);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30,
                             event_get_player(0)->work->pos.x,
                             event_get_player(0)->work->pos.y + 15.717F,
                             event_get_player(0)->work->pos.z + 24.9F);
        event_sound_start(0, SE_TIKAL_CHIME, 90);
        ev00b8_fade = event_fade_create(1.0F / 60.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_camera_release();
        event_pads_on();
        event_kill_task(&ev00b8_frog);
        event_reset_player(0);
        EVENT_KILL(ev00b8_fade);
        event_clear_paths(ev00b8_light);
        EVENT_KILL(ev00b8_light);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b8_textures[] = {
    {"BIG_KAERU", &big_kaeru_texlist},
    {NULL, NULL},
};
