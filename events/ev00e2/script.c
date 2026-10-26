/* EV00E2: in Big's ending, he and Froggy cross Mystic Ruins' jungle, where
 * the Tornado 2 lies crashed */
#include <ninja.h>

#include "ev00e2/script.h"
#include "ev00e2/tornado_object.h"
#include "ev00e2/tornado_textures.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_models.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/frog.h"
#include "shared/move.h"
#include "shared/prop.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

/* the crashed Tornado 2's prop slot */
#define TORNADO 0

extern NJS_ACTION ev00e2_big_action_0;
extern NJS_ACTION ev00e2_big_action_1;
extern NJS_ACTION ev00e2_big_action_2;
extern NJS_ACTION ev00e2_big_action_3;
extern NJS_ACTION ev00e2_big_action_4;
extern NJS_ACTION ev00e2_frog_action_0;
extern NJS_ACTION ev00e2_frog_action_1;
extern NJS_ACTION ev00e2_frog_action_2;
extern NJS_ACTION ev00e2_frog_action_3;
extern NJS_ACTION ev00e2_frog_action_4;

/* Big's run and jump, as pad keys a step each. the A button is held for
 * the 18 keys from key 37 */
static const struct event_pad_key ev00e2_jump_keys[87] = {
#include "ev00e2/script/ev00e2_jump_keys.inc"
};

static const struct event_pad_recording ev00e2_jump = {
    ev00e2_jump_keys, sizeof(ev00e2_jump_keys) / sizeof(ev00e2_jump_keys[0])};

/* a longer run that nothing plays, as pad keys a step each. the A button
 * is held for the 25 keys from key 255 */
static const struct event_pad_key ev00e2_spare_keys[314] = {
#include "ev00e2/script/ev00e2_spare_keys.inc"
};

/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct event_pad_recording ev00e2_spare = {
    ev00e2_spare_keys,
    sizeof(ev00e2_spare_keys) / sizeof(ev00e2_spare_keys[0])};

/* two paths that nothing plays. the second is the first moved by (9.163,
 * -1.0, -11.704), with each angle within 1 of the first's */
static struct event_path_point ev00e2_spare_points_0[88] = {
#include "ev00e2/script/ev00e2_spare_points_0.inc"
};

static struct event_path ev00e2_spare_path_0 = {88, ev00e2_spare_points_0};

static struct event_path_point ev00e2_spare_points_1[88] = {
#include "ev00e2/script/ev00e2_spare_points_1.inc"
};

static struct event_path ev00e2_spare_path_1 = {88, ev00e2_spare_points_1};

static struct task *ev00e2_frog = NULL;
/* no code uses this word. it keeps ev00e2_flash_0 at its address in the
 * image */
static struct task *dat_0cb87f44 = NULL;
static struct task *ev00e2_flash_0 = NULL;
static struct task *ev00e2_flash_1 = NULL;
static struct task *ev00e2_black = NULL;

