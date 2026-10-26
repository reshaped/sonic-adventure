/* EV00FA: Sonic and Tails find the crashed Tornado 2, and its emerald is
 * taken as water bursts up beside it */
#include <ninja.h>

#include "ev00fa/script.h"
#include "ev00fa/sonic_action_1.h"
#include "ev00fa/splash.h"
#include "ev00fa/tornado_object.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 23
#define TEXT_2_LENGTH 34
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 47
#define TEXT_5_LENGTH 25
#define TEXT_6_LENGTH 4
#define TEXT_7_LENGTH 1
#define TEXT_8_LENGTH 3
#include "ev00fa/script/text.inc"

/* the crashed Tornado 2's prop slot */
#define TORNADO 0

extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_skid_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_skid_action;

extern NJS_ACTION ev00fa_sonic_action_0;
extern NJS_ACTION ev00fa_tails_action_0;
extern NJS_ACTION ev00fa_tails_action_1;
extern NJS_ACTION ev00fa_tails_action_2;
extern NJS_ACTION ev00fa_tails_action_3;
extern NJS_ACTION ev00fa_tails_action_4;

/* a walk that nothing plays, as pad keys a step each. the stick is pushed
 * about 164 of 255, two thirds of the way, for the 50 keys from key 10, with
 * no button held */
static const struct event_pad_key ev00fa_walk_keys[70] = {
#include "ev00fa/script/ev00fa_walk_keys.inc"
};

static const struct event_pad_recording ev00fa_walk = {
    ev00fa_walk_keys, sizeof(ev00fa_walk_keys) / sizeof(ev00fa_walk_keys[0])};

/* the Tornado 2's emerald, the root's third child, which
 * ev00fa_emerald_hide takes off ev00fa_tornado_object's tree. null until
 * then */
static NJS_OBJECT *ev00fa_emerald;

static void ev00fa_emerald_hide(void)
{
    ev00fa_emerald = ev00fa_tornado_object.child->sibling->sibling;
    ev00fa_tornado_object.child->sibling->sibling = NULL;
    /* @hack redundant: the return keeps ev00fa_emerald_hide a function of its
     * own, which ev00fa_script calls by the BSR at 0x0cb806cc. without it,
     * SHC expands the call inline and omits the function, and 11 of 11 words
     * differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void ev00fa_emerald_show(void)
{
    /* @hack spelling: child as (&ev00fa_tornado_object)->child. SHC then loads
     * the object's address from the pool word ev00fa_emerald_hide uses
     * (0x0cb80064) and reads child 44 bytes past it (MOV.L @(44,R3),R2 at
     * 0x0cb80020). as ev00fa_tornado_object.child, a pool word of its own
     * holds child's address, and 5 of 7 words differ */
    (&ev00fa_tornado_object)->child->sibling->sibling = ev00fa_emerald;
}

static struct task *ev00fa_fade = NULL;

void ev00fa_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00fa.mlt");
        event_sound_init(3);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_set_pos(event_get_player(0), 1114.53F, 100.866F, -1037.79F);
        event_set_ang(event_get_player(0), 0xfd57, 0x2bb4, 0x0280);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(2, tails_step, 1109.32F, 101.0F, -1032.6F, 0xfce3,
                            0x7400, 0x0287);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &tails_run_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_prop_create(1226.0F, 99.0F, -904.66F, 0x1000, 0xd000, 0x0400,
                          1.0F, 1.0F, 1.0F, &ev00fa_tornado_object,
                          &ev00fa_tornado_texlist, TORNADO);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x9000, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1122.53F, 106.466F,
                             -1043.3357F);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0300, 0x9500, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 1130.67F, 108.2F,
                             -1039.86F);
        event_run_to_speed(event_get_player(0), 1139.7F, 103.658F, -1019.255F,
                           1.0F, 1.0F);
        event_run_to_speed(event_get_player(2), 1132.87F, 103.04F, -1013.1F,
                           1.0F, 0.9F);
        event_wait(20);
        event_play_action(event_get_player(0), &sonic_skid_action,
                          &sonic_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(2), &tails_skid_action,
                          &tails_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(2), &ev00fa_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 1131.54F, 108.2F,
                             -1040.355F);
        event_voice_play(1975);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf700, 0xad00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1206.0873F, 98.1867F,
                             -908.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf700, 0xbc00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 1205.3F, 98.1867F,
                             -901.21F);
        event_voice_play(1976);
        event_message_show(TEXT_2);
        event_clear_motions(event_get_player(0));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &tails_run_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_run_to_speed(event_get_player(0), 1178.0F, 97.6F, -968.27F, 1.0F,
                           1.0F);
        event_run_to_speed(event_get_player(2), 1165.44F, 99.6F, -967.0F, 1.0F,
                           0.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0x9a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1123.15F, 120.487F,
                             -1037.236F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0800, 0xa600, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 1166.96F, 99.4665F,
                             -974.5F);
        event_wait(50);
        event_sound_play(1224, 100, 0, 0);
        ev00fa_splash_create(1228.74F, 90.0F, -909.4F, 1.0F, 0.7F);
        event_wait(5);
        event_play_action(event_get_player(0), &ev00fa_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(2), &ev00fa_tails_action_4,
                          &tails_texlist, 1.0F, 0, 0);
        event_voice_play(995);
        event_message_show(TEXT_3);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x382e);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x1400, 0xa600, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 1171.59F, 98.44F, -971.43F);
        event_wait(50);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf600, 0x2300, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1255.168F, 123.57F, -899.8F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00fa_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0x3000, 0x0200);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1187.06F, 97.54F, -963.175F);
        event_wait(10);
        event_play_action(event_get_player(0), &ev00fa_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(2), &ev00fa_tails_action_2,
                          &tails_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(2), &ev00fa_tails_action_3,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(15);
        event_talk_start(event_get_player(0), TEXT_6);
        event_talk_start(event_get_player(2), TEXT_7);
        event_voice_play(1977);
        event_message_show(TEXT_4);
        event_wait(30);
        ev00fa_emerald_hide();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x5c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1198.92F, 100.9F, -900.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf300, 0xc200, 0x0200);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 35, 0x2000);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 35, 1206.5F, 98.4F, -900.934F);
        event_voice_wait();
        event_clear_motions(event_get_player(2));
        event_set_ang(event_get_player(2), 0, 0x3f00, 0);
        event_wait(10);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0d00, 0x2900, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1186.043F, 100.26F,
                             -961.48F);
        event_talk_start(event_get_player(2), TEXT_8);
        event_voice_play(1978);
        event_message_show(TEXT_5);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe77a, 0x20da, 0x0100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1224.55F, 160.75F, -911.7F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 255, 0x3111);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xe8fe, 0x2bdf, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 1467.97F, 487.98F,
                             -660.17F);
        event_wait(40);
        ev00fa_fade = event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(40);
        event_message_close();
        ev00fa_splash_kill();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), 1177.59F, 97.737F, -968.79F);
        event_set_ang(event_get_player(0), 0x02a7, 0x1afa, 0xfd1a);
        event_reset_player(0);
        event_kill_player(2);
        event_prop_kill(TORNADO);
        ev00fa_splash_kill();
        EVENT_KILL(ev00fa_fade);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00fa_textures[] = {
    {"TR2CRASH", &ev00fa_tornado_texlist},
    {"MILES", &tails_texlist},
    {"L_SIBUKI", &ev00fa_splash_texlist},
    {NULL, NULL},
};
