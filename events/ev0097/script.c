/* EV0097, Knuckles' story: in the past, Knuckles finds Tikal grieving at the
 * altar. she runs off, and the screen goes white */
#include <ninja.h>

#include "ev0097/script.h"
#include "ev0097/tikal_action_4.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 18
#define TEXT_2_LENGTH 19
#define TEXT_3_LENGTH 30
#define TEXT_4_LENGTH 43
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 15
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 3
#define TEXT_10_LENGTH 2
#define TEXT_11_LENGTH 2
#define TEXT_12_LENGTH 3
#include "ev0097/script/text.inc"

extern NJS_ACTION ev0097_knuckles_action_0;
extern NJS_ACTION ev0097_knuckles_action_1;
extern NJS_ACTION ev0097_knuckles_action_2;
extern NJS_ACTION ev0097_knuckles_action_3;
extern NJS_ACTION ev0097_tikal_action_0;
extern NJS_ACTION ev0097_tikal_action_1;
extern NJS_ACTION ev0097_tikal_action_2;
extern NJS_ACTION ev0097_tikal_action_3;

static struct task *ev0097_fade = NULL;

void ev0097_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_00F8.mlt");
        event_sound_init(2);
        event_set_pos(event_get_player(0), -18.64F, -11.09F, 496.33F);
        event_set_ang(event_get_player(0), 0x04f0, 0x83c8, 0);
        event_create_player(2, tikal_player_step, -15.8F, -4.2F, 434.39F,
                            0x04f0, 0x4c00, 0);
        event_wait(1);
        event_talk_start(event_get_player(2), TEXT_8);
        event_play_action(event_get_player(0), &ev0097_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev0097_tikal_action_0,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -37.8866F, 28.458F, 479.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe800, 0xed00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -29.1F, 14.58F, 464.93F);
        event_move(event_get_player(0), -11.0F, -10.45F, 496.0F, -18.74F, -4.9F,
                   442.0F, 55);
        event_wait(35);
        event_play_action(event_get_player(0), &ev0097_knuckles_action_1,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -19.854F, -0.7165F, 428.12F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x9200, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -19.7556F, -2.6655F,
                             428.558F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0900, 0x9200, 0x0200);
        event_voice_play(1640);
        event_talk_start(event_get_player(0), TEXT_9);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -25.873F, -3.23F, 437.537F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a00, 0xc300, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 190, -27.44F, -2.55F, 441.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 190, 0x0400, 0xca00, 0x0200);
        event_wait(5);
        event_voice_play(1641);
        event_message_show(TEXT_2);
        event_wait(40);
        event_play_action(event_get_player(2), &ev0097_tikal_action_1,
                          &tikal_texlist, 0.49F, 0, 4);
        event_voice_wait();
        event_wait(5);
        event_talk_start(event_get_player(2), TEXT_10);
        event_message_show(TEXT_3);
        event_voice_play(1642);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -18.68F, -1.727F, 437.35F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0x9100, 0x0400);
        event_camera_set_ang(EVENT_EASE_LINEAR, 170, 0x1700, 0x7f00, 0x0400);
        event_set_ang(event_get_player(2), 0x04f0, 0x7000, 0);
        event_message_show(TEXT_4);
        event_voice_play(1643);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_wait(10);
        event_sound_play(1227, 0, 0, 0);
        event_wait(5);
        event_play_action(event_get_player(0), &ev0097_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 10, -18.55F, -1.636F, 436.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 10, 0x1200, 0x7f00, 0x0300);
        event_message_show(TEXT_5);
        event_voice_play(1644);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -19.318F, -1.4F, 428.4826F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b00, 0x9000, 0x0500);
        event_play_action(event_get_player(2), &ev0097_tikal_action_2,
                          &tikal_texlist, 0.75F, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -13.132F, -1.4F, 428.976F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x1500, 0x72b0, 0x0500);
        event_wait_motions(event_get_player(2));
        event_set_pos(event_get_player(0), -18.74F, -4.9F, 437.0F);
        event_set_pos(event_get_player(2), -14.0F, -4.2F, 429.4F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0097_tikal_action_3,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev0097_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -14.88F, -2.0F, 456.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0bf0, 0xfdcc, 0x0500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -10.0F, 28.0F, 356.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0af0, 0, 0x0300);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.0F, 55.0F, 267.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x09f0, 0, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 0.0F, 85.0F, 167.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x08f0, 0, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -15.47F, -0.45F, 424.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x05f0, 0x77cc, 0);
        event_voice_play(1645);
        event_talk_start(event_get_player(0), TEXT_11);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_play_action(event_get_player(2), &ev0097_tikal_action_4,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 4);
        event_wait(10);
        event_run_to_speed(event_get_player(2), -28.6F, -1.75F, 417.0F, 0.5F,
                           0.5F);
        event_voice_play(1646);
        event_talk_start(event_get_player(0), TEXT_12);
        event_message_show(TEXT_7);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 2.5F,
                           0.0F);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 70, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -15.86F, 0.0F, 426.63F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0750, 0x77cc, 0);
        event_wait(10);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        /* the white fade, from 0.1 to full in 63 steps */
        ev0097_fade = event_fade_create(1.0F / 70.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        event_sound_stop(0);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_reset_player(0);
        event_sound_stop(0);
        event_kill_player(2);
        event_sound_stop_all();
        EVENT_KILL(ev0097_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0097_textures[] = {
    {"TIKAL", &tikal_texlist},
    {NULL, NULL},
};
