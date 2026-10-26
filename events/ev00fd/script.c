/* EV00FD: Chaos brings down the Egg Carrier 2, and Tikal comes to Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00fd/egg_carrier_textures.h"
#include "ev00fd/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/chaos7.h"
#include "shared/dome.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/highway.h"
#include "shared/light.h"
#include "shared/move.h"
#include "shared/orbit.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 58
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 29
#define TEXT_4_LENGTH 62
#define TEXT_5_LENGTH 40
#define TEXT_6_LENGTH 24
#define TEXT_7_LENGTH 17
#define TEXT_8_LENGTH 31
#define TEXT_9_LENGTH 25
#define TEXT_10_LENGTH 42
#define TEXT_11_LENGTH 13
#define TEXT_12_LENGTH 59
#define TEXT_13_LENGTH 35
#define TEXT_14_LENGTH 62
#define TEXT_15_LENGTH 44
#define TEXT_16_LENGTH 50
#define TEXT_17_LENGTH 17
#define TEXT_18_LENGTH 34
#define TEXT_19_LENGTH 45
#define TEXT_20_LENGTH 26
#define TEXT_21_LENGTH 19
#define TEXT_22_LENGTH 3
#define TEXT_23_LENGTH 1
#define TEXT_24_LENGTH 4
#define TEXT_25_LENGTH 2
#define TEXT_26_LENGTH 5
#define TEXT_27_LENGTH 6
#define TEXT_28_LENGTH 5
#define TEXT_29_LENGTH 4
#define TEXT_30_LENGTH 3
#define TEXT_31_LENGTH 6
#define TEXT_32_LENGTH 5
#define TEXT_33_LENGTH 3
#include "ev00fd/script/text.inc"

/* the Egg Carrier 2's prop slot */
#define EGG_CARRIER 0

/* sprite slots: four bombs and two bolts of lightning on the Egg Carrier 2,
 * and a bolt on the green emerald */
#define BOMB_SLOT_0 10
#define BOMB_SLOT_1 11
#define BOMB_SLOT_2 12
#define BOMB_SLOT_3 13
#define LIGHTNING_SLOT_0 14
#define LIGHTNING_SLOT_1 15
#define EMERALD_LIGHTNING_SLOT 16

extern NJS_ACTION sonic_stand_action;

/* an action of Tikal's, in TIKAL_PROG.PRS */
extern NJS_ACTION dat_0cb2d478;

/* @hack spelling: these are the EVENT_SPRITE_BOMB, EVENT_SPRITE_LIGHTNING and
 * EVENT_SPRITE_JET elements of event_sprite_texlists, each declared under a
 * name of its own at the element's address (0x0cbbeb60, 0x0cbbeb98,
 * 0x0cbbebe8). with the names, SHC loads each address from a pool word at
 * each call, as at 0x0cb80e2c, 0x0cb80f6e and 0x0cb81a78. without them,
 * ev00fd_script runs 28 bytes long and 3024 of 3247 words differ */
extern NJS_TEXLIST event_sprite_bomb_texlist;
extern NJS_TEXLIST event_sprite_lightning_texlist;
extern NJS_TEXLIST event_sprite_jet_texlist;

extern NJS_TEXLIST event_eggmobile_texlist;

extern NJS_ACTION ev00fd_sonic_action_0;
extern NJS_ACTION ev00fd_sonic_action_1;
extern NJS_ACTION ev00fd_sonic_action_2;
extern NJS_ACTION ev00fd_sonic_action_3;
extern NJS_ACTION ev00fd_sonic_action_4;
extern NJS_ACTION ev00fd_sonic_action_5;
extern NJS_ACTION ev00fd_tikal_action_0;
extern NJS_ACTION ev00fd_tikal_action_1;
extern NJS_ACTION ev00fd_eggman_action_0;
extern NJS_ACTION ev00fd_eggman_action_1;
extern NJS_ACTION ev00fd_emerald_action_1;
extern NJS_ACTION ev00fd_emerald_action_2;

