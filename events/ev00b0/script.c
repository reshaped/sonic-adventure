/* EV00B0, E-102's story: Eggman starts up and names E-102 */
#include <ninja.h>

#include "ev00b0/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/dat_8c5a644c.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/noise.h"
#include "shared/scanlines.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 31
#define TEXT_2_LENGTH 69
#define TEXT_3_LENGTH 58
#define TEXT_4_LENGTH 45
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 54
#include "ev00b0/script/text.inc"

/* @hack type: e102_stand_action declared as an array, which script.c passes by
 * its name. SHC then keeps the camera's x in FR13 and its y in FR12 (FMOV.S
 * FR13,FR4 at 0x0cb80462). as one NJS_ACTION passed as &e102_stand_action, the
 * two trade, and 84 of ev00b0_script's 1215 words differ */
extern NJS_ACTION e102_stand_action[];
/* @hack type: eggman_walk_action declared as an array, which script.c passes by
 * its name. SHC then keeps event_sound_start's 1800 in R8 (0x0cb800a4). as one
 * NJS_ACTION passed as &eggman_walk_action, R8 takes event_sound_start's
 * address instead, the code runs 24 bytes short, and 1112 of ev00b0_script's
 * 1215 words differ */
extern NJS_ACTION eggman_walk_action[];
extern NJS_ACTION eggman_turn_action;

/* @hack type: ev00b0_e102_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps the camera's x in FR13 and its y in FR12 (FMOV.S
 * FR13,FR4 at 0x0cb80462). as one NJS_ACTION passed as &ev00b0_e102_action_0,
 * the two trade, and 12 of ev00b0_script's 1215 words differ */
extern NJS_ACTION ev00b0_e102_action_0[];
extern NJS_ACTION ev00b0_eggman_action_0;
extern NJS_ACTION ev00b0_eggman_action_1;
extern NJS_ACTION ev00b0_eggman_action_2;
extern NJS_ACTION ev00b0_eggman_action_3;
extern NJS_ACTION ev00b0_eggman_action_4;

