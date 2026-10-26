/* EV008B, Knuckles' story: Eggman takes the two emeralds Sonic loses as he
 * clashes with Knuckles. Chaos 2 takes them and becomes Chaos 4 */
#include <ninja.h>

#include "chaos/models.h"
#include "ev008b/blue_emerald_textures.h"
#include "ev008b/green_emerald_textures.h"
#include "ev008b/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/beam.h"
#include "shared/chaos0_water_textures.h"
#include "shared/chaos2.h"
#include "shared/chaos4.h"
#include "shared/dust.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/rays.h"
#include "shared/shapes.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 32
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 37
#define TEXT_4_LENGTH 29
#define TEXT_5_LENGTH 26
#define TEXT_6_LENGTH 40
#define TEXT_7_LENGTH 49
#define TEXT_8_LENGTH 35
#define TEXT_9_LENGTH 17
#define TEXT_10_LENGTH 26
#define TEXT_11_LENGTH 27
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 15
#define TEXT_15_LENGTH 37
#define TEXT_16_LENGTH 3
#define TEXT_17_LENGTH 1
#define TEXT_18_LENGTH 5
#define TEXT_19_LENGTH 3
#define TEXT_20_LENGTH 1
#define TEXT_21_LENGTH 4
#include "ev008b/script/text.inc"

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_run_action;
extern NJS_ACTION knuckles_sprint_action;
/* @hack type: sonic_stand_action declared as an array, which script.c passes by
 * its name. SHC then keeps the camera's z, 85.1F, in FR13 for three
 * event_camera_set_pos calls (0x0cb80d12), and the blue emerald's z and y in
 * the frame slots at 36 and 56 for its event_move (0x0cb80f0e, 0x0cb80f18). as
 * one NJS_ACTION passed as &sonic_stand_action, FR13 holds the camera's x,
 * 84.08F, the two slots trade, and 17 of 3239 words differ */
extern NJS_ACTION sonic_stand_action[];
extern NJS_ACTION sonic_sprint_action;
extern NJS_ACTION sonic_jump_land_action;

extern NJS_TEXLIST event_eggmobile_texlist;

extern NJS_ACTION ev008b_knuckles_action_0;
extern NJS_ACTION ev008b_knuckles_action_1;
extern NJS_ACTION ev008b_knuckles_action_2;
extern NJS_ACTION ev008b_knuckles_action_3;
extern NJS_ACTION ev008b_knuckles_action_4;
/* @hack type: ev008b_sonic_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps the blue emerald's z and y in the frame slots at
 * 36 and 56 for its event_move (0x0cb80f0e, 0x0cb80f18). as one NJS_ACTION
 * passed as &ev008b_sonic_action_0, the two slots trade, and 4 of 3239 words
 * differ */
extern NJS_ACTION ev008b_sonic_action_0[];
extern NJS_ACTION ev008b_sonic_action_1;
extern NJS_ACTION ev008b_sonic_action_2;
extern NJS_ACTION ev008b_sonic_action_3;
extern NJS_ACTION ev008b_sonic_action_4;
extern NJS_ACTION ev008b_tails_action;
/* @hack type: ev008b_eggmobile_action_0 declared as an array, which script.c
 * passes by its name. SHC then keeps each emerald's x, y and z in the image's
 * frame slots for its event_move (0x0cb80f0e to 0x0cb80f70). as one NJS_ACTION
 * passed as &ev008b_eggmobile_action_0, all six take other slots, and 10 of
 * 3239 words differ */
extern NJS_ACTION ev008b_eggmobile_action_0[];
extern NJS_ACTION ev008b_eggmobile_action_1;
extern NJS_ACTION ev008b_eggmobile_action_2;
/* @hack type: ev008b_blue_emerald_action_0 declared as an array, which script.c
 * passes by its name. SHC then keeps the camera's z, 85.1F, in FR13
 * (0x0cb80d12) and the emeralds' coordinates in the image's frame slots for
 * their event_move calls (0x0cb80f0e to 0x0cb80f70). as one NJS_ACTION passed
 * as &ev008b_blue_emerald_action_0, FR13 holds the camera's x, 84.08F, five of
 * the six coordinates take other slots, and 22 of 3239 words differ */
