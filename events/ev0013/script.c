/* EV0013, Sonic's story: Eggman's robot Zero appears, and Sonic means to beat
 * it. Amy sees instead that couples get into Twinkle Park free. she runs in,
 * and Sonic follows her */
#include <ninja.h>

#include "adv00/twinkle_elevator.h"
#include "enemy/zero.h"
#include "ev0013/amy_action.h"
#include "ev0013/bird_action.h"
#include "ev0013/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 32
#define TEXT_2_LENGTH 51
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 15
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 69
#define TEXT_7_LENGTH 32
#define TEXT_8_LENGTH 26
#define TEXT_9_LENGTH 25
#define TEXT_10_LENGTH 3
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 1
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 3
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 3
#include "ev0013/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_walk_action;

extern NJS_ACTION ev0013_sonic_action_0;
extern NJS_ACTION ev0013_sonic_action_1;
extern NJS_ACTION ev0013_sonic_action_2;
extern NJS_ACTION ev0013_sonic_action_3;
extern NJS_ACTION ev0013_sonic_action_4;
extern NJS_ACTION ev0013_sonic_action_5;

/* player 0's pad as Sonic follows Amy, a key a step. no key holds a button.
 * from the 6th key on, the stick is pushed at least 228 out of 255. between
 * keys 94 and 117, its direction turns from 60 to 0, in 256ths of a turn */
static const struct event_pad_key ev0013_run_keys[176] = {
#include "ev0013/script/ev0013_run_keys.inc"
};

static const struct event_pad_recording ev0013_run = {
    ev0013_run_keys, sizeof(ev0013_run_keys) / sizeof(ev0013_run_keys[0])};

static struct task *ev0013_bird = NULL;
static struct task *ev0013_fade = NULL;

