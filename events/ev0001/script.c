/* EV0001: Sonic's story opens in Station Square. Sonic watches patrol cars race
 * by from high above the street. he jumps down and runs off */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/texlists.h"
#include "adv00/traffic.h"
#include "ev0001/helicopter_textures.h"
#include "ev0001/patrol_car_action.h"
#include "ev0001/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/light.h"
#include "shared/move.h"
#include "shared/picture.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 22
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 3
#define TEXT_4_LENGTH 1
#define TEXT_5_LENGTH 1
#include "ev0001/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_jump_land_action;

/* the models and motions of Station Square's people. they lie in ADV00.PRS,
 * which must stay loaded while the event plays */
extern NJS_OBJECT dat_0cabaec8;
extern NJS_OBJECT dat_0cac6330;
extern NJS_OBJECT dat_0cac8a7c;
extern NJS_OBJECT dat_0cae0360;
extern NJS_OBJECT dat_0cae679c;
extern NJS_OBJECT dat_0cae9ee0;
extern NJS_OBJECT dat_0caee8e4;
extern NJS_OBJECT dat_0caf12b8;
extern NJS_OBJECT dat_0caf6850;
extern NJS_MOTION dat_0cb0b2c8;
extern NJS_MOTION dat_0cb0c214;
extern NJS_MOTION dat_0cb0d3a0;
extern NJS_MOTION dat_0cb10e24;
extern NJS_MOTION dat_0cb1eb50;
extern NJS_MOTION dat_0cb218c8;
extern NJS_MOTION dat_0cb27d34;

extern NJS_ACTION ev0001_sonic_action_0;
extern NJS_ACTION ev0001_sonic_action_1;
extern NJS_ACTION ev0001_sonic_action_2;
extern NJS_ACTION ev0001_sonic_action_3;
extern NJS_TEXLIST ev0001_picture_texlist;

/* player 0's pad as Sonic jumps from high above the street, a key a step. the
 * 1st to 8th keys hold A, and so do the 17th to 24th */
static const struct event_pad_key ev0001_jump_keys[61] = {
#include "ev0001/script/ev0001_jump_keys.inc"
};

static const struct event_pad_recording ev0001_jumps = {
    ev0001_jump_keys, sizeof(ev0001_jump_keys) / sizeof(ev0001_jump_keys[0])};

/* player 0's pad as Sonic runs off, a key a step. from the 2nd key on, the
 * stick points at -50 to -42, in 256ths of a turn. from the 7th key on, it is
 * pushed 244 to 251 out of 255. the 11th to 22nd keys hold B too */
static const struct event_pad_key ev0001_run_keys[98] = {
#include "ev0001/script/ev0001_run_keys.inc"
};

static const struct event_pad_recording ev0001_run = {
    ev0001_run_keys, sizeof(ev0001_run_keys) / sizeof(ev0001_run_keys[0])};

static struct event_path_point ev0001_patrol_car_points_0[186] = {
#include "ev0001/script/ev0001_patrol_car_points_0.inc"
};

static struct event_path ev0001_patrol_car_path_0 = {
    186, ev0001_patrol_car_points_0};

static struct event_path_point ev0001_patrol_car_points_1[212] = {
#include "ev0001/script/ev0001_patrol_car_points_1.inc"
};

static struct event_path ev0001_patrol_car_path_1 = {
    212, ev0001_patrol_car_points_1};

static struct event_path_point ev0001_patrol_car_points_2[161] = {
#include "ev0001/script/ev0001_patrol_car_points_2.inc"
};

static struct event_path ev0001_patrol_car_path_2 = {
    161, ev0001_patrol_car_points_2};

static struct event_path_point ev0001_patrol_car_points_3[188] = {
#include "ev0001/script/ev0001_patrol_car_points_3.inc"
};

static struct event_path ev0001_patrol_car_path_3 = {
    188, ev0001_patrol_car_points_3};