extern NJS_ACTION ev008b_blue_emerald_action_0[];
/* @hack type: ev008b_blue_emerald_action_1 declared as an array, which script.c
 * passes by its name. SHC then also keeps the camera's z in FR13 and the
 * emeralds' coordinates in the image's frame slots. as one NJS_ACTION passed as
 * &ev008b_blue_emerald_action_1, FR13 holds the camera's x, five of the six
 * coordinates take other slots, and 22 of 3239 words differ */
extern NJS_ACTION ev008b_blue_emerald_action_1[];
extern NJS_ACTION ev008b_blue_emerald_action_2;
extern NJS_ACTION ev008b_blue_emerald_action_3;
/* @hack type: ev008b_green_emerald_action_0 declared as an array, which
 * script.c passes by its name. SHC then also keeps the camera's z in FR13 and
 * the emeralds' coordinates in the image's frame slots. as one NJS_ACTION
 * passed as &ev008b_green_emerald_action_0, FR13 holds the camera's x, five of
 * the six coordinates take other slots, and 22 of 3239 words differ */
extern NJS_ACTION ev008b_green_emerald_action_0[];
/* @hack type: ev008b_green_emerald_action_1 declared as an array, which
 * script.c passes by its name. SHC then also keeps the camera's z in FR13 and
 * the emeralds' coordinates in the image's frame slots. as one NJS_ACTION
 * passed as &ev008b_green_emerald_action_1, FR13 holds the camera's x, five of
 * the six coordinates take other slots, and 22 of 3239 words differ */
extern NJS_ACTION ev008b_green_emerald_action_1[];
extern NJS_ACTION ev008b_green_emerald_action_2;
extern NJS_ACTION ev008b_green_emerald_action_3;

static struct task *ev008b_blue_emerald = NULL;
static struct task *ev008b_green_emerald = NULL;
/* the dust under Sonic and under Knuckles as they clash, a task each */
static struct task *ev008b_sonic_dust = NULL;
static struct task *ev008b_knuckles_dust = NULL;
static struct task *ev008b_beam = NULL;
/* the flash as Chaos 2 becomes Chaos 4 */
static struct task *ev008b_flash = NULL;
/* the black fade that ends the event */
static struct task *ev008b_fade = NULL;

