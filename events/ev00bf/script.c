/* EV00BF, E-102's story: E-102 comes for the bird Amy holds in a cell, and
 * she begs him to help them. he tells her to run and frees them */
#include <ninja.h>

#include "adv0130/jdoor.h"
#include "ev00bf/amy_action_5.h"
#include "ev00bf/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 17
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 29
#define TEXT_5_LENGTH 18
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 25
#define TEXT_8_LENGTH 39
#define TEXT_9_LENGTH 17
#define TEXT_10_LENGTH 15
#define TEXT_11_LENGTH 52
#define TEXT_12_LENGTH 22
#define TEXT_13_LENGTH 54
#define TEXT_14_LENGTH 16
#define TEXT_15_LENGTH 48
#define TEXT_16_LENGTH 15
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 49
#define TEXT_19_LENGTH 27
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 5
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 16
#define TEXT_24_LENGTH 54
#define TEXT_25_LENGTH 11
#define TEXT_26_LENGTH 49
#define TEXT_27_LENGTH 45
#define TEXT_28_LENGTH 25
#define TEXT_29_LENGTH 3
#define TEXT_30_LENGTH 1
#define TEXT_31_LENGTH 2
#define TEXT_32_LENGTH 3
#define TEXT_33_LENGTH 7
#define TEXT_34_LENGTH 1
#define TEXT_35_LENGTH 3
#define TEXT_36_LENGTH 2
#define TEXT_37_LENGTH 1
#define TEXT_38_LENGTH 2
#define TEXT_39_LENGTH 2
#define TEXT_40_LENGTH 3
#include "ev00bf/script/text.inc"

/* in the Egg Carrier, where this event plays, the flag of each gate and floor
 * switch of adv0130/jdoor.h placed with an x angle of 2. the flags below 64
 * are the act's own, cleared as it starts, so in the Mystic Ruins this one is
 * FLAG_STATION_DOOR */
#define FLAG_JDOOR_2 (JDOOR_FLAGS + 2)

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_pull_action;
/* @hack type: e102_walk_action declared as an array, which script.c passes by
 * its name. SHC then keeps the camera's y, 15.67F, in FR12 (FMOV.S FR12,FR5 at
 * 0x0cb80288). as one NJS_ACTION passed as &e102_walk_action, FR12 takes the
 * camera's x, 115.31F, instead, and 6 of ev00bf_script's 2795 words differ */
extern NJS_ACTION e102_walk_action[];
/* @hack type: amy_run_action declared as an array, which script.c passes by its
 * name. SHC then keeps the camera's y, 15.67F, in FR12 (FMOV.S FR12,FR5 at
 * 0x0cb80288). as one NJS_ACTION passed as &amy_run_action, FR12 takes the
 * camera's x, 115.31F, instead, and 6 of ev00bf_script's 2795 words differ */
extern NJS_ACTION amy_run_action[];
/* @hack type: amy_stand_action declared as an array, which script.c passes by
 * its name. SHC then puts event_set_pos's address in R10 and
 * event_camera_set_ang's in R9 (0x0cb80026, 0x0cb8025a). as one NJS_ACTION
 * passed as &amy_stand_action, the two trade, and 94 of ev00bf_script's 2795
 * words differ */
extern NJS_ACTION amy_stand_action[];

extern NJS_ACTION ev00bf_amy_action_0;
extern NJS_ACTION ev00bf_amy_action_1;
extern NJS_ACTION ev00bf_amy_action_2;
extern NJS_ACTION ev00bf_amy_action_3;
extern NJS_ACTION ev00bf_amy_action_4;
extern NJS_ACTION ev00bf_bird_action_7;
extern NJS_ACTION ev00bf_e102_action_1;
extern NJS_ACTION ev00bf_e102_action_2;
/* @hack type: ev00bf_e102_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps the camera's y, 15.67F, in FR12 (FMOV.S FR12,FR5
 * at 0x0cb80288). as one NJS_ACTION passed as &ev00bf_e102_action_0, FR12 takes
 * the camera's x, 115.31F, instead, and 6 of ev00bf_script's 2795 words
 * differ */