static struct event_path_point ev0001_helicopter_points[85] = {
#include "ev0001/script/ev0001_helicopter_points.inc"
};

static struct event_path ev0001_helicopter_path = {85,
                                                   ev0001_helicopter_points};

static struct event_path_point ev0001_light_points[73] = {
#include "ev0001/script/ev0001_light_points.inc"
};

static struct event_path ev0001_light_path = {73, ev0001_light_points};

static struct task *ev0001_patrol_car_0 = NULL;
static struct task *ev0001_patrol_car_1 = NULL;
static struct task *ev0001_patrol_car_2 = NULL;
static struct task *ev0001_patrol_car_3 = NULL;
static struct task *ev0001_helicopter = NULL;
static struct task *ev0001_floor_0 = NULL;
static struct task *ev0001_floor_1 = NULL;
static struct task *ev0001_picture = NULL;
static struct task *ev0001_light = NULL;
static struct task *ev0001_fade = NULL;
static struct task *ev0001_person_0 = NULL;
static struct task *ev0001_person_1 = NULL;
static struct task *ev0001_person_2 = NULL;
static struct task *ev0001_person_3 = NULL;
static struct task *ev0001_person_4 = NULL;
static struct task *ev0001_person_5 = NULL;
static struct task *ev0001_person_6 = NULL;
static struct task *ev0001_person_7 = NULL;
static struct task *ev0001_person_8 = NULL;

