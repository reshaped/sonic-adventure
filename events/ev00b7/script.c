/* EV00B7, E-102's story: Eggman sends the E-Series after Froggy */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_lift.h"
#include "ev00b7/dim.h"
#include "ev00b7/frog_screen.h"
#include "ev00b7/script.h"
#include "ev00b7/spotlight.h"
#include "event/event.h"
#include "event/play.h"
#include "player/dat_8c5a644c.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/e103.h"
#include "shared/e104.h"
#include "shared/e105.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 75
#define TEXT_4_LENGTH 45
#define TEXT_5_LENGTH 29
#define TEXT_6_LENGTH 62
#define TEXT_7_LENGTH 26
#define TEXT_8_LENGTH 59
#define TEXT_9_LENGTH 18
#define TEXT_10_LENGTH 20
#include "ev00b7/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_run_action;
extern NJS_ACTION eggman_stand_action;
/* two more of Eggman's actions */
extern NJS_ACTION dat_8c59d224;
extern NJS_ACTION dat_8c5a0b78;

extern NJS_TEXLIST ev00b7_frog_texlist;
extern NJS_TEXLIST event_e103_texlist;
extern NJS_TEXLIST event_e104_texlist;
extern NJS_TEXLIST event_e105_texlist;
extern NJS_ACTION ev00b7_e102_action;
extern NJS_ACTION ev00b7_eggman_action;
extern NJS_ACTION ev00b7_e103_action_0;
extern NJS_ACTION ev00b7_e103_action_1;
extern NJS_ACTION ev00b7_e103_action_2;
extern NJS_ACTION ev00b7_e103_action_3;
extern NJS_ACTION ev00b7_e103_action_4;
extern NJS_ACTION ev00b7_e104_action_0;
extern NJS_ACTION ev00b7_e104_action_1;
extern NJS_ACTION ev00b7_e104_action_2;
extern NJS_ACTION ev00b7_e104_action_3;
extern NJS_ACTION ev00b7_e104_action_4;
extern NJS_ACTION ev00b7_e105_action_0;
extern NJS_ACTION ev00b7_e105_action_1;
extern NJS_ACTION ev00b7_e105_action_2;
extern NJS_ACTION ev00b7_e105_action_3;
extern NJS_ACTION ev00b7_e105_action_4;

static struct task *ev00b7_spotlight_0 = NULL;
static struct task *ev00b7_spotlight_1 = NULL;
static struct task *ev00b7_floor = NULL;
/* no code uses this word. it keeps ev00b7_fade at its address in the
 * image */
static struct task *dat_0cb83a5c = NULL;
static struct task *ev00b7_fade = NULL;
static struct task *ev00b7_e103 = NULL;
static struct task *ev00b7_e104 = NULL;
static struct task *ev00b7_e105 = NULL;

