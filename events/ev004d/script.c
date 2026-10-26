/* EV004D, Tails' story: Tails flies Amy to Station Square, where Eggman's
 * missile falls without going off */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/traffic.h"
#include "ev004d/eggmobile_action_3.h"
#include "ev004d/missile.h"
#include "ev004d/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/eggmobile.h"
#include "shared/move.h"
#include "shared/shapes.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 47
#define TEXT_2_LENGTH 43
#define TEXT_3_LENGTH 65
#define TEXT_4_LENGTH 32
#define TEXT_5_LENGTH 17
#define TEXT_6_LENGTH 18
#define TEXT_7_LENGTH 25
#define TEXT_8_LENGTH 48
#define TEXT_9_LENGTH 17
#define TEXT_10_LENGTH 63
#define TEXT_11_LENGTH 21
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 3
#define TEXT_14_LENGTH 2
#define TEXT_15_LENGTH 24
#define TEXT_16_LENGTH 3
#define TEXT_17_LENGTH 9
#include "ev004d/script/text.inc"

/* the pos.y ev004d_carry_create gives ev004d_carry */
#define CARRY_HEIGHT 5.0F

/* what ev004d_carry_step takes off Amy's y and z, so that she hangs
 * CARRY_HEIGHT - CARRY_DROP, 3, above Tails */
#define CARRY_DROP 2.0F
#define CARRY_BACK 3.5F

extern NJS_ACTION tails_run_action;
extern NJS_ACTION amy_stand_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_ACTION ev004d_tails_action_0;
extern NJS_ACTION ev004d_tails_action_1;
extern NJS_ACTION ev004d_tails_action_2;
extern NJS_ACTION ev004d_tails_action_3;
extern NJS_ACTION ev004d_tails_action_4;
extern NJS_ACTION ev004d_tails_action_5;
extern NJS_ACTION ev004d_tails_action_6;
extern NJS_ACTION ev004d_tails_action_7;
extern NJS_ACTION ev004d_amy_action_0;
extern NJS_ACTION ev004d_amy_action_1;
extern NJS_TEXLIST ev004d_bird_texlist;
extern NJS_ACTION ev004d_bird_action;
extern NJS_ACTION ev004d_eggmobile_action_0;
extern NJS_ACTION ev004d_eggmobile_action_1;
extern NJS_ACTION ev004d_eggmobile_action_2;

/* the path Tails flies down, carrying Amy. her bird starts along it 40
 * steps after him */
static struct event_path_point ev004d_flight_points[102] = {
#include "ev004d/script/ev004d_flight_points.inc"
};

static struct event_path ev004d_flight_path = {102, ev004d_flight_points};

static struct task *ev004d_bird = NULL;
static struct task *ev004d_missile = NULL;
static struct task *ev004d_carry = NULL;

/* hold Amy, player 2, at Tails' pos plus the task's, less CARRY_DROP in y
 * and CARRY_BACK in z. her ang.y is Tails' plus the task's. Tails is player
 * 0. nothing checks that either still exists, so both must outlive the task */
static void ev004d_carry_step(struct task *task)
{
    struct task_work *tails = event_get_player(0)->work;
    struct task_work *amy = event_get_player(2)->work;
    struct task_work *work = task->work;

    /* @hack redundant: + 0.0F, an add that changes nothing, which SHC
     * compiles to FLDI0 FR3 and FADD FR3,FR2 at 0x0cb8002c, as the image
     * has them. without it, ev004d_carry_step runs 4 bytes short, and 31 of
     * 47 words differ */
    amy->pos.x = tails->pos.x + work->pos.x + 0.0F;
    amy->pos.y = tails->pos.y + work->pos.y - CARRY_DROP;
    amy->pos.z = tails->pos.z + work->pos.z - CARRY_BACK;
    amy->ang.y = tails->ang.y + work->ang.y;
}

/* create ev004d_carry. its ang.y is one read of Tails' ang.y less another,
 * which is 0, so Amy takes Tails' ang.y */
static void ev004d_carry_create(void)
{
    struct task_work *tails;
    struct task_work *tails_again;
    struct task_work *work;

    ev004d_carry = task_create(TASK_WORK, 5, ev004d_carry_step);
    tails = event_get_player(0)->work;
    tails_again = event_get_player(0)->work;
    work = ev004d_carry->work;
    work->pos.x = 0.0F;
    work->pos.y = CARRY_HEIGHT;
    work->pos.z = 0.0F;
    work->ang.y = tails_again->ang.y - tails->ang.y;
}

