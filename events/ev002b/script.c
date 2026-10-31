/* EV002B: the police surround Chaos 0 and fire on it. their bullets fall
 * out of it, the police fall back, and Sonic jumps in */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev002b/police_action_10.h"
#include "ev002b/police_textures.h"
#include "ev002b/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/chaos0.h"
#include "shared/chaos0_hold.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/timer.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 26
#define TEXT_3_LENGTH 18
#define TEXT_4_LENGTH 21
#define TEXT_5_LENGTH 24
#define TEXT_6_LENGTH 34
#define TEXT_7_LENGTH 4
#include "ev002b/script/text.inc"

extern NJS_ACTION sonic_jump_land_action;

/* @hack spelling: event_sprite_explosion_texlist is
 * event_sprite_texlists[EVENT_SPRITE_EXPLOSION], declared under a name of its
 * own at the element's address, 0x0cbce6a8. with the name, SHC loads the
 * address from a pool word at each of the ten calls, as at 0x0cb812cc. as
 * &event_sprite_texlists[EVENT_SPRITE_EXPLOSION], ev002b_script runs 12 bytes
 * long, and 3707 of its 3838 words differ */
/* @hack type: event_sprite_explosion_texlist declared as an array, which
 * script.c passes by its name. SHC then loads the angle 0x00f3 into R8 at
 * 0x0cb80114, and 0xff5e into R10 at 0x0cb8010e. as one NJS_TEXLIST passed as
 * &event_sprite_explosion_texlist, the two trade registers, and 8 of
 * ev002b_script's 3838 words differ */
#ifdef GCC
/* GCC's link binds a name only where a definition starts, and this one
 * names a texlist inside event_sprite_texlists */
#define event_sprite_explosion_texlist                                         \
    (&event_sprite_texlists[EVENT_SPRITE_EXPLOSION])
#else
extern NJS_TEXLIST event_sprite_explosion_texlist[];
#endif

extern NJS_ACTION ev002b_police_action_0;
extern NJS_ACTION ev002b_police_action_1;
extern NJS_ACTION ev002b_police_action_2;
extern NJS_ACTION ev002b_police_action_3;
extern NJS_ACTION ev002b_police_action_4;
extern NJS_ACTION ev002b_police_action_5;
extern NJS_ACTION ev002b_police_action_6;
extern NJS_ACTION ev002b_police_action_7;
extern NJS_ACTION ev002b_police_action_8;
extern NJS_ACTION ev002b_police_action_9;
extern NJS_ACTION ev002b_bullet_action_0;
extern NJS_ACTION ev002b_bullet_action_1;

/* Sonic's run, a key a step. the stick is pushed nearly all the way for keys 1
 * to 9. A is held for keys 6 to 8 and 23 to 24. the stick is pushed lightly for
 * keys 55 to 58 */
static const struct event_pad_key ev002b_run_keys[60] = {
#include "ev002b/script/ev002b_run_keys.inc"
};

static const struct event_pad_recording ev002b_run = {
    ev002b_run_keys, sizeof(ev002b_run_keys) / sizeof(ev002b_run_keys[0])};

static struct event_path_point ev002b_retreat_points_0[13] = {
#include "ev002b/script/ev002b_retreat_points_0.inc"
};

static struct event_path ev002b_retreat_path_0 = {13, ev002b_retreat_points_0};

static struct event_path_point ev002b_retreat_points_1[13] = {
#include "ev002b/script/ev002b_retreat_points_1.inc"
};

static struct event_path ev002b_retreat_path_1 = {13, ev002b_retreat_points_1};

static struct event_path_point ev002b_retreat_points_2[15] = {
#include "ev002b/script/ev002b_retreat_points_2.inc"
};

static struct event_path ev002b_retreat_path_2 = {15, ev002b_retreat_points_2};

static struct event_path_point ev002b_retreat_points_3[11] = {
#include "ev002b/script/ev002b_retreat_points_3.inc"
};

static struct event_path ev002b_retreat_path_3 = {11, ev002b_retreat_points_3};

