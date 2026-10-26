/* EV0051, Tails' story: Tails beats the Egg Walker on his own, and flies
 * off */
#include <ninja.h>

#include "adv00/texlists.h"
#include "ev0051/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 30
#define TEXT_3_LENGTH 1
#define TEXT_4_LENGTH 1
#define TEXT_5_LENGTH 1
#define TEXT_6_LENGTH 5
#include "ev0051/script/text.inc"

/* Station Square's people, in ADV00.PRS, which must hold the stage slot
 * while the event plays */
extern NJS_OBJECT dat_0cabaec8;
extern NJS_OBJECT dat_0cabf974;
extern NJS_OBJECT dat_0cac8a7c;
extern NJS_OBJECT dat_0cacc504;
extern NJS_OBJECT dat_0cad4494;
extern NJS_OBJECT dat_0cadb7dc;
extern NJS_OBJECT dat_0cae3e38;
extern NJS_OBJECT dat_0cae9ee0;
extern NJS_OBJECT dat_0caee8e4;
extern NJS_OBJECT dat_0caf3924;
extern NJS_MOTION dat_0cb0c214;
extern NJS_MOTION dat_0cb13c58;
/* @hack type: three of the five motions declared as arrays without a size,
 * though each is one motion that two people share. SHC then keeps
 * event_play_motion in R9 and event_set_flags in R8 through the ten plays, as
 * the image does (0x0cb802ba, 0x0cb8029c). with only two of the motions as
 * arrays, event_set_flags takes R9, and each play loads event_play_motion
 * again. ev0051_script then runs 60 bytes long, and 827 of 938 words differ.
 * any three of the five as arrays build the same */
extern NJS_MOTION dat_0cb18618[];
extern NJS_MOTION dat_0cb25a4c[];
extern NJS_MOTION dat_0cb29110[];

extern NJS_ACTION ev0051_tails_action_0;
extern NJS_ACTION ev0051_tails_action_1;
extern NJS_ACTION ev0051_tails_action_2;
extern NJS_ACTION ev0051_tails_action_3;

/* Tails' run, then his jump and his flight. A is pressed at key 107 for the
 * jump. it is pressed again at key 120 for the flight, and held to key 228 */
static const struct event_pad_key ev0051_run_keys[235] = {
#include "ev0051/script/ev0051_run_keys.inc"
};

static const struct event_pad_recording ev0051_run = {
    ev0051_run_keys, sizeof(ev0051_run_keys) / sizeof(ev0051_run_keys[0])};

static struct task *ev0051_person_0 = NULL;
static struct task *ev0051_person_1 = NULL;
static struct task *ev0051_person_2 = NULL;
static struct task *ev0051_person_3 = NULL;
static struct task *ev0051_person_4 = NULL;
static struct task *ev0051_person_5 = NULL;
static struct task *ev0051_person_6 = NULL;
static struct task *ev0051_person_7 = NULL;
static struct task *ev0051_person_8 = NULL;
static struct task *ev0051_person_9 = NULL;
static struct task *ev0051_fade = NULL;