extern NJS_ACTION ev00bf_e102_action_0[];

static struct task *ev00bf_bird = NULL;

void ev00bf_script(int phase)
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
        sound_load_file("e_00bf.mlt");
        event_set_pos(event_get_player(0), 140.0F, 0.0F, 43.0F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_create_player(2, amy_step, -100.0F, 0.1F, -33.37F, 0, 0xf215, 0);
        event_place_actor(&ev00bf_bird, -100.0F, 5.5F, -5.5F, 0, 0, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(2), -100.0F, 0.1F, -33.37F);
        event_set_ang(event_get_player(2), 0, 0xf215, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00bf_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 1);
        event_set_flags(ev00bf_bird, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_pos(event_get_player(0), 140.0F, 0.0F, 43.0F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_set_pos(ev00bf_bird, -100.0F, 0.0F, -33.37F);
        event_set_ang(ev00bf_bird, 0, 0xf215, 0);
        event_play_action(ev00bf_bird, &ev00bf_bird_action_0,
                          &ev00bf_bird_texlist_1, 1.0F, EVENT_REPEAT, 1);
        stream_request_music(MUSIC_E102);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -92.69F, 20.88F, 105.44F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe00, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -92.69F, 20.88F, 31.44F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 115.31F, 15.67F, 67.44F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe00, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 115.31F, 15.67F, 44.44F);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 115.31F, 15.67F, 41.44F);
        event_wait(30);
        event_play_action(event_get_player(0), ev00bf_e102_action_0,
                          &e102_texlist, 1.0F, 0, 16);
        event_wait_motions(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), e102_walk_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_run_to_speed(event_get_player(0), 135.0F, 0.0F, 10.0F, 0.75F,
                           0.06F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 127.3F, 15.77F, 46.45F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0, 0);
        event_camera_follow(event_get_player(0));
        event_run_to_speed(event_get_player(0), -100.0F, 0.0F, 20.0F, 0.75F,
                           0.06F);
        event_run_wait(event_get_player(0));
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -129.52F, 16.45F, 42.86F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0xfc00, 0xe600, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           14.0F, 0.0F);
        event_play_action(event_get_player(0), ev00bf_e102_action_0,
                          &e102_texlist, 1.0F, 0, 16);
        event_wait_motions(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -134.85F, 17.33F, 50.06F);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 8);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -99.52F, 0.24F, -26.35F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x12e4, 0xf5f3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -101.45F, 0.24F, -26.84F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_29);
        event_message_show(TEXT_1);
        event_voice_play(1416);
        event_wait(40);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_play_action(ev00bf_bird, &ev00bf_bird_action_1,
                          &ev00bf_bird_texlist_1, 1.0F, 0, 0);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -102.08F, 4.23F, -52.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfde4, 0x7ff3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -102.08F, 16.21F, -52.38F);
        event_wait(90);
        event_voice_play(1418);
        event_message_wait(55, TEXT_2);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x0e39);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -118.82F, 24.59F, 63.98F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf7e4, 0xf4f3, 0);
        event_play_action(event_get_player(2), &ev00bf_amy_action_1,
                          &amy_texlist, 1.0F, 0, 0);
        event_play_action(ev00bf_bird, &ev00bf_bird_action_2,
                          &ev00bf_bird_texlist_1, 1.0F, 0, 0);
        event_wait(21);
        event_play_action(event_get_player(2), &ev00bf_amy_action_2,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00bf_bird, &ev00bf_bird_action_3,
                          &ev00bf_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_30);
        event_voice_play(1419);
        event_message_show(TEXT_3);
        event_wait(32);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -100.11F, 2.06F, -27.07F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0ae4, 0x7ff3, 0);
        event_voice_play(1420);
        event_message_wait(60, TEXT_4);
        event_message_close();
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -115.2F, 22.9F, 44.16F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf5e4, 0xf2f3, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_31);
        event_voice_play(1421);
        event_message_wait(45, TEXT_5);
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_wait(10);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -128.35F, 2.34F, -44.91F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x03e4, 0x95f3, 0);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -127.49F, 2.34F, -45.43F);
        event_voice_play(1422);
        event_message_wait(30, TEXT_6);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -119.13F, 1.76F, -57.38F);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -116.55F, 1.76F, -58.92F);
        event_wait(10);
        event_talk_start(event_get_player(2), TEXT_31);
        event_voice_play(1423);
        event_message_wait(60, TEXT_7);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_play_action(event_get_player(2), &ev00bf_amy_action_3,
                          &amy_texlist, 1.0F, 0, 0);
        event_play_action(ev00bf_bird, &ev00bf_bird_action_4,
                          &ev00bf_bird_texlist_1, 1.0F, 0, 0);
        event_wait(31);
        event_play_action(event_get_player(2), &ev00bf_amy_action_4,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev00bf_bird, &ev00bf_bird_action_5,
                          &ev00bf_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(2), TEXT_32);
        event_voice_play(1424);
        event_message_wait(90, TEXT_8);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -108.49F, 13.74F, -2.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x00e4, 0x93f3, 0);
        event_voice_play(1425);
        event_message_wait(40, TEXT_9);
        event_message_close();
        event_wait(40);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -116.01F, 0.88F, -23.63F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0ae4, 0xd2f3, 0);
        event_talk_start(event_get_player(2), TEXT_31);
        event_voice_play(1426);
        event_message_wait(40, TEXT_10);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x1555);
        event_wait(60);
        event_talk_start(event_get_player(2), TEXT_32);
        event_voice_play(1427);
        event_message_wait(90, TEXT_11);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -131.22F, 12.74F, 46.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfce4, 0xebf3, 0);
        event_wait(60);
        event_talk_start(event_get_player(2), TEXT_31);
        event_voice_play(1428);
        event_message_wait(60, TEXT_12);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00bf_amy_action_5,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(2), -100.0F, 0.0F, -11.0F, 0.75F,
                           0.06F);
        event_follow(ev00bf_bird, 0.0F, 0.0F, 0.0F, 50, event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -143.55F, 14.65F, 68.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0xfce4, 0xe5f3, 0);
        event_wait(20);
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &e102_pull_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 20.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 20.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 21.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 21.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 22.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 22.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 23.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 23.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 24.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 24.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 25.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 25.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 26.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 26.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 27.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 27.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 28.0F);
        event_wait(2);
        event_play_action(event_get_player(2), &ev00bf_amy_action_4,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           14.0F, 0.0F);
        event_set_pos(ev00bf_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y,
                      event_get_player(2)->work->pos.z);
        event_set_ang(ev00bf_bird, event_get_player(2)->work->ang.x,
                      0x4000 - event_get_player(2)->work->ang.y,
                      event_get_player(2)->work->ang.z);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 28.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 29.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 29.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 30.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 30.5F);
        event_wait(2);
        event_set_pos(event_get_player(0), -100.0F, 0.0F, 31.0F);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(45);
        event_talk_start(event_get_player(2), TEXT_33);
        event_voice_play(1429);
        event_message_wait(90, TEXT_13);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -100.03F, 21.86F, -57.11F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf8e4, 0x7ff3, 0);
        event_wait(45);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 2.5F, EVENT_REPEAT, 0);
        /* each sound_play_for and sound_play_at_for call takes an id of its
         * own, from EVENT_SOUND_ID up, so none refreshes another's sound */
        sound_play_for(1216, EVENT_SOUND_ID, 1, 0, 40);
        event_voice_play(1430);
        event_message_wait(40, TEXT_14);
        event_message_close();
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_voice_play(1431);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 2.5F, EVENT_REPEAT, 0);
        sound_play_for(1216, EVENT_SOUND_ID + 1, 1, 0, 120);
        event_message_show(TEXT_15);
        event_voice_wait();
        event_message_close();
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 2.5F, EVENT_REPEAT, 0);
        event_voice_play(1432);
        sound_play_for(1216, EVENT_SOUND_ID + 2, 1, 0, 50);
        event_message_wait(40, TEXT_16);
        event_message_close();
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -102.11F, 30.96F, 47.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xebe4, 0xfcf3, 0);
        event_wait(30);
        event_talk_start(event_get_player(2), TEXT_34);
        event_voice_play(1433);
        event_message_wait(30, TEXT_17);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_35);
        event_voice_play(1434);
        event_message_wait(90, TEXT_18);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -110.56F, 12.99F, 3.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xee0b, 0xe31a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -104.08F, 8.3F, -3.87F);
        event_wait(30);
        event_play_action(ev00bf_bird, &ev00bf_bird_action_6,
                          &ev00bf_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        sound_play_for(1226, EVENT_SOUND_ID + 3, 1, 0, 120);
        event_wait(30);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -128.5F, 18.39F, -10.87F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf2e4, 0xc0f3, 0);
        event_play_action(ev00bf_bird, &ev00bf_bird_action_7,
                          &ev00bf_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        event_move(ev00bf_bird, -100.0F, 0.0F, -11.0F,
                   event_get_player(0)->work->pos.x,
                   event_get_player(0)->work->pos.y + 17.0F,
                   event_get_player(0)->work->pos.z - 15.0F, 60);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 2.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(2), TEXT_36);
        event_voice_play(1435);
        event_message_show(TEXT_19);
        event_talk_stop(event_get_player(2));
        event_message_close();
        sound_play_for(1227, EVENT_SOUND_ID + 4, 1, 0, 240);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -127.69F, 18.39F, 24.12F);
        event_wait(60);
        event_play_action(event_get_player(2), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -100.0F, 16.65F, 27.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x03e4, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x238e);
        event_wait(60);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -91.66F, 17.67F, 15.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfbe4, 0x65f3, 0);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 3.0F, EVENT_REPEAT, 0);
        sound_play_for(1216, EVENT_SOUND_ID + 5, 1, 0, 150);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -79.78F, 19.69F, -0.14F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -143.59F, 22.68F, 26.86F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf1e4, 0xbdf3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -150.17F, 25.06F, 26.53F);
        event_wait(60);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), e102_walk_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -123.0F, 0.0F, 31.0F, 0.75F,
                           0.06F);
        event_wait(30);
        event_set_ang(ev00bf_bird, 0, 0xcb27, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -150.17F, 25.06F, 26.53F);
        sound_play_for(1224, EVENT_SOUND_ID + 6, 1, 0, 60);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 0.3F, EVENT_REPEAT, 16);
        event_wait(60);
        event_voice_play(1436);
        event_message_wait(60, TEXT_20);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x0e39);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -130.14F, 9.18F, 69.27F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfde4, 0x03f3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -97.3F, 9.18F, 66.08F);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -94.91F, 9.18F, 65.85F);
        event_talk_start(event_get_player(2), TEXT_37);
        event_voice_play(1437);
        event_message_show(TEXT_21);
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_wait(50);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x271c);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -145.76F, 3.59F, 41.21F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x023a, 0xd65d, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -146.32F, 15.57F, 41.55F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00bf_e102_action_1,
                          &e102_texlist, 0.5F, 0, 32);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -146.32F, 20.0F, 41.59F);
        event_play_action(event_get_player(0), &ev00bf_e102_action_2,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        sound_play_at(1223, 0, 0, 120, -123.0F, 0.0F, 31.0F);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -111.76F, 2.42F, 10.19F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0370, 0xe667, 0);
        event_flags[FLAG_JDOOR_2] |= JSWITCH_PRESS;
        sound_play_at_for(1225, EVENT_SOUND_ID + 7, 1, 120, -100.0F, 0.1F,
                          -33.37F, 120);
        vibration_start(0, 7, 50, 3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -120.68F, 2.21F, 22.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0e70, 0xe667, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x3111);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 270, -123.99F, 2.21F, 27.02F);
        event_wait(60);
        event_talk_start(event_get_player(2), TEXT_34);
        event_voice_play(1439);
        event_message_wait(60, TEXT_22);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -137.38F, 19.48F, 24.08F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0xf4e4, 0xbaf3, 0x0150);
        event_look_clear(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), amy_run_action, &amy_texlist,
                          1.5F, EVENT_REPEAT, 32);
        event_run_to_speed(event_get_player(2), -74.0F, 0.0F, 15.0F, 0.75F,
                           0.06F);
        event_set_ang(ev00bf_bird, 0, 0x4000, 0);
        event_wait(1);
        event_move(ev00bf_bird, -100.0F, 17.0F, 16.0F, -74.0F, 12.0F, 17.0F,
                   80);
        event_wait(80);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 1);
        event_set_ang(event_get_player(2), 0, 0xd44c, 0);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           14.0F, 0.0F);
        event_move_stop(ev00bf_bird);
        event_wait(1);
        event_set_ang(ev00bf_bird, 0, 0xc000, 0);
        event_wait(30);
        event_voice_play(1440);
        event_message_wait(60, TEXT_23);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -139.29F, 20.02F, 23.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0xf4e4, 0xbaf3, 0x0150);
        event_wait(45);
        event_voice_play(1441);
        event_message_wait(120, TEXT_24);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -87.57F, 3.9F, 9.86F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x06e4, 0xa9f3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 250, -86.02F, 3.9F, 7.29F);
        event_wait(30);
        event_talk_start(event_get_player(2), TEXT_38);
        event_voice_play(1442);
        event_message_show(TEXT_25);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_39);
        event_voice_play(1443);
        event_message_wait(90, TEXT_26);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -35.45F, 8.35F, 18.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfee4, 0x3ff3, 0);
        event_look_clear(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -31.45F, 8.46F, 18.92F);
        event_talk_start(event_get_player(2), TEXT_40);
        event_voice_play(1444);
        event_message_wait(80, TEXT_27);
        event_talk_start(event_get_player(2), TEXT_32);
        event_voice_play(1445);
        event_message_wait(60, TEXT_28);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(20);
        event_set_ang(ev00bf_bird, 0, 0x4000, 0);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_run_to_speed(event_get_player(2), -10.0F, 0.0F, 15.0F, 0.75F,
                           0.06F);
        event_follow(ev00bf_bird, 0.0F, 12.0F, 2.0F, 200, event_get_player(2));
        event_play_action(event_get_player(2), amy_run_action, &amy_texlist,
                          2.0F, EVENT_REPEAT, 16);
        event_wait(90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -136.71F, 18.85F, 11.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf2e4, 0xc0f3, 0x0500);
        event_set_pos(event_get_player(2), -74.0F, 0.0F, 15.0F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), amy_run_action, &amy_texlist,
                          2.0F, EVENT_REPEAT, 16);
        event_run_to_speed(event_get_player(2), 137.5F, 0.0F, 10.0F, 0.75F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -136.35F, 18.85F, 26.67F);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -104.83F, 11.15F, 4.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xffe4, 0x56f3, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -99.5F, 11.15F, 12.77F);
        event_wait(30);
        break;
    case EVENT_END:
        event_move_stop_all();
        stream_stop_music();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -123.0F, 0.0F, 31.0F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_kill_task(&ev00bf_bird);
        event_kill_player(2);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00bf_textures[] = {
    {"VER1_WING", &ev00bf_bird_texlist_1},
    {"VER2_WING", &ev00bf_bird_texlist_2},
    {"AMY", &amy_texlist},
    {NULL, NULL},
};
