/* EV001E, Sonic's story: the pyramid in the jungle rises, and Sonic curses
 * Eggman. a light then appears, and Sonic asks whether it is calling him */
#include <ninja.h>

#include "ev001e/script.h"
#include "ev001e/sonic_action_6.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 32
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 17
#define TEXT_6_LENGTH 3
#define TEXT_7_LENGTH 1
#define TEXT_8_LENGTH 2
#define TEXT_9_LENGTH 2
#include "ev001e/script/text.inc"

extern NJS_ACTION sonic_stand_action;

extern struct event_path ev001e_light_path;
extern NJS_ACTION ev001e_sonic_action_0;
extern NJS_ACTION ev001e_sonic_action_1;
extern NJS_ACTION ev001e_sonic_action_2;
extern NJS_ACTION ev001e_sonic_action_3;
extern NJS_ACTION ev001e_sonic_action_4;
extern NJS_ACTION ev001e_sonic_action_5;

static struct task *ev001e_light = NULL;

struct texture_pvm ev001e_textures[] = {{NULL, NULL}};

void ev001e_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_001E.mlt");
        event_sound_init(5);
        /* clearing the flag drops a risen pyramid to the ground. setting it
         * again after one step starts the pyramid's rise */
        event_flags[FLAG_SONIC_MR_ENTRANCE_RUIN] = 0;
        event_set_pos(event_get_player(0), -441.3367F, -0.4F, -328.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x4000);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -533.5F, 457.68F, -581.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc100, 0xf000, 0);
        event_flags[FLAG_SONIC_MR_ENTRANCE_RUIN] = 1;
        event_camera_set_fov(EVENT_EASE_SMOOTH, 180, 0x29f5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, -529.19F, 0.95F,
                             -592.032F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 55, 0xc100, 0xf000, 0x1c00);
        event_wait(55);
        event_camera_set_ang(EVENT_EASE_LINEAR, 85, 0xc100, 0xf000, 0xe300);
        event_wait(67);
        event_sound_play(1225, -20, 0, 0);
        event_wait(18);
        event_set_pos(event_get_player(0), -569.19F, 1.0F, -592.032F);
        event_set_ang(event_get_player(0), 10, 0x4e00, 0);
        event_play_action(event_get_player(0), &ev001e_sonic_action_0,
                          &sonic_texlist, 0.8F, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -553.882F, 2.0F, -611.275F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x2200, 0x6500, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0x0700, 0x6500, 0);
        event_wait(25);
        event_sound_play(1224, 0, 0, 0);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev001e_sonic_action_1,
                          &sonic_texlist, 1.5F, 0, 0);
        event_wait_motions(event_get_player(0));
        event_voice_play(1126);
        event_play_action(event_get_player(0), &ev001e_sonic_action_4,
                          &sonic_texlist, 0.5F, 0, 14);
        event_play_action(event_get_player(0), &ev001e_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 10);
        event_wait(2);
        event_talk_start(event_get_player(0), TEXT_6);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -571.12F, 3.0F, -616.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0700, 0x8100, 0);
        event_wait(20);
        event_play_action(event_get_player(0), &ev001e_sonic_action_3,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 10);
        event_wait(10);
        event_voice_play(1127);
        event_talk_start(event_get_player(0), TEXT_7);
        event_message_show(TEXT_2);
        event_wait(30);
        event_sound_start(2, 1094, 1800);
        event_play_action(event_get_player(0), &ev001e_sonic_action_6,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_message_close();
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -552.14F, 45.7F, -704.66F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e00, 0xfb00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, -566.5556F, 3.6F, -587.86F);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 65, 0x3111);
        event_wait(23);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, -567.36F, 1.24F, -581.3F);
        event_wait(75);
        event_sound_fade_volume(2, 90, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -350.47F, 67.83F, -851.96F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0x2200, 0x0400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -398.75F, 44.12F, -787.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x0b00, 0x1100, 0x0400);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -466.5F, 7.49F, -768.65F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x0f00, 0x0600, 0x0400);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -602.24F, 3.454F,
                             -849.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 300, 0x0f00, 0xf800, 0);
        event_sound_fade_volume(2, -60, 300);
        event_wait(300);
        event_set_ang(event_get_player(0), 10, 0x7600, 2);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -564.678F, 0.3166F,
                             -603.045F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1100, 0x6f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -566.9F, 2.748F, -598.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x1100, 0x6f00, 0);
        event_play_action(event_get_player(0), &ev001e_sonic_action_2,
                          &sonic_texlist, 0.4F, EVENT_REPEAT, 0);
        ev001e_light = event_light_create(-514.7547F, 14.636F, -650.215F);
        event_wait(20);
        event_set_flags(ev001e_light, EVENT_IGNORE_GROUND);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 6);
        event_wait(20);
        event_voice_play(933);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_sound_stop(2);
        stream_request_music(MUSIC_TICAL);
        event_sound_start(3, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(3, 0, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -566.5F, 2.77F, -578.933F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a00, 0xf800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 170, -565.555F, 3.985F,
                             -583.69F);
        event_wait(1);
        event_sound_fade_volume(3, -20, 180);
        event_wait(30);
        event_play_path(ev001e_light, &ev001e_light_path, 0.2F, 0);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -564.76F, -0.1F, -599.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0x6a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -564.2F, 1.6F, -599.95F);
        event_voice_play(1128);
        event_talk_start(event_get_player(0), TEXT_8);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -560.7F, 4.53F, -581.85F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, -548.16F, 47.0F, -776.35F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 240, 0x0900, 0xfa00, 0);
        event_voice_play(1129);
        event_talk_start(event_get_player(0), TEXT_9);
        event_message_show(TEXT_5);
        event_voice_wait();
        event_wait(180);
        event_message_close();
        event_wait(20);
        event_sound_stop(3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -564.124F, -0.445F, -600.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0x6800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -566.0F, 1.695F, -597.18F);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -560.44F, 1.8F, -580.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -566.37F, 1.8F, -581.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0c00, 0xfa00, 0);
        event_wait(80);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), -569.19F, -0.364F, -592.0355F);
        event_set_ang(event_get_player(0), 10, 0x7f00, 2);
        event_reset_player(0);
        EVENT_KILL(ev001e_light);
        event_sound_stop_all();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -566.1F, 5.45F, -577.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0800, 0xff00, 0);
        event_wait(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}
