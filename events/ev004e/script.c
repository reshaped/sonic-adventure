/* EV004E, Tails' story: Tails resolves to stop the missile without Sonic,
 * and runs after Eggman */
#include <ninja.h>

#include "ev004e/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/dust.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 47
#define TEXT_3_LENGTH 64
#define TEXT_4_LENGTH 17
#define TEXT_5_LENGTH 46
#define TEXT_6_LENGTH 52
#define TEXT_7_LENGTH 62
#define TEXT_8_LENGTH 1
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 1
#define TEXT_12_LENGTH 30
#define TEXT_13_LENGTH 1
#define TEXT_14_LENGTH 4
#define TEXT_15_LENGTH 6
#include "ev004e/script/text.inc"

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_ACTION ev004e_tails_action_0;
extern NJS_ACTION ev004e_tails_action_1;
extern NJS_ACTION ev004e_tails_action_2;
extern NJS_ACTION ev004e_tails_action_3;

/* Tails' run. from key 3 to key 71, the stick is pushed nearly all the way. its
 * direction there, in 256ths of a turn, stays at -70 to -65, near minus a
 * quarter. A is held for two jumps, at keys 22 to 27 and 44 to 48. from key 88
 * to key 92, the stick points the other way, at 57 to 66 */
static const struct event_pad_key ev004e_run_keys[165] = {
#include "ev004e/script/ev004e_run_keys.inc"
};

static const struct event_pad_recording ev004e_run = {
    ev004e_run_keys, sizeof(ev004e_run_keys) / sizeof(ev004e_run_keys[0])};

static struct task *ev004e_dust_0 = NULL;
static struct task *ev004e_dust_1 = NULL;
static struct task *ev004e_fade = NULL;

void ev004e_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_004e.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_flag_set(FLAG_TAILS_SS_ENTRANCE_HIGHWAY);
        stream_request_music(MUSIC_EVTBGM01);
        event_eggmobile_create(316.96F, 10.01F, 1374.82F, 0, 0x7800, 0);
        event_set_pos(event_get_player(0), 340.01F, 0.01F, 1650.25F);
        event_set_ang(event_get_player(0), 0, 0x7800, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 312.21F, 13.23F, 1587.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0xe300, 0);
        if (ev004e_dust_0 == NULL) {
            ev004e_dust_0 = event_dust_create();
        }
        if (ev004e_dust_0 != NULL) {
            DUST_WORK(ev004e_dust_0)->timer =
                DUST_WORK(ev004e_dust_0)->interval = 5;
        }
        if (ev004e_dust_1 == NULL) {
            ev004e_dust_1 = event_dust_create();
        }
        if (ev004e_dust_1 != NULL) {
            DUST_WORK(ev004e_dust_1)->timer =
                DUST_WORK(ev004e_dust_1)->interval = 10;
        }
        /* @hack spelling: the three tests as && expressions, not if statements.
         * SHC then loads the speed's 0.0 into FR3 where it stores it (FLDI0
         * FR3 at 0x0cb801f0) and keeps event_camera_set_ang in R9, as the
         * image does. as ifs, SHC keeps 0.0 in FR15 and event_camera_set_ang
         * in R10. ev004e_script then runs 16 bytes short, and 776 of 968
         * words differ */
        ev004e_dust_0 != NULL &&
            (DUST_WORK(ev004e_dust_0)->params->scale = 10.0F);
        ev004e_dust_1 != NULL &&
            (DUST_WORK(ev004e_dust_1)->params->scale = 20.0F);
        ev004e_dust_0 != NULL &&
            (DUST_WORK(ev004e_dust_0)->params->speed.x = -30.0F,
             DUST_WORK(ev004e_dust_0)->params->speed.y = 0.0F,
             DUST_WORK(ev004e_dust_0)->params->speed.z = 0.0F);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev004e_run);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 328.81F, 7.01F, 1575.926F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0xff00, 0xf700, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 80, 0x2000);
        event_wait(80);
        event_sound_play(1223, 0, 0, 0);
        event_set_pos(ev004e_dust_0, 370.555F, 1.045F, 1360.721F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_8);
        event_voice_play(1325);
        event_message_wait(60, TEXT_1);
        event_talk_stop(event_get_player(0));
        event_sound_start(0, 1228, 1800);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_wait(3);
        event_sound_fade_volume(0, -10, 100);
        event_eggmobile_slide_to(443.071F, 21.999F, 1371.183F, 60);
        event_wait(5);
        event_run_to_speed(event_get_player(0), 341.37F, 0.0F, 1372.9F, 0.0F,
                           0.0F);
        event_wait(20);
        event_set_pos(ev004e_dust_1, 340.74F, 11.38F, 1371.89F);
        event_wait(5);
        if (ev004e_dust_1 != NULL) {
            DUST_WORK(ev004e_dust_1)->timer =
                DUST_WORK(ev004e_dust_1)->interval = 60;
        }
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 308.97F, 11.38F, 1328.77F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd00, 0x9900, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_wait(5);
        event_message_clear();
        event_eggmobile_slide_to(615.606F, 17.837F, 1382.19F, 60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 330.21F, 8.65F, 1358.93F);
        event_wait(60);
        if (ev004e_dust_1 != NULL) {
            DUST_WORK(ev004e_dust_1)->timer =
                DUST_WORK(ev004e_dust_1)->interval = 0;
        }
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004e_tails_action_1,
                          &tails_texlist, 1.0F, 0, 8);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004e_tails_action_2,
                          &tails_texlist, 2.0F, 0, 0);
        event_wait_motions(event_get_player(0));
        event_sound_stop(0);
        event_set_pos(event_get_player(0), 342.37F, 0.0F, 1371.9F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004e_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        if (ev004e_dust_0 != NULL) {
            DUST_WORK(ev004e_dust_0)->timer =
                DUST_WORK(ev004e_dust_0)->interval = 0;
        }
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_9);
        event_voice_play(1326);
        event_message_wait(60, TEXT_2);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 353.6F, 13.59F, 1360.97F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xef00, 0x5f00, 0);
        event_look_at(
            event_get_player(0), event_get_player(0)->work->pos.x + 20.0F,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_10);
        event_voice_play(1328);
        event_message_wait(60, TEXT_3);
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_look_at(
            event_get_player(0), event_get_player(0)->work->pos.x + 10.0F,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(1329);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_clear();
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004e_tails_action_3,
                          &tails_texlist, 0.4F, 0, 20);
        event_wait(30);
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_wait(20);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004e_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(1330);
        event_message_wait(90, TEXT_5);
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 351.26F, 0.36F, 1382.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x2000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 346.74F, 3.19F, 1377.87F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(1331);
        event_message_wait(90, TEXT_6);
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 352.5F, 0.13F, 1361.89F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x6000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 346.69F, 3.77F, 1367.71F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_15);
        event_voice_play(1332);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        event_play_action(event_get_player(0), &ev004e_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 306.8F, 7.06F, 1374.22F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff00, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, 208.78F, 6.27F, 1374.22F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 300, 0x0500, 0xc000, 0);
        event_clear_motions(event_get_player(0));
        event_run_to_speed(event_get_player(0), 443.47F, 11.99F, 1371.18F, 0.0F,
                           0.0F);
        event_wait(40);
        ev004e_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_reset_player(0);
        EVENT_KILL(ev004e_dust_0);
        EVENT_KILL(ev004e_dust_1);
        event_eggmobile_kill();
        EVENT_KILL(ev004e_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev004e_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {NULL, NULL},
};
