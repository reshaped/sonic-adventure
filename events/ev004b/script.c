/* EV004B, Tails' story: Eggman takes the Chaos Emerald Amy's bird carries,
 * and sets E-102 on Tails and Sonic */
#include <ninja.h>

#include "adv0100/egg_lift.h"
#include "adv0100/gate.h"
#include "ev004b/bird_textures.h"
#include "ev004b/emerald_action.h"
#include "ev004b/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/e102.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/claw.h"
#include "shared/claw_textures.h"
#include "shared/eggmobile.h"
#include "shared/floor.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 22
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 21
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 30
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 33
#define TEXT_9_LENGTH 34
#define TEXT_10_LENGTH 25
#define TEXT_11_LENGTH 33
#define TEXT_12_LENGTH 18
#define TEXT_13_LENGTH 2
#define TEXT_14_LENGTH 1
#define TEXT_15_LENGTH 2
#include "ev004b/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION sonic_sprint_action;
extern NJS_ACTION tails_sprint_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_fall_start_action;
extern NJS_ACTION e102_hover_action;
extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev004b_sonic_action;
extern NJS_ACTION ev004b_tails_action;
extern NJS_ACTION ev004b_amy_action_0;
extern NJS_ACTION ev004b_amy_action_1;
extern NJS_ACTION ev004b_amy_action_2;
extern NJS_TEXLIST event_eggmobile_texlist;

/* Tails' run. A is held from key 3 to key 10, for a jump. from key 4 to key 44,
 * the stick is pushed all the way. its direction there, in 256ths of a turn,
 * stays at -65 to -63, near minus a quarter */
static const struct event_pad_key ev004b_run_keys[111] = {
#include "ev004b/script/ev004b_run_keys.inc"
};

static const struct event_pad_recording ev004b_run = {
    ev004b_run_keys, sizeof(ev004b_run_keys) / sizeof(ev004b_run_keys[0])};

/* no code uses ev004b_path or its points. they keep ev004b_bird at its
 * address in the image */
static struct event_path_point ev004b_path_points[107] = {
#include "ev004b/script/ev004b_path_points.inc"
};

static struct event_path ev004b_path = {107, ev004b_path_points};

static struct task *ev004b_bird = NULL;
static struct task *ev004b_claw = NULL;
static struct task *ev004b_emerald = NULL;
static struct task *ev004b_floor = NULL;

