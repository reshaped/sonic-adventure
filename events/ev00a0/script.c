/* EV00A0, Knuckles' story: Knuckles sees Eggman by a glowing light,
 * wonders if it is the Master Emerald, and chases him */
#include <ninja.h>

#include "ev00a0/eggman_action_0.h"
#include "ev00a0/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 24
#define TEXT_3_LENGTH 23
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 1
#define TEXT_6_LENGTH 2
#include "ev00a0/script/text.inc"

extern NJS_ACTION knuckles_run_action;

extern NJS_ACTION ev00a0_knuckles_action_0;
extern NJS_ACTION ev00a0_knuckles_action_1;

void ev00a0_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_0091.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_EGGMAN);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x4e39);
        event_create_player(2, eggman_step, -143.89F, 108.75F, 12.23F, 0,
                            0xa6fc, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), -368.3F, 0.0F, 1666.12F);
        event_set_ang(event_get_player(2), 0, 0xc294, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), ev00a0_eggman_action_0,
                          &eggman_texlist, 1.3F, EVENT_REPEAT, 8);
        event_set_pos(event_get_player(0), -342.3F, 0.0F, 1653.25F);
        event_set_ang(event_get_player(0), 0, 0xdd36, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -329.04F, 9.72F, 1658.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd89, 0x4600, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -355.72F, 8.09F, 1662.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xfd89, 0x4600, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -364.12F, 3.55F, 1670.98F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0289, 0xde00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00a0_knuckles_action_0,
                          &knuckles_texlist, 0.5F, 0, 0);
        event_wait(30);
        event_play_action(event_get_player(0), &ev00a0_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1668);
        event_message_wait(1, TEXT_1);
        event_wait(60);
        event_run_to_speed(event_get_player(2), -398.32F, 0.0F, 1666.29F, 0.07F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -358.89F, 16.17F, 1654.32F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf713, 0x5e1a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -367.48F, 17.0F, 1659.52F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf59e, 0x4de7, 0);
        event_sound_start(0, 1225, 1800);
        /* the glow beside Eggman that Knuckles takes for the Master
         * Emerald */
        event_rays_create(-374.0F, 13.29F, 1661.0F, 1, 5, 0.1F, 0.005F,
                          0xb4ffff91, 0x00ffff32);
        event_wait(20);
        event_rays_kill();
        event_sound_stop(0);
        event_voice_play(1669);
        event_message_wait(1, TEXT_2);
        event_wait(70);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1670);
        event_message_wait(1, TEXT_3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -355.12F, 5.45F, 1665.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0213, 0xe11a, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, -349.62F, 5.86F, 1659.37F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x0213, 0xe11a, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -351.95F, 12.08F, 1624.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd13, 0x641a, 0);
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1671);
        event_message_wait(0, TEXT_4);
        event_wait(30);
        event_kill_player(2);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          &knuckles_texlist, 2.0F, EVENT_REPEAT, 8);
        event_wait(1);
        event_run_to_speed(event_get_player(0), -364.1F, 0.0F, 1658.3F, 0.35F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -355.12F, 5.45F, 1665.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0213, 0xe11a, 0);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -329.04F, 9.72F, 1658.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xfd89, 0x4600, 0);
        event_wait(40);
        event_message_close();
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_rays_kill();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -364.1F, 0.0F, 1658.3F);
        event_set_ang(event_get_player(0), 0, 0xc44e, 0);
        event_wait(1);
        event_kill_player(2);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00a0_textures[] = {
    {"EGGMAN", &eggman_texlist},
    {NULL, NULL},
};
