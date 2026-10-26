/* EV000D, Sonic's story: the runway at Tails' workshop moves out. Sonic and
 * Tails take off in the Tornado */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/runway.h"
#include "ev000d/script.h"
#include "ev000d/sonic_action_2.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 21
#define TEXT_4_LENGTH 2
#define TEXT_5_LENGTH 12
#include "ev000d/script/text.inc"

extern NJS_ACTION sonic_stand_action;

extern struct event_path ev000d_tornado_path;
/* @hack type: ev000d_tornado_object declared as an array, and read as
 * ev000d_tornado_object->child. SHC then loads the object's address and
 * reads the child pointer through it, at offset 44 (0x0cb8000c,
 * 0x0cb80024). as one NJS_OBJECT read as ev000d_tornado_object.child, SHC
 * loads the address plus 44 from a pool word, ev000d_script runs 4 bytes
 * long, and 907 of the 980 words of the three functions that use it
 * differ */
extern NJS_OBJECT ev000d_tornado_object[];
extern NJS_ACTION ev000d_tornado_action;
extern NJS_ACTION ev000d_sonic_action_0;
extern NJS_ACTION ev000d_sonic_action_1;

/* a pad recording no code plays, a key a step. no key holds a button. the stick
 * is pushed at most 68 out of 255, which suggests a slow walk */
static const struct event_pad_key ev000d_walk_keys[523] = {
#include "ev000d/script/ev000d_walk_keys.inc"
};

/* no code plays this recording */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct event_pad_recording ev000d_walk = {
    ev000d_walk_keys, sizeof(ev000d_walk_keys) / sizeof(ev000d_walk_keys[0])};

/* player 0's pad as Sonic runs at the Tornado and jumps aboard, a key a step.
 * the 24th to 43rd keys hold A. the 89th to 97th hold Start, which
 * event_play_pad holds on every key anyway */
static const struct event_pad_key ev000d_board_keys[101] = {
#include "ev000d/script/ev000d_board_keys.inc"
};

static const struct event_pad_recording ev000d_board = {
    ev000d_board_keys,
    sizeof(ev000d_board_keys) / sizeof(ev000d_board_keys[0])};

static struct task *ev000d_tornado = NULL;
/* the node ev000d_hide_sonic unlinks: Sonic, the fourth child of the Tornado's
 * model. NULL until it runs */
static NJS_OBJECT *ev000d_tornado_sonic = NULL;
static struct task *ev000d_fade = NULL;

/* unlink Sonic, the fourth child of the Tornado's model, and keep him in
 * ev000d_tornado_sonic. the four children after him stay chained to his
 * sibling pointer, so they leave the model with him */
static void ev000d_hide_sonic(void)
{
    ev000d_tornado_sonic =
        ev000d_tornado_object->child->sibling->sibling->sibling;
    ev000d_tornado_object->child->sibling->sibling->sibling = NULL;
}

/* not static: the image holds a copy of this function (0x0cb80022) that
 * nothing calls, beside the two SHC expands in ev000d_script. SHC keeps no
 * copy of a static function it expands at every call */
void ev000d_show_sonic(void)
{
    ev000d_tornado_object->child->sibling->sibling->sibling =
        ev000d_tornado_sonic;
}

