/* EV0026, the end of Sonic's story: Eggman flies off, and Sonic and Tails run
 * after him */
#include <ninja.h>
#include <shinobi.h>

#include "ev0026/script.h"
#include "ev0026/sonic_fingers.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/textures.h"
#include "shared/clouds.h"
#include "shared/dust.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 1
#define TEXT_2_LENGTH 1
#define TEXT_3_LENGTH 7
#include "ev0026/script/text.inc"

/* @hack spelling: each motion as (&action)->motion. SHC then pools the
 * action's address and reads the motion 4 bytes past it (MOV.L @(4,R1),R3
 * at 0x0cb80474). as action.motion, SHC pools the address of the motion
 * field, and 1208 of ev0026_script's 1737 words differ */
#define MOTION_OF(action) ((&(action))->motion)

/* the node of Sonic's model whose sibling is his -z fingers */
extern NJS_OBJECT dat_8c1e0958;
extern NJS_OBJECT sonic_fingers_minus_z;
extern NJS_ACTION sonic_sprint_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_OBJECT tails_fly_object;
/* @hack spelling: player_work_2 is player_work[2], declared under a name of its
 * own at the element's address, for EVENT_END's read of Tails' slot. as
 * &player_work[2] there too, 187.45F and 420.0F trade FR15 and FR14 (FMOV
 * FR15,FR4 at 0x0cb8014e), and 6 of ev0026_script's 1737 words differ */
#ifdef GCC
/* GCC's link binds a name only where a definition starts, and this one
 * names an element inside player_work */
#define player_work_2 (player_work[2])
#else
extern struct task_work *player_work_2;
#endif

extern NJS_TEXLIST event_eggmobile_texlist;

/* @hack spelling: the sibling as (&dat_8c1e0958)->sibling. SHC then pools the
 * node's address for each of the two stores and stores 48 bytes past it,
 * as MOV.L R3,@(48,R2) at 0x0cb81156 does. as dat_8c1e0958.sibling, the
 * pool words hold the address of the node's sibling field, and 2 of
 * ev0026_script's 1737 words differ */
#define SET_SONIC_FINGERS(fingers) ((&dat_8c1e0958)->sibling = &(fingers))

/* Sonic's run and two jumps, a key a step. A is held for the 4 keys from key 23
 * and the 4 from key 44. the stick's direction runs from -26 to -116, in 256ths
 * of a turn, then back to -88. the last 34 keys let it go */
/* clang-format off */
static const struct event_pad_key ev0026_run_keys[88] = {
    {0, -26, 4}, {0, -33, 26}, {0, -36, 52}, {0, -38, 141}, {0, -39, 228},
    {0, -42, 226}, {0, -43, 223}, {0, -46, 223}, {0, -47, 224}, {0, -48, 223},
    {0, -48, 222}, {0, -49, 225}, {0, -49, 225}, {0, -49, 221}, {0, -49, 225},
    {0, -52, 223}, {0, -56, 225}, {0, -60, 224}, {0, -59, 228}, {0, -60, 226},
    {0, -60, 223}, {0, -67, 228}, {0, -87, 0}, {PDD_DGT_TA, -92, 242},
    {PDD_DGT_TA, -102, 239}, {PDD_DGT_TA, -109, 236}, {PDD_DGT_TA, -110, 232},
    {0, -110, 235}, {0, -112, 231}, {0, -114, 228}, {0, -114, 228},
    {0, -114, 228}, {0, -114, 229}, {0, -114, 233}, {0, -112, 231},
    {0, -112, 227}, {0, -112, 227}, {0, -112, 231}, {0, -114, 233},
    {0, -115, 232}, {0, -116, 232}, {0, -116, 232}, {0, -116, 232},
    {0, -115, 232}, {PDD_DGT_TA, -114, 228}, {PDD_DGT_TA, -112, 231},
    {PDD_DGT_TA, -112, 228}, {PDD_DGT_TA, -110, 228}, {0, -108, 233},
    {0, -107, 233}, {0, -104, 233}, {0, -99, 241}, {0, -91, 206}, {0, -88, 141},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
};
/* clang-format on */

static const struct event_pad_recording ev0026_run = {
    ev0026_run_keys, sizeof(ev0026_run_keys) / sizeof(ev0026_run_keys[0])};

static struct event_path_point ev0026_sonic_points[171] = {
#include "ev0026/script/ev0026_sonic_points.inc"
};

static struct event_path ev0026_sonic_path = {171, ev0026_sonic_points};

static struct event_path_point ev0026_tails_points_0[157] = {
#include "ev0026/script/ev0026_tails_points_0.inc"
};

