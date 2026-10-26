/* EV008A, Knuckles' story: Knuckles finds Sonic and Tails with an emerald in
 * Mystic Ruins. he knocks Sonic back with a punch and demands the emerald */
#include <ninja.h>

#include "ev008a/emerald_action.h"
#include "ev008a/emerald_textures.h"
#include "ev008a/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/shapes.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 17
#define TEXT_2_LENGTH 19
#define TEXT_3_LENGTH 23
#define TEXT_4_LENGTH 47
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 43
#define TEXT_9_LENGTH 24
#define TEXT_10_LENGTH 4
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 2
#define TEXT_13_LENGTH 3
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 4
#include "ev008a/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_run_action;

extern NJS_ACTION ev008a_sonic_action_0;
extern NJS_ACTION ev008a_sonic_action_1;
extern NJS_ACTION ev008a_sonic_action_2;
extern NJS_ACTION ev008a_sonic_action_3;
extern NJS_ACTION ev008a_sonic_action_4;
extern NJS_ACTION ev008a_sonic_action_5;
extern NJS_ACTION ev008a_knuckles_action_0;
extern NJS_ACTION ev008a_knuckles_action_1;
extern NJS_ACTION ev008a_knuckles_action_2;

/* player 0's pad as Knuckles punches Sonic, a key a step. B is held for 8, 7
 * and 23 steps. it is let go for 4 steps after the first hold and 7 after the
 * second */
static const struct event_pad_key ev008a_punch_keys[49] = {
#include "ev008a/script/ev008a_punch_keys.inc"
};

static const struct event_pad_recording ev008a_punch = {
    ev008a_punch_keys,
    sizeof(ev008a_punch_keys) / sizeof(ev008a_punch_keys[0])};

/* the green Chaos Emerald, placed above Sonic and killed before the punch */
static struct task *ev008a_emerald = NULL;