void ev004b_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_004b.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EGCARER1);
        event_flags[FLAG_TAILS_EC_EGG_LIFT] = 1;
        event_flags[FLAG_LIFT] = LIFT_LOW_NOW;
        event_place_actor(&ev004b_emerald, -547.33F, -0.5F, 973.9F, 0, 0, 0);
        event_set_flags(ev004b_emerald, EVENT_IGNORE_GROUND);
        event_eggmobile_create(22.8F, 1535.0F, 3231.6F, 0, 0x900, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_set_pos(event_get_player(0), -4.176F, 1495.0F, 3444.76F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004b_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(
            3, sonic_step, event_get_player(0)->work->pos.x + 8.0F,
            event_get_player(0)->work->pos.y - 4.0F,
            event_get_player(0)->work->pos.z + 8.0F, 0, 0x8000, 0);
        event_wait(1);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(5, amy_step, -15.0F, 1525.687F, 3245.5F, 0, 0x4fe0,
                            0);
        event_create_player(4, e102_step, -254.8F, 1525.67F, 3754.6F, 0, 0x8000,
                            0);
        event_wait(1);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_stand_action,
                          &e102_texlist, 0.1F, EVENT_REPEAT, 0);
        event_place_actor(&ev004b_bird, 0.0F, 5.5F, 0.0F, 0, 0x8000, 0);
        event_set_flags(ev004b_bird, EVENT_IGNORE_GROUND);
        event_set_pos(ev004b_bird, event_get_player(5)->work->pos.x,
                      event_get_player(5)->work->pos.y,
                      event_get_player(5)->work->pos.z);
        event_set_ang(ev004b_bird, 0, 0x2e1c, 0);
        event_clear_motions(ev004b_bird);
        event_clear_motions(event_get_player(5));
        event_play_action(ev004b_bird, &ev004b_bird_action_0,
                          &ev004b_bird_texlist, 1.0F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(5), &ev004b_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_flags[FLAG_LIFT] = LIFT_RISE;
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_set_pos(event_get_player(0), 0.0F, 1525.7F, 3443.76F);
        event_set_pos(event_get_player(3), -20.0F, 1525.0F, 3466.0F);
        event_set_ang(event_get_player(0), 0, 0x7800, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc00, 0x8100, 0x100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1.4734F, 1578.6515F,
                             3399.8914F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 1.07292F, 1556.64F,
                             3406.565F);
        event_wait(125);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 0.7595F, 1539.4206F,
                             3411.7881F);
        event_wait(60);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf600, 0xc00, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 45.11452F, 1564.748F,
                             3467.55F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 10.503F, 1567.301F,
                             3477.116F);
        event_wait(56);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev004b_run);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xec00, 0xf900, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -7.4F, 1579.79F, 3476.848F);
        event_wait(50);
        event_set_pos(event_get_player(0), -0.5F, 1526.0F, 3411.0F);
        event_play_action(event_get_player(0), &tails_sprint_action,
                          &tails_texlist, 2.5F, EVENT_REPEAT, 8);
        event_set_pos(event_get_player(3), 9.5F, 1526.0F, 3421.0F);
        event_play_action(event_get_player(3), &sonic_sprint_action,
                          &sonic_texlist, 2.7F, EVENT_REPEAT, 8);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3c72);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -5.4343F, 1537.0F,
                             3423.655F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf390, 0xfae0, 0x100);
        event_camera_follow(event_get_player(0));
        event_run_to_speed(event_get_player(0), -2.0F, 1526.0F, 3344.6F, 1.7F,
                           1.9F);
        event_run_to_speed(event_get_player(3), 8.0F, 1526.0F, 3354.6F, 1.5F,
                           1.9F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 23, 0x31c7);
        event_wait(25);
        event_camera_follow_clear();
        event_camera_set_ang(EVENT_EASE_LINEAR, 38, 0xa00, 0xe450, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 38, -24.8F, 1526.15F, 3268.1F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 38, 0x1c72);
        event_turn_by(event_get_player(5), 0x3d9, 0x3dfd, 0);
        event_look_at_task(event_get_player(5), event_get_player(3), 0.0F, 4.0F,
                           0.0F);
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_13);
        event_voice_play(0x507);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(5));
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 3.892F, 1550.1043F,
                             3235.406F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf900, 0xc800, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_voice_play(0x43a);
        event_message_show(TEXT_2);
        event_play_action(event_eggmobile_get_task(),
                          &ev004b_eggmobile_action_3, &event_eggmobile_texlist,
                          2.0F, 0, 5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 14.495F, 1548.1828F,
                             3233.1963F);
        event_wait_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev004b_eggmobile_action_0, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_look_clear(event_get_player(5));
        event_look_at(event_get_player(5), 39.0F, 1561.7F, 3226.2F);
        event_turn_by(event_get_player(5), 0x3d9, 0x3dfd, 0);
        event_set_ang(ev004b_bird, 0, 0x4fe1, 0);
        event_camera_follow_clear();
        event_set_pos(event_get_player(0), -8.5F, 1526.0F, 3336.6F);
        event_clear_motions(event_get_player(0));
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        ev004b_claw =
            event_claw_create(-27.57F, 1558.18F, 3221.0F, 0, 0, -0x4000);
        event_claw_set_rates(ev004b_claw, 0x900, 0x600);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe79d, 0x3047, -0x100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 36.25F, 1557.0F, 3247.65F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 58, 0xa9d, 0x3047, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 58, 47.53F, 1526.6F, 3250.95F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 58, 0x2aab);
        event_message_close();
        event_sound_start(0, 0x4c7, 1800);
        event_move_turn(ev004b_claw, -27.0F, 1558.2F, 3221.0F, -4.5F, 1525.0F,
                        3242.5F, 0, 0, -0x4000, -0x1000, 0, -0x2500, 30);
        event_wait(20);
        event_claw_set_mode(ev004b_claw, CLAW_OPEN);
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(5), &ev004b_amy_action_2,
                          &amy_texlist, 2.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(5), -15.0F, 1525.687F, 3245.5F, -20.0F,
                   1525.687F, 3249.0F, 20);
        event_follow(ev004b_bird, 0.0F, 0.0F, 0.0F, 10, event_get_player(5));
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 45.7F, 1526.83F, 3255.6F);
        event_claw_set_mode(ev004b_claw, CLAW_CLOSE);
        event_play_action(event_get_player(5), &ev004b_amy_action_0,
                          &amy_texlist, 1.0F, 0, 2);
        event_play_action(event_get_player(5), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_14);
        event_voice_play(0x5b4);
        event_message_show(TEXT_3);
        event_move_stop(ev004b_bird);
        event_set_pos(ev004b_bird, 42.55F, 1536.0F, 3227.0F);
        event_move_turn(ev004b_claw, -4.5F, 1525.0F, 3242.5F, 8.0F, 1563.0F,
                        3221.0F, -0x1000, 0, -0x2500, -0x1000, 0, -0x2500, 28);
        event_wait(50);
        event_sound_stop(0);
        event_message_close();
        event_turn_by(event_get_player(3), 0, 0x2e00, 0);
        event_look_clear(event_get_player(3));
        event_look_at(event_get_player(3), 39.0F, 1561.7F, 3226.2F);
        event_look_clear(event_get_player(5));
        event_set_pos(event_get_player(0), -5.0F, 1526.0F, 3304.8F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004b_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(24);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(3);
        event_clear_motions(ev004b_bird);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(ev004b_bird, &ev004b_bird_action_2,
                          &ev004b_bird_texlist, 1.0F, EVENT_REPEAT, 3);
        event_play_action(event_eggmobile_get_task(),
                          &ev004b_eggmobile_action_4, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(3);
        event_eggmobile_slide_to(58.8F, 1535.0F, 3230.08F, 2);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xea9d, 0xca47, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -34.57F, 1564.1F, 3256.9F);
        event_set_pos(event_get_player(5), -12.6F, 1526.0F, 3233.0F);
        event_set_ang(event_get_player(5), 0, 0x4000, 0);
        event_set_pos(event_get_player(0), 3.43F, 1526.4F, 3240.62F);
        event_set_ang(event_get_player(0), 0, 0x498d, 0);
        event_set_pos(event_get_player(3), -6.3463F, 1525.7272F, 3258.1814F);
        event_set_ang(event_get_player(3), 0, 0x4400, 0);
        event_play_action(event_get_player(3), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(2);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(20);
        event_sound_play(0x4c9, 0, 0, 0);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev004b_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, 0, 0);
        event_move(ev004b_bird, 42.55F, 1536.0F, 3227.0F, -12.864F, 1555.828F,
                   3227.0F, 60);
        event_set_pos(ev004b_emerald, 44.5F, 1527.5F, 3224.0F);
        event_wait(18);
        event_move_stop_all();
        event_wait(1);
        event_set_ang(ev004b_bird, 0xa00, 0xbf00, 0);
        event_clear_motions(ev004b_bird);
        event_play_action(ev004b_bird, &ev004b_bird_action_1,
                          &ev004b_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev004b_bird, 61.1F, 1580.0F, 3203.0F, -12.6F, 1526.0F,
                   3233.0F, 250);
        event_wait(1);
        /* @hack redundant: a call that never runs, passing 58.8, which SHC
         * still counts when it allocates registers. with it, the three
         * event_look_at calls at 90.0, 1567.7 and 3270.0 further down take x,
         * y and z from FR14, FR13 and FR12, as the image does (0x0cb81718 to
         * 0x0cb8173e). without it, they take them from FR13, FR12 and FR14,
         * and 9 of 2344 words differ. with 3.25 in place of 58.8, y and z
         * trade, and 6 of 2344 words differ */
        if (0) {
            event_floor_create(NULL, 58.8F);
        }
        event_look_at(event_get_player(3), 58.8F, 1545.0F, 3230.08F);
        event_look_at(event_get_player(5), 58.8F, 1545.0F, 3230.08F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x238e);
        event_clear_motions(event_eggmobile_get_task());
        event_clear_motions(ev004b_emerald);
        event_play_action(event_eggmobile_get_task(),
                          &ev004b_eggmobile_action_1, &event_eggmobile_texlist,
                          0.2F, EVENT_REPEAT, 0);
        event_play_action(ev004b_emerald, &ev004b_emerald_action,
                          &ev004b_emerald_texlist, 0.2F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe7a0, 0x5450, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 73.546F, 1566.08F,
                             3216.552F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 67.0F, 1561.0F, 3220.1F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_14);
        event_voice_play(0x508);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev004b_eggmobile_action_1, &event_eggmobile_texlist,
                          0.4F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x31c7);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x39d, 0xd647, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -17.9F, 1533.323F, 3247.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0xe9d, 0xe347, 0x500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -1.4646F, 1527.95F,
                             3254.62F);
        event_talk_stop(event_get_player(5));
        event_talk_start(event_get_player(5), TEXT_15);
        event_sound_play(0x4cc, 0, 0, 0);
        event_voice_play(0x5b6);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(5));
        event_message_close();
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1b00, 0xbb00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 50.8F, 1545.0F, 3228.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1b00, 0xd000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 52.22F, 1545.0F, 3234.0F);
        event_voice_play(0x43d);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_wait(1);
        ev004b_floor = event_floor_create(event_get_player(4), 1541.0F);
        event_wait(1);
        event_set_pos(event_get_player(4), 0.0F, 1556.0F, 3459.0F);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev004b_eggmobile_action_3, &event_eggmobile_texlist,
                          0.4F, 0, 6);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 55.56F, 1547.075F,
                             3231.04F);
        event_voice_play(0x43e);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_kill_task(&ev004b_emerald);
        event_message_close();
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -0.41781F, 1555.9421F,
                             3449.448F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1200, 0x8400, 0);
        event_sound_play(0x4ca, 0, 0, 0);
        event_voice_play(0x6e7);
        event_message_show(TEXT_8);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 21.47F, 1525.46F, 3417.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1c00, 0x6300, 0);
        event_wait(26);
        event_set_pos(ev004b_claw, -34.0F, 1420.0F, 3255.0F);
        event_set_pos(event_get_player(0), -1.0F, 1526.0F, 3275.864F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_pos(event_get_player(3), 10.26F, 1526.0F, 3256.78F);
        event_set_ang(event_get_player(3), 0, 0, 0);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(3));
        event_look_clear(event_get_player(5));
        event_turn_by(event_get_player(5), 0, 0x300, 0);
        event_eggmobile_turn(0, 0x3400, 0, 10, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 12.4F, 1525.46F, 3409.97F);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_fall_start_action,
                          &e102_texlist, 1.0F, 0, 0);
        EVENT_KILL(ev004b_floor);
        event_move(event_get_player(4), 0.0F, 1546.0F, 3459.0F, 0.0F, 1560.0F,
                   3426.0F, 25);
        event_wait(25);
        event_play_action(event_get_player(4), &e102_hover_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 8);
        event_move(event_get_player(4), 0.0F, 1560.0F, 3426.0F, 0.0F, 1560.0F,
                   3423.0F, 15);
        event_wait(15);
        event_move(event_get_player(4), 0.0F, 1560.0F, 3423.0F, 0.0F, 1535.0F,
                   3423.0F, 90);
        event_wait(60);
        event_clear_motions(event_get_player(4));
        event_wait(75);
        event_set_pos(event_get_player(4), 0.0F, 1524.0F, 3415.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.0F, 1539.4F, 3432.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x400, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x216c);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -1.0F, 1528.46F, 3431.08F);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.68891F, 1550.277F,
                             3246.7522F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf380, 0x8000, 0);
        event_voice_play(0x509);
        event_message_show(TEXT_9);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, -1.42631F, 1535.3642F,
                             3259.5923F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0xfd80, 0x7f00, 0);
        event_wait(1);
        event_voice_play(0x50a);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xea92, 0x7fd6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 1550.18F, 3398.0F);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(0x6e8);
        event_message_show(TEXT_11);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x271c);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf992, 0xf0d6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.37F, 1532.78F, 3298.74F);
        event_voice_wait();
        event_wait(10);
        event_voice_play(0x441);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x392, 0xded6, 0);
        /* @hack redundant: a call that never runs, passing 94.46, which SHC
         * still counts when it allocates registers. with it, the three
         * event_look_at calls below take x, y and z from FR14, FR13 and FR12,
         * as the image does (0x0cb81718 to 0x0cb8173e). without it, they take
         * them from FR13, FR12 and FR14, and 9 of 2344 words differ. with 3.25
         * in place of 94.46, y and z trade, and 6 of 2344 words differ */
        if (0) {
            event_floor_create(NULL, 94.46F);
        }
        event_look_at(event_get_player(0), 90.0F, 1567.7F, 3270.0F);
        event_look_at(event_get_player(3), 90.0F, 1567.7F, 3270.0F);
        event_look_at(event_get_player(5), 90.0F, 1567.7F, 3270.0F);
        event_eggmobile_fly_turn(94.46F, 1561.5475F, 3267.0F, 0, 0x8200, 0, 70,
                                 1);
        event_wait(72);
        event_message_close();
        event_sound_start(1, 0x4cb, 1800);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_wait(1);
        event_eggmobile_fly_to(126.0F, 1533.0F, 2759.857F, 80);
        event_wait(37);
        event_sound_fade_volume(1, -80, 35);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(3));
        event_look_clear(event_get_player(5));
        event_play_action(event_get_player(3), &ev004b_sonic_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(0), &ev004b_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -9.37F, 1532.78F, 3298.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x392, 0xeed6, 0xfc00);
        event_wait(70);
        event_sound_stop(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 14.37F, 1531.0F, 3380.35F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x992, 0x61d6, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 25.04F, 1529.1366F,
                             3393.19F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x992, 0x5bd6, 0);
        event_wait(20);
        event_play_action(event_get_player(4), &e102_slide_stop_action,
                          &e102_texlist, 0.7F, 0, 16);
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.42F, 1528.02F, 3284.726F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x792, 0x26d6, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 11.6F, 1527.65F, 3280.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x792, 0x30d6, 0);
        event_wait(100);
        event_look_clear(event_get_player(0));
        event_wait(10);
        break;
    case EVENT_END:
        event_flags[FLAG_LIFT] = LIFT_LOW_NOW;
        event_sound_stop_all();
        event_move_stop_all();
        EVENT_KILL(ev004b_floor);
        event_kill_player(3);
        event_kill_player(4);
        event_kill_player(5);
        event_reset_player(0);
        event_kill_task(&ev004b_emerald);
        event_kill_task(&ev004b_bird);
        EVENT_KILL(ev004b_claw);
        event_eggmobile_kill();
        event_flags[FLAG_GATE] = GATE_SHUT_NOW;
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev004b_textures[] = {
    {"SONIC", &sonic_texlist},
    {"AMY", &amy_texlist},
    {"E102", &e102_texlist},
    {"M_EM_SKY", &ev004b_emerald_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"MGHAND", &event_claw_texlist},
    {"VER2_WING", &ev004b_bird_texlist},
    {NULL, NULL},
};