void ev0051_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_004e.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -474.59F, 5.06F, 1034.16F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf9ce, 0xa2bd, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, -470.87F, 4.31F, 1037.41F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 300, 0xf9ce, 0xa2bd, 0);
        event_sound_start(0, 1227, 1800);
        event_set_pos(event_get_player(0), -466.11F, -2.5F, 1043.02F);
        event_set_ang(event_get_player(0), 0, 0x913a, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0051_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_actor(&ev0051_person_1, -528.16F, 6.0F, 1244.91F, 0, 0x050f,
                          0);
        event_place_actor(&ev0051_person_4, -532.17F, 10.0F, 1243.44F, 0,
                          0x050f, 0);
        event_place_actor(&ev0051_person_0, -527.04F, 8.3F, 1272.46F, 0, 0x050f,
                          0);
        event_place_actor(&ev0051_person_2, -538.07F, 10.0F, 1268.95F, 0,
                          0x050f, 0);
        event_place_actor(&ev0051_person_3, -533.53F, 6.0F, 1259.09F, 0, 0x050f,
                          0);
        event_place_actor(&ev0051_person_6, -534.29F, 6.0F, 1266.88F, 0, 0x050f,
                          0);
        event_place_actor(&ev0051_person_9, -539.62F, 10.0F, 1281.7F, 0, 0x050f,
                          0);
        event_place_actor(&ev0051_person_5, -541.08F, 8.3F, 1249.92F, 0, 0x050f,
                          0);
        event_place_actor(&ev0051_person_7, -534.69F, 10.0F, 1234.08F, 0,
                          0x050f, 0);
        event_place_actor(&ev0051_person_8, -526.91F, 6.0F, 1229.34F, 0, 0x050f,
                          0);
        event_wait(1);
        event_set_flags(ev0051_person_1, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_1);
        event_play_motion(ev0051_person_1, &dat_0cabaec8, dat_0cb29110,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_4, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_4);
        event_play_motion(ev0051_person_4, &dat_0caee8e4, &dat_0cb0c214,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_0, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_0);
        event_play_motion(ev0051_person_0, &dat_0cac8a7c, dat_0cb25a4c,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_2, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_2);
        event_play_motion(ev0051_person_2, &dat_0cae9ee0, &dat_0cb13c58,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_3, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_3);
        event_play_motion(ev0051_person_3, &dat_0cad4494, dat_0cb18618,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_6, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_6);
        event_play_motion(ev0051_person_6, &dat_0cabf974, dat_0cb29110,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_9, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_9);
        event_play_motion(ev0051_person_9, &dat_0caf3924, &dat_0cb0c214,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_5, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_5);
        event_play_motion(ev0051_person_5, &dat_0cacc504, dat_0cb25a4c,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_7, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_7);
        event_play_motion(ev0051_person_7, &dat_0cae3e38, &dat_0cb13c58,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev0051_person_8, EVENT_IGNORE_GROUND);
        event_clear_motions(ev0051_person_8);
        event_play_motion(ev0051_person_8, &dat_0cadb7dc, dat_0cb18618,
                          &ss_people_texlist, 0.5F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_message_wait(0, TEXT_1);
        event_voice_play(1349);
        event_wait(100);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -509.81F, 4.77F, 1239.43F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0x3b8c, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -513.52F, 4.77F, 1273.22F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x0c00, 0x3b8c, 0);
        event_voice_play(1361);
        event_wait(30);
        event_voice_play(1352);
        event_wait(30);
        event_voice_play(1360);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -472.03F, 8.39F, 1033.49F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xeb84, 0x9772, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, -470.12F, 6.46F, 1036.32F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0xeb84, 0x9772, 0);
        event_set_pos(ev0051_person_1, -431.57F, 3.0F, 1296.59F);
        event_set_ang(ev0051_person_1, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_0, -452.86F, 7.0F, 1292.42F);
        event_set_ang(ev0051_person_0, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_4, -460.02F, 5.3F, 1278.92F);
        event_set_ang(ev0051_person_4, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_2, -413.94F, 8.0F, 1281.87F);
        event_set_ang(ev0051_person_2, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_3, -427.59F, 3.0F, 1286.2F);
        event_set_ang(ev0051_person_3, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_6, -420.69F, 3.0F, 1287.15F);
        event_set_ang(ev0051_person_6, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_5, -444.58F, 7.0F, 1283.58F);
        event_set_ang(ev0051_person_5, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_9, -437.03F, 5.3F, 1288.17F);
        event_set_ang(ev0051_person_9, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_7, -450.43F, 8.0F, 1279.94F);
        event_set_ang(ev0051_person_7, 0, 0x79b9, 0);
        event_set_pos(ev0051_person_8, -457.18F, 3.0F, 1281.1F);
        event_set_ang(ev0051_person_8, 0, 0x79b9, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &ev0051_tails_action_1,
                          &tails_texlist, 0.5F, 0, 0);
        event_wait(15);
        event_play_action(event_get_player(0), &ev0051_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1350);
        event_message_wait(0, TEXT_2);
        stream_request_music(MUSIC_THEMILES);
        event_voice_play(1355);
        event_wait(1);
        event_message_close();
        event_talk_start(event_get_player(0), TEXT_6);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0051_tails_action_3,
                          &tails_texlist, 1.5F, 0, 16);
        event_wait(1);
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -463.66F, 51.35F, 1110.79F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe1ce, 0x0dbd, 0);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0051_run);
        event_wait(130);
        event_sound_fade_volume(0, 120, 60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -419.63F, -2.23F, 1316.47F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1384, 0x1172, 0);
        event_wait(60);
        event_sound_fade_volume(0, -120, 90);
        ev0051_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        event_kill_task(&ev0051_person_1);
        event_kill_task(&ev0051_person_2);
        event_kill_task(&ev0051_person_3);
        event_kill_task(&ev0051_person_4);
        event_kill_task(&ev0051_person_0);
        event_kill_task(&ev0051_person_6);
        event_kill_task(&ev0051_person_7);
        event_kill_task(&ev0051_person_8);
        event_kill_task(&ev0051_person_9);
        event_kill_task(&ev0051_person_5);
        EVENT_KILL(ev0051_fade);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0051_textures[] = {
    {NULL, NULL},
};
