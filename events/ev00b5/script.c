/* EV00B5, E-102's story: Eggman lets E-102 and E-101 aboard the Egg
 * Carrier */
#include <ninja.h>

#include "ev00b5/e101_action_3.h"
#include "ev00b5/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/dat_8c5a644c.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/e101.h"
#include "shared/e101_textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 28
#define TEXT_2_LENGTH 55
#define TEXT_3_LENGTH 58
#define TEXT_4_LENGTH 49
#define TEXT_5_LENGTH 26
#define TEXT_6_LENGTH 42
#define TEXT_7_LENGTH 19
#include "ev00b5/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_run_action;
extern NJS_ACTION eggman_walk_action;
extern NJS_ACTION eggman_turn_action;

extern NJS_ACTION ev00b5_eggman_action_0;
extern NJS_ACTION ev00b5_eggman_action_1;
extern NJS_ACTION ev00b5_e101_action_0;
extern NJS_ACTION ev00b5_e101_action_1;
extern NJS_ACTION ev00b5_e101_action_2;

static struct task *ev00b5_e101 = NULL;
static struct task *ev00b5_fade = NULL;

void ev00b5_script(int phase)
{
    /* @hack spelling: event_sound_play called through a local, set in its first
     * call. SHC then keeps its address in R10 and the 782 it plays in R9
     * (0x0cb802a0, 0x0cb802a2). called by name, the two trade, and 28 of 887
     * words differ */
    void (*sound_play)(int se, int volume, int pan, int pitch);
    /* @hack spelling: event_camera_set_ang called through a local, set in its
     * first call. SHC then keeps its address in R8 (0x0cb80274). called by
     * name, R8 takes event_camera_set_pos's address, and 137 of 887 words
     * differ */
    void (*camera_set_ang)(enum event_ease ease, int steps, Angle ang_x,
                           Angle ang_y, Angle ang_z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_THEE102);
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
        event_set_ang(event_get_player(2), 0, 0xa795, 0);
        event_reset_player(2);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 0);
        event_place_task(&ev00b5_e101, event_e101_create, -30.0F, -46.7F,
                         -30.0F, 0, 0xc000, 0);
        event_wait(1);
        event_set_shadow(ev00b5_e101, 1.3F);
        event_set_flags(ev00b5_e101, EVENT_IGNORE_GROUND);
        event_set_pos(ev00b5_e101, 48.36F, 14.5F, 3.06F);
        event_set_ang(ev00b5_e101, 0, 0x42ef, 0);
        event_clear_motions(ev00b5_e101);
        event_play_action(ev00b5_e101, &ev00b5_e101_action_0,
                          &event_e101_texlist, 0.5F, EVENT_REPEAT, 0);
        event_move(ev00b5_e101, 48.36F, 14.5F, 3.06F, 16.52F, 14.5F, -4.69F,
                   200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -67.44F, 19.95F, 7.77F);
        (camera_set_ang = event_camera_set_ang)(EVENT_EASE_LINEAR, 0, 0xf6a5,
                                                0xc350, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -6.5F, 16.03F, 2.72F);
        camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf8a5, 0xbb50, 0);
        (sound_play = event_sound_play)(782, 0, 0, 0);
        event_voice_play(1694);
        event_message_wait(0, TEXT_1);
        sound_play(782, 0, 0, 0);
        event_wait(30);
        sound_play(782, 0, 0, 0);
        event_wait(30);
        sound_play(782, 0, 0, 0);
        event_wait(30);
        sound_play(782, 0, 0, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 8.66F, 13.14F, 32.55F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfba5, 0x0d50, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -34.72F, 19.17F, -3.82F);
        camera_set_ang(EVENT_EASE_LINEAR, 300, 0xf8a5, 0xc350, 0);
        event_wait(20);
        sound_play(782, 0, 0, 0);
        event_voice_play(1697);
        event_message_wait(0, TEXT_2);
        event_wait(20);
        sound_play(782, 0, 0, 0);
        event_play_action(ev00b5_e101, &ev00b5_e101_action_1,
                          &event_e101_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(25);
        sound_play(782, 0, 0, 0);
        event_voice_play(1699);
        event_message_wait(0, TEXT_3);
        event_wait(80);
        event_play_action(event_get_player(2), &ev00b5_eggman_action_1,
                          &eggman_texlist, 1.0F, 0, 8);
        event_wait_motions(event_get_player(2));
        event_set_pos(ev00b5_e101, 16.42F, 14.5F, -0.48F);
        event_set_ang(ev00b5_e101, 0, 0x2188, 0);
        event_move(ev00b5_e101, 16.42F, 14.5F, -0.48F, 9.89F, 14.5F, -6.77F,
                   20);
        sound_play(782, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1.48F, 14.91F, 11.59F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x05a5, 0xe350, 0);
        event_wait(2);
        sound_play(782, 0, 0, 0);
        event_play_action(event_get_player(2), &ev00b5_eggman_action_0,
                          &eggman_texlist, 1.0F, 0, 0);
        event_voice_play(1700);
        event_message_wait(0, TEXT_4);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 11.8F, 16.32F, 6.85F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x02a5, 0x2550, 0);
        event_set_pos(ev00b5_e101, 7.63F, 14.5F, -19.11F);
        event_set_ang(ev00b5_e101, 0, 0xd9eb, 0);
        event_move(ev00b5_e101, 7.63F, 14.5F, -19.11F, 1.06F, 14.5F, -1.1F, 20);
        sound_play(782, 0, 0, 0);
        event_wait(10);
        sound_play(782, 0, 0, 0);
        event_wait(10);
        sound_play(782, 0, 0, 0);
        event_wait(40);
        event_voice_play(1701);
        event_message_wait(0, TEXT_5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1.13F, 16.38F, 6.18F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf2a5, 0xc650, 0);
        event_voice_play(1702);
        event_message_wait(0, TEXT_6);
        event_voice_play(1703);
        event_message_wait(0, TEXT_7);
        event_wait(20);
        event_move(ev00b5_e101, 1.06F, 12.5F, -1.1F, 1.06F, 14.5F, -1.1F, 5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 18.13F, 15.8F, 11.71F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0x02a5, 0x2550, 0);
        event_wait(30);
        event_play_action(event_get_player(2), &eggman_turn_action,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(1);
        event_wait_motions(event_get_player(2));
        event_set_pos(event_get_player(0), -25.78F, 0.47F, 7.28F);
        event_set_ang(event_get_player(0), 0, 0x332f, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -9.48F, 0.45F, 11.14F, 0.5F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 51.43F, 6.08F, 29.13F);
        camera_set_ang(EVENT_EASE_LINEAR, 0, 0xffa5, 0x2550, 0);
        event_run_to_speed(event_get_player(2), 55.17F, 0.54F, 11.45F, 0.3F,
                           0.06F);
        event_set_pos(event_get_player(2), 6.74F, 0.96F, 5.2F);
        event_set_ang(event_get_player(2), 0, 0x3cad, 0);
        event_play_action(event_get_player(2), &eggman_walk_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_message_close();
        event_wait(20);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, 0, 8);
        event_play_action(ev00b5_e101, &ev00b5_e101_action_1,
                          &event_e101_texlist, 1.0F, 0, 8);
        event_wait(8);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 8);
        event_play_action(ev00b5_e101, &ev00b5_e101_action_2,
                          &event_e101_texlist, 1.0F, 0, 8);
        event_wait(8);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(ev00b5_e101, &ev00b5_e101_action_3,
                          &event_e101_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(10);
        ev00b5_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        event_move_stop(ev00b5_e101);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_move_stop(ev00b5_e101);
        event_reset_player(0);
        event_kill_task(&ev00b5_e101);
        event_kill_player(2);
        /* @hack spelling: the kill and the clear as one statement. SHC then
         * reads the slot again for the call (MOV.L @R2,R4 at 0x0cb80986). as
         * two statements, event_sound_play's address and its 782 trade R10 and
         * R9, and 39 of 887 words differ */
        if (ev00b5_fade != NULL) {
            task_kill(ev00b5_fade), ev00b5_fade = NULL;
        }
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b5_textures[] = {
    {"EV_E101_BODY", &event_e101_texlist},
    {"EV_E101_FUN", &event_e101_jet_texlist},
    {NULL, NULL},
};
