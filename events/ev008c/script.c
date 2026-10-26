/* EV008C, Knuckles' story: after Chaos 4, Eggman shows off the Egg Carrier and
 * rises toward it. Knuckles leaves Sonic and Tails to go his own way */

/* EV000C plays the same scene in Sonic's story. here Knuckles is player 0
 * and Sonic player 2 */
#include <ninja.h>

#include "ev008c/eggmobile_action.h"
#include "ev008c/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/beam.h"
#include "shared/eggmobile.h"
#include "shared/move.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 45
#define TEXT_2_LENGTH 44
#define TEXT_3_LENGTH 31
#define TEXT_4_LENGTH 25
#define TEXT_5_LENGTH 52
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 56
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 56
#define TEXT_11_LENGTH 25
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 4
#include "ev008c/script/text.inc"

/* the Egg Carrier's prop slot */
#define EGG_CARRIER 0

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_run_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_OBJECT ev008c_egg_carrier_object;
extern NJS_TEXLIST ev008c_egg_carrier_texlist;
extern NJS_ACTION ev008c_egg_carrier_action;
extern NJS_ACTION ev008c_sonic_action;
extern NJS_ACTION ev008c_tails_action;

static struct task *ev008c_beam = NULL;

void ev008c_script(int phase)
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
        event_set_pos(event_get_player(0), 102.5F, 72.17F, 212.0F);
        event_set_ang(event_get_player(0), 0xff96, 0x9000, 18);
        event_create_player(2, sonic_step, 73.0F, 72.02F, 232.51F, 0, 0, 0);
        event_create_player(3, tails_step, 37.3F, 72.02F, 213.32F, 0, 0x6d73,
                            0);
        event_eggmobile_create(80.0F, 80.0F, 160.0F, 0, 0x3b00, 0);
        event_prop_create(300.0F, 1200.0F, 1000.0F, 0, 0x8000, 0, 0.25F, 0.25F,
                          0.25F, &ev008c_egg_carrier_object,
                          &ev008c_egg_carrier_texlist, EGG_CARRIER);
        event_prop_set_action(EGG_CARRIER, &ev008c_egg_carrier_action, 0);
        ev008c_beam =
            event_beam_create(80.0F, 80.0F, 160.0F, 0x1700, 0, 0xf790);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev008c_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(), &ev008c_eggmobile_action,
                          &event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_eggmobile_set_bob(0x100, 0.5F);
        event_prop_play(EGG_CARRIER, 0, 1.0F, EVENT_PROP_FOREVER);
        event_beam_set_shape(ev008c_beam, 0.3F, 0.0F, 0.3F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev008c_beam, 0.0F, 10.0F, 0.0F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev008c_beam, 0.0F, 0.0F, 0.0F, 0.001F, 0.0F, 0);
        event_beam_set_shape(ev008c_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.001F, 0);
        event_beam_set_shape(ev008c_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_wait(1);
        stream_request_music(MUSIC_EGCARER1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -24.48F, 170.83F, 153.45F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1f00, 0x9500, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x3555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 60.09F, 81.34F, 217.39F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 125, 0x1ab4, 0x93e6, 0);
        event_wait(10);
        event_voice_play(1016);
        event_message_show(TEXT_1);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 63.38F, 72.22F, 223.58F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 200, 0x38e4);
        event_wait(50);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 76.3F, 92.6F, 164.71F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe84, 0xe2e6, 0);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 90, 0x3555);
        event_voice_play(1017);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_message_show(TEXT_2);
        event_wait(90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 53.61F, 74.32F, 172.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0284, 0x92e6, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 48.99F, 77.36F, 102.96F);
        event_wait(30);
        event_voice_play(1018);
        event_message_show(TEXT_3);
        event_wait(30);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 150,
                                  event_eggmobile_get_task(), 0.0F, 6.0F, 0.0F,
                                  0);
        event_wait(60);
        event_message_close();
        event_beam_set_mode(ev008c_beam, BEAM_OPEN);
        sound_play_at_for(1226, EVENT_SOUND_ID, 1, 120, 80.0F, 80.0F, 160.0F,
                          185);
        event_wait(30);
        /* the Egg Mobile rises toward the Egg Carrier */
        event_move_turn(event_eggmobile_get_task(), 80.0F, 80.0F, 160.0F,
                        235.71F, 807.02F, 643.87F, 0, 0x3b00, 0, 0x1500, 0x3b00,
                        0, 180);
        event_wait(150);
        event_move_stop(event_eggmobile_get_task());
        event_camera_look_clear();
        event_wait(5);
        event_eggmobile_kill();
        event_beam_set_mode(ev008c_beam, BEAM_CLOSE);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_look_at(event_get_player(2), 95.0F, 140.0F, 300.0F);
        event_set_pos(event_get_player(0), 103.96F, 72.47F, 254.33F);
        event_set_ang(event_get_player(0), 0xff96, 0xea20, 18);
        event_set_pos(event_get_player(3), 37.3F, 72.02F, 213.32F);
        event_set_ang(event_get_player(3), 0, 0x1500, 0);
        event_wait(30);
        event_move_turn(event_prop_get(EGG_CARRIER), 300.0F, 1200.0F, 1500.0F,
                        1000.0F, 2000.0F, 5000.0F, 0, 0x8000, 0, 0, 0x8000, 0,
                        400);
        sound_play_at(1231, 0, 0, 120, 73.0F, 72.02F, 232.51F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 22.96F, 76.32F, 152.91F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b84, 0x8da5, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 20.12F, 73.81F, 144.75F);
        event_wait(120);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 74.78F, 75.57F, 249.39F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0400, 0x0f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 79.49F, 75.87F, 243.41F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0400, 0x1c00, 0);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_look_at_task(event_get_player(3), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_12);
        event_wait(5);
        event_voice_play(1019);
        event_message_show(TEXT_4);
        event_wait(45);
        event_talk_stop(event_get_player(3));
        event_move_stop(event_prop_get(EGG_CARRIER));
        event_wait(10);
        event_prop_kill(EGG_CARRIER);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_12);
        event_wait(5);
        event_voice_play(1020);
        event_message_show(TEXT_5);
        event_wait(55);
        event_look_clear(event_get_player(2));
        event_play_action(event_get_player(2), &ev008c_sonic_action,
                          &sonic_texlist, 0.4F, 0, 4);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_wait(5);
        event_message_show(TEXT_6);
        event_wait(5);
        event_talk_stop(event_get_player(3));
        event_voice_play(1021);
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(20);
        event_voice_play(1599);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 65, 105.57F, 77.51F, 268.32F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 65, 0x0400, 0x1500, 0);
        event_message_show(TEXT_7);
        event_wait(10);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_wait(55);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 94.08F, 76.07F, 256.29F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d00, 0xbb00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 94.56F, 76.08F, 252.32F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0d00, 0xbb00, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(10);
        event_voice_play(1600);
        event_message_show(TEXT_8);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 76.48F, 75.24F, 239.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1100, 0x1600, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_voice_play(1022);
        event_wait(5);
        event_message_show(TEXT_9);
        event_talk_stop(event_get_player(2));
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1023);
        event_wait(5);
        event_message_show(TEXT_10);
        event_wait(95);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_run_action,
                          &knuckles_texlist, 2.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 121.87F, 72.55F, 319.65F, 1.5F,
                           0.06F);
        event_talk_stop(event_get_player(2));
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -42.73F, 119.12F, 256.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf300, 0xc700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -79.22F, 131.35F, 262.46F);
        event_voice_play(1024);
        event_message_show(TEXT_11);
        event_look_clear(event_get_player(2));
        event_set_ang(event_get_player(2), 0, 0xb000, 0);
        event_wait(60);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_set_pos(event_get_player(0), 121.87F, 72.55F, 319.65F);
        event_set_ang(event_get_player(0), 0x00a0, 0x22f8, 0xffe4);
        event_move_stop_all();
        event_kill_player(2);
        event_kill_player(3);
        event_prop_kill(EGG_CARRIER);
        event_eggmobile_kill();
        event_beam_kill(ev008c_beam);
        ev008c_beam = NULL;
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev008c_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"CAPTUREBEAM", &event_beam_texlist},
    {"EC_LIGHT", &ev008c_egg_carrier_texlist},
    {"SONIC", &sonic_texlist},
    {"MILES", &tails_texlist},
    {NULL, NULL},
};
