/* EV0069, Amy's story: Eggman takes the bird's emerald and sets E-102 on
 * Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0069/bird_textures_1.h"
#include "ev0069/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/e102.h"
#include "player/tails.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/claw.h"
#include "shared/claw_textures.h"
#include "shared/eggmobile.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 36
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 15
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 21
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 30
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 33
#define TEXT_12_LENGTH 44
#define TEXT_13_LENGTH 25
#define TEXT_14_LENGTH 33
#define TEXT_15_LENGTH 18
#define TEXT_16_LENGTH 23
#define TEXT_17_LENGTH 2
#define TEXT_18_LENGTH 2
#define TEXT_19_LENGTH 1
#define TEXT_20_LENGTH 2
#define TEXT_21_LENGTH 1
#define TEXT_22_LENGTH 2
#include "ev0069/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_walk_action;
extern NJS_ACTION sonic_run_action;
extern NJS_ACTION sonic_land_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION tails_walk_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev0069_tails_action;
extern NJS_ACTION ev0069_amy_action_0;
extern NJS_ACTION ev0069_amy_action_1;
extern NJS_ACTION ev0069_amy_action_2;
extern NJS_ACTION ev0069_amy_action_3;
extern NJS_ACTION ev0069_amy_action_4;
extern NJS_TEXLIST ev0069_emerald_texlist;
extern NJS_ACTION ev0069_emerald_action;
extern NJS_TEXLIST event_eggmobile_texlist;

static struct task *ev0069_bird = NULL;
static struct task *ev0069_emerald = NULL;
static struct task *ev0069_claw_0 = NULL;
static struct task *ev0069_claw_1 = NULL;
/* no code uses this word. it keeps ev0069_textures at its address in the
 * image */
static Uint32 dat_0cb85ca8 = 0;

