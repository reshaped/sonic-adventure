/* EV0050, Tails' story: Eggman comes for Tails in the Egg Walker, and Tails
 * stands up to him alone */
#include <ninja.h>

#include "adv00/texlists.h"
#include "ev0050/car.h"
#include "ev0050/eggman_body_textures.h"
#include "ev0050/tails_action.h"
#include "ev0050/script.h"
#include "ev0050/walker.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/eggmobile.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 59
#define TEXT_2_LENGTH 24
#define TEXT_3_LENGTH 44
#define TEXT_4_LENGTH 29
#define TEXT_5_LENGTH 31
#define TEXT_6_LENGTH 3
#define TEXT_7_LENGTH 3
#include "ev0050/script/text.inc"

/* Station Square's people, in ADV00.PRS, which must hold the stage slot
 * while the event plays */
extern NJS_OBJECT dat_0cabaec8;
extern NJS_OBJECT dat_0cabf974;
extern NJS_OBJECT dat_0cac8a7c;
extern NJS_OBJECT dat_0cad4494;
extern NJS_OBJECT dat_0cad7e9c;
extern NJS_OBJECT dat_0caee8e4;
extern NJS_OBJECT dat_0caf3924;
extern NJS_MOTION dat_0cb0c214;
extern NJS_MOTION dat_0cb0d3a0;
extern NJS_MOTION dat_0cb25a4c;
/* @hack type: the model and the three motions two people share, declared as
 * arrays without a size, though each is one object. SHC then keeps two values
 * where the image does. 0x0b17, the camera's ang_x in the landing's shake,
 * is in R11 from 0x0cb80936. the closing shots' camera x, -433.56, is in FR14
 * from 0x0cb80bfa. as single objects, R11 holds 0x0f17, FR13 and FR14 trade,
 * and 60 of 1311 words differ. with any one of the four alone as a single
 * object, ev0050_script runs 4 bytes short, and 215 to 644 of 1311 words
 * differ */
extern NJS_OBJECT dat_0cae9ee0[];
extern NJS_MOTION dat_0cb13c58[];
extern NJS_MOTION dat_0cb18618[];
extern NJS_MOTION dat_0cb29110[];

extern NJS_TEXLIST event_eggmobile_texlist;

static struct task *ev0050_person_0 = NULL;
static struct task *ev0050_person_1 = NULL;
static struct task *ev0050_person_2 = NULL;
static struct task *ev0050_person_3 = NULL;
static struct task *ev0050_person_4 = NULL;
static struct task *ev0050_person_5 = NULL;
static struct task *ev0050_person_6 = NULL;
static struct task *ev0050_person_7 = NULL;
static struct task *ev0050_person_8 = NULL;
static struct task *ev0050_car_0 = NULL;
static struct task *ev0050_car_1 = NULL;
static struct task *ev0050_car_2 = NULL;
static struct task *ev0050_car_3 = NULL;

