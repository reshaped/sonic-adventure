/* EV0055, Tails' story: Tikal's light comes down to Tails and Froggy, and
 * the screen turns white */
#include <ninja.h>

#include "ev0055/script.h"
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

#define TEXT_1_LENGTH 12
#include "ev0055/script/text.inc"

extern NJS_ACTION ev0055_tails_action_0;
extern NJS_ACTION ev0055_tails_action_1;
extern NJS_TEXLIST ev0055_frog_texlist;
extern NJS_ACTION ev0055_frog_action;

static struct event_path_point ev0055_light_points[459] = {
#include "ev0055/script/ev0055_light_points.inc"
};

static struct event_path ev0055_light_path = {459, ev0055_light_points};

static struct task *ev0055_frog = NULL;
static struct task *ev0055_light = NULL;

void ev0055_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_0042.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        ev0055_light = event_light_create(1365.49F, -7370.0F, -15515.66F);
        event_set_flags(ev0055_light, EVENT_IGNORE_GROUND);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1445.2F, -7368.68F,
                             -15479.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0x2d7b, 0);
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), 1365.49F, -7370.0F, -15515.66F);
        event_set_ang(event_get_player(0), 0, 0xbac9, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0055_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_actor(&ev0055_frog, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z,
                          event_get_player(0)->work->ang.x,
                          0x4000 - event_get_player(0)->work->ang.y,
                          event_get_player(0)->work->ang.z);
        event_wait(1);
        event_set_flags(ev0055_frog, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_pos(ev0055_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 0.5F,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev0055_frog, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_play_action(ev0055_frog, &ev0055_frog_action,
                          &ev0055_frog_texlist, 1.0F, EVENT_REPEAT, 1);
        event_play_path(ev0055_light, &ev0055_light_path, 2.8F, 0);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1370.49F, -7367.7F,
                             -15507.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1700, 0x1a7b, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0055_tails_action_0,
                          &tails_texlist, 0.2F, EVENT_REPEAT, 0);
        event_wait(40);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0055_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1377.85F, -7368.08F,
                             -15517.22F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x07f3, 0x3f24, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 1377.46F, -7368.08F,
                             -15522.21F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x07f3, 0x4342, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1357.84F, -7366.31F,
                             -15511.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0af3, 0xdf42, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_1);
        event_voice_play(1255);
        event_white_start_hold(60, 30, 0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_wait(90);
        event_reset_player(0);
        event_kill_task(&ev0055_frog);
        EVENT_KILL(ev0055_light);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_clear_paths(ev0055_light);
        EVENT_KILL(ev0055_light);
        event_kill_task(&ev0055_frog);
        event_white_kill();
        event_sound_stop_all();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0055_textures[] = {
    {"FROG", &ev0055_frog_texlist},
    {NULL, NULL},
};