void ev0069_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        sound_load_file("e_0016.mlt");
        event_sound_init(5);
        event_reset_player(0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_place_actor(&ev0069_emerald, -547.33F, -0.5F, 973.9F, 0, 0, 0);
        event_set_flags(ev0069_emerald, EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(0), -55.6F, 1525.67F, 3320.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_create_player(2, sonic_step, 2.82F, 1633.08F, 3185.52F, 0, 0x8000,
                            0);
        event_create_player(3, tails_step, 2.82F, 1633.08F, 3190.52F, 0, 0x8000,
                            0);
        event_create_player(4, e102_step, 2.82F, 1633.08F, 3180.52F, 0, 0x8000,
                            0);
        event_eggmobile_create(122.17F, 1710.7F, 3169.04F, 0, 0x1400, 0);
        event_place_actor(&ev0069_bird, 2.82F, 1638.08F, 3195.52F, 0, -0x4000,
                          0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0069_amy_action_2,
                          &amy_texlist, 2.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_stand_action,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0069_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev0069_bird, &ev0069_bird_action_0,
                          &ev0069_bird_texlist_2, 2.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev0069_eggmobile_action_0, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 0);
        ev0069_claw_0 =
            event_claw_create(-27.57F, 2058.18F, 3221.0F, 0, 0, 0x8000);
        ev0069_claw_1 =
            event_claw_create(-27.57F, 2058.18F, 3221.0F, 0, -0x2000, -0x2000);
        event_claw_set_rates(ev0069_claw_1, 0x1100, 0x900);
        event_wait(1);
        stream_request_music(MUSIC_EGCARER1);
        event_eggmobile_set_bob(0x100, 0.5F);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -48.55F, 1550.71F, 3252.25F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x30f, 0x7757, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -49.77F, 1530.71F,
                             3257.92F);
        event_wait(15);
        event_run_to_speed(event_get_player(0), -55.6F, 1525.67F, 3273.19F,
                           2.0F, 0.06F);
        event_follow(ev0069_bird, 0.0F, 0.0F, 0.0F, 900, event_get_player(0));
        event_wait(70);
        event_voice_play(1456);
        event_talk_start(event_get_player(0), TEXT_17);
        event_message_show(TEXT_1);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev0069_bird);
        event_move_stop(ev0069_bird);
        event_set_ang(ev0069_bird, 0, 0x8000, 0);
        event_play_action(ev0069_bird, &ev0069_bird_action_2,
                          &ev0069_bird_texlist_2, 1.0F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(0), &ev0069_amy_action_4,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -65.59F, 1529.56F, 3265.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x20f, 0xb257, 0);
        event_wait(30);
        event_look_at(event_get_player(0), -64.4F, 1532.0F, 3266.0F);
        event_wait(10);
        event_look_at(event_get_player(0), -21.0F, 1532.0F, 3293.0F);
        event_wait(10);
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_clear_motions(ev0069_bird);
        event_play_action(event_get_player(0), &ev0069_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0069_bird, &ev0069_bird_action_1,
                          &ev0069_bird_texlist_2, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0, 0x5878, 0);
        event_set_ang(ev0069_bird, 0, 0x5878, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -64.38F, 1531.37F, 3286.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1900, 0xe000, 0);
        event_eggmobile_fly_to(122.17F, 1540.0F, 3169.04F, 60);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x400, 0xe000, 0);
        event_wait(80);
        event_eggmobile_fly_to(-9.6F, 1535.0F, 3241.01F, 40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -64.28F, 1528.4F, 3294.85F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x500, 0xe800, 0);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0069_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_follow(ev0069_bird, 0.0F, 0.0F, 0.0F, 900, event_get_player(0));
        event_move(event_get_player(0), -55.6F, 1525.6875F, 3273.657F, -65.89F,
                   1526.0F, 3280.36F, 60);
        event_wait(25);
        event_play_action(event_get_player(0), &ev0069_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 4);
        event_wait(10);
        event_move_stop_all();
        event_set_pos(event_get_player(0), -55.6F, 1525.6875F, 3273.657F);
        event_set_pos(ev0069_bird, -55.6F, 1525.6875F, 3273.657F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0xe39);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -0.09F, 1546.93F, 3197.85F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf90f, 0x69d3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 9.12F, 1549.81F, 3184.55F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xf90f, 0x69d3, 0);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), -9.6F, 1550.0F, 3241.01F);
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1457);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -13.33F, 1548.0F, 3241.61F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x100, 0xc300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -13.47F, 1548.0F,
                             3239.62F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x100, 0xb800, 0);
        event_voice_play(1458);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_start(event_get_player(0), TEXT_19);
        event_set_pos(event_get_player(2), -8.0F, 1525.69F, 3370.0F);
        event_wait(1);
        event_set_pos(event_get_player(3), 8.0F, 1525.69F, 3383.0F);
        event_wait(1);
        event_look_clear(event_get_player(3));
        event_look_at(event_get_player(3), -9.6F, 1550.0F, 3241.01F);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -50.86F, 1532.59F, 3258.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfeba, 0x75dd, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -47.41F, 1533.03F,
                             3244.46F);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(3), &ev0069_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 8.0F,
                           0.0F);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 80, 0x3111);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -56.27F, 1532.36F,
                             3209.03F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x80f, 0x9557, 0);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -11.21F, 1528.34F,
                             3359.11F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0xa0f, 0x9357, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -10.32F, 1528.83F,
                             3360.83F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xa0f, 0x9357, 0);
        event_voice_play(1459);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -14.71F, 1577.66F, 3205.51F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf20f, 0x8357, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_walk_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 10);
        event_run_to_speed(event_get_player(2), -8.0F, 1525.69F, 3320.0F, 1.0F,
                           0.06F);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &tails_walk_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 10);
        event_run_to_speed(event_get_player(3), 8.0F, 1525.69F, 3341.0F, 1.0F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -14.71F, 1562.65F,
                             3205.51F);
        event_wait(60);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_land_action,
                          &sonic_texlist, 2.0F, 0, 4);
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev0069_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -29.26F, 1551.69F,
                             3236.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xf60f, 0xb857, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -29.63F, 1551.69F, 3238.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xf60f, 0xbd57, 0);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), -9.6F, 1550.0F, 3241.01F);
        event_voice_play(1082);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -102.68F, 1532.38F,
                             3277.51F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x10f, 0xcb57, 0);
        event_wait(30);
        event_sound_start(0, 1226, 1800);
        event_move(ev0069_claw_0, 11.0F, 1547.48F, 3231.4F, 11.0F, 1560.48F,
                   3231.4F, 30);
        event_wait(30);
        event_move(ev0069_claw_1, -25.0F, 1580.0F, 3271.0F, -45.0F, 1525.0F,
                   3271.0F, 20);
        event_claw_set_mode(ev0069_claw_1, CLAW_OPEN);
        event_wait(30);
        event_claw_set_mode(ev0069_claw_1, CLAW_CLOSE);
        event_voice_wait();
        event_move(ev0069_claw_1, -45.0F, 1525.0F, 3271.0F, -25.0F, 1580.0F,
                   3271.0F, 20);
        event_move(ev0069_bird, -55.6F, 1525.6875F, 3273.657F, -35.6F,
                   1580.6875F, 3273.657F, 20);
        event_voice_play(1460);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0069_amy_action_0,
                          &amy_texlist, 1.0F, 0, 8);
        event_message_show(TEXT_6);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(40);
        event_sound_stop(0);
        event_message_close();
        EVENT_KILL(ev0069_claw_0);
        EVENT_KILL(ev0069_claw_1);
        event_set_pos(ev0069_bird, -26.0F, 1534.5F, 3242.0F);
        event_clear_motions(ev0069_bird);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(ev0069_bird, &ev0069_bird_action_3,
                          &ev0069_bird_texlist_2, 1.0F, EVENT_REPEAT, 3);
        event_play_action(event_eggmobile_get_task(),
                          &ev0069_eggmobile_action_3, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -50.9F, 1528.857F,
                             3285.253F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xa00, 0xef00, 0);
        event_sound_play(1228, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -56.54F, 1528.858F,
                             3283.086F);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -57.45F, 1528.858F,
                             3282.68F);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev0069_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, 0, 0);
        event_move(ev0069_bird, -26.0F, 1534.5F, 3242.0F, -50.0F, 1566.67F,
                   3257.0F, 100);
        event_set_pos(ev0069_emerald, -24.0F, 1525.5F, 3236.0F);
        event_wait(18);
        event_clear_motions(event_eggmobile_get_task());
        event_clear_motions(ev0069_emerald);
        event_play_action(event_eggmobile_get_task(),
                          &ev0069_eggmobile_action_1, &event_eggmobile_texlist,
                          0.2F, EVENT_REPEAT, 6);
        event_play_action(ev0069_emerald, &ev0069_emerald_action,
                          &ev0069_emerald_texlist, 0.2F, EVENT_REPEAT, 0);
        event_wait(60);
        event_set_pos(event_get_player(3), 3.0F, 1525.69F, 3345.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 10.48F, 1526.56F, 3283.85F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x150f, 0xa457, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 15, -14.64F, 1526.56F,
                             3315.01F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -16.52F, 1526.56F, 3317.3F);
        event_talk_stop(event_get_player(2));
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_20);
        event_voice_play(1461);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_talk_stop(event_get_player(3));
        event_set_pos(event_get_player(3), 8.0F, 1525.69F, 3341.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -25.55F, 1530.93F, 3276.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xb0f, 0x7057, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 15, -51.5F, 1530.93F, 3266.32F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -53.36F, 1530.93F,
                             3265.57F);
        event_talk_start(event_get_player(0), TEXT_21);
        event_voice_play(1462);
        event_message_show(TEXT_8);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -35.23F, 1541.97F, 3266.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xb0f, 0xe357, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -15.77F, 1541.97F,
                             3275.48F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xb0f, 0xf957, 0);
        event_voice_play(1085);
        event_message_show(TEXT_9);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -10.53F, 1543.0F, 3255.99F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xb0f, 0xfa57, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, -9.73F, 1544.5F, 3250.27F);
        event_voice_play(1086);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_set_pos(event_get_player(4), 0.0F, 1525.7F, 3402.0F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 0.0F, 1569.21F, 3388.22F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x271, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 25, 0.0F, 1543.27F, 3389.79F);
        event_sound_play(1229, 0, 0, 0);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 0.0F, 1544.27F, 3389.79F);
        event_voice_play(1767);
        event_message_show(TEXT_11);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_kill_task(&ev0069_emerald);
        event_clear_motions(event_eggmobile_get_task());
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -3.22F, 1532.25F, 3281.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0xff0f, 0x8057, 0);
        event_wait(45);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_look_clear(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_clear_motions(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_clear_motions(ev0069_bird);
        event_play_action(event_get_player(0), &ev0069_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0069_bird, &ev0069_bird_action_1,
                          &ev0069_bird_texlist_2, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), -55.6F, 1525.6875F, 3273.657F);
        event_set_pos(ev0069_bird, -55.6F, 1525.6875F, 3273.657F);
        event_set_ang(ev0069_bird, 0, 0x4000, 0);
        event_wait(20);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_set_ang(event_get_player(3), 0, 0, 0);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -30.65F, 1533.18F, 3474.31F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x20d, 0xf3a8, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -84.1F, 1533.18F,
                             3457.59F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x20d, 0xf2a8, 0);
        event_set_pos(event_get_player(3), 8.0F, 1525.69F, 3325.0F);
        event_voice_play(1087);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_voice_play(1088);
        event_message_wait(60, TEXT_13);
        event_wait(15);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -41.64F, 1526.67F, 3295.58F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x50d, 0x8d24, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -16.98F, 1526.67F,
                             3287.34F);
        event_voice_play(1768);
        event_message_show(TEXT_14);
        event_wait(10);
        event_voice_wait();
        event_message_close();
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -16.75F, 1544.79F, 3256.62F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xb00, 0xebe3, 0);
        event_voice_play(1089);
        event_message_show(TEXT_15);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_sound_start(1, 1231, 1800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -58.75F, 1527.34F,
                             3293.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xd0d, 0xee24, 0);
        event_eggmobile_fly_turn(-15.0F, 1560.0F, 3235.0F, 0, -0x1400, 0, 50,
                                 0);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -76.39F, 1526.74F,
                             3293.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xa87, 0xf443, 0);
        event_eggmobile_fly_turn(-90.0F, 1560.0F, 3200.0F, 0, -0x5000, 0, 50,
                                 0);
        event_wait(40);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), -44.0F, 1532.5F, 3269.3F);
        event_sound_stop(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.9F, 1529.7F, 3303.187F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xbae, 0x931f, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -13.72F, 1529.2F,
                             3311.133F);
        event_wait(50);
        event_play_action(event_get_player(2), &sonic_run_action,
                          &sonic_texlist, 3.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(2), 0.0F, 1525.7F, 3402.0F);
        event_wait(15);
        event_play_action(event_get_player(4), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(35);
        event_set_pos(event_get_player(3), 8.0F, 1525.69F, 3341.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -25.55F, 1530.93F, 3276.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x140f, 0x6057, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 15, -49.77F, 1529.64F,
                             3268.836F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, -51.36F, 1529.93F,
                             3266.57F);
        event_talk_start(event_get_player(0), TEXT_22);
        event_voice_play(1463);
        event_message_show(TEXT_16);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(3);
        event_kill_player(4);
        event_kill_task(&ev0069_bird);
        event_eggmobile_kill();
        if (ev0069_claw_0 != NULL) {
            task_kill(ev0069_claw_0);
            ev0069_claw_0 = NULL;
        }
        if (ev0069_claw_1 != NULL) {
            task_kill(ev0069_claw_1);
            ev0069_claw_1 = NULL;
        }
        event_kill_task(&ev0069_emerald);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0069_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"VER1_WING", &ev0069_bird_texlist_1},
    {"VER2_WING", &ev0069_bird_texlist_2},
    {"MGHAND", &event_claw_texlist},
    {"M_EM_SKY", &ev0069_emerald_texlist},
    {"SONIC", &sonic_texlist},
    {"MILES", &tails_texlist},
    {"E102", &e102_texlist},
    {NULL, NULL},
};