/* the path Tikal's light flies down to Sonic */
static struct event_path_point ev00fd_light_points[15] = {
#include "ev00fd/script/ev00fd_light_points.inc"
};

static struct event_path ev00fd_light_path = {15, ev00fd_light_points};

/* the seven emeralds, each named for the color event_orbit_add_sprite gives
 * its sprite */
static struct task *ev00fd_emerald_red = NULL;
static struct task *ev00fd_emerald_green = NULL;
static struct task *ev00fd_emerald_white = NULL;
static struct task *ev00fd_emerald_yellow = NULL;
static struct task *ev00fd_emerald_purple = NULL;
static struct task *ev00fd_emerald_cyan = NULL;
static struct task *ev00fd_emerald_blue = NULL;

static struct task *ev00fd_sonic_floor = NULL;
static struct task *ev00fd_tikal_floor = NULL;
static struct task *ev00fd_light = NULL;
static struct task *ev00fd_blast = NULL;
/* the pale yellow fade that hides Tikal's arrival */
static struct task *ev00fd_fade = NULL;

void ev00fd_script(int phase)
{
    /* @hack spelling: the emeralds' action through a local, which loads its
     * address into R12 once for the seven calls (0x0cb80666); without it, the
     * function runs 4 bytes short and 2538 of 3247 words differ */
    NJS_ACTION *emerald_action;
    /* @hack spelling: Sonic's z through a local, which the image keeps in FR14
     * from before the switch (0x0cb80022); without it, FR14 holds his x,
     * 806.63F, and 17 of 3247 words differ */
    Float sonic_z = -251.4F;
    /* @hack spelling: the Egg Mobile's bob speed through a local, which leaves
     * the move of the bob's height, FMOV FR13,FR4, in the delay slot of
     * event_eggmobile_set_bob's call (0x0cb80652); without it, SHC moves that
     * into the delay slot of the BRA before the call (0x0cb80602) and 2 of
     * 3247 words differ */
    Angle bob_speed;
    /* @hack spelling: event_talk_stop called through a local. SHC then keeps
     * its address in R10 and event_play_action's in R9 (0x0cb8076e,
     * 0x0cb8029a). called by name, the two trade, and 50 of 3247 words
     * differ */
    void (*talk_stop)(struct task *task);
    /* @hack spelling: event_voice_play called through a local. SHC then keeps
     * its address in R8 (0x0cb80786). called by name, the function runs 8 bytes
     * long, and 2327 of 3247 words differ */
    void (*voice_play)(int voice);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_wait(1);
        event_reset_player(0);
        event_highway_create();
        sound_load_file("e_00fd.mlt");
        event_sound_init(5);
        event_set_pos(event_get_player(0), 806.63F, 150.0F, sonic_z);
        event_set_ang(event_get_player(0), 0, 0xd265, 0);
        event_chaos7_hold(150.0F, 0.0F, 50.0F, 0, 0, 0, 910);
        event_eggmobile_create(86.85F, 200.0F, -725.0F, 0, 0x4000, 0);
        event_prop_create(3500.0F, 1500.0F, 100.0F, 0, 0x4000, 0x0700, 0.15F,
                          0.15F, 0.15F, &ev00fd_egg_carrier_object,
                          &ev00fd_egg_carrier_texlist, EGG_CARRIER);
        event_prop_set_action(EGG_CARRIER, &ev00fd_egg_carrier_action, 0);
        event_place_actor(&ev00fd_emerald_red, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00fd_emerald_green, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00fd_emerald_white, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00fd_emerald_yellow, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00fd_emerald_purple, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00fd_emerald_cyan, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        event_place_actor(&ev00fd_emerald_blue, 0.0F, 0.0F, 0.0F, 0, 0, 0);
        ev00fd_blast =
            event_dome_create(0.0F, 100.0F, 2000.0F, 200, 7.0F, 1800.0F);
        event_wait(1);
        ev00fd_light =
            event_light_create(event_get_player(0)->work->pos.x + 4.0F,
                               event_get_player(0)->work->pos.y + 9.0F,
                               event_get_player(0)->work->pos.z - 200.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00fd_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        ev00fd_sonic_floor = event_floor_create(event_get_player(0), 150.0F);
        event_set_pos(event_get_player(0), 806.63F, 150.0F, sonic_z);
        event_set_ang(event_get_player(0), 0, 0xd265, 0);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), &ev00fd_eggman_action_0,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(ev00fd_emerald_red, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fd_emerald_green, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fd_emerald_white, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fd_emerald_yellow, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fd_emerald_purple, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fd_emerald_cyan, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fd_emerald_blue, EVENT_IGNORE_GROUND);
        event_set_pos(ev00fd_emerald_red,
                      event_get_player(0)->work->pos.x + 8.0F,
                      event_get_player(0)->work->pos.y - 6.0F,
                      event_get_player(0)->work->pos.z - 8.0F);
        event_set_ang(ev00fd_emerald_red, 0, 0, 0);
        event_set_pos(ev00fd_emerald_green,
                      event_get_player(0)->work->pos.x + 8.0F,
                      event_get_player(0)->work->pos.y - 6.0F,
                      event_get_player(0)->work->pos.z + 8.0F);
        event_set_ang(ev00fd_emerald_green, 0, 0, 0);
        event_set_pos(ev00fd_emerald_white,
                      event_get_player(0)->work->pos.x + 8.0F,
                      event_get_player(0)->work->pos.y - 6.0F,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev00fd_emerald_white, 0, 0, 0);
        event_set_pos(ev00fd_emerald_yellow,
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y - 6.0F,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev00fd_emerald_yellow, 0, 0, 0);
        event_set_pos(ev00fd_emerald_purple, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 6.0F,
                      event_get_player(0)->work->pos.z + 8.0F);
        event_set_ang(ev00fd_emerald_purple, 0, 0, 0);
        event_set_pos(ev00fd_emerald_cyan, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 6.0F,
                      event_get_player(0)->work->pos.z - 8.0F);
        event_set_ang(ev00fd_emerald_cyan, 0, 0, 0);
        event_set_pos(ev00fd_emerald_blue,
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y - 6.0F,
                      event_get_player(0)->work->pos.z - 8.0F);
        event_set_ang(ev00fd_emerald_blue, 0, 0, 0);
        event_dome_set_growth(ev00fd_blast, 0.99F, 4.0F, 1800.0F);
        event_set_flags(ev00fd_light, EVENT_IGNORE_GROUND);
        event_wait(1);
        bob_speed = 0x0100;
        event_eggmobile_set_bob(bob_speed, 0.5F);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_prop_play(EGG_CARRIER, 0, 1.0F, EVENT_PROP_FOREVER);
        emerald_action = &ev00fd_emerald_action_0;
        event_play_action(ev00fd_emerald_red, emerald_action,
                          &ev00fd_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00fd_emerald_green, emerald_action,
                          &ev00fd_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00fd_emerald_white, emerald_action,
                          &ev00fd_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00fd_emerald_yellow, emerald_action,
                          &ev00fd_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00fd_emerald_purple, emerald_action,
                          &ev00fd_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00fd_emerald_cyan, emerald_action,
                          &ev00fd_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00fd_emerald_blue, emerald_action,
                          &ev00fd_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        stream_request_music(MUSIC_TICAL);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x38e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 816.39F, 155.76F, -271.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x2331, 0x6185, 0);
        event_wait(45);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0931, 0x6185, 0);
        event_wait(145);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 803.2F, 152.47F, -244.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1531, 0xdc85, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, 801.26F, 152.47F,
                             -247.04F);
        talk_stop = event_talk_stop;
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_22);
        voice_play = event_voice_play;
        voice_play(1984);
        event_message_show(TEXT_1);
        event_wait(30);
        talk_stop(event_get_player(0));
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_22);
        event_wait(15);
        talk_stop(event_get_player(0));
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_22);
        event_wait(75);
        talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 803.38F, 154.6F, -241.04F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0631, 0xf300, 0);
        event_wait(15);
        event_play_action(event_get_player(0), &ev00fd_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_move(event_prop_get(EGG_CARRIER), 3500.0F, 1500.0F, 100.0F,
                   2000.0F, 1000.0F, 100.0F, 450);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 803.38F, 154.6F, -241.04F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0x0f31, 0xbd00, 0);
        event_wait(30);
        voice_play(1985);
        event_message_show(TEXT_2);
        event_wait(30);
        stream_stop_music();
        event_wait(1);
        stream_request_music(MUSIC_EGCARER1);
        event_wait(15);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1102.88F, 499.72F, -152.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1631, 0xb100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 1273.65F, 610.59F,
                             -86.71F);
        event_sound_start(1, 1224, 1800);
        event_sound_fade_volume(1, 100, 60);
        event_wait(40);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00fd_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        /* the image passes 0x13265 as the y angle (0x0cb80a1c). a whole turn
         * past 0x3265, it gives the same heading */
        event_set_ang(event_get_player(0), 0, 0x13265, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 814.67F, 151.44F, -248.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1931, 0x2300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 812.06F, 151.44F, -245.71F);
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_22);
        voice_play(1986);
        event_message_wait(60, TEXT_3);
        talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x18e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 87.17F, 213.74F, -720.29F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfb31, 0x0200, 0);
        voice_play(2032);
        event_message_wait(90, TEXT_4);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1795.42F, 1494.81F,
                             -217.61F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe231, 0x4b00, 0xf600);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 1707.89F, 1425.44F,
                             -767.1F);
        voice_play(2033);
        event_message_show(TEXT_5);
        event_wait(50);
        event_chaos7_ray(5000.0F);
        event_wait(50);
        event_move_stop(event_prop_get(EGG_CARRIER));
        event_wait(1);
        event_move(event_prop_get(EGG_CARRIER), 3500.0F, 1500.0F, 100.0F,
                   2000.0F, 1000.0F, 100.0F, 400);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x4000);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0831, 0xac85, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.27F, 504.47F, -367.99F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1000, 156.85F, 544.2F,
                             -278.27F);
        voice_play(2034);
        event_message_show(TEXT_6);
        event_wait(55);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_message_close();
        event_sound_start(2, 1226, 90);
        event_sound_fade_volume(2, 120, 40);
        event_sound_fade_pitch(2, 8000, 40);
        event_sound_play(14, 120, 0, 0);
        event_wait(15);
        event_sound_stop(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 935.97F, 599.81F, 31.63F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe31, 0x4085, 0);
        event_wait(10);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_sound_play(14, 120, 0, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 1481.66F, 768.92F, 24.67F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xf631, 0x4085, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 80, 0x5555);
        event_sound_start(3, 1227, 90);
        event_sound_fade_volume(3, 120, 40);
        event_sound_fade_pitch(3, 8000, 40);
        event_wait(70);
        event_sound_stop(2);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x4000);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1193.63F, 674.3F, -93.49F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1631, 0xac85, 0);
        event_wait(30);
        event_sprite_create(event_prop_get(EGG_CARRIER), BOMB_SLOT_0, 100.0F,
                            -30.0F, 500.0F, &event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_0, 10.0F, 10.0F);
        event_sound_play(1228, 120, 0, 0);
        event_wait(5);
        event_sprite_create(event_prop_get(EGG_CARRIER), BOMB_SLOT_1, -100.0F,
                            10.0F, 100.0F, &event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_1, 8.0F, 8.0F);
        event_sound_play(1228, 120, 0, 0);
        event_wait(5);
        event_sprite_create(event_prop_get(EGG_CARRIER), BOMB_SLOT_2, 100.0F,
                            -200.0F, -250.0F, &event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_2, 8.0F, 8.0F);
        event_sound_play(1228, 120, 0, 0);
        event_wait(5);
        event_sprite_create(event_prop_get(EGG_CARRIER), BOMB_SLOT_3, -250.0F,
                            -250.0F, -300.0F, &event_sprite_bomb_texlist);
        event_sprite_set_scale(BOMB_SLOT_3, 8.0F, 8.0F);
        event_sprite_create(event_prop_get(EGG_CARRIER), LIGHTNING_SLOT_0,
                            100.0F, -50.0F, 0.0F,
                            &event_sprite_lightning_texlist);
        event_sprite_set_scale(LIGHTNING_SLOT_0, 12.0F, 12.0F);
        event_sprite_create(event_prop_get(EGG_CARRIER), LIGHTNING_SLOT_1,
                            -100.0F, -100.0F, 500.0F,
                            &event_sprite_lightning_texlist);
        event_sprite_set_scale(LIGHTNING_SLOT_1, 6.0F, 6.0F);
        event_sound_play(1228, 120, 0, 0);
        event_wait(20);
        event_sound_play(1228, 120, 0, 0);
        event_wait(20);
        event_sound_play(1228, 120, 0, 0);
        event_wait(20);
        event_move_stop(event_prop_get(EGG_CARRIER));
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 835.45F, 412.31F, -407.31F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfedd, 0x72f1, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 360, 1189.7F, 424.98F,
                             -314.87F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 360, 0xf9b9, 0x6fc1, 0);
        event_sound_start(0, 1229, 200);
        event_sound_fade_volume(0, 120, 45);
        event_move_turn(event_prop_get(EGG_CARRIER), 2000.0F, 1000.0F, 1000.0F,
                        -200.0F, -100.0F, 2000.0F, 0, 0x4000, 0x0700, 0x3000,
                        0x4000, 0x1700, 300);
        event_wait(150);
        event_dome_set_mode(ev00fd_blast, EVENT_DOME_GROW);
        event_wait(30);
        event_sound_play(1230, 120, 0, 0);
        event_wait(150);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), &ev00fd_eggman_action_1,
                          &event_eggmobile_texlist, 1.0F, 0, 0);
        event_play_action(event_eggmobile_get_task(), &ev00fd_eggman_action_0,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 65.08F, 237.6F, -654.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1e31, 0x2600, 0x0d00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 111.43F, 252.18F,
                             -694.93F);
        event_move_stop(event_prop_get(EGG_CARRIER));
        voice_play(2035);
        event_wait(20);
        event_message_show(TEXT_7);
        event_wait(60);
        event_message_close();
        event_set_ang(event_get_player(0), 0, 0xd265, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 797.76F, 155.42F, -248.48F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0431, 0xb500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 799.09F, 155.42F, -253.3F);
        event_talk_start(event_get_player(0), TEXT_23);
        voice_play(1987);
        event_message_show(TEXT_8);
        event_wait(5);
        talk_stop(event_get_player(0));
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_24);
        event_wait(45);
        talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        event_eggmobile_kill();
        stream_stop_music();
        event_wait(1);
        stream_request_music(MUSIC_TICAL);
        event_play_path(ev00fd_light, &ev00fd_light_path, 0.25F, 0);
        event_wait(1);
        event_sound_play(1231, 120, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 804.33F, 155.76F, -268.48F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x2431, 0x9885, 0);
        event_set_ang(event_get_player(0), 0, 0x2000, 0);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 15, ev00fd_light, 0.0F,
                                  0.0F, 0.0F, 0);
        event_wait(80);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 802.68F, 151.44F, -269.29F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 350, 831.47F, 151.44F,
                             -252.32F);
        event_clear_paths(ev00fd_light);
        event_set_pos(ev00fd_light, event_get_player(0)->work->pos.x + 4.0F,
                      event_get_player(0)->work->pos.y + 9.0F,
                      event_get_player(0)->work->pos.z + 4.0F);
        event_play_action(event_get_player(0), &ev00fd_sonic_action_3,
                          &sonic_texlist, 0.5F, 0, 32);
        event_play_action(event_get_player(0), &ev00fd_sonic_action_4,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_sound_start(4, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(4, 80, 160);
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_25);
        event_wait(5);
        voice_play(1988);
        event_message_show(TEXT_9);
        event_wait(15);
        talk_stop(event_get_player(0));
        event_wait(35);
        event_talk_start(event_get_player(0), TEXT_25);
        event_wait(45);
        talk_stop(event_get_player(0));
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_26);
        event_wait(5);
        voice_play(1989);
        event_message_show(TEXT_10);
        event_wait(10);
        talk_stop(event_get_player(0));
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_26);
        event_wait(70);
        talk_stop(event_get_player(0));
        event_message_close();
        event_wait(25);
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_26);
        event_wait(5);
        voice_play(1990);
        event_message_show(TEXT_11);
        event_wait(10);
        talk_stop(event_get_player(0));
        event_wait(15);
        event_message_close();
        event_camera_look_clear();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 798.81F, 156.04F, -270.16F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff31, 0x9985, 0);
        ev00fd_fade = event_fade_create(3.0F / 255.0F, 0.2F, 1.0F, 1.0F, 0.88F);
        event_wait(40);
        event_sound_stop(4);
        event_wait(40);
        event_create_player(2, tikal_player_step, 835.0F, 149.0F, -235.0F, 0,
                            0xa900, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 835.0F, 149.0F, -235.0F);
        event_set_ang(event_get_player(2), 0, 0xa900, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &dat_0cb2d478, &tikal_texlist,
                          0.5F, EVENT_REPEAT, 0);
        ev00fd_tikal_floor = event_floor_create(event_get_player(2), 150.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_wait(5);
        EVENT_KILL(ev00fd_light);
        if (ev00fd_fade != NULL) {
            event_fade_set_speed(ev00fd_fade, -1.0F / 60.0F);
        }
        event_wait(60);
        EVENT_KILL(ev00fd_fade);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 825.09F, 156.41F, -249.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff31, 0x9685, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 828.76F, 156.27F,
                             -243.61F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00fd_tikal_action_0,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 32);
        event_talk_start(event_get_player(2), TEXT_27);
        event_wait(5);
        voice_play(2036);
        event_message_show(TEXT_12);
        event_wait(130);
        talk_stop(event_get_player(2));
        event_voice_stop();
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 851.45F, 155.47F, -219.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff31, 0x2685, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 844.19F, 155.47F,
                             -210.85F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xff31, 0x1885, 0);
        event_talk_start(event_get_player(2), TEXT_27);
        event_wait(5);
        voice_play(2037);
        event_message_show(TEXT_13);
        event_wait(80);
        talk_stop(event_get_player(2));
        event_wait(15);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x38e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 552.12F, 537.8F, -371.31F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0731, 0x7b85, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 752.9F, 537.8F, -349.14F);
        event_talk_start(event_get_player(2), TEXT_28);
        event_wait(5);
        voice_play(2038);
        event_message_show(TEXT_14);
        event_wait(120);
        talk_stop(event_get_player(2));
        event_message_close();
        event_set_ang(event_get_player(2), 0, 0xd000, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00fd_tikal_action_1,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x471c);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 893.67F, 178.0F, -298.35F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1a31, 0x5d85, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 881.06F, 161.38F,
                             -292.53F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0x1431, 0x5d85, 0);
        event_orbit_create(
            671.0F, 750.0F, -73.22F, 200.0F, 182, ev00fd_emerald_red,
            ev00fd_emerald_green, ev00fd_emerald_white, ev00fd_emerald_yellow,
            ev00fd_emerald_purple, ev00fd_emerald_cyan, ev00fd_emerald_blue);
        event_orbit_add_sprite(1, &event_sprite_jet_texlist, 1.5F, 1.0F, 1.0F,
                               0.0F, 0.0F);
        event_orbit_add_sprite(2, &event_sprite_jet_texlist, 1.5F, 1.0F, 0.0F,
                               1.0F, 0.0F);
        event_orbit_add_sprite(3, &event_sprite_jet_texlist, 1.5F, 1.0F, 1.0F,
                               1.0F, 1.0F);
        event_orbit_add_sprite(4, &event_sprite_jet_texlist, 1.5F, 1.0F, 1.0F,
                               1.0F, 0.0F);
        event_orbit_add_sprite(5, &event_sprite_jet_texlist, 1.5F, 1.0F, 1.0F,
                               0.0F, 1.0F);
        event_orbit_add_sprite(6, &event_sprite_jet_texlist, 1.5F, 1.0F, 0.0F,
                               1.0F, 1.0F);
        event_orbit_add_sprite(7, &event_sprite_jet_texlist, 1.5F, 1.0F, 0.0F,
                               0.0F, 1.0F);
        event_chaos7_mode_13();
        event_wait(180);
        event_orbit_scatter(10.0F);
        event_sound_play(1232, 120, 0, 0);
        event_wait(120);
        event_orbit_kill();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 804.29F, 158.71F, -268.21F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xec36, 0x1822, 0);
        event_wait(30);
        event_sprite_create(ev00fd_emerald_green, EMERALD_LIGHTNING_SLOT, 0.0F,
                            0.0F, 0.0F, &event_sprite_lightning_texlist);
        event_sprite_set_scale(EMERALD_LIGHTNING_SLOT, 0.13F, 0.13F);
        /* @hack redundant: + 0.0F, as the image adds FR15's 0.0F to y (FADD
         * FR15,FR5 at 0x0cb81c16). without it, the code runs 4 bytes short,
         * and 2843 of ev00fd_script's 3247 words differ */
        event_set_pos(ev00fd_emerald_green,
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y + 0.0F,
                      event_get_player(0)->work->pos.z - 30.0F);
        event_set_ang(ev00fd_emerald_green, 0, 0x7000, 0);
        event_clear_motions(ev00fd_emerald_green);
        event_play_action(ev00fd_emerald_green, &ev00fd_emerald_action_1,
                          &ev00fd_emerald_texlist, 1.0F, 0, 0);
        event_wait(10);
        event_sound_play(1234, 80, 0, 0);
        event_wait_motions(ev00fd_emerald_green);
        event_sprite_kill(EMERALD_LIGHTNING_SLOT);
        event_play_action(ev00fd_emerald_green, &ev00fd_emerald_action_2,
                          &ev00fd_emerald_black_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(45);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x471c);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 836.51F, 153.31F, -251.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1c31, 0x6885, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 835.11F, 150.23F, -249.62F);
        event_set_pos(ev00fd_emerald_green,
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y - 10.0F,
                      event_get_player(0)->work->pos.z - 30.0F);
        event_set_ang(ev00fd_emerald_green, 0, 0x7000, 0);
        event_talk_start(event_get_player(2), TEXT_28);
        event_wait(5);
        voice_play(2039);
        event_message_show(TEXT_15);
        event_wait(50);
        talk_stop(event_get_player(2));
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 831.27F, 156.14F, -219.04F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0031, 0xf785, 0);
        event_talk_start(event_get_player(2), TEXT_29);
        event_wait(5);
        voice_play(2040);
        event_message_show(TEXT_16);
        event_wait(70);
        talk_stop(event_get_player(2));
        event_message_close();
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 846.53F, 157.09F, -198.06F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 20, 0xff31, 0x1385, 0);
        event_talk_start(event_get_player(0), TEXT_30);
        event_wait(5);
        voice_play(1994);
        event_message_show(TEXT_17);
        event_wait(10);
        talk_stop(event_get_player(0));
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 826.94F, 154.14F, -245.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0431, 0x3986, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, 821.05F, 154.75F,
                             -246.86F);
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00fd_sonic_action_0,
                          &sonic_texlist, 0.5F, 0, 4);
        event_play_action(event_get_player(0), &ev00fd_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_talk_start(event_get_player(0), TEXT_31);
        event_wait(5);
        voice_play(1997);
        event_message_show(TEXT_18);
        event_wait(50);
        talk_stop(event_get_player(0));
        event_wait(25);
        event_talk_start(event_get_player(0), TEXT_32);
        event_wait(5);
        voice_play(1999);
        event_message_show(TEXT_19);
        event_wait(100);
        talk_stop(event_get_player(0));
        event_wait(25);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 821.06F, 158.47F, -238.37F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfc31, 0x2486, 0);
        event_talk_start(event_get_player(0), TEXT_31);
        event_wait(5);
        voice_play(2001);
        event_message_show(TEXT_20);
        event_set_ang(event_get_player(2), 0, 0xa900, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00fd_tikal_action_0,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 10, 0x11c7);
        event_wait(25);
        talk_stop(event_get_player(0));
        event_message_close();
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 789.37F, 156.55F, -242.61F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe31, 0xbb86, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 790.14F, 156.55F,
                             -249.57F);
        event_talk_start(event_get_player(2), TEXT_33);
        event_wait(5);
        voice_play(2041);
        event_message_show(TEXT_21);
        event_wait(30);
        talk_stop(event_get_player(2));
        event_message_close();
        event_wait(20);
        event_wait(50);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_sound_stop_all();
        EVENT_KILL(ev00fd_sonic_floor);
        EVENT_KILL(ev00fd_tikal_floor);
        event_sprite_kill(BOMB_SLOT_0);
        event_sprite_kill(BOMB_SLOT_1);
        event_sprite_kill(BOMB_SLOT_2);
        event_sprite_kill(BOMB_SLOT_3);
        event_sprite_kill(LIGHTNING_SLOT_0);
        event_sprite_kill(LIGHTNING_SLOT_1);
        event_sprite_kill(EMERALD_LIGHTNING_SLOT);
        EVENT_KILL(ev00fd_blast);
        event_orbit_kill();
        event_kill_task(&ev00fd_emerald_red);
        event_kill_task(&ev00fd_emerald_green);
        event_kill_task(&ev00fd_emerald_white);
        event_kill_task(&ev00fd_emerald_yellow);
        event_kill_task(&ev00fd_emerald_purple);
        event_kill_task(&ev00fd_emerald_cyan);
        event_kill_task(&ev00fd_emerald_blue);
        event_reset_player(0);
        event_kill_player(2);
        event_prop_kill(EGG_CARRIER);
        event_eggmobile_kill();
        event_set_pos(event_get_player(0), 806.63F, 150.0F, sonic_z);
        event_set_ang(event_get_player(0), 0, 0xd265, 0);
        event_chaos7_release(FALSE);
        event_highway_kill();
        EVENT_KILL(ev00fd_light);
        EVENT_KILL(ev00fd_fade);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00fd_textures[] = {
    {"TIKAL", &tikal_texlist},
    {"LAST1A_HIGHWAY_A", &event_highway_texlist},
    {"EC_LIGHT", &ev00fd_egg_carrier_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"M_EM_BLUE", &ev00fd_emerald_texlist},
    {"M_EM_BLACK", &ev00fd_emerald_black_texlist},
    {NULL, NULL},
};
