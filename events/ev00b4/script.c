/* EV00B4, E-102's story: Eggman pits E-102 against E-101 in a test for a
 * place on the Egg Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00b4/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/dat_8c5a644c.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/dust.h"
#include "shared/e101.h"
#include "shared/e101_textures.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 32
#define TEXT_2_LENGTH 65
#define TEXT_3_LENGTH 52
#define TEXT_4_LENGTH 17
#define TEXT_5_LENGTH 58
#define TEXT_6_LENGTH 74
#define TEXT_7_LENGTH 58
#include "ev00b4/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION eggman_walk_action;

extern NJS_ACTION ev00b4_e102_action_0;
extern NJS_ACTION ev00b4_e102_action_1;
extern NJS_ACTION ev00b4_eggman_action_0;
extern NJS_ACTION ev00b4_eggman_action_1;
extern NJS_ACTION ev00b4_e101_action_0;
extern NJS_ACTION ev00b4_e101_action_1;
extern NJS_ACTION ev00b4_e101_action_2;

static struct task *ev00b4_dust = NULL;
static struct task *ev00b4_e101 = NULL;

void ev00b4_script(int phase)
{
    /* @hack spelling: event_camera_set_ang called through a local. SHC then
     * puts event_camera_set_pos's address in R11 and event_camera_set_ang's in
     * R12 (0x0cb80106, 0x0cb80120). called by name, the two trade, and 42 of
     * 1130 words differ */
    void (*camera_set_ang)(enum event_ease ease, int steps, Angle ang_x,
                           Angle ang_y, Angle ang_z);
    /* @hack spelling: the camera's shake position through the locals x, y
     * and z, which SHC keeps in FR14, FR12 and FR13 from 0x0cb804ec on; as
     * literals, the code runs 8 bytes long and 714 of 1130 words differ */
    Float x;
    Float y;
    Float z;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -15.54F, 0.45F, -10.36F);
        event_set_ang(event_get_player(0), 0, 0x25c6, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(2, eggman_step, -162.8F, 0.21F, 22.26F, 0, 0x5982,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(2), 6.74F, 0.96F, 5.2F);
        event_set_ang(event_get_player(2), 0, 0xc3f7, 0);
        event_reset_player(2);
        stream_request_music(MUSIC_THEE102);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -52.58F, 15.46F, -0.99F);
        camera_set_ang = event_camera_set_ang;
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfaa5, 0xc150, 0);
        event_voice_play(1687);
        event_message_wait(0, TEXT_1);
        event_wait(31);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &eggman_walk_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), -36.97F, 0.5F, 0.66F, 0.15F,
                           0.06F);
        event_voice_play(1688);
        event_message_wait(0, TEXT_2);
        event_wait(80);
        event_play_action(event_get_player(0), &ev00b4_e102_action_0,
                          &e102_texlist, 0.5F, 0, 16);
        event_wait(20);
        event_wait_motions(event_get_player(0));
        event_set_pos(event_get_player(0), -15.54F, 0.45F, -10.36F);
        event_set_ang(event_get_player(0), 0, 0xc8d1, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -2.17F, 20.44F, 2.19F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf1a5, 0x3d50, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(event_get_player(2), &ev00b4_eggman_action_1,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(1);
        event_wait_motions(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_set_pos(event_get_player(2), -36.97F, 0.5F, 0.66F);
        event_set_ang(event_get_player(2), 0, 0x4ab2, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -48.04F, 15.28F, -3.11F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfea5, 0xbd50, 0);
        event_voice_play(1689);
        event_message_wait(0, TEXT_3);
        event_wait(30);
        event_play_action(event_get_player(2), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 16);
        event_wait(120);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00b4_eggman_action_0,
                          &eggman_texlist, 1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -24.09F, 4.83F, -3.27F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x15a5, 0x4b50, 0);
        event_wait(10);
        event_voice_play(1690);
        event_message_wait(0, TEXT_4);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -38.53F, 10.35F, 12.96F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x01a5, 0xdb50, 0);
        /* E-101 drops in from above. the camera shakes when it lands */
        event_place_task(&ev00b4_e101, event_e101_create, -30.0F, -46.7F,
                         -30.0F, 0, 0xc000, 0);
        event_wait(1);
        event_set_shadow(ev00b4_e101, 1.3F);
        event_set_flags(ev00b4_e101, EVENT_IGNORE_GROUND);
        event_set_pos(ev00b4_e101, 9.98F, 135.43F, -4.15F);
        event_set_ang(ev00b4_e101, 0, 0x46d5, 0);
        event_clear_motions(ev00b4_e101);
        event_play_action(ev00b4_e101, &ev00b4_e101_action_0,
                          &event_e101_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev00b4_e101, 9.98F, 135.43F, -4.15F, 9.98F, 14.5F, -4.15F,
                   50);
        event_wait(20);
        /* @hack spelling: y and z set ahead of x, which loads them in that
         * order (0x0cb804ee to 0x0cb804f6); set as x, y, z, 5 of 1130 words
         * differ */
        y = 8.32F;
        z = 21.56F;
        x = -49.37F;
        /* @hack spelling: the height as the literal y holds, which SHC
         * still takes from FR12; passed as y, y and z trade FR12 and FR13,
         * and 20 of 1130 words differ */
        event_camera_set_pos(EVENT_EASE_LINEAR, 5, x, 8.32F, z);
        camera_set_ang(EVENT_EASE_LINEAR, 5, 0x06a5, 0xdb50, 0);
        event_wait(20);
        event_play_action(event_get_player(0), &ev00b4_e102_action_1,
                          &e102_texlist, 1.0F, 0, 0);
        event_wait(11);
        if (ev00b4_dust == NULL) {
            ev00b4_dust = event_dust_create();
        }
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then keeps z in FR13 and x in FR14 (0x0cb804f2, 0x0cb804f6), and
         * moves each color into FR3 through FPUL (0x0cb805ee, 0x0cb805fe). as
         * an if, the code runs 4 bytes short, and 591 of 1130 words differ */
        ev00b4_dust != NULL &&
            (DUST_WORK(ev00b4_dust)->params->color.a = 150.0F,
             DUST_WORK(ev00b4_dust)->params->color.r = 50.0F,
             DUST_WORK(ev00b4_dust)->params->color.g = 50.0F,
             DUST_WORK(ev00b4_dust)->params->color.b = 50.0F);
        if (ev00b4_dust != NULL) {
            DUST_WORK(ev00b4_dust)->timer = DUST_WORK(ev00b4_dust)->interval =
                1;
        }
        event_move(ev00b4_dust, 9.98F, 2.5F, -4.15F, 9.98F, 2.5F, -4.15F, 1);
        event_sound_play(785, 0, 0, 0);
        vibration_start(0, 3, 5, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x07a5, 0xdb50, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x06a5, 0xdb50, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x07a5, 0xdb50, 0);
        event_wait(2);
        event_sound_play(785, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x05a5, 0xdb50, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x06a5, 0xdb50, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x05a5, 0xdb50, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x07a5, 0xdb50, 0);
        event_sound_play(785, 0, 0, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, x, y, z);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x06a5, 0xdb50, 0);
        event_move_stop(ev00b4_dust);
        if (ev00b4_dust != NULL) {
            DUST_WORK(ev00b4_dust)->timer = DUST_WORK(ev00b4_dust)->interval =
                0;
        }
        event_wait(33);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1.73F, 17.28F, -5.08F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x09a5, 0xbb50, 0);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1691);
        event_message_wait(0, TEXT_5);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1.98F, 13.37F, -10.44F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x05a5, 0xa450, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -3.88F, 1.86F, -1.41F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0ba5, 0xcd50, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -6.55F, 11.46F, -0.5F);
        camera_set_ang(EVENT_EASE_LINEAR, 140, 0x0ba5, 0xcd50, 0);
        event_play_action(ev00b4_e101, &ev00b4_e101_action_0,
                          &event_e101_texlist, 1.0F, 0, 8);
        event_wait(8);
        event_play_action(ev00b4_e101, &ev00b4_e101_action_1,
                          &event_e101_texlist, 1.0F, 0, 8);
        event_wait(8);
        event_play_action(ev00b4_e101, &ev00b4_e101_action_2,
                          &event_e101_texlist, 1.0F, EVENT_REPEAT, 8);
        event_set_pos(event_get_player(0), -32.4F, 0.51F, -23.22F);
        event_set_ang(event_get_player(0), 0, 0x2177, 0);
        event_voice_play(1692);
        event_message_wait(0, TEXT_6);
        event_wait(1);
        event_set_pos(event_get_player(2), -164.7F, 0.21F, -23.03F);
        event_set_ang(event_get_player(2), 0, 0xba38, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -59.71F, 7.09F, 24.04F);
        camera_set_ang(EVENT_EASE_LINEAR, 100, 0x02a5, 0xde50, 0);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 8);
        event_voice_play(1693);
        event_message_wait(0, TEXT_7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -51.61F, 27.04F, -28.78F);
        camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf1a5, 0xb350, 0);
        event_wait(60);
        /* E-101 moves off, raising dust again */
        event_move(ev00b4_e101, 9.98F, 14.5F, -4.15F, 77.61F, 14.5F, 14.89F,
                   30);
        event_sound_start(0, 786, 1800);
        event_sound_fade_volume(0, 200, 1);
        if (ev00b4_dust == NULL) {
            ev00b4_dust = event_dust_create();
        }
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then keeps z in FR13 and x in FR14 (0x0cb804f2, 0x0cb804f6). as an
         * if, the two trade, and 151 of 1130 words differ */
        ev00b4_dust != NULL &&
            (DUST_WORK(ev00b4_dust)->params->color.a = 250.0F,
             DUST_WORK(ev00b4_dust)->params->color.r = 250.0F,
             DUST_WORK(ev00b4_dust)->params->color.g = 250.0F,
             DUST_WORK(ev00b4_dust)->params->color.b = 250.0F);
        if (ev00b4_dust != NULL) {
            DUST_WORK(ev00b4_dust)->timer = DUST_WORK(ev00b4_dust)->interval =
                2;
        }
        event_move(ev00b4_dust, 9.98F, 2.5F, -4.15F, 77.61F, 2.5F, 14.89F, 30);
        event_wait(20);
        event_sound_stop(0);
        event_move_stop(ev00b4_dust);
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then keeps z in FR13 and x in FR14 (0x0cb804f2, 0x0cb804f6). as an
         * if, the two trade, the code runs 24 bytes long, and 361 of 1130 words
         * differ */
        ev00b4_dust != NULL && (DUST_WORK(ev00b4_dust)->timer =
                                    DUST_WORK(ev00b4_dust)->interval = 0);
        event_wait(40);
        event_message_close();
        event_move_stop(ev00b4_e101);
        break;
    case EVENT_END:
        event_reset_player(2);
        event_reset_player(0);
        event_kill_player(2);
        event_move_stop_all();
        event_kill_task(&ev00b4_e101);
        EVENT_KILL(ev00b4_dust);
        event_sound_stop_all();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b4_textures[] = {
    {"EV_E101_BODY", &event_e101_texlist},
    {"EV_E101_FUN", &event_e101_jet_texlist},
    {NULL, NULL},
};
