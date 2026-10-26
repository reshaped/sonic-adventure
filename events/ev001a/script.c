/* EV001A, Sonic's story: Eggman takes the emerald Amy's bird carries and sets
 * E-102 on Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_lift.h"
#include "adv0100/gate.h"
#include "ev001a/script.h"
#include "ev001a/sonic_action_1.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/e102.h"
#include "player/tails.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/claw.h"
#include "shared/claw_textures.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 21
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 30
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 33
#define TEXT_9_LENGTH 45
#define TEXT_10_LENGTH 25
#define TEXT_11_LENGTH 33
#define TEXT_12_LENGTH 18
#define TEXT_13_LENGTH 2
#define TEXT_14_LENGTH 1
#define TEXT_15_LENGTH 1
#include "ev001a/script/text.inc"

/* kill the task in slot, if any, and clear slot */
/* @hack spelling: slot tested in an if statement, where src/event/play.h's
 * EVENT_KILL tests it in an expression. with EVENT_KILL at KILL's five uses,
 * ev001a_script runs 48 bytes long, and 2492 of 2599 words differ */
/* @hack spelling: the kill and the clear as one expression, as EVENT_KILL has
 * them. SHC then loads event_flags's address into R13 and event_reset_player's
 * into R12 before the switch, at 0x0cb8002a and 0x0cb8002c. as two statements,
 * the two addresses trade registers, ev001a_script runs 48 bytes long, and 2499
 * of 2599 words differ */
#define KILL(slot)                                                             \
    do {                                                                       \
        if ((slot) != NULL) {                                                  \
            task_kill(slot), (slot) = NULL;                                    \
        }                                                                      \
    } while (0)

extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION sonic_sprint_action;
extern NJS_ACTION tails_sprint_action;
extern NJS_ACTION tails_turn_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_fall_start_action;
extern NJS_ACTION e102_hover_action;
extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev001a_tails_action;
extern NJS_ACTION ev001a_amy_action_0;
extern NJS_ACTION ev001a_amy_action_1;
extern NJS_ACTION ev001a_amy_action_2;
extern NJS_ACTION ev001a_sonic_action_0;
extern NJS_TEXLIST ev001a_emerald_texlist;
extern NJS_ACTION ev001a_emerald_action;
extern NJS_TEXLIST event_eggmobile_texlist;

static struct task *ev001a_bird = NULL;
static struct task *ev001a_emerald = NULL;
static struct task *ev001a_claw = NULL;
static struct task *ev001a_e102_fade = NULL;
static struct task *ev001a_fade = NULL;
static struct task *ev001a_floor = NULL;

/* Sonic's pad recordings, one key a step */

/* a run once the claw has the bird, with two jumps on the way. A is held
 * for the 34th and 35th keys, and for the 42nd to 45th */
static const struct event_pad_key ev001a_run_keys[105] = {
#include "ev001a/script/ev001a_run_keys.inc"
};

static const struct event_pad_recording ev001a_run = {
    ev001a_run_keys, sizeof(ev001a_run_keys) / sizeof(ev001a_run_keys[0])};

/* a run with one jump, which nothing plays. A is held for the 4th to 21st
 * keys */
static const struct event_pad_key ev001a_unplayed_run_keys[74] = {
#include "ev001a/script/ev001a_unplayed_run_keys.inc"
};

/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct event_pad_recording ev001a_unplayed_run = {
    ev001a_unplayed_run_keys,
    sizeof(ev001a_unplayed_run_keys) / sizeof(ev001a_unplayed_run_keys[0])};

/* two jumps on the spot. A is held for the 5th to 9th keys, and for the
 * 14th to 19th. the stick is never pushed */
static const struct event_pad_key ev001a_jump_keys[60] = {
#include "ev001a/script/ev001a_jump_keys.inc"
};

static const struct event_pad_recording ev001a_jumps = {
    ev001a_jump_keys, sizeof(ev001a_jump_keys) / sizeof(ev001a_jump_keys[0])};

