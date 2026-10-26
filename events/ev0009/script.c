/* EV0009, Sonic's story: by the casino, Eggman's beam takes the white Chaos
 * Emerald, and Eggman flies off laughing */
#include <ninja.h>

#include "ev0009/eggmobile_action.h"
#include "ev0009/emerald_textures.h"
#include "ev0009/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
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

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 32
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 25
#define TEXT_7_LENGTH 23
#define TEXT_8_LENGTH 2
#define TEXT_9_LENGTH 2
#include "ev0009/script/text.inc"

extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_skid_action;
extern NJS_ACTION sonic_land_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_skid_action;
extern NJS_ACTION tails_stand_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_ACTION ev0009_emerald_action_0;
extern NJS_ACTION ev0009_emerald_action_1;
extern NJS_ACTION ev0009_sonic_action_0;
extern NJS_ACTION ev0009_sonic_action_1;
extern NJS_ACTION ev0009_tails_action_0;
extern NJS_ACTION ev0009_tails_action_1;

/* player 0's pad as Sonic runs, a key a step. the stick stays pushed near full
 * to the 34th key and lets go at the 36th. the 5th to 8th keys hold B, and so
 * do the 12th to 18th */
static const struct event_pad_key ev0009_sonic_keys[107] = {
#include "ev0009/script/ev0009_sonic_keys.inc"
};

static const struct event_pad_recording ev0009_sonic_run = {
    ev0009_sonic_keys,
    sizeof(ev0009_sonic_keys) / sizeof(ev0009_sonic_keys[0])};

/* player 2's pad as Tails runs, a key a step. the 1st to 35th keys hold A, and
 * the stick lets go at the 57th */
static const struct event_pad_key ev0009_tails_keys[90] = {
#include "ev0009/script/ev0009_tails_keys.inc"
};

static const struct event_pad_recording ev0009_tails_run = {
    ev0009_tails_keys,
    sizeof(ev0009_tails_keys) / sizeof(ev0009_tails_keys[0])};

static struct task *ev0009_emerald = NULL;
static struct task *ev0009_beam = NULL;
static struct task *ev0009_fade = NULL;

