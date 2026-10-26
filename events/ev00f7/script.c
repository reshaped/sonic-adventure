/* EV00F7: Tikal and the Chao stand in Pachacamac's way at the altar, and the
 * seven emeralds rise and circle above it */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00f7/echidna_normal_textures.h"
#include "ev00f7/emerald_red_action.h"
#include "ev00f7/emerald_red_textures.h"
#include "ev00f7/emerald_textures.h"
#include "ev00f7/script.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/orbit.h"
#include "shared/sound.h"
#include "shared/wave.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 20
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 18
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 65
#define TEXT_6_LENGTH 32
#define TEXT_7_LENGTH 24
#define TEXT_8_LENGTH 46
#define TEXT_9_LENGTH 28
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 47
#define TEXT_12_LENGTH 18
#define TEXT_13_LENGTH 15
#define TEXT_14_LENGTH 1
#define TEXT_15_LENGTH 3
#define TEXT_16_LENGTH 4
#define TEXT_17_LENGTH 4
#define TEXT_18_LENGTH 16
#define TEXT_19_LENGTH 45
#include "ev00f7/script/text.inc"

/* the x where the actors wait between their shots, away from the altar */
#define OFFSTAGE_X (-261.3F)

/* @hack spelling: each echidna motion as (&action)->motion. SHC then pools the
 * action's address, as &ev00f7_echidna_action_2 at 0x0cb80598, and reads the
 * motion 4 bytes past it (MOV.L @(4,R6),R6 at 0x0cb804ac). as
 * action.motion, SHC pools the address of each action's motion field
 * instead. ev00f7_script is then 16 bytes shorter than the image's, and 3077
 * of its 3273 words differ */
#define ACTION_MOTION(action) ((&(action))->motion)

/* @hack spelling: event_sprite_jet_texlist is
 * event_sprite_texlists[EVENT_SPRITE_JET], declared under a name of its own
 * at the element's address. with the name, each event_orbit_add_sprite call
 * loads the address into R5 from the pool word at 0x0cb82474. as
 * &event_sprite_texlists[EVENT_SPRITE_JET], SHC keeps the address in R13
 * across the seven calls, and 2984 of 3273 words differ */
extern NJS_TEXLIST event_sprite_jet_texlist;

extern NJS_ACTION ev00f7_tikal_action_0;
extern NJS_ACTION ev00f7_tikal_action_1;
extern NJS_ACTION ev00f7_tikal_action_2;
extern NJS_ACTION ev00f7_tikal_action_3;
extern NJS_ACTION ev00f7_chao_action_0;
extern NJS_ACTION ev00f7_chao_action_1;
extern NJS_ACTION ev00f7_chao_action_2;
extern NJS_TEXLIST ev00f7_pachacamac_texlist;
extern NJS_ACTION ev00f7_pachacamac_action_0;
extern NJS_ACTION ev00f7_pachacamac_action_1;
extern NJS_ACTION ev00f7_pachacamac_action_2;
extern NJS_ACTION ev00f7_pachacamac_action_3;
extern NJS_ACTION ev00f7_pachacamac_action_4;
extern NJS_ACTION ev00f7_pachacamac_action_5;

extern NJS_ACTION ev00f7_emerald_blue_action;
extern NJS_ACTION ev00f7_emerald_green_action;
extern NJS_ACTION ev00f7_emerald_purple_action;
extern NJS_ACTION ev00f7_emerald_sky_action;
extern NJS_ACTION ev00f7_emerald_white_action;
extern NJS_ACTION ev00f7_emerald_yellow_action;

static struct task *ev00f7_emerald_blue = NULL;
static struct task *ev00f7_emerald_green = NULL;
static struct task *ev00f7_emerald_purple = NULL;
static struct task *ev00f7_emerald_sky = NULL;
static struct task *ev00f7_emerald_white = NULL;
static struct task *ev00f7_emerald_yellow = NULL;
static struct task *ev00f7_emerald_red = NULL;
static struct task *ev00f7_pachacamac = NULL;
static struct task *ev00f7_echidna_normal = NULL;
static struct task *ev00f7_echidna_stout = NULL;
static struct task *ev00f7_echidna_long_0 = NULL;
static struct task *ev00f7_echidna_long_1 = NULL;
static struct task *ev00f7_chao_0 = NULL;
static struct task *ev00f7_chao_1 = NULL;
static struct task *ev00f7_chao_2 = NULL;
static struct task *ev00f7_chao_3 = NULL;
static struct task *ev00f7_flash = NULL;
static struct task *ev00f7_fade = NULL;

