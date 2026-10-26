/* EV0035, Tails' story: the white Chaos Emerald flies from Tails by the casino.
 * Eggman takes it up in his capture beam. Tails is player 0 and Sonic player
 * 2 */
#include <ninja.h>

#include "ev0035/emerald_textures.h"
#include "ev0035/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/beam.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 21
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 25
#define TEXT_6_LENGTH 17
#define TEXT_7_LENGTH 2
#define TEXT_8_LENGTH 24
#include "ev0035/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_turn_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_skid_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_ACTION ev0035_emerald_action_0;
extern NJS_ACTION ev0035_emerald_action_1;
extern NJS_ACTION ev0035_tails_action_0;
extern NJS_ACTION ev0035_tails_action_1;
extern NJS_ACTION ev0035_tails_action_2;
extern NJS_ACTION ev0035_tails_action_3;
extern NJS_ACTION ev0035_tails_action_4;
extern NJS_ACTION ev0035_sonic_action;

/* Tails' jump, a key a step. A is held for keys 4 to 25. the stick is let go
 * throughout */
static const struct event_pad_key ev0035_jump_keys[72] = {
#include "ev0035/script/ev0035_jump_keys.inc"
};

static const struct event_pad_recording ev0035_jump = {
    ev0035_jump_keys, sizeof(ev0035_jump_keys) / sizeof(ev0035_jump_keys[0])};

static struct task *ev0035_emerald = NULL;
static struct task *ev0035_beam = NULL;
static struct task *ev0035_fade = NULL;