static struct task *ev002b_police_0 = NULL;
static struct task *ev002b_police_1 = NULL;
static struct task *ev002b_police_2 = NULL;
static struct task *ev002b_police_3 = NULL;
static struct task *ev002b_police_4 = NULL;
static struct task *ev002b_police_5 = NULL;
static struct task *ev002b_police_6 = NULL;
/* the police's bullets, which fall out of Chaos 0 */
static struct task *ev002b_bullet_0 = NULL;
static struct task *ev002b_bullet_1 = NULL;
static struct task *ev002b_bullet_2 = NULL;
static struct task *ev002b_bullet_3 = NULL;
static struct task *ev002b_bullet_4 = NULL;
static struct task *ev002b_bullet_5 = NULL;
static struct task *ev002b_bullet_6 = NULL;
static struct task *ev002b_bullet_7 = NULL;
static struct task *ev002b_bullet_8 = NULL;
static struct task *ev002b_bullet_9 = NULL;
static struct task *ev002b_bullet_10 = NULL;
static struct task *ev002b_bullet_11 = NULL;
static struct task *ev002b_bullet_12 = NULL;
static struct task *ev002b_bullet_13 = NULL;
static struct task *ev002b_bullet_14 = NULL;
static struct task *ev002b_bullet_15 = NULL;
static struct task *ev002b_bullet_16 = NULL;
static struct task *ev002b_bullet_17 = NULL;
static struct task *ev002b_bullet_18 = NULL;
static struct task *ev002b_bullet_19 = NULL;

struct texture_pvm ev002b_textures[] = {
    {"MP_10000_POLICE", &ev002b_police_texlist},
    {"CHAOS0", &event_chaos0_texlist},
    {"BULLET", &ev002b_bullet_texlist},
    {NULL, NULL},
};

