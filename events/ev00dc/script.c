/* EV00DC, Big's story: in the past, Tikal is let into the altar of the seven
 * Emeralds and reads its tablet, and the screen turns white. Big is player 0,
 * and Tikal is player 2 */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00dc/script.h"
#include "ev00dc/tikal_action_2.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_models.h"
#include "player/textures.h"
#include "shared/sound.h"
#include "shared/wave.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 34
#define TEXT_3_LENGTH 35
#define TEXT_4_LENGTH 27
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 31
#define TEXT_8_LENGTH 39
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 21
#define TEXT_11_LENGTH 43
#define TEXT_12_LENGTH 1
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 4
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 1
#define TEXT_17_LENGTH 2
#define TEXT_18_LENGTH 6
#include "ev00dc/script/text.inc"

/* Tikal's actions in TIKAL_PROG.PRS, which loads beside ADV03.PRS, the
 * Past's module */
extern NJS_ACTION tikal_walk_action;
extern NJS_ACTION dat_0cb2d478;

extern NJS_ACTION ev00dc_tikal_action_0;
extern NJS_ACTION ev00dc_tikal_action_1;

void ev00dc_script(int phase)
{
    /* @hack spelling: event_play_action called through a local. R10 holds its
     * address either way. SHC then puts event_message_wait's address in R8 and
     * 200 in R9 (0x0cb8011a, 0x0cb80232). called by name, event_voice_play's
     * address takes R9 and 200 takes R8, and 81 of 1144 words differ */
    void (*play)(struct task *task, NJS_ACTION *action, NJS_TEXLIST *texlist,
                 Float speed, int flags, int blend);
    /* @hack spelling: event_camera_set_ang called through a local. SHC then
     * puts its address in R12 and event_camera_set_pos's in R11 (0x0cb80104,
     * 0x0cb800ea). called by name, the two trade, and 123 of 1144 words
     * differ */
    void (*set_ang)(enum event_ease ease, int steps, Angle ang_x, Angle ang_y,
                    Angle ang_z);

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
        stream_request_music(MUSIC_TICAL);
        event_create_player(2, tikal_player_step, -2.74F, 7.51F, 254.65F, 0,
                            0x71be, 0);
        event_wait(1);
        event_reset_player(2);
        event_clear_motions(event_get_player(2));
        /* @hack spelling: the assignment and the call as one statement. SHC
         * then pools 0.1F ahead of event_play_action's address (0x0cb801ac,
         * 0x0cb801b0). as two statements, the two trade slots, and 2 of 1144
         * words differ */
        play = event_play_action,
        play(event_get_player(2), &ev00dc_tikal_action_1, &tikal_texlist, 0.1F,
             0, 8);
        event_set_pos(event_get_player(0), 2.9F, 0.01F, 329.1F);
        event_set_ang(event_get_player(0), 0, 0x8058, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.08F, 12.97F, 312.72F);
        set_ang = event_camera_set_ang;
        set_ang(EVENT_EASE_SMOOTH, 0, 0x263, 0x81dc, 0);
        event_voice_play(1852);
        event_wait(30);
        event_message_wait(1, TEXT_1);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 0.33F, 14.86F, 242.48F);
        set_ang(EVENT_EASE_SMOOTH, 20, 0x263, 0x7fdc, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 5.41F, 9.32F, 285.08F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xd63, 0x1dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 5.11F, 11.58F, 278.46F);
        set_ang(EVENT_EASE_SMOOTH, 200, 0xd63, 0x1dc, 0);
        event_voice_play(1853);
        event_message_wait(1, TEXT_2);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -69.16F, 21.98F, 202.54F);
        set_ang(EVENT_EASE_LINEAR, 0, 0xe200, 0x97dc, 0);
        event_sound_play(1225, 128, -100, 0);
        event_wave_emitter_create(-46.84F, -3.0F, 245.56F, 0.25F, 1.0F, 6, 18,
                                  1);
        event_wait(30);
        event_wave_emitter_create(-46.84F, -3.0F, 245.56F, 0.2F, 1.0F, 7, 22,
                                  1);
        event_sound_play(1225, 110, -70, 0);
        event_wave_emitter_create(-56.37F, -3.0F, 225.74F, 0.3F, 1.0F, 6, 19,
                                  3);
        event_wait(40);
        event_wave_emitter_create(-56.37F, -3.0F, 225.74F, 0.3F, 1.0F, 6, 19,
                                  3);
        event_message_close();
        event_wait(100);
        event_wave_emitter_kill(1);
        event_wave_emitter_kill(3);
        event_clear_motions(event_get_player(2));
        play(event_get_player(2), &ev00dc_tikal_action_0, &tikal_texlist, 0.1F,
             EVENT_REPEAT, 32);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.53F, 9.14F, 244.72F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xf00, 0x5fdc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 3.74F, 9.97F, 248.26F);
        set_ang(EVENT_EASE_SMOOTH, 200, 0xf00, 0x5fdc, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1854);
        event_message_wait(1, TEXT_3);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.24F, 8.16F, 289.48F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xf63, 0x1e24, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, -2.24F, 8.16F, 289.48F);
        set_ang(EVENT_EASE_SMOOTH, 200, 0xf63, 0xdc24, 0);
        event_voice_play(1855);
        event_message_wait(1, TEXT_4);
        event_wait(200);
        event_message_close();
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.99F, 14.16F, 230.02F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0x263, 0x7adc, 0);
        event_clear_motions(event_get_player(2));
        play(event_get_player(2), &tikal_walk_action, &tikal_texlist, 1.3F,
             EVENT_REPEAT, 8);
        event_run_to_speed(event_get_player(2), 0.67F, 7.5F, 231.75F, 0.5F,
                           0.06F);
        event_wait(60);
        play(event_get_player(0), &big_run_action, &big_texlist, 0.6F,
             EVENT_REPEAT, 8);
        event_wait(10);
        event_run_to_speed(event_get_player(0), -3.17F, -0.01F, 295.2F, 0.4F,
                           0.06F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 66.44F, 120.12F, 247.92F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0xadc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 23.3F, 103.95F, 89.51F);
        set_ang(EVENT_EASE_SMOOTH, 100, 0xfc00, 0xadc, 0);
        event_set_pos(event_get_player(2), 3.55F, 90.0F, 89.66F);
        event_set_ang(event_get_player(2), 0, 0x8058, 0);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_clear_motions(event_get_player(0));
        play(event_get_player(2), &tikal_walk_action, &tikal_texlist, 1.6F,
             EVENT_REPEAT, 4);
        event_wait(1);
        event_run_to_speed(event_get_player(2), -0.1F, 90.0F, 65.16F, 0.5F,
                           0.06F);
        event_wait(50);
        play(event_get_player(2), &dat_0cb2d478, &tikal_texlist, 0.3F,
             EVENT_REPEAT, 4);
        event_run_wait(event_get_player(2));
        event_voice_play(1856);
        event_message_wait(1, TEXT_5);
        play(event_get_player(2), &tikal_walk_action, &tikal_texlist, 1.4F,
             EVENT_REPEAT, 4);
        event_wait(70);
        event_message_close();
        event_run_to_speed(event_get_player(2), 0.11F, 90.0F, 43.12F, 0.3F,
                           0.06F);
        event_wait(40);
        play(event_get_player(0), &big_stand_action, &big_texlist, 1.0F,
             EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), 10.49F, 90.0F, 91.11F);
        event_set_ang(event_get_player(0), 0, 0x8a83, 0);
        event_wait(30);
        play(event_get_player(2), &ev00dc_tikal_action_1, &tikal_texlist, 0.1F,
             EVENT_REPEAT, 8);
        event_wait(20);
        event_voice_play(1857);
        event_message_wait(1, TEXT_6);
        event_wait(30);
        event_run_clear(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1.12F, 99.31F, 32.53F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0x6cdc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 2.9F, 99.31F, 33.43F);
        set_ang(EVENT_EASE_SMOOTH, 40, 0xf400, 0x6cdc, 0);
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1858);
        event_message_wait(1, TEXT_7);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, 4.68F, 99.31F, 34.34F);
        set_ang(EVENT_EASE_SMOOTH, 300, 0xf400, 0x6cdc, 0);
        event_wait(50);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1859);
        event_message_wait(1, TEXT_8);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -248.85F, 474.97F, 176.23F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xd800, 0xdcdc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -231.94F, 474.97F,
                             195.97F);
        set_ang(EVENT_EASE_SMOOTH, 150, 0xd800, 0xdcdc, 0);
        event_voice_play(1860);
        event_talk_start(event_get_player(2), TEXT_16);
        event_message_wait(1, TEXT_9);
        play(event_get_player(2), &ev00dc_tikal_action_1, &tikal_texlist, 0.1F,
             0, 8);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -105.83F, 148.95F, 27.75F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xed01, 0xc8dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -108.46F, 143.59F, 28.33F);
        set_ang(EVENT_EASE_SMOOTH, 100, 0xed01, 0xc8dc, 0);
        event_wait(1);
        event_voice_play(1861);
        event_message_wait(1, TEXT_10);
        play(event_get_player(2), &ev00dc_tikal_action_2, &tikal_texlist, 0.1F,
             0, 8);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.77F, 102.2F, 30.46F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0xf000, 0x79dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 10, 2.08F, 100.28F, 35.03F);
        set_ang(EVENT_EASE_SMOOTH, 10, 0xf000, 0x79dc, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_17);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 1.94F, 99.9F, 35.94F);
        set_ang(EVENT_EASE_SMOOTH, 200, 0xf000, 0x79dc, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 10.09F, 94.95F, 69.26F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0x909, 0x4c0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 8.5F, 98.03F, 55.7F);
        set_ang(EVENT_EASE_SMOOTH, 200, 0x909, 0x4c0, 0);
        event_voice_play(1862);
        event_message_wait(0, TEXT_11);
        event_wait(120);
        event_message_close();
        event_wait(10);
        event_talk_start(event_get_player(2), TEXT_18);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.93F, 96.97F, 30.04F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0x100, 0x7fdc, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 5.83F, 104.03F, 77.3F);
        set_ang(EVENT_EASE_SMOOTH, 30, 0xfe00, 0x89dc, 0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_wait(40);
        event_sound_fade_volume(0, -120, 120);
        event_white_start_hold(60, 30, 0);
        event_wait(90);
        event_sound_stop(0);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        event_white_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00dc_textures[] = {
    {"TIKAL", &tikal_texlist},
    {"WAVE7_WA", &event_wave_texlist},
    {NULL, NULL},
};
