/* EV0020, Sonic's story: a light leads Sonic to a mural in Lost World. the
 * screen then fades to white */
#include <ninja.h>

#include "ev0020/script.h"
#include "ev0020/sonic_action.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/wave.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 1
#define TEXT_4_LENGTH 1
#include "ev0020/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;

static struct task *ev0020_light = NULL;

struct texture_pvm ev0020_textures[] = {
    {"WAVE7_WA", &event_wave_texlist},
    {NULL, NULL},
};

void ev0020_script(int phase)
{
    /* @hack spelling: event_wait called through a local. SHC then loads the
     * light's x into FR13 at 0x0cb80078, and its z into FR14 at 0x0cb8007c.
     * called by name, the two values trade registers, and 6 of 705 words
     * differ */
    void (*wait)(int steps);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_0068.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_EVTBGM04);
        ev0020_light = event_light_create(-23.14F, 25.0F, -1.6F);
        wait = event_wait;
        wait(1);
        event_set_flags(ev0020_light, EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(0), 0.41F, 10.0F, 2.67F);
        event_set_ang(event_get_player(0), 0, 0xbdb1, 0);
        wait(1);
        event_clear_motions(event_get_player(0));
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -100, 200);
        event_look_at(event_get_player(0), -23.14F, 25.0F, -1.6F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.53F, 11.85F, 20.96F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b00, 0x1666, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 9.0F, 11.85F, 19.27F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0b00, 0x1966, 0);
        wait(80);
        event_move(ev0020_light, -23.14F, 25.0F, -1.6F, -445.76F, 25.0F,
                   -11.85F, 300);
        wait(20);
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 16);
        event_run_to_speed(event_get_player(0), -385.39F, 30.02F, 4.65F, 1.72F,
                           0.06F);
        wait(10);
        event_look_clear(event_get_player(0));
        wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -109.36F, 24.08F, 46.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0xd866, 0);
        wait(50);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -257.52F, 222.07F, -125.52F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xde00, 0xa066, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -260.42F, 211.65F,
                             -128.35F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0xde00, 0xa066, 0);
        wait(60);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_sound_play(729, 0, 0, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 80, event_get_player(0), 2.0F, 0,
                           -0x471b, 0, 0, -0x6aaa, 0, 11.0F, 30.0F);
        wait(65);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_wave_emitter_create(-183.74F, 5.84F, 56.89F, 0.4F, 1.0F, 3, 10,
                                  1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -197.12F, 28.84F, 29.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe6e0, 0x9b15, 0);
        event_sound_play(1225, 100, 0, 0);
        wait(30);
        event_wave_emitter_create(-183.74F, 5.84F, 56.89F, 0.4F, 1.0F, 3, 10,
                                  1);
        wait(40);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 0);
        event_wave_emitter_kill(1);
        event_kill_task(&ev0020_light);
        event_move_stop_all();
        wait(10);
        event_set_pos(event_get_player(0), -365.35F, 30.02F, 7.09F);
        event_set_ang(event_get_player(0), 0, 0xbc2c, 0);
        wait(1);
        event_run_to_speed(event_get_player(0), -385.39F, 30.02F, 4.65F, 0.5F,
                           0.06F);
        wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -402.53F, 35.21F, 13.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0100, 0xd566, 0);
        wait(20);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        wait(30);
        event_play_action(event_get_player(0), &ev0020_sonic_action,
                          &sonic_texlist, 0.2F, EVENT_REPEAT, 16);
        wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -476.8F, 159.23F, -30.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xd7e0, 0xa715, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -465.89F, 159.23F,
                             -45.59F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xd7e0, 0xa715, 0);
        wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1130);
        event_message_wait(0, TEXT_1);
        wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -400.47F, 33.89F, -6.35F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x01da, 0xa566, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -368.01F, 33.59F, -0.49F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0699, 0x3cd0, 0);
        wait(30);
        event_voice_play(1131);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_message_wait(0, TEXT_2);
        wait(30);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -370.1F, 31.0F, -2.55F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x1eda, 0x3f66, 0);
        wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 500, -470.37F, 152.22F,
                             -43.99F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 500, 0x21da, 0x4066, 0);
        wait(250);
        event_sound_start(1, 110, 1800);
        event_white_start_hold(60, 30, 0);
        wait(90);
        /* no break: the EVENT_PLAY case runs on into the EVENT_END case.
         * play.c's event_play then calls the script again with EVENT_END,
         * which runs that code a second time */
    case EVENT_END:
        event_kill_task(&ev0020_light);
        event_move_stop_all();
        event_camera_follow_clear();
        event_camera_look_clear();
        event_sound_stop_all();
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        EVENT_KILL(ev0020_light);
        event_white_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
