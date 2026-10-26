/* EV0011, Sonic's story: Sonic falls from the sky into Station Square after the
 * Tornado's crash. he calls for Tails and finds no sign of him */
#include <ninja.h>

#include "adv00/texlists.h"
#include "ev0011/script.h"
#include "ev0011/sonic_action_4.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 21
#define TEXT_4_LENGTH 23
#define TEXT_5_LENGTH 1
#define TEXT_6_LENGTH 1
#include "ev0011/script/text.inc"

extern NJS_ACTION sonic_trot_action;

/* the model and motion of one of Station Square's people. they lie in
 * ADV00.PRS, which must stay loaded while the event plays */
extern NJS_OBJECT dat_0cad0ffc;
extern NJS_MOTION dat_0cb18618;

extern NJS_ACTION ev0011_sonic_action_0;
extern NJS_ACTION ev0011_sonic_action_1;
extern NJS_ACTION ev0011_sonic_action_2;
extern NJS_ACTION ev0011_sonic_action_3;

static struct task *ev0011_person = NULL;
static struct task *ev0011_fade = NULL;

void ev0011_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_000d.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -673.78F, 1000.75F, 2131.0F);
        event_set_ang(event_get_player(0), 0xf700, 0xb170, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        /* sound 1229, held while Sonic falls. its pitch jumps to 8000 and
         * sweeps toward -8000 over 200 steps. after 180 of them, the pitch
         * jumps to 8000 again and sweeps to -8000 over 100 steps, louder. the
         * sound stops as he hits the ground */
        event_sound_start(0, 1229, 1800);
        event_sound_fade_pitch(0, 8000, 1);
        event_sound_fade_volume(0, -30, 1);
        event_wait(1);
        event_sound_fade_pitch(0, -8000, 200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -658.09F, 861.51F, 2361.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0300, 0x02b3, 0);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -318.22F, 5.71F, 1933.21F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x267b, 0x5484, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -318.22F, 5.71F, 1933.21F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x127b, 0x5484, 0);
        event_wait(100);
        event_set_pos(event_get_player(0), -1073.92F, 14000.8F, 2005.61F);
        event_set_ang(event_get_player(0), 0xf800, 0x42b8, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -611.57F, 379.99F, 2143.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xbc00, 0x0200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -620.31F, -1.97F,
                             2125.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xbc00, 0x6200, 0);
        event_sound_fade_volume(0, 100, 1);
        event_sound_fade_pitch(0, 8000, 1);
        event_wait(1);
        event_sound_fade_pitch(0, -8000, 100);
        event_wait(100);
        /* Sonic hits the ground. the screen goes black, and stays black while
         * he calls for Tails */
        event_sound_stop(0);
        vibration_start(0, 7, 50, 3);
        ev0011_fade = event_fade_create(1.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_sound_play(1230, 100, 0, 0);
        event_wait(5);
        event_sound_play(1230, 100, 0, 0);
        event_wait(5);
        event_sound_play(1230, 100, 0, 0);
        event_wait(5);
        event_place_actor(&ev0011_person, -653.24F, -3.5F, 2134.64F, 0, 0x840d,
                          0);
        event_wait(1);
        event_set_flags(ev0011_person, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0011_person);
        event_play_motion(ev0011_person, &dat_0cad0ffc, &dat_0cb18618,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), -673.09F, 75.0F, 2134.06F);
        event_set_ang(event_get_player(0), 0, 0x5261, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0011_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_message_wait(0, TEXT_1);
        event_voice_play(1038);
        event_wait(45);
        event_message_wait(0, TEXT_2);
        event_voice_play(1039);
        event_wait(70);
        EVENT_KILL(ev0011_fade);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 0.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 600, event_get_player(0), 15.0F,
                           0, 0x4000, 0, 0, 0x8000, 0, 80.0F, 80.0F);
        event_play_action(event_get_player(0), &ev0011_sonic_action_2,
                          &sonic_texlist, 1.0F, 0, 0);
        event_wait(30);
        event_play_action(event_get_player(0), &ev0011_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(70);
        event_message_close();
        event_play_action(event_get_player(0), &ev0011_sonic_action_3,
                          &sonic_texlist, 0.7F, 0, 0);
        event_wait(25);
        event_sound_play(1230, 100, 0, 0);
        event_wait(70);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -686.78F, -2.94F, 2124.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfa00, 0xab33, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0011_sonic_action_0,
                          &sonic_texlist, 0.2F, EVENT_REPEAT, 8);
        event_wait(120);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1040);
        event_message_wait(0, TEXT_3);
        event_wait(50);
        event_play_action(event_get_player(0), &ev0011_sonic_action_4,
                          &sonic_texlist, 0.8F, 0, 8);
        event_message_close();
        event_wait(10);
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_run_to_speed(event_get_player(0), -612.21F, -10.0F, 2106.88F,
                           2.0F, 0.06F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -603.44F, -2.36F, 2122.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf9cc, 0x3f66, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -603.44F, -2.36F, 2122.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xf9cc, 0x1466, 0);
        event_wait(25);
        event_voice_play(1041);
        event_message_wait(0, TEXT_4);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_set_ang(ev0011_person, 0, 0x55cd, 0);
        event_wait(1);
        event_set_pos(event_get_player(0), -556.67F, -9.98F, 2105.26F);
        event_set_ang(event_get_player(0), 0, 0x5aa8, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -661.44F, 0.26F, 2141.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0xcb33, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -661.58F, -1.73F, 2141.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0xfd00, 0xcb33, 0);
        event_run_to_speed(event_get_player(0), -438.19F, -0.01F, 1990.77F,
                           1.5F, 0.06F);
        event_wait(50);
        event_message_close();
        event_wait(120);
        event_reset_player(0);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_sound_stop_all();
        EVENT_KILL(ev0011_fade);
        event_set_pos(event_get_player(0), -438.19F, -0.01F, 1990.77F);
        event_set_ang(event_get_player(0), 0, 0x65dd, 0);
        event_wait(1);
        event_kill_task(&ev0011_person);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0011_textures[] = {{NULL, NULL}};
