/* EV008F, Knuckles' story: in the past, Tikal warns Chaos that her father and
 * his people are coming. Chaos can't leave. Tikal resolves to talk to her
 * father again */
#include <ninja.h>

#include "ev008f/chao_action.h"
#include "ev008f/script.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "shared/sound.h"
#include "shared/wave.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 39
#define TEXT_3_LENGTH 26
#define TEXT_4_LENGTH 37
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 49
#define TEXT_7_LENGTH 50
#define TEXT_8_LENGTH 40
#define TEXT_9_LENGTH 51
#define TEXT_10_LENGTH 31
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 1
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 2
#define TEXT_16_LENGTH 3
#define TEXT_17_LENGTH 5
#define TEXT_18_LENGTH 2
#define TEXT_19_LENGTH 3
#define TEXT_20_LENGTH 6
#include "ev008f/script/text.inc"

/* in TIKAL_PROG.PRS, which loads beside the past's module */
extern NJS_ACTION tikal_walk_action;

extern NJS_ACTION ev008f_tikal_action_0;
extern NJS_ACTION ev008f_tikal_action_1;
extern NJS_ACTION ev008f_tikal_action_2;
extern NJS_ACTION ev008f_tikal_action_3;

static struct task *ev008f_chao_0 = NULL;
static struct task *ev008f_chao_1 = NULL;
static struct task *ev008f_chao_2 = NULL;
static struct task *ev008f_chao_3 = NULL;
static struct task *ev008f_chao_4 = NULL;

