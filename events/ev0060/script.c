/* EV0060, the start of Amy's story: Amy misses her days of chasing Sonic in
 * Station Square */
#include <ninja.h>

#include "adv00/texlists.h"
#include "ev0060/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/black.h"
#include "shared/dust.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/move.h"
#include "shared/picture.h"
#include "shared/shapes.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 17
#define TEXT_3_LENGTH 17
#define TEXT_4_LENGTH 23
#define TEXT_5_LENGTH 46
#define TEXT_6_LENGTH 23
#define TEXT_7_LENGTH 36
#define TEXT_8_LENGTH 67
#define TEXT_9_LENGTH 63
#define TEXT_10_LENGTH 25
#define TEXT_11_LENGTH 62
#define TEXT_12_LENGTH 1
#define TEXT_13_LENGTH 2
#define TEXT_14_LENGTH 2
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 2
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 16
#define TEXT_19_LENGTH 4
#define TEXT_20_LENGTH 5
#define TEXT_21_LENGTH 33
#include "ev0060/script/text.inc"

/* sprite slots: the four bombs */
#define BOMB_SLOT_0 0
#define BOMB_SLOT_1 1
#define BOMB_SLOT_2 2
#define BOMB_SLOT_3 3

/* Station Square's people. the models and motions below are ADV00.PRS's.
 * the event plays only while ADV00.PRS is loaded */
extern NJS_OBJECT dat_0cabaec8;
extern NJS_OBJECT dat_0cac8a7c;
extern NJS_OBJECT dat_0cad0ffc;
extern NJS_OBJECT dat_0cae0360;
extern NJS_OBJECT dat_0caee8e4;
extern NJS_MOTION dat_0cb0c214;
extern NJS_MOTION dat_0cb10e24;
extern NJS_MOTION dat_0cb179bc;
extern NJS_MOTION dat_0cb18618;
extern NJS_MOTION dat_0cb218c8;
extern NJS_MOTION dat_0cb27d34;

extern NJS_ACTION ev0060_amy_action_0;
extern NJS_ACTION ev0060_amy_action_1;
extern NJS_ACTION ev0060_amy_action_2;
extern NJS_ACTION ev0060_amy_action_3;
extern NJS_ACTION ev0060_amy_action_4;
extern NJS_ACTION ev0060_amy_action_5;
extern NJS_ACTION ev0060_amy_action_6;
extern NJS_ACTION ev0060_amy_action_7;
extern NJS_ACTION ev0060_amy_action_8;
extern NJS_ACTION ev0060_amy_action_9;
extern NJS_ACTION ev0060_amy_action_10;
extern NJS_ACTION ev0060_sonic_action;
extern NJS_TEXLIST ev0060_bag_texlist;
extern NJS_ACTION ev0060_bag_action_0;
extern NJS_ACTION ev0060_bag_action_1;
extern NJS_ACTION ev0060_bag_action_2;
extern NJS_ACTION ev0060_bag_action_3;
extern NJS_ACTION ev0060_bag_action_4;
extern NJS_ACTION ev0060_bag_action_5;
extern NJS_ACTION ev0060_bag_action_6;
extern NJS_ACTION ev0060_bag_action_7;
extern NJS_ACTION ev0060_bag_action_8;
extern NJS_ACTION ev0060_bag_action_9;
extern NJS_TEXLIST ev0060_picture_texlist;
extern NJS_TEXLIST ev0060_metal_sonic_texlist;
extern NJS_ACTION ev0060_metal_sonic_action;

/* @hack spelling: event_sprite_texlists[EVENT_SPRITE_BOMB] under a name of its
 * own, at the element's address (0x0cbea020). SHC then loads the address from a
 * pool word at each of the four calls that pass it (0x0cb80f62, 0x0cb81068,
 * 0x0cb81214, 0x0cb81266). as &event_sprite_texlists[EVENT_SPRITE_BOMB], it
 * keeps the address in a frame slot for the last two calls instead.
 * ev0060_script then runs 8 bytes long, and 1193 of its 2650 words differ */