static struct event_path ev0026_tails_path_0 = {157, ev0026_tails_points_0};

static struct event_path_point ev0026_tails_points_1[155] = {
#include "ev0026/script/ev0026_tails_points_1.inc"
};

static struct event_path ev0026_tails_path_1 = {155, ev0026_tails_points_1};

static struct task *ev0026_smoke = NULL;
static struct task *ev0026_flash = NULL;
static struct task *ev0026_fade = NULL;

void ev0026_script(int phase)
{
    /* @hack spelling: Tails' flags through a local that holds his slot's
     * address. SHC keeps it on the stack (MOV.L R3,@R15 at 0x0cb8024e) and
     * reads it again for each change (MOV.L @R15,R3 at 0x0cb80efc). with no
     * local, ev0026_script runs 8 bytes short, and 1654 of its 1737 words
     * differ */
    struct task_work **tails_slot;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        if (ev0026_smoke == NULL) {
            ev0026_smoke = event_dust_create();
        }
        /* @hack spelling: each test and its stores as one && expression. each
         * value then goes through FPUL into FR3 just before its first store
         * (0x0cb80096). as ifs, SHC loads 2.0F and 0.5F into FR12 and FR13
         * ahead of the tests, ev0026_script runs 12 bytes short, and 1642 of
         * its 1737 words differ */
        ev0026_smoke != NULL && (DUST_WORK(ev0026_smoke)->params->scale = 2.0F);
        ev0026_smoke != NULL &&
            (DUST_WORK(ev0026_smoke)->params->color.a = 0.9F,
             DUST_WORK(ev0026_smoke)->params->color.r = 0.5F,
             DUST_WORK(ev0026_smoke)->params->color.g = 0.5F,
             DUST_WORK(ev0026_smoke)->params->color.b = 0.5F);
        event_set_pos(event_get_player(0), 338.5F, 204.0F, 684.4F);
        event_set_ang(event_get_player(0), 0xf800, 0x9300, 0);
        event_play_action(event_get_player(0), &ev0026_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_create_player(2, tails_step, -235.6F, 1528.6F, 3543.3F, 0, 0x8000,
                            0);
        event_eggmobile_create(187.45F, 350.0F, 420.0F, 0, 0x5500, 0);
        event_set_pos(ev0026_smoke, 187.45F, 370.0F, 420.0F);
        stream_request_music(MUSIC_SONIC);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x590, 0xae0, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 430.2F, 210.6F, 700.5F);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x590, 0xae0, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 340.1F, 207.0F, 703.0F);
        event_wait(50);
        tails_slot = &player_work[2];
        TAILS_FLAGS(*tails_slot) |= TAILS_TAIL_SPIN;
        TAILS_FLAGS(*tails_slot) |= TAILS_FLYING_HEAD;
        if (ev0026_smoke != NULL) {
            DUST_WORK(ev0026_smoke)->timer = DUST_WORK(ev0026_smoke)->interval =
                1;
        }
        event_eggmobile_set_bob(0x250, 5.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 337.74F, 207.0F, 703.5F);
        event_wait(70);
        event_eggmobile_fly_turn(421.27F, 174.0F, 1108.0F, 0, 0, 0, 170, 0);
        event_move(ev0026_smoke, 187.45F, 360.0F, 420.0F, 421.27F, 184.0F,
                   1108.0F, 170);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x2990, 0x18e0, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 339.8F, 202.37F, 691.0F);
        event_wait(37);
        event_play_action(event_get_player(0), &ev0026_sonic_action_1,
                          &sonic_texlist, 0.5F, 0, 8);
        event_wait(45);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd90, 0x77e0, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 343.2F, 204.646F, 672.3F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 342.84F, 205.3F, 674.14F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0026_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0, 0x1100, 0);
        event_wait(110);
        event_play_action(event_get_player(0), &ev0026_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_look_at(event_get_player(0), 857.116F, 138.824F, 1096.548F);
        event_wait(25);
        event_talk_start(event_get_player(0), TEXT_1);
        event_wait(20);
        event_set_flags(event_get_player(2), EVENT_IGNORE_GROUND);
        event_play_path(event_get_player(2), &ev0026_tails_path_0, 1.0F, 0);
        event_clear_motions(event_get_player(2));
        event_play_motion(event_get_player(2), &tails_fly_object,
                          MOTION_OF(ev0026_tails_action_2), &tails_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0, 0x800, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x890, 0xbae0, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 324.7F, 206.3F, 683.57F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 324.22F, 206.3F, 687.5F);
        event_wait(80);
        event_clouds_start(2.5F, 4);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf000, 0x2a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 734.0F, 319.0F, 791.0F);
        event_camera_follow(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_2);
        event_wait(76);
        event_camera_follow_clear();
        event_clouds_stop();
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x400, 0xd800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 305.63F, 208.56F, 722.1F);
        event_wait(5);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0026_run);
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xa00, 0xe000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 290.17F, 195.27F, 704.47F);
        event_wait(30);
        event_clear_motions(event_get_player(2));
        event_play_motion(event_get_player(2), &tails_fly_object,
                          MOTION_OF(ev0026_tails_action_1), &tails_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(2), 0, 0x9600, 0);
        event_move(event_get_player(2), 358.7F, 219.0F, 704.4F, 154.0F, 219.0F,
                   548.6F, 200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 217.57F, 11.2F, 667.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x3c00, 0xe100, 0xfc00);
        event_camera_set_fov(EVENT_EASE_LINEAR, 70, 0x3d28);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 254.44F, -15.56F, 623.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x3c00, 0xe100, 0xfc00);
        event_wait(90);
        event_eggmobile_kill();
        EVENT_KILL(ev0026_smoke);
        event_wait(1);
        event_move_stop_all();
        event_clear_motions(event_get_player(0));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(0), &sonic_sprint_action,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 8);
        event_play_motion(event_get_player(2), &tails_fly_object,
                          MOTION_OF(ev0026_tails_action_1), &tails_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_set_flags(event_get_player(2), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(0), EVENT_GROUND | EVENT_SLOPE);
        event_play_path(event_get_player(0), &ev0026_sonic_path, 0.32F,
                        EVENT_REPEAT);
        event_play_path(event_get_player(2), &ev0026_tails_path_1, 0.32F, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 982.96F, 132.13F, 753.1227F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x400, 0x9500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 43, 942.36F, 133.2F, 684.55F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 43, 0xff00, 0x9100, 0);
        event_wait(35);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 0.0F,
                           0.0F);
        event_talk_start(event_get_player(2), TEXT_2);
        event_wait(8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 32, 889.17F, 119.58F, 655.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 32, 0xff00, 0xa700, 0);
        event_wait(32);
        event_talk_stop(event_get_player(2));
        event_look_clear(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 16, 868.8F, 104.41F, 647.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 16, 0xff50, 0xa6c7, 0xffc2);
        event_wait(16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 18, 858.2F, 84.7F, 614.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 18, 0xfd50, 0x82c7, 0xffc2);
        event_wait(18);
        event_camera_set_pos(EVENT_EASE_LINEAR, 18, 849.3F, 65.86F, 594.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 18, 0xf750, 0x73c7, 0xf8c2);
        event_wait(18);
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 0.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 32, 795.2F, 8.58F, 569.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 32, 0x1650, 0x85c7, 0xf8c2);
        event_wait(32);
        event_camera_set_pos(EVENT_EASE_LINEAR, 28, 735.5F, 12.7F, 623.624F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 28, 0x1050, 0xbbc7, 0x400);
        event_wait(28);
        event_camera_set_pos(EVENT_EASE_LINEAR, 38, 665.55F, -18.5567F, 614.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 38, 0x1f50, 0xb6c7, 0x400);
        event_wait(38);
        event_camera_set_pos(EVENT_EASE_LINEAR, 11, 653.27F, -29.74F,
                             611.3334F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 11, 0x1eff, 0xb700, 0x400);
        event_wait(11);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 617.3F, -26.266F, 597.135F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x950, 0xc4c7, 0x400);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 22, 592.868F, -10.894F,
                             556.115F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 22, 0xb62, 0xb1bb, 0x400);
        event_wait(22);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_talk_start(event_get_player(0), TEXT_1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 22, 563.073F, -2.07F,
                             532.0875F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 22, 0x762, 0xb3bb, 0x400);
        event_wait(22);
        event_play_motion(event_get_player(2), &tails_fly_object,
                          MOTION_OF(ev0026_tails_action_1), &tails_texlist,
                          1.5F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 13, 545.324F, 8.16F, 521.8355F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 13, 0xff62, 0xadbb, 0x100);
        event_wait(13);
        event_talk_start(event_get_player(2), TEXT_2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 25, 506.2254F, 13.2F, 511.085F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0xfb62, 0xaf00, 0x100);
        event_wait(25);
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 24, 462.483F, 10.648F, 524.19F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 24, 0xf762, 0xc5bb, 0);
        event_wait(24);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 0.0F,
                           0.0F);
        event_play_action(event_get_player(0), &sonic_sprint_action,
                          &sonic_texlist, 2.2F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 29, 407.758F, 4.178F, 538.59F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 29, 0, 0xcb00, 0);
        event_wait(29);
        event_talk_start(event_get_player(0), TEXT_1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 22, 360.95F, -6.86F, 561.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 22, 0x400, 0xd600, 0xfc00);
        event_wait(22);
        event_look_clear(event_get_player(0));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 0.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 18, 307.528F, -0.3F, 542.5785F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 18, 0xfc00, 0xaf00, 0xfc00);
        event_wait(18);
        event_camera_set_pos(EVENT_EASE_LINEAR, 26, 257.1233F, 13.0F, 500.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 26, 0xf900, 0xaf00, 0x200);
        event_wait(26);
        event_camera_set_pos(EVENT_EASE_LINEAR, 21, 257.0F, 27.55F, 457.978F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 21, 0xf911, 0x8b00, 0x1df);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 269.774F, 63.16F, 357.128F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xf600, 0xa000, 0x1df);
        event_set_pos(event_get_player(2), 2088.27F, 186.47F, 1560.4F);
        ev0026_flash = event_fade_create(0.1F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_wait(20);
        event_clear_paths(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_sprint_action,
                          &sonic_texlist, 0.7F, EVENT_REPEAT, 0);
        TAILS_FLAGS(*tails_slot) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(*tails_slot) &= ~TAILS_TAIL_STILL;
        TAILS_FLAGS(*tails_slot) &= ~TAILS_FLYING_HEAD;
        event_clouds_start(2.5F, 6);
        event_talk_stop(event_get_player(0));
        event_talk_stop(event_get_player(2));
        event_clear_paths(event_get_player(0));
        event_clear_paths(event_get_player(2));
        event_clear_motions(event_get_player(0));
        event_clear_motions(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 2.0F,
                           0.0F);
        event_set_pos(event_get_player(2), 2088.27F, 186.47F, 1560.4F);
        event_set_ang(event_get_player(2), 0xffba, 0x30e5, 0x33);
        event_set_pos(event_get_player(0), 2099.88F, 186.476F, 1556.354F);
        event_set_ang(event_get_player(0), 0xffba, 0x22cb, 0x33);
        event_play_action(event_get_player(2), &ev0026_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_wait(10);
        event_set_ang(event_get_player(0), 0xffba, 0x22cb, 0x33);
        event_wait(10);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 2095.6F, 187.126F, 1547.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfc1, 0x93bd, 0);
        if (ev0026_flash != NULL) {
            event_fade_set_speed(ev0026_flash, -1.0F / 60.0F);
        }
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 2111.7F, 187.545F,
                             1552.053F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0xedb, 0x50b4, 0x400);
        event_talk_start(event_get_player(0), TEXT_3);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 2110.9F, 187.08F,
                             1565.4435F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x1264, 0x23fe, 0x400);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 2141.0F, 165.0F, 1591.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 55, 0x1300, 0x2600, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 55, 0x471c);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_clear_motions(event_get_player(0));
        SET_SONIC_FINGERS(ev0026_sonic_fingers);
        event_play_action(event_get_player(0), &ev0026_sonic_action_2,
                          &sonic_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev0026_sonic_action_3,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(43);
        event_camera_set_pos(EVENT_EASE_LINEAR, 4, 2137.34F, 173.545F,
                             1589.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x460, 0x2600, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 4, 0x2aab);
        event_wait(40);
        ev0026_fade = event_fade_create(1.0F / 30.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(30);
        break;
    case EVENT_END:
        tails_slot = &player_work_2;
        TAILS_FLAGS(*tails_slot) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(*tails_slot) &= ~TAILS_TAIL_STILL;
        TAILS_FLAGS(*tails_slot) &= ~TAILS_FLYING_HEAD;
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_clear_paths(event_get_player(0));
        event_clear_paths(event_get_player(2));
        SET_SONIC_FINGERS(sonic_fingers_minus_z);
        event_set_pos(event_get_player(0), 1199.0F, 126.0F, 851.0F);
        event_reset_player(0);
        event_kill_player(2);
        if (ev0026_flash != NULL) {
            task_kill(ev0026_flash);
            ev0026_flash = NULL;
        }
        if (ev0026_fade != NULL) {
            /* @hack spelling: the kill and the clear as one statement. SHC then
             * reads the slot again for the call (MOV.L @R0,R4 at 0x0cb812e8).
             * as two statements, the test's load passes the task, and 9 of
             * ev0026_script's 1737 words differ */
            task_kill(ev0026_fade), ev0026_fade = NULL;
        }
        event_clouds_stop();
        event_eggmobile_kill();
        EVENT_KILL(ev0026_smoke);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0026_textures[] = {
    {"MILES", &tails_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"EV_ECCLOUD", &event_clouds_texlist},
    {NULL, NULL},
};
