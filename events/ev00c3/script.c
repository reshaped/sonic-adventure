/* EV00C3, E-102's story: E-102 leaves Eggman's service and flies off the Egg
 * Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00c3/script.h"
#include "ev00c3/textures.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/noise.h"
#include "shared/picture.h"
#include "shared/scanlines.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 38
#define TEXT_2_LENGTH 21
#define TEXT_3_LENGTH 11
#include "ev00c3/script/text.inc"

extern NJS_ACTION e102_hover_action;

/* E-102's jump. A is held for all 594 keys. the stick's direction stays near a
 * quarter turn, 59 to 81 in 256ths, and it is pushed 228 to 252 out of 255 */
static const struct event_pad_key ev00c3_jump_keys[594] = {
#include "ev00c3/script/ev00c3_jump_keys.inc"
};

static const struct event_pad_recording ev00c3_jump = {
    ev00c3_jump_keys, sizeof(ev00c3_jump_keys) / sizeof(ev00c3_jump_keys[0])};

static struct event_path_point ev00c3_e102_flight_points[195] = {
#include "ev00c3/script/ev00c3_e102_flight_points.inc"
};

static struct event_path ev00c3_e102_flight = {195, ev00c3_e102_flight_points};

static struct task *ev00c3_picture_0 = NULL;
static struct task *ev00c3_picture_1 = NULL;
static struct task *ev00c3_picture_2 = NULL;
static struct task *ev00c3_picture_3 = NULL;
static struct task *ev00c3_picture_4 = NULL;
static struct task *ev00c3_red = NULL;
static struct task *ev00c3_black = NULL;

