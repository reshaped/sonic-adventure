/* EV0040, Tails' story: Tails wakes from a dream of Sonic. he resolves to find
 * a Chaos Emerald for his new plane */
#include <ninja.h>

#include "ev0040/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/textures.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 37
#define TEXT_2_LENGTH 55
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 30
#define TEXT_5_LENGTH 51
#define TEXT_6_LENGTH 60
#define TEXT_7_LENGTH 36
#define TEXT_8_LENGTH 31
#define TEXT_9_LENGTH 29
#define TEXT_10_LENGTH 3
#define TEXT_11_LENGTH 2
#define TEXT_12_LENGTH 3
#define TEXT_13_LENGTH 2
#define TEXT_14_LENGTH 3
#define TEXT_15_LENGTH 5
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 5
#define TEXT_18_LENGTH 2
#include "ev0040/script/text.inc"

extern NJS_ACTION tails_stand_action;

extern NJS_ACTION ev0040_tails_action_0;
extern NJS_ACTION ev0040_tails_action_1;
extern NJS_ACTION ev0040_tails_action_2;
extern NJS_ACTION ev0040_tails_action_3;
extern NJS_ACTION ev0040_tails_action_4;
extern NJS_ACTION ev0040_tails_action_5;
extern NJS_ACTION ev0040_tails_action_6;
extern NJS_ACTION ev0040_tails_action_7;

void ev0040_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        TAILS_FLAGS(player_work[0]) |= TAILS_TAIL_STILL;
        event_set_pos(event_get_player(0), 125.92F, 72.64F, 267.9F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_talk_start(event_get_player(0), TEXT_9);
        event_play_action(event_get_player(0), &ev0040_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1f00, 0x0400, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 125.05F, 93.46F, 295.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 240, 0x1400, 0x0400, 0);
        event_wait(120);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd100, 0xf800, 0x8000);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 124.11F, 101.86F, 279.36F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 121.11F, 136.6F, 294.46F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xd100, 0xf800, 0);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 116.86F, 81.93F, 285.16F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf000, 0xeb00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 120.95F, 78.48F, 277.92F);
        event_wait(72);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_10);
        event_wait(158);
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(1242);
        event_message_show(TEXT_1);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 121.74F, 74.45F, 266.76F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1800, 0xb700, 0xfe00);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0x1900, 0xb300, 0xfe00);
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(1243);
        event_message_show(TEXT_2);
        event_wait(120);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(1244);
        event_message_show(TEXT_3);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 113.59F, 75.25F, 265.08F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0xb700, 0xfe00);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0e00, 0xaa00, 0xfe00);
        event_play_action(event_get_player(0), &ev0040_tails_action_1,
                          &tails_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 120.62F, 74.96F, 288.12F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xf2bc, 0);
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_STILL;
        event_set_pos(event_get_player(0), 126.23F, 72.64F, 272.26F);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0040_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &ev0040_tails_action_3,
                          &tails_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, 118.72F, 73.93F, 293.72F);
        event_play_action(event_get_player(0), &ev0040_tails_action_4,
                          &tails_texlist, 1.0F, 0, 8);
        event_wait(20);
        event_play_action(event_get_player(0), &ev0040_tails_action_5,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(1245);
        event_message_show(TEXT_4);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 133.15F, 75.05F, 299.91F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0x08bc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 500, 130.4F, 76.33F, 287.27F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_stand_action,
                          &tails_texlist, 0.5F, EVENT_REPEAT, 16);
        event_talk_start(event_get_player(0), TEXT_15);
        event_voice_play(1246);
        event_message_show(TEXT_5);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_wait(50);
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1247);
        event_message_show(TEXT_6);
        event_wait(120);
        event_talk_stop(event_get_player(0));
        event_wait(100);
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1248);
        event_message_show(TEXT_7);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0040_tails_action_6,
                          &tails_texlist, 1.0F, 0, 10);
        event_play_action(event_get_player(0), &ev0040_tails_action_7,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1249);
        event_message_show(TEXT_8);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_wait(72);
        event_clear_motions(event_get_player(0));
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), 126.23F, 72.64F, 272.26F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_reset_player(0);
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_STILL;
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0040_textures[] = {{NULL, NULL}};
