/* EV003B, Tails' story: Tails and Sonic take off in the Tornado */
#include <ninja.h>

#include "adv02/runway.h"
#include "ev003b/script.h"
#include "ev003b/tornado_textures.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 21
#define TEXT_4_LENGTH 3
#include "ev003b/script/text.inc"

extern struct event_path ev003b_tornado_path;
/* @hack type: ev003b_tornado_object declared as an array, and read as
 * ev003b_tornado_object->child. each function that reads it then loads its
 * address from a pool word, and reads the child field 44 bytes past it, as
 * ev003b_show_sonic does at 0x0cb80040 and 0x0cb80042. as one NJS_OBJECT read
 * as ev003b_tornado_object.child, those pool words hold the child field's own
 * address, and the code runs 4 bytes long. 727 of the 830 words of
 * ev003b_script and the four hide and show functions then differ */
extern NJS_OBJECT ev003b_tornado_object[];
extern NJS_ACTION ev003b_tornado_action;

/* Sonic's run and jump onto the Tornado. the stick is pushed 249 or 250 of 255
 * at keys 8 to 118. it is pushed less at keys 7 and 119. A is held at keys 66
 * to 68 */
static const struct event_pad_key ev003b_sonic_board_keys[134] = {
#include "ev003b/script/ev003b_sonic_board_keys.inc"
};

static const struct event_pad_recording ev003b_sonic_board = {
    ev003b_sonic_board_keys,
    sizeof(ev003b_sonic_board_keys) / sizeof(ev003b_sonic_board_keys[0])};

/* Tails' run and flight up to the Tornado. the stick is pushed from key 0
 * through key 69. A is held at keys 36 to 45 */
static const struct event_pad_key ev003b_tails_board_keys[85] = {
#include "ev003b/script/ev003b_tails_board_keys.inc"
};

static const struct event_pad_recording ev003b_tails_board = {
    ev003b_tails_board_keys,
    sizeof(ev003b_tails_board_keys) / sizeof(ev003b_tails_board_keys[0])};

static struct task *ev003b_fade = NULL;
static struct task *ev003b_tornado = NULL;
/* the Tornado's Sonic and Tails nodes, saved while the stubs below take
 * their places */
static NJS_OBJECT *ev003b_tornado_sonic = NULL;
static NJS_OBJECT *ev003b_tornado_tails = NULL;

/* the stubs, which ev003b_hide_sonic and ev003b_hide_tails fill. each stub
 * takes a node's place, with that node's child and sibling and no model. a
 * stub's scale, which the bss leaves at 0, hides every node below the stub */
static NJS_OBJECT ev003b_tornado_sonic_stub;
static NJS_OBJECT ev003b_tornado_tails_stub;

/* put the stub in place of Sonic's node, the root's fourth child, and save
 * the node for ev003b_show_sonic */
static void ev003b_hide_sonic(void)
{
    ev003b_tornado_sonic_stub.model = NULL;
    ev003b_tornado_sonic_stub.child =
        ev003b_tornado_object->child->sibling->sibling->sibling->child;
    ev003b_tornado_sonic_stub.sibling =
        ev003b_tornado_object->child->sibling->sibling->sibling->sibling;
    ev003b_tornado_sonic =
        ev003b_tornado_object->child->sibling->sibling->sibling;
    ev003b_tornado_object->child->sibling->sibling->sibling =
        &ev003b_tornado_sonic_stub;
}

/* not static: the image keeps a copy of this function at 0x0cb80040 that
 * nothing calls, though SHC expands both of ev003b_script's calls to it. SHC
 * drops a static function it expands at every call */
void ev003b_show_sonic(void)
{
    ev003b_tornado_object->child->sibling->sibling->sibling =
        ev003b_tornado_sonic;
}

/* the same for Tails, the root's third child */
static void ev003b_hide_tails(void)
{
    ev003b_tornado_tails_stub.model = NULL;
    ev003b_tornado_tails_stub.child =
        ev003b_tornado_object->child->sibling->sibling->child;
    ev003b_tornado_tails_stub.sibling =
        ev003b_tornado_object->child->sibling->sibling->sibling;
    ev003b_tornado_tails = ev003b_tornado_object->child->sibling->sibling;
    ev003b_tornado_object->child->sibling->sibling = &ev003b_tornado_tails_stub;
}

/* not static, for the same reason as ev003b_show_sonic. the image keeps a copy
 * of this function at 0x0cb80080 that nothing calls, though SHC expands both of
 * ev003b_script's calls to it */
void ev003b_show_tails(void)
{
    ev003b_tornado_object->child->sibling->sibling = ev003b_tornado_tails;
}

