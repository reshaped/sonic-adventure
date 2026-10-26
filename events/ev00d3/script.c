/* EV00D3, Big's story: Big finds Froggy on Tails as he sleeps, and Froggy gets
 * away again. Big is player 0, and Tails is player 3 */
#include <ninja.h>

#include "ev00d3/big_action_1.h"
#include "ev00d3/script.h"
#include "ev00e3/frog.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_models.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 31
#define TEXT_7_LENGTH 13
#include "ev00d3/script/text.inc"

extern NJS_ACTION ev00d3_tails_action_0;
extern NJS_ACTION ev00d3_tails_action_1;
extern NJS_ACTION ev00d3_tails_action_2;
extern NJS_ACTION ev00d3_big_action_0;
extern NJS_ACTION ev00d3_frog_action_0;
extern NJS_ACTION ev00d3_frog_action_1;
extern NJS_ACTION ev00d3_frog_action_2;
extern NJS_ACTION ev00d3_frog_action_3;

static struct task *ev00d3_frog = NULL;

void ev00d3_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_0046.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_THEBIG);
        event_set_pos(event_get_player(0), -65.58F, 45.91F, 525.96F);
        event_set_ang(event_get_player(0), 0x553, 0x815f, 0xf992);
        event_wait(1);
        event_play_action(event_get_player(0), &big_walk_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_create_player(3, tails_step, 29.08F, 72.02F, 177.29F, 0, 0x400,
                            0);
        event_wait(1);
        event_play_action(event_get_player(3), &ev00d3_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_task(&ev00d3_frog, ev00e3_frog_create, 29.08F, 72.02F,
                         177.29F, 0, 0x3205, 0);
        event_wait(1);
        event_set_flags(ev00d3_frog, EVENT_IGNORE_GROUND);
        event_play_action(ev00d3_frog, &ev00d3_frog_action_1,
                          &ev00d3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -85.07F, 55.96F, 557.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0xe800, 0);
        event_camera_follow(event_get_player(0));
        event_sound_start(0, 1223, 1800);
        event_run_to_speed(event_get_player(0), -11.62F, 50.0F, 395.14F, 0.8F,
                           0.0F);
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_follow_clear();
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -15.31F, 100.75F, 375.77F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0xf700, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x222);
        event_wait(60);
        event_voice_play(1822);
        event_message_wait(90, TEXT_1);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 30.12F, 79.36F, 159.34F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0x7d00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1555);
        event_play_action(event_get_player(0), &big_run_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 23.61F, 72.86F, 202.42F, 0.9F,
                           1.0F);
        event_wait(35);
        vibration_start(0, 7, 20, 10);
        event_camera_follow(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -5.32F, 84.27F, 354.34F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0xf600, 0x200);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xf900, 0xf600, 0xff00);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfd00, 0xf600, 0xfe00);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfa00, 0xf600, 0xff00);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfd00, 0xf600, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfa00, 0xf600, 0x100);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfe00, 0xf600, 0x200);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xf900, 0xf600, 0x100);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfd00, 0xf600, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfa00, 0xf600, 0x100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfe00, 0xf600, 0x200);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xf900, 0xf600, 0x100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfd00, 0xf600, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfa00, 0xf600, 0x100);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfe00, 0xf600, 0x200);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xf900, 0xf600, 0x100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfd00, 0xf600, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfa00, 0xf600, 0x100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfe00, 0xf600, 0x200);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xf900, 0xf600, 0x100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfd00, 0xf600, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfa00, 0xf600, 0x100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfe00, 0xf600, 0x200);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xf900, 0xf600, 0x100);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xfb00, 0xf600, 0);
        event_camera_follow_clear();
        event_wait(3);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 30.12F, 79.36F, 159.34F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0x7d00, 0);
        event_talk_start(event_get_player(3), TEXT_7);
        event_wait(23);
        event_sound_stop(0);
        event_play_action(event_get_player(0), &ev00d3_big_action_0,
                          &big_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev00d3_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1823);
        event_message_show(TEXT_2);
        event_wait(42);
        event_sound_play(1224, 0, 0, 0);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xf900, 0x7d00, 0);
        event_wait(3);
        event_follow(ev00d3_frog, 0.0F, 0.0F, 0.0F, 200, event_get_player(3));
        event_move(event_get_player(3), 29.08F, 72.02F, 177.29F, 29.08F, 78.02F,
                   177.29F, 25);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0xfb00, 0x7d00, 0);
        event_voice_play(1824);
        event_message_show(TEXT_3);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.29F, 85.65F, 210.39F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf500, 0xd000, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0xf500, 0xd300, 0);
        event_play_action(event_get_player(3), &ev00d3_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00d3_frog, &ev00d3_frog_action_3,
                          &ev00d3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(3), 29.08F, 72.02F, 177.29F, 29.08F, 72.02F,
                   167.69F, 60);
        event_wait(33);
        event_message_close();
        event_wait(30);
        event_play_action(event_get_player(3), &ev00d3_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00d3_frog, &ev00d3_frog_action_1,
                          &ev00d3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 26.55F, 76.4F, 183.41F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf000, 0x7a00, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x700, 0x7a00, 0);
        event_wait(60);
        event_voice_play(1825);
        event_message_show(TEXT_4);
        event_wait(58);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 29.93F, 78.83F, 175.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x700, 0x400, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0xf000, 0x400, 0);
        event_voice_play(1271);
        event_message_show(TEXT_5);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.03F, 79.85F, 170.03F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xae00, 0);
        event_clear_motions(ev00d3_frog);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev00d3_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(ev00d3_frog, &ev00d3_frog_action_0,
                          &ev00d3_frog_texlist, 0.6F, EVENT_REPEAT, 0);
        event_wait(1);
        event_sound_play(1226, 0, 0, 0);
        event_clear_motions(ev00d3_frog);
        event_move(ev00d3_frog, 28.58F, 72.02F, 172.29F, 27.88F, 74.04F,
                   177.33F, 30);
        event_play_action(ev00d3_frog, &ev00d3_frog_action_0,
                          &ev00d3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_move(ev00d3_frog, 27.88F, 74.04F, 177.33F, 24.88F, 84.74F,
                   196.33F, 24);
        event_wait(26);
        event_sound_play(1227, 0, 0, 0);
        event_move(event_get_player(0), 23.47F, 72.02F, 203.18F, 23.47F, 74.02F,
                   203.18F, 1);
        event_wait(2);
        event_move(ev00d3_frog, 24.88F, 84.74F, 196.33F, 12.45F, 70.48F,
                   243.92F, 48);
        event_wait(49);
        event_set_flags(ev00d3_frog, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 26.43F, 84.51F, 181.45F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe63e, 0x7d00, 0);
        event_voice_play(1831);
        event_message_show(TEXT_6);
        event_clear_motions(ev00d3_frog);
        event_play_action(ev00d3_frog, &ev00d3_frog_action_2,
                          &ev00d3_frog_texlist, 2.8F, EVENT_REPEAT, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 27.4F, 94.53F, 176.61F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe700, 0x7a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 800, 32.2F, 117.56F, 144.26F);
        event_set_pos(ev00d3_frog, 12.45F, 74.48F, 243.92F);
        event_set_ang(ev00d3_frog, 0, 0, 0);
        event_set_flags(ev00d3_frog, EVENT_IGNORE_GROUND);
        event_set_radius(ev00d3_frog, 2.0F);
        event_wait(20);
        event_move(ev00d3_frog, 12.45F, 74.48F, 243.92F, 40.39F, 73.3F, 541.19F,
                   420);
        event_wait(10);
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_wait(100);
        event_kill_task(&ev00d3_frog);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_set_pos(event_get_player(0), 23.47002F, 72.01948F, 203.18012F);
        event_set_ang(event_get_player(0), 0, 0x78a9, 0);
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(3);
        event_kill_task(&ev00d3_frog);
        vibration_start(0, 0, 0, 0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00d3_textures[] = {
    {"MILES", &tails_texlist},
    {"FROG", &ev00d3_frog_texlist},
    {NULL, NULL},
};
