/* EV0046, Tails' story: Big chases Froggy past Tails, and Tails picks up the
 * red emerald */
#include <ninja.h>

#include "ev0046/big_action_2.h"
#include "ev0046/emerald_action_4.h"
#include "ev0046/emerald_textures.h"
#include "ev0046/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 22
#define TEXT_4_LENGTH 17
#define TEXT_5_LENGTH 38
#define TEXT_6_LENGTH 56
#define TEXT_7_LENGTH 1
#define TEXT_8_LENGTH 1
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 6
#include "ev0046/script/text.inc"

extern NJS_ACTION ev0046_tails_action_0;
extern NJS_ACTION ev0046_tails_action_1;
extern NJS_ACTION ev0046_tails_action_2;
extern NJS_ACTION ev0046_tails_action_3;
extern NJS_ACTION ev0046_tails_action_4;
extern NJS_ACTION ev0046_tails_action_5;
extern NJS_ACTION ev0046_tails_action_6;
extern NJS_ACTION ev0046_tails_action_7;
extern NJS_ACTION ev0046_big_action_0;
extern NJS_ACTION ev0046_big_action_1;
extern NJS_ACTION ev0046_frog_action_0;
extern NJS_ACTION ev0046_frog_action_1;
extern NJS_ACTION ev0046_frog_action_2;
extern NJS_ACTION ev0046_frog_action_3;
extern NJS_ACTION ev0046_emerald_action_0;
extern NJS_ACTION ev0046_emerald_action_1;
extern NJS_ACTION ev0046_emerald_action_2;
extern NJS_ACTION ev0046_emerald_action_3;

static struct task *ev0046_white = NULL;
static struct task *ev0046_emerald = NULL;
static struct task *ev0046_frog = NULL;