void ev003b_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_000D.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_place_actor(&ev003b_tornado, 1475.05F, 209.9F, 779.59F, 0, 0x6600,
                          0);
        event_set_flags(ev003b_tornado, EVENT_IGNORE_GROUND);
        event_play_action(ev003b_tornado, &ev003b_tornado_action,
                          &ev003b_tornado_texlist, 0.001F, EVENT_REPEAT, 1);
        ev003b_hide_sonic();
        ev003b_hide_tails();
        event_create_player(3, sonic_step, 1385.0F, 190.81F, 878.06F, 0, 0x6e00,
                            0);
        event_set_pos(event_get_player(0), 1418.05F, 201.8F, 844.43F);
        event_set_ang(event_get_player(0), 0, 0x7a00, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1434.64F, 211.19F, 839.28F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf700, 0x4f00, 0);
        event_run_to_speed(event_get_player(0), 1412.85F, 201.8F, 819.4F, 0.4F,
                           2.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xff00, 0x1100, 0);
        /* the runway and its trees move out, and the runway turns solid */
        event_flags[FLAG_RUNWAY] = 1;
        event_wait(60);
        event_run_wait(event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(3), 0.0F, 4.0F,
                           0.0F);
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1236);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_set_pos(event_get_player(0), 1434.1F, 201.8F, 796.23F);
        event_set_ang(event_get_player(0), 0, 0x654e, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1430.46F, 209.28F, 798.79F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff00, 0xd500, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x58e4);
        ev003b_fade = event_fade_create(1.0F / 1300, 0.5F, 0.0F, 0.0F, 0.0F);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x31c7);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev003b_tails_board);
        event_wait(25);
        event_sound_start(0, 1226, 1800);
        if (ev003b_fade != NULL) {
            event_fade_set_speed(ev003b_fade, 1.0F / 30);
        }
        event_wait(40);
        /* player 0 moves away from the Tornado. from ev003b_show_tails below
         * on, the Tornado's own Tails node shows Tails */
        event_set_pos(event_get_player(0), 1395.04F, 221.55F, 701.0F);
        event_set_ang(event_get_player(0), 0, 0x1612, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1449.16F, 216.6F, 768.89F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0xa800, 0);
        event_set_pos(event_get_player(3), 1445.68F, 190.82F, 866.08F);
        event_set_ang(event_get_player(3), 0, 0x6600, 0);
        event_wait(1);
        event_clear_motions(ev003b_tornado);
        event_wait(1);
        ev003b_show_tails();
        event_play_action(ev003b_tornado, &ev003b_tornado_action,
                          &ev003b_tornado_texlist, 0.7F, EVENT_REPEAT, 1);
        stream_request_music(MUSIC_THEMILES);
        event_sound_start(1, 1225, 1800);
        /* the runway's panels move out too */
        event_flags[FLAG_RUNWAY_PANELS] = 1;
        /* @hack spelling: the test and the call as one expression. SHC then
         * keeps event_camera_set_pos's address in R10 and
         * event_camera_set_ang's in R11 (0x0cb80190, 0x0cb801aa). as an if,
         * the two trade, and 40 of 763 words differ */
        ev003b_fade != NULL ? event_fade_set_speed(ev003b_fade, -0.005F)
                            : (void)0;
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 1462.85F, 219.505F,
                             763.11F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0xf600, 0x9a00, 0);
        event_wait(380);
        event_sound_stop(1);
        event_sound_start(2, 1228, 1800);
        event_move(ev003b_tornado, 1475.05F, 211.9F, 779.59F, 1529.07F, 211.9F,
                   857.49F, 400);
        event_look_at_task(event_get_player(3), ev003b_tornado, 0.0F, 10.0F,
                           0.0F);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1516.095F, 287.175F,
                             825.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd100, 0x1e00, 0);
        event_camera_follow(ev003b_tornado);
        event_wait(200);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1540.63F, 222.12F, 849.62F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf700, 0x4e00, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, 1541.01F, 218.075F,
                             856.15F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0xf700, 0x4f00, 0);
        event_look_clear(event_get_player(3));
        event_set_ang(event_get_player(3), 0, 0x4000, 0);
        event_wait(1);
        event_voice_play(1237);
        event_sound_fade_volume(2, -100, 100);
        event_message_wait(60, TEXT_2);
        event_wait(100);
        event_message_clear();
        event_clear_motions(event_get_player(3));
        event_play_pad(3, &ev003b_sonic_board);
        event_wait(100);
        ev003b_show_sonic();
        event_wait(30);
        event_voice_play(1238);
        event_message_wait(60, TEXT_3);
        /* player 3 moves away from the Tornado. since ev003b_show_sonic
         * above, the Tornado's own Sonic node shows Sonic */
        event_set_pos(event_get_player(3), 1395.04F, 221.55F, 700.56F);
        event_set_ang(event_get_player(3), 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1556.44F, 224.11F, 841.84F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, ev003b_tornado, 0.0F,
                                  0.0F, 0.0F, 0);
        event_play_action(ev003b_tornado, &ev003b_tornado_action,
                          &ev003b_tornado_texlist, 1.0F, EVENT_REPEAT, 1);
        event_wait(1);
        event_camera_follow(ev003b_tornado);
        event_message_close();
        event_sound_fade_volume(2, 0, 1);
        event_play_path(ev003b_tornado, &ev003b_tornado_path, 0.5F, 0);
        event_wait(40);
        event_sound_fade_volume(2, -200, 160);
        event_camera_follow_clear();
        event_wait(40);
        event_camera_look_clear();
        event_wait(20);
        event_sound_stop(0);
        event_sound_start(3, 1227, 100);
        event_sound_fade_volume(3, -200, 100);
        event_wait(50);
        if (ev003b_fade != NULL) {
            event_fade_set_speed(ev003b_fade, 1.0F / 60);
        }
        event_wait(80);
        event_sound_stop(2);
        event_sound_stop(3);
        event_wait(10);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_move_stop_all();
        EVENT_KILL(ev003b_fade);
        event_flags[FLAG_RUNWAY_PANELS] = 0;
        event_flags[FLAG_RUNWAY] = 0;
        ev003b_show_tails();
        ev003b_show_sonic();
        event_clear_paths(ev003b_tornado);
        event_kill_task(&ev003b_tornado);
        event_kill_player(3);
        event_reset_player(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev003b_textures[] = {
    {"EV_TR1_WITH_SONIC", &ev003b_tornado_texlist},
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
