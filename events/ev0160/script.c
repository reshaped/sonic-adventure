/* EV0160: the echidnas reach the altar in the past, the emeralds circle above
 * it, and Chaos rises before Pachacamac */
#include <ninja.h>

#include "chaos/models.h"
#include "ev0160/echidna_normal_textures.h"
#include "ev0160/emerald_textures.h"
#include "ev0160/pachacamac_action.h"
#include "ev0160/script.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "shared/chaos0.h"
#include "shared/chaos0_water_textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/orbit.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 22
#define TEXT_2_LENGTH 19
#define TEXT_3_LENGTH 33
#include "ev0160/script/text.inc"

/* the flame's sprite slot */
#define FLAME_SLOT 10

extern NJS_ACTION ev0160_tikal_action;
extern NJS_ACTION ev0160_chao_action_0;
extern NJS_ACTION ev0160_chao_action_1;
extern NJS_ACTION ev0160_chao_action_2;

/* @hack spelling: event_sprite_jet_texlist is
 * event_sprite_texlists[EVENT_SPRITE_JET], declared under a name of its own at
 * the element's address, 0x0cbd8948. with the name, SHC loads the address from
 * a pool word at each of the seven calls, as at 0x0cb80b26. as
 * &event_sprite_texlists[EVENT_SPRITE_JET], SHC keeps the address in a stack
 * slot it adds to the frame. ev0160_script then runs 8 bytes long, and 1736 of
 * 1781 words differ */
extern NJS_TEXLIST event_sprite_jet_texlist;

extern NJS_TEXLIST ev0160_emerald_blue_texlist;
extern NJS_ACTION ev0160_emerald_blue_action;
extern NJS_ACTION ev0160_emerald_green_action;
extern NJS_ACTION ev0160_emerald_purple_action;
extern NJS_ACTION ev0160_emerald_sky_action;
extern NJS_ACTION ev0160_emerald_white_action;
extern NJS_ACTION ev0160_emerald_yellow_action;
extern NJS_ACTION ev0160_emerald_red_action;

/* the pale green fade that comes over the screen with the last message,
 * TEXT_2. it deepens for 60 steps, then clears at once as the camera returns
 * to Sonic */
static struct task *ev0160_message_fade = NULL;
/* the black fade the event opens on. it clears over 40 steps, from 31 steps
 * in */
static struct task *ev0160_fade = NULL;
static struct task *ev0160_emerald_blue = NULL;
static struct task *ev0160_emerald_green = NULL;
static struct task *ev0160_emerald_purple = NULL;
static struct task *ev0160_emerald_sky = NULL;
static struct task *ev0160_emerald_white = NULL;
static struct task *ev0160_emerald_yellow = NULL;
static struct task *ev0160_emerald_red = NULL;
static struct task *ev0160_chao_0 = NULL;
static struct task *ev0160_chao_1 = NULL;
static struct task *ev0160_chao_2 = NULL;
static struct task *ev0160_pachacamac = NULL;
static struct task *ev0160_echidna_normal = NULL;
static struct task *ev0160_echidna_stout = NULL;
static struct task *ev0160_echidna_long = NULL;