void ev00b0_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_00b0.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x22d8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0300, 0xbff2, 0);
        event_sound_start(0, 1223, 1800);
        event_noise_start(10, 50, 60, 20);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_set_pos(event_get_player(0), -167.36F, 114.88F, 0.26F);
        event_set_ang(event_get_player(0), 0, 0x3fde, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_create_player(2, eggman_step, -143.89F, 108.75F, 12.23F, 0,
                            0xa6fc, 0);
        event_wait(1);
        event_reset_player(2);
        event_set_pos(event_get_player(2), -146.42F, 108.76F, -0.07F);
        event_set_ang(event_get_player(2), 0, 0xbed8, 0);
        event_wait(120);
        event_sound_stop(0);
        event_voice_play(1672);
        event_message_wait(0, TEXT_1);
        event_wait(5);
        event_sound_start(0, 1224, 1800);
        vibration_start(0, 2, 10, 0);
        event_play_action(event_get_player(2), &ev00b0_eggman_action_0,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(20);
        stream_request_music(MUSIC_THEE102);
        event_wait(20);
        event_play_action(event_get_player(2), &ev00b0_eggman_action_3,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(35);
        event_play_action(event_get_player(0), ev00b0_e102_action_0,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_wait(50);
        event_message_close();
        /* the camera shakes: its x angle moves among 0x0200, 0x0300 and
         * 0x0400, a step or two at a time. pad 0 vibrates with it */
        vibration_start(0, -1, 20, 10);
        event_sound_start(0, 821, 1800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x0400, 0xbff2, 0);
        event_wait(1);
        event_sound_play(822, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x0200, 0xbff2, 0);
        event_wait(1);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x0300, 0xbff2, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x0400, 0xbff2, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x0200, 0xbff2, 0);
        event_sound_play(822, 0, 0, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x0300, 0xbff2, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x0200, 0xbff2, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x0400, 0xbff2, 0);
        event_wait(2);
        event_sound_start(0, 822, 1800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x0200, 0xbff2, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, -152.68F, 122.18F, -0.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x0300, 0xbff2, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -161.77F, 127.83F, 0.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x044d, 0xc0f2, 0);
        event_sound_start(0, 822, 1800);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, -161.77F, 127.83F, 0.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x0752, 0xe74f, 0);
        event_sound_stop(0);
        event_sound_start(0, 822, 1800);
        event_wait(30);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -161.77F, 127.83F, 0.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x0752, 0x9b4f, 0);
        event_sound_stop(0);
        event_sound_start(0, 822, 1800);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, -161.77F, 127.83F, 0.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x044d, 0xc0f2, 0);
        event_sound_stop(0);
        event_sound_start(0, 822, 1800);
        event_wait(30);
        event_voice_play(1673);
        event_message_wait(0, TEXT_2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -161.47F, 126.5F, 5.29F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0, 0xcdf2, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -161.77F, 127.83F, 0.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xf04d, 0xc0f2, 0);
        event_sound_stop(0);
        event_sound_start(0, 822, 1800);
        event_wait(100);
        event_sound_stop(0);
        event_scanlines_kill();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -154.26F, 114.4F, 4.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0x32f2, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -154.81F, 122.37F, 4.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 300, 0xfd00, 0x32f2, 0);
        event_wait(50);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00b0_eggman_action_1,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1674);
        event_wait(10);
        event_message_wait(0, TEXT_3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -157.0F, 120.64F, -8.31F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0x57f2, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -157.43F, 127.62F, -8.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xfd00, 0x57f2, 0);
        event_wait(50);
        event_set_pos(event_get_player(2), -140.31F, 108.74F, -0.07F);
        event_set_ang(event_get_player(2), 0, 0x4053, 0);
        event_run_to_speed(event_get_player(2), -64.2F, 108.77F, 1.95F, 0.3F,
                           0.06F);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -159.19F, 128.89F, 0.52F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xef51, 0xc0f2, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -160.11F, 129.29F, 0.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf500, 0xc0f2, 0);
        event_wait(120);
        event_scanlines_kill();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -157.21F, 130.93F, -5.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0x5256, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -158.91F, 130.71F, 6.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0xfd00, 0x2956, 0);
        event_wait(70);
        event_voice_play(1675);
        event_message_wait(0, TEXT_4);
        event_run_clear(event_get_player(2));
        event_play_action(event_get_player(2), &ev00b0_eggman_action_4,
                          &eggman_texlist, 1.0F, 0, 8);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -160.11F, 129.29F, 0.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfad4, 0xbdf2, 0);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_wait_motions(event_get_player(2));
        event_scanlines_kill();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -154.3F, 117.31F, 0.72F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0x3ef2, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -153.27F, 131.27F, 0.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0300, 0x3ef2, 0);
        event_play_action(event_get_player(0), ev00b0_e102_action_0,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_message_close();
        event_wait(20);
        event_play_action(event_get_player(2), &ev00b0_eggman_action_2,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(20);
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(50);
        event_voice_play(1676);
        event_message_wait(0, TEXT_5);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -114.82F, 126.23F, 0.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0, 0x40f2, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), -102.29F, 108.77F, 6.09F);
        event_set_ang(event_get_player(2), 0, 0xc000, 0);
        event_run_to_speed(event_get_player(2), -140.31F, 108.74F, -0.07F, 0.3F,
                           0.06F);
        event_play_action(event_get_player(2), eggman_walk_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -96.4F, 109.65F, -39.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0x56f2, 0);
        event_wait(28);
        event_message_wait(0, TEXT_6);
        event_voice_play(1677);
        event_wait(20);
        event_play_action(event_get_player(2), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 0);
        event_wait(130);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -111.66F, 108.76F, 24.41F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f00, 0x26f2, 0);
        event_play_action(event_get_player(2), &eggman_turn_action,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -112.72F, 165.43F, -40.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe600, 0x62ef, 0);
        event_clear_motions(event_get_player(2));
        event_set_pos(event_get_player(2), -140.31F, 108.74F, -0.07F);
        event_set_ang(event_get_player(2), 0, 0x6d6b, 0);
        event_play_action(event_get_player(2), eggman_walk_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), -104.46F, 108.76F, -43.14F,
                           0.3F, 0.06F);
        event_wait(100);
        event_clear_motions(event_get_player(2));
        event_clear_motions(event_get_player(0));
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_kill_player(2);
        event_reset_player(0);
        event_noise_kill();
        event_scanlines_kill();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b0_textures[] = {
    {"EGGMAN", &eggman_texlist},
    {NULL, NULL},
};
