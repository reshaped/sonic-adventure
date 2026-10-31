/* EV0045, Tails' story: Tails meets Tikal in the past. she recites an old
 * saying of the seven Chaos */
#include <ninja.h>

#include "ev0045/script.h"
#include "ev0045/tikal_action_3.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 38
#define TEXT_3_LENGTH 17
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 63
#define TEXT_6_LENGTH 43
#define TEXT_7_LENGTH 30
#define TEXT_8_LENGTH 38
#define TEXT_9_LENGTH 38
#define TEXT_10_LENGTH 53
#define TEXT_11_LENGTH 40
#define TEXT_12_LENGTH 27
#define TEXT_13_LENGTH 13
#define TEXT_14_LENGTH 32
#define TEXT_15_LENGTH 44
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 4
#define TEXT_18_LENGTH 11
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 3
#define TEXT_21_LENGTH 2
#define TEXT_22_LENGTH 1
#define TEXT_23_LENGTH 1
#define TEXT_24_LENGTH 4
#include "ev0045/script/text.inc"

extern NJS_ACTION tails_run_action;

/* @hack type: tikal_player_step by its address in TIKAL_PROG.PRS, not by
 * name. SHC then keeps event_wait's address in R13 and event_camera_set_pos's
 * in R12 (0x0cb8008a, 0x0cb80108). by name, the two trade, and 54 of 977
 * words differ */
#ifdef GCC
/* GCC's link lays TIKAL_PROG.PRS out anew, so tikal_player_step needn't lie
 * at 0x0cb00000 */
#define TIKAL_PLAYER_STEP tikal_player_step
#else
#define TIKAL_PLAYER_STEP ((void (*)(struct task *))0x0cb00000)
#endif

extern NJS_ACTION ev0045_tikal_action_0;
extern NJS_ACTION ev0045_tikal_action_1;
extern NJS_ACTION ev0045_tikal_action_2;

void ev0045_script(int phase)
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
        stream_request_music(MUSIC_TICAL);
        event_create_player(2, TIKAL_PLAYER_STEP, 17.69F, 0.01F, 480.85F, 0,
                            0x8872, 0);
        event_wait(1);
        event_reset_player(2);
        event_set_pos(event_get_player(2), 17.69F, -0.1F, 480.85F);
        event_set_ang(event_get_player(2), 0, 0x8872, 0);
        event_wait(1);
        event_set_pos(event_get_player(0), -51.4F, -0.02F, 528.23F);
        event_set_ang(event_get_player(0), 0, 0x263d, 0);
        event_wait(1);
        event_run_to_speed(event_get_player(0), -27.91F, 0.007F, 536.82F, 0.2F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -19.36F, 2.65F, 541.28F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0285, 0x2f11, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, -19.98F, 2.04F, 549.88F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x0500, 0x1511, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_run_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1256);
        event_message_wait(0, TEXT_1);
        event_wait(100);
        event_clear_motions(event_get_player(0));
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 25, -30.93F, 4.24F, 552.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 25, 0x0200, 0xe911, 0);
        event_clear_motions(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x6ad7, 0);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 19.16F, 7.49F, 470.77F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0x6e11, 0);
        event_play_action(event_get_player(2), &ev0045_tikal_action_1,
                          &tikal_texlist, 0.5F, 0, 8);
        event_wait(1);
        event_wait_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 12.15F, 6.37F, 493.61F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0xee11, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 13.0F, 6.17F, 491.81F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xfc00, 0xee11, 0);
        event_set_ang(event_get_player(2), 0, 0xee12, 0);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0045_tikal_action_0,
                          &tikal_texlist, 0.1F, 0, 8);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_16);
        event_voice_play(1257);
        event_message_wait(0, TEXT_2);
        event_wait(30);
        event_wait_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.03F, 3.15F, 524.97F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x8511, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -28.03F, 3.15F, 524.97F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0300, 0x8511, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1258);
        event_message_wait(0, TEXT_3);
        event_wait(20);
        event_voice_play(1259);
        event_message_wait(0, TEXT_4);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -25.15F, 5.16F, 560.33F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0xf911, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 330, -45.09F, 4.19F, 543.34F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 330, 0xfb00, 0xd311, 0);
        event_voice_play(1260);
        event_message_wait(0, TEXT_5);
        event_wait(100);
        event_message_close();
        event_wait(30);
        event_voice_play(1261);
        event_message_wait(0, TEXT_6);
        event_wait(60);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_18);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 16.29F, 0.84F, 493.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1000, 0x0011, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 250, 16.28F, 2.75F, 488.78F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 250, 0x1000, 0x0011, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0045_tikal_action_2,
                          &tikal_texlist, 0.1F, 0, 8);
        event_wait(50);
        event_voice_play(1262);
        event_message_wait(0, TEXT_7);
        event_wait(90);
        event_voice_play(1263);
        event_message_wait(0, TEXT_8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.36F, 6.0F, 529.11F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0x8011, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -28.36F, 5.94F, 527.11F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x0100, 0x8011, 0);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.65F, 5.88F, 486.38F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0xcf11, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 7.58F, 5.93F, 486.02F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0x0200, 0xcf11, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_19);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0045_tikal_action_3,
                          &tikal_texlist, 0.1F, 0, 8);
        event_voice_play(1264);
        event_message_wait(0, TEXT_9);
        event_wait(140);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -118.87F, 72.92F, 494.91F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xea00, 0xbc11, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, -120.51F, 72.92F, 511.83F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0xea00, 0xbc11, 0);
        event_voice_play(1265);
        event_message_wait(0, TEXT_10);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_20);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 22.13F, 4.19F, 470.12F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x7011, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 20.28F, 4.19F, 469.36F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0200, 0x7011, 0);
        event_voice_play(1266);
        event_message_wait(0, TEXT_11);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_21);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 16.79F, 6.77F, 488.61F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff00, 0x014a, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_22);
        event_voice_play(1267);
        event_message_wait(0, TEXT_12);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.29F, 5.9F, 528.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x8411, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, -28.19F, 5.98F, 529.697F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0x0300, 0x8411, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_23);
        event_voice_play(1268);
        event_message_wait(0, TEXT_13);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -27.46F, 5.98F, 553.15F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0xf611, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -21.56F, 5.98F, 554.16F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x0200, 0xfe11, 0);
        event_voice_play(1269);
        event_message_wait(0, TEXT_14);
        event_wait(40);
        event_voice_play(1270);
        event_message_wait(0, TEXT_15);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 13.88F, 5.58F, 487.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0xee11, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_24);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0045_tikal_action_0,
                          &tikal_texlist, 0.07F, 0, 32);
        event_wait(60);
        event_message_close();
        event_white_start_hold(60, 30, 0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_wait(90);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        /* the event_sound_stop_all two lines up freed the slots, so this one
         * does nothing */
        event_sound_stop_all();
        event_kill_player(2);
        event_white_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0045_textures[] = {
    {"TIKAL", &tikal_texlist},
    {NULL, NULL},
};