void ev0001_script(int phase)
{
    NJS_ACTION *action;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        sound_load_file("e_0001.mlt");
        event_reset_player(0);
        event_sound_init(6);
        event_set_pos(event_get_player(0), 362.5F, 0.0F, 1450.72F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_flags[FLAG_TRAFFIC] = TRUE;
        event_place_actor(&ev0001_patrol_car_0, 508.59F, 0.0F, 727.0F, 0,
                          0x8000, 0);
        event_place_actor(&ev0001_patrol_car_1, 536.63F, 0.0F, 718.0F, 0,
                          0x8000, 0);
        event_place_actor(&ev0001_patrol_car_2, 569.0F, 0.0F, 718.0F, 0, 0x8000,
                          0);
        event_place_actor(&ev0001_patrol_car_3, 609.0F, 0.0F, 720.0F, 0, 0x8000,
                          0);
        event_place_actor(&ev0001_helicopter, 508.59F, 50.0F, 727.0F, 0, 0x8000,
                          0);
        ev0001_picture = event_picture_create(&ev0001_picture_texlist);
        event_picture_set_depth(ev0001_picture, -1000.0F);
        ev0001_light = event_light_create(536.63F, 30.0F, 718.0F);
        event_place_actor(&ev0001_person_0, 131.44F, 9.8F, 1503.57F, 0, 0x386f,
                          0);
        event_place_actor(&ev0001_person_3, 32.36F, 8.3F, 1504.04F, 0, 0x138e4,
                          0);
        event_place_actor(&ev0001_person_8, 19.67F, 5.66F, 1502.95F, 0, 0x47f3,
                          0);
        event_place_actor(&ev0001_person_4, 580.55F, 8.3F, 1063.28F, 0, 0x16af9,
                          0);
        event_place_actor(&ev0001_person_5, 330.06F, 10.85F, 1058.57F, 0,
                          0x0e18, 0);
        event_place_actor(&ev0001_person_1, 377.54F, 9.8F, 1158.57F, 0, 0x9563,
                          0);
        event_place_actor(&ev0001_person_6, 197.89F, 10.85F, 1152.89F, 0,
                          0x45cb, 0);
        event_place_actor(&ev0001_person_7, 173.05F, 10.85F, 1270.47F, 0,
                          0xba3a, 0);
        event_place_actor(&ev0001_person_2, 183.29F, 9.8F, 1265.27F, 0, 0xb9ac,
                          0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_patrol_car_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev0001_patrol_car_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev0001_patrol_car_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev0001_patrol_car_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev0001_helicopter, EVENT_IGNORE_GROUND);
        event_set_flags(ev0001_light, EVENT_IGNORE_GROUND);
        event_set_pos(ev0001_patrol_car_0, 508.59F, 0.0F, 727.0F);
        event_set_ang(ev0001_patrol_car_0, 0, 0x8000, 0);
        event_set_pos(ev0001_patrol_car_1, 536.63F, 0.0F, 718.0F);
        event_set_ang(ev0001_patrol_car_1, 0, 0x8000, 0);
        event_set_pos(ev0001_patrol_car_2, 569.0F, 0.0F, 718.0F);
        event_set_ang(ev0001_patrol_car_2, 0, 0x8000, 0);
        event_set_pos(ev0001_patrol_car_3, 609.0F, 0.0F, 720.0F);
        event_set_ang(ev0001_patrol_car_3, 0, 0x8000, 0);
        event_set_pos(ev0001_helicopter, 609.0F, 50.0F, 720.0F);
        event_set_ang(ev0001_helicopter, 0, 0x8000, 0);
        /* @hack spelling: the action's address through a local, which SHC loads
         * once into R8 for the four event_play_action calls below (MOV.L at
         * 0x0cb8047c). by name, each call loads it from the pool, and 255 of
         * 1790 words differ */
        action = &ev0001_patrol_car_action;
        event_play_action(ev0001_patrol_car_0, action,
                          &ev0001_patrol_car_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0001_patrol_car_1, action,
                          &ev0001_patrol_car_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0001_patrol_car_2, action,
                          &ev0001_patrol_car_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0001_patrol_car_3, action,
                          &ev0001_patrol_car_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0001_helicopter, &ev0001_helicopter_action,
                          &ev0001_helicopter_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_0, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_0);
        event_play_motion(ev0001_person_0, &dat_0caee8e4, &dat_0cb0c214,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_3, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_3);
        event_play_motion(ev0001_person_3, &dat_0cac8a7c, &dat_0cb1eb50,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_8, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_8);
        event_play_motion(ev0001_person_8, &dat_0cabaec8, &dat_0cb27d34,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_4, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_4);
        event_play_motion(ev0001_person_4, &dat_0cac6330, &dat_0cb218c8,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_5, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_5);
        event_play_motion(ev0001_person_5, &dat_0cae9ee0, &dat_0cb10e24,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_1, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_1);
        event_play_motion(ev0001_person_1, &dat_0caf6850, &dat_0cb0d3a0,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_6, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_6);
        event_play_motion(ev0001_person_6, &dat_0cae0360, &dat_0cb10e24,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_7, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_7);
        event_play_motion(ev0001_person_7, &dat_0cae679c, &dat_0cb10e24,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0001_person_2, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0001_person_2);
        event_play_motion(ev0001_person_2, &dat_0caf12b8, &dat_0cb0b2c8,
                          &ss_people_texlist, 0.8F, EVENT_REPEAT, 0);
        event_set_shadow(ev0001_person_0, 1.0F);
        event_set_shadow(ev0001_person_1, 1.0F);
        event_set_shadow(ev0001_person_2, 1.0F);
        event_set_shadow(ev0001_person_3, 1.0F);
        event_set_shadow(ev0001_person_4, 1.0F);
        event_set_shadow(ev0001_person_8, 1.0F);
        event_set_shadow(ev0001_person_5, 1.0F);
        event_set_shadow(ev0001_person_6, 1.0F);
        event_set_shadow(ev0001_person_7, 1.0F);
        event_wait(1);
        stream_request_music(MUSIC_S_SQUARE);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 116.84F, 14.44F, 1653.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0627, 0x12eb, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 161.54F, 14.44F, 1630.65F);
        event_wait(110);
        event_move(ev0001_person_5, 307.23F, 10.85F, 1054.78F, 365.69F, 10.85F,
                   1051.68F, 200);
        event_move(ev0001_person_4, 610.07F, 8.3F, 1053.4F, 508.43F, 8.3F,
                   1099.62F, 550);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 232.68F, 22.09F, 1144.67F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0023, 0xd1eb, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 250.72F, 22.09F, 1183.03F);
        event_wait(110);
        event_move(ev0001_person_6, 197.89F, 10.85F, 1152.89F, 197.46F, 10.85F,
                   1209.71F, 210);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 297.07F, 17.95F, 1184.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0123, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 298.8F, 17.94F, 1271.98F);
        event_move(ev0001_person_7, 172.95F, 10.85F, 1296.21F, 172.68F, 10.85F,
                   1206.63F, 240);
        event_move(ev0001_person_2, 182.66F, 9.8F, 1287.36F, 183.04F, 10.85F,
                   1200.08F, 240);
        event_wait(120);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 621.22F, 1.15F, 1035.13F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1123, 0x49e8, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 614.96F, 1.15F, 1009.89F);
        event_play_path(ev0001_helicopter, &ev0001_helicopter_path, 0.5F, 0);
        event_wait(20);
        event_sound_start(0, 1224, 1800);
        event_sound_fade_volume(0, -120, 1);
        event_wait(1);
        event_sound_fade_volume(0, 120, 90);
        event_wait(70);
        event_play_path(ev0001_light, &ev0001_light_path, 0.7F, 0);
        event_wait(30);
        event_sound_fade_volume(0, -120, 60);
        event_wait(10);
        event_sound_play(1228, 100, 0, 0);
        event_wait(20);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 364.62F, 216.24F, 1124.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0xd4eb, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 292.38F, 234.87F,
                             1164.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0xf700, 0xd4eb, 0);
        event_set_pos(event_get_player(0), 623.66F, 0.0F, 757.8F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_move_stop_all();
        event_wait(10);
        event_kill_task(&ev0001_person_0);
        event_kill_task(&ev0001_person_3);
        event_kill_task(&ev0001_person_8);
        event_kill_task(&ev0001_person_4);
        event_kill_task(&ev0001_person_5);
        event_kill_task(&ev0001_person_1);
        event_kill_task(&ev0001_person_6);
        event_kill_task(&ev0001_person_7);
        event_kill_task(&ev0001_person_2);
        stream_request_music(MUSIC_THESONIC);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0001_sonic_action_3,
                          &sonic_texlist, 0.25F, 0, 0);
        event_wait(20);
        event_wait(60);
        event_sound_stop(0);
        event_sound_play(SE_JUMP, 100, 0, 0);
        event_wait(30);
        event_flags[FLAG_TRAFFIC] = FALSE;
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 12.69F, 770.32F, 1382.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf800, 0x4000, 0);
        event_picture_fade(ev0001_picture, 1.0F, 1);
        event_set_pos(event_get_player(0), -50.55F, 780.0F, 1381.84F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_clear_motions(event_get_player(0));
        ev0001_floor_0 = event_floor_create(event_get_player(0), 740.0F);
        event_wait(90);
        EVENT_KILL(ev0001_picture);
        EVENT_KILL(ev0001_floor_0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 306.11F, 419.31F, 1376.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1e00, 0xd500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 362.84F, 478.4F, 1344.7F);
        event_set_pos(event_get_player(0), 370.0F, 480.0F, 1342.17F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        ev0001_floor_1 = event_floor_create(event_get_player(0), 480.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(event_get_player(0), &ev0001_sonic_action_0,
                          &sonic_texlist, 2.0F, 0, 8);
        event_play_action(event_get_player(0), &ev0001_sonic_action_1,
                          &sonic_texlist, 2.0F, 0, 15);
        event_play_action(event_get_player(0), &ev0001_sonic_action_2,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 15);
        event_voice_play(931);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_wait(5);
        event_message_show(TEXT_1);
        event_talk_start(event_get_player(0), TEXT_3);
        event_wait(10);
        event_voice_play(932);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_wait(15);
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 364.99F, 482.34F, 1344.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1e00, 0xd300, 0);
        event_sound_start(5, 1225, 1800);
        event_sound_fade_volume(5, 80, 120);
        event_sound_start(3, 1226, 1800);
        event_sound_fade_volume(3, 100, 60);
        event_wait(15);
        event_sound_start(4, 1226, 1800);
        event_sound_fade_volume(4, 100, 30);
        event_voice_play(933);
        event_look_at(event_get_player(0), -85.34F, 8.0F, 1590.0F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_play_path(ev0001_patrol_car_0, &ev0001_patrol_car_path_0, 1.0F,
                        0);
        event_play_path(ev0001_patrol_car_1, &ev0001_patrol_car_path_1, 1.0F,
                        0);
        event_play_path(ev0001_patrol_car_2, &ev0001_patrol_car_path_2, 1.0F,
                        0);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 300.23F, 493.31F, 1425.24F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xc900, 0x3a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 316.22F, 493.31F,
                             1317.41F);
        event_play_path(ev0001_patrol_car_3, &ev0001_patrol_car_path_3, 1.0F,
                        0);
        event_wait(30);
        event_sound_play(1227, 100, 0, 0);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0001_jumps);
        event_wait(10);
        EVENT_KILL(ev0001_floor_1);
        event_sound_play(1227, 60, 0, 0);
        event_wait(30);
        event_sound_play(1227, 80, 0, 0);
        event_sound_fade_volume(5, -80, 120);
        event_wait(60);
        event_kill_task(&ev0001_patrol_car_0);
        event_kill_task(&ev0001_patrol_car_2);
        event_sound_fade_volume(4, -100, 80);
        event_sound_fade_volume(3, -100, 80);
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_set_pos(event_get_player(0), 118.25F, 30.0F, 1509.55F);
        event_set_ang(event_get_player(0), 0, 0x6000, 0);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 103.03F, 9.46F, 1534.32F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_wait(60);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.2F, EVENT_REPEAT, 0);
        event_sound_stop(5);
        event_kill_task(&ev0001_patrol_car_1);
        event_kill_task(&ev0001_patrol_car_3);
        event_camera_look_clear();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 123.08F, 2.2F, 1502.99F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x12c7, 0x6c4a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 120.42F, 2.2F, 1501.59F);
        event_wait(10);
        event_voice_play(934);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_message_show(TEXT_2);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_sound_stop(3);
        event_sound_stop(4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 103.03F, 9.46F, 1534.32F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfcc7, 0xea4a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 116.24F, 8.4F, 1525.95F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 60, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0001_run);
        ev0001_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_sound_stop_all();
        event_move_stop_all();
        EVENT_KILL(ev0001_picture);
        EVENT_KILL(ev0001_fade);
        EVENT_KILL(ev0001_floor_0);
        EVENT_KILL(ev0001_floor_1);
        EVENT_KILL(ev0001_light);
        event_kill_task(&ev0001_patrol_car_0);
        event_kill_task(&ev0001_patrol_car_1);
        event_kill_task(&ev0001_patrol_car_2);
        event_kill_task(&ev0001_patrol_car_3);
        event_kill_task(&ev0001_helicopter);
        event_kill_task(&ev0001_person_0);
        event_kill_task(&ev0001_person_3);
        event_kill_task(&ev0001_person_8);
        event_kill_task(&ev0001_person_4);
        event_kill_task(&ev0001_person_5);
        event_kill_task(&ev0001_person_1);
        event_kill_task(&ev0001_person_6);
        event_kill_task(&ev0001_person_7);
        event_kill_task(&ev0001_person_2);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0001_textures[] = {
    {"ICM0001_3", &ev0001_picture_texlist},
    {"SSPATCAR_BODY", &ev0001_patrol_car_texlist},
    {"EV_HELI", &ev0001_helicopter_texlist},
    {NULL, NULL},
};
