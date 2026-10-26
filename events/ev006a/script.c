/* EV006A, Amy's story: Amy stops E-102's fight with Sonic. Tails flies her off
 * the falling Egg Carrier */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev006a/bird_action.h"
#include "ev006a/script.h"
#include "ev006a/sonic_fingers.h"
#include "event/event.h"
#include "event/play.h"
#include "player/e102.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/dust_ring.h"
#include "shared/fade.h"
#include "shared/lightning.h"
#include "shared/shapes.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 42
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 50
#define TEXT_6_LENGTH 25
#define TEXT_7_LENGTH 28
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 40
#define TEXT_10_LENGTH 52
#define TEXT_11_LENGTH 57
#define TEXT_12_LENGTH 13
#define TEXT_13_LENGTH 72
#define TEXT_14_LENGTH 57
#define TEXT_15_LENGTH 19
#define TEXT_16_LENGTH 26
#define TEXT_17_LENGTH 70
#define TEXT_18_LENGTH 27
#define TEXT_19_LENGTH 44
#define TEXT_20_LENGTH 46
#define TEXT_21_LENGTH 53
#define TEXT_22_LENGTH 17
#define TEXT_23_LENGTH 43
#define TEXT_24_LENGTH 1
#define TEXT_25_LENGTH 1
#define TEXT_26_LENGTH 5
#define TEXT_27_LENGTH 5
#define TEXT_28_LENGTH 3
#define TEXT_29_LENGTH 3
#define TEXT_30_LENGTH 3
#define TEXT_31_LENGTH 3
#define TEXT_32_LENGTH 1
#define TEXT_33_LENGTH 6
#define TEXT_34_LENGTH 4
#define TEXT_35_LENGTH 5
#define TEXT_36_LENGTH 4
#define TEXT_37_LENGTH 3
#define TEXT_38_LENGTH 3
#include "ev006a/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_run_action;
extern NJS_ACTION sonic_skid_action;
extern NJS_ACTION sonic_jump_land_action;
/* the first child of sonic_hand_minus_z in 1ST_READ.BIN. its sibling is
 * sonic_fingers_minus_z, which the script swaps for ev006a_sonic_fingers
 * while Sonic says TEXT_3 */
extern NJS_OBJECT dat_8c1e0958;
extern NJS_OBJECT sonic_fingers_minus_z;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_water_skate_action;
extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_trot_action;

extern NJS_TEXLIST ev006a_bird_texlist_2;

static struct event_path_point ev006a_tails_points_0[240] = {
#include "ev006a/script/ev006a_tails_points_0.inc"
};
static struct event_path ev006a_tails_path_0 = {240, ev006a_tails_points_0};

static struct event_path_point ev006a_e102_points[200] = {
#include "ev006a/script/ev006a_e102_points.inc"
};
static struct event_path ev006a_e102_path = {200, ev006a_e102_points};

static struct event_path_point ev006a_tails_points_1[200] = {
#include "ev006a/script/ev006a_tails_points_1.inc"
};
static struct event_path ev006a_tails_path_1 = {200, ev006a_tails_points_1};

/* no code uses this word. it keeps ev006a_bird at its address in the
 * image */
static Uint32 dat_0cb86090 = 0;
static struct task *ev006a_bird = NULL;
static struct task *ev006a_fade = NULL;
static struct task *ev006a_carry = NULL;

/* hold Amy at Tails' pos plus the task's pos, and at his ang.y plus the
 * task's ang.y. Amy is player 0, and Tails player 3. nothing checks that
 * either still exists, so both must outlive the task */
static void ev006a_carry_step(struct task *task)
{
    struct task_work *tails = event_get_player(3)->work;
    struct task_work *amy = event_get_player(0)->work;
    struct task_work *work = task->work;

    amy->pos.x = tails->pos.x + work->pos.x;
    amy->pos.y = tails->pos.y + work->pos.y;
    amy->pos.z = tails->pos.z + work->pos.z;
    amy->ang.y = tails->ang.y + work->ang.y;
}

/* make the task that holds Amy 5 above Tails. its ang.y is 0, Tails' ang.y less
 * itself, so Amy takes Tails' ang.y */