void ev0160_script(int phase)
{
    /* @hack spelling: ev0160_echidna_action_0 through a local. SHC then keeps
     * its address in R13 for the normal echidna's event_play_action and the
     * other two's motion reads (0x0cb804f8). without it, ev0160_script runs 4
     * bytes long, and 1294 of 1781 words differ */
    NJS_ACTION *echidna_action;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_00F7.mlt");
        event_sound_init(5);
        stream_request_music(MUSIC_EVTBGM04);
        event_set_pos(event_get_player(0), 0.0F, -71.0F, 797.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_place_actor(&ev0160_emerald_blue, -8.63F, 94.55F, 20.32F, 0,
                          0x4000, 0x2000);
        event_place_actor(&ev0160_emerald_green, 8.63F, 94.55F, 20.32F, 0,
                          0xb000, 0xe000);
        event_place_actor(&ev0160_emerald_purple, 20.0F, 94.83F, 7.87F, 0,
                          0x2000, 0x2000);
        event_place_actor(&ev0160_emerald_sky, -20.0F, 94.83F, 7.87F, 0, 0x6000,
                          0xe000);
        event_place_actor(&ev0160_emerald_white, 18.56F, 94.6F, -6.0F, 0,
                          0x8000, 0x2000);
        event_place_actor(&ev0160_emerald_yellow, -18.56F, 94.6F, -6.0F, 0,
                          0x4000, 0xe000);
        event_place_actor(&ev0160_emerald_red, 0.0F, 94.6F, -22.0F, 0, 0x4000,
                          0xe000);
        event_create_player(2, tikal_player_step, -15.8F, -4.2F, 434.39F,
                            0x04f0, 0x4c00, 0);
        event_place_actor(&ev0160_echidna_stout, 4.8F, 6.0F, 408.74F, 0, 0xc000,
                          0);
        event_place_actor(&ev0160_echidna_normal, -4.1F, 6.0F, 410.5F, 0,
                          0xc000, 0);
        event_place_actor(&ev0160_echidna_long, -11.0F, 6.0F, 406.3F, 0, 0xc000,
                          0);
        event_set_flags(ev0160_echidna_normal, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_echidna_stout, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_echidna_long, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0160_echidna_normal, 1.0F);
        event_set_shadow(ev0160_echidna_stout, 1.0F);
        event_set_shadow(ev0160_echidna_long, 1.0F);
        event_place_actor(&ev0160_pachacamac, -261.3F, 3.0F, 485.0F, 0, 0x4000,
                          0);
        event_set_flags(ev0160_pachacamac, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0160_pachacamac, 1.0F);
        event_place_task(&ev0160_chao_0, event_chao_create, -28.0F, -3.0F,
                         431.0F, 0, 0x3000, 0);
        event_place_task(&ev0160_chao_1, event_chao_create, -15.0F, -2.0F,
                         426.0F, 0, 0xe000, 0);
        event_place_task(&ev0160_chao_2, event_chao_create, -27.0F, -4.0F,
                         440.0F, 0, 0x7300, 0);
        event_set_flags(ev0160_chao_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_chao_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_chao_2, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0160_chao_0, 0.4F);
        event_set_shadow(ev0160_chao_1, 0.4F);
        event_set_shadow(ev0160_chao_2, 0.4F);
        event_chaos0_create(0.0F, 85.0F, 43.4F, 0, 0x4000, 0, 80);
        ev0160_fade = event_fade_create(1.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(30);
        event_wait(1);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then moves 0 into a
         * scratch register for each later call that passes 0, as into R2 at
         * 0x0cb80436. as an if, SHC keeps 0 in R14 for those calls,
         * ev0160_script runs 56 bytes short, and 1509 of 1781 words differ */
        ev0160_fade != NULL && (event_fade_set_speed(ev0160_fade, -0.025F), 0);
        event_talk_start(event_get_player(2), TEXT_3);
        event_play_action(event_get_player(2), &ev0160_tikal_action,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 0);
        event_chaos0_puddle();
        event_set_flags(ev0160_emerald_blue, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_emerald_green, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_emerald_purple, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_emerald_sky, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_emerald_white, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_emerald_yellow, EVENT_IGNORE_GROUND);
        event_set_flags(ev0160_emerald_red, EVENT_IGNORE_GROUND);
        event_play_action(ev0160_chao_0, &ev0160_chao_action_0,
                          &event_chao_texlist, 0.05F, EVENT_REPEAT, 4);
        event_play_action(ev0160_chao_1, &ev0160_chao_action_1,
                          &event_chao_texlist, 0.05F, EVENT_REPEAT, 4);
        event_play_action(ev0160_chao_2, &ev0160_chao_action_2,
                          &event_chao_texlist, 0.05F, EVENT_REPEAT, 4);
        event_chao_set_eyes(ev0160_chao_0, CHAO_EYES_SING);
        event_chao_set_eyes(ev0160_chao_1, CHAO_EYES_SING);
        event_chao_set_eyes(ev0160_chao_2, CHAO_EYES_SING);
        event_clear_motions(ev0160_echidna_normal);
        event_clear_motions(ev0160_echidna_stout);
        event_clear_motions(ev0160_echidna_long);
        echidna_action = &ev0160_echidna_action_0;
        event_play_action(ev0160_echidna_normal, echidna_action,
                          &ev0160_echidna_normal_texlist, 1.0F, EVENT_REPEAT,
                          0);
        event_play_motion(ev0160_echidna_stout, &ev0160_echidna_stout_object,
                          echidna_action->motion, &ev0160_echidna_stout_texlist,
                          1.0F, EVENT_REPEAT, 4);
        event_play_motion(ev0160_echidna_long, &ev0160_echidna_long_object,
                          echidna_action->motion, &ev0160_echidna_long_texlist,
                          1.0F, EVENT_REPEAT, 4);
        event_move(ev0160_echidna_normal, -14.22F, 1.0F, 447.35F, 14.22F, 6.0F,
                   395.0F, 55);
        event_move(ev0160_echidna_stout, -5.9F, 1.0F, 470.0F, -5.9F, 6.0F,
                   400.0F, 60);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -33.0F, -2.076F, 439.35F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -33.0F, -2.076F, 433.35F);
        event_wait(20);
        event_move(ev0160_echidna_long, -27.0F, 1.0F, 460.0F, -27.0F, 6.0F,
                   400.0F, 65);
        event_wait(50);
        event_move_stop_all();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0xee00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -37.0F, 3.9F, 409.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1000, 0x1100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 23.5F, 1.5F, 323.0F);
        event_move(ev0160_echidna_normal, 14.22F, 6.0F, 395.0F, 13.0F, 6.0F,
                   312.0F, 80);
        event_move(ev0160_echidna_stout, -5.9F, 6.0F, 400.0F, -2.0F, 6.0F,
                   297.0F, 80);
        event_move(ev0160_echidna_long, -27.0F, 6.0F, 400.0F, -18.3F, 6.0F,
                   312.0F, 80);
        event_wait(81);
        /* raise each emerald to a y of 120, the height event_orbit_create
         * circles them at */
        event_set_pos(ev0160_emerald_blue, -8.63F, 120.0F, 20.32F);
        event_set_ang(ev0160_emerald_blue, 0, 0x4000, 0x2000);
        event_set_pos(ev0160_emerald_green, 8.63F, 120.0F, 20.32F);
        event_set_ang(ev0160_emerald_green, 0, 0xb000, 0xe000);
        event_set_pos(ev0160_emerald_purple, 20.0F, 120.0F, 7.87F);
        event_set_ang(ev0160_emerald_purple, 0, 0x2000, 0x2000);
        event_set_pos(ev0160_emerald_sky, -20.0F, 120.0F, 7.87F);
        event_set_ang(ev0160_emerald_sky, 0, 0x6000, 0xe000);
        event_set_pos(ev0160_emerald_white, 18.56F, 120.0F, -6.0F);
        event_set_ang(ev0160_emerald_white, 0, 0x8000, 0x2000);
        event_set_pos(ev0160_emerald_yellow, -18.56F, 120.0F, -6.0F);
        event_set_ang(ev0160_emerald_yellow, 0, 0x4000, 0xe000);
        event_set_pos(ev0160_emerald_red, 0.0F, 120.0F, -22.0F);
        event_set_ang(ev0160_emerald_red, 0, 0x4000, 0xe000);
        event_play_action(ev0160_emerald_blue, &ev0160_emerald_blue_action,
                          &ev0160_emerald_blue_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0160_emerald_green, &ev0160_emerald_green_action,
                          &ev0160_emerald_green_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0160_emerald_white, &ev0160_emerald_white_action,
                          &ev0160_emerald_white_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0160_emerald_purple, &ev0160_emerald_purple_action,
                          &ev0160_emerald_purple_texlist, 1.0F, EVENT_REPEAT,
                          0);
        event_play_action(ev0160_emerald_sky, &ev0160_emerald_sky_action,
                          &ev0160_emerald_sky_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0160_emerald_yellow, &ev0160_emerald_yellow_action,
                          &ev0160_emerald_yellow_texlist, 1.0F, EVENT_REPEAT,
                          0);
        event_play_action(ev0160_emerald_red, &ev0160_emerald_red_action,
                          &ev0160_emerald_red_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0160_chao_0, -10.6F, 56.3F, 158.38F);
        event_set_pos(ev0160_chao_1, 11.0F, 84.5F, 114.3F);
        event_set_pos(ev0160_echidna_normal, 12.5F, 96.8F, 76.0F);
        event_set_pos(ev0160_echidna_stout, -7.6F, 96.8F, 85.676F);
        /* @hack spelling: the motion as (&ev0160_echidna_action_2)->motion. SHC
         * then reads it 4 bytes past the action's address, which it loads into
         * R6 (MOV.L @(4,R6),R6 at 0x0cb80a80). as
         * ev0160_echidna_action_2.motion, SHC loads the motion's own address
         * into R1, and 12 of 1781 words differ */
        event_play_motion(ev0160_echidna_stout, &ev0160_echidna_stout_object,
                          (&ev0160_echidna_action_2)->motion,
                          &ev0160_echidna_stout_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev0160_echidna_normal, &ev0160_echidna_action_2,
                          &ev0160_echidna_normal_texlist, 0.7F, EVENT_REPEAT,
                          0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1800, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 40.4F, 190.6F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 0.0F, 101.4F, 102.5F);
        /* the camera's x angle swings between 0x1600 and 0x1800 as it climbs,
         * every 2 or 3 steps for 50 steps */
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_sound_start(0, 1230, 1800);
        event_sound_fade_volume(0, -60, 1);
        event_orbit_create(0.0F, 120.0F, 0.0F, 30.0F, 182, ev0160_emerald_blue,
                           ev0160_emerald_green, ev0160_emerald_purple,
                           ev0160_emerald_sky, ev0160_emerald_white,
                           ev0160_emerald_yellow, ev0160_emerald_red);
        event_orbit_add_sprite(1, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.0F,
                               0.0F, 1.0F);
        event_orbit_add_sprite(2, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.0F,
                               1.0F, 0.0F);
        event_orbit_add_sprite(3, &event_sprite_jet_texlist, 0.4F, 0.7F, 1.0F,
                               0.0F, 1.0F);
        event_orbit_add_sprite(4, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.5F,
                               0.5F, 1.0F);
        event_orbit_add_sprite(5, &event_sprite_jet_texlist, 0.4F, 0.7F, 1.0F,
                               1.0F, 1.0F);
        event_orbit_add_sprite(6, &event_sprite_jet_texlist, 0.4F, 0.7F, 0.3F,
                               1.0F, 0.5F);
        event_orbit_add_sprite(7, &event_sprite_jet_texlist, 0.4F, 0.7F, 1.0F,
                               0.0F, 0.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_sound_fade_volume(0, 60, 50);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1600, 0, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0xfe00, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 5, 0.0F, 99.4F, 102.5F);
        event_wait(3);
        event_move_stop_all();
        event_kill_player(2);
        event_kill_task(&ev0160_chao_2);
        event_chaos0_slide(0.0F, 90.0F, 43.4F, 15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 0.0F, 99.4F, 55.5F);
        event_wait(15);
        event_sound_play(1224, 0, 0, 0);
        event_chaos0_rise();
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 8, 0xfe00, 0, 0);
        event_wait(20);
        event_wait(35);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 25, 0x0d00, 0, 0);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 0.0F, 95.0F, 60.0F);
        event_wait(30);
        event_kill_task(&ev0160_echidna_normal);
        event_kill_task(&ev0160_chao_0);
        event_kill_task(&ev0160_chao_1);
        event_set_pos(ev0160_pachacamac, 0.0F, 96.8F, 72.0F);
        event_set_pos(ev0160_echidna_stout, -10.0F, 96.8F, 78.0F);
        event_set_pos(ev0160_echidna_long, 8.5F, 96.8F, 82.0F);
        event_set_ang(ev0160_pachacamac, 0, 0x8000, 0);
        event_set_ang(ev0160_echidna_stout, 0, 0x8000, 0);
        event_set_ang(ev0160_echidna_long, 0, 0x8000, 0);
        event_clear_motions(ev0160_pachacamac);
        event_clear_motions(ev0160_echidna_stout);
        event_clear_motions(ev0160_echidna_long);
        event_play_action(ev0160_pachacamac, &ev0160_pachacamac_action,
                          &ev0160_pachacamac_texlist, 0.6F, 0, 0);
        /* @hack spelling: both motions as (&ev0160_echidna_action_1)->motion.
         * SHC then reads each 4 bytes past the action's address (0x0cb80f0e,
         * 0x0cb80f28). as ev0160_echidna_action_1.motion, R10 and R11 trade
         * throughout ev0160_script, from event_set_pos's R11 at 0x0cb80086 on,
         * and 55 of 1781 words differ */
        event_play_motion(ev0160_echidna_stout, &ev0160_echidna_stout_object,
                          (&ev0160_echidna_action_1)->motion,
                          &ev0160_echidna_stout_texlist, 0.43F, 0, 0);
        event_play_motion(ev0160_echidna_long, &ev0160_echidna_long_object,
                          (&ev0160_echidna_action_1)->motion,
                          &ev0160_echidna_long_texlist, 0.43F, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -7.9F, 91.0F, 41.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a00, 0x8f00, 0);
        event_voice_play(1951);
        event_message_show(TEXT_1);
        event_sound_play(1225, 120, 0, 0);
        event_move(ev0160_pachacamac, 0.0F, 96.8F, 72.0F, 0.0F, 96.8F, 80.0F,
                   30);
        event_move(ev0160_echidna_long, 8.5F, 96.8F, 82.0F, 8.5F, 96.8F, 91.0F,
                   30);
        event_move(ev0160_echidna_stout, -10.0F, 96.8F, 78.0F, -10.0F, 96.8F,
                   88.0F, 30);
        event_wait_motions(ev0160_pachacamac);
        event_chaos0_set_speed(0.4F);
        event_sprite_create(NULL, FLAME_SLOT, 0.0F, 105.0F, 37.0F,
                            &event_sprite_texlists[EVENT_SPRITE_FLAME]);
        event_sprite_set_scale(FLAME_SLOT, 2.5F, 2.5F);
        event_sprite_set_speed(FLAME_SLOT, 0.3F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.14F, 106.08F, 56.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf500, 0xfa00, 0x0400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 1.14F, 106.08F, 56.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xf500, 0x0600, -0x0400);
        event_message_close();
        event_wait(80);
        event_sound_play(1227, 0, 0, 0);
        event_wait(20);
        event_sprite_kill(FLAME_SLOT);
        event_rays_create(0.0F, 100.0F, 0.0F, 5, 10, 0.7F, 0.3F, 0xff00ff00,
                          0x88ffffff);
        event_sound_fade_volume(0, 120, 20);
        event_sound_play(1229, 120, 0, 0);
        event_sound_play(1228, 120, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 101.0F, 57.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1000, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 20, 0x5555);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 0.0F, 90.8F, 64.67F);
        event_wait(40);
        ev0160_message_fade =
            event_fade_create(1.0F / 70.0F, 0.1F, 0.75F, 1.0F, 0.75F);
        event_voice_play(1952);
        event_message_show(TEXT_2);
        event_wait(20);
        event_message_close();
        event_wait(40);
        event_sound_stop(0);
        if (ev0160_message_fade != NULL) {
            event_fade_set_speed(ev0160_message_fade, -1.0F);
        }
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -5.4F, -69.25F, 811.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0xf900, 0);
        event_wait(10);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0160_pachacamac);
        event_chaos0_kill();
        event_kill_task(&ev0160_echidna_normal);
        event_kill_task(&ev0160_echidna_stout);
        event_kill_task(&ev0160_echidna_long);
        event_kill_task(&ev0160_chao_0);
        event_kill_task(&ev0160_chao_1);
        event_kill_task(&ev0160_chao_2);
        event_sprite_kill(FLAME_SLOT);
        event_orbit_kill();
        event_kill_task(&ev0160_emerald_blue);
        event_kill_task(&ev0160_emerald_red);
        event_kill_task(&ev0160_emerald_yellow);
        event_kill_task(&ev0160_emerald_sky);
        event_kill_task(&ev0160_emerald_green);
        event_kill_task(&ev0160_emerald_white);
        event_kill_task(&ev0160_emerald_purple);
        event_rays_kill();
        EVENT_KILL(ev0160_message_fade);
        EVENT_KILL(ev0160_fade);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0160_textures[] = {
    {"CHAOS0", &event_chaos0_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {"EV_K_PATYA", &ev0160_pachacamac_texlist},
    {"KNUCKLES_NORMAL", &ev0160_echidna_normal_texlist},
    {"KNUCKLES_LONG", &ev0160_echidna_long_texlist},
    {"KNUCKLES_DEBU", &ev0160_echidna_stout_texlist},
    {"EV_ALIFE", &event_chao_texlist},
    {"TIKAL", &tikal_texlist},
    {"M_EM_BLUE", &ev0160_emerald_blue_texlist},
    {"M_EM_GREEN", &ev0160_emerald_green_texlist},
    {"M_EM_PURPLE", &ev0160_emerald_purple_texlist},
    {"M_EM_SKY", &ev0160_emerald_sky_texlist},
    {"M_EM_RED", &ev0160_emerald_red_texlist},
    {"M_EM_WHITE", &ev0160_emerald_white_texlist},
    {"M_EM_YELLOW", &ev0160_emerald_yellow_texlist},
    {NULL, NULL},
};
