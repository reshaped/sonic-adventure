/* EV00BA, E-102's story: in the past, Tikal stops E-102 short of the Chao
 * and tells him of Chaos, who guards them. the screen turns white */
#include <ninja.h>

#include "ev00ba/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/wave.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 39
#define TEXT_3_LENGTH 45
#define TEXT_4_LENGTH 17
#define TEXT_5_LENGTH 54
#define TEXT_6_LENGTH 56
#define TEXT_7_LENGTH 51
#define TEXT_8_LENGTH 64
#define TEXT_9_LENGTH 36
#define TEXT_10_LENGTH 45
#define TEXT_11_LENGTH 2
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 3
#define TEXT_14_LENGTH 54
#define TEXT_15_LENGTH 4
#include "ev00ba/script/text.inc"

/* @hack type: e102_stand_action declared as an array, which script.c passes by
 * its name. SHC then keeps the addresses of ev00ba_chao_5, _6 and _7 in R8, R9
 * and R12 (0x0cb802b4 to 0x0cb802c4). as one NJS_ACTION passed as
 * &e102_stand_action, those registers take ev00ba_chao_4's, _5's and _6's, and
 * 122 of ev00ba_script's 3187 words differ */
extern NJS_ACTION e102_stand_action[];

/* @hack type: ev00ba_chao_action declared as an array, which script.c passes by
 * its name. SHC then keeps the addresses of ev00ba_chao_5, _6, _7 and _9 in R8,
 * R9, R12 and R11 (0x0cb802b4 to 0x0cb802d4). as one NJS_ACTION passed as
 * &ev00ba_chao_action, those registers take ev00ba_chao_15's, _14's, _13's and
 * _0's, the code runs 8 bytes long, and 2818 of ev00ba_script's 3187 words
 * differ */
extern NJS_ACTION ev00ba_chao_action[];
extern NJS_ACTION ev00ba_tikal_action_0;
extern NJS_ACTION ev00ba_tikal_action_1;
extern NJS_ACTION ev00ba_tikal_action_2;
extern NJS_ACTION ev00ba_tikal_action_3;
extern NJS_ACTION ev00ba_tikal_action_4;

static struct task *ev00ba_chao_0 = NULL;
static struct task *ev00ba_chao_1 = NULL;
static struct task *ev00ba_chao_2 = NULL;
static struct task *ev00ba_chao_3 = NULL;
static struct task *ev00ba_chao_4 = NULL;
static struct task *ev00ba_chao_5 = NULL;
static struct task *ev00ba_chao_6 = NULL;
static struct task *ev00ba_chao_7 = NULL;
static struct task *ev00ba_chao_8 = NULL;
static struct task *ev00ba_chao_9 = NULL;
static struct task *ev00ba_chao_10 = NULL;
static struct task *ev00ba_chao_11 = NULL;
static struct task *ev00ba_chao_12 = NULL;
static struct task *ev00ba_chao_13 = NULL;
static struct task *ev00ba_chao_14 = NULL;
static struct task *ev00ba_chao_15 = NULL;
static struct task *ev00ba_fade = NULL;