void ev00f7_script(int phase)
{
    /* @hack spelling: every clear of motions goes through clear_motions, a
     * local. SHC then keeps event_clear_motions's address in R8
     * (0x0cb80a24). called directly, event_wait's address and
     * event_play_action's trade R12 and R13 (0x0cb80418, 0x0cb80420),
     * ev00f7_script runs 4 bytes short, and 2114 of 3273 words differ */
    void (*clear_motions)(struct task *task);
    /* @hack spelling: the Chao's first action goes through chao_action_0, a
     * local. SHC then keeps its address in R11 for the four Chao
     * (0x0cb804ec). written in place, ev00f7_script runs 12 bytes short, and
     * 3006 of 3273 words differ */
    NJS_ACTION *chao_action_0;
    /* @hack spelling: the Chao's second action goes through chao_action_1, a
     * local. SHC then keeps its address in R14 (0x0cb81d5e). written in
     * place, 305 of 3273 words differ */
    NJS_ACTION *chao_action_1;
    /* @hack spelling: the speed 0.15 goes through speed, a local. SHC then
     * keeps it in FR12 for the three calls that play at it (0x0cb8191c).
     * written in place, ev00f7_script runs 4 bytes long, and 1111 of 3273 words
     * differ */
    Float speed;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_00F7.mlt");
        event_sound_init(5);
        stream_request_music(MUSIC_TICAL);
        event_place_actor(&ev00f7_emerald_blue, -8.63F, 94.55F, 20.32F, 0,
                          0x4000, 0x2000);
        event_place_actor(&ev00f7_emerald_green, 8.63F, 94.55F, 20.32F, 0,
                          0xb000, 0xe000);
        event_place_actor(&ev00f7_emerald_purple, 20.0F, 94.83F, 7.87F, 0,
                          0x2000, 0x2000);
        event_place_actor(&ev00f7_emerald_sky, -20.0F, 94.83F, 7.87F, 0, 0x6000,
                          0xe000);
        event_place_actor(&ev00f7_emerald_white, 18.56F, 94.6F, -6.0F, 0,
                          0x8000, 0x2000);
        event_place_actor(&ev00f7_emerald_yellow, -18.56F, 94.6F, -6.0F, 0,
                          0x4000, 0xe000);
        event_place_actor(&ev00f7_emerald_red, 0.0F, 94.6F, -22.0F, 0, 0x4000,
                          0xe000);
        event_set_pos(event_get_player(0), 0.0F, -71.0F, 797.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_create_player(2, tikal_player_step, OFFSTAGE_X, 3.0F, 455.0F, 0,
                            0, 0);
        event_place_actor(&ev00f7_pachacamac, 0.0F, 6.79F, 401.0F, 0, 0xc000,
                          0);
        event_set_flags(ev00f7_pachacamac, EVENT_IGNORE_GROUND);
        event_set_shadow(ev00f7_pachacamac, 1.0F);
        event_place_task(&ev00f7_chao_0, event_chao_create, OFFSTAGE_X, 3.0F,
                         485.0F, 0, 0x4000, 0);
        event_place_task(&ev00f7_chao_1, event_chao_create, OFFSTAGE_X, 13.0F,
                         485.0F, 0, 0x4000, 0);
        event_place_task(&ev00f7_chao_2, event_chao_create, OFFSTAGE_X, 23.0F,
                         485.0F, 0, 0x4000, 0);
        event_place_task(&ev00f7_chao_3, event_chao_create, OFFSTAGE_X, 33.0F,
                         485.0F, 0, 0x4000, 0);
        event_set_flags(ev00f7_chao_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_chao_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_chao_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_chao_3, EVENT_IGNORE_GROUND);
        event_set_shadow(ev00f7_chao_0, 0.4F);
        event_set_shadow(ev00f7_chao_1, 0.4F);
        event_set_shadow(ev00f7_chao_2, 0.4F);
        event_set_shadow(ev00f7_chao_3, 0.4F);
        event_place_actor(&ev00f7_echidna_stout, 4.8F, 6.0F, 408.74F, 0, 0xc500,
                          0);
        event_place_actor(&ev00f7_echidna_normal, -4.1F, 6.0F, 410.5F, 0,
                          0xc100, 0);
        event_place_actor(&ev00f7_echidna_long_0, -11.0F, 6.0F, 406.3F, 0,
                          0xca00, 0);
        event_place_actor(&ev00f7_echidna_long_1, 12.0F, 6.0F, 403.0F, 0,
                          0xb900, 0);
        event_set_flags(ev00f7_echidna_normal, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_echidna_stout, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_echidna_long_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_echidna_long_1, EVENT_IGNORE_GROUND);
        event_set_shadow(ev00f7_echidna_normal, 1.0F);
        event_set_shadow(ev00f7_echidna_stout, 1.0F);
        event_set_shadow(ev00f7_echidna_long_0, 1.0F);
        event_set_shadow(ev00f7_echidna_long_1, 1.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_play_action(event_get_player(2), &ev00f7_tikal_action_1,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_0,
                          &ev00f7_pachacamac_texlist, 0.7F, EVENT_REPEAT, 0);
        event_set_flags(ev00f7_emerald_blue, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_emerald_green, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_emerald_purple, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_emerald_sky, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_emerald_white, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_emerald_yellow, EVENT_IGNORE_GROUND);
        event_set_flags(ev00f7_emerald_red, EVENT_IGNORE_GROUND);
        event_play_action(ev00f7_echidna_normal, &ev00f7_echidna_action_0,
                          &ev00f7_echidna_normal_texlist, 0.7F, EVENT_REPEAT,
                          0);
        event_play_motion(ev00f7_echidna_stout, &ev00f7_echidna_stout_object,
                          ACTION_MOTION(ev00f7_echidna_action_2),
                          &ev00f7_echidna_stout_texlist, 0.9F, EVENT_REPEAT, 0);
        event_play_motion(ev00f7_echidna_long_0, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_0),
                          &ev00f7_echidna_long_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_motion(ev00f7_echidna_long_1, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_2),
                          &ev00f7_echidna_long_texlist, 0.4F, EVENT_REPEAT, 0);
        chao_action_0 = &ev00f7_chao_action_0;
        event_play_action(ev00f7_chao_0, chao_action_0, &event_chao_texlist,
                          0.2F, EVENT_REPEAT, 4);
        event_play_action(ev00f7_chao_1, chao_action_0, &event_chao_texlist,
                          0.3F, EVENT_REPEAT, 4);
        event_play_action(ev00f7_chao_2, chao_action_0, &event_chao_texlist,
                          0.2F, EVENT_REPEAT, 4);
        event_play_action(ev00f7_chao_3, chao_action_0, &event_chao_texlist,
                          0.3F, EVENT_REPEAT, 4);
        event_chao_set_eyes(ev00f7_chao_0, CHAO_EYES_SING);
        event_chao_set_eyes(ev00f7_chao_1, CHAO_EYES_SING);
        event_chao_set_eyes(ev00f7_chao_2, CHAO_EYES_SING);
        event_chao_set_eyes(ev00f7_chao_3, CHAO_EYES_SING);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 0.0F, 72.3F, 438.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 0.0F, 0.75F, 425.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0f00, 0, 0);
        event_wait(87);
        event_set_pos(ev00f7_emerald_blue, -8.63F, 94.55F, 20.32F);
        event_set_ang(ev00f7_emerald_blue, 0, 0x4000, 0x2000);
        event_set_pos(ev00f7_emerald_green, 8.63F, 94.55F, 20.32F);
        event_set_ang(ev00f7_emerald_green, 0, 0xb000, 0xe000);
        event_set_pos(ev00f7_emerald_purple, 20.0F, 94.83F, 7.87F);
        event_set_ang(ev00f7_emerald_purple, 0, 0x2000, 0x2000);
        event_set_pos(ev00f7_emerald_sky, -20.0F, 94.83F, 7.87F);
        event_set_ang(ev00f7_emerald_sky, 0, 0x6000, 0xe000);
        event_set_pos(ev00f7_emerald_white, 18.56F, 94.6F, -6.0F);
        event_set_ang(ev00f7_emerald_white, 0, 0x8000, 0x2000);
        event_set_pos(ev00f7_emerald_yellow, -18.56F, 94.6F, -6.0F);
        event_set_ang(ev00f7_emerald_yellow, 0, 0x4000, 0xe000);
        event_set_pos(ev00f7_emerald_red, 0.0F, 94.6F, -22.0F);
        event_set_ang(ev00f7_emerald_red, 0, 0x4000, 0xe000);
        event_play_action(ev00f7_emerald_blue, &ev00f7_emerald_blue_action,
                          &ev00f7_emerald_blue_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_emerald_green, &ev00f7_emerald_green_action,
                          &ev00f7_emerald_green_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_emerald_white, &ev00f7_emerald_white_action,
                          &ev00f7_emerald_white_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_emerald_purple, &ev00f7_emerald_purple_action,
                          &ev00f7_emerald_purple_texlist, 1.0F, EVENT_REPEAT,
                          0);
        event_play_action(ev00f7_emerald_sky, &ev00f7_emerald_sky_action,
                          &ev00f7_emerald_sky_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_emerald_yellow, &ev00f7_emerald_yellow_action,
                          &ev00f7_emerald_yellow_texlist, 1.0F, EVENT_REPEAT,
                          0);
        event_play_action(ev00f7_emerald_red, &ev00f7_emerald_red_action,
                          &ev00f7_emerald_red_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 0.0F, 0.39F, 428.44F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x1100, 0, 0);
        event_wait(5);
        event_voice_play(1930);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_wait(10);
        event_voice_play(1950);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_set_pos(event_get_player(2), 0.0F, 0.0F, 381.0F);
        event_set_pos(ev00f7_chao_0, -2.6F, 3.0F, 372.0F);
        event_set_pos(ev00f7_chao_1, -1.0F, 2.0F, 376.0F);
        event_set_pos(ev00f7_chao_2, 3.0F, 4.0F, 370.0F);
        event_set_pos(ev00f7_chao_3, 1.0F, 3.0F, 379.0F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_set_ang(ev00f7_chao_0, 0, 0, 0);
        event_set_ang(ev00f7_chao_1, 0, 0, 0);
        event_set_ang(ev00f7_chao_2, 0, 0, 0);
        event_set_ang(ev00f7_chao_3, 0, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2000);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 3.9F, 8.3F, 408.66F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0x0600, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 3.9F, 4.3F, 408.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0200, 0x0a00, 0);
        event_voice_play(1931);
        event_message_show(TEXT_3);
        event_wait(1);
        clear_motions = event_clear_motions;
        clear_motions(ev00f7_pachacamac);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_3,
                          &ev00f7_pachacamac_texlist, 1.0F, 0, 0);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_4,
                          &ev00f7_pachacamac_texlist, 1.5F, EVENT_REPEAT, 4);
        event_voice_wait();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x4000);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.6577F, 1.0F, 392.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f00, 0xf000, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 18, -3.5F, 3.2F, 386.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0x1200, 0xf000, 0xfd00);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 15, 0x2aab);
        event_voice_play(1953);
        event_talk_start(event_get_player(2), TEXT_14);
        event_message_show(TEXT_4);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -3.14F, 3.56F, 386.04F);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_set_pos(ev00f7_chao_0, OFFSTAGE_X, 3.0F, 485.0F);
        event_set_pos(ev00f7_chao_1, OFFSTAGE_X, 13.0F, 485.0F);
        event_set_pos(ev00f7_chao_2, OFFSTAGE_X, 23.0F, 485.0F);
        event_set_pos(ev00f7_chao_3, OFFSTAGE_X, 33.0F, 485.0F);
        event_play_action(event_get_player(2), &ev00f7_tikal_action_1,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_1,
                          &ev00f7_pachacamac_texlist, 0.6F, 0, 4);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_0,
                          &ev00f7_pachacamac_texlist, 0.7F, 0, 4);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_1,
                          &ev00f7_pachacamac_texlist, 0.6F, 0, 4);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_4,
                          &ev00f7_pachacamac_texlist, 1.0F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 4.12F, 5.72F, 372.07F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x01c0, 0x7c00, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 190, 0x2eef);
        event_camera_set_pos(EVENT_EASE_LINEAR, 190, -5.984F, 6.83F, 375.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 190, 0xfc00, 0x8e00, 0);
        event_voice_play(1932);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -6.35F, 7.3F, 376.428F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0xfd2b, 0x8c50, 0x0014);
        event_wait(5);
        event_voice_play(1933);
        event_message_show(TEXT_6);
        event_wait(35);
        clear_motions(ev00f7_pachacamac);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_3,
                          &ev00f7_pachacamac_texlist, 0.8F, 0, 12);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_4,
                          &ev00f7_pachacamac_texlist, 1.0F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -5.42F, 3.3F, 386.24F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0b00, 0x9000, 0x0100);
        event_camera_set_fov(EVENT_EASE_LINEAR, 40, 0x238e);
        event_voice_wait();
        event_wait(10);
        event_set_pos(event_get_player(2), 0.0F, 0.0F, 381.0F);
        event_set_pos(ev00f7_chao_0, -2.6F, 3.0F, 372.0F);
        event_set_pos(ev00f7_chao_1, -1.0F, 2.0F, 376.0F);
        event_set_pos(ev00f7_chao_2, 8.0F, 4.0F, 370.0F);
        event_set_pos(ev00f7_chao_3, 5.0F, 3.0F, 379.0F);
        event_set_pos(ev00f7_echidna_normal, OFFSTAGE_X, 3.0F, 485.0F);
        event_set_pos(ev00f7_echidna_stout, OFFSTAGE_X, 13.0F, 485.0F);
        event_set_pos(ev00f7_echidna_long_0, OFFSTAGE_X, 23.0F, 485.0F);
        event_set_pos(ev00f7_echidna_long_1, OFFSTAGE_X, 33.0F, 485.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -15.7F, 1.0F, 383.36F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c00, 0xd200, 0x0400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -13.2F, 1.0F, 388.8F);
        event_voice_play(1954);
        event_talk_start(event_get_player(2), TEXT_15);
        event_message_show(TEXT_7);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -15.67F, 1.0F, 402.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0c70, 0xc900, 0x0400);
        event_voice_play(1955);
        event_talk_start(event_get_player(2), TEXT_16);
        event_message_show(TEXT_8);
        event_set_pos(ev00f7_echidna_stout, 16.0F, 6.0F, 423.0F);
        event_set_pos(ev00f7_echidna_normal, -10.1F, 6.0F, 422.5F);
        event_set_pos(ev00f7_echidna_long_0, -8.0F, 6.0F, 429.3F);
        event_set_pos(ev00f7_echidna_long_1, 12.0F, 6.0F, 423.0F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -15.1F, 1.0F, 405.5F);
        event_wait(80);
        event_play_action(event_get_player(2), &ev00f7_tikal_action_0,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 9);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -4.06F, 9.575F, 391.785F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf300, 0xf200, 0x0400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, -3.1F, 8.634F, 389.1F);
        event_voice_play(1956);
        event_talk_start(event_get_player(2), TEXT_17);
        event_message_show(TEXT_9);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -2.78F, 8.32F, 388.2F);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(5);
        event_set_pos(ev00f7_echidna_stout, 16.0F, 6.0F, 403.0F);
        event_set_pos(ev00f7_echidna_normal, -10.1F, 6.0F, 402.5F);
        event_set_pos(ev00f7_echidna_long_0, -8.0F, 6.0F, 409.3F);
        event_set_pos(ev00f7_echidna_long_1, 12.0F, 6.0F, 403.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.36F, 5.56F, 394.745F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x9b00, 0x0300);
        clear_motions(ev00f7_pachacamac);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_2,
                          &ev00f7_pachacamac_texlist, 0.5F, 0, 4);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_4,
                          &ev00f7_pachacamac_texlist, 1.0F, EVENT_REPEAT, 4);
        event_voice_play(1934);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_wait(5);
        event_play_action(ev00f7_echidna_normal, &ev00f7_echidna_action_0,
                          &ev00f7_echidna_normal_texlist, 0.3F, EVENT_REPEAT,
                          0);
        event_play_motion(ev00f7_echidna_stout, &ev00f7_echidna_stout_object,
                          ACTION_MOTION(ev00f7_echidna_action_2),
                          &ev00f7_echidna_stout_texlist, 0.3F, EVENT_REPEAT, 0);
        event_play_motion(ev00f7_echidna_long_0, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_2),
                          &ev00f7_echidna_long_texlist, 0.6F, EVENT_REPEAT, 0);
        event_play_motion(ev00f7_echidna_long_1, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_0),
                          &ev00f7_echidna_long_texlist, 0.7F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, -10.67F, 0.29F, 385.375F);
        event_voice_play(1935);
        event_message_show(TEXT_11);
        event_wait(25);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_3,
                          &ev00f7_pachacamac_texlist, 0.9F, 0, 4);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_4,
                          &ev00f7_pachacamac_texlist, 0.5F, EVENT_REPEAT, 4);
        event_voice_wait();
        event_voice_play(1936);
        event_message_show(TEXT_12);
        event_wait(1);
        clear_motions(ev00f7_pachacamac);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_5,
                          &ev00f7_pachacamac_texlist, 0.6F, 0, 4);
        event_play_action(ev00f7_pachacamac, &ev00f7_pachacamac_action_4,
                          &ev00f7_pachacamac_texlist, 0.5F, EVENT_REPEAT, 4);
        event_voice_wait();
        event_set_pos(event_get_player(2), OFFSTAGE_X, 3.0F, 455.0F);
        event_set_pos(ev00f7_chao_0, OFFSTAGE_X, 3.0F, 485.0F);
        event_set_pos(ev00f7_chao_1, OFFSTAGE_X, 13.0F, 485.0F);
        event_set_pos(ev00f7_chao_2, OFFSTAGE_X, 23.0F, 485.0F);
        event_set_pos(ev00f7_chao_3, OFFSTAGE_X, 33.0F, 485.0F);
        event_play_action(event_get_player(2), &ev00f7_tikal_action_1,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 0);
        event_sound_start(0, 1226, 1800);
        event_sound_fade_volume(0, 120, 1);
        event_sound_start(1, 1227, 1800);
        event_sound_fade_volume(1, 120, 1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 30, 0x4e39);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -5.2F, 6.35F, 375.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0800, 0x8a00, 0);
        clear_motions(ev00f7_echidna_normal);
        clear_motions(ev00f7_echidna_stout);
        clear_motions(ev00f7_echidna_long_0);
        clear_motions(ev00f7_echidna_long_1);
        event_play_action(ev00f7_echidna_normal, &ev00f7_echidna_action_1,
                          &ev00f7_echidna_normal_texlist, 1.0F, EVENT_REPEAT,
                          0);
        event_play_motion(ev00f7_echidna_stout, &ev00f7_echidna_stout_object,
                          ACTION_MOTION(ev00f7_echidna_action_1),
                          &ev00f7_echidna_stout_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_motion(ev00f7_echidna_long_0, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_1),
                          &ev00f7_echidna_long_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_motion(ev00f7_echidna_long_1, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_1),
                          &ev00f7_echidna_long_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev00f7_echidna_long_1, 12.0F, 6.0F, 403.0F, 4.244F, 6.0F,
                   364.33F, 39);
        event_move(ev00f7_echidna_normal, -10.1F, 6.0F, 402.5F, -3.58F, 6.0F,
                   364.4F, 40);
        event_wait(4);
        event_move(ev00f7_echidna_stout, 16.0F, 6.0F, 403.0F, 9.0F, 6.0F,
                   361.0F, 41);
        event_move(ev00f7_echidna_long_0, -8.0F, 6.0F, 409.3F, -3.0F, 6.0F,
                   361.33F, 41);
        event_wait(24);
        event_move_stop_all();
        event_set_pos(ev00f7_pachacamac, OFFSTAGE_X, 13.0F, 455.0F);
        event_set_pos(ev00f7_echidna_normal, OFFSTAGE_X, 3.0F, 485.0F);
        event_set_pos(ev00f7_echidna_stout, OFFSTAGE_X, 13.0F, 485.0F);
        event_set_pos(ev00f7_echidna_long_0, OFFSTAGE_X, 23.0F, 485.0F);
        event_set_pos(ev00f7_echidna_long_1, OFFSTAGE_X, 33.0F, 485.0F);
        event_set_pos(event_get_player(2), 0.0F, 0.0F, 381.0F);
        event_set_pos(ev00f7_chao_0, -2.6F, 3.0F, 372.0F);
        event_set_pos(ev00f7_chao_1, -1.0F, 2.0F, 376.0F);
        event_set_pos(ev00f7_chao_2, 5.0F, 4.0F, 370.0F);
        event_set_pos(ev00f7_chao_3, 1.0F, 3.0F, 379.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 0.0F, 5.5F, 421.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 40, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 0.0F, 5.5F, 391.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0200, 0, 0);
        clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00f7_tikal_action_2,
                          &tikal_texlist, 0.6F, EVENT_REPEAT, 4);
        stream_stop_music();
        event_voice_play(1957);
        event_talk_start(event_get_player(2), TEXT_18);
        event_message_show(TEXT_13);
        event_sound_stop(0);
        event_sound_stop(1);
        event_message_close();
        ev00f7_flash = event_fade_create(1.0F / 7.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_play_action(event_get_player(2), &ev00f7_tikal_action_1,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(2));
        event_set_pos(ev00f7_chao_0, OFFSTAGE_X, 3.0F, 485.0F);
        event_set_pos(ev00f7_chao_1, OFFSTAGE_X, 13.0F, 485.0F);
        event_set_pos(ev00f7_chao_2, OFFSTAGE_X, 23.0F, 485.0F);
        event_set_pos(ev00f7_chao_3, OFFSTAGE_X, 33.0F, 485.0F);
        event_wait(8);
        clear_motions(ev00f7_echidna_normal);
        clear_motions(ev00f7_echidna_stout);
        clear_motions(ev00f7_echidna_long_0);
        clear_motions(ev00f7_echidna_long_1);
        event_play_action(ev00f7_echidna_normal, &ev00f7_echidna_action_1,
                          &ev00f7_echidna_normal_texlist, 0.09F, EVENT_REPEAT,
                          0);
        event_play_motion(ev00f7_echidna_stout, &ev00f7_echidna_stout_object,
                          ACTION_MOTION(ev00f7_echidna_action_1),
                          &ev00f7_echidna_stout_texlist, 0.1F, EVENT_REPEAT, 0);
        speed = 0.15F;
        event_play_motion(ev00f7_echidna_long_0, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_1),
                          &ev00f7_echidna_long_texlist, speed, EVENT_REPEAT, 0);
        event_play_motion(ev00f7_echidna_long_1, &ev00f7_echidna_long_object,
                          ACTION_MOTION(ev00f7_echidna_action_1),
                          &ev00f7_echidna_long_texlist, 0.08F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x18e4);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -16.07F, 4.45F, 366.23F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0xa400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 290, -15.2F, 3.2F, 362.35F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 290, 0x0100, 0xa100, 0);
        event_move(ev00f7_echidna_long_1, 4.0F, 5.8F, 450.0F, 4.0F, 5.8F,
                   330.0F, 420);
        event_move(ev00f7_echidna_normal, 10.1F, 5.8F, 395.5F, 10.1F, 5.8F,
                   330.0F, 350);
        event_move(ev00f7_echidna_stout, -6.0F, 5.8F, 390.0F, -6.0F, 5.8F,
                   320.0F, 350);
        event_move(ev00f7_echidna_long_0, 13.0F, 5.8F, 410.0F, 13.0F, 5.8F,
                   350.0F, 350);
        event_sound_play(1228, 0, 0, 0);
        clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00f7_tikal_action_3,
                          &tikal_texlist, 0.2F, 0, 0);
        event_talk_start(event_get_player(2), TEXT_19);
        event_wait(7);
        stream_request_music(MUSIC_EVTBGM04);
        if (ev00f7_flash != NULL) {
            event_fade_set_speed(ev00f7_flash, -0.25F);
        }
        event_wait(2);
        if (ev00f7_flash != NULL) {
            event_fade_set_speed(ev00f7_flash, 0.0F);
        }
        event_wait(235);
        event_sound_play(1228, 0, 0, 0);
        event_wait(20);
        event_wave_emitter_create(-203.8F, 0.0F, 18.8F, 0.25F, 1.5F, 6, 18, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -184.32F, 30.58F, 71.63F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe305, 0x0200, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_wait(55);
        event_move(ev00f7_chao_0, 6.8F, 0.0F, 327.0F, -1.38F, 0.0F, 321.08F,
                   200);
        event_move(ev00f7_chao_2, 0.0F, 1.7F, 332.0F, 0.0F, 0.0F, 321.0F, 200);
        /* @bug logic: this Chao's move heads for a z of 3170 (FMOV.S @R0,FR9
         * at 0x0cb81c60, of the float at 0x0cb81d2c), 2839 from its start,
         * where the two Chao moved above head for points 10 and 11 from
         * theirs. 317, 14 from its start, reads as the z meant, an inference
         * from those two moves */
        event_move(ev00f7_chao_3, 18.3F, 0.0F, 331.2F, 18.3F, 0.0F, 3170.0F,
                   200);
        event_set_pos(ev00f7_chao_1, 12.35F, 0.66F, 335.13F);
        event_set_pos(ev00f7_chao_3, 18.3F, 0.0F, 331.2F);
        clear_motions(ev00f7_chao_0);
        clear_motions(ev00f7_chao_1);
        clear_motions(ev00f7_chao_2);
        clear_motions(ev00f7_chao_3);
        event_play_action(ev00f7_chao_0, &ev00f7_chao_action_2,
                          &event_chao_texlist, 0.13F, EVENT_REPEAT, 0);
        chao_action_1 = &ev00f7_chao_action_1;
        event_play_action(ev00f7_chao_1, chao_action_1, &event_chao_texlist,
                          0.18F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_chao_2, chao_action_1, &event_chao_texlist,
                          0.13F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_chao_3, &ev00f7_chao_action_2,
                          &event_chao_texlist, 0.18F, EVENT_REPEAT, 0);
        event_wait(5);
        event_sound_play(1229, 0, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -2.126F, 0.29F, 321.713F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0x9f00, 0x0900);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -3.62F, 0.7F, 323.0F);
        event_wave_emitter_create(-203.8F, 0.0F, 18.8F, 0.4F, 1.5F, 6, 19, 3);
        event_wait(20);
        event_sound_play(1229, 0, 0, 0);
        event_wait(50);
        event_sound_stop(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -184.32F, 30.58F, 71.63F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe305, 0x0200, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_wait(5);
        event_wave_emitter_create(-185.0F, 0.0F, 56.65F, 0.4F, 1.5F, 6, 19, 3);
        event_wait(60);
        event_move_stop_all();
        event_move(ev00f7_echidna_long_1, 4.0F, 5.8F, 330.0F, 4.0F, 5.8F,
                   310.0F, 120);
        event_move(ev00f7_echidna_normal, 10.1F, 5.8F, 330.0F, 10.1F, 5.8F,
                   290.0F, 120);
        event_move(ev00f7_echidna_long_0, 12.0F, 5.8F, 322.0F, 12.0F, 5.8F,
                   260.0F, 120);
        event_move(ev00f7_chao_1, 13.85F, 0.0F, 327.36F, 13.85F, 0.0F, 320.0F,
                   120);
        event_move(ev00f7_chao_2, 8.0F, 1.7F, 324.0F, 8.0F, 0.0F, 300.0F, 150);
        clear_motions(ev00f7_chao_0);
        clear_motions(ev00f7_chao_1);
        clear_motions(ev00f7_chao_2);
        clear_motions(ev00f7_chao_3);
        event_play_action(ev00f7_chao_0, &ev00f7_chao_action_2,
                          &event_chao_texlist, 0.1F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_chao_1, chao_action_1, &event_chao_texlist,
                          speed, EVENT_REPEAT, 0);
        event_play_action(ev00f7_chao_2, chao_action_1, &event_chao_texlist,
                          0.1F, EVENT_REPEAT, 0);
        event_play_action(ev00f7_chao_3, &ev00f7_chao_action_2,
                          &event_chao_texlist, speed, EVENT_REPEAT, 0);
        event_wait(5);
        event_sound_play(1229, 0, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 11.4F, 1.5F, 334.15F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0800, 0x1400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 16.7F, 1.46F, 331.3F);
        event_wave_emitter_kill(0);
        event_wave_emitter_kill(1);
        event_wait(20);
        event_wave_emitter_create(-203.8F, 0.0F, 18.8F, 0.35F, 1.5F, 6, 18, 1);
        event_wave_emitter_create(-167.0F, 0.0F, 23.0F, 0.4F, 1.5F, 6, 19, 3);
        event_sound_play(1229, 0, 0, 0);
        event_wait(5);
        event_sound_play(1229, 0, 0, 0);
        event_wait(35);
        event_sound_stop(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -184.32F, 30.58F, 71.63F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe305, 0x0200, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_wait(5);
        event_wave_emitter_create(-185.0F, 0.0F, 56.65F, 0.4F, 1.5F, 7, 22, 1);
        event_wait(70);
        event_wave_emitter_kill(0);
        event_wave_emitter_kill(1);
        event_wave_emitter_kill(2);
        if (ev00f7_flash != NULL) {
            event_fade_set_speed(ev00f7_flash, -1.0F);
        }
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 93.0F, 68.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0800, 0, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x1200, 0, 0);
        event_move(ev00f7_emerald_blue, -8.63F, 94.55F, 20.32F, -8.63F, 120.0F,
                   20.32F, 70);
        event_move(ev00f7_emerald_green, 8.63F, 94.55F, 20.32F, 8.63F, 120.0F,
                   20.32F, 70);
        event_move(ev00f7_emerald_purple, 20.0F, 94.83F, 7.87F, 20.0F, 120.0F,
                   7.87F, 70);
        event_move(ev00f7_emerald_sky, -20.0F, 94.83F, 7.87F, -20.0F, 120.0F,
                   7.87F, 70);
        event_move(ev00f7_emerald_white, 18.56F, 94.6F, -6.0F, 18.56F, 120.0F,
                   -6.0F, 70);
        event_move(ev00f7_emerald_yellow, -18.56F, 94.6F, -6.0F, -18.56F,
                   120.0F, -6.0F, 70);
        event_move(ev00f7_emerald_red, 0.0F, 94.6F, -22.0F, 0.0F, 120.0F,
                   -22.0F, 70);
        event_wait(70);
        event_orbit_create(0.0F, 120.0F, 0.0F, 30.0F, 182, ev00f7_emerald_blue,
                           ev00f7_emerald_green, ev00f7_emerald_purple,
                           ev00f7_emerald_sky, ev00f7_emerald_white,
                           ev00f7_emerald_yellow, ev00f7_emerald_red);
        event_orbit_add_sprite(1, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.0F,
                               0.0F, 1.0F);
        event_orbit_add_sprite(2, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.0F,
                               1.0F, 0.0F);
        event_orbit_add_sprite(3, &event_sprite_jet_texlist, 0.4F, 0.7F, 1.0F,
                               0.0F, 1.0F);
        event_orbit_add_sprite(4, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.5F,
                               0.5F, 1.0F);
        event_orbit_add_sprite(5, &event_sprite_jet_texlist, 0.4F, 0.7F, 1.0F,
                               1.0F, 1.0F);
        event_orbit_add_sprite(6, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.3F,
                               1.0F, 0.5F);
        event_orbit_add_sprite(7, &event_sprite_jet_texlist, 0.4F, 0.7F, 1.0F,
                               0.0F, 0.0F);
        event_wait(125);
        stream_stop_music();
        ev00f7_fade = event_fade_create(0.1F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_sound_start(2, 1230, 1800);
        event_sound_fade_volume(2, 127, 1);
        event_wait(20);
        event_sound_stop(2);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev00f7_pachacamac);
        event_orbit_kill();
        event_kill_task(&ev00f7_emerald_blue);
        event_kill_task(&ev00f7_emerald_red);
        event_kill_task(&ev00f7_emerald_yellow);
        event_kill_task(&ev00f7_emerald_sky);
        event_kill_task(&ev00f7_emerald_green);
        event_kill_task(&ev00f7_emerald_white);
        event_kill_task(&ev00f7_emerald_purple);
        event_sound_stop_all();
        event_kill_task(&ev00f7_echidna_normal);
        event_kill_task(&ev00f7_echidna_stout);
        event_kill_task(&ev00f7_echidna_long_0);
        event_kill_task(&ev00f7_echidna_long_1);
        event_kill_task(&ev00f7_chao_0);
        event_kill_task(&ev00f7_chao_1);
        event_kill_task(&ev00f7_chao_2);
        event_kill_task(&ev00f7_chao_3);
        EVENT_KILL(ev00f7_flash);
        EVENT_KILL(ev00f7_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f7_textures[] = {
    {"EV_K_PATYA", &ev00f7_pachacamac_texlist},
    {"KNUCKLES_NORMAL", &ev00f7_echidna_normal_texlist},
    {"KNUCKLES_LONG", &ev00f7_echidna_long_texlist},
    {"KNUCKLES_DEBU", &ev00f7_echidna_stout_texlist},
    {"M_EM_BLUE", &ev00f7_emerald_blue_texlist},
    {"M_EM_GREEN", &ev00f7_emerald_green_texlist},
    {"M_EM_PURPLE", &ev00f7_emerald_purple_texlist},
    {"M_EM_SKY", &ev00f7_emerald_sky_texlist},
    {"M_EM_RED", &ev00f7_emerald_red_texlist},
    {"M_EM_WHITE", &ev00f7_emerald_white_texlist},
    {"M_EM_YELLOW", &ev00f7_emerald_yellow_texlist},
    {"EV_ALIFE", &event_chao_texlist},
    {"TIKAL", &tikal_texlist},
    {"WAVE7_WA", &event_wave_texlist},
    {NULL, NULL},
};