/* @hack type: event_sprite_bomb_texlist declared as an array, which script.c
 * passes by its name. SHC then holds the camera's x angle 0x0200 in R9 at
 * 0x0cb81848. as one NJS_TEXLIST passed as &event_sprite_bomb_texlist, R9 holds
 * the y angle 0x5e00 instead, ev0060_script runs 12 bytes long, and 395 of its
 * 2650 words differ */
extern NJS_TEXLIST event_sprite_bomb_texlist[];

static struct event_path_point ev0060_run_points_0[44] = {
#include "ev0060/script/ev0060_run_points_0.inc"
};

static struct event_path ev0060_run_path_0 = {44, ev0060_run_points_0};

static struct event_path_point ev0060_run_points_1[32] = {
#include "ev0060/script/ev0060_run_points_1.inc"
};

static struct event_path ev0060_run_path_1 = {32, ev0060_run_points_1};

static struct event_path_point ev0060_metal_sonic_points[51] = {
#include "ev0060/script/ev0060_metal_sonic_points.inc"
};

static struct event_path ev0060_metal_sonic_path = {51,
                                                    ev0060_metal_sonic_points};

static struct task *ev0060_smoke_0 = NULL;
static struct task *ev0060_smoke_1 = NULL;
static struct task *ev0060_metal_sonic = NULL;
static struct task *ev0060_bag_0 = NULL;
static struct task *ev0060_bag_1 = NULL;
static struct task *ev0060_person_0 = NULL;
static struct task *ev0060_person_1 = NULL;
static struct task *ev0060_person_2 = NULL;
static struct task *ev0060_person_3 = NULL;
static struct task *ev0060_person_4 = NULL;
static struct task *ev0060_person_5 = NULL;
static struct task *ev0060_fade = NULL;
static struct task *ev0060_picture_0 = NULL;
static struct task *ev0060_picture_1 = NULL;
static struct task *ev0060_picture_2 = NULL;
static struct task *ev0060_floor_0 = NULL;
static struct task *ev0060_floor_1 = NULL;