void ev00e2_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        ev00e2_black = event_fade_create(1.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        stream_request_music(MUSIC_BIG);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_set_pos(event_get_player(0), -1108.0F, 354.145F, 365.745F);
        event_set_ang(event_get_player(0), 0xffd0, 0x6700, 0xffef);
        event_reset_player(0);
        event_place_task(&ev00e2_frog, event_frog_create,
                         event_get_player(0)->work->pos.x,
                         event_get_player(0)->work->pos.y + 12.0F,
                         event_get_player(0)->work->pos.z, 0, 0xd300, 0);
        event_prop_create(1226.0F, 99.0F, -904.66F, 0x1000, 0xd000, 0x0400,
                          1.0F, 1.0F, 1.0F, &ev00e2_tornado_object,
                          &ev00e2_tornado_texlist, TORNADO);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x38e4);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a00, 0xd900, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1145.23F, 562.12F, -81.76F);
        event_wait(1);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then keeps
         * event_get_player's address in R12 and event_wait's in R13
         * (0x0cb80026, 0x0cb8014a). as an if, the two trade, and 84 of 1287
         * words differ */
        ev00e2_black != NULL &&
            (event_fade_set_speed(ev00e2_black, -1.0F / 30.0F), 0);
        event_wait(18);
        event_set_flags(ev00e2_frog, EVENT_IGNORE_GROUND);
        event_set_shadow(ev00e2_frog, 0.8F);
        event_set_pos(ev00e2_frog, -1118.336F, 354.9F, 355.33F);
        event_set_ang(ev00e2_frog, 0xf400, 0x6700, 0);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_0,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_frog_set_shape(2);
        event_wait(1);
        event_follow(ev00e2_frog, -10.0F, 0.3F, -10.0F, 999,
                     event_get_player(0));
        event_set_radius(ev00e2_frog, 1.0F);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0500, 0xd900, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -1107.27F, 209.343F,
                             -108.52F);
        event_wait(200);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0800, 0x5500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -570.0F, 222.9F, 197.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 144, 0x0800, 0x5800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 144, -655.5F, 224.46F, 62.68F);
        event_wait(144);
        ev00e2_flash_0 = event_fade_create(0.025F, 0.1F, 0.9F, 0.9F, 0.9F);
        event_wait(10);
        /* @hack spelling: the test and the call as one expression, as
         * EVENT_KILL has them. SHC then keeps event_get_player's address in
         * R12 and event_wait's in R13. as an if, the two trade, and 83 of
         * 1287 words differ */
        ev00e2_flash_0 != NULL &&
            (event_fade_set_speed(ev00e2_flash_0, 0.0F), 0);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 110, 0x2e39);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c00, 0x5800, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1098.0F, 365.55F, 360.0F);
        event_wait(10);
        /* @hack spelling: the test and the call as one expression, as
         * EVENT_KILL has them. SHC then keeps event_get_player's address in
         * R12 and event_wait's in R13. as an if, the two trade, and 83 of
         * 1287 words differ */
        ev00e2_flash_0 != NULL &&
            (event_fade_set_speed(ev00e2_flash_0, -0.025F), 0);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0c00, 0x4700, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -1098.4F, 367.16F, 363.66F);
        event_wait(90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3c72);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf400, 0xe100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1152.4F, 388.7F, 371.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0xf500, 0xe700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -1138.3F, 388.2266F,
                             392.0F);
        event_wait(46);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev00e2_jump);
        event_clear_motions(ev00e2_frog);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_1,
                          &event_frog_texlist, 1.1F, EVENT_REPEAT, 0);
        event_wait(45);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_1,
                          &event_frog_texlist, 0.0F, EVENT_REPEAT, 0);
        event_wait(76);
        event_move_stop_all();
        event_clear_motions(ev00e2_frog);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &big_run_action, &big_texlist,
                          1.3F, EVENT_REPEAT, 0);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_1,
                          &event_frog_texlist, 1.1F, EVENT_REPEAT, 0);
        event_follow(ev00e2_frog, 10.0F, 0.5F, 13.0F, 999, event_get_player(0));
        event_set_pos(event_get_player(0), -1034.44F, 107.1F, 318.443F);
        event_set_ang(event_get_player(0), 0xf97c, 0x5800, 0xf821);
        event_run_to_speed(event_get_player(0), -881.65F, 46.0F, 205.6F, 0.0F,
                           0.0F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 1, event_get_player(0),
                                  0.0F, 9.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1005.0F, 123.0F, 310.0F);
        event_wait(1);
        event_camera_follow(event_get_player(0));
        event_wait(90);
        event_camera_look_clear();
        event_camera_follow_clear();
        event_wait(9);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe300, 0x2d00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 313.3F, 74.92F, -1185.87F);
        event_wait(5);
        event_move_stop_all();
        event_run_clear(event_get_player(0));
        event_set_pos(event_get_player(0), 220.74F, -2.214F, -1224.85F);
        event_set_ang(event_get_player(0), 0xfa07, 0xb477, 0x0d02);
        event_set_pos(ev00e2_frog, 219.47F, 13.5F, -1225.3F);
        event_set_ang(ev00e2_frog, 0xf9f4, 0xac19, 0x0d15);
        event_set_radius(ev00e2_frog, 0.0F);
        event_set_flags(ev00e2_frog, EVENT_IGNORE_GROUND);
        event_clear_motions(ev00e2_frog);
        event_clear_motions(event_get_player(0));
        event_play_action(ev00e2_frog, &ev00e2_frog_action_0,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev00e2_big_action_0,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0800, 0x3100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 301.39F, 9.05F, -1198.83F);
        event_wait(130);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1200, 0xcf00, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 187.87F, -8.0438F,
                             -1211.433F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 280, 0x1200, 0xbe00, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 280, 184.122F, -8.62F,
                             -1223.76F);
        event_wait(30);
        event_clear_motions(ev00e2_frog);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_3,
                          &event_frog_texlist, 0.9F, 0, 16);
        event_wait(5);
        event_move(ev00e2_frog, 219.47F, 13.5F, -1225.3F, 217.625F, 0.0F,
                   -1211.5F, 16);
        event_wait(16);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_4,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(0), &ev00e2_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 30);
        event_wait(60);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_2,
                          &event_frog_texlist, 2.0F, 0, 15);
        event_wait(23);
        event_set_ang(ev00e2_frog, 0xf9f4, 0x8500, 0x0d15);
        event_wait(15);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_0,
                          &event_frog_texlist, 5.5F, EVENT_REPEAT, 0);
        event_wait(5);
        event_play_action(event_get_player(0), &ev00e2_big_action_1,
                          &big_texlist, 5.0F, EVENT_REPEAT, 0);
        event_wait(55);
        event_play_action(event_get_player(0), &ev00e2_big_action_2,
                          &big_texlist, 0.8F, 0, 0);
        event_play_action(event_get_player(0), &ev00e2_big_action_3,
                          &big_texlist, 0.8F, EVENT_REPEAT, 0);
        event_wait(40);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_0,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(20);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_0,
                          &event_frog_texlist, 0.01F, EVENT_REPEAT, 0);
        event_wait(15);
        event_set_ang(ev00e2_frog, 0xf9f4, 0x7000, 0x0d15);
        event_wait(25);
        ev00e2_flash_1 = event_fade_create(0.05F, 0.1F, 0.9F, 0.9F, 0.9F);
        event_wait(10);
        if (ev00e2_flash_1 != NULL) {
            event_fade_set_speed(ev00e2_flash_1, 0.0F);
        }
        event_wait(10);
        event_move_stop_all();
        event_set_ang(ev00e2_frog, 0, 0x9000, 0);
        event_clear_motions(ev00e2_frog);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &big_run_action, &big_texlist,
                          1.4F, EVENT_REPEAT, 0);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_1,
                          &event_frog_texlist, 1.1F, EVENT_REPEAT, 0);
        event_follow_facing(ev00e2_frog, -18.0F, 0.5F, -5.0F, 999,
                            event_get_player(0));
        event_set_pos(event_get_player(0), 613.7F, 28.353F, -1074.874F);
        event_set_ang(event_get_player(0), 0xfe60, 0x8530, 0x0620);
        event_set_radius(ev00e2_frog, 1.0F);
        event_set_flags(ev00e2_frog, EVENT_GROUND | EVENT_SLOPE);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 606.8F, 30.59F, -1053.545F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b00, 0xfd00, 0x0200);
        event_run_to_speed(event_get_player(0), 605.89F, 24.6255F, -1257.91F,
                           1.0F, 1.0F);
        event_wait(1);
        event_camera_follow(event_get_player(0));
        if (ev00e2_flash_1 != NULL) {
            event_fade_set_speed(ev00e2_flash_1, -0.025F);
        }
        event_wait(130);
        event_run_to_speed(event_get_player(0), 499.0F, 10.0F, -1309.6F, 1.0F,
                           1.0F);
        event_wait(45);
        event_camera_follow_clear();
        event_run_clear(event_get_player(0));
        event_move_stop_all();
        event_set_radius(ev00e2_frog, 0.0F);
        event_set_flags(ev00e2_frog, EVENT_IGNORE_GROUND);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00e2_frog);
        event_set_pos(event_get_player(0), -519.0F, 155.0F, -1081.8F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_pos(ev00e2_frog, -519.0F, 141.3F, -1052.0F);
        event_set_ang(ev00e2_frog, 0, 0x8000, 0xfd00);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_0,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev00e2_big_action_4,
                          &big_texlist, 0.8F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -520.9F, 157.5F, -1058.25F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b00, 0xfd00, 0x0200);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(59);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -519.0F, 140.78F,
                             -1043.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x1200, 0, 0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -519.0F, 140.4F, -1043.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1200, 0, 0);
        event_wait(30);
        event_set_pos(ev00e2_frog, -519.0F, 141.3F, -1058.0F);
        event_clear_motions(ev00e2_frog);
        event_play_action(ev00e2_frog, &ev00e2_frog_action_1,
                          &event_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -537.0F, 196.0F, -1025.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe100, 0xf500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 500, -608.6F, 451.0F, -767.0F);
        event_wait(275);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_clear_paths(event_get_player(0));
        event_clear_paths(ev00e2_frog);
        event_reset_player(0);
        event_prop_kill(TORNADO);
        event_kill_task(&ev00e2_frog);
        EVENT_KILL(ev00e2_flash_0);
        EVENT_KILL(ev00e2_flash_1);
        EVENT_KILL(ev00e2_black);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00e2_textures[] = {
    {"SHAPE_FROG", &event_frog_texlist},
    {"TR2CRASH", &ev00e2_tornado_texlist},
    {NULL, NULL},
};