void ev0050_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_004e.mlt");
        event_sound_init(2);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_voice_play(1338);
        event_wait(1);
        event_set_pos(event_get_player(0), -432.95F, -2.47F, 1261.66F);
        event_set_ang(event_get_player(0), 0, 0x7e3b, 0);
        event_wait(1);
        event_look_at(event_get_player(0), -438.75F, 35.0F, 1216.08F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0050_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -432.78F, 7.81F, 1254.71F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe369, 0x8124, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -433.32F, 23.94F,
                             1235.62F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xe369, 0x8124, 0);
        event_eggmobile_create(-431.51F, 50.0F, 1163.69F, 0, 0x439e, 0);
        event_place_task(&ev0050_car_0, ev0050_car_create, -504.1F, -7.52F,
                         1167.42F, 0, 0x3e5d, 0);
        event_place_task(&ev0050_car_1, ev0050_car_create, -384.47F, -7.52F,
                         1310.17F, 0, 0xc08d, 0);
        event_place_task(&ev0050_car_2, ev0050_car_create, -497.72F, -7.52F,
                         1368.96F, 0, 0xbed6, 0);
        event_place_task(&ev0050_car_3, ev0050_car_create, -499.81F, -7.52F,
                         1296.52F, 0, 0x4161, 0);
        event_wait(1);
        event_set_flags(ev0050_car_0, EVENT_GROUND);
        event_set_flags(ev0050_car_1, EVENT_GROUND);
        event_set_flags(ev0050_car_2, EVENT_GROUND);
        event_set_flags(ev0050_car_3, EVENT_GROUND);
        event_sound_start(0, 1226, 1800);
        event_place_actor(&ev0050_person_0, -431.57F, 3.0F, 1296.59F, 0, 0xb13c,
                          0);
        event_place_actor(&ev0050_person_4, -452.86F, 7.0F, 1292.42F, 0, 0xccbc,
                          0);
        event_place_actor(&ev0050_person_1, -460.02F, 5.3F, 1278.92F, 0, 0xccbc,
                          0);
        event_place_actor(&ev0050_person_2, -413.94F, 8.0F, 1281.87F, 0, 0xb13e,
                          0);
        event_place_actor(&ev0050_person_3, -427.59F, 3.0F, 1286.2F, 0, 0xb13e,
                          0);
        event_place_actor(&ev0050_person_5, -420.69F, 3.0F, 1287.15F, 0, 0xb13e,
                          0);
        event_place_actor(&ev0050_person_8, -444.58F, 7.0F, 1283.58F, 0, 0xccbc,
                          0);
        event_place_actor(&ev0050_person_6, -450.43F, 8.0F, 1279.94F, 0, 0xccbc,
                          0);
        event_place_actor(&ev0050_person_7, -457.18F, 3.0F, 1281.1F, 0, 0xccbc,
                          0);
        event_wait(1);
        event_set_flags(ev0050_person_0, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_0);
        event_play_motion(ev0050_person_0, &dat_0cabaec8, dat_0cb29110,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_4, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_4);
        event_play_motion(ev0050_person_4, &dat_0caee8e4, &dat_0cb0d3a0,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_1, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_1);
        event_play_motion(ev0050_person_1, &dat_0cac8a7c, &dat_0cb25a4c,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_2, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_2);
        event_play_motion(ev0050_person_2, dat_0cae9ee0, dat_0cb13c58,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_3, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_3);
        event_play_motion(ev0050_person_3, &dat_0cad4494, dat_0cb18618,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_5, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_5);
        event_play_motion(ev0050_person_5, &dat_0cabf974, dat_0cb29110,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_8, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_8);
        event_play_motion(ev0050_person_8, &dat_0caf3924, &dat_0cb0c214,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_6, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_6);
        event_play_motion(ev0050_person_6, dat_0cae9ee0, dat_0cb13c58,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0050_person_7, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0050_person_7);
        event_play_motion(ev0050_person_7, &dat_0cad7e9c, dat_0cb18618,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -453.06F, 67.86F, 1193.33F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf269, 0xe424, 0);
        event_voice_play(1333);
        event_message_wait(0, TEXT_1);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -458.52F, 8.27F, 1296.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x02a1, 0xf101, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 220, -420.26F, 7.31F, 1295.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 220, 0x02a1, 0x0c1c, 0);
        event_wait(100);
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -425.09F, -1.8F, 1278.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0a17, 0x051f, 0);
        event_wait(10);
        event_message_wait(0, TEXT_2);
        event_voice_play(1334);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x2317, 0x0263, 0);
        event_kill_task(&ev0050_person_0);
        event_kill_task(&ev0050_person_2);
        event_kill_task(&ev0050_person_3);
        event_kill_task(&ev0050_person_4);
        event_kill_task(&ev0050_person_1);
        event_kill_task(&ev0050_person_5);
        event_kill_task(&ev0050_person_6);
        event_kill_task(&ev0050_person_7);
        event_kill_task(&ev0050_person_8);
        event_eggmobile_fly_to(-431.51F, 550.0F, 1113.69F, 90);
        event_sound_start(1, 1228, 1800);
        event_sound_fade_pitch(1, 8000, 100);
        event_sound_fade_volume(1, -100, 100);
        event_wait(100);
        event_message_close();
        event_eggmobile_kill();
        ev0050_walker_create(-431.51F, 350.0F, 1163.69F, 0, 0xfb58, 0);
        event_wait(3);
        ev0050_walker_set_shadow(4.0F, 2.5F);
        event_wait(2);
        ev0050_walker_set_mode(WALKER_FALL);
        event_sound_fade_pitch(1, -8000, 80);
        event_sound_fade_volume(1, 100, 80);
        event_wait(40);
        event_sound_fade_volume(0, 100, 1);
        event_voice_play(1315);
        event_wait(1);
        event_voice_play(1317);
        event_wait(1);
        event_sound_fade_volume(0, -100, 100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x0e17, 0x0263, 0);
        event_wait(30);
        event_sound_stop(1);
        event_wait(1);
        event_sound_start(1, 1225, 1800);
        event_sound_fade_volume(1, 120, 1);
        event_sound_fade_volume(1, -100, 100);
        event_wait(1);
        /* the Egg Walker's landing shakes the pad and the camera */
        vibration_start(0, 7, 10, 5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f17, 0x0263, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c17, 0x0263, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f17, 0x0263, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b17, 0x0263, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e17, 0x0263, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b17, 0x0263, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d17, 0x0263, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b17, 0x0263, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d17, 0x0263, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f17, 0x0263, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d17, 0x0263, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -423.34F, -0.28F, 1288.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e17, 0x0263, 0);
        event_wait(40);
        event_sound_stop(1);
        event_sound_play(701, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -443.15F, -1.11F, 1249.12F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0eed, 0x1888, 0);
        event_wait(25);
        event_sound_play(701, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -400.65F, 40.22F, 1221.33F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x08fb, 0x13c6, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -444.14F, 5.04F, 1267.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf4fb, 0xd1c6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 15, -530.16F, 75.71F, 1310.63F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 15, 0xf4fb, 0xe1c6, 0);
        event_wait(15);
        event_sound_play(701, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -539.27F, 79.45F,
                             1320.57F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0xf4fb, 0xe1c6, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -421.5F, 93.08F, 1160.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe2e4, 0x7760, 0);
        event_voice_play(1335);
        event_message_wait(20, TEXT_3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 1000, -433.56F, 8.4F, 1253.92F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 1000, 0xe4e4, 0x8057, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 500, -433.56F, 8.4F, 1253.92F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 500, 0xe4e4, 0x8057, 0);
        event_voice_play(1336);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_message_wait(0, TEXT_4);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -433.56F, 8.4F, 1253.92F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xe4e4, 0x8057, 0);
        event_voice_play(1337);
        event_talk_start(event_get_player(0), TEXT_7);
        event_message_wait(0, TEXT_5);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 10, -433.56F, 8.4F, 1253.92F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 10, 0xe4e4, 0x8057, 0);
        event_wait(20);
        event_message_close();
        event_look_clear(event_get_player(0));
        event_move_stop_all();
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_move_stop_all();
        event_reset_player(0);
        ev0050_walker_kill();
        event_eggmobile_kill();
        event_kill_task(&ev0050_person_0);
        event_kill_task(&ev0050_person_2);
        event_kill_task(&ev0050_person_3);
        event_kill_task(&ev0050_person_4);
        event_kill_task(&ev0050_person_1);
        event_kill_task(&ev0050_person_5);
        event_kill_task(&ev0050_person_6);
        event_kill_task(&ev0050_person_7);
        event_kill_task(&ev0050_person_8);
        event_kill_task(&ev0050_car_0);
        event_kill_task(&ev0050_car_1);
        event_kill_task(&ev0050_car_2);
        event_kill_task(&ev0050_car_3);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0050_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"EV_EGGMOBLE2", &ev0050_walker_texlist},
    {"EV_EGGMAN_BODY", &ev0050_eggman_body_texlist},
    {NULL, NULL},
};