void ev000d_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_000D.mlt");
        event_sound_init(9);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_set_pos(event_get_player(0), 1385.0F, 190.81F, 878.06F);
        event_set_ang(event_get_player(0), 0, 0x6e00, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(3, tails_step, 1418.05F, 201.80896F, 844.43F, 0,
                            0x7a2d, 0);
        event_wait(1);
        event_place_actor(&ev000d_tornado, 1475.05F, 211.9F, 779.59F, 0, 0x6600,
                          0);
        event_set_flags(ev000d_tornado, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_play_action(ev000d_tornado, &ev000d_tornado_action,
                          &ev000d_tornado_texlist, 0.7F, EVENT_REPEAT, 1);
        ev000d_hide_sonic();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1370.45F, 195.86F, 883.61F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0xd5f4, 0x0200);
        event_run_to_speed(event_get_player(3), 1412.85F, 201.8F, 819.4F, 0.4F,
                           2.0F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 1372.23F, 201.93F,
                             892.45F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xf500, 0xe2f4, 0);
        event_run_wait(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 4.0F,
                           0.0F);
        event_talk_start(event_get_player(3), TEXT_4);
        event_voice_play(1025);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(3));
        event_clear_motions(event_get_player(3));
        event_run_to_speed(event_get_player(3), 1459.93F, 201.81F, 781.48F,
                           1.0F, 0.5F);
        event_run_wait(event_get_player(3));
        event_wait(30);
        event_play_action(event_get_player(0), &ev000d_sonic_action_2,
                          &sonic_texlist, 0.4F, EVENT_REPEAT, 16);
        event_wait(180);
        event_camera_follow(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_run_to_speed(event_get_player(0), 1451.51F, 190.82F, 872.013F,
                           0.4F, 1.0F);
        event_wait(160);
        /* the ground shakes: the camera's pitch swings back and forth, and
         * the pad vibrates */
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0x0300, 0xe2f4, 0);
        event_play_action(event_get_player(0), &ev000d_sonic_action_1,
                          &sonic_texlist, 0.6F, 0, 8);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 0.4F, 0, 20);
        event_wait(3);
        vibration_start(0, 7, 20, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xf700, 0xe2f4, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0x0100, 0xe2f4, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0xf900, 0xe2f4, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 5, 0, 0xe2f4, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfa00, 0xe2f4, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xff00, 0xe2f4, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfb00, 0xe2f4, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfe00, 0xe2f4, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 6, 0xfc00, 0xe2f4, 0);
        event_wait(5);
        event_camera_follow_clear();
        event_clear_motions(event_get_player(0));
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_5);
        event_play_action(event_get_player(0), &ev000d_sonic_action_0,
                          &sonic_texlist, 0.2F, 0, 16);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1461.0508F, 196.81357F,
                             868.3691F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0x5000, 0);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        /* the runway and its six trees move out */
        event_flags[FLAG_RUNWAY] = TRUE;
        event_sound_start(0, 1223, 1800);
        event_sound_start(1, 1224, 1800);
        stream_request_music(MUSIC_THEMILES);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1247.4388F, 239.8842F,
                             898.55311F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0xb600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 1434.947F, 203.6006F,
                             840.14502F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0200, 0xa700, 0);
        event_wait(120);
        event_sound_stop(0);
        event_set_pos(event_get_player(3), 1395.04F, 221.55F, 700.56F);
        event_set_ang(event_get_player(3), 0, 0x569c, 0);
        event_wait(1);
        /* the runway's four panels move out */
        event_flags[FLAG_RUNWAY_PANELS] = TRUE;
        event_sound_start(2, 1225, 1800);
        event_sound_start(3, 1226, 1800);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1780.7604F, 238.06543F,
                             1087.6015F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0x2b00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 2176.6646F, 226.99631F,
                             1588.4917F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0xf900, 0x2b00, 0);
        event_wait(200);
        event_sound_stop(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1496.079F, 311.10575F,
                             820.10883F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc200, 0xb4f4, 0xfb00);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x238e);
        event_sound_stop(2);
        event_set_ang(event_get_player(0), 0, 0x569c, 0);
        event_move(ev000d_tornado, 1475.05F, 211.9F, 779.59F, 1529.07F, 211.9F,
                   857.49F, 400);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1510.6126F, 210.20572F,
                             833.8554F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a00, 0x1800, 0);
        event_camera_follow(ev000d_tornado);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(250);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1546.378F, 224.242F,
                             857.76F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf100, 0x4000, 0x0100);
        event_voice_play(1026);
        event_message_wait(60, TEXT_2);
        event_wait(10);
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev000d_board);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1474.82F, 193.45F, 843.83F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0x58f4, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x471c);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 38, event_get_player(0),
                                  0.0F, 3.0F, 0.0F, 0);
        ev000d_show_sonic();
        event_wait(38);
        event_camera_look_clear();
        event_wait(60);
        /* the Tornado's model holds its own Sonic again, so the player's
         * Sonic moves out of sight */
        event_set_pos(event_get_player(0), 1322.12F, 234.0F, 471.21F);
        event_set_ang(event_get_player(0), 0xf800, 0x1400, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1556.44F, 224.11F, 841.84F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, ev000d_tornado, 10.0F,
                                  0.0F, 10.0F, 0);
        event_wait(80);
        event_voice_play(1027);
        event_message_wait(60, TEXT_3);
        event_camera_follow(ev000d_tornado);
        event_play_action(ev000d_tornado, &ev000d_tornado_action,
                          &ev000d_tornado_texlist, 1.0F, EVENT_REPEAT, 1);
        event_wait(1);
        event_message_close();
        event_sound_start(5, 1228, 1800);
        event_play_path(ev000d_tornado, &ev000d_tornado_path, 0.5F, 0);
        event_sound_fade_volume(5, -100, 160);
        event_wait(40);
        event_camera_follow_clear();
        event_wait(40);
        event_camera_look_clear();
        event_wait(30);
        event_sound_stop(3);
        event_sound_start(4, 1227, 1800);
        event_wait(40);
        ev000d_fade = event_fade_create(1.0F / 60, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(80);
        event_sound_stop(4);
        event_sound_stop(5);
        event_wait(10);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        EVENT_KILL(ev000d_fade);
        ev000d_show_sonic();
        event_clear_paths(ev000d_tornado);
        event_flags[FLAG_RUNWAY_PANELS] = FALSE;
        event_flags[FLAG_RUNWAY] = FALSE;
        event_kill_task(&ev000d_tornado);
        event_kill_player(3);
        event_reset_player(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev000d_textures[] = {
    {"MILES", &tails_texlist},
    {"EV_TR1_WITH_SONIC", &ev000d_tornado_texlist},
    {NULL, NULL},
};