void ev00c3_script(int phase)
{
    /* @hack spelling: the scan lines' alpha, 128, through a local, which R8
     * holds from 0x0cb80256 through all six event_scanlines_create calls; as
     * the literal at each call, 658 of 1143 words differ */
    Uint8 alpha;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_noise_create();
        /* a black fade, from alpha 0.1 at a speed of 1 a step. the first
         * step takes the alpha past 1, where it holds, so the fade is opaque
         * at once. the speed of -1/60 set below clears it over 60 steps */
        ev00c3_black = event_fade_create(1.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        sound_load_file("e_00b0.mlt");
        event_sound_init(5);
        event_set_pos(event_get_player(0), 222.82F, 1853.0F, -661.414F);
        event_set_ang(event_get_player(0), 0, 0x0d97, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 253.94F, 1841.0F, -608.45F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0800, 0x3597, 0x0700);
        ev00c3_picture_0 = event_picture_create(&ev00c3_picture_texlist);
        event_picture_set_texture(ev00c3_picture_0, 0);
        event_picture_set_depth(ev00c3_picture_0, -2.14F);
        ev00c3_picture_1 = event_picture_create(&ev00c3_picture_texlist);
        event_picture_set_texture(ev00c3_picture_1, 1);
        event_picture_set_depth(ev00c3_picture_1, -2.13F);
        ev00c3_picture_2 = event_picture_create(&ev00c3_picture_texlist);
        event_picture_set_texture(ev00c3_picture_2, 2);
        event_picture_set_depth(ev00c3_picture_2, -2.12F);
        ev00c3_picture_3 = event_picture_create(&ev00c3_picture_texlist);
        event_picture_set_texture(ev00c3_picture_3, 3);
        event_picture_set_depth(ev00c3_picture_3, -2.11F);
        ev00c3_picture_4 = event_picture_create(&ev00c3_picture_texlist);
        event_picture_set_texture(ev00c3_picture_4, 1);
        event_picture_set_depth(ev00c3_picture_4, -2.1F);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev00c3_jump);
        event_wait(7);
        if (ev00c3_black != NULL) {
            event_fade_set_speed(ev00c3_black, -1.0F / 60.0F);
        }
        event_wait(40);
        event_camera_follow(event_get_player(0));
        event_wait(90);
        event_sound_play(1224, 0, 0, 0);
        /* @hack spelling: alpha set here, after the first sound, so that R8
         * takes 128 at 0x0cb80256, just before the first
         * event_scanlines_create; initialized where it is declared, R8 takes
         * 128 at the function's entry and 801 of 1143 words differ */
        alpha = 128;
        /* @hack redundant: a wait that never runs. it keeps
         * event_picture_create's address in R11 and event_picture_set_depth's
         * in R12, at 0x0cb800de and 0x0cb800f6; without it and its else, the
         * two trade and 12 of 1143 words differ */
        /* @hack redundant: the empty else, which gives event_scanlines_create's
         * address R3 and its fifth argument, 5, R2 from 0x0cb80252; without
         * it, the two trade and 146 of 1143 words differ */
        if (0) {
            event_wait(1);
        } else {
        }
        event_scanlines_create(alpha, 0, 80, 85, 5);
        event_picture_fade(ev00c3_picture_0, 1.0F, 40);
        event_wait(70);
        event_picture_fade(ev00c3_picture_0, 0.0F, 30);
        event_scanlines_kill();
        event_wait(20);
        event_sound_play(1224, 0, 0, 0);
        event_scanlines_create(alpha, 0, 80, 85, 5);
        event_picture_fade(ev00c3_picture_1, 1.0F, 40);
        event_wait(70);
        event_picture_fade(ev00c3_picture_1, 0.0F, 30);
        event_scanlines_kill();
        event_wait(20);
        event_sound_play(1224, 0, 0, 0);
        event_scanlines_create(alpha, 0, 80, 85, 5);
        event_picture_fade(ev00c3_picture_2, 1.0F, 40);
        event_wait(70);
        event_picture_fade(ev00c3_picture_2, 0.0F, 30);
        event_scanlines_kill();
        event_wait(20);
        event_sound_play(1224, 0, 0, 0);
        event_scanlines_create(alpha, 0, 80, 85, 5);
        event_picture_fade(ev00c3_picture_3, 1.0F, 40);
        event_wait(70);
        event_picture_fade(ev00c3_picture_3, 0.0F, 30);
        event_scanlines_kill();
        event_wait(30);
        event_sound_play(1224, 0, 0, 0);
        event_scanlines_create(alpha, 0, 80, 85, 5);
        event_picture_fade(ev00c3_picture_0, 1.0F, 15);
        event_wait(22);
        event_picture_fade(ev00c3_picture_0, 0.0F, 15);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_1, 1.0F, 15);
        event_wait(22);
        event_picture_fade(ev00c3_picture_1, 0.0F, 15);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_2, 1.0F, 15);
        event_wait(22);
        event_picture_fade(ev00c3_picture_2, 0.0F, 15);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_3, 1.0F, 15);
        event_wait(22);
        event_picture_fade(ev00c3_picture_3, 0.0F, 15);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_0, 1.0F, 1);
        event_wait(12);
        event_picture_fade(ev00c3_picture_0, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_1, 1.0F, 1);
        event_wait(12);
        event_picture_fade(ev00c3_picture_1, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_2, 1.0F, 1);
        event_wait(12);
        event_picture_fade(ev00c3_picture_2, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_3, 1.0F, 1);
        event_wait(12);
        event_picture_fade(ev00c3_picture_3, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_0, 1.0F, 1);
        event_wait(7);
        event_picture_fade(ev00c3_picture_0, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_1, 1.0F, 1);
        event_wait(7);
        event_picture_fade(ev00c3_picture_1, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_2, 1.0F, 1);
        event_wait(7);
        event_picture_fade(ev00c3_picture_2, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_3, 1.0F, 1);
        event_wait(7);
        event_picture_fade(ev00c3_picture_3, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_0, 1.0F, 1);
        event_wait(5);
        event_picture_fade(ev00c3_picture_0, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_1, 1.0F, 1);
        event_wait(5);
        event_picture_fade(ev00c3_picture_1, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_2, 1.0F, 1);
        event_wait(5);
        event_picture_fade(ev00c3_picture_2, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_3, 1.0F, 1);
        event_wait(5);
        event_picture_fade(ev00c3_picture_3, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_0, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_0, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_1, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_1, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_2, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_2, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_3, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_3, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_0, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_0, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_1, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_1, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_2, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_2, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_3, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_3, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_0, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_0, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_1, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_1, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_2, 1.0F, 1);
        event_wait(2);
        event_picture_fade(ev00c3_picture_2, 0.0F, 1);
        event_sound_play(1224, 0, 0, 0);
        event_picture_fade(ev00c3_picture_3, 1.0F, 1);
        /* red from alpha 0.1, rising by 1/12 a step. a speed of 0 three
         * steps on holds it, and a speed of -1 later clears it at once */
        ev00c3_red = event_fade_create(1.0F / 12.0F, 0.1F, 1.0F, 0.0F, 0.0F);
        event_wait(3);
        event_sound_play(1225, 60, 0, 0);
        if (ev00c3_red != NULL) {
            event_fade_set_speed(ev00c3_red, 0.0F);
        }
        event_noise_show(56, 150);
        event_wait(57);
        event_scanlines_kill();
        stream_request_music(MUSIC_THEE102);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_camera_follow_clear();
        event_wait(1);
        event_play_path(event_get_player(0), &ev00c3_e102_flight, 0.5F,
                        EVENT_REPEAT);
        event_play_action(event_get_player(0), &e102_hover_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 414.56F, 671.025F, 523.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0xf400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 287.85F, 520.338F,
                             647.14F);
        if (ev00c3_red != NULL) {
            event_fade_set_speed(ev00c3_red, -1.0F);
        }
        event_picture_fade(ev00c3_picture_3, 0.0F, 1);
        event_voice_play(1776);
        event_message_show(TEXT_1);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 282.57F, 508.956F, 656.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xe100, 0x4800, 0xff00);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 205, -0.56F, 200.5658F,
                             852.67F);
        event_wait(20);
        event_message_close();
        event_wait(23);
        event_voice_play(1777);
        event_message_show(TEXT_2);
        event_wait(33);
        event_sound_play(1224, 0, 0, 0);
        event_scanlines_create(alpha, 0, 80, 85, 5);
        event_picture_fade(ev00c3_picture_4, 1.0F, 40);
        event_wait(60);
        event_picture_fade(ev00c3_picture_4, 0.0F, 40);
        event_scanlines_kill();
        event_camera_set_fov(EVENT_EASE_LINEAR, 90, 0x3c72);
        event_voice_play(1778);
        event_message_show(TEXT_3);
        event_message_close();
        event_wait(40);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_set_pos(event_get_player(0), -88.0F, 62.6546F, 894.8F);
        event_set_ang(event_get_player(0), 0xff5e, 0xdd42, 0xffce);
        event_clear_paths(event_get_player(0));
        event_reset_player(0);
        EVENT_KILL(ev00c3_picture_0);
        EVENT_KILL(ev00c3_picture_1);
        EVENT_KILL(ev00c3_picture_2);
        EVENT_KILL(ev00c3_picture_3);
        EVENT_KILL(ev00c3_picture_4);
        EVENT_KILL(ev00c3_red);
        EVENT_KILL(ev00c3_black);
        event_scanlines_kill();
        event_noise_kill();
        event_sound_stop_all();
        event_wait(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00c3_textures[] = {
    {"ICM00C3", &ev00c3_picture_texlist},
    {NULL, NULL},
};