void ev008b_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        sound_load_file("e_0009.mlt");
        event_set_pos(event_get_player(0), 73.0F, 73.05F, 380.6F);
        event_set_ang(event_get_player(0), 0xff96, 0x8000, 18);
        event_create_player(2, sonic_step, 73.0F, 72.0F, 170.0F, 0xf924, 0, 0);
        event_create_player(3, tails_step, -104.07F, 75.46F, 349.37F, 0, 0x6d73,
                            0);
        event_eggmobile_create(85.0F, 40.0F, 80.0F, 0, 0x3b00, 0);
        ev008b_beam = event_beam_create(77.0F, 72.05F, 185.0F, 0, 0, 0);
        if (ev008b_sonic_dust == NULL) {
            ev008b_sonic_dust = event_dust_create();
        }
        if (ev008b_knuckles_dust == NULL) {
            ev008b_knuckles_dust = event_dust_create();
        }
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_sprint_action,
                          &knuckles_texlist, 2.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_sprint_action,
                          &sonic_texlist, 2.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev008b_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), ev008b_eggmobile_action_0,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_eggmobile_set_bob(0x100, 0.5F);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_beam_set_shape(ev008b_beam, 0.1F, 0.0F, 0.1F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev008b_beam, 0.0F, 5.0F, 0.0F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev008b_beam, 0.0F, 0.0F, 0.0F, 0.001F, 0.0F, 0);
        event_beam_set_shape(ev008b_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.001F, 0);
        event_beam_set_shape(ev008b_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_place_actor(&ev008b_blue_emerald,
                          event_get_player(2)->work->pos.x,
                          event_get_player(2)->work->pos.y,
                          event_get_player(2)->work->pos.z, 0, 0, 0);
        event_place_actor(&ev008b_green_emerald,
                          event_get_player(2)->work->pos.x,
                          event_get_player(2)->work->pos.y,
                          event_get_player(2)->work->pos.z, 0, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 66.33F, 76.64F, 394.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x189, 0xf56d, 0xf400);
        event_camera_follow(event_get_player(0));
        event_camera_set_fov(EVENT_EASE_LINEAR, 30, 0x238e);
        event_move(event_get_player(2), 73.0F, 72.0F, 170.0F, 73.0F, 72.2F,
                   230.0F, 30);
        event_move(event_get_player(0), 73.0F, 72.0F, 380.6F, 73.0F, 73.05F,
                   320.6F, 30);
        event_wait(15);
        event_set_flags(ev008b_green_emerald, EVENT_IGNORE_GROUND);
        event_set_flags(ev008b_blue_emerald, EVENT_IGNORE_GROUND);
        event_wait(15);
        event_set_pos(ev008b_blue_emerald, 81.0F, 72.05F, 180.0F);
        event_set_ang(ev008b_blue_emerald, 0, 0, 0);
        event_set_pos(ev008b_green_emerald, 73.0F, 72.05F, 190.0F);
        event_set_ang(ev008b_green_emerald, 0, 0x6000, 0);
        event_set_pos(ev008b_sonic_dust, 73.0F, 72.0F, 230.0F);
        /* @bug logic: places Sonic's dust a second time, at z 320, where
         * Knuckles' dust ends its move below. nothing places Knuckles'
         * dust before that move. both calls pass ev008b_sonic_dust
         * (MOV.L @(0x0cb80588),R3 at 0x0cb804a0 and 0x0cb804b2) */
        event_set_pos(ev008b_sonic_dust, 73.0F, 72.0F, 320.0F);
        event_move_stop(event_get_player(0));
        event_move_stop(event_get_player(2));
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(2));
        event_wait(1);
        event_camera_follow_clear();
        event_camera_set_fov(EVENT_EASE_LINEAR, 30, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 61.51F, 76.85F, 206.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x189, 0x8e6d, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, 62.63F, 78.22F, 247.54F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev008b_sonic_action_2,
                          &sonic_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(2), &ev008b_sonic_action_3,
                          &sonic_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(2), &ev008b_sonic_action_4,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(2), event_get_player(2)->work->pos.x,
                   event_get_player(2)->work->pos.y,
                   event_get_player(2)->work->pos.z,
                   event_get_player(2)->work->pos.x, 72.35F,
                   event_get_player(2)->work->pos.z + 48.0F, 15);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev008b_knuckles_action_0,
                          &knuckles_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev008b_knuckles_action_1,
                          &knuckles_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev008b_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(0), event_get_player(0)->work->pos.x,
                   event_get_player(0)->work->pos.y,
                   event_get_player(0)->work->pos.z,
                   event_get_player(0)->work->pos.x, 72.35F,
                   event_get_player(0)->work->pos.z - 48.0F, 15);
        event_wait(10);
        sound_play_at(1228, 0, 0, 120, event_get_player(0)->work->pos.x, 72.35F,
                      event_get_player(0)->work->pos.z + 48.0F);
        event_wait(5);
        event_set_shadow(ev008b_green_emerald, 0.3F);
        event_set_shadow(ev008b_blue_emerald, 0.3F);
        event_set_shadow(event_eggmobile_get_task(), 1.35F);
        vibration_start(0, 7, 50, 1);
        event_move_stop(event_get_player(0));
        event_move_stop(event_get_player(2));
        event_wait(5);
        event_camera_set_fov(EVENT_EASE_LINEAR, 30, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 61.81F, 77.94F, 208.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe89, 0x866d, 0xfd00);
        event_move(event_get_player(2), event_get_player(2)->work->pos.x,
                   event_get_player(2)->work->pos.y,
                   event_get_player(2)->work->pos.z, 73.0F, 72.0F, 230.0F, 30);
        event_move(event_get_player(0), event_get_player(0)->work->pos.x,
                   event_get_player(0)->work->pos.y,
                   event_get_player(0)->work->pos.z, 73.0F, 72.0F, 320.6F, 30);
        event_move(ev008b_sonic_dust, event_get_player(2)->work->pos.x,
                   event_get_player(2)->work->pos.y,
                   event_get_player(2)->work->pos.z, 73.0F, 72.0F, 230.0F, 35);
        event_move(ev008b_knuckles_dust, event_get_player(0)->work->pos.x,
                   event_get_player(0)->work->pos.y,
                   event_get_player(0)->work->pos.z, 73.0F, 72.0F, 320.0F, 35);
        event_voice_play(995);
        event_wait(10);
        event_set_pos(ev008b_blue_emerald, 73.0F, 75.0F, 250.0F);
        event_play_action(ev008b_blue_emerald, ev008b_blue_emerald_action_0,
                          &ev008b_blue_emerald_texlist, 1.0F, 0, 0);
        event_play_action(ev008b_blue_emerald, ev008b_blue_emerald_action_1,
                          &ev008b_blue_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev008b_blue_emerald, ev008b_blue_emerald->work->pos.x,
                   ev008b_blue_emerald->work->pos.y,
                   ev008b_blue_emerald->work->pos.z, 81.0F, 72.05F, 180.0F, 30);
        sound_play_at(1224, 0, 0, 120, 81.0F, 72.05F, 180.0F);
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then keeps 26.83F and 159.98F, the x and z of Chaos 2's place, in
         * FR13 and FR12 for event_chaos2_create and event_chaos2_set_pos
         * (0x0cb81b56, 0x0cb81b66). it also keeps the blue emerald's z and x in
         * the frame slots at 36 and 40 (0x0cb80f0e, 0x0cb80f2a). as an if, the
         * registers trade, the slots trade, and 15 of 3239 words differ */
        ev008b_sonic_dust != NULL &&
            (DUST_WORK(ev008b_sonic_dust)->timer =
                 DUST_WORK(ev008b_sonic_dust)->interval = 5);
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then also keeps Chaos 2's x and z in FR13 and FR12 and the blue
         * emerald's z and x in their frame slots. as an if, the registers
         * trade, the slots trade, and 15 of 3239 words differ */
        ev008b_knuckles_dust != NULL &&
            (DUST_WORK(ev008b_knuckles_dust)->timer =
                 DUST_WORK(ev008b_knuckles_dust)->interval = 5);
        event_wait(15);
        sound_play_at(1224, 0, 0, 120, 81.0F, 72.05F, 180.0F);
        event_set_pos(ev008b_green_emerald, 73.0F, 75.0F, 235.0F);
        event_play_action(ev008b_green_emerald, ev008b_green_emerald_action_0,
                          &ev008b_green_emerald_texlist, 1.0F, 0, 0);
        event_play_action(ev008b_green_emerald, ev008b_green_emerald_action_1,
                          &ev008b_green_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev008b_green_emerald, ev008b_green_emerald->work->pos.x,
                   ev008b_green_emerald->work->pos.y,
                   ev008b_green_emerald->work->pos.z, 73.0F, 72.05F, 190.0F,
                   30);
        event_wait(5);
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_LINEAR, 30, ev008b_blue_emerald,
                                  0.0F, 0.0F, 0.0F, 0);
        event_move_stop(event_get_player(0));
        event_move_stop(event_get_player(2));
        event_move_stop(ev008b_sonic_dust);
        event_voice_play(994);
        event_message_show(TEXT_1);
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then keeps 0 in R14 for EVENT_END's three clears (0x0cb8220a). as an
         * if, each clear loads a 0 of its own, script.c's code ends 4 bytes
         * past the image's, and 100 of 3239 words differ */
        ev008b_sonic_dust != NULL &&
            (DUST_WORK(ev008b_sonic_dust)->timer =
                 DUST_WORK(ev008b_sonic_dust)->interval = 0);
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then also keeps 0 in R14 for EVENT_END's three clears. as an if, each
         * clear loads a 0 of its own, script.c's code ends 4 bytes past the
         * image's, and 100 of 3239 words differ */
        ev008b_knuckles_dust != NULL &&
            (DUST_WORK(ev008b_knuckles_dust)->timer =
                 DUST_WORK(ev008b_knuckles_dust)->interval = 0);
        event_look_clear(event_get_player(2));
        event_look_at(event_get_player(2), 75.0F, 75.0F, 233.5F);
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_wait(5);
        EVENT_KILL(ev008b_sonic_dust);
        EVENT_KILL(ev008b_knuckles_dust);
        event_wait(5);
        event_move_stop(ev008b_blue_emerald);
        event_wait(15);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_16);
        event_move_stop(ev008b_green_emerald);
        event_wait(5);
        event_message_close();
        event_camera_look_clear();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x71c);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 64.68F, 80.58F, 89.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe5d, 0x84a3, 0);
        event_wait(15);
        stream_request_music(MUSIC_EGGMAN);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 61.47F, 89.64F, -86.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0xfe5d, 0x83a3, 0);
        event_wait(15);
        event_talk_stop(event_get_player(2));
        event_eggmobile_slide_to(85.0F, 80.0F, 80.0F, 50);
        sound_play_at_for(1227, EVENT_SOUND_ID, 1, 120, 85.0F, 80.0F, 80.0F,
                          70);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 55.77F, 92.21F, -150.59F);
        event_voice_play(1125);
        event_wait(30);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 84.08F, 111.19F, 85.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0xfd00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, 84.08F, 94.19F, 85.1F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 84.08F, 93.19F, 85.1F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 103.13F, 102.22F, 110.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xf100, 0x1800, 0);
        event_eggmobile_set_mode(EGGMOBILE_HOVER);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 106.24F, 104.38F, 115.47F);
        event_wait(15);
        event_eggmobile_slide_to(80.0F, 80.0F, 160.0F, 45);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        sound_play_at(1231, 0, 0, 120, 80.0F, 80.0F, 160.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 45, 103.13F, 102.22F, 110.82F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 45, 0xf500, 0x7000, 0);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 107.19F, 105.15F, 101.03F);
        event_voice_play(1902);
        event_message_show(TEXT_2);
        event_wait(15);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 83.63F, 73.7F, 120.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x200, 0x7c00, 0);
        sound_play_at_for(1226, EVENT_SOUND_ID + 1, 1, 120, 80.0F, 80.0F,
                          160.0F, 60);
        event_set_ang(event_eggmobile_get_task(), 0, 0, 18);
        event_wait(10);
        event_beam_set_mode(ev008b_beam, BEAM_OPEN);
        event_wait(10);
        event_move(ev008b_blue_emerald, ev008b_blue_emerald->work->pos.x,
                   ev008b_blue_emerald->work->pos.y,
                   ev008b_blue_emerald->work->pos.z,
                   ev008b_blue_emerald->work->pos.x,
                   ev008b_blue_emerald->work->pos.y + 10.0F,
                   ev008b_blue_emerald->work->pos.z, 30);
        event_move(ev008b_green_emerald, ev008b_green_emerald->work->pos.x,
                   ev008b_green_emerald->work->pos.y,
                   ev008b_green_emerald->work->pos.z,
                   ev008b_green_emerald->work->pos.x,
                   ev008b_green_emerald->work->pos.y + 10.0F,
                   ev008b_green_emerald->work->pos.z, 35);
        event_wait(20);
        event_voice_play(977);
        event_eggmobile_set_jets(EGGMOBILE_JETS_OFF);
        event_wait(10);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(5);
        event_move_stop(ev008b_blue_emerald);
        event_move_stop(ev008b_green_emerald);
        event_beam_set_mode(ev008b_beam, BEAM_CLOSE);
        event_wait(5);
        event_clear_motions(event_eggmobile_get_task());
        event_clear_motions(ev008b_green_emerald);
        /* @bug logic: clears the green emerald's motions a second time, and
         * neither call clears the blue emerald's. both pass
         * ev008b_green_emerald, which R9 addresses from 0x0cb80450
         * (MOV.L @R9,R4 at 0x0cb80fc4 and 0x0cb80fca). so
         * ev008b_blue_emerald_action_2 below queues behind the repeating
         * ev008b_blue_emerald_action_1, and starts at the end of its current
         * pass */
        event_clear_motions(ev008b_green_emerald);
        event_play_action(event_eggmobile_get_task(),
                          &ev008b_eggmobile_action_1, &event_eggmobile_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_play_action(ev008b_green_emerald, &ev008b_green_emerald_action_2,
                          &ev008b_green_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev008b_blue_emerald, &ev008b_blue_emerald_action_2,
                          &ev008b_blue_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 82.3F, 95.06F, 166.44F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf500, 0x1300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 89.67F, 99.59F, 181.07F);
        event_set_pos(ev008b_green_emerald,
                      event_eggmobile_get_task()->work->pos.x + 1.0F,
                      event_eggmobile_get_task()->work->pos.y - 9.2F,
                      event_eggmobile_get_task()->work->pos.z);
        /* @bug logic: sets the green emerald's angles, and below the blue
         * emerald's, from the Egg Mobile's position. each coordinate is
         * truncated to an Angle (FTRC at 0x0cb810f4 to 0x0cb81116 and
         * 0x0cb81162 to 0x0cb81184, before the JSRs at 0x0cb8111c and
         * 0x0cb8118a) */
        event_set_ang(ev008b_green_emerald,
                      event_eggmobile_get_task()->work->pos.x,
                      event_eggmobile_get_task()->work->pos.y,
                      event_eggmobile_get_task()->work->pos.z);
        event_set_pos(ev008b_blue_emerald,
                      event_eggmobile_get_task()->work->pos.x,
                      event_eggmobile_get_task()->work->pos.y - 9.2F,
                      event_eggmobile_get_task()->work->pos.z);
        event_set_ang(ev008b_blue_emerald,
                      event_eggmobile_get_task()->work->pos.x,
                      event_eggmobile_get_task()->work->pos.y,
                      event_eggmobile_get_task()->work->pos.z);
        event_set_pos(event_get_player(3), 37.3F, 72.02F, 213.32F);
        event_look_clear(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(2), event_get_player(2)->work->ang.x,
                      -event_get_player(2)->work->ang.y - 0x4000,
                      event_get_player(2)->work->ang.z);
        event_set_pos(event_get_player(0), 105.82F, 72.13F, 349.73F);
        event_set_ang(event_get_player(0), 0xff96, 0x9000, 18);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          &knuckles_texlist, 2.0F, EVENT_REPEAT, 0);
        event_voice_play(1003);
        event_message_show(TEXT_3);
        event_wait(70);
        event_message_close();
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x18e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 99.21F, 109.2F, 135.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xee00, 0x6d00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 108.56F, 119.03F, 117.38F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 59.42F, 76.16F, 233.15F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x200, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 59.42F, 76.16F, 220.73F);
        event_move(event_get_player(0), 107.76F, 72.52F, 258.72F, 102.5F,
                   72.17F, 220.0F, 30);
        event_wait(30);
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 100.98F, 72.87F, 209.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1600, 0x7900, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 103.93F, 72.87F, 209.71F);
        event_talk_start(event_get_player(0), TEXT_17);
        event_wait(5);
        event_voice_play(1004);
        event_message_show(TEXT_4);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 88.39F, 94.31F, 175.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x400, 0xe00, 0);
        event_look_at_task(event_get_player(0), event_eggmobile_get_task(),
                           0.0F, 12.0F, 0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 15, 86.72F, 94.8F, 170.57F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 85.31F, 104.18F, 182.86F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0xed00, 0xd00, 0);
        event_voice_play(1005);
        event_message_show(TEXT_5);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x18e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 53.66F, 111.64F, 139.66F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xed00, 0x9d00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 59.5F, 107.15F, 146.43F);
        event_voice_play(1006);
        event_message_show(TEXT_6);
        event_wait(90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 68.16F, 74.54F, 223.09F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1200, 0x9f00, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 65.04F, 72.4F, 219.82F);
        event_talk_start(event_get_player(2), TEXT_17);
        event_wait(5);
        event_voice_play(1007);
        event_message_show(TEXT_7);
        event_wait(20);
        event_talk_stop(event_get_player(2));
        event_wait(30);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_18);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_LINEAR, 45, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 42.23F, 74.5F, 210.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0x400, 0xb100, 0);
        event_wait(30);
        event_talk_stop(event_get_player(2));
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, 38.52F, 74.11F, 209.25F);
        event_look_clear(event_get_player(2));
        event_play_action(event_get_player(2), ev008b_sonic_action_0,
                          &sonic_texlist, 0.4F, 0, 4);
        event_play_action(event_get_player(2), sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_play(1008);
        event_message_show(TEXT_8);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_19);
        event_wait(60);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_wait(15);
        event_play_action(event_get_player(0), &ev008b_knuckles_action_3,
                          &knuckles_texlist, 1.0F, 0, 4);
        event_play_action(event_get_player(0), &ev008b_knuckles_action_4,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1009);
        event_message_show(TEXT_9);
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_voice_play(1010);
        event_message_show(TEXT_10);
        event_play_action(event_get_player(2), &ev008b_sonic_action_1,
                          &sonic_texlist, 0.25F, 0, 16);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_wait(4);
        event_play_action(event_get_player(2), sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0xfa5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 134.05F, 76.4F, 185.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x5e00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 143.69F, 76.4F, 177.07F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_21);
        event_wait(30);
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0xe39);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 79.86F, 97.41F, 177.16F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf500, 0, 0);
        event_wait(1);
        stream_stop_music();
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x3111);
        event_voice_play(1011);
        event_message_show(TEXT_11);
        event_wait(90);
        event_message_close();
        event_wait(10);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_look_clear(event_get_player(3));
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 93.47F, 106.04F, 258.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xef00, 0xf00, 0);
        event_wait(20);
        event_voice_play(1012);
        event_message_show(TEXT_12);
        stream_request_music(MUSIC_EVTBGM01);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 97.09F, 110.5F, 268.33F);
        event_wait(60);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 66.12F, 76.35F, 223.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x6f8, 0xaa00, 0);
        event_wait(10);
        event_chaos2_create(26.83F, 69.8F, 159.98F, 0, 0x5500, 0, 240);
        event_wait(1);
        event_chaos2_set_pos(26.83F, 69.8F, 159.98F);
        event_chaos2_set_ang(0, 0x5500, 0);
        event_chaos2_puddle();
        event_chaos2_set_speed(0.5F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 43.0F, 76.17F, 196.59F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x400, 0xaa00, 0);
        event_chaos2_rise();
        event_wait(20);
        sound_play_at_for(1229, EVENT_SOUND_ID + 2, 1, 120, 26.83F, 69.8F,
                          159.98F, 265);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 8.39F, 91.06F, 149.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0xf700, 0xaa00, 0);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -14.2F, 96.97F, 135.48F);
        event_wait(45);
        event_set_ang(event_eggmobile_get_task(), 0, 0x4000, 0);
        event_set_ang(ev008b_green_emerald, 0, 0xc000, 0);
        event_set_ang(ev008b_blue_emerald, 0, 0xc000, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x11c7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 76.12F, 74.18F, 293.91F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x300, 0xd00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 134.15F, 74.21F, 275.72F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0x300, 0x2200, 0);
        event_wait(130);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 38.23F, 110.21F, 180.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf000, 0xd200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 22.36F, 117.48F, 187.61F);
        event_clear_motions(event_eggmobile_get_task());
        event_clear_motions(ev008b_green_emerald);
        event_clear_motions(ev008b_blue_emerald);
        event_play_action(event_eggmobile_get_task(),
                          &ev008b_eggmobile_action_2, &event_eggmobile_texlist,
                          1.0F, 0, 0);
        event_play_action(event_eggmobile_get_task(), ev008b_eggmobile_action_0,
                          &event_eggmobile_texlist, 1.0F, 0, 8);
        event_play_action(ev008b_green_emerald, &ev008b_green_emerald_action_3,
                          &ev008b_green_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev008b_green_emerald, ev008b_green_emerald_action_0,
                          &ev008b_green_emerald_texlist, 0.3F, 0, 0);
        event_play_action(ev008b_green_emerald, ev008b_green_emerald_action_1,
                          &ev008b_green_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev008b_blue_emerald, &ev008b_blue_emerald_action_3,
                          &ev008b_blue_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev008b_blue_emerald, ev008b_blue_emerald_action_0,
                          &ev008b_blue_emerald_texlist, 0.3F, 0, 0);
        event_play_action(ev008b_blue_emerald, ev008b_blue_emerald_action_1,
                          &ev008b_blue_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_play(1013);
        event_message_wait(50, TEXT_13);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 62.97F, 82.42F, 245.27F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x100, 0xf00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 47.65F, 83.4F, 208.32F);
        event_set_pos(ev008b_blue_emerald, 30.0F, 100.0F, 163.0F);
        event_move(ev008b_blue_emerald, 30.0F, 100.0F, 163.0F, 30.0F, 86.0F,
                   163.0F, 60);
        event_set_pos(ev008b_green_emerald, 26.0F, 95.0F, 162.0F);
        event_move(ev008b_green_emerald, 26.0F, 95.0F, 162.0F, 26.0F, 85.5F,
                   162.0F, 60);
        event_set_ang(event_get_player(0), 0, 0x9400, 0);
        event_set_ang(event_get_player(3), 0, 0x8200, 0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 73.16F, 81.24F, 252.15F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x1300, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 94.74F, 81.24F, 297.7F);
        event_wait(10);
        event_rays_create(26.83F, 85.5F, 159.98F, 3, 5, 0.2F, 4.0F, 0xb4ffff91,
                          0xffff32);
        event_wait(30);
        ev008b_flash = event_fade_create(1.0F / 90.0F, 0.2F, 1.0F, 1.0F, 0.88F);
        event_wait(70);
        sound_play_at(1230, 0, 0, 120, 26.83F, 60.0F, 159.98F);
        event_chaos2_kill();
        event_kill_task(&ev008b_blue_emerald);
        event_kill_task(&ev008b_green_emerald);
        event_chaos4_create(26.83F, 60.0F, 159.98F, 0, 0x5500, 0, 120);
        event_wait(1);
        event_chaos4_set_speed(0.5F);
        event_wait(10);
        event_rays_kill();
        if (ev008b_flash != NULL) {
            event_fade_set_speed(ev008b_flash, -1.0F / 60.0F);
        }
        event_wait(60);
        EVENT_KILL(ev008b_flash);
        event_wait(10);
        event_voice_play(1014);
        event_message_wait(50, TEXT_14);
        event_voice_play(1228);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 101.03F, 81.24F, 310.21F);
        event_message_show(TEXT_15);
        event_wait(90);
        event_message_close();
        ev008b_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_move_stop_all();
        EVENT_KILL(ev008b_flash);
        EVENT_KILL(ev008b_fade);
        event_rays_kill();
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_chaos2_kill();
        event_chaos4_kill();
        event_kill_player(2);
        event_kill_player(3);
        event_kill_task(&ev008b_blue_emerald);
        event_kill_task(&ev008b_green_emerald);
        event_eggmobile_kill();
        /* @hack spelling: the kill and the clear as one statement. SHC then
         * sets R14 to 0 ahead of event_beam_kill (0x0cb8220a), for this clear
         * and the two EVENT_KILLs after it. as two statements, SHC sets R14
         * after the call, and 18 of 3239 words differ */
        event_beam_kill(ev008b_beam), ev008b_beam = NULL;
        EVENT_KILL(ev008b_sonic_dust);
        EVENT_KILL(ev008b_knuckles_dust);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev008b_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"CAPTUREBEAM", &event_beam_texlist},
    {"M_EM_BLUE", &ev008b_blue_emerald_texlist},
    {"M_EM_GREEN", &ev008b_green_emerald_texlist},
    {"SONIC", &sonic_texlist},
    {"MILES", &tails_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {"CHAOS2", &event_chaos2_texlist},
    {"CHAOS4_COMMON", &event_chaos4_texlist},
    {NULL, NULL},
};
