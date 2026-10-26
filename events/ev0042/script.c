/* EV0042, Tails' story: Tails finds the red emerald, but Froggy takes it, and
 * Tails gives chase */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0042/emerald_action.h"
#include "ev0042/emerald_textures.h"
#include "ev0042/frog_textures.h"
#include "ev0042/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 28
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 17
#define TEXT_4_LENGTH 2
#define TEXT_5_LENGTH 1
#define TEXT_6_LENGTH 5
#include "ev0042/script/text.inc"

extern NJS_ACTION tails_run_action;

extern NJS_ACTION ev0042_tails_action;
extern NJS_ACTION ev0042_frog_action_0;
extern NJS_ACTION ev0042_frog_action_1;

static struct task *ev0042_frog = NULL;
static struct task *ev0042_emerald = NULL;

void ev0042_script(int phase)
{
    /* @hack spelling: event_play_action called through play, a local set just
     * before its first call. SHC then keeps the z angle the emerald and the
     * frog share, 0xf767, in R9 from 0x0cb80114. called as
     * event_play_action, R9 holds their x angle, 0xfc6f, and 12 of 736 words
     * differ */
    void (*play)(struct task *task, NJS_ACTION *action, NJS_TEXLIST *texlist,
                 Float speed, int flags, int blend);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_0042.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_MSTCLN);
        event_set_pos(event_get_player(0), -1128.37F, 29.3F, -255.28F);
        event_set_ang(event_get_player(0), 0, 0xd4ba, 0);
        event_wait(1);
        event_look_at(event_get_player(0), -1200.55F, 84.82F, -191.3F);
        event_clear_motions(event_get_player(0));
        play = event_play_action;
        play(event_get_player(0), &ev0042_tails_action, &tails_texlist, 1.0F,
             EVENT_REPEAT, 0);
        event_place_actor(&ev0042_emerald, -1193.45F, 49.84F, -194.97F, 0,
                          0x64c4, 0);
        event_wait(1);
        event_set_flags(ev0042_emerald, EVENT_IGNORE_GROUND);
        event_set_pos(ev0042_emerald, -1193.45F, 49.84F, -194.97F);
        event_set_ang(ev0042_emerald, 0xfc6f, 0xca03, 0xf767);
        event_wait(1);
        play(ev0042_emerald, &ev0042_emerald_action, &ev0042_emerald_texlist,
             1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1195.63F, 52.48F, -190.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf000, 0xe7c4, 0);
        event_wait(10);
        event_sound_stop(0);
        event_place_actor(&ev0042_frog, -1156.71F, 40.21F, -200.71F, 0xfc6f,
                          0xc209, 0xf767);
        event_wait(1);
        event_set_pos(ev0042_frog, -1156.71F, 42.21F, -200.71F);
        event_set_ang(ev0042_frog, 0xfc6f, 0xc209, 0xf767);
        event_wait(1);
        event_set_flags(ev0042_frog, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0042_frog);
        play(ev0042_frog, &ev0042_frog_action_0, &ev0042_frog_texlist, 1.0F,
             EVENT_REPEAT, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -1136.07F, 42.44F,
                             -247.79F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xe300, 0xdfc4, 0);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1250);
        event_message_wait(0, TEXT_1);
        event_wait(50);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1120.08F, 38.43F, -285.94F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0400, 0x64c4, 0xf900);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -1168.37F, 47.0F, -229.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x0400, 0x64c4, 0xf900);
        event_look_at(event_get_player(0), -1200.55F, 54.82F, -191.3F);
        event_clear_motions(event_get_player(0));
        play(event_get_player(0), &tails_run_action, &tails_texlist, 1.0F,
             EVENT_REPEAT, 20);
        event_wait(1);
        event_run_to_speed(event_get_player(0), -1180.8F, 45.61F, -209.11F,
                           0.55F, 0.06F);
        event_wait(10);
        event_sound_play(1225, 0, 0, 0);
        play(ev0042_frog, &ev0042_frog_action_1, &ev0042_frog_texlist, 1.0F,
             EVENT_REPEAT, 0);
        event_wait(30);
        event_sound_play(1224, 0, 0, 0);
        event_move(ev0042_frog, -1156.71F, 42.21F, -200.71F, -1208.55F, 56.11F,
                   -194.17F, 90);
        event_wait(30);
        event_sound_play(1224, 0, 0, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1196.19F, 53.85F, -184.19F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf600, 0xe9c4, 0);
        event_wait(20);
        event_sound_play(1224, 0, 0, 0);
        event_wait(5);
        event_sound_play(1223, 0, 0, 0);
        event_kill_task(&ev0042_emerald);
        event_wait(30);
        play(event_get_player(0), &ev0042_tails_action, &tails_texlist, 1.0F,
             EVENT_REPEAT, 8);
        event_look_at(event_get_player(0), -1263.15F, 114.87F, -137.09F);
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1251);
        event_message_wait(0, TEXT_2);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1170.3F, 44.47F, -207.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1499, 0x5ac4, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -1179.08F, 47.17F,
                             -200.96F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1499, 0x5ac4, 0);
        event_wait(1);
        event_set_pos(ev0042_frog, -1250.16F, 75.0F, -158.41F);
        event_set_ang(ev0042_frog, 0xefc1, 0xdd1a, 0xeb8c);
        event_wait(1);
        event_move(ev0042_frog, -1250.16F, 75.0F, -158.41F, -1262.08F, 80.95F,
                   -140.29F, 28);
        event_sound_play(1224, 0, 0, 0);
        event_clear_motions(event_get_player(0));
        play(event_get_player(0), &tails_run_action, &tails_texlist, 1.0F,
             EVENT_REPEAT, 20);
        event_wait(1);
        event_run_to_speed(event_get_player(0), -1197.47F, 50.99F, -193.91F,
                           0.5F, 0.06F);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1252);
        event_message_wait(0, TEXT_3);
        event_sound_play(1224, 0, 0, 0);
        event_wait(17);
        event_sound_play(1224, 0, 0, 0);
        event_move(ev0042_frog, -1262.08F, 80.95F, -140.29F, -1288.67F, 83.0F,
                   -124.28F, 28);
        play(event_get_player(0), &ev0042_tails_action, &tails_texlist, 1.0F,
             EVENT_REPEAT, 8);
        event_wait(10);
        event_sound_play(1224, 0, 0, 0);
        event_wait(10);
        event_sound_play(1224, 0, 0, 0);
        event_wait(10);
        event_move_stop_all();
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_task(&ev0042_frog);
        event_kill_task(&ev0042_emerald);
        event_set_pos(event_get_player(0), -1197.47F, 50.99F, -193.91F);
        event_set_ang(event_get_player(0), 0, 0xd4ba, 0);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0042_textures[] = {
    {"M_EM_RED", &ev0042_emerald_texlist},
    {"FROG", &ev0042_frog_texlist},
    {NULL, NULL},
};
