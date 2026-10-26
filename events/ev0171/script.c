/* EV0171: Big takes the Life Belt */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0171/big_action_2.h"
#include "ev0171/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 50
#define TEXT_2_LENGTH 64
#include "ev0171/script/text.inc"

extern NJS_ACTION ev0171_big_action_0;
extern NJS_ACTION ev0171_big_action_1;

static struct event_path_point ev0171_light_points[662] = {
#include "ev0171/script/ev0171_light_points.inc"
};

static struct event_path ev0171_light_path = {662, ev0171_light_points};

static struct task *ev0171_light = NULL;

void ev0171_script(int phase)
{
    /* @hack spelling: event_play_action called through a local, assigned just
     * before its first call. SHC then keeps the image's MOV.L of
     * event_play_action into R12 at 0x0cb8009c, and of
     * event_camera_follow_clear into R11 at 0x0cb80018. called by name, the two
     * functions swap R12 and R11. 8 of 317 words differ */
    void (*play_action)(struct task *task, NJS_ACTION *action,
                        NJS_TEXLIST *texlist, Float speed, int flags,
                        int blend);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -951.0F, 28.0F, 255.0F);
        event_set_ang(event_get_player(0), 0, 0xbc31, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        play_action = event_play_action;
        play_action(event_get_player(0), &ev0171_big_action_0, &big_texlist,
                    0.2F, 0, 16);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 10.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 230, event_get_player(0), 15.0F,
                           0, 0xd555, 0, 0, -0x4e38, 0, 30.0F, 30.0F);
        play_action(event_get_player(0), &ev0171_big_action_1, &big_texlist,
                    0.5F, EVENT_REPEAT, 8);
        event_voice_play(478);
        event_message_wait(0, TEXT_1);
        ev0171_light =
            event_light_create(event_get_player(0)->work->pos.x + 6.803F,
                               event_get_player(0)->work->pos.y + 35.9997F,
                               event_get_player(0)->work->pos.z + 30.68F);
        event_wait(1);
        event_set_flags(ev0171_light, EVENT_IGNORE_GROUND);
        event_play_path(ev0171_light, &ev0171_light_path, 2.5F,
                        EVENT_FROM_HERE);
        event_wait(40);
        event_sound_stop(0);
        event_wait(50);
        event_message_wait(0, TEXT_2);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -973.63F, 42.014F,
                             246.77F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xf946, 0xb1c8, 0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        play_action(event_get_player(0), &ev0171_big_action_2, &big_texlist,
                    0.25F, 0, 8);
        event_wait(110);
        event_sound_fade_volume(0, -120, 120);
        event_wait(30);
        event_message_close();
        event_clear_paths(ev0171_light);
        EVENT_KILL(ev0171_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev0171_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_voice_stop();
        EVENT_KILL(ev0171_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0171_textures[] = {{NULL, NULL}};