void ev008a_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_set_pos(event_get_player(0), 193.56F, 70.98F, 356.53F);
        event_set_ang(event_get_player(0), 0x1190, 0x7f2d, 0xfbe0);
        event_create_player(2, sonic_step, 55.93F, 73.16F, 289.96F, 0x00a0,
                            0xeee9, 0xffe4);
        event_create_player(3, tails_step, 50.76F, 73.23F, 305.75F, 0xff6e,
                            0x600c, 0xffa2);
        event_place_actor(&ev008a_emerald, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          &knuckles_texlist, 2.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev008a_sonic_action_3,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(ev008a_emerald, EVENT_IGNORE_GROUND);
        event_set_pos(ev008a_emerald, event_get_player(2)->work->pos.x - 1.2F,
                      event_get_player(2)->work->pos.y + 3.6F,
                      event_get_player(2)->work->pos.z + 2.0F);
        event_set_ang(ev008a_emerald, 0, 0, 0);
        event_wait(1);
        event_play_action(ev008a_emerald, &ev008a_emerald_action,
                          &ev008a_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        stream_request_music(MUSIC_THEKNKLS);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 187.57F, 78.4F, 291.57F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 186.21F, 78.51F, 293.03F);
        event_run_to_speed(event_get_player(0), 178.87F, 72.65F, 302.99F, 1.75F,
                           0.06F);
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0x00a0, 0x80e5, 0xffe4);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x023c, 0x6166, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 186.21F, 78.51F, 293.03F);
        event_wait(15);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 205.03F, 78.4F, 286.85F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x023c, 0x4a66, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 210.83F, 78.07F, 285.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x023c, 0x4a66, 0);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 90.87F, 76.46F, 293.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x033c, 0x4466, 0);
        event_voice_play(1591);
        event_message_show(TEXT_1);
        event_wait(20);
        event_message_close();
        event_talk_start(event_get_player(0), TEXT_10);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 182.35F, 73.19F, 288.24F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x113c, 0x7366, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 180.68F, 75.65F, 293.46F);
        event_voice_play(1592);
        event_message_show(TEXT_2);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 72.79F, 78.37F, 302.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe3c, 0x2966, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, 65.15F, 77.99F, 298.02F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0xfe3c, 0x2966, 0);
        event_message_show(TEXT_3);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 63.45F, 77.9F, 296.97F);
        event_wait(30);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 183.89F, 74.49F, 292.14F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x093c, 0x6866, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, 196.67F, 78.19F, 292.07F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 110, 0x013c, 0x4e66, 0);
        event_voice_play(1593);
        event_talk_start(event_get_player(0), TEXT_11);
        event_message_show(TEXT_4);
        event_wait(105);
        event_talk_stop(event_get_player(0));
        event_wait(5);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 88.95F, 76.09F, 277.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x033c, 0x5566, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 77.73F, 77.12F, 284.07F);
        event_set_pos(event_get_player(0), 87.87F, 72.71F, 252.82F);
        event_set_ang(event_get_player(0), 0xff6e, 0xdef7, 0xffa2);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(3), 0xff96, 0x8000, 18);
        event_kill_task(&ev008a_emerald);
        event_wait(20);
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_wait(15);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 56.62F, 77.45F, 307.81F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 50, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0);
        event_wait(20);
        event_play_action(event_get_player(2), &ev008a_sonic_action_5,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(30);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x013c, 0xf566, 0);
        event_wait(10);
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 51.48F, 76.84F, 327.13F);
        event_wait(30);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x058c, 0xf58a, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 83.98F, 78.0F, 261.87F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x2aab);
        event_voice_play(1594);
        event_message_show(TEXT_5);
        event_wait(10);
        event_set_pos(event_get_player(2), 55.93F, 73.16F, 289.96F);
        event_set_ang(event_get_player(2), 0xff6e, 0x600c, 0xffa2);
        event_wait(10);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_set_pos(event_get_player(3), 55.76F, 73.23F, 320.75F);
        event_set_ang(event_get_player(3), 0xff6e, 0x600c, 0xffa2);
        event_wait(10);
        event_message_close();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 57.37F, 77.57F, 284.06F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        /* Knuckles' punch knocks Sonic back */
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev008a_punch);
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x071c);
        event_wait(10);
        event_play_action(event_get_player(2), &ev008a_sonic_action_4,
                          &sonic_texlist, 1.0F, 0, 0);
        event_wait(7);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 56.41F, 77.77F, 311.87F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0xf800);
        event_move(event_get_player(2), event_get_player(2)->work->pos.x,
                   event_get_player(2)->work->pos.y,
                   event_get_player(2)->work->pos.z,
                   event_get_player(2)->work->pos.x - 25.0F,
                   event_get_player(2)->work->pos.y,
                   event_get_player(2)->work->pos.z - 25.0F, 35);
        event_wait(40);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 38.54F, 76.28F, 321.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0412, 0xf8f8, 0xf800);
        event_wait(10);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_play_action(event_get_player(2), &ev008a_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 0);
        event_set_ang(event_get_player(2), 0xff6e, 0x0cf2, 0xffa2);
        event_set_pos(event_get_player(2), 30.93F, 70.0F, 264.96F);
        event_set_pos(event_get_player(3),
                      event_get_player(3)->work->pos.x - 20.0F,
                      event_get_player(3)->work->pos.y,
                      event_get_player(3)->work->pos.z + 20.0F);
        event_wait_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev008a_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 28.93F, 77.7F, 285.53F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0412, 0xf8f8, 0xf800);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 45, 30.12F, 78.4F, 278.67F);
        event_set_ang(event_get_player(0), 0xff6e, 0x89c2, 0xffa2);
        event_set_pos(event_get_player(0), 42.96F, 73.43F, 301.29F);
        event_play_action(event_get_player(0), &ev008a_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_message_show(TEXT_6);
        event_voice_play(1595);
        event_wait(25);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(20);
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.38F, 92.46F, 247.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf4f6, 0xa7ec, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -14.98F, 95.67F, 240.62F);
        event_play_action(event_get_player(2), &ev008a_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 16);
        event_talk_start(event_get_player(0), TEXT_13);
        event_wait(5);
        event_voice_play(1596);
        event_message_show(TEXT_7);
        event_wait(35);
        event_talk_stop(event_get_player(0));
        event_voice_play(1597);
        event_play_action(event_get_player(0), &ev008a_knuckles_action_1,
                          &knuckles_texlist, 1.0F, 0, 2);
        event_play_action(event_get_player(0), &ev008a_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_message_show(TEXT_8);
        event_talk_start(event_get_player(0), TEXT_14);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 40.04F, 78.59F, 319.81F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x00f6, 0xfdec, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 34.05F, 78.59F, 319.51F);
        event_voice_play(1598);
        event_message_show(TEXT_9);
        event_talk_start(event_get_player(2), TEXT_15);
        event_wait(40);
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_wait(80);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(3);
        event_kill_task(&ev008a_emerald);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev008a_textures[] = {
    {"SONIC", &sonic_texlist},
    {"MILES", &tails_texlist},
    {"M_EM_GREEN", &ev008a_emerald_texlist},
    {NULL, NULL},
};