void ev0009_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0009.mlt");
        event_sound_init(5);
        stream_request_music(MUSIC_S_SQUARE);
        event_set_pos(event_get_player(0), -610.353F, 0.0F, 925.89F);
        event_set_ang(event_get_player(0), 0, 0x1f9c, 0);
        event_reset_player(0);
        event_create_player(2, tails_step, -607.568F, 0.0F, 913.72F, 0, 0x2048,
                            0);
        event_eggmobile_create(-411.44F, 13.0F, 1091.1F, 0xfc00, 0xe000, 0);
        event_place_actor(&ev0009_emerald, -547.33F, -0.5F, 973.9F, 0, 0, 0);
        event_set_flags(ev0009_emerald, EVENT_IGNORE_GROUND);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0x1c00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -589.43F, 66.12F, 943.8F);
        event_wait(9);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_ang(EVENT_EASE_LINEAR, 105, 0x1000, 0x1a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 105, -564.0F, 48.27F, 974.87F);
        event_wait(110);
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 1.7F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &tails_run_action,
                          &tails_texlist, 1.7F, EVENT_REPEAT, 0);
        ev0009_beam = event_beam_create(-568.4F, 0.0F, 973.42F, 0, 0, 0);
        event_wait(1);
        event_beam_set_shape(ev0009_beam, 0.05F, 0.0F, 0.05F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_beam_set_shape(ev0009_beam, 0.0F, 0.0F, 0.0F, 0.08F, 0.0F, 0);
        event_wait(1);
        event_beam_set_shape(ev0009_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.005F, 0);
        event_wait(1);
        event_beam_set_shape(ev0009_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_beam_set_shape(ev0009_beam, 0.0F, 0.1F, 0.0F, 0.0F, 0.0F, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x0ff3, 0x348a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -558.51F, 0.247F, 939.06F);
        event_wait(90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x3c72);
        event_run_to_speed(event_get_player(0), -552.786F, -2.5F, 964.99F, 1.0F,
                           0.9F);
        event_run_to_speed(event_get_player(2), -553.4F, -2.5F, 954.44F, 0.9F,
                           0.9F);
        event_wait(20);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x105c, 0x32ec, -0x100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -482.0F, -2.065F,
                             998.373F);
        event_wait(40);
        stream_stop_music();
        event_run_wait(event_get_player(0));
        event_voice_play(933);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_skid_action,
                          &sonic_texlist, 0.9F, 0, 0);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &tails_skid_action,
                          &tails_texlist, 0.9F, 0, 0);
        event_play_action(event_get_player(2), &ev0009_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_message_show(TEXT_1);
        event_eggmobile_fly_to(-503.47F, 4.0F, 1000.72F, 30);
        event_sound_start(2, 1227, 1800);
        event_wait(20);
        stream_request_music(MUSIC_EGGMAN);
        event_wait(15);
        event_eggmobile_fly_to(-446.92F, 10.0F, 1054.21F, 1);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x075b, 0x9eb1, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -517.6F, 24.054F, 980.9F);
        event_wait(1);
        event_eggmobile_fly_to(-516.2445F, 18.0F, 992.215F, 22);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 22, 0x2e39);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 22, 0x030e, 0x82f8, 0xff00);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf50e, 0x29f8, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -522.33F, 9.21F, 978.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 22, 0xf50e, 0x25f8, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 22, -543.85F, 5.6F, 970.6F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0009_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_land_action,
                          &sonic_texlist, 1.5F, 0, 0);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(22);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x010e, 0xc5f8, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -606.6F, 6.0F, 978.7F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -606.0F, 6.0F, 982.67F);
        event_eggmobile_fly_to(-602.729F, -1.0F, 927.89F, 38);
        event_wait(2);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0009_sonic_run);
        event_clear_motions(event_get_player(2));
        event_play_pad(2, &ev0009_tails_run);
        event_voice_play(993);
        event_message_show(TEXT_2);
        event_wait(25);
        event_voice_stop();
        event_message_close();
        event_sound_stop(2);
        event_set_pos(event_get_player(0), -509.52F, -2.37F, 987.18F);
        event_set_ang(event_get_player(0), 0, 0xb3f8, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(2), -520.66F, 0.5F, 947.218F);
        event_set_ang(event_get_player(2), 0, 0xf0f8, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0009_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd50e, 0xcdf8, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -557.9F, 15.978F, 980.87F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -555.0F, 11.6F, 982.0F);
        event_set_pos(ev0009_emerald, -548.06F, -2.0F, 978.4F);
        event_wait(1);
        event_clear_motions(ev0009_emerald);
        event_play_action(ev0009_emerald, &ev0009_emerald_action_0,
                          &ev0009_emerald_texlist, 1.0F, 0, 0);
        event_sound_play(1224, 0, 0, 0);
        event_play_action(ev0009_emerald, &ev0009_emerald_action_1,
                          &ev0009_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(38);
        event_eggmobile_fly_to(-603.37F, 4.0F, 974.05F, 1);
        event_wait(3);
        event_eggmobile_turn(0, 0x7cf8, 0, 1, 0);
        event_wait(3);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_voice_stop();
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0xfb00, 0xce5a, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -556.35F, 1.55F, 981.7378F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_8);
        event_voice_play(994);
        event_message_show(TEXT_3);
        event_wait(20);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -540.2F, -2.51F, 980.85F, 0.9F,
                           0.6F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &tails_run_action,
                          &tails_texlist, 1.1F, EVENT_REPEAT, 8);
        event_run_to_speed(event_get_player(2), -542.87F, -2.51F, 965.486F,
                           0.75F, 0.0F);
        event_eggmobile_fly_turn(-568.4F, 7.0F, 973.42F, 0, 0x2000, 0, 1, 0);
        event_message_close();
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -564.0F, 19.7F, 984.44F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0a70, 0x2c70, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, -561.7F, 19.74F, 974.6F);
        event_run_clear(event_get_player(0));
        event_run_clear(event_get_player(2));
        event_wait(1);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), &ev0009_eggmobile_action,
                          &event_eggmobile_texlist, 1.3F, 0, 0);
        event_set_pos(event_get_player(0), -538.9F, -2.457F, 981.127F);
        event_set_ang(event_get_player(0), 0, 0xc523, 0);
        event_set_pos(event_get_player(2), -531.88F, 0.5F, 966.28F);
        event_set_ang(event_get_player(2), 0, 0xcef8, 0);
        event_set_pos(ev0009_emerald, -550.2F, -2.5F, 978.15F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0009_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1013);
        event_message_show(TEXT_4);
        event_wait_motions(event_eggmobile_get_task());
        event_sound_play(1225, 0, 0, 0);
        event_eggmobile_set_jets(EGGMOBILE_JETS_PUFF);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1070, 0x2170, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -549.26F, 7.18F, 984.72F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -538.127F, -1.944F,
                             992.57F);
        event_voice_play(995);
        event_talk_start(event_get_player(0), TEXT_9);
        event_message_show(TEXT_5);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_ang(EVENT_EASE_LINEAR, 170, 0x1070, 0x2170, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 170, -535.46F, -2.123F, 989.6F);
        event_wait(20);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf10e, 0x38f8, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -461.856F, 27.16F, 990.454F);
        event_eggmobile_slide_to(-568.4F, 11.0F, 973.42F, 1);
        event_set_pos(ev0009_emerald, -568.4F, -2.0F, 973.42F);
        event_wait(1);
        event_set_pos(event_get_player(0), -530.0F, -1.46F, 985.9F);
        event_set_ang(event_get_player(0), 0, 0xb3f8, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0009_sonic_action_1,
                          &sonic_texlist, 0.25F, EVENT_REPEAT, 8);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0009_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_set_pos(event_get_player(2), -546.8F, -1.5F, 955.32F);
        event_set_ang(event_get_player(2), 0, 0xf0f8, 0);
        event_look_clear(event_get_player(2));
        event_sound_start(1, 1225, 1800);
        event_beam_set_mode(ev0009_beam, BEAM_OPEN);
        event_wait(13);
        event_kill_task(&ev0009_emerald);
        event_wait(13);
        event_beam_set_mode(ev0009_beam, BEAM_CLOSE);
        event_sound_stop(1);
        event_voice_play(2073);
        event_message_show(TEXT_6);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -509.47F, 9.5F, 984.083F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0xf70e, 0x36f8, 0xfe00);
        event_eggmobile_fly_to(-448.5F, 25.0F, 982.14F, 50);
        event_voice_play(996);
        event_message_show(TEXT_7);
        event_wait(30);
        ev0009_fade = event_fade_create(1.0F / 60.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_message_close();
        event_wait(40);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        /* @hack redundant: a store of NULL to ev0009_beam that never runs. with
         * it, SHC loads ev0009_beam's address into R8 at the function's entry
         * (0x0cb80024) and keeps it there for each use, such as the store of
         * the beam's task (0x0cb80248). without it, SHC loads the address
         * from a pool at each use, ev0009_script runs 4 bytes short, and 1098
         * of its 1379 words differ */
        if (0) {
            ev0009_beam = NULL;
        }
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0009_emerald);
        event_eggmobile_kill();
        EVENT_KILL(ev0009_fade);
        event_beam_kill(ev0009_beam);
        ev0009_beam = NULL;
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0009_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"CAPTUREBEAM", &event_beam_texlist},
    {"M_EM_WHITE", &ev0009_emerald_texlist},
    {"MILES", &tails_texlist},
    {NULL, NULL},
};