static void ev006a_carry_create(void)
{
    struct task_work *tails;
    struct task_work *tails_again;
    struct task_work *work;

    ev006a_carry = task_create(TASK_WORK, 5, ev006a_carry_step);
    tails = event_get_player(3)->work;
    tails_again = event_get_player(3)->work;
    work = ev006a_carry->work;
    work->pos.x = 0.0F;
    work->pos.y = 5.0F;
    work->pos.z = 0.0F;
    work->ang.y = tails_again->ang.y - tails->ang.y;
}

void ev006a_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        sound_load_file("e_001B.mlt");
        event_sound_init(5);
        event_reset_player(0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_set_pos(event_get_player(0), -55.6F, 1525.67F, 3270.19F);
        event_set_ang(event_get_player(0), 0, 0x5878, 0);
        event_create_player(2, sonic_step, -8.0F, 1525.69F, 3320.0F, 0, 0, 0);
        event_create_player(3, tails_step, 8.0F, 1525.69F, 3322.0F, 0, 0, 0);
        event_create_player(4, e102_step, 0.0F, 1525.7F, 3402.0F, 0, 0x8000, 0);
        event_place_actor(&ev006a_bird, 2.82F, 1638.08F, 3195.52F, 0, 0, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), -90.0F, 1560.0F, 3200.0F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev006a_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(ev006a_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev006a_bird, &ev006a_bird_action,
                          &ev006a_bird_texlist_1, 1.0F, EVENT_REPEAT, 1);
        event_lightning_create(event_get_player(4)->work->pos.x,
                               event_get_player(4)->work->pos.y,
                               event_get_player(4)->work->pos.z, 4);
        event_look_clear(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        stream_request_music(MUSIC_EVTBGM00);
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -58.47F, 1530.72F, 3256.29F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x8700, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -2.34F, 1530.72F, 3245.95F);
        event_wait(60);
        event_sound_play(1224, 0, 0, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -45.58F, 1529.58F, 3428.98F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0680, 0xedb3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -18.04F, 1529.57F,
                             3437.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0680, 0xf3b3, 0);
        event_wait(60);
        event_sound_play(1224, 0, 0, 0);
        event_wait(40);
        event_lightning_kill();
        event_look_clear(event_get_player(0));
        event_set_pos(event_get_player(2), -3.18F, 1525.68F, 3300.0F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_set_pos(event_get_player(3), -55.6F, 1525.67F, 3250.19F);
        event_set_pos(event_get_player(4), -3.18F, 1525.68F, 3390.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.69F, 1530.97F, 3313.28F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0080, 0xf3b3, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -13.77F, 1530.64F,
                             3339.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0x0080, 0xedb3, 0);
        event_wait(10);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_run_action,
                          &sonic_texlist, 3.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(2), event_get_player(2)->work->pos.x,
                     event_get_player(2)->work->pos.y,
                     event_get_player(2)->work->pos.z + 60.0F);
        event_wait(35);
        event_set_pos(event_get_player(2), -3.18F, 1525.68F, 3300.0F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_run_action,
                          &sonic_texlist, 3.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(2), event_get_player(2)->work->pos.x,
                     event_get_player(2)->work->pos.y,
                     event_get_player(2)->work->pos.z + 60.0F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0100, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -11.74F, 1530.14F, 3305.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0100, 0x8700, 0x0800);
        event_wait(25);
        event_set_pos(event_get_player(2), -3.18F, 1525.68F, 3300.0F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_set_pos(event_get_player(0), -63.18F, 1525.68F, 3370.0F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3c72);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_run_action,
                          &sonic_texlist, 3.5F, EVENT_REPEAT, 10);
        event_run_to(event_get_player(2), event_get_player(2)->work->pos.x,
                     event_get_player(2)->work->pos.y,
                     event_get_player(2)->work->pos.z + 60.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xef00, 0x9000, 0x0500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.64F, 1565.87F, 3262.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 35, 0x0200, 0x9000, 0x0600);
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, -9.76F, 1526.92F, 3337.49F);
        event_wait(30);
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0x0f00, 0x9500, 0x0700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, -9.53F, 1527.28F, 3353.3F);
        event_wait(15);
        event_set_pos(event_get_player(0),
                      event_get_player(4)->work->pos.x - 20.0F,
                      event_get_player(4)->work->pos.y,
                      event_get_player(4)->work->pos.z - 15.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f00, 0x4d00, 0x0700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -27.0F, 1535.43F, 3339.28F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -18.15F, 1529.81F, 3373.1F);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -17.75F, 1529.56F,
                             3374.68F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_24);
        event_voice_play(1464);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_set_pos(event_get_player(3), 50.0F, 1525.69F, 3250.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 64.47F, 1529.06F, 3316.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0800, 0x5000, 0x0700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 5.15F, 1529.8F, 3353.71F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0600, 0x6500, 0x0700);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          2.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(0), event_get_player(4)->work->pos.x,
                     event_get_player(4)->work->pos.y,
                     event_get_player(4)->work->pos.z - 15.0F);
        event_set_pos(event_get_player(2), -3.18F, 1525.68F, 3270.0F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_run_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(2), event_get_player(2)->work->pos.x,
                     event_get_player(2)->work->pos.y,
                     event_get_player(2)->work->pos.z + 80.0F);
        event_wait(60);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), event_get_player(2)->work->ang.x,
                      -0x4000 - event_get_player(2)->work->ang.y,
                      event_get_player(2)->work->ang.z);
        event_run_wait(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_skid_action,
                          &sonic_texlist, 0.8F, 0, 0);
        event_run_to(event_get_player(2), event_get_player(2)->work->pos.x,
                     event_get_player(2)->work->pos.y,
                     event_get_player(2)->work->pos.z + 10.0F);
        event_wait_motions(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 2.41F, 1534.15F, 3379.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0x1200, 0);
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_25);
        event_voice_play(1465);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_voice_play(1466);
        event_talk_start(event_get_player(2), TEXT_26);
        event_message_show(TEXT_3);
        event_look_clear(event_get_player(2));
        event_wait(10);
        dat_8c1e0958.sibling = &ev006a_sonic_fingers;
        event_play_action(event_get_player(2), &ev006a_sonic_action_2,
                          &sonic_texlist, 1.0F, 0, 4);
        event_play_action(event_get_player(2), &ev006a_sonic_action_4,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        dat_8c1e0958.sibling = &sonic_fingers_minus_z;
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -13.62F, 1534.03F, 3348.96F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf900, 0x9300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -10.07F, 1532.66F, 3356.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev006a_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_talk_start(event_get_player(0), TEXT_25);
        event_voice_play(1467);
        event_message_show(TEXT_4);
        event_wait(10);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 20);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_voice_play(1468);
        event_talk_start(event_get_player(0), TEXT_27);
        event_message_show(TEXT_5);
        event_wait(20);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_voice_play(1469);
        event_talk_start(event_get_player(0), TEXT_27);
        event_message_show(TEXT_6);
        event_wait(20);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_talk_start(event_get_player(0), TEXT_28);
        event_voice_play(1470);
        event_message_show(TEXT_7);
        event_wait(20);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        event_set_pos(event_get_player(3),
                      event_get_player(2)->work->pos.x - 40.0F,
                      event_get_player(2)->work->pos.y,
                      event_get_player(2)->work->pos.z - 10.0F);
        event_set_ang(event_get_player(3), event_get_player(2)->work->ang.x,
                      0x7000 - event_get_player(2)->work->ang.y,
                      event_get_player(2)->work->ang.z);
        event_look_clear(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1.31F, 1526.14F, 3370.95F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1300, 0x1100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -1.59F, 1529.74F, 3364.41F);
        event_wait(60);
        event_voice_play(1471);
        event_talk_start(event_get_player(2), TEXT_25);
        event_message_show(TEXT_8);
        event_wait(20);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 32);
        event_wait(60);
        event_voice_play(1097);
        event_talk_start(event_get_player(2), TEXT_29);
        event_message_show(TEXT_9);
        event_wait(20);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 25.39F, 1527.59F, 3344.71F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0480, 0x5d00, 0x0500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 11.44F, 1529.77F,
                             3352.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        event_sound_start(0, 1223, 1800);
        stream_stop_music();
        event_play_action(event_get_player(2), &ev006a_sonic_action_5,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(0), &ev006a_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        stream_request_music(MUSIC_EGCARER1);
        vibration_start(0, 7, 50, 3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x0500);
        event_wait(3);
        event_sound_fade_volume(0, -30, 1);
        event_play_action(event_get_player(2), &ev006a_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev006a_amy_action_1,
                          &amy_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0980, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0980, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_sound_fade_volume(0, -70, 21);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0580, 0x5d00, 0x0500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0480, 0x5d00, 0x0500);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -35.19F, 1532.11F, 3359.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf780, 0x1500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -33.26F, 1532.94F,
                             3362.46F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_voice_play(1098);
        event_talk_start(event_get_player(3), TEXT_30);
        event_message_show(TEXT_10);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf680, 0x1500, 0);
        event_wait(3);
        event_voice_wait();
        event_talk_stop(event_get_player(3));
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 6.01F, 1530.3F, 3369.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0080, 0x2a00, 0);
        event_message_close();
        event_play_action(event_get_player(2), &ev006a_sonic_action_1,
                          &sonic_texlist, 0.4F, 0, 3);
        event_wait(30);
        event_voice_play(1099);
        event_talk_start(event_get_player(2), TEXT_31);
        event_message_show(TEXT_11);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.71F, 1530.29F, 3368.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0880, 0x9800, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_voice_play(1472);
        event_talk_start(event_get_player(0), TEXT_32);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_look_clear(event_get_player(2));
        event_wait(10);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 8.0F,
                           0.0F);
        event_play_action(event_get_player(2), &ev006a_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(2), &ev006a_sonic_action_3,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 7.12F, 1531.76F, 3383.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0080, 0x1100, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x271c);
        event_voice_play(1101);
        event_talk_start(event_get_player(2), TEXT_33);
        event_message_show(TEXT_13);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_look_clear(event_get_player(2));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 12.35F, 1542.55F, 3384.94F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf580, 0x1100, 0x0500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 36.64F, 1558.34F,
                             3439.73F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_run_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 10);
        event_run_to(event_get_player(2),
                     event_get_player(2)->work->pos.x - 80.0F,
                     event_get_player(2)->work->pos.y,
                     event_get_player(2)->work->pos.z - 240.0F);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_wait(60);
        event_kill_player(2);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(3));
        event_look_at(event_get_player(0), -83.09F, 1525.68F, 3119.04F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.13F, 1529.579834F,
                             3361.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0880, 0xa014, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -2.88F, 1529.579834F,
                             3355.73F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0880, 0x7d14, 0);
        event_wait(80);
        event_talk_start(event_get_player(0), TEXT_34);
        event_voice_play(1473);
        event_message_show(TEXT_14);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 3.0F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 10, 0x2000);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 10, -2.7F, 1529.5999F, 3347.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 10, 0x0a80, 0x7f14, 0);
        event_wait(30);
        event_look_at(event_get_player(0), -3.18F, 1550.68F, 3200.0F);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 18.81F, 1528.25F, 3344.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0580, 0x6e14, 0);
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(4), 0.0F,
                           12.0F, 0.0F);
        event_set_pos(event_get_player(0), -3.18F, 1525.68F, 3360.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 10.87F, 1528.25F,
                             3340.77F);
        event_wait(15);
        event_voice_play(1474);
        event_message_show(TEXT_15);
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -23.96F, 1527.03F, 3405.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0487, 0xec14, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 500, -17.89F, 1528.47F,
                             3394.47F);
        event_voice_play(1475);
        event_message_show(TEXT_16);
        event_wait(1);
        event_voice_wait();
        event_voice_play(1476);
        event_talk_start(event_get_player(0), TEXT_35);
        event_message_show(TEXT_17);
        event_wait(40);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -14.95F, 1527.19F, 3334.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0880, 0x8b14, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -28.73F, 1527.19F,
                             3342.69F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 300, 0x0880, 0x9d14, 0);
        event_wait(10);
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 3.0F, EVENT_REPEAT, 0);
        event_voice_play(1775);
        event_message_show(TEXT_18);
        event_wait(30);
        event_play_action(event_get_player(4), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 0.04F, 1532.59F, 3370.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd80, 0x1014, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -0.74F, 1532.4F, 3368.03F);
        event_voice_wait();
        event_message_close();
        event_set_pos(ev006a_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev006a_bird, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev006a_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1.01F, 1532.36F, 3360.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d80, 0x7914, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -2.44F, 1535.29F,
                             3369.34F);
        event_voice_play(1477);
        event_message_show(TEXT_19);
        event_wait(20);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1.74F, 1535.99F, 3373.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf280, 0x1114, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 0.59F, 1535.02F, 3371.14F);
        event_voice_play(1478);
        event_talk_start(event_get_player(0), TEXT_36);
        event_message_show(TEXT_20);
        event_wait(10);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 16.14F, 1530.8F, 3342.16F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0380, 0x6814, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 28.83F, 1528.82F,
                             3323.08F);
        event_voice_play(1479);
        event_talk_start(event_get_player(0), TEXT_36);
        event_message_show(TEXT_21);
        event_wait(40);
        event_play_action(event_get_player(4), &e102_stand_action,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 16);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -32.06F, 1528.37F, 3342.32F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0780, 0x5314, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -30.26F, 1528.37F,
                             3345.88F);
        event_voice_play(1480);
        event_talk_start(event_get_player(3), TEXT_37);
        event_message_show(TEXT_22);
        event_wait(30);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 42.19F, 1527.97F, 3392.77F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0580, 0x3214, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 48.72F, 1527.03F,
                             3395.09F);
        event_voice_play(1481);
        event_talk_start(event_get_player(0), TEXT_38);
        event_message_show(TEXT_23);
        event_wait(70);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 80, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 8.21F, 1527.15F, 3343.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0980, 0x6f14, 0);
        event_wait(40);
        event_shapes_set_firm(event_get_player(0));
        event_set_pos(event_get_player(3), -5.05F, 1525.68F, 3311.0F);
        event_wait(1);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(3), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(4), EVENT_IGNORE_GROUND);
        event_dust_ring_create(event_get_player(4)->work->pos.x,
                               event_get_player(4)->work->pos.y,
                               event_get_player(4)->work->pos.z);
        event_play_path(event_get_player(3), &ev006a_tails_path_0, 0.6F,
                        EVENT_FROM_HERE);
        event_play_path(event_get_player(3), &ev006a_tails_path_1, 2.0F,
                        EVENT_FROM_HERE);
        event_wait(40);
        event_play_action(event_get_player(3), &ev006a_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(11);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x2847, 0x6168, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev006a_amy_action_2,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_kill_task(&ev006a_bird);
        TAILS_FLAGS(player_work[3]) |= TAILS_TAIL_SPIN;
        ev006a_carry_create();
        event_wait(30);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, -93.03F, 1635.66F, 3375.8F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30, event_get_player(0),
                                  0.0F, 4.0F, 0.0F, 0);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 37, -142.16F, 1628.347F,
                             3386.73F);
        event_wait(25);
        event_play_action(event_get_player(4), &e102_water_skate_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(11);
        event_dust_ring_set_mode(EVENT_DUST_RING_ON);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -141.63F, 1623.64F,
                             3358.01F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0xede6, 0xb33a, 0);
        event_wait(15);
        event_play_path(event_get_player(4), &ev006a_e102_path, 0.7F,
                        EVENT_FROM_HERE);
        event_wait(35);
        event_dust_ring_set_mode(EVENT_DUST_RING_WAIT);
        event_wait(75);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x382e);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -215.96F, 1616.454F,
                             3405.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfce6, 0x2b3a, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 250, -178.91F, 1619.46F,
                             3412.29F);
        event_kill_player(4);
        event_sound_stop(0);
        event_wait(150);
        ev006a_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        TAILS_FLAGS(player_work[3]) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[3]) &= ~TAILS_TAIL_STILL;
        event_camera_release();
        event_pads_on();
        EVENT_KILL(ev006a_fade);
        dat_8c1e0958.sibling = &sonic_fingers_minus_z;
        event_set_flags(event_get_player(3), EVENT_GROUND);
        event_shapes_clear_firm_players(event_get_player(0));
        event_clear_paths(event_get_player(4));
        /* SHC loads ev006a_carry into R4 once, at 0x0cb825ce, for both this
         * test and task_kill's argument. with EVENT_KILL, SHC loads
         * ev006a_carry again in the call's delay slot, and 8 of ev006a_script's
         * 3608 words differ */
        if (ev006a_carry != NULL) {
            task_kill(ev006a_carry);
            ev006a_carry = NULL;
        }
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(3);
        event_kill_player(4);
        event_kill_task(&ev006a_bird);
        event_dust_ring_kill();
        event_lightning_kill();
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev006a_textures[] = {
    {"VER1_WING", &ev006a_bird_texlist_1},
    {"VER2_WING", &ev006a_bird_texlist_2},
    {"SONIC", &sonic_texlist},
    {"E102", &e102_texlist},
    {"MILES", &tails_texlist},
    {NULL, NULL},
};
