/* EV00C7, E-102's story ends: E-101 mkII and then E-102 are destroyed, and two
 * birds fly off */
#include <ninja.h>

#include "ev00c7/e102_action_7.h"
#include "ev00c7/pink_bird_action.h"
#include "ev00c7/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/dome.h"
#include "shared/dust.h"
#include "shared/fade.h"
#include "shared/lightning.h"
#include "shared/move.h"
#include "shared/noise.h"
#include "shared/picture.h"
#include "shared/rays.h"
#include "shared/scanlines.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_ACTION e102_slide_stand_action;

extern NJS_ACTION ev00c7_e102_action_0;
extern NJS_ACTION ev00c7_e102_action_1;
extern NJS_ACTION ev00c7_e102_action_2;
extern NJS_ACTION ev00c7_e102_action_3;
extern NJS_ACTION ev00c7_e102_action_4;
extern NJS_ACTION ev00c7_e102_action_5;
extern NJS_ACTION ev00c7_e102_action_6;
extern NJS_TEXLIST ev00c7_pink_bird_texlist;
extern NJS_ACTION ev00c7_gray_bird_action_0;
extern NJS_ACTION ev00c7_gray_bird_action_1;
extern NJS_TEXLIST ev00c7_picture_texlist;

static struct event_path_point ev00c7_path_0_points[32] = {
#include "ev00c7/script/ev00c7_path_0_points.inc"
};
static struct event_path ev00c7_path_0 = {32, ev00c7_path_0_points};

/* no code uses paths 1 and 2. they keep ev00c7_path_3 at its address in
 * the image */
static struct event_path_point ev00c7_path_1_points[36] = {
#include "ev00c7/script/ev00c7_path_1_points.inc"
};
static struct event_path ev00c7_path_1 = {36, ev00c7_path_1_points};
static struct event_path_point ev00c7_path_2_points[55] = {
#include "ev00c7/script/ev00c7_path_2_points.inc"
};
static struct event_path ev00c7_path_2 = {55, ev00c7_path_2_points};

static struct event_path_point ev00c7_path_3_points[59] = {
#include "ev00c7/script/ev00c7_path_3_points.inc"
};
static struct event_path ev00c7_path_3 = {59, ev00c7_path_3_points};

static struct task *ev00c7_picture = NULL;
static struct task *ev00c7_white_0 = NULL;
static struct task *ev00c7_white_1 = NULL;
static struct task *ev00c7_black_0 = NULL;
static struct task *ev00c7_black_1 = NULL;
static struct task *ev00c7_fade_out = NULL;
static struct task *ev00c7_e101 = NULL;
static struct task *ev00c7_gray_bird = NULL;
/* an actor the script places and moves beside the gray bird, but plays no
 * motion on, so event_actor_step never draws it */
static struct task *ev00c7_unseen = NULL;
static struct task *ev00c7_pink_bird = NULL;
static struct task *ev00c7_smoke_0 = NULL;
static struct task *ev00c7_smoke_1 = NULL;
static struct task *ev00c7_e101_dome = NULL;
static struct task *ev00c7_e102_dome = NULL;