void ev004d_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_004D.mlt");
        event_sound_init(4);
        event_reset_player(0);
        event_flags[FLAG_TRAFFIC] = FALSE;
        event_set_pos(event_get_player(0), 340.01F, 0.01F, 1650.25F);
        event_set_ang(event_get_player(0), 0, 0x4300, 0);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_create_player(2, amy_step, 340.01F, 0.01F, 1630.25F, 0, 0x7800,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(2), 340.01F, 0.01F, 1630.25F);
        event_set_ang(event_get_player(2), 0, 0x7800, 0);
        event_place_actor(&ev004d_bird, 340.01F, 0.01F, 1630.25F,
                          event_get_player(2)->work->pos.x,
                          event_get_player(2)->work->pos.y,
                          event_get_player(2)->work->pos.z);
        event_wait(1);
        event_set_flags(ev004d_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev004d_bird, &ev004d_bird_action,
                          &ev004d_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_task(&ev004d_missile, ev004d_missile_create, 701.5F,
                         -1300.25F, 3900.2F, 0xfff0, 0, 0);
        event_wait(1);
        ev004d_missile_set_scale(ev004d_missile, 0.25F);
        event_eggmobile_create(233.3F, 82.8F, 1545.0F, 0, 0xd000, 0);
        event_wait(2);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(2);
        event_clear_motions(event_eggmobile_get_task());
        event_shapes_set_firm(event_get_player(2));
        event_wait(1);
        stream_request_music(MUSIC_S_SQUARE);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 136.0F, 30.2F, 1507.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1400, 0x2c00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 280, 76.78F, 35.2F, 1586.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 280, 0x139a, 0x6177, 0);
        ev004d_carry_create();
        event_wait(20);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(2), EVENT_IGNORE_GROUND);
        event_play_action(event_get_player(0), &ev004d_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev004d_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_path(event_get_player(0), &ev004d_flight_path, 0.2F, 0);
        event_wait(40);
        event_play_path(ev004d_bird, &ev004d_flight_path, 0.2F, 0);
        event_wait(220);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, -0.3F, 3.39F, 1510.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0xc9a, 0x6377, 0);
        event_wait(200);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -67.4F, 40.0F, 1497.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x347, 0x4ed7, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 170, -69.3F, 4.4F, 1496.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 170, 0x247, 0x3207, 0);
        event_clear_paths(event_get_player(0));
        if (ev004d_carry) {
            task_kill(ev004d_carry);
            ev004d_carry = NULL;
        }
        event_clear_paths(ev004d_bird);
        event_set_flags(event_get_player(2), EVENT_GROUND | EVENT_SLOPE);
        /* @hack redundant: a call that never runs, passing 0.0 three times,
         * which SHC still counts when it allocates registers. with it, SHC
         * keeps 0.0 in FR12 from 0x0cb80514, event_get_player in R12 from
         * 0x0cb800d4, and event_camera_set_pos in R13 from 0x0cb80332, as the
         * image does. without it, each call loads its own 0.0, R12 and R13
         * trade, ev004d_script runs 60 bytes short, and 3276 of 3700 words
         * differ */
        if (0) {
            event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 0.0F, 0.0F);
        }
        event_set_pos(event_get_player(2), -91.7F, 0.0F, 1479.1F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_flags(event_get_player(0), EVENT_GROUND | EVENT_SLOPE);
        event_set_pos(event_get_player(0), -82.2F, 0.0F, 1493.28F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004d_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_set_pos(ev004d_bird, -90.0F, 55.0F, 1520.5F);
        /* this call and the three like it below set the bird's angles from
         * Amy's pos, not her ang. each is her pos.x, pos.y or pos.z, offset or
         * not. event_set_ang's Angle parameters convert each Float */
        event_set_ang(ev004d_bird, event_get_player(2)->work->pos.x - 0x300,
                      event_get_player(2)->work->pos.y - 0x3000,
                      event_get_player(2)->work->pos.z);
        event_set_flags(ev004d_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev004d_bird, &ev004d_bird_action,
                          &ev004d_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev004d_bird, -90.0F, 54.0F, 1512.5F,
                   event_get_player(2)->work->pos.x + -0.5F,
                   event_get_player(2)->work->pos.y + 6.5F,
                   event_get_player(2)->work->pos.z + 6.0F, 200);
        event_wait(85);
        event_play_action(event_get_player(2), &ev004d_amy_action_1,
                          &amy_texlist, 1.2F, EVENT_REPEAT, 16);
        event_wait(75);
        event_play_action(event_get_player(0), &ev004d_tails_action_2,
                          &tails_texlist, 1.4F, 0, 0);
        event_play_action(event_get_player(0), &ev004d_tails_action_3,
                          &tails_texlist, 1.4F, EVENT_REPEAT, 8);
        event_wait(38);
        event_eggmobile_fly_turn(180.9F, 8.5F, 1303.0F, 0xe000, 0xd600, 0xe000,
                                 120, 0);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_set_ang(event_get_player(0), 0, 0x5900, 0);
        event_set_pos(ev004d_bird, event_get_player(2)->work->pos.x + -0.5F,
                      event_get_player(2)->work->pos.y + 4.5F,
                      event_get_player(2)->work->pos.z + 5.4F);
        event_set_ang(ev004d_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 0x2900,
                      event_get_player(2)->work->pos.z);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004d_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        /* @hack redundant: a call that never runs, passing -90.6, which SHC
         * still counts when it allocates registers. with it, SHC keeps the
         * camera's x, -90.6, in FR14 through the shake below, as the image
         * does (FMOV.S @R0,FR14 at 0x0cb80888). without it, FR14 holds the
         * camera's z, 1498.1, and 65 of 3700 words differ */
        if (0) {
            ev004d_missile_set_scale(NULL, -90.6F);
        }
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -90.6F, 5.4F, 1498.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc47, 0xc1d7, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0xa47, 0xcfd7, 0);
        event_wait(140);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_sound_play(25, 60, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 6.0F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.7F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.8F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.6F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.9F, 1498.1F);
        event_wait(1);
        event_sound_play(25, 40, 0, 0);
        vibration_start(0, -2, 50, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.7F, 1498.1F);
        event_wait(1);
        event_sound_play(16, 30, 0, 0);
        vibration_start(0, -1, 50, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.9F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.4F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.6F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.4F, 1498.1F);
        event_wait(1);
        vibration_start(0, -3, 60, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.5F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.4F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.5F, 1498.1F);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1, -90.6F, 5.4F, 1498.1F);
        event_wait(40);
        stream_stop_music();
        event_set_ang(event_get_player(0), 0, 0x3000, 0);
        event_look_clear(event_get_player(0));
        event_set_ang(event_get_player(2), 0, 0x1000, 0);
        event_set_pos(ev004d_bird, event_get_player(2)->work->pos.x + -2.0F,
                      event_get_player(2)->work->pos.y + 5.0F,
                      event_get_player(2)->work->pos.z + 4.5F);
        event_set_ang(ev004d_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 0x2800,
                      event_get_player(2)->work->pos.z);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -78.42F, 6.0F, 1500.88F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x147, 0x1907, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -75.2F, 6.0F, 1498.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x147, 0x2bd7, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, -76.02F, 6.02F, 1498.3F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_11);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_set_pos(ev004d_bird, event_get_player(2)->work->pos.x + -5.0F,
                      event_get_player(2)->work->pos.y + 4.5F,
                      event_get_player(2)->work->pos.z + 6.5F);
        event_set_ang(ev004d_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 0x3000,
                      event_get_player(2)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -102.8F, 14.7F, 1508.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb47, 0xdbd7, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -100.17F, 8.7F, 1512.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xfb47, 0xe8d7, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 6.0F, 5.0F,
                           0.0F);
        event_play_action(event_get_player(0), &ev004d_tails_action_4,
                          &tails_texlist, 0.8F, 0, 0);
        event_play_action(event_get_player(0), &ev004d_tails_action_5,
                          &tails_texlist, 1.0F, 0, 0);
        event_wait(50);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 40, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_wait_motions(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_run_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 11.2F, 0.0F, 1523.0F, 0.7F,
                           0.03F);
        event_wait(70);
        event_kill_player(2);
        event_kill_task(&ev004d_bird);
        event_run_clear(event_get_player(0));
        event_run_to_speed(event_get_player(0), 118.2F, 0.0F, 1465.0F, 0.3F,
                           0.03F);
        event_set_pos(event_get_player(0), 21.01F, 0.01F, 1523.25F);
        event_set_ang(event_get_player(0), 0, 0x5646, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 37.8F, 6.5F, 1518.5F);
        event_camera_follow(event_get_player(0));
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(1303);
        event_message_show(TEXT_1);
        event_wait(90);
        event_camera_follow_clear();
        event_message_close();
        event_camera_look_clear();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 120.9F, 4.6F, 1445.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xead, 0xf490, 0x400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 149.7F, 7.48F, 1455.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x7ad, 0xf790, 0x400);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 155.18F, 9.23F, 1430.86F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x5ad, 0xf790, 0x400);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 185.43F, 13.66F, 1289.67F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x8ad, 0x8a00, 0x100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 185.9F, 14.0F, 1291.57F);
        event_voice_play(1304);
        event_message_show(TEXT_2);
        event_voice_wait();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x11ad, 0x8f00, 0x100);
        event_voice_play(1305);
        event_message_show(TEXT_3);
        event_set_pos(event_get_player(0), 136.2F, 0.0F, 1460.86F);
        event_set_ang(event_get_player(0), 0, 0x7600, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004d_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(77);
        event_camera_set_pos(EVENT_EASE_LINEAR, 9, 186.22F, 14.42F, 1292.4F);
        event_sound_play(1225, 110, 0, 0);
        event_wait(7);
        stream_request_music(MUSIC_EVTBGM01);
        event_voice_wait();
        event_message_close();
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev004d_eggmobile_action_1, &event_eggmobile_texlist,
                          1.2F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 180.11F, 14.24F, 1281.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfdad, 0x8800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 193.84F, 18.27F, 1278.45F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xfdad, 0x8800, 0);
        event_wait(20);
        event_play_action(event_eggmobile_get_task(),
                          &ev004d_eggmobile_action_3, &event_eggmobile_texlist,
                          1.4F, 0, 4);
        event_play_action(event_eggmobile_get_task(),
                          &ev004d_eggmobile_action_0, &event_eggmobile_texlist,
                          1.4F, EVENT_REPEAT, 8);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 193.08F, 45.84F, 1275.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x3ad, 0x8800, 0);
    /* @hack redundant: a label nothing jumps to. SHC pops the stack arguments
     * of the calls above it where the label stands, 120 bytes in the slot of
     * event_wait(150) at 0x0cb8122e. without it, 226 of 3700 words differ */
    egg_rises: /* NOLINT(clang-diagnostic-unused-label) */
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 216.11F, 53.27F, 1391.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x26d, 0x8800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 167.11F, 29.84F, 1574.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x2ad, 0x9c00, 0x600);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 113.11F, 40.04F, 1618.2F);
        event_eggmobile_fly_turn(236.7F, 40.5F, 1404.0F, 0xffff2000, -0x5600,
                                 0xffff2000, 120, 0);
        event_wait(140);
        event_move_turn(ev004d_missile, 701.5F, -180.25F, 3300.2F, 701.5F,
                        86.25F, 3300.2F, 0xffe0, 0, 0, 0xfff0, 0x2000, 0, 200);
        event_wait(3);
        event_set_pos(event_get_player(0), 118.2F, 0.0F, 1465.0F);
        event_set_ang(event_get_player(0), 0, 0x1000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 117.6F, 18.44F, 1455.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xad, 0x958d, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 106.0F, 4.11F, 1437.0F);
        event_play_action(event_get_player(0), &ev004d_tails_action_6,
                          &tails_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev004d_tails_action_7,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 96.33F, 1.87F, 1442.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xead, 0x9c8d, 0x500);
        event_play_action(event_eggmobile_get_task(),
                          &ev004d_eggmobile_action_3, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_eggmobile_fly_turn(297.7F, 8.5F, 1665.0F, 0, 0, 0, 140, 0);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 96.9F, 1.2F, 1454.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0xead, 0x9c8d, 0x500);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 281.59F, 14.42F, 1640.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xead, 0x998d, 0xfa27);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 277.55F, 16.05F, 1645.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x7c1, 0x9b7d, 0xfa27);
        event_eggmobile_fly_turn(297.7F, 9.5F, 1665.0F, 0, 0x8000, 0, 120, 0);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 274.19F, 15.2F, 1648.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x7c1, 0x8f57, 0xfa27);
        event_wait(120);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev004d_tails_action_1,
                          &tails_texlist, 0.8F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 115.23F, 3.38F, 1476.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x9ad, 0xf68d, 0x100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 116.0F, 4.56F, 1471.8F);
        event_eggmobile_fly_turn(297.7F, 9.0F, 1665.0F, 0, -0x8000, 0, 40, 0);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_wait(20);
        event_voice_play(1318);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 292.7F, 17.51F, 1659.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x18ad, 0xab8d, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 8, 294.6F, 19.0F, 1661.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 8, 0x18ad, 0xa48d, 0xfe00);
        event_voice_play(1319);
        event_message_show(TEXT_5);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 192.56F, 19.78F, 1430.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x7ad, 0x838d, 0x800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 111.46F, 0.42F, 1457.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x7ad, 0x8b8d, 0x800);
        vibration_start(0, 1, 60, 10);
        ev004d_missile_set_effects(
            ev004d_missile, MISSILE_LAMPS | MISSILE_EXHAUST | MISSILE_SCROLL);
        event_sound_start(2, 706, 1800);
        event_sound_fade_volume(2, 10, 1);
        event_wait(1);
        event_sound_fade_volume(2, 100, 80);
        event_move_turn(ev004d_missile, 701.5F, 86.25F, 3300.2F, 701.5F, 86.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(2);
        event_move_turn(ev004d_missile, 701.5F, 86.25F, 3300.2F, 701.5F, 84.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(2);
        event_move_turn(ev004d_missile, 701.5F, 84.25F, 3300.2F, 701.5F, 86.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(2);
        event_move_turn(ev004d_missile, 701.5F, 86.25F, 3300.2F, 701.5F, 84.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(2);
        event_move_turn(ev004d_missile, 701.5F, 84.25F, 3300.2F, 701.5F, 86.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 86.25F, 3300.2F, 701.5F, 84.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 84.25F, 3300.2F, 701.5F, 86.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 86.25F, 3300.2F, 701.5F, 84.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 84.25F, 3300.2F, 701.5F, 87.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 87.25F, 3300.2F, 701.5F, 84.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 84.25F, 3300.2F, 701.5F, 87.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 87.25F, 3300.2F, 701.5F, 84.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
    /* @hack redundant: a label nothing jumps to. SHC pops the stack arguments
     * of the calls above it where the label stands, 112 bytes at 0x0cb81926.
     * without it, ev004d_script runs 4 bytes long and 913 of 3700 words
     * differ */
    shaking: /* NOLINT(clang-diagnostic-unused-label) */
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 84.25F, 3300.2F, 701.5F, 87.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 87.25F, 3300.2F, 701.5F, 85.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 85.25F, 3300.2F, 701.5F, 84.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 80.25F, 3300.2F, 701.5F, 86.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 86.25F, 3300.2F, 701.5F, 85.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 85.25F, 3300.2F, 701.5F, 80.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 80.25F, 3300.2F, 701.5F, 80.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 80.25F, 3300.2F, 701.5F, 85.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 85.25F, 3300.2F, 701.5F, 85.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 85.25F, 3300.2F, 701.5F, 80.25F,
                        3300.2F, 0xffff, 0x2000, 0, 0xffff, 0x2000, 0, 2);
        event_wait(1);
        event_move_turn(ev004d_missile, 701.5F, 85.25F, 3300.2F, 701.5F,
                        1100.25F, 3300.2F, 0xffff, 0x2000, 0, 0xf000, 0x4000, 0,
                        200);
        event_wait(80);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_14);
        event_wait(20);
        event_move_stop_all();
        event_sound_stop(2);
        event_sound_start(2, 706, 1800);
        event_sound_fade_volume(2, 50, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 119.8F, 5.8F, 1471.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x3ad, 0x88d, 0xff00);
        event_wait(15);
        event_move_turn(ev004d_missile, 351.5F, 320.25F, 2300.2F, 471.5F,
                        1030.25F, 1800.2F, 0xd800, 0x2000, 0, 0xd800, 0x6000, 0,
                        100);
        ev004d_missile_set_scale(ev004d_missile, 0.6F);
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_sound_start(2, 706, 1800);
        event_sound_fade_volume(2, 70, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 120.19F, 0.9F, 1458.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xcfb, 0x828d, 0x300);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 75, 118.19F, 0.62F, 1457.15F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 75, 0x18fb, 0x8c8d, 0xc00);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 80, 12197);
        event_sound_stop(2);
        event_sound_start(2, 706, 1800);
        event_sound_fade_volume(2, 80, 1);
        event_wait(10);
        event_eggmobile_fly_turn(297.7F, 9.0F, 1665.0F, 0, -0x3400, 0, 30, 0);
        event_wait(5);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(65);
        event_set_pos(event_get_player(0), 89.67F, 0.0F, 1541.1F);
        event_set_ang(event_get_player(0), 0x7ad, 0x3e90, 0);
        ev004d_missile_set_scale(ev004d_missile, 0.25F);
        event_move_stop_all();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -15.7F, 270.95F, 1730.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x247, 0xd2d5, 0xfa00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 170, -12.1F, 31.32F, 1689.4F);
        event_move_turn(ev004d_missile, 1600.5F, 1300.25F, 1602.2F, 1612.5F,
                        100.25F, 1000.2F, 0x8b00, 0, 0x1c00, 0x8b00, 0x4000, 0,
                        160);
        event_wait(160);
        event_sound_stop(2);
        /* @hack redundant: a call that never runs, passing 1689.4, which SHC
         * still counts when it allocates registers. with it, SHC loads 1 into
         * R10 once, at 0x0cb82462. every call from there to 0x0cb826a2 that
         * passes 1 then takes it from R10, as the image does. without it,
         * each of those calls loads its own 1, ev004d_script runs 4 bytes
         * long, and 284 of 3700 words differ */
        if (0) {
            ev004d_missile_set_scale(NULL, 1689.4F);
        }
        stream_stop_music();
        event_sound_play(1229, 110, 0, 0);
        vibration_start(0, 3, 60, 2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 32.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 33.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 32.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 34.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 33.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.32F, 1689.4F);
        event_wait(2);
        vibration_start(0, -1, 60, 2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 32.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 33.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 32.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 34.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 33.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 32.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 33.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 32.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 34.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 33.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 32.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.32F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 31.0F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.5F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.8F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.6F, 1689.4F);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 2, -12.1F, 30.7F, 1689.4F);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, -26.39F, 29.0F, 1696.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 130, 0x247, 0xd2d5, 0xfb00);
        event_wait(130);
        event_kill_task(&ev004d_missile);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 101.28F, 4.66F, 1538.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x347, 0x57d5, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 99.66F, 4.58F, 1536.12F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_15);
        event_wait(100);
        event_talk_stop(event_get_player(0));
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 302.7F, 23.2F, 1660.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf047, 0x5fd5, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 298.1F, 19.2F, 1658.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1147, 0x78d5, 0xfb00);
        event_voice_play(1320);
        event_message_show(TEXT_6);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 292.9F, 16.0F, 1657.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1300, 0x96d5, 0xfb00);
        event_play_action(event_eggmobile_get_task(),
                          &ev004d_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_voice_play(1321);
        event_message_show(TEXT_7);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 280.0F, 6.4F, 1698.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x247, 0xf4d5, 0x600);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 245.86F, 10.36F, 1698.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x247, 0xf4d5, 0x600);
        event_voice_play(1322);
        event_message_show(TEXT_8);
        event_eggmobile_fly_turn(297.7F, 13.0F, 1665.0F, 0, -0x1000, 0, 60, 1);
        stream_request_music(MUSIC_HURRYUP);
        event_clear_motions(event_eggmobile_get_task());
        event_wait(65);
        event_sound_start(1, 1226, 1800);
        event_sound_fade_volume(1, -60, 1);
        event_wait(1);
        event_sound_fade_volume(1, -50, 80);
        event_eggmobile_fly_turn(309.7F, 19.0F, 1416.0F, 0, 0, 0, 100, 1);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 76.0F, 7.2F, 1552.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff47, 0xc6d5, 0x600);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 73.7F, 5.2F, 1538.8F);
        event_wait(50);
        event_sound_fade_volume(1, -80, 80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 104.0F, 0.8F, 1536.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd47, 0x4ad5, 0x600);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 96.5F, 3.67F, 1539.1F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1323);
        event_message_show(TEXT_9);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1324);
        event_message_show(TEXT_10);
        event_eggmobile_fly_turn(309.7F, 27.0F, 1416.0F, 0, -0x4000, 0, 125, 1);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 82.2F, 8.0F, 1549.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1347, 0xd5d5, 0x600);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 83.9F, 2.27F, 1549.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xa47, 0xd5d5, 0x600);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_sound_stop(1);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop(1);
        event_sound_stop_all();
        event_move_stop_all();
        event_clear_paths(event_get_player(0));
        EVENT_KILL(ev004d_carry);
        event_clear_paths(ev004d_bird);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev004d_bird);
        event_eggmobile_kill();
        event_kill_task(&ev004d_missile);
        event_wait(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev004d_textures[] = {
    {"AMY", &amy_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"VER2_WING", &ev004d_bird_texlist},
    {"EGG_MISSILE", &ev004d_missile_texlist},
    {NULL, NULL},
};
