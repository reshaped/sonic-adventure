/* EV0080, Knuckles' story: the opening on the floating island. Chaos 0 shatters
 * the Master Emerald, knocks Knuckles away and escapes. Knuckles sees that the
 * island will fall */
#include <ninja.h>

#include "adv02/master_emerald.h"
#include "chaos/models.h"
#include "ev0080/knuckles_action_10.h"
#include "ev0080/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/chaos0.h"
#include "shared/clouds.h"
#include "shared/dust.h"
#include "shared/fade.h"
#include "shared/light.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 46
#define TEXT_2_LENGTH 56
#define TEXT_3_LENGTH 62
#define TEXT_4_LENGTH 29
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 23
#define TEXT_9_LENGTH 19
#define TEXT_10_LENGTH 25
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 13
#define TEXT_14_LENGTH 21
#define TEXT_15_LENGTH 13
#define TEXT_16_LENGTH 33
#define TEXT_17_LENGTH 47
#define TEXT_18_LENGTH 68
#define TEXT_19_LENGTH 1
#define TEXT_20_LENGTH 1
#define TEXT_21_LENGTH 1
#define TEXT_22_LENGTH 2
#define TEXT_23_LENGTH 2
#define TEXT_24_LENGTH 1
#define TEXT_25_LENGTH 1
#define TEXT_26_LENGTH 1
#define TEXT_27_LENGTH 2
#include "ev0080/script/text.inc"

extern NJS_ACTION knuckles_sprint_action;
extern NJS_ACTION knuckles_skid_action;
extern NJS_ACTION knuckles_stand_action;

extern NJS_ACTION ev0080_knuckles_action_0;
/* @hack type: ev0080_knuckles_action_1 declared as an array, which script.c
 * passes by its name. SHC then loads event_voice_play's address at each call,
 * as at 0x0cb80158. as one NJS_ACTION passed as &ev0080_knuckles_action_1, SHC
 * holds the address in R8, script.c's code ends 36 bytes short of the image's,
 * and 1993 of 2263 words differ */
extern NJS_ACTION ev0080_knuckles_action_1[];
/* @hack type: ev0080_knuckles_action_2 declared as an array, which script.c
 * passes by its name. SHC then also loads event_voice_play's address at each
 * call. as one NJS_ACTION passed as &ev0080_knuckles_action_2, SHC holds the
 * address in R8, script.c's code ends 36 bytes short of the image's, and 1993
 * of 2263 words differ */
extern NJS_ACTION ev0080_knuckles_action_2[];
extern NJS_ACTION ev0080_knuckles_action_3;
extern NJS_ACTION ev0080_knuckles_action_4;
extern NJS_ACTION ev0080_knuckles_action_5;
extern NJS_ACTION ev0080_knuckles_action_6;
extern NJS_ACTION ev0080_knuckles_action_7;
extern NJS_ACTION ev0080_knuckles_action_8;
extern NJS_ACTION ev0080_knuckles_action_9;
extern NJS_TEXLIST event_chaos0_water_texlist;

/* player 0's pad as Knuckles punches at Chaos 0, a key a step. B is held for 4,
 * 5 and 5 steps, each after 2 steps up */
static const struct event_pad_key ev0080_punch_keys[20] = {
#include "ev0080/script/ev0080_punch_keys.inc"
};

static const struct event_pad_recording ev0080_punches = {
    ev0080_punch_keys,
    sizeof(ev0080_punch_keys) / sizeof(ev0080_punch_keys[0])};

static struct task *ev0080_light = NULL;
/* the black fade that ends the event */
static struct task *ev0080_fade = NULL;
/* the dust where Knuckles lands */
static struct task *ev0080_dust = NULL;

