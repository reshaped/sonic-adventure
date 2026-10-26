/* EV001D, Sonic's story: Eggman escapes in the Egg Mobile after Chaos 6's
 * defeat. Knuckles arrives late, and Sonic jumps after Eggman. Sonic is player
 * 0 and Knuckles player 2, as event_get_player numbers them */
#include <ninja.h>

#include "ev001d/eggmobile_action.h"
#include "ev001d/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/knuckles.h"
#include "player/textures.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 26
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 31
#define TEXT_4_LENGTH 17
#define TEXT_5_LENGTH 23
#define TEXT_6_LENGTH 22
#define TEXT_7_LENGTH 3
#define TEXT_8_LENGTH 4
#define TEXT_9_LENGTH 1
#include "ev001d/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_run_action;

extern NJS_TEXLIST event_eggmobile_texlist;

/* Sonic's two jumps after the Egg Mobile, one key a step. A is held for the 3rd
 * to 13th keys, and for the 29th to 40th. the stick is pushed at most 1 out of
 * 255 until the last three keys. those push it 246, 130 and 27 */
static const struct event_pad_key ev001d_jump_keys[63] = {
#include "ev001d/script/ev001d_jump_keys.inc"
};

static const struct event_pad_recording ev001d_jumps = {
    ev001d_jump_keys, sizeof(ev001d_jump_keys) / sizeof(ev001d_jump_keys[0])};

static struct task *ev001d_floor = NULL;
static struct task *ev001d_fade = NULL;

struct texture_pvm ev001d_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"KNUCKLES", &knuckles_texlist},
    {NULL, NULL},
};

void ev001d_script(int phase)
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
        stream_request_music(MUSIC_EGCARER1);
        event_set_pos(event_get_player(0), -44.9F, 745.65F, -231.87F);
        event_set_ang(event_get_player(0), 0x02ce, 0x725e, 0x0119);
        event_create_player(2, knuckles_step, -113.79F, 743.72F, -510.44F,
                            0xfcd3, 0xf5ea, 0xffff);
        event_eggmobile_create(0.0F, 780.0F, -386.04F, 0, 0x4000, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), &ev001d_eggmobile_action,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        sound_play_at_for(1227, EVENT_SOUND_ID, 1, 100, 0.0F, 780.0F, -386.04F,
                          600);
        event_wait(1);
        event_eggmobile_set_bob(0x0100, 0.5F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -24.51F, 754.06F, -310.82F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfcca, 0x78e5, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -77.29F, 749.43F,
                             -184.83F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 60, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_wait(100);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0x03ca, 0xebe5, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, -82.48F, 748.77F, -176.33F);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -21.03F, 780.5F, -342.65F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x07ca, 0xe8ad, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, -27.75F, 780.5F, -346.99F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x07ca, 0xe5a5, 0);
        event_set_pos(event_get_player(2), 139.57F, 743.65F, -265.68F);
        event_set_ang(event_get_player(2), 0x01cb, 0xbff7, 0xfd71);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &knuckles_run_action,
                          &knuckles_texlist, 2.5F, EVENT_REPEAT, 0);
        event_voice_play(1119);
        event_message_show(TEXT_1);
        event_play_action(event_eggmobile_get_task(), &ev001d_eggmobile_action,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 8);
        event_wait(40);
        event_message_close();
        event_run_to_speed(event_get_player(2), 21.97F, 747.63F, -253.9F, 1.75F,
                           0.06F);
        event_wait(30);
        sound_stop_id(EVENT_SOUND_ID);
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 116.31F, 752.72F, -322.3F);
        event_voice_play(1649);
        event_message_wait(30, TEXT_2);
        event_message_close();
        event_run_wait(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(2), 0x0317, 0x8772, 0);
        event_look_at(event_get_player(2), 0.0F, 803.0F, -387.04F);
        event_wait(15);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F,
                           10.0F, 0.0F);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 15, 51.72F, 751.31F, -278.45F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 15, 0xff50, 0x57db, 0);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_wait(10);
        event_voice_play(1120);
        event_message_show(TEXT_3);
        event_wait(35);
        event_talk_stop(event_get_player(0));
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 45, -27.59F, 750.82F, -240.23F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 45, 0xfe50, 0x50db, 0);
        event_message_show(TEXT_4);
        event_talk_start(event_get_player(0), TEXT_8);
        event_wait(60);
        event_talk_stop(event_get_player(0));
        event_wait(5);
        event_talk_start(event_get_player(0), TEXT_8);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_9);
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_message_close();
        event_wait(5);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(5);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 18.65F, 778.13F, -373.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f94, 0x2e0c, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 69.36F, 771.59F, -342.66F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0d94, 0x2c0c, 0xfd00);
        event_look_clear(event_get_player(0));
        event_voice_play(1124);
        event_eggmobile_fly_turn(0.0F, 785.0F, -386.04F, 0, -0x4000, 0, 50, 0);
        event_message_wait(45, TEXT_5);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, 73.58F, 769.95F, -340.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 35, 0x0d94, 0x2d0c, 0xfd00);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_eggmobile_fly_turn(-300.0F, 755.0F, -400.0F, 0, 0, 0, 90, 1);
        sound_play_at(1231, 0, 0, 120, 0.0F, 780.0F, -386.04F);
        event_wait(25);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -35.05F, 749.3F, -230.52F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0194, 0x2b0c, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -22.0F, 748.72F, -223.14F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.75F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -217.33F, 748.34F, -350.01F,
                           1.75F, 0.06F);
        event_voice_play(1123);
        event_message_wait(45, TEXT_6);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -233.18F, 750.1F, -345.83F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_set_pos(event_get_player(0), -175.43F, 743.18F, -321.23F);
        event_run_to_speed(event_get_player(0), -217.33F, 748.34F, -350.01F,
                           1.75F, 0.06F);
        event_wait(10);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev001d_jumps);
        event_wait(40);
        event_eggmobile_fly_turn(-600.0F, 300.0F, -500.0F, 0, -0x0500, 0, 120,
                                 1);
        event_wait(20);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -233.08F, 753.91F, -382.2F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_set_pos(event_get_player(0), -217.33F, 748.34F, -350.01F);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev001d_jumps);
        event_wait(60);
        event_camera_look_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -447.75F, 635.0F, -436.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xdcf8, 0x3180, 0);
        ev001d_floor = event_floor_create(event_get_player(0), 600.0F);
        event_set_pos(event_get_player(0), -400.0F, 600.0F, -420.0F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev001d_jumps);
        event_wait(20);
        EVENT_KILL(ev001d_floor);
        ev001d_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        EVENT_KILL(ev001d_floor);
        /* @hack spelling: the kill and the clear as one statement, inside the
         * if. SHC then reads ev001d_fade again in task_kill's delay slot, at
         * 0x0cb80b3c. it loads -386.04 into FR14 at 0x0cb800c4, and 6.0 into
         * FR13 at 0x0cb8025c. as two statements, SHC leaves the slot a NOP, the
         * two constants trade registers, and 35 of 1043 words differ. as
         * EVENT_KILL, the form of ev001d_floor's kill above, only the constants
         * trade, and 17 of 1043 words differ */
        if (ev001d_fade != NULL) {
            task_kill(ev001d_fade), ev001d_fade = NULL;
        }
        event_reset_player(0);
        event_kill_player(2);
        event_eggmobile_kill();
        event_set_pos(event_get_player(0), -44.9F, 745.65F, -231.87F);
        event_set_ang(event_get_player(0), 0x02ce, 0x725e, 0x0119);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