void ev0035_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0009.mlt");
        event_sound_init(5);
        stream_request_music(MUSIC_EGGMAN);
        event_set_pos(event_get_player(0), -626.334F, 0.0F, 905.68F);
        event_set_ang(event_get_player(0), 0, 0x1f9c, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(2, sonic_step, -615.5F, 0.0F, 897.256F, 0, 0x2048,
                            0);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0102, 0xb500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -626.93F, 5.4F, 918.19F);
        event_eggmobile_create(-409.0F, 285.0F, 1091.1F, -0x1500, -0x0500,
                               0x3500);
        event_wait(2);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_wait(1);
        event_place_actor(&ev0035_emerald, -550.33F, 1.5F, 973.9F, 0, 0, 0);
        event_set_flags(ev0035_emerald, EVENT_IGNORE_GROUND);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_run_action,
                          &tails_texlist, 1.2F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -577.35F, -2.5F, 953.7F, 1.0F,
                           1.5F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_trot_action,
                          &sonic_texlist, 1.3F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), -560.11F, -2.5F, 952.784F, 0.9F,
                           0.9F);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 95, 0x0246, 0x9ff4, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 95, -593.5F, 1.6F, 936.73F);
        event_wait(30);
        ev0035_beam = event_beam_create(-584.0F, -134.0F, 958.01F, 0, 0, 0);
        event_wait(1);
        event_beam_set_shape(ev0035_beam, 0.04F, 0.0F, 0.04F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_beam_set_shape(ev0035_beam, 0.0F, 0.0F, 0.0F, 0.005F, 0.0F, 0);
        event_wait(1);
        event_beam_set_shape(ev0035_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.005F, 0);
        event_wait(1);
        event_beam_set_shape(ev0035_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 100, 0x1555);
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &tails_skid_action,
                          &tails_texlist, 1.5F, 0, 4);
        event_play_action(event_get_player(2), &sonic_turn_action,
                          &sonic_texlist, 1.0F, 0, 4);
        event_wait(25);
        event_play_action(event_get_player(0), &ev0035_tails_action_0,
                          &tails_texlist, 1.5F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.5F, EVENT_REPEAT, 8);
        event_wait(25);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_sound_start(0, 1223, 1800);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xcb51, 0x380e, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -404.8F, 321.04F, 1083.5F);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_eggmobile_fly_to(-406.05F, 17.53F, 1078.7F, 80);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xc975, 0x2c0b, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -394.0F, 52.0F, 1072.0F);
        event_wait(50);
        event_sound_fade_volume(0, -40, 1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0xa100, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -590.844F, -0.0164F, 939.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x0200, 0xa800, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -593.6F, -0.0164F, 942.6F);
        event_wait(32);
        event_sound_stop(0);
        event_sound_start(1, 1227, 1800);
        event_sound_fade_volume(1, -50, 1);
        event_eggmobile_turn(0x1500, -0x1500, -0x3000, 5, 0);
        event_wait(1);
        event_sound_fade_volume(1, 0, 30);
        event_wait(6);
        event_eggmobile_fly_turn(-619.5F, 15.5F, 936.417F, 0, -0x0500, -0x0500,
                                 33, 0);
        event_wait(5);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0035_jump);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_trot_action,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), -541.85F, 3.53F, 945.2F, 1.7F,
                           1.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 25, 0x27d4, 0xa800, 0);
        event_voice_play(1211);
        event_message_show(TEXT_1);
        event_set_pos(ev0035_emerald, -575.35F, 15.0F, 951.7F);
        event_wait(1);
        event_play_action(ev0035_emerald, &ev0035_emerald_action_0,
                          &ev0035_emerald_texlist, 1.0F, 0, 0);
        event_move(ev0035_emerald, -575.35F, 15.0F, 951.7F, -560.35F, 5.0F,
                   951.7F, 16);
        event_wait(14);
        event_camera_look_clear();
        event_wait(2);
        event_sound_stop(0);
        event_message_close();
        event_clear_motions(ev0035_emerald);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xcd00, 0x8600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -565.4F, 13.13F, 962.29F);
        event_set_ang(ev0035_emerald, 0, -0x4900, 0);
        event_play_action(ev0035_emerald, &ev0035_emerald_action_0,
                          &ev0035_emerald_texlist, 1.5F, 0, 0);
        event_play_action(ev0035_emerald, &ev0035_emerald_action_1,
                          &ev0035_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev0035_emerald, -550.0F, 3.0F, 986.0F, -564.4F, -2.5F,
                   966.65F, 9);
        event_wait(5);
        event_sound_play(1224, 0, 0, 0);
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_set_pos(event_get_player(0), -531.367F, -2.46F, 982.2577F);
        event_set_ang(event_get_player(0), 0, 0xaf00, 0);
        event_play_action(event_get_player(0), &ev0035_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(2), -520.66F, 0.5F, 947.218F);
        event_set_ang(event_get_player(2), 0, 0xcf00, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.5F, EVENT_REPEAT, 0);
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 75, 0xf221, 0xb8e5, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 75, -580.7F, 7.87F, 964.3F);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_voice_play(1212);
        event_message_show(TEXT_2);
        event_eggmobile_turn(0, -0x7400, 0, 1, 0);
        event_wait(10);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0035_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_run_to_speed(event_get_player(0), -558.78F, -2.5F, 969.6F, 0.6F,
                           1.1F);
        event_play_action(event_get_player(2), &sonic_trot_action,
                          &sonic_texlist, 1.3F, EVENT_REPEAT, 1);
        event_wait(8);
        event_run_to_speed(event_get_player(2), -549.3F, -2.5F, 957.44F, 0.7F,
                           0.0F);
        event_eggmobile_fly_to(-584.0F, 13.0F, 958.01F, 30);
        event_message_close();
        event_wait(2);
        event_set_pos(ev0035_emerald, -584.0F, -2.5F, 958.01F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -549.5F, -1.06F, 982.354F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0d00, 0x2000, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -550.7887F, -0.967F,
                             983.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0d00, 0x1c00, 0xfe00);
        event_clear_motions(event_get_player(2));
        event_sound_play(1225, 0, 0, 0);
        event_eggmobile_set_jets(EGGMOBILE_JETS_PUFF);
        event_voice_play(1013);
        event_wait(11);
        event_wait(5);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0035_tails_action_3,
                          &tails_texlist, 1.0F, 0, 8);
        event_message_show(TEXT_3);
        event_play_action(event_get_player(0), &ev0035_tails_action_4,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1213);
        event_look_clear(event_get_player(0));
        event_message_show(TEXT_4);
        event_wait(5);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0035_sonic_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_sound_start(2, 1226, 1800);
        event_beam_set_mode(ev0035_beam, BEAM_OPEN);
        event_wait(20);
        event_message_close();
        event_play_action(event_get_player(0), &ev0035_tails_action_2,
                          &tails_texlist, 0.8F, EVENT_REPEAT, 8);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -575.58F, 11.79F, 965.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xdf00, 0x1e97, 0);
        event_wait(15);
        event_move(ev0035_emerald, -584.0F, -2.5F, 958.01F, -584.0F, 16.0F,
                   958.01F, 40);
        event_wait(30);
        event_move_stop_all();
        event_kill_task(&ev0035_emerald);
        event_beam_set_mode(ev0035_beam, BEAM_CLOSE);
        event_wait(19);
        event_sound_stop(2);
        event_eggmobile_slide_to(-564.4F, 11.5F, 966.65F, 2);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -469.0F, 116.06F, 992.955F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xdb00, 0x2f00, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -495.25F, 83.0F, 981.3F);
        event_wait(3);
        event_eggmobile_turn(0, 0, 0x0200, 10, 0);
        event_wait(12);
        event_eggmobile_fly_turn(-255.7F, 279.0F, 1051.0F, 0, 0x1000, 0x4000,
                                 200, 0);
        event_sound_start(0, 1223, 1800);
        event_sound_fade_volume(0, -40, 1);
        event_voice_play(2073);
        event_message_show(TEXT_5);
        event_sound_fade_volume(0, 0, 25);
        event_wait(25);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -562.456F, 1.34F, 969.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0xc92d, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -561.78F, 1.34F, 972.54F);
        event_sound_fade_volume(0, -40, 1);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_look_at(event_get_player(0), -300.0F, 253.4F, 986.0F);
        event_look_at(event_get_player(2), -300.0F, 253.4F, 986.0F);
        event_wait(1);
        event_sound_fade_volume(0, -120, 70);
        event_talk_stop(event_get_player(0));
        event_voice_play(1214);
        event_talk_start(event_get_player(0), TEXT_8);
        event_message_show(TEXT_6);
        event_wait(15);
        ev0035_fade = event_fade_create(1.0F / 60.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_message_close();
        event_wait(60);
        event_sound_stop(0);
        event_sound_stop(1);
        event_sound_stop(2);
        event_wait(1);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0035_emerald);
        event_eggmobile_kill();
        event_beam_kill(ev0035_beam);
        ev0035_beam = NULL;
        EVENT_KILL(ev0035_fade);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0035_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"M_EM_WHITE", &ev0035_emerald_texlist},
    {"SONIC", &sonic_texlist},
    {"CAPTUREBEAM", &event_beam_texlist},
    {NULL, NULL},
};