void ev0080_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0080.mlt");
        event_sound_init(5);
        event_look_clear(event_get_player(0));
        if (ev0080_dust == NULL) {
            ev0080_dust = event_dust_create();
        }
        event_set_pos(ev0080_dust, -1.78F, -238.4925F, 2398.0F);
        event_clouds_start(2.5F, 10);
        event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_WHOLE;
        event_set_pos(event_get_player(0), -3.0F, -157.083F, 2251.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_reset_player(0);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_4,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1231.86F, 262.36F,
                             3331.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf500, 0xe200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 290, -859.1F, 247.95F,
                             3485.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 290, 0xf500, 0xe900, 0);
        event_wait(1);
        event_voice_play(1529);
        event_message_show(TEXT_1);
        event_wait(120);
        event_wait(1);
        event_voice_wait();
        event_wait(15);
        event_voice_play(1530);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 158.28F, -74.886F,
                             2620.085F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf100, 0x0500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, -25.4F, -78.26F, 2629.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 240, 0xf100, 0xfe00, 0);
        event_voice_play(1531);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1532);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2e39);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.27F, -139.6F, 2262.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0300, 0xf9f7, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -6.39F, -148.39F,
                             2261.825F);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -6.4688F, -154.37F,
                             2261.374F);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 5.4757F, -150.7F, 2260.144F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf200, 0x22f7, 0x0a00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 5.4757F, -151.4F,
                             2260.144F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf200, 0x22f7, 0x0a00);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x3111);
        event_chaos0_create(-3.0F, -156.8F, 2190.0F, 0, 0x4000, 0, 90);
        event_chaos0_set_speed(0.3F);
        /* the emerald's object sets the same for Knuckles once event 130
         * has played */
        event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_SHATTERED;
        event_wait(100);
        vibration_start(0, 3, 50, 6);
        event_sound_start(0, 1223, 1800);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfc00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe800, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xeb00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xea00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfc00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe900, 0x22f7, 0x0a00);
        event_wait(2);
        event_talk_stop(event_get_player(0));
        /* @bug logic: no mouth takes TEXT_19. the call passes a null task
         * (MOV #0,R4 at 0x0cb804d4, in the slot of the JSR at 0x0cb804d2),
         * which event_talk_start ignores, where the stop above passes player
         * 0 */
        event_talk_start(NULL, TEXT_19);
        event_look_at(event_get_player(0), -2.5F, -147.0F, 2262.8F);
        event_sound_play(1224, 0, 0, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xee00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfa00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xeb00, 0x22f7, 0x0a00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf200, 0x22f7, 0x0a00);
        event_wait(3);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 5.82F, -148.44F, 2260.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xf200, 0x22f7, 0x0a00);
        event_play_action(event_get_player(0), ev0080_knuckles_action_1,
                          &knuckles_texlist, 1.5F, 0, 10);
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 5.42F, -155.5668F,
                             2249.22F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0c00, 0x41f7, 0x0800);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_7,
                          &knuckles_texlist, 1.3F, 0, 8);
        event_look_clear(event_get_player(0));
        event_voice_play(1533);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_message_show(TEXT_5);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_8,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(55);
        stream_request_music(MUSIC_EVTBGM01);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 4.464F, -155.2765F,
                             2249.266F);
        event_wait(5);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -3.27F, -155.145F, 2260.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0x0bf7, 0xff00);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_8,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 0.5488F, -155.24F,
                             2259.08F);
        event_wait(40);
        ev0080_light = event_light_create(-2.56F, -135.623F, 2153.42F);
        event_wait(40);
        event_move(ev0080_light, -2.56F, -135.623F, 2153.42F, -19.2F, -116.76F,
                   2157.9F, 50);
        event_wait(50);
        event_move(ev0080_light, -19.2F, -116.76F, 2157.9F, 61.17F, -38.143F,
                   2157.82F, 80);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2b61);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 6.8232F, -156.53F,
                             2198.575F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0x24f7, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 7.2F, -149.54F, 2198.65F);
        event_wait(69);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -13.2855F, -152.3265F,
                             2182.07F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0400, 0xa3f7, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -13.92F, -145.36F,
                             2181.766F);
        event_wait(69);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -8.076F, -147.166F,
                             2199.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1500, 0xe3f7, 0xfb00);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -14.356F, -156.2F,
                             2202.755F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1500, 0xe3f7, 0xfb00);
        event_wait(90);
        event_set_pos(ev0080_light, 15.95F, -187.42F, 2147.44F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.59F, -152.8F, 2241.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1100, 0x82f7, 0xf900);
        event_voice_play(1534);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_message_show(TEXT_6);
        event_voice_wait();
        event_wait(5);
        event_look_at(event_get_player(0), -0.53F, -147.5F, 2292.17F);
        event_talk_start(event_get_player(0), TEXT_21);
        event_message_show(TEXT_7);
        event_voice_play(1535);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.936F, -150.87F,
                             2211.178F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0500, 0xf5f7, 0);
        event_chaos0_set_speed(0.5F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, -10.645F, -147.69F,
                             2186.157F);
        event_voice_play(1536);
        event_message_show(TEXT_8);
        event_set_pos(event_get_player(0), -3.5F, -157.08F, 2226.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0x000f);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), ev0080_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_clear(event_get_player(0));
        event_wait(35);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -11.54F, -147.886F,
                             2184.378F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0400, 0xa0f0, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0400, 0x93f0, 0);
        event_wait(30);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0400, 0x8ef0, 0);
        event_play_action(event_get_player(0), ev0080_knuckles_action_2,
                          &knuckles_texlist, 1.2F, 0, 8);
        event_voice_play(1538);
        event_talk_start(event_get_player(0), TEXT_22);
        event_message_show(TEXT_9);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_wait();
        event_talk_start(event_get_player(0), TEXT_23);
        event_voice_play(1540);
        event_message_show(TEXT_10);
        event_wait(50);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0080_punches);
        event_chaos0_set_speed(1.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -9.2296F, -149.258F,
                             2168.938F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x0400, 0x89f0, 0);
        event_message_close();
        event_wait(25);
        event_chaos0_hop2(-3.0F, -162.0F, 2160.0F, 40.0F, 25);
        event_wait(25);
        event_chaos0_hop3(-3.0F, -157.5F, 2222.0F, 70.0F, 115);
        event_wait(4);
        event_clear_motions(event_get_player(0));
        event_wait(3);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), -3.1999F, -157.0768F, 2224.1F);
        event_look_at(event_get_player(0), -3.0F, -100.0F, 2160.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x4000);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -3.32F, -130.5F, 2209.387F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x2300, 0x09f0, 0xfc00);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 0.55F, -153.7F, 2229.0F);
        event_wait(49);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 1.06F, -156.73F, 2231.6F);
        event_wait(43);
        event_sound_play(1225, 0, 0, 0);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_5,
                          &knuckles_texlist, 0.8F, 0, 0);
        event_wait(3);
        event_sound_start(1, 1226, 1800);
        event_move(event_get_player(0), -3.1999F, -157.0768F, 2224.1F, -1.71F,
                   -238.4925F, 2388.586F, 100);
        event_voice_play(671);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x2800, 0x09f0, 0xfc00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1f00, 0x09f0, 0xfc00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x2700, 0x09f0, 0xfc00);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x2000, 0x09f0, 0xfc00);
        event_wait(3);
        event_message_show(TEXT_11);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -4.0F, -166.0F, 2286.08F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1800, 0xf580, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -6.963F, -238.773F,
                             2398.62F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x1000, 0xef80, 0xfb00);
        event_wait(70);
        event_sound_stop(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -9.22F, -239.235F, 2404.5F);
        event_move(event_get_player(0), -1.71F, -238.4925F, 2388.586F, -1.78F,
                   -238.4925F, 2398.0F, 16);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_6,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_24);
        event_voice_play(1541);
        event_message_show(TEXT_12);
        if (ev0080_dust != NULL) {
            DUST_WORK(ev0080_dust)->timer = DUST_WORK(ev0080_dust)->interval =
                3;
        }
        event_wait(10);
        if (ev0080_dust != NULL) {
            DUST_WORK(ev0080_dust)->timer = DUST_WORK(ev0080_dust)->interval =
                0;
        }
        event_wait(10);
        event_message_close();
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_sprint_action,
                          &knuckles_texlist, 1.7F, EVENT_REPEAT, 8);
        event_wait(4);
        event_run_to_speed(event_get_player(0), -3.1999F, -157.0768F, 2239.1F,
                           0.9F, 0.9F);
        event_wait(30);
        event_chaos0_walk(-3.0F, -157.5F, 2190.0F, 2);
        event_wait(10);
        event_chaos0_walk(-3.0F, -157.5F, 2193.0F, 2);
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -0.48F, -154.0F, 2188.16F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0500, 0x8680, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -4.5F, -153.3F, 2188.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0500, 0x8680, 0xfd00);
        event_wait(40);
        event_play_action(event_get_player(0), &knuckles_skid_action,
                          &knuckles_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), ev0080_knuckles_action_2,
                          &knuckles_texlist, 1.5F, 0, 4);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(80);
        event_chaos0_melt();
        event_sound_start(2, 1227, 1800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -6.0F, -154.5F, 2178.93F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0500, 0x8680, 0xfd00);
        event_wait(80);
        event_sound_stop(2);
        event_chaos0_slide(-3.0F, -165.0F, 2193.0F, 65);
        event_talk_start(event_get_player(0), TEXT_24);
        event_voice_play(1671);
        event_message_show(TEXT_13);
        event_wait(20);
        event_wait(15);
        event_message_close();
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.247F, -152.6F, 2232.825F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1300, 0x9680, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -7.078F, -152.665F,
                             2233.375F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x1300, 0x9a80, 0xfd00);
        event_look_at(event_get_player(0), -3.2F, -154.157F, 2228.1F);
        event_wait(35);
        event_voice_play(1542);
        event_talk_start(event_get_player(0), TEXT_25);
        event_message_show(TEXT_14);
        event_voice_wait();
        event_wait(30);
        vibration_start(0, 2, 50, 5);
        event_sound_start(0, 1223, 1800);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1e00, 0x9a80, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0800, 0x9a80, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1c00, 0x9a80, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0600, 0x9a80, 0);
        event_wait(3);
        event_sound_stop(0);
        event_look_at(event_get_player(0), -3.2F, -149.0F, 2228.1F);
        event_talk_start(event_get_player(0), TEXT_21);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1d00, 0x9a80, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0700, 0x9a80, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x1c00, 0x9a80, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0600, 0x9a80, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1a00, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0a00, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1b00, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0f00, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1a00, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0f00, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1700, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1000, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1500, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1300, 0x9a80, 0);
        event_wait(3);
        event_voice_play(1543);
        event_talk_start(event_get_player(0), TEXT_26);
        event_message_show(TEXT_15);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1500, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1100, 0x9a80, 0);
        event_wait(2);
        event_look_at(event_get_player(0), 0.53F, -149.0F, 2231.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1500, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1200, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1600, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1100, 0x9a80, 0);
        event_wait(2);
        event_look_at(event_get_player(0), -6.55F, -149.0F, 2234.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1400, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1200, 0x9a80, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1300, 0x9a80, 0);
        event_wait(3);
        event_voice_wait();
        event_wait(20);
        event_voice_play(1544);
        event_talk_start(event_get_player(0), TEXT_25);
        event_message_show(TEXT_16);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_7,
                          &knuckles_texlist, 1.3F, 0, 8);
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -3.1F, -148.357F, 2221.686F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf600, 0x8000, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 180, 0x38e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -3.1F, -65.19F,
                             2118.5662F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe200, 0x8000, 0);
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_set_pos(event_get_player(0), -3.1F, -157.0F, 2235.637F);
        event_set_ang(event_get_player(0), 0, 0x26b1, 0);
        event_wait(10);
        event_voice_play(1545);
        event_talk_start(event_get_player(0), TEXT_27);
        event_message_show(TEXT_17);
        event_wait(20);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_9,
                          &knuckles_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev0080_knuckles_action_10,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_move(ev0080_light, -50.1F, -76.05F, 2142.4556F, 20.1F, -73.0F,
                   2131.7F, 160);
        event_wait(130);
        ev0080_fade = event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(40);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_set_pos(event_get_player(0), -3.1F, -157.0F, 2235.637F);
        event_set_ang(event_get_player(0), 0, 0x26b1, 0);
        if (ev0080_light != NULL) {
            task_kill(ev0080_light);
            ev0080_light = NULL;
        }
        event_reset_player(0);
        event_chaos0_kill();
        EVENT_KILL(ev0080_dust);
        event_clouds_stop();
        event_sound_stop_all();
        EVENT_KILL(ev0080_fade);
        event_flags[FLAG_MASTER_EMERALD] = 0;
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0080_textures[] = {
    {"EV_ECCLOUD", &event_clouds_texlist},
    {"CHAOS0", &event_chaos0_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};