void ev001a_script(int phase)
{
    /* @hack spelling: EVENT_PLAY's stores to event_flags[FLAG_LIFT] go through
     * lift_flag, a local. SHC then loads event_flags's address into R13 before
     * the switch, and adds FLAG_LIFT to it in EVENT_PLAY, at 0x0cb8002a and
     * 0x0cb800a6. stored in place, each store loads the flag's address from a
     * pool word of its own, ev001a_script runs 32 bytes long, and 1778 of 2599
     * words differ */
    Uint8 *lift_flag;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0016.mlt");
        event_sound_init(5);
        stream_request_music(MUSIC_EGCARER1);
        event_flags[FLAG_SONIC_EC_EGG_LIFT] = 1;
        /* @hack spelling: lift_flag takes &event_flags[FLAG_LIFT] inside the
         * first store through it. SHC then stores through R13, which holds the
         * address, as MOV.B R3,@R13 at 0x0cb800ac does. as two statements, SHC
         * stores through a copy in R3, ev001a_script runs 4 bytes long, and
         * 2421 of 2599 words differ */
        *(lift_flag = &event_flags[FLAG_LIFT]) = LIFT_LOW_NOW;
        event_place_actor(&ev001a_emerald, -547.33F, -0.5F, 973.9F, 0, 0, 0);
        event_set_flags(ev001a_emerald, EVENT_IGNORE_GROUND);
        event_eggmobile_create(22.8F, 1535.0F, 3231.6F, 0, 0x900, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_set_pos(event_get_player(0), -4.176F, 1495.0F, 3444.76F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(
            3, tails_step, event_get_player(0)->work->pos.x + 8.0F,
            event_get_player(0)->work->pos.y - 4.0F,
            event_get_player(0)->work->pos.z + 8.0F, 0, 0x8000, 0);
        event_reset_player(3);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev001a_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(4, e102_step, -254.8F, 1525.67F, 3754.6F, 0, 0x8000,
                            0);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_stand_action,
                          &e102_texlist, 0.1F, EVENT_REPEAT, 0);
        event_create_player(5, amy_step, -15.0F, 1525.687F, 3245.5F, 0, 0x4fe0,
                            0);
        ev001a_fade = event_fade_create(1.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_place_actor(&ev001a_bird, 0.0F, 5.5F, 0.0F, 0, 0x8000, 0);
        event_wait(1);
        event_set_flags(ev001a_bird, EVENT_IGNORE_GROUND);
        event_set_pos(ev001a_bird, event_get_player(5)->work->pos.x,
                      event_get_player(5)->work->pos.y,
                      event_get_player(5)->work->pos.z);
        event_set_ang(ev001a_bird, 0, 0x2e1c, 0);
        event_clear_motions(ev001a_bird);
        event_clear_motions(event_get_player(5));
        event_play_action(ev001a_bird, &ev001a_bird_action_0,
                          &ev001a_bird_texlist, 1.0F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(5), &ev001a_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xa90, 0xfe0, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1.7386F, 1496.6F, 3458.6F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 1.7386F, 1546.6F, 3458.6F);
        *lift_flag = LIFT_RISE;
        event_wait(15);
        if (ev001a_fade != NULL) {
            event_fade_set_speed(ev001a_fade, -1.0F / 15.0F);
        }
        event_wait(20);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_camera_follow_clear();
        event_set_pos(event_get_player(0), 0.0F, 1525.7F, 3443.76F);
        event_set_pos(event_get_player(3), 6.3F, 1525.7F, 3041.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x6f00, 0x100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 52.855F, 1527.0F, 3423.787F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 15.58F, 1527.87F,
                             3406.753F);
        event_wait(68);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 12.85F, 1527.94F, 3405.5F);
        event_wait(60);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 59, 0xf000, 0x800, 0xfc00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 59, 5.4556F, 1555.6F, 3455.64F);
        event_wait(56);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev001a_jumps);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xec00, 0xf900, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -7.4F, 1579.79F, 3476.848F);
        event_wait(52);
        event_set_pos(event_get_player(0), -0.5F, 1526.0F, 3411.0F);
        event_play_action(event_get_player(0), &sonic_sprint_action,
                          &sonic_texlist, 2.7F, EVENT_REPEAT, 8);
        event_set_pos(event_get_player(3), 9.5F, 1526.0F, 3421.0F);
        event_play_action(event_get_player(3), &tails_sprint_action,
                          &tails_texlist, 2.5F, EVENT_REPEAT, 8);
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
        event_look_at_task(event_get_player(5), event_get_player(0), 0.0F, 4.0F,
                           0.0F);
        event_talk_stop(event_get_player(5));
        event_voice_play(0x5b3);
        event_talk_start(event_get_player(5), TEXT_13);
        event_message_show(TEXT_1);
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0xa00, 0xda50, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -30.6F, 1526.15F, 3262.62F);
        event_voice_wait();
        event_message_close();
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x271c);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc20, 0xe150, 0xfb00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 19.64F, 1546.0F, 3238.444F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xe20, 0xc050, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 14.55F, 1545.3F, 3230.27F);
        event_voice_play(0x43a);
        event_message_show(TEXT_2);
        event_play_action(event_eggmobile_get_task(),
                          &ev001a_eggmobile_action_3, &event_eggmobile_texlist,
                          2.0F, 0, 5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0xddb, 0xbbb7, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 15.13F, 1545.6F, 3229.38F);
        event_wait_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev001a_eggmobile_action_0, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_look_clear(event_get_player(5));
        event_look_at(event_get_player(5), 39.0F, 1561.7F, 3226.2F);
        event_turn_by(event_get_player(5), 0x3d9, 0x3dfd, 0);
        event_set_ang(ev001a_bird, 0, 0x4fe1, 0);
        event_camera_follow_clear();
        event_set_pos(event_get_player(0), -8.5F, 1526.0F, 3336.6F);
        event_clear_motions(event_get_player(0));
        *lift_flag = LIFT_LOW_NOW;
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        ev001a_claw =
            event_claw_create(-27.57F, 1558.18F, 3221.0F, 0, 0, -0x4000);
        event_claw_set_rates(ev001a_claw, 0x900, 0x600);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe79d, 0x3047, -0x100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 36.25F, 1557.0F, 3247.65F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 58, 0xa9d, 0x3047, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 58, 47.53F, 1526.6F, 3250.95F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 58, 0x2aab);
        event_message_close();
        event_sound_start(0, 0x4ca, 1800);
        event_move_turn(ev001a_claw, -27.0F, 1558.2F, 3221.0F, -4.5F, 1525.0F,
                        3242.5F, 0, 0, -0x4000, -0x1000, 0, -0x2500, 30);
        event_wait(20);
        event_claw_set_mode(ev001a_claw, CLAW_OPEN);
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(5), &ev001a_amy_action_2,
                          &amy_texlist, 2.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(5), -15.0F, 1525.687F, 3245.5F, -20.0F,
                   1525.687F, 3249.0F, 20);
        event_follow(ev001a_bird, 0.0F, 0.0F, 0.0F, 10, event_get_player(5));
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 45.7F, 1526.83F, 3255.6F);
        event_claw_set_mode(ev001a_claw, CLAW_CLOSE);
        event_play_action(event_get_player(5), &ev001a_amy_action_0,
                          &amy_texlist, 1.0F, 0, 2);
        event_play_action(event_get_player(5), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_voice_play(0x5b4);
        event_talk_start(event_get_player(5), TEXT_14);
        event_message_show(TEXT_3);
        event_move_stop(ev001a_bird);
        event_set_pos(ev001a_bird, 42.55F, 1536.0F, 3227.0F);
        event_move_turn(ev001a_claw, -4.5F, 1525.0F, 3242.5F, 8.0F, 1563.0F,
                        3221.0F, -0x1000, 0, -0x2500, -0x1000, 0, -0x2500, 28);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev001a_run);
        event_wait(30);
        event_sound_stop(0);
        event_wait(50);
        event_message_close();
        event_turn_by(event_get_player(0), 0, 0x2e00, 0);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), 39.0F, 1561.7F, 3226.2F);
        event_look_clear(event_get_player(5));
        event_set_pos(event_get_player(3), -5.0F, 1526.0F, 3304.8F);
        event_clear_motions(event_get_player(3));
        event_wait(24);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(3);
        event_clear_motions(ev001a_bird);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(ev001a_bird, &ev001a_bird_action_2,
                          &ev001a_bird_texlist, 1.0F, EVENT_REPEAT, 3);
        event_play_action(event_eggmobile_get_task(),
                          &ev001a_eggmobile_action_4, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(3);
        event_eggmobile_slide_to(58.8F, 1535.0F, 3230.08F, 2);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xea9d, 0xca47, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -34.57F, 1564.1F, 3256.9F);
        event_sound_start(1, 0x4cc, 1800);
        event_set_pos(event_get_player(5), -12.6F, 1526.0F, 3233.0F);
        event_set_ang(event_get_player(5), 0, 0x4000, 0);
        event_set_pos(event_get_player(0), 3.43F, 1526.4F, 3240.62F);
        event_set_ang(event_get_player(0), 0, 0x498d, 0);
        event_wait(1);
        event_wait(1);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_play_action(event_get_player(3), &tails_sprint_action,
                          &tails_texlist, 1.5F, EVENT_REPEAT, 8);
        event_run_to_speed(event_get_player(3), -6.4F, 1529.3F, 3257.8F, 1.5F,
                           2.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xfb9d, 0xca50, 0x80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -48.32F, 1540.76F,
                             3256.445F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 120, 0x2000);
        event_run_wait(event_get_player(3));
        event_play_action(event_get_player(3), &tails_turn_action,
                          &tails_texlist, 1.0F, 0, 8);
        event_wait_motions(event_get_player(3));
        event_set_ang(event_get_player(3), 0, 0x5100, 0);
        event_play_action(event_get_player(3), &ev001a_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(10);
        event_sound_stop(1);
        event_wait(20);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev001a_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, 0, 0);
        event_move(ev001a_bird, 42.55F, 1536.0F, 3227.0F, -12.864F, 1555.828F,
                   3227.0F, 60);
        event_set_pos(ev001a_emerald, 44.5F, 1527.5F, 3224.0F);
        event_wait(18);
        event_move_stop_all();
        event_wait(1);
        event_set_ang(ev001a_bird, 0xa00, 0xbf00, 0);
        event_clear_motions(ev001a_bird);
        event_play_action(ev001a_bird, &ev001a_bird_action_1,
                          &ev001a_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev001a_bird, 61.1F, 1580.0F, 3203.0F, -12.6F, 1526.0F,
                   3233.0F, 250);
        event_wait(1);
        /* @hack redundant: a call of event_floor_create with 1545.0 that never
         * runs. with it, SHC loads 3459.0 into FR6 at 0x0cb8152e, for the
         * event_move that lifts E-102 from 1520.0 to 1546.0. that call passes
         * 3459.0 twice, and SHC copies it to FR9 at 0x0cb81538. without it, SHC
         * loads 3459.0 into FR12 before the event_get_player call at
         * 0x0cb81522, ev001a_script runs 4 bytes long, and 728 of 2599 words
         * differ */
        if (0) {
            event_floor_create(NULL, 1545.0F);
        }
        event_look_at(event_get_player(3), 58.8F, 1545.0F, 3230.08F);
        event_look_at(event_get_player(5), 58.8F, 1545.0F, 3230.08F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x238e);
        event_clear_motions(event_eggmobile_get_task());
        event_clear_motions(ev001a_emerald);
        event_play_action(event_eggmobile_get_task(),
                          &ev001a_eggmobile_action_1, &event_eggmobile_texlist,
                          0.2F, EVENT_REPEAT, 0);
        event_play_action(ev001a_emerald, &ev001a_emerald_action,
                          &ev001a_emerald_texlist, 0.2F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe7a0, 0x5450, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 73.546F, 1566.08F,
                             3216.552F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 67.0F, 1561.0F, 3220.1F);
        event_sound_play(0x4ce, 0, 0, 0);
        event_voice_play(0x5b5);
        event_talk_start(event_get_player(3), TEXT_15);
        event_message_show(TEXT_4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 65.0F, 1559.5F, 3221.17F);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev001a_eggmobile_action_1, &event_eggmobile_texlist,
                          0.4F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x31c7);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x39d, 0xd647, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -17.9F, 1533.323F, 3247.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0xe9d, 0xe347, 0x500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -1.4646F, 1527.95F,
                             3254.62F);
        event_voice_play(0x5b6);
        event_message_show(TEXT_5);
        event_wait(30);
        event_message_close();
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1b00, 0xbb00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 50.8F, 1545.0F, 3228.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1b00, 0xd000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 52.22F, 1545.0F, 3234.0F);
        event_voice_play(0x43d);
        event_message_show(TEXT_6);
        event_set_pos(event_get_player(4), 0.0F, 1495.0F, 3469.0F);
        event_wait(70);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev001a_eggmobile_action_3, &event_eggmobile_texlist,
                          1.0F, 0, 3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 55.56F, 1547.075F,
                             3231.04F);
        event_voice_play(0x43e);
        event_message_show(TEXT_7);
        event_voice_wait();
        event_kill_task(&ev001a_emerald);
        ev001a_e102_fade = event_fade_create(1.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 1511.0F, 3461.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0x8000, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        event_message_close();
        event_wait(50);
        if (ev001a_e102_fade != NULL) {
            event_fade_set_speed(ev001a_e102_fade, -0.1F);
        }
        event_sound_play(0x4cd, 0, 0, 0);
        event_wait(2);
        if (ev001a_e102_fade != NULL) {
            event_fade_set_speed(ev001a_e102_fade, 0.0F);
        }
        event_wait(38);
        event_voice_play(0x6e7);
        event_message_show(TEXT_8);
        event_wait(60);
        event_message_close();
        event_set_pos(event_get_player(4), -254.8F, 1525.67F, 3754.6F);
        *lift_flag = LIFT_RISE;
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.2F, 1521.26F, 3451.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf500, 0x5700, 0xfc00);
        /* @hack spelling: the test and the call as one expression, as
         * EVENT_KILL has them. SHC then loads -1.0 straight into FR4, at
         * 0x0cb814ee. as an if, SHC loads -1.0 into FR12 and keeps it there for
         * the calls below that pass it, ev001a_script runs 4 bytes short, and
         * 457 of 2599 words differ */
        ev001a_e102_fade != NULL ? event_fade_set_speed(ev001a_e102_fade, -1.0F)
                                 : (void)0;
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(37);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -7.08F, 1534.76F, 3446.26F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x9d00, 0x300);
        event_move(event_get_player(4), 0.0F, 1520.0F, 3459.0F, 0.0F, 1546.0F,
                   3459.0F, 59);
        event_wait(35);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 21.47F, 1525.46F, 3417.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1c00, 0x6300, 0);
        event_wait(26);
        ev001a_floor = event_floor_create(event_get_player(4), 1541.0F);
        event_set_pos(ev001a_claw, -34.0F, 1420.0F, 3255.0F);
        event_set_pos(event_get_player(0), -1.0F, 1526.0F, 3275.864F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_pos(event_get_player(3), 10.26F, 1526.0F, 3256.78F);
        event_set_ang(event_get_player(3), 0, 0, 0);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(3));
        event_look_clear(event_get_player(5));
        event_turn_by(event_get_player(5), 0, 0x300, 0);
        event_eggmobile_turn(0, 0x3400, 0, 10, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 12.4F, 1525.46F, 3409.97F);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_fall_start_action,
                          &e102_texlist, 1.0F, 0, 0);
        KILL(ev001a_floor);
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
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -6.6187F, 1529.36F,
                             3259.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x400, 0x84c0, 0x200);
        event_voice_play(0x43f);
        event_message_show(TEXT_9);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 4.31F, 1529.0F, 3258.33F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0x400, 0x7cc0, 0x200);
        event_wait(1);
        event_voice_wait();
        event_voice_play(0x440);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xea92, 0x7fd6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 1550.18F, 3398.0F);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(0x6e8);
        event_message_show(TEXT_11);
        event_voice_wait();
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x271c);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf992, 0xf0d6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.37F, 1532.78F, 3298.74F);
        event_voice_play(0x441);
        event_message_show(TEXT_12);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x392, 0xded6, 0);
        event_look_at(event_get_player(0), 90.0F, 1567.7F, 3270.0F);
        event_look_at(event_get_player(3), 90.0F, 1567.7F, 3270.0F);
        event_look_at(event_get_player(5), 90.0F, 1567.7F, 3270.0F);
        event_eggmobile_fly_turn(94.46F, 1561.5475F, 3267.0F, 0, 0x8200, 0, 70,
                                 1);
        /* @hack redundant: a call of event_floor_create with 1567.7 that
         * never runs. with it, the three event_look_at calls above take 90.0
         * from FR13, 1567.7 from FR15 and 3270.0 from FR12, at 0x0cb819ee to
         * 0x0cb819f4. without it, the three constants rotate among those
         * registers, and 9 of 2599 words differ */
        if (0) {
            event_floor_create(NULL, 1567.7F);
        }
        event_play_action(event_get_player(0), &ev001a_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(72);
        event_message_close();
        event_sound_start(2, 0x4cf, 1800);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_wait(1);
        event_eggmobile_fly_to(126.0F, 1533.0F, 2759.857F, 80);
        event_wait(37);
        event_sound_fade_volume(2, -80, 35);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(3));
        event_look_clear(event_get_player(5));
        event_play_action(event_get_player(0), &ev001a_sonic_action_0,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -9.37F, 1532.78F, 3298.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x392, 0xeed6, 0xfc00);
        event_wait(70);
        event_sound_stop(2);
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
        break;
    case EVENT_END:
        event_move_stop_all();
        event_camera_release();
        event_pads_on();
        KILL(ev001a_floor);
        event_set_pos(event_get_player(0), 0.0F, 1525.0F, 3404.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_reset_player(0);
        event_kill_player(3);
        event_kill_player(4);
        event_kill_player(5);
        event_kill_task(&ev001a_bird);
        event_kill_task(&ev001a_emerald);
        KILL(ev001a_claw);
        event_eggmobile_kill();
        KILL(ev001a_e102_fade);
        KILL(ev001a_fade);
        event_sound_stop_all();
        event_flags[FLAG_LIFT] = LIFT_LOW_NOW;
        event_flags[FLAG_GATE] = GATE_SHUT_NOW;
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev001a_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"VER2_WING", &ev001a_bird_texlist},
    {"MGHAND", &event_claw_texlist},
    {"MILES", &tails_texlist},
    {"E102", &e102_texlist},
    {"AMY", &amy_texlist},
    {"M_EM_SKY", &ev001a_emerald_texlist},
    {NULL, NULL},
};