void ev00b7_script(int phase)
{
    /* @hack spelling: the height E-102 stands at through a local, which
     * SHC keeps in FR12 from 0x0cb8008c on; as a literal at each use, the
     * code runs 16 bytes short and 974 of 1331 words differ */
    Float e102_y;
    /* @hack spelling: E-104's slot through a pointer, which SHC keeps in R9
     * from 0x0cb80172 on; by name, the code runs 12 bytes long and 1084 of 1331
     * words differ */
    struct task **e104_slot;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_00B7.mlt");
        event_sound_init(3);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2ccd);
        event_flags[FLAG_E102_EC_EGG_LIFT] = 1;
        event_flags[FLAG_LIFT] = LIFT_LOW_NOW;
        e102_y = 8.16F;
        event_set_pos(event_get_player(0), 10.5F, e102_y, -210.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_task(&ev00b7_e103, event_e103_create, -10.5F, e102_y,
                         -210.0F, 0, 0x8000, 0);
        event_place_task(&ev00b7_e104, event_e104_create, -10.5F, e102_y,
                         -200.0F, 0, 0x8000, 0);
        event_place_task(&ev00b7_e105, event_e105_create, -0.5F, e102_y,
                         -200.0F, 0, 0x8000, 0);
        event_wait(2);
        event_set_flags(ev00b7_e103, EVENT_IGNORE_GROUND);
        event_set_pos(ev00b7_e103, -10.5F, 21.66F, -210.0F);
        event_set_ang(ev00b7_e103, 0, 0x8000, 0);
        event_clear_motions(ev00b7_e103);
        event_play_action(ev00b7_e103, &ev00b7_e103_action_2,
                          &event_e103_texlist, 1.0F, EVENT_REPEAT, 0);
        e104_slot = &ev00b7_e104;
        event_set_flags(*e104_slot, EVENT_IGNORE_GROUND);
        event_set_pos(*e104_slot, 32.5F, 24.66F, -210.0F);
        event_set_ang(*e104_slot, 0, 0x8000, 0);
        event_clear_motions(*e104_slot);
        event_play_action(*e104_slot, &ev00b7_e104_action_2,
                          &event_e104_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_set_flags(ev00b7_e105, EVENT_IGNORE_GROUND);
        event_set_pos(ev00b7_e105, -32.5F, 24.66F, -210.0F);
        event_set_ang(ev00b7_e105, 0, 0x8000, 0);
        event_clear_motions(ev00b7_e105);
        event_play_action(ev00b7_e105, &ev00b7_e105_action_2,
                          &event_e105_texlist, 1.0F, EVENT_REPEAT, 0);
        /* Eggman stands on a floor of his own, 55 up */
        event_create_player(6, eggman_step, 0.0F, 55.0F, -320.44F, 0, 0, 0);
        event_wait(1);
        ev00b7_floor = event_floor_create(event_get_player(6), 55.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0x5c00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 121.25F, 35.1F, 32.2F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 205, 146.9F, 14.32F, -215.4F);
        stream_request_music(MUSIC_EVTBGM05);
        event_wait(140);
        event_set_pos(event_get_player(6), 0.0F, 55.0F, -320.44F);
        event_set_ang(event_get_player(6), 0, 0, 0);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        ev00b7_dim_create();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0800, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 163.17F, -28.04F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 205, 0x0800, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 205, 0.0F, 4.29F, -59.64F);
        event_wait(230);
        ev00b7_spotlight_0 = ev00b7_spotlight_create(65.0F, 145.0F, -250.44F);
        ev00b7_spotlight_1 = ev00b7_spotlight_create(-65.0F, 145.0F, -250.44F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0xf900, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, 0.0F, 48.71F, -144.78F);
        event_wait(150);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x31c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 0.0F, 57.7F, -290.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0d00, 0, 0);
        event_wait(90);
        /* both spotlights turn to Eggman as the dimming ends */
        ev00b7_spotlight_aim(ev00b7_spotlight_0, 0.0F, 55.0F, -320.44F);
        ev00b7_spotlight_aim(ev00b7_spotlight_1, 0.0F, 55.0F, -320.44F);
        ev00b7_dim_kill();
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 0);
        event_wait_motions(event_get_player(6));
        stream_stop_music();
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        stream_request_music(MUSIC_THEE102);
        event_wait(30);
        event_turn_by(event_get_player(6), 0, -0x3000, 0);
        event_voice_play(1705);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_voice_play(1706);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_turn_by(event_get_player(6), 0, -0x3200, 0);
        event_wait(15);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c5a0b78, &eggman_texlist,
                          1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -0.57F, 3.45F, -149.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1400, 0xff00, 0);
        event_voice_play(1707);
        event_message_show(TEXT_3);
        event_wait(25);
        event_wait_motions(event_get_player(6));
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 0);
        event_wait_motions(event_get_player(6));
        event_voice_wait();
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1200, 0x0b00, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 3.4334F, 64.457F, -311.66F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 75, 3.5F, 65.36F, -311.24F);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 0);
        event_voice_play(1708);
        event_message_show(TEXT_4);
        event_wait(10);
        event_wait_motions(event_get_player(6));
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(55);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1200, 0xf900, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -16.38F, 50.86F, -278.95F);
        event_voice_play(1709);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_sound_play(1223, 0, 0, 0);
        ev00b7_frog_screen_create();
        event_wait(25);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 95, 0x1a00, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 95, 0.0F, 86.3F, -356.74F);
        event_voice_play(1710);
        event_message_show(TEXT_6);
        event_voice_wait();
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xed00, 0x5f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -4.95F, 33.635F, -234.74F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 31.08F, 31.02F, -234.5F);
        event_wait(30);
        event_voice_play(1711);
        event_message_show(TEXT_7);
        event_wait(100);
        event_voice_play(1712);
        event_message_show(TEXT_8);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, 37.48F, 35.95F, -232.33F);
        event_wait(60);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.3F, 0, 16);
        event_play_action(ev00b7_e103, &ev00b7_e103_action_3,
                          &event_e103_texlist, 1.3F, 0, 16);
        event_play_action(*e104_slot, &ev00b7_e104_action_3,
                          &event_e104_texlist, 1.3F, 0, 16);
        event_play_action(ev00b7_e105, &ev00b7_e105_action_3,
                          &event_e105_texlist, 1.3F, 0, 16);
        event_wait(16);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 0.8F, EVENT_REPEAT, 16);
        event_play_action(ev00b7_e103, &ev00b7_e103_action_4,
                          &event_e103_texlist, 0.8F, EVENT_REPEAT, 16);
        event_play_action(*e104_slot, &ev00b7_e104_action_4,
                          &event_e104_texlist, 0.8F, EVENT_REPEAT, 16);
        event_play_action(ev00b7_e105, &ev00b7_e105_action_4,
                          &event_e105_texlist, 0.8F, EVENT_REPEAT, 16);
        event_wait(20);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1400, 0x0f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 30.65F, 9.38F, -172.96F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x238e);
        event_wait(35);
        event_voice_play(1713);
        event_message_show(TEXT_9);
        event_wait(20);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 66.0F, -303.82F);
        event_play_action(event_get_player(6), &ev00b7_eggman_action,
                          &eggman_texlist, 1.0F, 0, 0);
        event_message_show(TEXT_10);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 70, 0x18e4);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 75, 0x0200, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 75, 0.0F, 18.8F, -46.34F);
        event_wait_motions(event_get_player(6));
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &dat_8c59d224, &eggman_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(5);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00b7_e102_action,
                          &e102_texlist, 1.0F, 0, 16);
        event_clear_motions(ev00b7_e103);
        event_play_action(ev00b7_e103, &ev00b7_e103_action_0,
                          &event_e103_texlist, 1.0F, 0, 16);
        event_clear_motions(*e104_slot);
        event_play_action(*e104_slot, &ev00b7_e104_action_0,
                          &event_e104_texlist, 1.0F, 0, 16);
        event_clear_motions(ev00b7_e105);
        event_play_action(ev00b7_e105, &ev00b7_e105_action_0,
                          &event_e105_texlist, 1.0F, 0, 16);
        event_wait(40);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xed00, 0x7e00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 4.05F, 75.4F, -330.2F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_ang(ev00b7_e103, 0, 0, 0);
        event_set_ang(*e104_slot, 0, 0, 0);
        event_set_ang(ev00b7_e105, 0, 0, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          1.3F, EVENT_REPEAT, 16);
        event_clear_motions(ev00b7_e103);
        event_play_action(ev00b7_e103, &ev00b7_e103_action_1,
                          &event_e103_texlist, 1.3F, EVENT_REPEAT, 16);
        event_clear_motions(*e104_slot);
        event_play_action(*e104_slot, &ev00b7_e104_action_1,
                          &event_e104_texlist, 1.3F, EVENT_REPEAT, 16);
        event_clear_motions(ev00b7_e105);
        event_play_action(ev00b7_e105, &ev00b7_e105_action_1,
                          &event_e105_texlist, 1.3F, EVENT_REPEAT, 16);
        event_wait(16);
        event_move(event_get_player(0), 10.5F, e102_y, -210.0F, 10.0F, -5.0F,
                   -34.0F, 80);
        event_move(*e104_slot, 32.5F, 24.96F, -210.0F, 30.0F, 8.15F, -34.0F,
                   80);
        event_move(ev00b7_e103, -10.5F, 21.66F, -210.0F, -10.0F, 8.15F, -34.0F,
                   80);
        event_move(ev00b7_e105, -32.5F, 24.96F, -210.0F, -30.0F, 8.15F, -34.0F,
                   80);
        event_wait(10);
        ev00b7_fade = event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_message_close();
        event_wait(60);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        EVENT_KILL(ev00b7_floor);
        event_reset_player(0);
        event_kill_task(&ev00b7_e103);
        event_kill_task(&ev00b7_e104);
        event_kill_task(&ev00b7_e105);
        event_kill_player(6);
        ev00b7_spotlight_kill(ev00b7_spotlight_0);
        ev00b7_spotlight_kill(ev00b7_spotlight_1);
        ev00b7_frog_screen_kill();
        event_sound_stop_all();
        ev00b7_dim_kill();
        EVENT_KILL(ev00b7_fade);
        event_flags[FLAG_E102_EC_EGG_LIFT] = 0;
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b7_textures[] = {
    {"FROG", &ev00b7_frog_texlist},
    {"EGGMAN", &eggman_texlist},
    {"EV_E103_BODY", &event_e103_texlist},
    {"EV_E104_BODY", &event_e104_texlist},
    {"EV_E105_BODY", &event_e105_texlist},
    {"EV_SPOTLIGHT", &ev00b7_spotlight_texlist},
    {NULL, NULL},
};