void ev002b_script(int phase)
{
    /* @hack spelling: event_set_ang called through a local, assigned just
     * before its first call. SHC then keeps event_camera_set_pos in R11 and
     * sound_play_at in R10, loaded at 0x0cb8081c and 0x0cb809e0. called by
     * name, event_set_ang still goes in R9 at 0x0cb8009e, but the two trade
     * registers, and 80 of ev002b_script's 3838 words differ */
    void (*set_ang)(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z);
    /* @hack spelling: 381.94, the z ev002b_police_2 walks to and most sounds
     * play at, through a local. SHC loads it into FR12 at 0x0cb80bf6 and keeps
     * it there for all 39 uses. as the literal at each use, it still goes in
     * FR12, but event_camera_set_pos and sound_play_at trade registers as
     * they do without set_ang, and 80 of ev002b_script's 3838 words differ */
    Float sound_z;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        draw_off = TRUE;
        event_wait(1);
        event_reset_player(0);
        sound_load_file("e_0001.mlt");
        event_set_pos(event_get_player(0), 411.71F, 1.14F, 398.55F);
        set_ang = event_set_ang;
        set_ang(event_get_player(0), 0, 0x67c1, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_chaos0_create(271.61F, 490.15F, 180.0F, 0, 0x4000, 0, 0);
        event_wait(1);
        event_chaos0_hold(407.91F, 1.0F, 432.55F, 0, 0x4000, 0, 60);
        event_place_actor(&ev002b_police_0, 339.6F, 10.589499F, 329.38F, 0x00f3,
                          0x97cf, 0xff5e);
        event_place_actor(&ev002b_police_1, 336.46F, 9.4595F, 380.69F, 0,
                          0x9a4b, 0);
        event_place_actor(&ev002b_police_2, 279.15F, 9.4595F, 399.43F, 0x0103,
                          0x8be1, 0xffcd);
        event_place_actor(&ev002b_police_3, 301.15F, 9.4595F, 398.41F, 0,
                          0x88e4, 0);
        event_place_actor(&ev002b_police_4, 236.89F, 11.0395F, 330.36F, 0x0108,
                          0x726c, 0x0034);
        event_place_actor(&ev002b_police_5, 339.6F, 10.589499F, 329.38F, 0x00f3,
                          0x97cf, 0xff5e);
        event_place_actor(&ev002b_police_6, 336.46F, 9.4595F, 380.69F, 0,
                          0x9a4b, 0);
        event_place_actor(&ev002b_bullet_0, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_1, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_2, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_3, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_4, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_5, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_6, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_7, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_8, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_9, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_10, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_11, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_12, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_13, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_14, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_15, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_16, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_17, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_18, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_place_actor(&ev002b_bullet_19, 268.0F, 11.7F, 300.0F, 0, 0x4000,
                          0);
        event_wait(1);
        event_set_flags(ev002b_police_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_police_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_police_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_police_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_police_4, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_police_5, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_police_6, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_4, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_5, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_6, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_7, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_8, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_9, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_10, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_11, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_12, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_13, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_14, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_15, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_16, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_17, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_18, EVENT_IGNORE_GROUND);
        event_set_flags(ev002b_bullet_19, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_pos(ev002b_police_0, 379.46F, 9.4595F, 353.04F);
        set_ang(ev002b_police_0, 0, 0x9905, 0);
        event_set_pos(ev002b_police_1, 313.1F, 9.4595F, 408.84F);
        set_ang(ev002b_police_1, 0, 0x9a4b, 0);
        event_set_pos(ev002b_police_2, 275.89F, 9.4595F, 423.5F);
        set_ang(ev002b_police_2, 0, 0x8019, 0);
        event_set_pos(ev002b_police_3, 256.0F, 9.4595F, 401.11F);
        set_ang(ev002b_police_3, 0, 0x7bb0, 0);
        event_set_pos(ev002b_police_4, 220.3F, 9.4595F, 393.95F);
        set_ang(ev002b_police_4, 0, 0x746b, 0);
        event_set_pos(ev002b_police_5, 330.45F, 9.4595F, 372.68F);
        set_ang(ev002b_police_5, 0x00f3, 0x8ca7, 0xff5e);
        event_set_pos(ev002b_police_6, 196.91F, 9.4595F, 376.54F);
        set_ang(ev002b_police_6, 0x0103, 0x6c33, 0x00ad);
        event_wait(1);
        event_play_action(ev002b_police_0, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_1, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_2, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_3, &ev002b_police_action_1,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_4, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_5, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_6, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_set_shadow(ev002b_police_0, 0.8F);
        event_set_shadow(ev002b_police_1, 0.8F);
        event_set_shadow(ev002b_police_2, 0.8F);
        event_set_shadow(ev002b_police_3, 0.8F);
        event_set_shadow(ev002b_police_4, 0.8F);
        event_set_shadow(ev002b_police_5, 0.8F);
        event_set_shadow(ev002b_police_6, 0.8F);
        draw_off = FALSE;
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 285.38F, 4.98F, 483.49F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1049, 0x0a00, 0xfe00);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x38e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 286.29F, 3.76F, 487.12F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1466, 0x0a00, 0xfe00);
        event_wait(70);
        event_chaos0_hop2(271.61F, 280.0F, 202.0F, 30.0F, 1);
        event_wait(1);
        event_chaos0_hop2(271.61F, 58.849995F, 227.0F, 30.0F, 90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 289.05F, 301.38F, 201.36F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xcc00, 0x5f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 298.51F, 344.44F, 192.35F);
        event_wait(15);
        stream_request_music(MUSIC_EVTBGM01);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 282.11F, 49.0F, 246.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x3027, 0x2000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 283.95F, 53.12F, 248.77F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1800, 0x2000, 0);
        event_wait(46);
        sound_play_at(1232, 0, 0, 100, 271.61F, 58.849995F, 227.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1750, 0x2000, 0);
        event_wait(1);
        vibration_start(0, 5, 50, 1);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0x2000, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x1700, 0x2000, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0x2000, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x1700, 0x2000, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0x2000, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 1, 0x1750, 0x2000, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0x1800, 0x2000, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 278.66F, 58.12F, 243.48F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 210.2F, 83.52F, 210.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xeb00, 0x8100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 324.09F, 83.52F, 206.2F);
        event_clear_motions(ev002b_police_6);
        event_play_action(ev002b_police_6, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev002b_police_6, 196.91F, 9.4595F, 376.54F, 221.05F,
                   10.5795F, 334.09F, 30);
        sound_play_at_for(1229, EVENT_SOUND_ID, 1, 100, 221.05F, 10.5795F,
                          334.09F, 30);
        event_wait(10);
        event_message_show(TEXT_1);
        event_voice_play(935);
        event_play_action(ev002b_police_4, &ev002b_police_action_5,
                          &ev002b_police_texlist, 1.0F, 0, 6);
        event_play_action(ev002b_police_4, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(ev002b_police_2);
        event_play_action(ev002b_police_2, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        sound_z = 381.94F;
        event_move(ev002b_police_2, 275.89F, 9.4595F, 423.5F, 280.99F, 9.4595F,
                   sound_z, 50);
        sound_play_at_for(1229, EVENT_SOUND_ID + 1, 1, 120, 280.99F, 9.4595F,
                          sound_z, 50);
        event_play_action(ev002b_police_6, &ev002b_police_action_2,
                          &ev002b_police_texlist, 1.0F, 0, 2);
        event_play_action(ev002b_police_6, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        sound_play_at(1233, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(20);
        event_move_stop(ev002b_police_6);
        event_play_action(ev002b_police_3, &ev002b_police_action_5,
                          &ev002b_police_texlist, 1.0F, 0, 6);
        event_play_action(ev002b_police_3, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        sound_play_at(1233, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(25);
        event_play_action(ev002b_police_1, &ev002b_police_action_5,
                          &ev002b_police_texlist, 1.0F, 0, 6);
        event_play_action(ev002b_police_1, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_2, &ev002b_police_action_2,
                          &ev002b_police_texlist, 1.0F, 0, 2);
        event_play_action(ev002b_police_2, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_message_show(TEXT_2);
        event_voice_play(936);
        event_move(ev002b_police_5, 330.45F, 9.4595F, 372.68F, 318.65F,
                   10.679499F, 339.67F, 45);
        sound_play_at_for(1229, EVENT_SOUND_ID + 2, 1, 100, 318.65F, 10.679499F,
                          339.67F, 45);
        sound_play_at(1233, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(15);
        event_play_action(ev002b_police_3, &ev002b_police_action_2,
                          &ev002b_police_texlist, 1.0F, 0, 2);
        event_play_action(ev002b_police_3, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move_stop(ev002b_police_2);
        sound_play_at(1233, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(20);
        event_play_action(ev002b_police_5, &ev002b_police_action_2,
                          &ev002b_police_texlist, 1.0F, 0, 2);
        event_play_action(ev002b_police_5, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev002b_police_0, 379.46F, 9.4595F, 353.04F, 353.33F, 9.4595F,
                   348.8F, 30);
        sound_play_at_for(1229, EVENT_SOUND_ID + 3, 1, 80, 353.33F, 9.4595F,
                          348.8F, 30);
        sound_play_at(1233, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(10);
        event_move_stop(ev002b_police_5);
        event_wait(10);
        sound_play_at(1233, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 331.09F, 83.52F, 206.02F);
        event_play_action(ev002b_police_0, &ev002b_police_action_5,
                          &ev002b_police_texlist, 1.0F, 0, 2);
        event_play_action(ev002b_police_0, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        sound_play_at(1233, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_move_stop(ev002b_police_0);
        event_wait(10);
        event_voice_stop();
        event_message_close();
        event_voice_stop();
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 264.7F, 6.62F, 408.24F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x06fb, 0x229e, 0);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 331.79F, 6.88F, 347.37F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 335.09F, 6.88F, 343.62F);
        event_wait(30);
        event_chaos0_hop2(268.0F, 3.1500006F, 300.0F, 120.0F, 60);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30,
                                  event_chaos0_get_task(), 0.0F, 2.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 293.88F, 9.39F, 277.17F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x238e);
        set_ang(ev002b_police_0, 0, 0xa500, 0);
        set_ang(ev002b_police_4, 0, 0x7000, 0);
        set_ang(ev002b_police_5, 0x0103, 0xa600, 0x00ad);
        set_ang(ev002b_police_6, 0x0103, 0x5d00, 0x00ad);
        event_play_action(ev002b_police_3, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0x00e8, 0x661c, 0);
        sound_play_at(1232, 0, 0, 100, 268.0F, 3.1500006F, 300.0F);
        vibration_start(0, 5, 50, 1);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 296.71F, 11.05F, 253.35F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x02c4, 0x6f28, 0);
        event_wait(20);
        event_message_show(TEXT_3);
        event_voice_play(938);
        event_wait(10);
        event_play_action(ev002b_police_3, &ev002b_police_action_2,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_3, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        sound_play_at(1233, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(30);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_chaos0_get_task(),
                                  0.0F, -5.0F, 0.0F, 0xfd00);
        event_camera_orbit(EVENT_EASE_LINEAR, 100, event_chaos0_get_task(),
                           15.0F, 0, 0x2aab, 0, 0, -0x2aaa, 0, 160.0F, 100.0F);
        event_play_action(ev002b_police_0, &ev002b_police_action_4,
                          &ev002b_police_texlist, 2.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_0, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_sprite_create(ev002b_police_0, 0, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(0, 0.18F, 0.18F);
        event_play_action(ev002b_police_5, &ev002b_police_action_4,
                          &ev002b_police_texlist, 2.0F, EVENT_REPEAT, 0);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_sprite_create(ev002b_police_5, 1, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(1, 0.18F, 0.18F);
        event_wait(5);
        event_play_action(ev002b_police_0, &ev002b_police_action_7,
                          &ev002b_police_texlist, 2.0F, EVENT_REPEAT, 0);
        event_sprite_create(ev002b_police_0, 2, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(2, 0.18F, 0.18F);
        event_wait(5);
        event_play_action(ev002b_police_1, &ev002b_police_action_7,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_sprite_create(ev002b_police_1, 3, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(3, 0.18F, 0.18F);
        event_wait(5);
        event_play_action(ev002b_police_2, &ev002b_police_action_4,
                          &ev002b_police_texlist, 1.8F, EVENT_REPEAT, 0);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_sprite_create(ev002b_police_2, 4, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(4, 0.18F, 0.18F);
        event_wait(5);
        event_play_action(ev002b_police_3, &ev002b_police_action_4,
                          &ev002b_police_texlist, 1.5F, EVENT_REPEAT, 0);
        event_sprite_create(ev002b_police_3, 5, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(5, 0.18F, 0.18F);
        event_wait(5);
        event_play_action(ev002b_police_4, &ev002b_police_action_7,
                          &ev002b_police_texlist, 4.0F, EVENT_REPEAT, 0);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_sprite_create(ev002b_police_4, 6, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(6, 0.18F, 0.18F);
        event_wait(5);
        event_play_action(ev002b_police_6, &ev002b_police_action_4,
                          &ev002b_police_texlist, 2.5F, EVENT_REPEAT, 0);
        event_voice_stop();
        event_sprite_create(ev002b_police_6, 7, 9.2F, 0.75F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(7, 0.18F, 0.18F);
        event_wait(5);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(10);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(10);
        event_wait(5);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(10);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(10);
        event_wait(5);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(10);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(5);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_wait(1);
        event_sprite_kill(0);
        event_sprite_kill(1);
        event_sprite_kill(2);
        event_sprite_kill(3);
        event_sprite_kill(4);
        event_sprite_kill(5);
        event_sprite_kill(6);
        event_sprite_kill(7);
        event_chaos0_kill();
        event_chaos0_hold_set_pos(268.0F, 3.1500006F, 300.0F);
        event_chaos0_hold_set_ang(0, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 250.48F, 7.27F, 344.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x039a, 0xeeca, 0x0900);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 287.24F, 7.83F, 327.21F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x059a, 0x1dc8, 0xf900);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 269.37F, 21.02F, 328.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf49a, 0x01c8, 0xf800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 269.04F, 18.81F, 320.99F);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 271.05F, 4.73F, 313.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0x0f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 273.92F, 5.31F, 320.94F);
        event_set_pos(ev002b_bullet_0, 267.0F, 2.51F, 303.0F);
        set_ang(ev002b_bullet_0, 0, 0xb000, 0);
        event_set_pos(ev002b_bullet_1, 267.0F, 2.51F, 304.0F);
        set_ang(ev002b_bullet_1, 0, 0xa000, 0);
        event_set_pos(ev002b_bullet_2, 268.0F, 2.51F, 301.5F);
        set_ang(ev002b_bullet_2, 0, 0xb900, 0);
        event_set_pos(ev002b_bullet_3, 268.0F, 2.51F, 302.5F);
        set_ang(ev002b_bullet_3, 0, 0xb500, 0);
        event_set_pos(ev002b_bullet_4, 267.5F, 2.51F, 301.0F);
        set_ang(ev002b_bullet_4, 0, 0xc000, 0);
        event_set_pos(ev002b_bullet_5, 268.1F, 2.51F, 303.0F);
        set_ang(ev002b_bullet_5, 0, 0xc800, 0);
        event_set_pos(ev002b_bullet_6, 268.15F, 2.51F, 302.0F);
        set_ang(ev002b_bullet_6, 0, 0xca00, 0);
        event_set_pos(ev002b_bullet_7, 268.2F, 2.51F, 301.0F);
        set_ang(ev002b_bullet_7, 0, 0xd000, 0);
        event_set_pos(ev002b_bullet_8, 268.3F, 2.51F, 300.0F);
        set_ang(ev002b_bullet_8, 0, 0xc900, 0);
        event_set_pos(ev002b_bullet_9, 268.5F, 2.51F, 300.0F);
        set_ang(ev002b_bullet_9, 0, 0xc000, 0);
        event_set_pos(ev002b_bullet_10, 272.0F, 2.51F, 307.0F);
        set_ang(ev002b_bullet_10, 0, 0xb000, 0);
        event_set_pos(ev002b_bullet_11, 271.5F, 2.51F, 306.0F);
        set_ang(ev002b_bullet_11, 0, 0xa000, 0);
        event_set_pos(ev002b_bullet_12, 271.0F, 2.51F, 305.5F);
        set_ang(ev002b_bullet_12, 0, 0xd000, 0);
        event_set_pos(ev002b_bullet_13, 271.0F, 2.51F, 307.0F);
        set_ang(ev002b_bullet_13, 0, 0xb500, 0);
        event_set_pos(ev002b_bullet_14, 272.0F, 2.51F, 305.0F);
        set_ang(ev002b_bullet_14, 0, 0xc000, 0);
        event_set_pos(ev002b_bullet_15, 264.0F, 2.51F, 308.0F);
        set_ang(ev002b_bullet_15, 0, 0xc800, 0);
        event_set_pos(ev002b_bullet_16, 267.0F, 2.51F, 307.0F);
        set_ang(ev002b_bullet_16, 0, 0xca00, 0);
        event_set_pos(ev002b_bullet_17, 264.5F, 2.51F, 306.0F);
        set_ang(ev002b_bullet_17, 0, 0xd000, 0);
        event_set_pos(ev002b_bullet_18, 264.0F, 2.51F, 306.0F);
        set_ang(ev002b_bullet_18, 0, 0xc900, 0);
        event_set_pos(ev002b_bullet_19, 264.0F, 2.51F, 305.5F);
        set_ang(ev002b_bullet_19, 0, 0xc000, 0);
        event_kill_task(&ev002b_police_0);
        event_kill_task(&ev002b_police_4);
        event_play_action(ev002b_bullet_13, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_17, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_18, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_8, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_1, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_wait(5);
        event_play_action(ev002b_bullet_7, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_3, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_wait(2);
        event_play_action(ev002b_bullet_15, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_16, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_4, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(3);
        event_play_action(ev002b_bullet_11, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_12, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_2, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_6, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(2);
        event_play_action(ev002b_bullet_5, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_14, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_10, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(5);
        event_play_action(ev002b_bullet_19, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_0, &ev002b_bullet_action_0,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        event_play_action(ev002b_bullet_9, &ev002b_bullet_action_1,
                          &ev002b_bullet_texlist, 1.3F, EVENT_REPEAT, 0);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(2);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(3);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(2);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(3);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(3);
        sound_play_at(1234, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(2);
        sound_play_at(1234, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(3);
        sound_play_at(1234, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(2);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(3);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(2);
        sound_play_at(1234, 0, 0, 110, 280.99F, 9.4595F, sound_z);
        event_wait(2);
        sound_play_at(1234, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_wait(3);
        sound_play_at(1234, 0, 0, 120, 280.99F, 9.4595F, sound_z);
        event_wait(5);
        event_wait(40);
        event_kill_task(&ev002b_bullet_0);
        event_kill_task(&ev002b_bullet_1);
        event_kill_task(&ev002b_bullet_2);
        event_kill_task(&ev002b_bullet_3);
        event_kill_task(&ev002b_bullet_4);
        event_kill_task(&ev002b_bullet_5);
        event_kill_task(&ev002b_bullet_6);
        event_kill_task(&ev002b_bullet_7);
        event_kill_task(&ev002b_bullet_8);
        event_kill_task(&ev002b_bullet_9);
        event_kill_task(&ev002b_bullet_10);
        event_kill_task(&ev002b_bullet_11);
        event_kill_task(&ev002b_bullet_12);
        event_kill_task(&ev002b_bullet_13);
        event_kill_task(&ev002b_bullet_14);
        event_kill_task(&ev002b_bullet_15);
        event_kill_task(&ev002b_bullet_16);
        event_kill_task(&ev002b_bullet_17);
        event_kill_task(&ev002b_bullet_18);
        event_kill_task(&ev002b_bullet_19);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 267.59F, 4.45F, 349.87F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0840, 0x88e0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 262.64F, 1.78F, 327.48F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0x0840, 0x88e0, 0);
        event_play_action(ev002b_police_1, &ev002b_police_action_6,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_3, &ev002b_police_action_3,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev002b_police_2, &ev002b_police_action_9,
                          &ev002b_police_texlist, 1.0F, 0, 6);
        event_play_action(ev002b_police_2, &ev002b_police_action_8,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 6);
        sound_play_at_for(1230, EVENT_SOUND_ID + 4, 1, 80, 280.99F, 9.4595F,
                          sound_z, 60);
        event_message_show(TEXT_4);
        event_wait(10);
        event_voice_play(939);
        event_wait(20);
        event_move(ev002b_police_2, 280.99F, 9.4595F, 382.94F, 280.99F, 9.4595F,
                   393.0F, 45);
        event_clear_motions(ev002b_police_1);
        event_play_action(ev002b_police_1, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.2F, EVENT_REPEAT, 0);
        event_play_path(ev002b_police_1, &ev002b_retreat_path_0, 0.1F, 0);
        event_clear_motions(ev002b_police_3);
        event_play_action(ev002b_police_3, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_path(ev002b_police_3, &ev002b_retreat_path_1, 0.2F, 0);
        event_voice_play(940);
        event_wait(25);
        sound_stop_id(EVENT_SOUND_ID + 4);
        event_message_show(TEXT_5);
        event_wait(20);
        event_clear_paths(ev002b_police_3);
        event_kill_task(&ev002b_police_3);
        event_clear_motions(ev002b_police_5);
        event_play_action(ev002b_police_5, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.2F, EVENT_REPEAT, 0);
        event_play_path(ev002b_police_5, &ev002b_retreat_path_2, 0.1F, 0);
        event_move_stop(ev002b_police_2);
        event_play_action(ev002b_police_2, &ev002b_police_action_10,
                          &ev002b_police_texlist, 2.0F, 0, 6);
        event_play_action(ev002b_police_2, &ev002b_police_action_10,
                          &ev002b_police_texlist, 2.0F, 0, 6);
        event_play_action(ev002b_police_2, &ev002b_police_action_8,
                          &ev002b_police_texlist, 1.0F, EVENT_REPEAT, 6);
        event_wait(30);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_sprite_create(ev002b_police_2, 0, 9.2F, 4.25F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(0, 0.18F, 0.18F);
        event_wait(10);
        event_sprite_kill(0);
        sound_play_at(1231, 0, 0, 100, 280.99F, 9.4595F, sound_z);
        event_sprite_create(ev002b_police_2, 1, 9.2F, 4.25F, 1.0F,
                            event_sprite_explosion_texlist);
        event_sprite_set_scale(1, 0.18F, 0.18F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 248.06F, 8.09F, 261.61F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0440, 0x88e0, 0);
        event_clear_motions(ev002b_police_6);
        event_play_action(ev002b_police_6, &ev002b_police_action_0,
                          &ev002b_police_texlist, 1.2F, EVENT_REPEAT, 0);
        event_play_path(ev002b_police_6, &ev002b_retreat_path_3, 0.1F, 0);
        event_wait(20);
        event_sprite_kill(1);
        event_move(ev002b_police_2, 280.99F, 9.4595F, 393.0F, 280.99F, 9.4595F,
                   403.0F, 20);
        event_wait(20);
        event_voice_stop();
        event_message_close();
        event_clear_paths(ev002b_police_1);
        event_kill_task(&ev002b_police_1);
        event_clear_paths(ev002b_police_5);
        event_kill_task(&ev002b_police_5);
        event_move_stop(ev002b_police_2);
        event_play_action(ev002b_police_2, &ev002b_police_action_9,
                          &ev002b_police_texlist, 2.0F, EVENT_REPEAT, 6);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 246.34F, 7.25F, 253.84F);
        event_wait(30);
        event_clear_paths(ev002b_police_6);
        event_kill_task(&ev002b_police_6);
        event_set_pos(event_get_player(0), 289.66F, 50.0F, 435.25F);
        set_ang(event_get_player(0), 0, 0x83bb, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 279.76F, 19.53F, 433.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe30, 0x044d, 0);
        event_kill_task(&ev002b_police_2);
        event_voice_play(937);
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, 283.16F, 21.46F, 465.94F);
        event_wait(30);
        event_voice_play(941);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 283.89F, 21.64F, 473.9F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_wait(30);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 265.07F, 3.82F, 389.02F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c15, 0x96b6, 0xfb00);
        event_camera_set_fov(EVENT_EASE_LINEAR, 20, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 282.59F, 16.89F, 426.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x1015, 0x9bb6, 0xfb00);
        event_message_show(TEXT_6);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 284.91F, 18.43F, 429.7F);
        event_wait(25);
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_voice_stop();
        event_message_close();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 20, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 267.92F, 5.19F, 379.61F);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev002b_run);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 252.02F, 17.26F, 397.67F);
        event_wait(30);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xf718, 0xf0f1, 0);
        event_wait(20);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 248.14F, 19.65F, 407.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xf718, 0xf0f1, 0);
        event_wait(45);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x11c7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 236.01F, 10.18F, 225.98F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0118, 0x8cf1, 0);
        event_set_pos(event_get_player(0), 268.0F, 0.0F, 377.0F);
        set_ang(event_get_player(0), 0x0209, 0x813a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 239.75F, 10.5F, 237.37F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 260.31F, 6.67F, 354.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xfe18, 0x8ef1, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 262.46F, 6.4F, 360.09F);
        event_wait(60);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_reset_player(0);
        event_chaos0_kill();
        event_chaos0_hold_release();
        event_kill_task(&ev002b_police_0);
        event_kill_task(&ev002b_police_1);
        event_kill_task(&ev002b_police_2);
        event_kill_task(&ev002b_police_3);
        event_kill_task(&ev002b_police_4);
        event_kill_task(&ev002b_police_5);
        event_kill_task(&ev002b_police_6);
        event_clear_paths(ev002b_police_1);
        event_clear_paths(ev002b_police_3);
        event_clear_paths(ev002b_police_5);
        event_clear_paths(ev002b_police_6);
        event_kill_task(&ev002b_bullet_0);
        event_kill_task(&ev002b_bullet_1);
        event_kill_task(&ev002b_bullet_2);
        event_kill_task(&ev002b_bullet_3);
        event_kill_task(&ev002b_bullet_4);
        event_kill_task(&ev002b_bullet_5);
        event_kill_task(&ev002b_bullet_6);
        event_kill_task(&ev002b_bullet_7);
        event_kill_task(&ev002b_bullet_8);
        event_kill_task(&ev002b_bullet_9);
        event_kill_task(&ev002b_bullet_10);
        event_kill_task(&ev002b_bullet_11);
        event_kill_task(&ev002b_bullet_12);
        event_kill_task(&ev002b_bullet_13);
        event_kill_task(&ev002b_bullet_14);
        event_kill_task(&ev002b_bullet_15);
        event_kill_task(&ev002b_bullet_16);
        event_kill_task(&ev002b_bullet_17);
        event_kill_task(&ev002b_bullet_18);
        event_kill_task(&ev002b_bullet_19);
        event_sprite_kill(0);
        event_sprite_kill(1);
        event_sprite_kill(2);
        event_sprite_kill(3);
        event_sprite_kill(4);
        event_sprite_kill(5);
        event_sprite_kill(6);
        event_sprite_kill(7);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