void ev0013_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_000D.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_flags[FLAG_ELEVATOR] = ELEVATOR_TO_BOTTOM;
        event_set_pos(event_get_player(0), 333.5F, 0.02F, 1550.8F);
        event_set_ang(event_get_player(0), 0, 0xf900, 0);
        event_create_player(2, amy_step, 343.5F, 0.01F, 1544.8F, 0, 0xfa00, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 343.5F, 0.01F, 1544.8F);
        event_set_ang(event_get_player(2), 0, 0xfa00, 0);
        event_play_action(event_get_player(2), &ev0013_amy_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 1);
        event_play_action(event_get_player(0), &sonic_walk_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 1);
        event_wait(1);
        /* @bug logic: gives the bird angles from Amy's position, here and
         * where the script places it again below. the 0xc000 the script takes
         * from her y is an angle offset. each coordinate, offset or not, is
         * truncated to an Angle (FTRC at 0x0cb80120 to 0x0cb80146 and
         * 0x0cb8063e to 0x0cb8065e, before the JSRs at 0x0cb80188 and
         * 0x0cb80692) */
        event_place_actor(&ev0013_bird, event_get_player(2)->work->pos.x + 5.0F,
                          event_get_player(2)->work->pos.y + 12.5F,
                          event_get_player(2)->work->pos.z + 5.5F,
                          event_get_player(2)->work->pos.x,
                          event_get_player(2)->work->pos.y - 0xc000,
                          event_get_player(2)->work->pos.z);
        event_set_flags(ev0013_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev0013_bird, &ev0013_bird_action,
                          &ev0013_bird_texlist, 1.0F, EVENT_REPEAT, 1);
        event_follow(ev0013_bird, 2.5F, 5.0F, 2.0F, 900, event_get_player(2));
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 221.67F, 98.65F, 1581.23F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xed00, 0xc000, 0);
        event_run_to_speed(event_get_player(0), 333.5F, 0.02F, 1570.8F, 0.2F,
                           0.2F);
        event_run_to_speed(event_get_player(2), 343.5F, 0.01F, 1564.8F, 0.2F,
                           0.2F);
        event_create_player(7, zero_event_init, 334.93F, 12.0F, 1204.71F, 0,
                            0xf300, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 182.04F, 102.99F,
                             1606.22F);
        stream_request_music(MUSIC_EGGROBO);
        event_wait(30);
        event_run_to_speed(event_get_player(7), 331.97F, 12.0F, 1473.61F, 0.5F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 336.47F, 12.25F, 1583.45F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf000, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 336.9F, 24.53F, 1605.22F);
        event_clear_motions(event_get_player(0));
        event_clear_motions(event_get_player(2));
        event_wait(20);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 5.0F,
                           0.0F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 336.5F, 5.59F, 1561.63F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0, 0);
        event_look_clear(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 336.5F, 5.59F, 1450.63F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 336.31F, 10.02F, 1330.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_10);
        event_voice_play(1404);
        event_message_show(TEXT_1);
        event_wait(30);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_set_ang(ev0013_bird, 0, -0x4000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 335.63F, 9.07F, 1591.34F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0x0300, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0013_sonic_action_2,
                          &sonic_texlist, 0.2F, 0, 0);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 345.94F, 4.82F, 1563.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0128, 0x51dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 343.5F, 4.91F, 1564.77F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_set_pos(event_get_player(2), 348.61F, 0.0F, 1598.77F);
        event_set_ang(event_get_player(2), 0, 0x5100, 0);
        event_look_clear(event_get_player(2));
        event_look_at(event_get_player(2), 363.1F, 22.8F, 1612.2F);
        event_place_actor(
            &ev0013_bird, event_get_player(2)->work->pos.x,
            event_get_player(2)->work->pos.y, event_get_player(2)->work->pos.z,
            event_get_player(2)->work->pos.x, event_get_player(2)->work->pos.y,
            event_get_player(2)->work->pos.z);
        event_set_flags(ev0013_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev0013_bird, &ev0013_bird_action,
                          &ev0013_bird_texlist, 1.0F, EVENT_REPEAT, 1);
        event_move_stop_all();
        event_wait(1);
        event_follow(ev0013_bird, 2.5F, 5.0F, 2.0F, 900, event_get_player(2));
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0013_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev0013_sonic_action_3,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(1046);
        event_message_show(TEXT_2);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_set_pos(event_get_player(7), 334.93F, 0.0F, 1204.71F);
        event_wait(29);
        stream_stop_music();
        event_sound_play(1232, 0, 0, 0);
        event_wait(21);
        stream_request_music(MUSIC_THEAMY);
        event_look_at(event_get_player(2), 366.58F, 15.0F, 1587.15F);
        event_voice_play(1405);
        event_message_show(TEXT_3);
        event_wait(30);
        event_play_action(event_get_player(0), &ev0013_sonic_action_1,
                          &sonic_texlist, 0.25F, 0, 16);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(1047);
        event_message_show(TEXT_4);
        event_wait(60);
        event_message_clear();
        event_set_pos(event_get_player(7), 334.93F, 0.0F, 1204.71F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 335.82F, 5.768F, 1577.486F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0728, 0xa8c8, 0);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 302.37F, 5.9F, 1569.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0x0b28, 0xb1c8, 0);
        event_play_action(event_get_player(0), &ev0013_sonic_action_5,
                          &sonic_texlist, 1.0F, 0, 8);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x3000, 0);
        event_look_at(event_get_player(0), 366.58F, 15.0F, 1587.15F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_sound_play(1231, 0, 0, 0);
        event_voice_play(1048);
        event_message_show(TEXT_5);
        event_wait(1);
        event_message_close();
        event_wait(40);
        event_move_stop_all();
        event_wait(2);
        event_follow(ev0013_bird, 2.5F, 5.0F, 2.0F, 900, event_get_player(2));
        event_wait(1);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1406);
        event_message_show(TEXT_6);
        event_wait(100);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 316.17F, 4.59F, 1604.07F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0xff28, 0xdbdc, 0);
        event_look_clear(event_get_player(2));
        event_wait(1);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 5.0F,
                           0.0F);
        event_set_pos(event_get_player(7), 334.93F, 0.0F, 1204.71F);
        event_wait(1);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1407);
        event_message_show(TEXT_7);
        event_wait(90);
        event_look_clear(event_get_player(2));
        event_run_to_speed(event_get_player(2), 390.53F, 0.0F, 1610.5F, 0.0F,
                           0.0F);
        event_wait(30);
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_15);
        event_run_to(event_get_player(0), 339.87F, 0.01F, 1573.15F);
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &ev0013_sonic_action_5,
                          &sonic_texlist, 0.25F, 0, 16);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, 0, 16);
        event_voice_play(1049);
        event_message_show(TEXT_8);
        event_wait(60);
        event_wait_motions(event_get_player(0));
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_run_wait(event_get_player(2));
        event_wait(20);
        event_move_stop_all();
        event_kill_task(&ev0013_bird);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 363.14F, 4.03F, 1575.02F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_wait(20);
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1050);
        event_message_show(TEXT_9);
        event_wait(10);
        event_play_action(event_get_player(0), &ev0013_sonic_action_4,
                          &sonic_texlist, 0.25F, 0, 16);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_kill_player(2);
        event_message_close();
        event_run_to_speed(event_get_player(0), 393.9F, 2.0F, 1619.3F, 2.0F,
                           2.0F);
        event_run_wait(event_get_player(0));
        event_camera_look_clear();
        event_wait(1);
        event_wait(60);
        event_set_pos(event_get_player(0), 400.22F, 50.0F, 1649.67F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 394.61F, 60.75F, 1600.67F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 402.0F, 60.5F, 1710.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 459.19F, 174.42F, 1611.12F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xda00, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0013_run);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 456.0F, 66.75F, 1773.47F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 553.1F, 70.5F, 1771.27F);
        event_wait(70);
        ev0013_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(7);
        event_kill_task(&ev0013_bird);
        EVENT_KILL(ev0013_fade);
        /* @hack spelling: the store through the array's address. SHC then keeps
         * event_clear_motions' address in R8 for its 9 calls (MOV.L at
         * 0x0cb8034a). as event_flags[FLAG_ELEVATOR], R8 holds
         * event_set_ang's address instead, and 1233 of ev0013_script's 1380
         * words differ */
        (*&event_flags)[FLAG_ELEVATOR] = ELEVATOR_STAY;
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0013_textures[] = {
    {"AMY", &amy_texlist},
    {"VER2_WING", &ev0013_bird_texlist},
    {"EGGROB", &zero_texlist},
    {NULL, NULL},
};