void ev0060_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0060.mlt");
        event_sound_init(5);
        if (ev0060_smoke_0 == NULL) {
            ev0060_smoke_0 = event_dust_create();
        }
        if (ev0060_smoke_1 == NULL) {
            ev0060_smoke_1 = event_dust_create();
        }
        event_set_pos(event_get_player(0), 127.0F, 0.0F, 868.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_look_clear(event_get_player(0));
        event_reset_player(0);
        event_place_actor(&ev0060_metal_sonic, 130.0F, 0.0F, 850.0F, 0, 0, 0);
        event_place_actor(&ev0060_bag_0, 130.0F, 0.0F, 850.0F, 0, 0, 0);
        event_wait(1);
        event_set_flags(ev0060_bag_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev0060_metal_sonic, EVENT_IGNORE_GROUND);
        event_set_pos(ev0060_smoke_0, 2847.0F, 252.0F, 820.6F);
        event_set_pos(ev0060_smoke_1, 2868.0F, 259.0F, 848.0F);
        /* @hack spelling: each dust task's test and its store as one &&
         * expression. each 0.3 then goes through FPUL into FR3 just before its
         * store (0x0cb80188). as ifs, SHC holds 0.3 in FR12 from smoke 0's rate
         * on, and 2341 of ev0060_script's 2650 words differ */
        ev0060_smoke_0 != NULL &&
            (DUST_WORK(ev0060_smoke_0)->params->scale = 6.0F);
        ev0060_smoke_1 != NULL &&
            (DUST_WORK(ev0060_smoke_1)->params->scale = 2.0F);
        ev0060_smoke_0 != NULL &&
            (DUST_WORK(ev0060_smoke_0)->params->rate = 0.3F);
        ev0060_smoke_1 != NULL &&
            (DUST_WORK(ev0060_smoke_1)->params->rate = 0.3F);
        ev0060_smoke_0 != NULL &&
            (DUST_WORK(ev0060_smoke_0)->params->growth = 0.3F);
        /* this repeats smoke 0's growth. no store sets smoke 1's growth, so it
         * keeps event_dust_create's 0.038F */
        ev0060_smoke_0 != NULL &&
            (DUST_WORK(ev0060_smoke_0)->params->growth = 0.3F);
        event_set_pos(ev0060_bag_0, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev0060_bag_0, 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0060_bag_0);
        event_play_action(event_get_player(0), &ev0060_amy_action_1,
                          &amy_texlist, 1.1F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_0, &ev0060_bag_action_0,
                          &ev0060_bag_texlist, 1.1F, EVENT_REPEAT, 0);
        event_create_player(2, sonic_step, 142.0F, 20.0F, 400.0F, 0, 0, 0);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 120.123F, 227.1F, 792.176F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfa00, 0xc100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 210, 106.7F, 7.51F, 792.965F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 210, 0, 0xc100, 0);
        event_place_actor(&ev0060_person_0, 129.7F, 0.0F, 840.78F, 0, 0x8000,
                          0);
        event_place_actor(&ev0060_person_1, 116.0F, 0.0F, 707.0F, 0, 0, 0x8000);
        event_place_actor(&ev0060_person_2, 116.0F, 0.0F, 707.0F, 0, 0, 0x8000);
        event_place_actor(&ev0060_person_3, 412.5F, 0.0F, 929.0F, 0, 0x528c, 0);
        event_place_actor(&ev0060_person_4, 292.7F, 0.0F, 729.7F, 0, 0x3b98, 0);
        event_place_actor(&ev0060_person_5, 292.7F, 0.0F, 729.7F, 0, 0x3b98, 0);
        event_wait(1);
        event_set_flags(ev0060_person_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev0060_person_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev0060_person_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev0060_person_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev0060_person_4, EVENT_IGNORE_GROUND);
        event_set_flags(ev0060_person_5, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_shadow(ev0060_person_0, 1.0F);
        event_set_shadow(ev0060_person_1, 1.0F);
        event_set_shadow(ev0060_person_2, 1.0F);
        event_set_shadow(ev0060_person_3, 1.0F);
        event_set_shadow(ev0060_person_4, 1.0F);
        event_set_shadow(ev0060_person_5, 1.0F);
        event_wait(8);
        event_set_pos(ev0060_person_0, 129.7F, 5.0F, 840.78F);
        event_set_ang(ev0060_person_0, 0, 0x8000, 0);
        event_play_motion(ev0060_person_0, &dat_0cac8a7c, &dat_0cb218c8,
                          &ss_people_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0060_person_1, 116.0F, 0.0F, 707.0F);
        event_set_ang(ev0060_person_1, 0, 0, 0);
        event_play_motion(ev0060_person_1, &dat_0cae0360, &dat_0cb10e24,
                          &ss_people_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0060_person_2, 284.4F, 8.5F, 697.7F);
        event_set_ang(ev0060_person_2, 0, 0x4300, 0);
        event_play_motion(ev0060_person_2, &dat_0caee8e4, &dat_0cb0c214,
                          &ss_people_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0060_person_3, 412.5F, 8.5F, 929.0F);
        event_set_ang(ev0060_person_3, 0, 0x528c, 0);
        event_play_motion(ev0060_person_3, &dat_0caee8e4, &dat_0cb0c214,
                          &ss_people_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0060_person_4, 240.6F, 5.5F, 778.1F);
        event_set_ang(ev0060_person_4, 0, 0xb871, 0);
        event_play_motion(ev0060_person_4, &dat_0cabaec8, &dat_0cb27d34,
                          &ss_people_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0060_person_5, 232.48F, 5.5F, 650.733F);
        event_set_ang(ev0060_person_5, 0, 0x045d, 0);
        event_play_motion(ev0060_person_5, &dat_0cad0ffc, &dat_0cb179bc,
                          &ss_people_texlist, 1.5F, EVENT_REPEAT, 0);
        event_move(ev0060_person_3, 412.5F, 9.0F, 929.0F, 563.67F, 9.0F, 858.0F,
                   350);
        event_wait(180);
        event_wait(10);
        event_move(ev0060_person_1, 116.0F, 10.0F, 757.0F, 116.5F, 10.0F,
                   850.0F, 300);
        event_move(ev0060_person_0, 129.7F, 8.5F, 840.78F, 129.7F, 8.5F,
                   700.78F, 300);
        event_move(ev0060_person_5, 232.48F, 5.5F, 660.733F, 240.0F, 5.5F,
                   763.53F, 193);
        event_wait(10);
        event_move(event_get_player(0), 127.0F, 0.0F, 868.0F, 127.0F, 0.0F,
                   740.0F, 320);
        event_follow(ev0060_bag_0, 0.0F, 0.0F, 0.0F, 320, event_get_player(0));
        event_wait(70);
        stream_request_music(MUSIC_THEAMY);
        event_wait(123);
        event_set_ang(ev0060_person_4, 0, 0x8000, 0);
        event_play_motion(ev0060_person_5, &dat_0cad0ffc, &dat_0cb18618,
                          &ss_people_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 185, 105.4F, 7.536F, 717.8F);
        event_move_stop(ev0060_person_0);
        event_kill_task(&ev0060_person_0);
        event_move_stop(ev0060_person_1);
        event_kill_task(&ev0060_person_1);
        event_move_stop(ev0060_person_3);
        event_kill_task(&ev0060_person_3);
        event_set_pos(event_get_player(2), 271.5F, 3.5F, 273.5F);
        event_wait(127);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0060_amy_action_2,
                          &amy_texlist, 1.0F, 0, 0);
        event_play_action(ev0060_bag_0, &ev0060_bag_action_1,
                          &ev0060_bag_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev0060_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_0, &ev0060_bag_action_2,
                          &ev0060_bag_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(12);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 6, 105.97F, 7.536F, 740.8F);
        event_wait(14);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 123.57F, 16.757F, 735.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xd600, 0x9c00, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 125, 120.4F, 12.04F, 731.126F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 125, 0xe700, 0x9c00, 0xff00);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(1362);
        event_message_show(TEXT_1);
        event_wait(78);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 128.2F, 7.56F, 739.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f00, 0xd900, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0f00, 0xca8f, 0);
        event_voice_play(1363);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_wait(17);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 126.6863F, 7.8F, 738.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0a00, 0x1700, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0a00, 0x2700, 0);
        event_voice_play(1364);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_wait(17);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 118.14F, 4.387F, 734.317F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0500, 0xa700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 105, 121.4836F, 4.387F,
                             732.08F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 105, 0x0500, 0x9a00, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0060_bag_0);
        event_play_action(event_get_player(0), &ev0060_amy_action_4,
                          &amy_texlist, 0.7F, 0, 0);
        event_play_action(ev0060_bag_0, &ev0060_bag_action_3,
                          &ev0060_bag_texlist, 0.7F, 0, 0);
        event_play_action(event_get_player(0), &ev0060_amy_action_5,
                          &amy_texlist, 0.6F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_0, &ev0060_bag_action_4,
                          &ev0060_bag_texlist, 0.6F, EVENT_REPEAT, 0);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(1365);
        event_message_show(TEXT_4);
        event_wait(40);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 122.487F, 4.54F, 734.87F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e00, 0x9f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 95, 124.37F, 4.54F, 734.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 95, 0x0e00, 0x8e00, 0);
        event_talk_start(event_get_player(0), TEXT_15);
        event_voice_play(1368);
        event_message_show(TEXT_5);
        event_wait(20);
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(2));
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(2), EVENT_IGNORE_GROUND);
        event_wait(40);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 75, 123.5753F, 11.15F, 732.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 75, 0x0e00, 0x8e00, 0);
        ev0060_picture_0 = event_picture_create(&ev0060_picture_texlist);
        event_picture_set_blend(ev0060_picture_0, NJD_SOURCE_COLOR,
                                NJD_COLOR_BLENDING_SRCALPHA);
        event_picture_set_blend(ev0060_picture_0, NJD_DESTINATION_COLOR,
                                NJD_COLOR_BLENDING_ONE);
        event_picture_fade(ev0060_picture_0, 1.0F, 60);
        ev0060_picture_1 = event_picture_create(&ev0060_picture_texlist);
        event_picture_set_texture(ev0060_picture_1, 1);
        event_picture_set_depth(ev0060_picture_1, -1.1F);
        event_picture_fade(ev0060_picture_1, 1.0F, 60);
        event_wait(60);
        event_picture_set_depth(ev0060_picture_1, -1000.0F);
        ev0060_floor_0 = event_floor_create(event_get_player(0), 250.0F);
        ev0060_floor_1 = event_floor_create(event_get_player(2), 250.0F);
        event_move_stop(ev0060_bag_0);
        event_kill_task(&ev0060_bag_0);
        event_play_action(event_get_player(0), &ev0060_amy_action_0,
                          &amy_texlist, 2.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev0060_sonic_action,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_16);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff00, 0x2a25, 0xf900);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2868.9F, 257.48F, 850.3285F);
        event_play_path(event_get_player(0), &ev0060_run_path_0, 0.35F, 0);
        event_play_path(event_get_player(2), &ev0060_run_path_0, 0.35F, 0);
        event_voice_play(1369);
        event_message_show(TEXT_6);
        event_play_path(ev0060_metal_sonic, &ev0060_metal_sonic_path, 0.51F, 0);
        event_set_ang(ev0060_metal_sonic, 0, 0x2700, 0x4000);
        event_play_action(ev0060_metal_sonic, &ev0060_metal_sonic_action,
                          &ev0060_metal_sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(22);
        ev0060_picture_2 = event_picture_create(&ev0060_picture_texlist);
        event_picture_set_texture(ev0060_picture_2, 2);
        event_picture_set_depth(ev0060_picture_2, -1001.0F);
        event_picture_fade(ev0060_picture_2, 1.0F, 62);
        event_wait(6);
        if (ev0060_smoke_1 != NULL) {
            DUST_WORK(ev0060_smoke_1)->timer =
                DUST_WORK(ev0060_smoke_1)->interval = 1;
        }
        event_sprite_create(event_get_player(0), BOMB_SLOT_3, 3.0F, 3.0F, 3.0F,
                            event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_3, 0.5F, 0.4F);
        event_wait(2);
        event_set_pos(ev0060_smoke_1, 100.0F, 10.0F, 10.0F);
        event_wait(6);
        event_sprite_kill(BOMB_SLOT_3);
        event_wait(44);
        event_voice_play(1372);
        if (ev0060_smoke_0 != NULL) {
            DUST_WORK(ev0060_smoke_0)->timer =
                DUST_WORK(ev0060_smoke_0)->interval = 1;
        }
        event_sprite_create(NULL, BOMB_SLOT_2, 2846.5F, 252.0F, 820.0F,
                            event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_2, 1.5F, 1.5F);
        event_wait(2);
        event_set_pos(ev0060_smoke_0, 0.0F, 0.0F, 0.0F);
        event_wait(8);
        event_sprite_kill(BOMB_SLOT_2);
        event_wait(10);
        event_clear_motions(ev0060_metal_sonic);
        EVENT_KILL(ev0060_picture_1);
        event_clear_paths(event_get_player(0));
        event_clear_paths(event_get_player(2));
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x1025, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2845.0F, 263.0F, 861.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0300, 0x1025, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 2855.1F, 254.0F, 856.0F);
        event_play_path(event_get_player(0), &ev0060_run_path_1, 0.34F, 0);
        event_play_path(event_get_player(2), &ev0060_run_path_1, 0.34F, 0);
        event_message_show(TEXT_7);
        event_wait(10);
        event_play_path(ev0060_metal_sonic, &ev0060_metal_sonic_path, 0.64F, 0);
        event_set_ang(ev0060_metal_sonic, 0x8000, 0x8000, 0x4000);
        event_play_action(ev0060_metal_sonic, &ev0060_metal_sonic_action,
                          &ev0060_metal_sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0060_smoke_1, 2704.0F, 255.0F, 658.0F);
        event_sprite_create(NULL, BOMB_SLOT_0, 2704.0F, 255.0F, 658.0F,
                            event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_0, 0.5F, 0.5F);
        event_wait(2);
        event_set_pos(ev0060_smoke_1, 0.0F, 10.0F, 0.0F);
        event_wait(6);
        event_sprite_kill(BOMB_SLOT_0);
        event_wait(40);
        event_set_pos(ev0060_smoke_0, 2830.0F, 253.0F, 825.6F);
        event_sprite_create(NULL, BOMB_SLOT_1, 2830.0F, 252.0F, 825.6F,
                            event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_1, 1.5F, 1.5F);
        event_wait(2);
        event_set_pos(ev0060_smoke_0, 10.0F, 10.0F, 10.0F);
        event_wait(6);
        event_sprite_kill(BOMB_SLOT_1);
        event_wait(9);
        event_picture_fade(ev0060_picture_0, 0.0F, 60);
        event_picture_fade(ev0060_picture_2, 0.0F, 60);
        event_reset_player(0);
        EVENT_KILL(ev0060_floor_0);
        EVENT_KILL(ev0060_floor_1);
        event_talk_start(event_get_player(0), TEXT_17);
        event_look_at(event_get_player(0), 127.0F, 11.0F, 720.0F);
        event_place_actor(&ev0060_bag_1, 130.0F, 0.0F, 850.0F, 0, 0, 0);
        event_wait(1);
        event_set_flags(ev0060_bag_1, EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(0), 127.0F, 0.0F, 740.0F);
        event_set_pos(ev0060_bag_1, 127.0F, 0.0F, 740.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_set_ang(ev0060_bag_1, 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 123.5753F, 11.15F, 732.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e00, 0x8e00, 0);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0060_bag_1);
        event_play_action(event_get_player(0), &ev0060_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_2,
                          &ev0060_bag_texlist, 1.0F, EVENT_REPEAT, 0);
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        EVENT_KILL(ev0060_smoke_0);
        EVENT_KILL(ev0060_smoke_1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 127.05F, 5.084F, 732.36F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 55, 0x0a00, 0x7f00, 0);
        event_message_close();
        event_wait(35);
        EVENT_KILL(ev0060_picture_0);
        EVENT_KILL(ev0060_picture_2);
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(5);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0060_bag_1);
        event_play_action(event_get_player(0), &ev0060_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_0,
                          &ev0060_bag_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(0), 127.0F, 0.0F, 740.0F, 127.0F, 0.0F,
                   695.0F, 135);
        event_follow(ev0060_bag_1, 0.0F, 0.0F, 0.0F, 135, event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 124.83F, 9.16F, 751.287F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf300, 0xf500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 127.36F, 8.8F, 701.966F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0xf400, 0xfe00, 0);
        event_kill_player(2);
        event_voice_play(1373);
        event_message_show(TEXT_8);
        event_wait(120);
        event_play_action(event_get_player(0), &ev0060_amy_action_10,
                          &amy_texlist, 1.3F, 0, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_9,
                          &ev0060_bag_texlist, 1.3F, 0, 0);
        event_wait(18);
        event_camera_set_pos(EVENT_EASE_LINEAR, 8, 127.36F, 9.387F, 703.877F);
        event_wait_motions(ev0060_bag_1);
        event_set_pos(event_get_player(0), 127.72F, 0.0F, 695.0F);
        event_set_pos(ev0060_bag_1, 127.72F, 0.0F, 695.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_ang(ev0060_bag_1, 0, 0, 0);
        event_play_action(event_get_player(0), &ev0060_amy_action_2,
                          &amy_texlist, 1.0F, 0, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_1,
                          &ev0060_bag_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev0060_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_2,
                          &ev0060_bag_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1375);
        event_message_show(TEXT_9);
        event_camera_set_fov(EVENT_EASE_LINEAR, 160, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 134.0F, 8.5F, 702.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0xf800, 0x1c00, 0);
        event_wait(20);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), 127.0F, 11.0F, 720.0F);
        event_voice_wait();
        ev0060_fade = event_fade_create(0.0F, 0.4F, 0.0F, 0.0F, 0.0F);
        event_wait(1);
        stream_stop_music();
        event_wait(25);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 140.0F, 6.4F, 701.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x0200, 0x2e00, 0x0200);
        event_wait(16);
        event_voice_play(1377);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_play_action(event_get_player(0), &ev0060_amy_action_5,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_4,
                          &ev0060_bag_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 131.9F, 9.66F, 690.27F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0x5e00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 132.62F, 5.768F, 689.637F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0x0200, 0x5e00, 0);
        event_talk_start(event_get_player(0), TEXT_20);
        event_voice_play(1378);
        event_message_show(TEXT_11);
        event_wait(10);
        event_play_action(event_get_player(0), &ev0060_amy_action_6,
                          &amy_texlist, 0.3F, 0, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_5,
                          &ev0060_bag_texlist, 0.3F, 0, 0);
        event_sound_start(0, 1223, 1800);
        event_sound_fade_volume(0, -40, 1);
        event_play_action(event_get_player(0), &ev0060_amy_action_7,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_6,
                          &ev0060_bag_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_sound_fade_volume(0, 80, 90);
        event_voice_wait();
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_21);
        event_wait(40);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xf300, 0x5e00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 132.6F, 8.75F, 689.67F);
        event_play_action(event_get_player(0), &ev0060_amy_action_8,
                          &amy_texlist, 1.0F, 0, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_7,
                          &ev0060_bag_texlist, 1.0F, 0, 0);
        event_sound_play(1225, 40, 0, 0);
        event_play_action(event_get_player(0), &ev0060_amy_action_9,
                          &amy_texlist, 0.3F, EVENT_REPEAT, 0);
        event_play_action(ev0060_bag_1, &ev0060_bag_action_8,
                          &ev0060_bag_texlist, 0.3F, EVENT_REPEAT, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 65, 203.64F, 40.44F, 625.26F);
        event_wait(40);
        event_sound_stop(0);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_sound_stop_all();
        EVENT_KILL(ev0060_floor_0);
        EVENT_KILL(ev0060_floor_1);
        event_shapes_clear_firm_players(event_get_player(0));
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0060_bag_0);
        event_kill_task(&ev0060_bag_1);
        event_kill_task(&ev0060_person_0);
        event_kill_task(&ev0060_person_1);
        event_kill_task(&ev0060_person_2);
        event_kill_task(&ev0060_person_3);
        event_kill_task(&ev0060_person_4);
        event_kill_task(&ev0060_person_5);
        event_kill_task(&ev0060_metal_sonic);
        event_black_kill();
        EVENT_KILL(ev0060_picture_0);
        EVENT_KILL(ev0060_picture_1);
        EVENT_KILL(ev0060_picture_2);
        event_sprite_kill(BOMB_SLOT_0);
        event_sprite_kill(BOMB_SLOT_1);
        event_sprite_kill(BOMB_SLOT_2);
        event_sprite_kill(BOMB_SLOT_3);
        EVENT_KILL(ev0060_smoke_0);
        EVENT_KILL(ev0060_smoke_1);
        EVENT_KILL(ev0060_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0060_textures[] = {
    {"FPACK", &ev0060_bag_texlist},
    {"EV_S_MSBODY", &ev0060_metal_sonic_texlist},
    {"ICM0060", &ev0060_picture_texlist},
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