void ev0046_script(int phase)
{
    /* @hack spelling: event_voice_wait called through voice_wait, a local set
     * just before its first call. SHC then keeps its address in R11 from
     * 0x0cb8064c, and event_clear_motions's in R8 from 0x0cb802ec. called as
     * event_voice_wait, R8 holds event_camera_set_pos's address, the code
     * runs 12 bytes short, and 1141 of 1374 words differ */
    void (*voice_wait)(void);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_0046.mlt");
        event_sound_init(8);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_start(3, SE_TIKAL_CHIME, 90);
        ev0046_white = event_fade_create(1.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_place_actor(&ev0046_emerald, 64.11F, -16.658F, 36.233F, 0, 0, 0);
        event_set_flags(ev0046_emerald, EVENT_IGNORE_GROUND);
        event_create_player(4, big_step, -11.62F, 71.56F, 395.14F, 0x0553,
                            0x775f, 0xf992);
        event_set_pos(event_get_player(0), 29.08F, 72.02F, 177.29F);
        event_set_ang(event_get_player(0), 0, 0x0400, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &ev0046_tails_action_5,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(4), &ev0046_big_action_2,
                          &big_texlist, 1.0F, EVENT_REPEAT, 16);
        event_place_actor(&ev0046_frog, 29.08F, 72.02F, 177.29F, 0, 0x3205, 0);
        event_wait(1);
        event_set_flags(ev0046_frog, EVENT_IGNORE_GROUND);
        event_play_action(ev0046_frog, &ev0046_frog_action_1,
                          &ev0046_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 33.8F, 80.647F, 197.53F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf700, 0x0900, 0);
        event_wait(26);
        if (ev0046_white != NULL) {
            event_fade_set_speed(ev0046_white, -1.0F / 60.0F);
        }
        event_wait(60);
        event_sound_stop(2);
        stream_request_music(MUSIC_THEBIG);
        event_wait(30);
        event_look_at(event_get_player(0), 33.8F, 80.647F, 197.53F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 32.51F, 80.28F, 156.76F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0x7700, 0x0200);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 400, 31.236F, 83.24F, 168.305F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0xf800, 0x7700, 0);
        event_look_clear(event_get_player(0));
        event_wait(60);
        event_sound_start(0, 1223, 1800);
        event_sound_fade_volume(0, -50, 1);
        event_wait(1);
        event_sound_fade_volume(0, 0, 160);
        event_clear_motions(event_get_player(4));
        event_run_to_speed(event_get_player(4), 23.61F, 72.86F, 202.42F, 0.9F,
                           1.0F);
        event_wait(30);
        vibration_start(0, 3, 20, 12);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 12, 0xf900, 0x7700, 0);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 12, 0xfd00, 0x7700, 0);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 12, 0xfa00, 0x7700, 0);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 10, 0xfd00, 0x7700, 0);
        event_wait(8);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 10, 0xfa00, 0x7700, 0x0100);
        event_wait(8);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfe00, 0x7700, 0x0200);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xf900, 0x7700, 0x0100);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfd00, 0x7700, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfa00, 0x7700, 0x0100);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfe00, 0x7700, 0x0200);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xf900, 0x7700, 0x0100);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfd00, 0x7700, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfa00, 0x7700, 0x0100);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfe00, 0x7700, 0x0200);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xf900, 0x7700, 0x0100);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfd00, 0x7700, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xfa00, 0x7700, 0x0100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xfe00, 0x7700, 0x0200);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xf900, 0x7700, 0x0100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xfd00, 0x7700, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xf900, 0x7700, 0x0100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xfd00, 0x7700, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xf900, 0x7700, 0x0100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xfd00, 0x7700, 0);
        event_wait(3);
        event_sound_fade_volume(0, -128, 1);
        event_wait(1);
        event_play_action(event_get_player(4), &ev0046_big_action_0,
                          &big_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(4), &ev0046_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1823);
        event_message_show(TEXT_1);
        event_wait(20);
        event_message_close();
        event_sound_play(1224, 0, 0, 0);
        event_follow(ev0046_frog, 0.0F, 0.0F, 0.0F, 100, event_get_player(0));
        event_move(event_get_player(0), 29.08F, 72.02F, 177.29F, 29.08F, 78.02F,
                   177.29F, 25);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xfb00, 0x7700, 0x0200);
        event_voice_play(1271);
        event_talk_start(event_get_player(0), TEXT_7);
        event_message_show(TEXT_2);
        event_wait(1);
        voice_wait = event_voice_wait;
        voice_wait();
        event_wait(1);
        event_clear_motions(ev0046_frog);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.29F, 85.65F, 210.39F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf500, 0xd000, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0xf500, 0xd300, 0);
        event_play_action(event_get_player(0), &ev0046_tails_action_7,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(ev0046_frog, &ev0046_frog_action_3,
                          &ev0046_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(0), 29.08F, 72.02F, 177.29F, 29.08F, 72.02F,
                   167.69F, 60);
        event_wait(33);
        event_message_close();
        event_wait(27);
        event_clear_motions(ev0046_frog);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0046_tails_action_5,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0046_frog, &ev0046_frog_action_1,
                          &ev0046_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 18.16F, 78.77F, 157.46F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x9400, 0);
        event_wait(58);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0046_tails_action_6,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(1);
        event_sound_play(1226, 0, 0, 0);
        event_clear_motions(ev0046_frog);
        event_move(ev0046_frog, 28.58F, 72.02F, 172.29F, 27.88F, 74.04F,
                   177.33F, 30);
        event_play_action(ev0046_frog, &ev0046_frog_action_0,
                          &ev0046_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(ev0046_frog, 0, 0x0200, 0);
        event_wait(32);
        event_move(ev0046_frog, 27.88F, 74.04F, 177.33F, 24.88F, 84.74F,
                   196.33F, 24);
        event_wait(26);
        event_sound_play(1227, 0, 0, 0);
        event_move(event_get_player(4), 23.47F, 72.02F, 203.18F, 23.47F, 74.02F,
                   203.18F, 1);
        event_wait(2);
        event_move(ev0046_frog, 24.88F, 84.74F, 196.33F, 12.45F, 74.48F,
                   243.92F, 48);
        event_wait(49);
        event_set_flags(ev0046_frog, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_clear_motions(ev0046_frog);
        event_play_action(ev0046_frog, &ev0046_frog_action_2,
                          &ev0046_frog_texlist, 2.8F, EVENT_REPEAT, 0);
        event_wait(2);
        event_sound_start(1, 1225, 1800);
        event_sound_fade_volume(1, -30, 100);
        event_move(ev0046_frog, 12.45F, 74.48F, 243.92F, 40.39F, 73.3F, 541.19F,
                   320);
        event_wait(20);
        event_clear_motions(event_get_player(4));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 16.9F, 115.94F, 234.52F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xed3e, 0x81de, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 17.445F, 123.93F, 202.76F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xe23e, 0x81de, 0);
        event_wait(20);
        event_sound_fade_volume(0, 0, 1);
        event_wait(1);
        event_sound_fade_volume(0, -50, 100);
        event_run_to_speed(event_get_player(4), 40.39F, 73.3F, 541.19F, 0.8F,
                           0.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0046_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0046_emerald, &ev0046_emerald_action_0,
                          &ev0046_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_voice_play(1815);
        event_message_show(TEXT_3);
        event_wait(100);
        event_message_close();
        EVENT_KILL(ev0046_white);
        event_move_stop(ev0046_frog);
        event_kill_task(&ev0046_frog);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 30.71F, 77.62F, 175.51F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0x0800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 31.38F, 75.567F, 178.875F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x1600, 0x0900, 0);
        event_sound_stop(0);
        event_sound_stop(1);
        event_talk_start(event_get_player(0), TEXT_8);
        event_voice_play(1272);
        event_message_show(TEXT_4);
        event_wait(1);
        voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_talk_start(event_get_player(0), TEXT_9);
        event_voice_play(1273);
        event_message_show(TEXT_5);
        event_set_pos(ev0046_emerald, 25.98F, 72.32F, 168.49F);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(0), &ev0046_tails_action_1,
                          &tails_texlist, 1.0F, 0, 8);
        event_wait(1);
        event_play_action(ev0046_emerald, &ev0046_emerald_action_1,
                          &ev0046_emerald_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev0046_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(ev0046_emerald, &ev0046_emerald_action_2,
                          &ev0046_emerald_texlist, 1.0F, EVENT_REPEAT, 8);
        voice_wait();
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 32.14F, 76.1F, 182.25F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0400, 0x0900, 0);
        event_wait(40);
        event_play_action(event_get_player(0), &ev0046_tails_action_3,
                          &tails_texlist, 1.0F, 0, 0);
        event_play_action(ev0046_emerald, &ev0046_emerald_action_3,
                          &ev0046_emerald_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev0046_tails_action_4,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0046_emerald, &ev0046_emerald_action_4,
                          &ev0046_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_camera_orbit(EVENT_EASE_SMOOTH, 200, event_get_player(0), 10.0F,
                           0, 0, 0, 0, 0x8000, 0, 10.0F, 20.0F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_talk_start(event_get_player(0), TEXT_10);
        event_message_show(TEXT_6);
        event_voice_play(1274);
        event_wait(1);
        voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(100);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), 29.08F, 72.02F, 177.29F);
        event_set_ang(event_get_player(0), 0, 0x0400, 0);
        event_camera_look_clear();
        event_sound_stop_all();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(4);
        event_kill_task(&ev0046_frog);
        event_kill_task(&ev0046_emerald);
        vibration_start(0, 0, 0, 0);
        if (ev0046_white != NULL) {
            task_kill(ev0046_white);
            ev0046_white = NULL;
        }
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0046_textures[] = {
    {"BIG", &big_texlist},
    {"M_EM_RED", &ev0046_emerald_texlist},
    {"FROG", &ev0046_frog_texlist},
    {NULL, NULL},
};
