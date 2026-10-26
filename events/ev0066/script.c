/* EV0066, Amy's story: Amy clears Hot Shelter and rises with the balloon into
 * Tikal's light */
#include <ninja.h>

#include "ev0066/amy_action.h"
#include "ev0066/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/move.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 1
#include "ev0066/script/text.inc"

/* the balloon's slot among the event's props */
#define BALLOON_SLOT 0

/* the balloon at the goal of Amy's stages, with its knot and sign */
extern NJS_OBJECT balloon_object;

/* a rise of 615 straight up. the script plays it with EVENT_FROM_HERE, so it
 * starts where Amy stands */
static struct event_path_point ev0066_rise_points[2] = {
    {0, 0, 0, {0.0F, 0.0F, 0.0F}},
    {0, 0, 0, {0.0F, 615.0F, 0.0F}},
};

static struct event_path ev0066_rise = {2, ev0066_rise_points};

static struct task *ev0066_light = NULL;

struct texture_pvm ev0066_textures[] = {{NULL, NULL}};

void ev0066_script(int phase)
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
        event_prop_create(0.0F, 0.0F, 0.0F, 0, 0, 0, 0.5F, 0.5F, 0.5F,
                          &balloon_object, &obj_regular_texlist, BALLOON_SLOT);
        event_set_pos(event_get_player(0), 7.0F, 545.0F, -3613.0F);
        event_set_ang(event_get_player(0), 0, 0x38a6, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_play_action(event_get_player(0), &ev0066_amy_action, &amy_texlist,
                          0.5F, EVENT_REPEAT, 8);
        event_follow_weld0(event_prop_get(BALLOON_SLOT), 0, -0.5F, -1.0F, -0.5F,
                           380);
        event_clear_paths(event_get_player(0));
        event_play_path(event_get_player(0), &ev0066_rise, 0.0012F,
                        EVENT_FROM_HERE);
        event_wait(1);
        stream_request_music(MUSIC_TICAL);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  5.0F, 5.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 1, event_get_player(0), 12.0F, 0,
                           0xb8e4, 0, 0, 0xb8e4, 0, 7.0F, 7.0F);
        event_wait(80);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 3.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 350, event_get_player(0), 0.0F,
                           -0x238d, 0, 0, 0x071c, -0xa38d, 0, 70.0F, 30.0F);
        ev0066_light =
            event_light_create(event_get_player(0)->work->pos.x - 5.1599F,
                               event_get_player(0)->work->pos.y + 200.0F,
                               event_get_player(0)->work->pos.z + 0.32F);
        event_set_flags(ev0066_light, EVENT_IGNORE_GROUND);
        event_wait(50);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_move(ev0066_light, event_get_player(0)->work->pos.x - 30.1599F,
                   event_get_player(0)->work->pos.y + 200.0F,
                   event_get_player(0)->work->pos.z - 30.32F,
                   event_get_player(0)->work->pos.x - 5.1599F,
                   event_get_player(0)->work->pos.y + 80.0F,
                   event_get_player(0)->work->pos.z + 0.32F, 100);
        event_wait(95);
        event_move(ev0066_light, event_get_player(0)->work->pos.x - 5.1599F,
                   event_get_player(0)->work->pos.y + 5.0F,
                   event_get_player(0)->work->pos.z + 0.32F,
                   event_get_player(0)->work->pos.x - 5.1599F,
                   event_get_player(0)->work->pos.y + 180.0F,
                   event_get_player(0)->work->pos.z + 0.32F, 230);
        event_wait(1);
        event_voice_play(1450);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_1);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 200, event_get_player(0), -0.0F,
                           0, -0x238d, 0, 0, -0x471b, 0, 10.0F, 10.0F);
        event_white_start_hold(60, 30, 0);
        event_wait(90);
        event_move_stop_all();
        event_clear_paths(event_get_player(0));
        EVENT_KILL(ev0066_light);
        event_prop_kill(BALLOON_SLOT);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(event_get_player(0));
        event_camera_follow_clear();
        event_camera_look_clear();
        event_move_stop_all();
        EVENT_KILL(ev0066_light);
        event_prop_kill(BALLOON_SLOT);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
