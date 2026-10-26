/* EV0052, Tails' story: the ending, where Tails flies down to Sonic and the
 * two set off together */
#include <ninja.h>

#include "ev0052/script.h"
#include "ev0052/sonic_action_2.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/clouds.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/shapes.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 1
#define TEXT_2_LENGTH 1
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 2
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 4
#define TEXT_7_LENGTH 48
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 26
#include "ev0052/script/text.inc"

/* @hack spelling: an action's motion read through the action's address, as
 * (&action)->motion. SHC then loads the action's address and reads the
 * motion 4 bytes past it, as the image does (MOV.L @(4,R2),R3 at
 * 0x0cb800fc). as action.motion, SHC loads the field's own address from the
 * pool, ev0052_script runs 64 bytes short, and 1522 of 1647 words differ */
#define MOTION_OF(action) ((&(action))->motion)

extern NJS_ACTION sonic_sprint_action;
extern NJS_OBJECT tails_fly_object;

extern NJS_ACTION ev0052_tails_action_0;
extern NJS_ACTION ev0052_tails_action_1;
extern NJS_ACTION ev0052_tails_action_2;
extern NJS_ACTION ev0052_tails_action_3;
extern NJS_ACTION ev0052_tails_action_4;
extern NJS_ACTION ev0052_tails_action_5;
extern NJS_ACTION ev0052_tails_action_6;
extern NJS_ACTION ev0052_tails_action_7;
extern NJS_ACTION ev0052_tails_action_8;
extern NJS_ACTION ev0052_tails_action_9;
extern NJS_ACTION ev0052_sonic_action_0;
extern NJS_ACTION ev0052_sonic_action_1;

/* Tails' run. from key 1 to key 90, the stick is pushed nearly all the way. its
 * direction there, in 256ths of a turn, stays at 28 to 41, near an eighth. A is
 * held from key 20 to key 33, for a jump */
static const struct event_pad_key ev0052_run_keys[91] = {
#include "ev0052/script/ev0052_run_keys.inc"
};

static const struct event_pad_recording ev0052_run = {
    ev0052_run_keys, sizeof(ev0052_run_keys) / sizeof(ev0052_run_keys[0])};

static struct event_path_point ev0052_tails_points_0[117] = {
#include "ev0052/script/ev0052_tails_points_0.inc"
};

static struct event_path ev0052_tails_path_0 = {117, ev0052_tails_points_0};

static struct event_path_point ev0052_tails_points_1[230] = {
#include "ev0052/script/ev0052_tails_points_1.inc"
};

static struct event_path ev0052_tails_path_1 = {230, ev0052_tails_points_1};

static struct event_path_point ev0052_tails_points_2[2] = {
    {0, 92, 0, {0.0F, 0.0F, 0.0F}},
    {0, 92, 0, {0.0F, 0.0F, 0.0F}},
};

static struct event_path ev0052_tails_path_2 = {2, ev0052_tails_points_2};

static struct event_path_point ev0052_tails_points_3[26] = {
#include "ev0052/script/ev0052_tails_points_3.inc"
};

static struct event_path ev0052_tails_path_3 = {26, ev0052_tails_points_3};

static struct event_path_point ev0052_sonic_points[201] = {
#include "ev0052/script/ev0052_sonic_points.inc"
};

static struct event_path ev0052_sonic_path = {201, ev0052_sonic_points};

static struct task *ev0052_flash = NULL;
static struct task *ev0052_fade_in = NULL;
static struct task *ev0052_fade_out = NULL;

