/* EV00DA, Big's story: Big catches Froggy in Hot Shelter, and Tikal's light
 * appears over them */
#include <ninja.h>

#include "ev00da/big_action_2.h"
#include "ev00da/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 47
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#include "ev00da/script/text.inc"

extern NJS_ACTION ev00da_big_action_0;
extern NJS_ACTION ev00da_big_action_1;
extern NJS_ACTION ev00da_frog_action_0;
extern NJS_ACTION ev00da_frog_action_1;

static struct event_path_point ev00da_light_points[662] = {
#include "ev00da/script/ev00da_light_points.inc"
};

static struct event_path ev00da_light_path = {662, ev00da_light_points};

static struct task *ev00da_frog = NULL;
static struct task *ev00da_light = NULL;

void ev00da_script(int phase)
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
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), 710.12F, 40.0F, -648.96F);
        event_set_ang(event_get_player(0), 0, 0x9b2e, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 590.02F, -59.92F, -669.08F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd9c, 0xbbb0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 579.4F, -10.05F, -670.21F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0x089c, 0xbbb0, 0);
        event_voice_play(1846);
        event_message_wait(0, TEXT_1);
        event_place_actor(&ev00da_frog, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z,
                          event_get_player(0)->work->ang.x,
                          0x4000 - event_get_player(0)->work->ang.y,
                          event_get_player(0)->work->ang.z);
        event_wait(1);
        event_set_flags(ev00da_frog, EVENT_IGNORE_GROUND);
        event_set_pos(ev00da_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 0.5F,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev00da_frog, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_play_action(ev00da_frog, &ev00da_frog_action_1,
                          &ev00da_frog_texlist, 2.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev00da_big_action_1,
                          &big_texlist, 0.2F, 0, 32);
        event_wait(50);
        event_set_pos(ev00da_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 0.5F,
                      event_get_player(0)->work->pos.z);
        event_wait(50);
        event_message_close();
        event_wait(100);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 10.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 150, event_get_player(0), 15.0F,
                           0, -0x1554, 0, 0, -0x6aaa, 0, 150.0F, 25.0F);
        event_play_action(event_get_player(0), &ev00da_big_action_2,
                          &big_texlist, 0.6F, EVENT_REPEAT, 0);
        event_wait(20);
        event_voice_play(1847);
        event_message_wait(1, TEXT_2);
        event_set_pos(ev00da_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 0.5F,
                      event_get_player(0)->work->pos.z);
        event_wait(130);
        event_message_close();
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  -5.0F, 12.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 150, event_get_player(0), 10.0F,
                           0, -0x471b, 0, 0, -0x471b, 0, 20.0F, 15.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00da_big_action_0,
                          &big_texlist, 0.2F, EVENT_REPEAT, 16);
        event_set_pos(ev00da_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 0.5F,
                      event_get_player(0)->work->pos.z);
        event_clear_motions(ev00da_frog);
        event_play_action(ev00da_frog, &ev00da_frog_action_0,
                          &ev00da_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(20);
        ev00da_light =
            event_light_create(event_get_player(0)->work->pos.x + 6.803F,
                               event_get_player(0)->work->pos.y + 35.9997F,
                               event_get_player(0)->work->pos.z + 30.68F);
        event_wait(1);
        event_set_flags(ev00da_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev00da_light);
        event_play_path(ev00da_light, &ev00da_light_path, 4.0F,
                        EVENT_FROM_HERE);
        event_wait(30);
        event_voice_play(1848);
        event_message_wait(0, TEXT_3);
        event_wait(30);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 15.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 100, event_get_player(0), 15.0F,
                           0, -0x6aaa, 0, 0, -0x6aaa, 0, 13.0F, 7.0F);
        event_voice_play(1849);
        event_message_wait(0, TEXT_4);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_white_start_hold(60, 30, 0);
        event_wait(30);
        event_message_close();
        event_clear_paths(ev00da_light);
        EVENT_KILL(ev00da_light);
        event_kill_task(&ev00da_frog);
        event_wait(40);
        event_camera_follow_clear();
        event_camera_look_clear();
        break;
    case EVENT_END:
        event_clear_paths(ev00da_light);
        event_sound_stop(0);
        event_sound_stop_all();
        event_reset_player(0);
        event_message_close();
        event_camera_follow_clear();
        event_camera_look_clear();
        event_voice_stop();
        event_kill_task(&ev00da_frog);
        EVENT_KILL(ev00da_light);
        event_white_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00da_textures[] = {
    {"FROG", &ev00da_frog_texlist},
    {NULL, NULL},
};