void ev008f_script(int phase)
{
    /* @hack spelling: event_camera_set_ang called through a local. SHC then
     * keeps its address in R12, event_camera_set_pos's in R11 and
     * event_play_action's in R8 (0x0cb8010c, 0x0cb800f6, 0x0cb8014e). called by
     * name, the two camera calls trade registers, and R8 holds
     * event_voice_play's address instead. most of the code from 0x0cb8011c on
     * then moves, and 995 of 1192 words differ */
    void (*camera_set_ang)(enum event_ease ease, int frames, Angle ang_x,
                           Angle ang_y, Angle ang_z);

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
        event_create_player(2, tikal_player_step, 0.4F, 90.0F, 48.03F, 0,
                            0x71be, 0);
        event_wait(1);
        /* @bug logic: clears player 1's motions, not those of Tikal, who is
         * player 2 and was created above (MOV #1,R4 at 0x0cb8009c, for the
         * event_get_player whose task the JSR at 0x0cb800a0 clears).
         * event_reset_player(2) clears hers next (JSR at 0x0cb800a4) */
        event_clear_motions(event_get_player(1));
        event_reset_player(2);
        event_set_pos(event_get_player(0), 10.49F, 90.0F, 91.11F);
        event_set_ang(event_get_player(0), 0, 0x8a83, 0);
        event_wait(1);
        event_set_pos(event_get_player(0), 10.49F, 90.0F, 91.11F);
        event_set_ang(event_get_player(0), 0, 0x8a83, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 11.34F, 96.0F, 79.64F);
        camera_set_ang = event_camera_set_ang;
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x75dc, 0);
        event_wait(10);
        event_voice_play(1602);
        event_message_wait(1, TEXT_1);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_11);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev008f_tikal_action_0,
                          &tikal_texlist, 0.07F, 0, 8);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 14.07F, 94.54F, 32.38F);
        camera_set_ang(EVENT_EASE_SMOOTH, 20, 0xfc00, 0x6fdc, 0);
        event_wait(50);
        event_voice_play(1603);
        event_message_wait(1, TEXT_2);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -0.48F, 96.85F, 39.83F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x85dc, 0);
        event_wait(60);
        event_voice_play(1604);
        event_message_wait(1, TEXT_3);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -200.11F, 131.67F, 41.61F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd00, 0xc7dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -184.45F, 130.5F, 38.55F);
        camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xfd00, 0xc7dc, 0);
        event_voice_play(1605);
        event_message_wait(1, TEXT_4);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 11.21F, 104.26F, 39.92F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe700, 0x59dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, -3.21F, 96.85F, 42.29F);
        camera_set_ang(EVENT_EASE_SMOOTH, 20, 0, 0x97dc, 0);
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_message_wait(1, TEXT_5);
        event_voice_play(1606);
        event_wait(20);
        event_message_close();
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -75.26F, 96.92F, 63.11F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf300, 0xd2dc, 0);
        event_wait(10);
        event_sound_play(1225, 20, 0, 0);
        event_wave_emitter_create(-40.7F, 80.0F, 52.68F, 0.22F, 1.0F, 6, 18, 1);
        event_wait(30);
        event_sound_play(1225, 20, 0, 0);
        event_wave_emitter_create(-40.7F, 80.0F, 52.68F, 0.22F, 1.0F, 6, 18, 1);
        event_sound_play(1225, 20, 0, 0);
        event_wave_emitter_create(-52.01F, 80.0F, 56.42F, 0.3F, 1.0F, 6, 19, 2);
        event_wait(130);
        event_wave_emitter_kill(1);
        event_wave_emitter_kill(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.24F, 100.15F, 44.73F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x2adc, 0);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, -1.24F, 100.15F, 44.73F);
        camera_set_ang(EVENT_EASE_SMOOTH, 200, 0, 0xcbdc, 0);
        event_wait(190);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -39.03F, 95.39F, 24.88F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x200, 0xb4dc, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev008f_tikal_action_1,
                          &tikal_texlist, 0.19F, 0, 0);
        event_wait(15);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_16);
        event_voice_play(1607);
        event_message_wait(1, TEXT_6);
        event_wait(60);
        event_voice_play(1608);
        event_message_wait(1, TEXT_7);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_17);
        event_play_action(event_get_player(2), &ev008f_tikal_action_2,
                          &tikal_texlist, 0.8F, 0, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.3F, 93.14F, 41.19F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc00, 0x62dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, 4.51F, 93.97F, 43.25F);
        camera_set_ang(EVENT_EASE_SMOOTH, 300, 0xc00, 0x62dc, 0);
        event_wait(130);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_18);
        event_voice_play(1609);
        event_message_wait(1, TEXT_8);
        event_play_action(event_get_player(2), &ev008f_tikal_action_3,
                          &tikal_texlist, 1.0F, 0, 8);
        event_wait(1);
        event_wait_motions(event_get_player(2));
        event_place_task(&ev008f_chao_0, event_chao_create, 15.95F, 92.0F,
                         50.15F, 0, 0xef39, 0);
        event_place_task(&ev008f_chao_1, event_chao_create, 16.04F, 92.0F,
                         46.17F, 0, 0xef39, 0);
        event_place_task(&ev008f_chao_2, event_chao_create, 7.85F, 92.0F,
                         52.52F, 0, 0xef39, 0);
        event_place_task(&ev008f_chao_3, event_chao_create, 3.98F, 92.0F,
                         50.32F, 0, 0xef39, 0);
        event_place_task(&ev008f_chao_4, event_chao_create, 11.28F, 92.0F,
                         59.23F, 0, 0xef39, 0);
        event_chao_set_eyes(ev008f_chao_0, CHAO_EYES_SING);
        event_chao_set_eyes(ev008f_chao_1, CHAO_EYES_SING);
        event_chao_set_eyes(ev008f_chao_2, CHAO_EYES_SING);
        event_chao_set_eyes(ev008f_chao_3, CHAO_EYES_SING);
        event_chao_set_eyes(ev008f_chao_4, CHAO_EYES_SING);
        event_set_flags(ev008f_chao_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev008f_chao_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev008f_chao_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev008f_chao_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev008f_chao_4, EVENT_IGNORE_GROUND);
        event_set_shadow(ev008f_chao_0, 0.4F);
        event_set_shadow(ev008f_chao_1, 0.4F);
        event_set_shadow(ev008f_chao_2, 0.4F);
        event_set_shadow(ev008f_chao_3, 0.4F);
        event_set_shadow(ev008f_chao_4, 0.4F);
        event_play_action(ev008f_chao_0, &ev008f_chao_action,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 4);
        event_play_action(ev008f_chao_1, &ev008f_chao_action,
                          &event_chao_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_action(ev008f_chao_2, &ev008f_chao_action,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 4);
        event_play_action(ev008f_chao_3, &ev008f_chao_action,
                          &event_chao_texlist, 0.9F, EVENT_REPEAT, 4);
        event_play_action(ev008f_chao_4, &ev008f_chao_action,
                          &event_chao_texlist, 0.9F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 11.13F, 99.74F, 57.59F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xef00, 0x1ddc, 0);
        event_clear_motions(event_get_player(2));
        event_set_ang(event_get_player(2), 0, 0x84e, 0);
        event_look_angles(event_get_player(2), -0x1fff, -0x1fff, 0);
        event_wait(50);
        event_sound_play(1223, 0, 0, 0);
        event_wait(10);
        event_sound_play(1224, 0, 0, 0);
        event_wait(20);
        event_sound_play(1223, 0, 0, 0);
        event_wait(10);
        event_sound_play(1224, 0, 0, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 26.9F, 90.7F, 53.75F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd00, 0x38dc, 0);
        event_voice_play(1610);
        event_message_wait(1, TEXT_9);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_19);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -13.39F, 93.19F, 58.37F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd00, 0xcedc, 0);
        event_look_clear(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &tikal_walk_action,
                          &tikal_texlist, 0.8F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), 1.97F, 90.0F, 88.99F, 0.2F,
                           0.06F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.14F, 95.7F, 71.95F);
        camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x700, 0x5dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 4.28F, 92.93F, 87.05F);
        camera_set_ang(EVENT_EASE_SMOOTH, 200, 0xd00, 0x5dc, 0);
        event_set_ang(ev008f_chao_0, 0, 0x12b, 0);
        event_set_ang(ev008f_chao_1, 0, 0x12b, 0);
        event_set_ang(ev008f_chao_2, 0, 0x12b, 0);
        event_set_ang(ev008f_chao_3, 0, 0x12b, 0);
        event_set_ang(ev008f_chao_4, 0, 0x12b, 0);
        event_voice_play(1611);
        /* @hack redundant: a block that never runs, which keeps
         * event_talk_start's address in R9 and event_play_action's in R8
         * (0x0cb80136, 0x0cb8014e). without it, the two trade registers, and
         * 28 of 1192 words differ */
        if (0) {
            event_message_wait(0, NULL);
            event_talk_start(NULL, NULL);
        }
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_20);
        event_message_wait(0, TEXT_10);
        event_wait(90);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 6.05F, 96.27F, 84.31F);
        camera_set_ang(EVENT_EASE_SMOOTH, 20, 0x200, 0x9bdc, 0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_white_start_hold(60, 30, 0);
        event_wait(80);
        event_run_clear(event_get_player(2));
        event_wait(10);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_kill_task(&ev008f_chao_0);
        event_kill_task(&ev008f_chao_1);
        event_kill_task(&ev008f_chao_2);
        event_kill_task(&ev008f_chao_3);
        event_kill_task(&ev008f_chao_4);
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

struct texture_pvm ev008f_textures[] = {
    {"EV_ALIFE", &event_chao_texlist},
    {"TIKAL", &tikal_texlist},
    {"WAVE7_WA", &event_wave_texlist},
    {NULL, NULL},
};
