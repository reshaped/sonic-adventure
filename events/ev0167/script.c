/* EV0167: Sonic takes the Ancient Light */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0167/script.h"
#include "ev0167/sonic_action_2.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 17
#define TEXT_2_LENGTH 44
#define TEXT_3_LENGTH 62
#include "ev0167/script/text.inc"

/* @hack spelling: event_sprite_sparkle_texlist is
 * event_sprite_texlists[EVENT_SPRITE_SPARKLE], declared under a name of its own
 * at the element's address, 0x0cb8df98. the name keeps the image's load of that
 * address at each call, as the MOV.L at 0x0cb8011a. as
 * &event_sprite_texlists[EVENT_SPRITE_SPARKLE], SHC keeps the address in R11.
 * script.c's code then ends at 0x0cb80658, 16 bytes past the image's. 542 of
 * 601 words differ */
#ifdef GCC
/* GCC's link binds a name only where a definition starts, and this one
 * names a texlist inside event_sprite_texlists */
#define event_sprite_sparkle_texlist                                           \
    (event_sprite_texlists[EVENT_SPRITE_SPARKLE])
#else
extern NJS_TEXLIST event_sprite_sparkle_texlist;
#endif

extern NJS_ACTION ev0167_sonic_action_0;
extern NJS_ACTION ev0167_sonic_action_1;

static struct event_path_point ev0167_light_points[483] = {
#include "ev0167/script/ev0167_light_points.inc"
};

static struct event_path ev0167_light_path = {483, ev0167_light_points};

static struct task *ev0167_light = NULL;

void ev0167_script(int phase)
{
    /* @hack spelling: event_camera_look_clear called through a local. SHC then
     * keeps the image's MOV.L of event_camera_look_clear into R9 at 0x0cb80026,
     * and its load of event_camera_follow_clear at each call. called by name,
     * SHC loads event_camera_look_clear at each call and keeps
     * event_camera_follow_clear in R8. event_play_action moves from R8 to R9.
     * 38 of 601 words differ */
    void (*look_clear)(void) = event_camera_look_clear;
    /* @hack spelling: event_play_action called through a local, assigned just
     * before its first call. SHC then keeps the image's FLDI0 FR13 at
     * 0x0cb800c8. called by name, FR12, FR13 and FR14 trade what they hold. 21
     * of 601 words differ */
    void (*play_action)(struct task *task, NJS_ACTION *action,
                        NJS_TEXLIST *texlist, Float speed, int flags,
                        int blend);
    /* @hack spelling: event_message_wait called through a local, assigned just
     * before its first call. SHC then keeps the image's MOV.L of
     * event_camera_look_clear into R9 at 0x0cb80026, and of event_play_action
     * into R8 at 0x0cb800a8. called by name, the two functions swap R9 and R8.
     * FR12 and FR13 trade what they hold. 51 of 601 words differ */
    void (*message_wait)(int steps, const char *text);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -1662.0F, -366.0F, 2200.0F);
        event_set_ang(event_get_player(0), 0, 0x4bdb, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        play_action = event_play_action;
        play_action(event_get_player(0), &ev0167_sonic_action_0, &sonic_texlist,
                    0.2F, 0, 16);
        event_camera_follow_clear();
        look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 290, event_get_player(0), 8.0F, 0,
                           0x8000, 0, 0, -0xb8e3, 0, 18.0F, 18.0F);
        event_sprite_create(event_get_player(0), 0, -4.0F, 8.0F, -2.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 1, 4.0F, 8.0F, 4.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 2, 3.0F, 4.0F, -2.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 3, -3.0F, 4.0F, 3.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 5, 1.0F, 7.0F, 2.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 6, 2.0F, 2.0F, -1.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 7, -1.0F, 3.0F, 1.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 8, 4.0F, 6.0F, 3.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 9, 3.0F, 1.0F, -1.0F,
                            &event_sprite_sparkle_texlist);
        event_sprite_create(event_get_player(0), 10, -1.0F, 2.0F, 2.0F,
                            &event_sprite_sparkle_texlist);
        event_wait(1);
        event_sprite_set_scale(0, -0.1F, -0.1F);
        event_sprite_set_scale(1, -0.05F, -0.05F);
        event_sprite_set_scale(2, -0.03F, -0.03F);
        event_sprite_set_scale(3, -0.08F, -0.08F);
        event_sprite_set_scale(5, -0.09F, -0.09F);
        event_sprite_set_scale(6, -0.07F, -0.07F);
        event_sprite_set_scale(7, -0.04F, -0.04F);
        event_sprite_set_scale(8, -0.05F, -0.05F);
        event_sprite_set_scale(9, -0.09F, -0.09F);
        event_sprite_set_scale(10, -0.1F, -0.1F);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_wait(40);
        event_voice_play(473);
        message_wait = event_message_wait;
        message_wait(0, TEXT_1);
        ev0167_light =
            event_light_create(event_get_player(0)->work->pos.x + 8.535F,
                               event_get_player(0)->work->pos.y + 27.0F,
                               event_get_player(0)->work->pos.z + 13.89F);
        event_wait(1);
        event_set_flags(ev0167_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev0167_light);
        event_play_path(ev0167_light, &ev0167_light_path, 2.0F,
                        EVENT_FROM_HERE);
        play_action(event_get_player(0), &ev0167_sonic_action_1, &sonic_texlist,
                    0.5F, EVENT_REPEAT, 8);
        event_wait(30);
        message_wait(0, TEXT_2);
        event_wait(60);
        message_wait(0, TEXT_3);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_wait(10);
        event_sprite_kill(0);
        event_sprite_kill(1);
        event_sprite_kill(2);
        event_sprite_kill(3);
        event_sprite_kill(5);
        event_sprite_kill(6);
        event_sprite_kill(7);
        event_sprite_kill(8);
        event_sprite_kill(9);
        event_sprite_kill(10);
        event_wait(1);
        event_sprite_create(event_get_player(0), 4, 2.0F, 6.0F, 0.0F,
                            &event_sprite_sparkle_texlist);
        event_wait(1);
        event_sprite_set_growth(4, 0.05F, 0.05F);
        event_wait(14);
        event_sprite_set_growth(4, -0.05F, -0.05F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -1650.146F, -358.66F,
                             2197.91F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xfb7e, 0x471d, 0);
        play_action(event_get_player(0), &ev0167_sonic_action_2, &sonic_texlist,
                    0.5F, 0, 8);
        event_camera_follow_clear();
        look_clear();
        event_sound_fade_volume(0, -120, 120);
        event_wait(90);
        event_message_close();
        event_sprite_kill(0);
        event_sprite_kill(1);
        event_sprite_kill(2);
        event_sprite_kill(3);
        event_sprite_kill(4);
        event_sprite_kill(5);
        event_sprite_kill(6);
        event_sprite_kill(7);
        event_sprite_kill(8);
        event_sprite_kill(9);
        event_sprite_kill(10);
        event_clear_paths(ev0167_light);
        EVENT_KILL(ev0167_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev0167_light);
        event_sprite_kill(0);
        event_sprite_kill(1);
        event_sprite_kill(2);
        event_sprite_kill(3);
        event_sprite_kill(4);
        event_sprite_kill(5);
        event_sprite_kill(6);
        event_sprite_kill(7);
        event_sprite_kill(8);
        event_sprite_kill(9);
        event_sprite_kill(10);
        event_reset_player(0);
        event_camera_follow_clear();
        look_clear();
        event_voice_stop();
        event_message_close();
        EVENT_KILL(ev0167_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0167_textures[] = {{NULL, NULL}};
