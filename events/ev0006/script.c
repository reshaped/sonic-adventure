/* EV0006, Sonic's story: Sonic finds Tails by his crashed plane. Tails shows
 * him the purple Chaos Emerald and invites him to his workshop */
#include <ninja.h>

#include "ev0006/emerald_action.h"
#include "ev0006/emerald_textures.h"
#include "ev0006/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 41
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 25
#define TEXT_4_LENGTH 44
#define TEXT_5_LENGTH 58
#define TEXT_6_LENGTH 48
#define TEXT_7_LENGTH 41
#define TEXT_8_LENGTH 21
#define TEXT_9_LENGTH 19
#define TEXT_10_LENGTH 42
#define TEXT_11_LENGTH 38
#define TEXT_12_LENGTH 48
#define TEXT_13_LENGTH 54
#define TEXT_14_LENGTH 63
#define TEXT_15_LENGTH 6
#define TEXT_16_LENGTH 4
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 11
#define TEXT_19_LENGTH 7
#define TEXT_20_LENGTH 7
#define TEXT_21_LENGTH 5
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 2
#define TEXT_24_LENGTH 5
#include "ev0006/script/text.inc"

extern NJS_ACTION sonic_walk_action;
extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_walk_action;

extern NJS_ACTION ev0006_sonic_action;
extern NJS_ACTION ev0006_tails_action;

static struct task *ev0006_fade = NULL;
static struct task *ev0006_emerald = NULL;

struct texture_pvm ev0006_textures[] = {
    {"M_EM_PURPLE", &ev0006_emerald_texlist},
    {"MILES", &tails_texlist},
    {NULL, NULL},
};

void ev0006_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -460.99F, 0.01F, 2013.55F);
        event_set_ang(event_get_player(0), 0, 0x640b, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_walk_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(3, tails_step,
                            event_get_player(0)->work->pos.x - 20.0F,
                            event_get_player(0)->work->pos.y,
                            event_get_player(0)->work->pos.z + 10.0F,
                            event_get_player(0)->work->ang.x,
                            0x4000 - event_get_player(0)->work->ang.y,
                            event_get_player(0)->work->ang.z);
        event_wait(1);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &tails_walk_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_actor(&ev0006_emerald, event_get_player(3)->work->pos.x,
                          event_get_player(3)->work->pos.y,
                          event_get_player(3)->work->pos.z,
                          event_get_player(3)->work->ang.x,
                          0x4000 - event_get_player(3)->work->ang.y,
                          event_get_player(3)->work->ang.z);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xdc00, 0x6200, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -386.45F, 64.6F, 1920.33F);
        event_wait(1);
        stream_request_music(MUSIC_THEMILES);
        event_look_at_task(event_get_player(0), event_get_player(3), 0.0F, 6.0F,
                           0.0F);
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_wait(1);
        event_run_to_speed(event_get_player(0), -450.67F, 0.01F, 1901.05F,
                           0.58F, 0.06F);
        event_run_to_speed(event_get_player(3), -460.17F, 1.01F, 1912.28F,
                           0.56F, 0.06F);
        event_wait(60);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xfd00, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -453.86F, 8.05F, 1881.74F);
        event_wait(30);
        event_clear_motions(event_get_player(3));
        event_clear_motions(event_get_player(0));
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_15);
        event_voice_play(950);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(951);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_clear();
        event_wait(30);
        event_set_ang(event_get_player(3), 0, 0x6800, 0);
        event_set_ang(event_get_player(0), 0, 0xe200, 0);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf800, 0x1b00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -452.91F, 7.07F, 1927.46F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xfb00, 0x0a00, 0);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(952);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_voice_play(953);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(5);
        event_run_to_speed(
            event_get_player(3), event_get_player(0)->work->pos.x - 8.0F,
            event_get_player(0)->work->pos.y,
            event_get_player(0)->work->pos.z + 8.0F, 0.56F, 0.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xfb00, 0x0400, 0);
        event_wait(30);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_18);
        event_voice_play(954);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(955);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0x5300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -442.65F, 5.6F, 1902.02F);
        event_set_pos(ev0006_emerald, event_get_player(3)->work->pos.x,
                      event_get_player(3)->work->pos.y,
                      event_get_player(3)->work->pos.z);
        event_set_ang(ev0006_emerald, event_get_player(3)->work->ang.x,
                      0x4000 - event_get_player(3)->work->ang.y,
                      event_get_player(3)->work->ang.z);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_20);
        event_voice_play(956);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_voice_play(957);
        event_message_show(TEXT_8);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_sound_play(704, 0, 0, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe100, 0x6700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -444.47F, 26.94F, 1889.1F);
        event_play_action(event_get_player(3), &ev0006_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0006_emerald, &ev0006_emerald_action,
                          &ev0006_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xe100, 0x6700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -456.14F, 6.09F, 1906.57F);
        event_wait(90);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfb00, 0xe100, 0);
        event_play_action(event_get_player(0), &ev0006_sonic_action,
                          &sonic_texlist, 0.3F, EVENT_REPEAT, 8);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_21);
        event_voice_play(958);
        event_message_show(TEXT_9);
        event_wait(1);
        event_wait(45);
        event_message_close();
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0x5d00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -450.8F, 5.61F, 1901.96F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 240, 0xfd00, 0x2400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, -441.94F, 6.71F, 1917.03F);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_22);
        event_voice_play(959);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_wait(20);
        event_voice_play(960);
        event_message_show(TEXT_11);
        event_wait(1);
        event_voice_wait();
        event_voice_play(961);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_message_clear();
        event_wait(45);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_23);
        event_voice_play(962);
        event_message_show(TEXT_13);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_look_at_task(event_get_player(0), event_get_player(3), 0.0F, 6.0F,
                           0.0F);
        event_look_clear(event_get_player(3));
        event_look_at(event_get_player(3), event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -444.835F, 6.55F, 1912.076F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0x1800, 0);
        event_clear_motions(event_get_player(3));
        event_clear_motions(ev0006_emerald);
        event_run_to_speed(event_get_player(3), -463.14F, 0.0F, 1878.86F, 0.46F,
                           0.05F);
        event_wait(10);
        /* Sonic turns 0x200 a step to watch Tails run off. the turn skips
         * 0xb600, so one step turns 0x400 */
        event_set_ang(event_get_player(0), 0, 0xca00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xc800, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xc600, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xc400, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xc200, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xbe00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xbc00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xba00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xb800, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xb400, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xb200, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xb000, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xae00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xac00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xaa00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xa800, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xa600, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xa400, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xa200, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xa000, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0x9e00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0x9c00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0x9a00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0x9800, 0);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_24);
        event_voice_play(963);
        event_message_show(TEXT_14);
        event_wait(1);
        event_run_to_speed(event_get_player(3), -392.21F, 0.0F, 1844.77F, 0.46F,
                           0.05F);
        event_wait(90);
        event_message_close();
        event_talk_stop(event_get_player(3));
        ev0006_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), -450.67F, 0.01F, 1901.05F);
        event_set_ang(event_get_player(0), 0, 0x9800, 0);
        event_kill_task(&ev0006_emerald);
        event_kill_player(3);
        event_reset_player(0);
        EVENT_KILL(ev0006_fade);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