void ev00ba_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00BA.mlt");
        event_sound_init(9);
        event_create_player(2, tikal_player_step, 0.0F, 0.0F, 511.5F, 0x04f0,
                            0xb350, 0);
        event_place_task(&ev00ba_chao_0, event_chao_create, -123.4F, 0.0F,
                         313.4F, 0, 0, 0);
        event_place_task(&ev00ba_chao_1, event_chao_create, -139.0F, 0.0F,
                         346.6F, 0, 0, 0);
        event_place_task(&ev00ba_chao_2, event_chao_create, -145.0F, 0.0F,
                         363.6F, 0, 0, 0);
        event_place_task(&ev00ba_chao_3, event_chao_create, -135.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_4, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_5, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_6, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_7, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_8, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_9, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_10, event_chao_create, -123.4F, 0.0F,
                         313.4F, 0, 0, 0);
        event_place_task(&ev00ba_chao_11, event_chao_create, -139.0F, 0.0F,
                         346.6F, 0, 0, 0);
        event_place_task(&ev00ba_chao_12, event_chao_create, -145.0F, 0.0F,
                         363.6F, 0, 0, 0);
        event_place_task(&ev00ba_chao_13, event_chao_create, -135.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_14, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_place_task(&ev00ba_chao_15, event_chao_create, -115.0F, 0.0F,
                         358.8F, 0, 0, 0);
        event_wait(1);
        event_set_flags(ev00ba_chao_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_4, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_5, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_6, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_7, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_8, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_9, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_10, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_11, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_12, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_13, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_14, EVENT_IGNORE_GROUND);
        event_set_flags(ev00ba_chao_15, EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(2), -80.0F, 0.0F, 369.5F);
        event_set_ang(event_get_player(2), 0, 0xb200, 0);
        event_set_pos(ev00ba_chao_0, 0.0F, 3.0F, 351.0F);
        event_set_ang(ev00ba_chao_0, 0, 0, 0);
        event_set_pos(ev00ba_chao_1, -14.0F, 1.5F, 360.0F);
        event_set_ang(ev00ba_chao_1, 0, 0xa500, 0);
        event_set_pos(ev00ba_chao_2, 14.0F, 1.5F, 360.0F);
        event_set_ang(ev00ba_chao_2, 0, 0x6500, 0);
        event_set_pos(ev00ba_chao_3, -5.0F, 1.5F, 370.0F);
        event_set_ang(ev00ba_chao_3, 0, 0x8300, 0);
        event_set_pos(ev00ba_chao_4, 5.0F, 1.5F, 370.0F);
        event_set_ang(ev00ba_chao_4, 0, 0x7d00, 0);
        event_set_pos(ev00ba_chao_5, -23.0F, 1.5F, 363.0F);
        event_set_ang(ev00ba_chao_5, 0, 0x8500, 0);
        event_set_pos(ev00ba_chao_6, -13.0F, 1.5F, 370.0F);
        event_set_ang(ev00ba_chao_6, 0, 0x8200, 0);
        event_set_pos(ev00ba_chao_7, 0.0F, 1.5F, 376.0F);
        event_set_ang(ev00ba_chao_7, 0, 0x8000, 0);
        event_set_pos(ev00ba_chao_8, 14.0F, 1.5F, 370.0F);
        event_set_ang(ev00ba_chao_8, 0, 0x7e00, 0);
        event_set_pos(ev00ba_chao_9, 23.0F, 1.5F, 363.0F);
        event_set_ang(ev00ba_chao_9, 0, 0x7b00, 0);
        event_set_pos(ev00ba_chao_10, -18.0F, 1.5F, 347.0F);
        event_set_ang(ev00ba_chao_10, 0, 0xfc00, 0);
        event_set_pos(ev00ba_chao_11, 18.0F, 1.5F, 347.0F);
        event_set_ang(ev00ba_chao_11, 0, 0x0400, 0);
        event_set_pos(ev00ba_chao_12, -20.0F, 1.5F, 380.0F);
        event_set_ang(ev00ba_chao_12, 0, 0x7f00, 0);
        event_set_pos(ev00ba_chao_13, -6.0F, 1.5F, 383.0F);
        event_set_ang(ev00ba_chao_13, 0, 0x8100, 0);
        event_set_pos(ev00ba_chao_14, 20.0F, 1.5F, 380.0F);
        event_set_ang(ev00ba_chao_14, 0, 0x8000, 0);
        event_set_pos(ev00ba_chao_15, 6.0F, 1.5F, 383.0F);
        event_set_ang(ev00ba_chao_15, 0, 0x7f00, 0);
        event_play_action(ev00ba_chao_0, ev00ba_chao_action,
                          &event_chao_texlist, 0.85F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_1, ev00ba_chao_action,
                          &event_chao_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_2, ev00ba_chao_action,
                          &event_chao_texlist, 0.87F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_3, ev00ba_chao_action,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_4, ev00ba_chao_action,
                          &event_chao_texlist, 0.89F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_5, ev00ba_chao_action,
                          &event_chao_texlist, 0.87F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_6, ev00ba_chao_action,
                          &event_chao_texlist, 1.1F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_7, ev00ba_chao_action,
                          &event_chao_texlist, 1.05F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_8, ev00ba_chao_action,
                          &event_chao_texlist, 0.89F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_9, ev00ba_chao_action,
                          &event_chao_texlist, 0.885F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_10, ev00ba_chao_action,
                          &event_chao_texlist, 1.03F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_11, ev00ba_chao_action,
                          &event_chao_texlist, 1.05F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_12, ev00ba_chao_action,
                          &event_chao_texlist, 0.84F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_13, ev00ba_chao_action,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_14, ev00ba_chao_action,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev00ba_chao_15, ev00ba_chao_action,
                          &event_chao_texlist, 1.1F, EVENT_REPEAT, 0);
        event_chao_set_eyes(ev00ba_chao_0, CHAO_EYES_MERRY);
        event_chao_set_eyes(ev00ba_chao_1, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev00ba_chao_2, CHAO_EYES_WORRY);
        event_chao_set_eyes(ev00ba_chao_3, CHAO_EYES_MERRY);
        event_chao_set_eyes(ev00ba_chao_4, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev00ba_chao_5, CHAO_EYES_WORRY);
        event_chao_set_eyes(ev00ba_chao_6, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev00ba_chao_7, CHAO_EYES_ANGRY);
        event_chao_set_eyes(ev00ba_chao_8, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev00ba_chao_9, CHAO_EYES_MERRY);
        event_chao_set_eyes(ev00ba_chao_10, CHAO_EYES_MERRY);
        event_chao_set_eyes(ev00ba_chao_11, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev00ba_chao_12, CHAO_EYES_WORRY);
        event_chao_set_eyes(ev00ba_chao_13, CHAO_EYES_MERRY);
        event_chao_set_eyes(ev00ba_chao_14, CHAO_EYES_WORRY);
        event_chao_set_eyes(ev00ba_chao_15, CHAO_EYES_PLAIN);
        event_set_shadow(ev00ba_chao_0, 0.6F);
        event_set_shadow(ev00ba_chao_1, 0.6F);
        event_set_shadow(ev00ba_chao_2, 0.6F);
        event_set_shadow(ev00ba_chao_3, 0.6F);
        event_set_shadow(ev00ba_chao_4, 0.6F);
        event_set_shadow(ev00ba_chao_5, 0.6F);
        event_set_shadow(ev00ba_chao_6, 0.6F);
        event_set_shadow(ev00ba_chao_7, 0.6F);
        event_set_shadow(ev00ba_chao_8, 0.6F);
        event_set_shadow(ev00ba_chao_9, 0.6F);
        event_set_shadow(ev00ba_chao_10, 0.6F);
        event_set_shadow(ev00ba_chao_11, 0.6F);
        event_set_shadow(ev00ba_chao_12, 0.6F);
        event_set_shadow(ev00ba_chao_13, 0.6F);
        event_set_shadow(ev00ba_chao_14, 0.6F);
        event_set_shadow(ev00ba_chao_15, 0.6F);
        event_reset_player(0);
        event_set_pos(event_get_player(0), 0.0F, -10.9F, 475.9F);
        event_wait(5);
        event_set_ang(event_get_player(0), 0x0500, 0x8000, 0x0045);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -4.8F, -6.0F, 492.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xf400, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, -11.8F, -5.26F, 491.55F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x0700, 0xf800, 0);
        event_sound_play(1226, 0, 70, 0);
        event_sound_play(1227, 0, -20, 0);
        event_wait(25);
        event_sound_play(1226, 10, 80, 0);
        event_sound_play(1226, 0, 60, 0);
        event_wait(20);
        event_sound_play(1227, 20, -20, 0);
        event_wait(25);
        event_sound_play(1227, 0, 80, 0);
        event_wait(15);
        event_sound_play(1226, 20, 30, 0);
        event_sound_play(1227, 10, -80, 0);
        event_wait(15);
        event_sound_play(1227, 0, -90, 0);
        event_sound_play(1226, 10, 80, 0);
        event_sound_play(1227, 0, -20, 0);
        event_wait(25);
        event_sound_play(1226, 0, 80, 0);
        event_sound_play(1226, -20, -40, 0);
        event_wait(20);
        event_sound_play(1227, 0, -20, 0);
        event_wait(25);
        event_sound_play(1227, 10, 70, 0);
        event_wait(15);
        event_sound_play(1226, -20, 30, 0);
        event_sound_play(1227, 0, -60, 0);
        event_wait(15);
        event_sound_play(1227, 20, 0, 0);
        /* @hack spelling: x is negative zero in these two moves, the one pool
         * word of H'80000000 that both read (0x0cb80aca, 0x0cb80aea). as 0.0F,
         * the code runs 12 bytes short, and 2589 of ev00ba_script's 3187 words
         * differ */
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -0.0F, 3.94F, 361.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0800, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -0.0F, 3.5F, 363.2F);
        event_sound_play(1226, 0, 120, 0);
        event_sound_play(1227, 10, -20, 0);
        event_wait(25);
        event_sound_play(1226, 20, -100, 0);
        event_sound_play(1226, 0, 90, 0);
        event_wait(20);
        event_sound_play(1227, -30, -20, 0);
        event_wait(25);
        event_sound_play(1227, 10, 80, 0);
        event_wait(15);
        event_sound_play(1226, -30, 30, 0);
        event_sound_play(1227, 0, -90, 0);
        event_wait(15);
        event_sound_play(1227, -20, 90, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -37.5F, 21.7F, 293.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0x0a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -37.9F, 27.5F, 291.54F);
        event_sound_play(1224, 128, -20, 0);
        event_wave_emitter_create(-73.0F, -0.6F, 223.6F, 0.25F, 1.0F, 6, 18, 1);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -13.2F, 2.18F, 354.17F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0x7b21, 0);
        event_sound_play(1227, 80, -20, 0);
        event_wait(25);
        event_sound_play(1227, 60, 80, 0);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -11.25F, 2.2F, 354.26F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0400, 0x7e21, 0);
        event_sound_play(1226, 30, 120, 0);
        event_sound_play(1227, 40, -20, 0);
        event_wait(25);
        event_sound_play(1226, 50, 80, 0);
        event_sound_play(1226, 50, -100, 0);
        event_wait(20);
        event_sound_play(1227, 40, -20, 0);
        event_wait(25);
        event_sound_play(1227, 20, 80, 0);
        event_wait(15);
        event_sound_play(1226, 30, 30, 0);
        event_sound_play(1227, 30, 80, 0);
        event_wait(25);
        event_sound_play(1227, 40, -90, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -37.5F, 21.7F, 293.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0x0a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -37.9F, 27.5F, 291.54F);
        event_sound_play(1224, 128, 40, 0);
        event_wave_emitter_create(-66.0F, -2.3F, 234.6F, 0.3F, 1.0F, 6, 19, 3);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 13.2F, 2.18F, 354.17F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0x8038, 0);
        event_sound_play(1227, 30, 0, 0);
        event_wait(25);
        event_sound_play(1227, 20, 40, 0);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 11.25F, 2.2F, 354.26F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0400, 0x8038, 0);
        event_sound_play(1226, 20, 120, 0);
        event_sound_play(1227, -30, -20, 0);
        event_wait(25);
        event_sound_play(1226, 40, 0, 0);
        event_sound_play(1227, 80, 90, 0);
        event_wait(20);
        event_sound_play(1227, -20, -20, 0);
        event_wait(25);
        event_sound_play(1227, 30, 80, 0);
        event_wait(15);
        event_sound_play(1226, 40, 100, 0);
        event_sound_play(1226, 50, -100, 0);
        event_wait(25);
        event_sound_play(1227, 60, 90, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -37.5F, 21.7F, 293.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0x0a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -37.9F, 27.5F, 291.54F);
        event_sound_play(1225, 128, -100, 0);
        event_wave_emitter_create(-73.0F, -0.6F, 223.6F, 0.25F, 1.0F, 6, 18, 1);
        event_wait(30);
        event_sound_play(1224, 128, -100, 0);
        event_wave_emitter_create(-73.0F, -0.6F, 223.6F, 0.2F, 1.0F, 7, 22, 1);
        event_sound_play(1223, 128, -100, 0);
        event_wave_emitter_create(-66.0F, -2.3F, 234.6F, 0.3F, 1.0F, 6, 19, 3);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 15.18F, 382.79F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0xfb21, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 0.0F, 3.12F, 382.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0a00, 0, 0);
        event_sound_play(1226, 10, 120, 0);
        event_sound_play(1227, 40, -20, 0);
        event_wait(15);
        event_sound_play(1227, 50, 30, 0);
        event_sound_play(1226, 80, -90, 0);
        event_wait(10);
        event_sound_play(1226, 60, 0, 0);
        event_sound_play(1227, 70, 90, 0);
        event_sound_play(1227, -12, -90, 0);
        event_wait(20);
        event_sound_play(1227, 10, -20, 0);
        event_wait(25);
        event_sound_play(1227, -11, 80, 0);
        event_wait(15);
        event_sound_play(1226, 40, 80, 0);
        event_sound_play(1226, -12, -100, 0);
        event_wait(25);
        event_sound_play(1227, 80, -60, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 0.0F, 0.9F, 390.94F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0a00, 0, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -37.5F, 21.7F, 293.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0x0a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -37.9F, 27.5F, 291.54F);
        event_sound_play(1223, 128, -100, 0);
        event_wave_emitter_create(-75.0F, -0.6F, 220.6F, 0.25F, 1.0F, 6, 18, 1);
        event_wait(30);
        event_sound_play(1224, 110, 60, 0);
        event_wave_emitter_create(-75.0F, -0.6F, 220.6F, 0.2F, 1.0F, 7, 22, 1);
        event_sound_play(1225, 110, -70, 0);
        event_wave_emitter_create(-61.0F, -2.3F, 234.6F, 0.3F, 1.0F, 6, 19, 3);
        event_wait(40);
        event_wait(1);
        event_reset_player(0);
        event_set_pos(event_get_player(2), -40.0F, -4.1F, 433.5F);
        event_set_ang(event_get_player(2), 0x0500, 0x8000, 0);
        event_set_pos(event_get_player(0), 0.0F, -8.9F, 420.0F);
        event_set_ang(event_get_player(0), 0x0500, 0x8000, 0x0045);
        event_run_to_speed(event_get_player(0), 0.0F, 0.0F, 396.32F, 0.4F,
                           0.06F);
        event_set_pos(event_get_player(2), -27.0F, 0.0F, 417.5F);
        event_set_ang(event_get_player(2), 0, 0x6300, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -8.18F, 20.1F, 400.65F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf000, 0xf521, 0x0300);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -9.66F, 26.4F, 405.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xeb00, 0xf521, 0x0300);
        event_sound_play(1226, 40, -90, 0);
        event_wait(20);
        event_sound_play(1227, 60, -20, 0);
        event_wait(25);
        event_sound_play(1227, 40, 80, 0);
        event_wait(15);
        event_sound_play(1226, 20, 100, 0);
        event_sound_play(1227, -30, -100, 0);
        event_wait(25);
        event_sound_play(1226, 80, -60, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.2F, 14.3F, 384.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0x8b21, 0);
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          0.4F, EVENT_REPEAT, 16);
        event_sound_play(1226, 40, 90, 0);
        event_wait(20);
        event_sound_play(1227, 60, -20, 0);
        event_wait(25);
        event_sound_play(1227, 50, 20, 0);
        event_wait(15);
        event_sound_play(1226, 20, 30, 0);
        event_sound_play(1226, 10, -40, 0);
        event_wait(25);
        event_sound_play(1227, -60, -10, 0);
        event_voice_play(1718);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, -1.94F, 14.7F, 385.4F);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, -2.2F, 14.3F, 384.5F);
        event_message_show(TEXT_1);
        event_sound_play(1226, 0, -40, 0);
        event_sound_play(1227, -60, -10, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -19.8F, 2.6F, 427.88F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xfa21, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, -23.8F, 2.6F, 427.3F);
        event_wait(40);
        stream_request_music(MUSIC_TICAL);
        event_wait(70);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -22.58F, 3.7F, 411.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe900, 0x6721, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -21.73F, 6.3F, 410.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xf700, 0x6721, 0);
        event_wait(120);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_11);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_0,
                          &tikal_texlist, 1.0F, 0, 4);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_1,
                          &tikal_texlist, 1.2F, EVENT_REPEAT, 8);
        event_voice_play(1719);
        event_wait(30);
        event_message_show(TEXT_2);
        event_wait(70);
        event_talk_stop(event_get_player(2));
        event_voice_wait();
        event_wait(50);
        event_message_close();
        event_clear_motions(event_get_player(2));
        event_set_pos(event_get_player(2), -15.2F, 0.0F, 397.9F);
        event_set_ang(event_get_player(2), 0x0500, 0x7600, 0);
        event_run_to_speed(event_get_player(2), -13.5F, 0.0F, 391.17F, 0.6F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -14.6F, 8.25F, 376.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x9621, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -17.16F, 8.33F, 377.5F);
        event_run_wait(event_get_player(2));
        event_play_action(event_get_player(2), &ev00ba_tikal_action_1,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(80);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_voice_play(1720);
        event_message_show(TEXT_3);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_sound_play(1226, 20, 100, 0);
        event_sound_play(1227, -30, -100, 0);
        event_wait(25);
        event_sound_play(1226, 0, -30, 0);
        event_wait(20);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_2,
                          &tikal_texlist, 1.2F, 0, 0);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_1,
                          &tikal_texlist, 1.2F, EVENT_REPEAT, 16);
        event_voice_play(1721);
        event_message_show(TEXT_4);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_sound_play(1226, 20, 100, 0);
        event_sound_play(1227, -30, -100, 0);
        event_wait(25);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_sound_play(1226, 0, -10, 0);
        event_wait(20);
        event_sound_play(1227, -50, -60, 0);
        event_wait(15);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -23.15F, 6.87F, 347.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa00, 0x8c21, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, 10.57F, 6.0F, 338.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 300, 0xfb00, 0x7321, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1722);
        event_message_show(TEXT_5);
        event_sound_play(1226, 20, 90, 0);
        event_wait(20);
        event_sound_play(1227, 10, -20, 0);
        event_wait(25);
        event_sound_play(1226, -30, -10, 0);
        event_wait(20);
        event_sound_play(1227, -20, 60, 0);
        event_wait(15);
        event_sound_play(1226, 30, -10, 0);
        event_wait(20);
        event_sound_play(1226, 0, 70, 0);
        event_wait(20);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_sound_play(1227, 30, 20, 0);
        event_sound_play(1226, 10, 40, 0);
        event_wait(20);
        event_sound_play(1227, -20, -60, 0);
        event_wait(15);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1723);
        event_message_show(TEXT_6);
        event_sound_play(1227, -20, 60, 0);
        event_wait(15);
        event_sound_play(1226, 10, -10, 0);
        event_wait(20);
        event_sound_play(1226, -20, 70, 0);
        event_wait(20);
        event_sound_play(1226, 20, 90, 0);
        event_wait(20);
        event_sound_play(1227, 10, -20, 0);
        event_wait(25);
        event_sound_play(1226, 10, -10, 0);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_sound_play(1227, -10, 40, 0);
        event_wait(15);
        event_sound_play(1226, 60, -10, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.3F, 13.3F, 371.25F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfdf9, 0x6f21, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, 1.38F, 10.3F, 370.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x0200, 0x7921, 0);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_3,
                          &tikal_texlist, 1.2F, 0, 0);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_1,
                          &tikal_texlist, 1.2F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1724);
        event_message_show(TEXT_7);
        event_sound_play(1227, -10, 100, 0);
        event_wait(20);
        event_sound_play(1226, 0, 90, 0);
        event_wait(20);
        event_sound_play(1227, 0, -80, 0);
        event_wait(25);
        event_sound_play(1226, -10, -110, 0);
        event_sound_play(1227, -20, 60, 0);
        event_wait(15);
        event_sound_play(1226, 0, -110, 0);
        event_talk_stop(event_get_player(2));
        event_voice_wait();
        event_sound_play(1226, 0, 90, 0);
        event_wait(20);
        event_sound_play(1227, -20, -100, 0);
        event_wait(25);
        event_sound_play(1226, -10, -110, 0);
        event_sound_play(1227, -40, 60, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1.38F, 10.3F, 370.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x7921, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 230, -27.13F, 9.0F, 376.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 230, 0, 0xa021, 0);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_4,
                          &tikal_texlist, 0.7F, 0, 0);
        event_play_action(event_get_player(2), &ev00ba_tikal_action_1,
                          &tikal_texlist, 1.2F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1725);
        event_message_show(TEXT_8);
        event_sound_play(1227, -20, 60, 0);
        event_wait(15);
        event_sound_play(1226, -30, -10, 0);
        event_wait(20);
        event_sound_play(1227, 0, 70, 0);
        event_wait(20);
        event_sound_play(1226, -20, 90, 0);
        event_wait(20);
        event_sound_play(1227, -10, -20, 0);
        event_wait(25);
        event_sound_play(1226, -30, -10, 0);
        event_wait(20);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_sound_play(1227, 10, -20, 0);
        event_wait(25);
        event_sound_play(1226, -30, -10, 0);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -27.13F, 9.0F, 376.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xa021, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -27.2F, 7.1F, 376.28F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1726);
        event_message_show(TEXT_9);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 7.0F, 434.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0xf221, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, -25.57F, 5.1F, 419.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0x0500, 0xf221, 0);
        event_voice_play(1727);
        event_message_show(TEXT_10);
        event_sound_play(1226, 0, 70, 0);
        event_sound_play(1227, 0, -20, 0);
        event_wait(25);
        event_sound_play(1226, 10, 80, 0);
        event_sound_play(1226, 0, 60, 0);
        event_wait(20);
        event_sound_play(1227, 20, -20, 0);
        event_wait(25);
        event_sound_play(1227, 0, 80, 0);
        event_wait(15);
        event_sound_play(1226, 20, 30, 0);
        event_sound_play(1227, 10, -80, 0);
        event_wait(15);
        event_sound_play(1227, 0, -90, 0);
        event_sound_play(1226, 10, 80, 0);
        event_sound_play(1227, 0, -20, 0);
        event_wait(25);
        event_sound_play(1226, 0, 80, 0);
        event_sound_play(1226, -20, -40, 0);
        event_wait(20);
        event_sound_play(1227, 20, -20, 0);
        event_wait(25);
        event_sound_play(1227, 30, 70, 0);
        event_wait(15);
        event_sound_play(1226, 20, 30, 0);
        event_sound_play(1227, 10, -60, 0);
        event_wait(15);
        event_sound_play(1227, 40, 0, 0);
        event_voice_wait();
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.4F, 12.4F, 382.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0x8821, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -1.67F, 13.76F, 386.1F);
        event_sound_play(1226, 0, 80, 0);
        event_sound_play(1226, -20, -40, 0);
        event_wait(20);
        ev00ba_fade = event_fade_create(0.016F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_sound_start(1, SE_TIKAL_CHIME, 90);
        event_wait(25);
        event_sound_play(1226, -10, 80, 0);
        event_sound_play(1226, -30, -40, 0);
        event_wait(10);
        event_sound_play(1227, -30, -20, 0);
        event_wait(55);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev00ba_fade);
        event_kill_task(&ev00ba_chao_0);
        event_kill_task(&ev00ba_chao_1);
        event_kill_task(&ev00ba_chao_2);
        event_kill_task(&ev00ba_chao_3);
        event_kill_task(&ev00ba_chao_4);
        event_kill_task(&ev00ba_chao_5);
        event_kill_task(&ev00ba_chao_6);
        event_kill_task(&ev00ba_chao_7);
        event_kill_task(&ev00ba_chao_8);
        event_kill_task(&ev00ba_chao_9);
        event_kill_task(&ev00ba_chao_10);
        event_kill_task(&ev00ba_chao_11);
        event_kill_task(&ev00ba_chao_12);
        event_kill_task(&ev00ba_chao_13);
        event_kill_task(&ev00ba_chao_14);
        event_kill_task(&ev00ba_chao_15);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00ba_textures[] = {
    {"WAVE7_WA", &event_wave_texlist},
    {"TIKAL", &tikal_texlist},
    {"EV_ALIFE", &event_chao_texlist},
    {NULL, NULL},
};