void ev0052_script(int phase)
{
    /* @hack spelling: event_set_ang called through a local. SHC then puts its
     * address in R9 and event_play_action's in R8 (0x0cb800ae, 0x0cb80282).
     * called by name, the two trade, and 16 of 1647 words differ */
    void (*set_ang)(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        ev0052_fade_in = event_fade_create(1.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        stream_request_music(MUSIC_TAILS);
        event_clouds_start(2.5F, 7);
        event_set_pos(event_get_player(0), 885.3F, 126.24F, 1097.55F);
        set_ang = event_set_ang;
        set_ang(event_get_player(0), 0, 0x152c, 0);
        event_reset_player(0);
        TAILS_FLAGS(player_work[0]) |= TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[0]) |= TAILS_FLYING_HEAD;
        event_wait(1);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then moves 0 into a
         * scratch register for each call that passes it, as into R1 at
         * 0x0cb800f2. as an if, SHC keeps 0 in R10 for the calls that follow,
         * ev0052_script runs 48 bytes short, and 1600 of 1647 words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-unused-value) */
        ev0052_fade_in != NULL &&
            (event_fade_set_speed(ev0052_fade_in, -1.0F / 30.0F), 1);
        event_play_motion(event_get_player(0), &tails_fly_object,
                          MOTION_OF(ev0052_tails_action_5), &tails_texlist,
                          0.9F, EVENT_REPEAT, 0);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_create_player(2, sonic_step, 860.36F, 126.24F, 1082.87F, 0,
                            0xca00, 0);
        event_play_path(event_get_player(0), &ev0052_tails_path_0, 0.5F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 937.0F, 315.04F, 1570.069F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b75, 0x71a6, 0x0587);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 1, event_get_player(0),
                                  -0.5F, 2.0F, 0.0F, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 860.36F, 126.24F, 1082.87F);
        set_ang(event_get_player(2), 0, 0xca00, 0);
        event_camera_follow(event_get_player(0));
        event_wait(84);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0052_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 812.3F, 278.877F,
                             1422.4485F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x10d5, 0xa3f0, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, 811.0269F, 278.9224F,
                             1424.0F);
        event_talk_start(event_get_player(0), TEXT_1);
        event_play_motion(event_get_player(0), &tails_fly_object,
                          MOTION_OF(ev0052_tails_action_6), &tails_texlist,
                          1.0F, 0, 0);
        event_play_motion(event_get_player(0), &tails_fly_object,
                          MOTION_OF(ev0052_tails_action_7), &tails_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(38);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 856.4F, 249.44F, 1330.626F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf253, 0xde82, 0xf600);
        event_wait(2);
        event_camera_follow(event_get_player(0));
        event_wait(8);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 60, event_get_player(2),
                                  0.0F, 0.0F, 0.0F, 0xf600);
        event_wait(50);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_clear_motions(event_get_player(0));
        event_play_motion(event_get_player(0), &tails_fly_object,
                          MOTION_OF(ev0052_tails_action_8), &tails_texlist,
                          1.0F, 0, 0);
        event_play_motion(event_get_player(0), &tails_fly_object,
                          MOTION_OF(ev0052_tails_action_9), &tails_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 900.659F, 172.0854F,
                             1156.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e53, 0xa482, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, 886.96F, 176.757F,
                             1170.576F);
        event_talk_start(event_get_player(0), TEXT_2);
        event_wait(35);
        event_clouds_stop();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 856.415F, 141.79F,
                             1106.537F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xee25, 0xf6b1, 0xf600);
        event_camera_set_pos(EVENT_EASE_LINEAR, 25, 858.877F, 136.7F,
                             1095.954F);
        event_wait(9);
        event_play_action(event_get_player(2), &ev0052_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 6);
        event_look_at(event_get_player(2), 857.116F, 138.824F, 1096.548F);
        event_wait(10);
        event_talk_start(event_get_player(2), TEXT_1);
        event_wait(20);
        event_clear_paths(event_get_player(0));
        event_set_pos(event_get_player(0), 882.07F, 126.6F, 1089.6647F);
        set_ang(event_get_player(0), 0, 0xbeb0, 0);
        event_clear_motions(event_get_player(0));
        event_wait(4);
        event_play_motion(event_get_player(0), &tails_fly_object,
                          MOTION_OF(ev0052_tails_action_1), &tails_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(2));
        event_wait(4);
        set_ang(event_get_player(2), 0, 0xbe00, 0);
        event_play_action(event_get_player(2), &ev0052_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 6);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 860.93F, 136.94F, 1102.29F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0xffb0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 860.95F, 136.07F, 1099.4F);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 0.0F,
                           0.0F);
        event_talk_start(event_get_player(2), TEXT_1);
        event_wait(10);
        event_play_path(event_get_player(0), &ev0052_tails_path_3, 0.7F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 846.66F, 132.1F, 1083.423F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0xb6b0, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 841.2F, 126.078F, 1089.27F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1a00, 0xb8b0, 0xfb00);
        event_wait(10);
        event_play_motion(event_get_player(0), &tails_fly_object,
                          MOTION_OF(ev0052_tails_action_2), &tails_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(25);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_wait(1);
        event_play_action(event_get_player(2), &sonic_sprint_action,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 0);
        event_play_path(event_get_player(0), &ev0052_tails_path_1, 0.2F, 0);
        event_play_path(event_get_player(2), &ev0052_sonic_path, 0.2F, 0);
        event_talk_start(event_get_player(0), TEXT_3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 982.96F, 132.13F, 753.1227F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0x9500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 86, 943.3F, 132.17F, 683.29F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 86, 0x0400, 0x927c, 0);
        event_wait(86);
        event_camera_set_pos(EVENT_EASE_LINEAR, 19, 931.8835F, 131.04F,
                             669.2666F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 19, 0x03e9, 0x90c2, 0xfe00);
        event_wait(19);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 0.0F,
                           0.0F);
        event_talk_start(event_get_player(0), TEXT_4);
        event_talk_start(event_get_player(2), TEXT_1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 25, 909.67F, 123.64F, 661.788F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0x0be5, 0xa485, 0xfe00);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 867.55F, 104.0F, 644.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x104a, 0xa583, 0);
        event_wait(25);
        event_look_clear(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 841.288F, 67.15F, 608.855F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0b3b, 0xa125, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 34, 829.33F, 43.788F, 588.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 34, 0x136a, 0x9b0c, 0);
        event_wait(34);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_talk_start(event_get_player(2), TEXT_6);
        event_camera_set_pos(EVENT_EASE_LINEAR, 38, 799.758F, 32.0F, 584.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 38, 0x0722, 0x9bf1, 0xfd00);
        event_wait(38);
        event_camera_set_pos(EVENT_EASE_LINEAR, 37, 764.7F, 10.49F, 600.632F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 37, 0x1576, 0xaad4, 0x0a00);
        event_wait(37);
        event_look_clear(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 717.79F, 4.0374F, 622.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1477, 0xc054, 0x0300);
        event_wait(60);
        event_look_clear(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 666.39F, -14.026F,
                             619.774F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x15ea, 0xbf7c, 0x0100);
        event_wait(50);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_talk_start(event_get_player(0), TEXT_6);
        event_camera_set_pos(EVENT_EASE_LINEAR, 22, 647.4F, -29.136F, 604.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 22, 0x1a36, 0xb723, 0);
        event_wait(22);
        event_look_clear(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 608.6F, -23.97F, 601.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x098d, 0xc414, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 22, 597.29F, -16.44F, 579.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 22, 0x0a6c, 0xbefd, 0x0300);
        event_wait(22);
        event_camera_set_pos(EVENT_EASE_LINEAR, 33, 577.668F, 6.525F,
                             538.7679F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 33, 0xfc23, 0xac25, 0);
        event_wait(33);
        event_camera_set_pos(EVENT_EASE_LINEAR, 27, 540.6667F, 10.324F,
                             523.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 27, 0xfd28, 0xae14, 0x0b00);
        event_wait(27);
        event_camera_set_pos(EVENT_EASE_LINEAR, 13, 520.9F, 12.03F, 520.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 13, 0x0259, 0xc1d5, 0x0500);
        event_wait(13);
        event_camera_set_pos(EVENT_EASE_LINEAR, 23, 484.3F, 10.09F, 521.776F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 23, 0x0259, 0xc1d5, 0x0500);
        event_wait(23);
        event_camera_set_pos(EVENT_EASE_LINEAR, 42, 432.47F, 3.0F, 528.688F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 42, 0x0c84, 0xb7e9, 0xef00);
        event_wait(42);
        event_camera_set_pos(EVENT_EASE_LINEAR, 26, 399.6665F, 4.2325F,
                             543.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 26, 0x0ba7, 0xbd34, 0xf512);
        event_wait(26);
        event_camera_set_pos(EVENT_EASE_LINEAR, 44, 340.431F, 12.147F, 548.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 44, 0x0a17, 0xc6bb, 0);
        event_wait(44);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 20, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_camera_follow(event_get_player(0));
        event_wait(40);
        event_wait(18);
        ev0052_flash = event_fade_create(0.02F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_wait(40);
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_STILL;
        TAILS_FLAGS(player_work[0]) &= ~TAILS_FLYING_HEAD;
        event_talk_stop(event_get_player(0));
        event_clear_paths(event_get_player(0));
        event_clear_paths(event_get_player(2));
        event_clear_motions(event_get_player(0));
        event_clear_motions(event_get_player(2));
        event_set_pos(event_get_player(2), 2088.27F, 186.47F, 1560.4F);
        set_ang(event_get_player(2), 0xffba, 0x30e5, 0x0033);
        event_set_pos(event_get_player(0), 2099.88F, 186.476F, 1556.354F);
        set_ang(event_get_player(0), 0xffba, 0x22cb, 0x0033);
        event_play_action(event_get_player(0), &ev0052_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_wait(10);
        set_ang(event_get_player(2), 0xffba, 0x30e5, 0x0033);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 2095.6F, 187.126F, 1547.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0fc1, 0x93bd, 0);
        if (ev0052_flash != NULL) {
            event_fade_set_speed(ev0052_flash, -1.0F / 60.0F);
        }
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 2105.776F, 189.08F,
                             1551.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x10de, 0x3c69, 0);
        event_talk_start(event_get_player(0), TEXT_8);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 2105.142F, 189.08F,
                             1558.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x10de, 0x3469, 0xfd00);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 2163.2754F, 157.33F,
                             1622.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x0dde, 0x1b69, 0xfd00);
        event_wait(100);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0052_run);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 85, 2171.4F, 118.0F, 1630.995F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 85, 0x1b6d, 0x200e, 0xfd00);
        event_wait(93);
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0x066d, 0x200e, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, 2171.3F, 124.4F, 1630.665F);
        event_talk_start(event_get_player(0), TEXT_9);
        event_play_action(event_get_player(0), &ev0052_tails_action_3,
                          &tails_texlist, 1.0F, 0, 12);
        event_play_action(event_get_player(0), &ev0052_tails_action_4,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_play_path(event_get_player(0), &ev0052_tails_path_2, 0.0F,
                        EVENT_FROM_HERE);
        event_wait(50);
        ev0052_fade_out =
            event_fade_create(1.0F / 30.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(30);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_clear_paths(event_get_player(0));
        event_clear_paths(event_get_player(2));
        event_set_pos(event_get_player(0), 1199.0F, 126.0F, 851.0F);
        event_reset_player(0);
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[0]) &= ~TAILS_TAIL_STILL;
        event_kill_player(2);
        EVENT_KILL(ev0052_flash);
        EVENT_KILL(ev0052_fade_in);
        EVENT_KILL(ev0052_fade_out);
        event_clouds_stop();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0052_textures[] = {
    {"EV_ECCLOUD", &event_clouds_texlist},
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