void ev00c7_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_00C5.mlt");
        event_sound_init(5);
        ev00c7_picture = event_picture_create(&ev00c7_picture_texlist);
        event_picture_set_texture(ev00c7_picture, 0);
        event_picture_set_depth(ev00c7_picture, -1.14F);
        ev00c7_e101_dome =
            event_dome_create(-110.4F, 749.0F, -305.0F, 290, 2.0F, 380.0F);
        ev00c7_e102_dome =
            event_dome_create(36.5F, 745.0F, -194.4F, 550, 2.0F, 350.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        if (ev00c7_smoke_0 == NULL) {
            ev00c7_smoke_0 = event_dust_create();
        }
        if (ev00c7_smoke_1 == NULL) {
            ev00c7_smoke_1 = event_dust_create();
        }
        /* @hack spelling: each dust task's test and its color stores as one &&
         * expression. each color value then goes through FPUL into FR3 just
         * before its first store (0x0cb80116). as ifs, SHC holds 0.5 in FR12
         * from before the first test, and 2392 of 2562 words differ */
        ev00c7_smoke_0 != NULL &&
            (DUST_WORK(ev00c7_smoke_0)->params->color.a = 0.2F,
             DUST_WORK(ev00c7_smoke_0)->params->color.r = 0.5F,
             DUST_WORK(ev00c7_smoke_0)->params->color.g = 0.5F,
             DUST_WORK(ev00c7_smoke_0)->params->color.b = 0.5F);
        ev00c7_smoke_1 != NULL &&
            (DUST_WORK(ev00c7_smoke_1)->params->color.a = 0.5F,
             DUST_WORK(ev00c7_smoke_1)->params->color.r = 0.3F,
             DUST_WORK(ev00c7_smoke_1)->params->color.g = 0.3F,
             DUST_WORK(ev00c7_smoke_1)->params->color.b = 0.3F);
        event_noise_create();
        event_set_pos(event_get_player(0), -6.33F, 745.6F, -219.78F);
        event_set_ang(event_get_player(0), 0x0317, 0xa620, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 0.4F, EVENT_REPEAT, 8);
        event_place_actor(&ev00c7_gray_bird, -100.0F, 5.5F, -5.5F, 0, 0x2000,
                          0);
        event_set_flags(ev00c7_gray_bird, EVENT_IGNORE_GROUND);
        event_place_actor(&ev00c7_unseen, -100.0F, 0.5F, -5.5F, 0, 0x2000, 0);
        event_set_flags(ev00c7_unseen, EVENT_IGNORE_GROUND);
        event_place_actor(&ev00c7_pink_bird, -100.0F, 0.5F, -5.5F, 0, 0x2000,
                          0);
        event_set_flags(ev00c7_pink_bird, EVENT_IGNORE_GROUND);
        event_place_actor(&ev00c7_e101, -108.23F, 747.3F, -306.56F, 0x01d0,
                          0x20fb, 0x028b);
        event_wait(1);
        event_set_flags(ev00c7_e101, EVENT_IGNORE_GROUND);
        event_set_pos(ev00c7_e101, -108.23F, 797.3F, -306.56F);
        event_set_ang(ev00c7_e101, 0x01d0, 0x2420, 0x028b);
        event_play_action(ev00c7_gray_bird, &ev00c7_gray_bird_action_1,
                          &ev00c7_gray_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0x0317, 0xa620, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 10.408F, 757.2466F,
                             -206.845F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0x2c20, 0x0600);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 13.8F, 745.376F, -220.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x1200, 0x3520, 0x0600);
        event_wait(30);
        event_sound_play(766, -50, 0, 0);
        event_play_action(ev00c7_e101, &ev00c7_e101_action_1,
                          &ev00c7_e101_texlist, 1.3F, 0, 0);
        event_wait(20);
        event_play_action(ev00c7_e101, &ev00c7_e101_action_1,
                          &ev00c7_e101_texlist, 0.9F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(ev00c7_e101, &ev00c7_e101_action_0,
                          &ev00c7_e101_texlist, 0.4F, EVENT_REPEAT, 8);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 15.24F, 746.3F, -225.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x11ea, 0x3aef, 0x0700);
        event_wait(90);
        event_move_turn(ev00c7_e101, -108.23F, 797.3F, -306.56F, -108.23F,
                        785.3F, -306.56F, 0x01d0, 0x2420, 0x028b, 0x9000,
                        0x9000, 0xe000, 17);
        event_wait(20);
        event_move(ev00c7_e101, -108.23F, 785.3F, -306.56F, -108.23F, 757.0F,
                   -306.56F, 8);
        event_wait(8);
        event_sound_play(782, 0, 0, 0);
        event_move(ev00c7_e101, -108.23F, 757.0F, -306.56F, -108.23F, 760.0F,
                   -306.56F, 2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x12ea, 0x3aef, 0x0700);
        event_wait(2);
        event_move(ev00c7_e101, -108.23F, 760.0F, -306.56F, -108.23F, 759.0F,
                   -306.56F, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x10ea, 0x3aef, 0x0700);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x11ea, 0x3aef, 0x0700);
        event_clear_motions(ev00c7_e101);
        event_play_action(ev00c7_e101, &ev00c7_e101_action_1,
                          &ev00c7_e101_texlist, 1.0F, 0, 8);
        event_play_action(ev00c7_e101, &ev00c7_e101_action_0,
                          &ev00c7_e101_texlist, 0.4F, EVENT_REPEAT, 8);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 18.3F, 748.78F, -240.937F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00c7_e102_action_0,
                          &e102_texlist, 0.65F, EVENT_REPEAT, 0);
        event_sound_start(0, 782, 1800);
        event_run_to_speed(event_get_player(0), -72.7F, 745.6F, -276.3F, 0.58F,
                           0.6F);
        event_wait(50);
        event_camera_set_fov(EVENT_EASE_LINEAR, 66, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -31.7F, 762.6F, -242.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf400, 0x2400, 0);
        event_wait(1);
        event_camera_follow(event_get_player(0));
        event_wait(70);
        event_move_turn(ev00c7_e101, -108.23F, 759.0F, -307.0F, -108.23F,
                        759.0F, -307.0F, -0x7000, 0x9000, -0x2000, 0x01d0,
                        0x1d00, 0x028b, 10);
        event_clear_motions(ev00c7_e101);
        event_play_action(ev00c7_e101, &ev00c7_e101_action_2,
                          &ev00c7_e101_texlist, 1.0F, 0, 0);
        event_wait(44);
        event_sound_play(1225, 0, 0, 0);
        event_sound_stop(0);
        stream_request_music(MUSIC_E102);
        event_set_pos(ev00c7_smoke_0, -103.37F, 766.4F, -302.23F);
        event_set_pos(ev00c7_smoke_1, -103.3F, 757.7F, -295.39F);
        event_sprite_create(event_get_player(0), 0, -4.0F, 15.0F, 4.0F,
                            &event_sprite_texlists[EVENT_SPRITE_EXPLOSION]);
        event_sprite_set_scale(2, 0.8F, 0.8F);
        event_run_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00c7_e102_action_1,
                          &e102_texlist, 0.8F, 0, 0);
        event_wait(2);
        event_move(event_get_player(0), -72.116F, 752.6F, -275.8F, -26.85F,
                   746.0F, -231.91F, 35);
        event_camera_follow_clear();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.584F, 749.29F, -228.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c00, 0x3500, 0);
        event_wait(16);
        event_sprite_kill(0);
        event_wait(19);
        event_sprite_create(NULL, 1, -108.23F, 759.0F, -306.56F,
                            &event_sprite_texlists[EVENT_SPRITE_LIGHTNING]);
        /* @hack spelling: each dust task's test and its timer and interval
         * stores as one && expression. SHC then leaves -100.0 in FR14 and -5.5
         * in FR15 (0x0cb80270): the x and z at which the script places the gray
         * bird, ev00c7_unseen and the pink bird. as ifs, FR14 and FR15 trade
         * the two values, and 6 of 2562 words and 2 pool words differ */
        ev00c7_smoke_0 != NULL && (DUST_WORK(ev00c7_smoke_0)->timer =
                                       DUST_WORK(ev00c7_smoke_0)->interval = 3);
        ev00c7_smoke_1 != NULL && (DUST_WORK(ev00c7_smoke_1)->timer =
                                       DUST_WORK(ev00c7_smoke_1)->interval = 4);
        event_move_turn_degrees(ev00c7_e101, -108.23F, 759.0F, -306.56F,
                                -108.23F, 759.0F, -306.56F, 464.0F, 9248.0F,
                                651.0F, 464.0F, 9504.0F, 651.0F, 60);
        event_play_action(ev00c7_e101, &ev00c7_e101_action_1,
                          &ev00c7_e101_texlist, 0.7F, EVENT_REPEAT, 18);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -8.4F, 749.29F, -223.42F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0c00, 0x2c00, 0);
        event_wait_motions(event_get_player(0));
        event_rays_create(-110.4F, 758.0F, -305.0F, 2, 20, 0.3F, 2.0F,
                          0xffffffff, 0x00ffffff);
        event_sprite_kill(1);
        event_play_action(event_get_player(0), &ev00c7_e102_action_2,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 12);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0c00, 0x2f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -5.4F, 752.57F, -224.76F);
        event_clear_motions(ev00c7_gray_bird);
        event_play_action(ev00c7_gray_bird, &ev00c7_gray_bird_action_1,
                          &ev00c7_gray_bird_texlist, 1.2F, 0, 0);
        event_play_action(ev00c7_gray_bird, &ev00c7_gray_bird_action_0,
                          &ev00c7_gray_bird_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(40);
        event_set_pos(ev00c7_smoke_0, 0.0F, 0.0F, 0.0F);
        event_set_pos(ev00c7_smoke_1, 0.0F, 0.0F, 0.0F);
        event_dome_set_mode(ev00c7_e101_dome, EVENT_DOME_GROW);
        event_wait(52);
        event_sound_start(1, 782, 1800);
        event_play_action(event_get_player(0), &ev00c7_e102_action_3,
                          &e102_texlist, 0.3F, 0, 16);
        event_wait(25);
        event_kill_task(&ev00c7_e101);
        event_move(ev00c7_gray_bird, -91.1F, 769.7F, -300.48F, -39.1F, 780.84F,
                   -263.0F, 55);
        event_wait_motions(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x2200, 0);
        event_set_pos(event_get_player(0), -21.6F, 744.0F, -228.1F);
        event_play_action(event_get_player(0), &ev00c7_e102_action_2,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0d00, 0x3400, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -3.1266F, 749.27F,
                             -223.434F);
        event_rays_kill();
        event_wait(20);
        event_play_action(event_get_player(0), &ev00c7_e102_action_6,
                          &e102_texlist, 0.6F, EVENT_REPEAT, 6);
        event_wait(30);
        event_camera_follow(event_get_player(0));
        event_move(ev00c7_gray_bird, -39.1F, 780.84F, -263.0F, -39.1F, 800.84F,
                   -280.0F, 60);
        event_follow(ev00c7_smoke_0, -6.0F, 20.0F, 0.0F, 9999,
                     event_get_player(0));
        event_follow(ev00c7_smoke_1, 6.0F, 10.0F, 3.0F, 9999,
                     event_get_player(0));
        event_run_to_speed(event_get_player(0), 5.5F, 745.0F, -201.54F, 0.3F,
                           0.2F);
        event_wait(80);
        event_move(ev00c7_gray_bird, -39.1F, 800.84F, -280.0F, 17.7F, 780.0F,
                   -206.5F, 60);
        event_run_wait(event_get_player(0));
        event_sound_stop(1);
        event_play_action(event_get_player(0), &ev00c7_e102_action_4,
                          &e102_texlist, 0.5F, 0, 4);
        event_play_action(event_get_player(0), &ev00c7_e102_action_4,
                          &e102_texlist, 0.1F, EVENT_REPEAT, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 13.9F, 756.75F, -199.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0d00, 0x3a00, 0x0100);
        event_wait(10);
        EVENT_KILL(ev00c7_e101_dome);
        event_set_pos(ev00c7_gray_bird, 17.4F, 755.0F, -188.4F);
        event_set_ang(ev00c7_gray_bird, 0, 0xa000, 0);
        event_play_action(ev00c7_gray_bird, &ev00c7_gray_bird_action_0,
                          &ev00c7_gray_bird_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(55);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 20.4F, 754.0F, -187.37F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 20, 0x1000, 0x2200, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 21.7845F, 753.22F,
                             -186.13F);
        event_wait(80);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00c7_e102_action_4,
                          &e102_texlist, 0.4F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -4.6F, 744.9F, -178.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1000, 0xe000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -9.0F, 744.9F, -182.77F);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 15.26F, 750.51F, -196.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1700, 0x8a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 16.287F, 753.186F,
                             -191.888F);
        event_wait(55);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e00, 0x4200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 11.46F, 758.3F, -200.113F);
        event_wait(40);
        event_play_action(event_get_player(0), &ev00c7_e102_action_6,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 4);
        event_sound_start(1, 782, 1800);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 12.95F, 755.85F, -217.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1000, 0x7000, 0);
        event_wait(20);
        event_set_ang(ev00c7_gray_bird, 0, 0x1000, 0);
        event_move(ev00c7_gray_bird, 31.9F, 758.0F, -205.47F, 8.35F, 758.0F,
                   -213.8F, 150);
        event_wait(20);
        event_move_turn(event_get_player(0), 4.97F, 743.6844F, -202.06F, 25.5F,
                        744.0F, -202.2F, 0x0317, 0x1500, 0, 0x0317, 0, 0, 180);
        event_wait(130);
        event_move_stop_all();
        event_sound_stop(1);
        event_wait(1);
        event_set_ang(ev00c7_gray_bird, 0, 0x1000, 0);
        event_move(ev00c7_gray_bird, 8.37F, 772.07F, -247.0F, -38.64F, 789.0F,
                   -282.1F, 150);
        event_set_pos(event_get_player(0), 0.8F, 743.0F, -231.4F);
        event_set_ang(event_get_player(0), 0, 0x2000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 11.37F, 762.26F, -227.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0x2a00, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 400, 47.5F, 761.0F, -191.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 400, 0xfd00, 0x3000, 0x0100);
        event_sound_start(3, 782, 1800);
        event_move_turn(event_get_player(0), 0.8F, 743.0F, -231.4F, 36.5F,
                        745.0F, -194.38F, 0, 0x2000, 0, 0, 0x2000, 0, 360);
        event_wait(50);
        ev00c7_white_0 = event_fade_create(1.0F / 3.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_wait(5);
        if (ev00c7_white_0 != NULL) {
            event_fade_set_speed(ev00c7_white_0, -1.0F / 3.0F);
        }
        event_wait(40);
        event_sound_play(1223, 0, 0, 0);
        event_picture_fade(ev00c7_picture, 1.0F, 5);
        ev00c7_black_0 = event_fade_create(0.25F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_noise_show(20, 255);
        event_wait(3);
        if (ev00c7_black_0 != NULL) {
            event_fade_set_speed(ev00c7_black_0, 0.0F);
        }
        event_wait(17);
        event_picture_fade(ev00c7_picture, 0.0F, 5);
        event_wait(4);
        event_scanlines_kill();
        if (ev00c7_black_0 != NULL) {
            event_fade_set_speed(ev00c7_black_0, -1.0F);
        }
        event_wait(31);
        event_play_path(ev00c7_gray_bird, &ev00c7_path_0, 0.2F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -31.266F, 783.627F,
                             -275.66F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1600, 0x2800, 0x0100);
        event_wait(56);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 31.5F, 761.06F, -206.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfdc0, 0x2f00, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 194, 47.5F, 761.0F, -191.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 194, 0xfd00, 0x3000, 0x0100);
        event_wait(30);
        ev00c7_white_1 = event_fade_create(1.0F / 3.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_wait(5);
        if (ev00c7_white_1 != NULL) {
            event_fade_set_speed(ev00c7_white_1, -1.0F / 3.0F);
        }
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -46.33F, 788.4F, -297.546F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b00, 0x0300, 0);
        event_wait(53);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 41.57F, 761.2F, -197.0366F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfdd2, 0x2efd, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 47.5F, 761.0F, -191.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0xfd00, 0x3000, 0x0100);
        if (ev00c7_white_1 != NULL) {
            event_fade_set_speed(ev00c7_white_1, 1.0F / 3.0F);
        }
        event_wait(5);
        if (ev00c7_white_1 != NULL) {
            event_fade_set_speed(ev00c7_white_1, -1.0F / 3.0F);
        }
        event_play_action(event_get_player(0), &ev00c7_e102_action_6,
                          &e102_texlist, 2.5F, EVENT_REPEAT, 5);
        event_wait(35);
        event_sound_stop(3);
        event_play_action(event_get_player(0), &ev00c7_e102_action_4,
                          &e102_texlist, 0.1F, EVENT_REPEAT, 5);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 45.545F, 761.72F,
                             -191.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf800, 0x3000, 0x0100);
        event_lightning_create(34.0F, 741.0F, -196.0F, 4);
        event_wait(100);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_picture_fade(ev00c7_picture, 1.0F, 5);
        ev00c7_black_1 = event_fade_create(0.25F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_noise_show(21, 255);
        event_wait(3);
        if (ev00c7_black_1 != NULL) {
            event_fade_set_speed(ev00c7_black_1, 0.0F);
        }
        event_wait(17);
        if (ev00c7_black_1 != NULL) {
            event_fade_set_speed(ev00c7_black_1, -1.0F);
        }
        event_wait(1);
        stream_stop_music();
        event_wait(65);
        event_picture_fade(ev00c7_picture, 0.0F, 5);
        EVENT_KILL(ev00c7_smoke_0);
        EVENT_KILL(ev00c7_smoke_1);
        event_scanlines_kill();
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00c7_e102_action_7,
                          &e102_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev00c7_e102_action_5,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 48.76F, 746.4F, -183.0226F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0x2b00, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 48.48F, 745.456F, -183.21F);
        event_wait(30);
        event_sound_play(782, 0, 0, 0);
        event_wait(160);
        event_camera_set_pos(EVENT_EASE_LINEAR, 230, 34.835F, 745.04F,
                             -172.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 230, 0x0e00, 0x0500, 0);
        event_wait(220);
        event_clear_motions(ev00c7_gray_bird);
        event_clear_motions(ev00c7_unseen);
        event_play_action(ev00c7_gray_bird, &ev00c7_gray_bird_action_0,
                          &ev00c7_gray_bird_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -50.9F, 801.9F, -351.544F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf7a0, 0x8760, 0);
        event_camera_follow(ev00c7_gray_bird);
        event_wait(1);
        event_move(ev00c7_gray_bird, -48.52F, 798.84F, -335.426F, -55.8F,
                   802.0F, -363.3F, 120);
        event_wait(120);
        stream_request_music(MUSIC_EVTBGM02);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 41.825F, 762.28F, -89.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x05e0, 0x0c90, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 45.2F, 770.39F, -49.177F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0200, 0x1000, 0);
        event_wait(50);
        event_rays_create(36.5F, 745.0F, -194.4F, 2, 20, 0.3F, 2.0F, 0xffffffff,
                          0x00ffffff);
        event_wait(50);
        event_dome_set_mode(ev00c7_e102_dome, EVENT_DOME_GROW);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -60.87F, 804.76F, -367.03F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf500, 0xa700, 0x0700);
        event_camera_follow(ev00c7_gray_bird);
        event_wait(1);
        event_move(ev00c7_unseen, -51.8F, 800.0F, -357.3F, -52.36F, 804.8F,
                   -376.657F, 150);
        event_move(ev00c7_gray_bird, -55.8F, 802.0F, -363.3F, -56.36F, 806.8F,
                   -382.657F, 150);
        event_wait(150);
        event_camera_follow_clear();
        event_set_pos(ev00c7_pink_bird, -47.5F, 769.6F, -254.55F);
        event_set_ang(ev00c7_pink_bird, 0, 0x8000, 0);
        event_play_action(ev00c7_pink_bird, &ev00c7_pink_bird_action,
                          &ev00c7_pink_bird_texlist, 0.7F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -56.18F, 803.0F, -367.55F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0xf800, 0x0700);
        event_move(ev00c7_gray_bird, -56.36F, 806.8F, -382.657F, -54.0F, 834.0F,
                   -420.0F, 350);
        event_camera_set_pos(EVENT_EASE_LINEAR, 360, -65.16F, 788.08F, -303.3F);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -56.0F, 819.42F, -405.69F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf200, 0x8900, 0x0600);
        event_camera_follow(ev00c7_gray_bird);
        event_wait(120);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -57.276F, 823.0F, -399.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0xfa00, 0x0600);
        event_wait(130);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -59.4F, 831.78F,
                             -414.176F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x1000, 0xeb00, 0x0600);
        event_wait(60);
        event_play_path(ev00c7_gray_bird, &ev00c7_path_3, 1.0F, 0);
        event_move(ev00c7_pink_bird, -54.55F, 827.6F, -392.55F, -55.9F, 835.6F,
                   -424.4F, 213);
        event_wait(100);
        EVENT_KILL(ev00c7_e102_dome);
        ev00c7_fade_out = event_fade_create(0.01F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(115);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_task(&ev00c7_e101);
        event_kill_task(&ev00c7_gray_bird);
        event_kill_task(&ev00c7_unseen);
        event_kill_task(&ev00c7_pink_bird);
        event_scanlines_kill();
        event_noise_kill();
        event_lightning_kill();
        EVENT_KILL(ev00c7_smoke_0);
        EVENT_KILL(ev00c7_smoke_1);
        EVENT_KILL(ev00c7_picture);
        EVENT_KILL(ev00c7_white_0);
        EVENT_KILL(ev00c7_white_1);
        EVENT_KILL(ev00c7_black_0);
        EVENT_KILL(ev00c7_black_1);
        EVENT_KILL(ev00c7_fade_out);
        EVENT_KILL(ev00c7_e101_dome);
        EVENT_KILL(ev00c7_e102_dome);
        event_rays_kill();
        event_sound_stop_all();
        event_sprite_kill(0);
        event_sprite_kill(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00c7_textures[] = {
    {"EV_E101KAI", &ev00c7_e101_texlist},
    {"WING_T", &ev00c7_gray_bird_texlist},
    {"WING_P", &ev00c7_pink_bird_texlist},
    {"VER2_WING", &ev00c7_blue_bird_texlist},
    {"ICM00C7", &ev00c7_picture_texlist},
    {NULL, NULL},
};
