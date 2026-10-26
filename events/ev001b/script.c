/* EV001B, Sonic's story: Amy stops Sonic's fight with E-102. Sonic sends Tails
 * off the falling Egg Carrier with her and goes after Eggman. Sonic is player
 * 0, Amy player 2, Tails player 3 and E-102 player 4, as event_get_player
 * numbers them */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev001b/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/e102.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/lightning.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 26
#define TEXT_3_LENGTH 44
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 41
#define TEXT_7_LENGTH 52
#define TEXT_8_LENGTH 56
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 26
#define TEXT_11_LENGTH 45
#define TEXT_12_LENGTH 2
#define TEXT_13_LENGTH 1
#define TEXT_14_LENGTH 2
#define TEXT_15_LENGTH 3
#define TEXT_16_LENGTH 3
#define TEXT_17_LENGTH 1
#define TEXT_18_LENGTH 3
#define TEXT_19_LENGTH 4
#define TEXT_20_LENGTH 3
#define TEXT_21_LENGTH 1
#define TEXT_22_LENGTH 6
#include "ev001b/script/text.inc"

/* the first child of sonic_hand_minus_z in 1ST_READ.BIN. its sibling is
 * sonic_fingers_minus_z */
/* @hack type: dat_8c1e0958 declared as an array, and written as
 * dat_8c1e0958->sibling. SHC then pools the node's address and writes its
 * sibling field 48 bytes past it, as MOV.L R2,@(48,R3) at 0x0cb80c6c does. as
 * one NJS_OBJECT written as dat_8c1e0958.sibling, the pool word holds the
 * sibling field's address instead, and 5 of 2726 words differ. as one
 * NJS_OBJECT written as (&dat_8c1e0958)->sibling, the stores match, but SHC
 * holds -3.0 in FR12 and 1525.0 in FR13, and 28 of 2726 words differ */
extern NJS_OBJECT dat_8c1e0958[];
/* @hack type: sonic_fingers_minus_z declared as an array, which script.c stores
 * by its name. SHC then loads 3390.0 into FR12 at 0x0cb8035a and -3.0 into FR13
 * at 0x0cb8035e, for the sound_play_at calls that pass them. as one NJS_OBJECT
 * stored as &sonic_fingers_minus_z, SHC holds -3.0 in FR12 and 1525.0 in FR13
 * instead, and 28 of 2726 words differ */
extern NJS_OBJECT sonic_fingers_minus_z[];
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION sonic_run_action;
extern NJS_ACTION sonic_skid_action;
extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_trot_action;

extern NJS_OBJECT ev001b_sonic_fingers;
extern NJS_ACTION ev001b_sonic_action_0;
extern NJS_ACTION ev001b_sonic_action_1;
extern NJS_ACTION ev001b_sonic_action_2;
extern NJS_ACTION ev001b_sonic_action_3;
extern NJS_ACTION ev001b_sonic_action_4;
extern NJS_ACTION ev001b_sonic_action_5;
extern NJS_ACTION ev001b_tails_action_0;
extern NJS_ACTION ev001b_amy_action_0;
extern NJS_ACTION ev001b_amy_action_1;
extern NJS_ACTION ev001b_e102_action_0;
extern NJS_ACTION ev001b_e102_action_1;
extern NJS_ACTION ev001b_e102_action_2;
extern NJS_ACTION ev001b_e102_action_3;

/* no code uses this word. it keeps ev001b_textures at its address in the
 * image */
static Uint32 dat_0cb821a8 = 0;

void ev001b_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_001b.mlt");
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe00, 0x8e00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 100.72F, 1527.35F, 3327.55F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -5.67F, 1527.35F,
                             3365.62F);
        event_set_pos(event_get_player(0), -3.18F, 1525.68F, 3300.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_create_player(4, e102_step, event_get_player(0)->work->pos.x,
                            event_get_player(0)->work->pos.y,
                            event_get_player(0)->work->pos.z + 90.0F,
                            event_get_player(0)->work->ang.x,
                            -0x4000 - event_get_player(0)->work->ang.y,
                            event_get_player(0)->work->ang.z);
        event_wait(1);
        event_lightning_create(event_get_player(4)->work->pos.x,
                               event_get_player(4)->work->pos.y,
                               event_get_player(4)->work->pos.z, 4);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev001b_e102_action_0,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_create_player(
            2, amy_step, event_get_player(4)->work->pos.x - 120.0F,
            event_get_player(4)->work->pos.y,
            event_get_player(4)->work->pos.z + 10.0F,
            event_get_player(4)->work->ang.x, -event_get_player(4)->work->ang.y,
            event_get_player(4)->work->ang.z);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_create_player(3, tails_step,
                            event_get_player(0)->work->pos.x - 15.0F,
                            event_get_player(0)->work->pos.y,
                            event_get_player(0)->work->pos.z - 17.0F,
                            event_get_player(0)->work->ang.x,
                            0x5000 - event_get_player(0)->work->ang.y,
                            event_get_player(0)->work->ang.z);
        event_wait(1);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev001b_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        stream_request_music(MUSIC_THESONIC);
        event_look_at_task(event_get_player(3), event_get_player(4), 0.0F, 6.0F,
                           0.0F);
        event_wait(60);
        sound_play_at(1224, 0, 0, 100, -3.0F, 1525.0F, 3390.0F);
        event_wait(35);
        sound_play_at(1224, 0, 0, 100, -3.0F, 1525.0F, 3390.0F);
        event_wait(15);
        sound_play_at(1224, 0, 0, 100, -3.0F, 1525.0F, 3390.0F);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, -12.8F, 1527.08F,
                             3370.53F);
        sound_play_at(1224, 0, 0, 100, -3.0F, 1525.0F, 3390.0F);
        event_wait(15);
        sound_play_at(1224, 0, 0, 100, -3.0F, 1525.0F, 3390.0F);
        event_wait(5);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev001b_e102_action_1,
                          &e102_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(4), &ev001b_e102_action_2,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        sound_play_at(1225, 0, 0, 120, -3.0F, 1525.0F, 3390.0F);
        event_wait(30);
        event_wait(50);
        event_lightning_kill();
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x600, 0x9f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -32.7F, 1531.14F, 3308.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x600, 0x9400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -16.51F, 1529.26F,
                             3286.42F);
        event_wait(70);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x600, 0x8b00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -10.48F, 1529.4F,
                             3285.62F);
        event_wait(50);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.69F, 1530.97F, 3313.28F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x80, 0xf3b3, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -13.77F, 1530.64F,
                             3339.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0x80, 0xedb3, 0);
        event_wait(10);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_run_action,
                          &sonic_texlist, 3.5F, EVENT_REPEAT, 8);
        event_run_to(event_get_player(0), event_get_player(0)->work->pos.x,
                     event_get_player(0)->work->pos.y,
                     event_get_player(0)->work->pos.z + 60.0F);
        event_wait(30);
        event_set_pos(event_get_player(0), -3.18F, 1525.68F, 3300.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_run_action,
                          &sonic_texlist, 3.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(0), event_get_player(0)->work->pos.x,
                     event_get_player(0)->work->pos.y,
                     event_get_player(0)->work->pos.z + 60.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x100, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -11.74F, 1530.14F, 3305.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x100, 0x8700, 0x800);
        event_wait(25);
        event_set_pos(event_get_player(0), -3.18F, 1525.68F, 3300.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3c72);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_run_action,
                          &sonic_texlist, 3.5F, EVENT_REPEAT, 10);
        event_run_to(event_get_player(0), event_get_player(0)->work->pos.x,
                     event_get_player(0)->work->pos.y,
                     event_get_player(0)->work->pos.z + 60.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xef00, 0x8600, 0x500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.64F, 1565.87F, 3262.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 35, 0x200, 0x8600, 0x600);
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, -9.76F, 1526.92F, 3337.49F);
        event_wait(30);
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0xf00, 0x8d00, 0x700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, -9.53F, 1527.28F, 3353.3F);
        event_wait(15);
        event_set_pos(event_get_player(2),
                      event_get_player(4)->work->pos.x - 20.0F,
                      event_get_player(4)->work->pos.y,
                      event_get_player(4)->work->pos.z - 15.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf00, 0x4d00, 0x700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -27.0F, 1535.43F, 3339.28F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, -18.15F, 1529.81F, 3373.1F);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -17.75F, 1529.56F,
                             3374.68F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_voice_play(1090);
        event_message_wait(50, TEXT_1);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 64.47F, 1529.06F, 3316.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x800, 0x5000, 0x700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 5.15F, 1529.8F, 3353.71F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x600, 0x6500, 0x700);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_trot_action, &amy_texlist,
                          2.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(2), event_get_player(4)->work->pos.x,
                     event_get_player(4)->work->pos.y,
                     event_get_player(4)->work->pos.z - 15.0F);
        event_set_pos(event_get_player(0), -3.18F, 1525.68F, 3270.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_run_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        event_run_to(event_get_player(0), event_get_player(0)->work->pos.x,
                     event_get_player(0)->work->pos.y,
                     event_get_player(0)->work->pos.z + 80.0F);
        event_wait(60);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(2), event_get_player(0)->work->ang.x,
                      -0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_run_wait(event_get_player(0));
        sound_play(SE_SKID, 0, 0, 0);
        event_play_action(event_get_player(0), &sonic_skid_action,
                          &sonic_texlist, 0.8F, 0, 0);
        event_run_to(event_get_player(0), event_get_player(0)->work->pos.x,
                     event_get_player(0)->work->pos.y,
                     event_get_player(0)->work->pos.z + 10.0F);
        event_wait_motions(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 2.41F, 1534.15F, 3379.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0x1200, 0);
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_wait(10);
        event_voice_play(1091);
        event_message_show(TEXT_2);
        event_play_action(event_get_player(0), &ev001b_sonic_action_2,
                          &sonic_texlist, 1.0F, 0, 2);
        event_wait(10);
        event_talk_start(event_get_player(0), TEXT_14);
        /* ev001b_sonic_fingers takes sonic_fingers_minus_z's place until
         * TEXT_2 closes. dat_8c1e0958 belongs to 1ST_READ.BIN's Sonic, who
         * outlives the event, so EVENT_END puts sonic_fingers_minus_z back
         * too */
        dat_8c1e0958->sibling = &ev001b_sonic_fingers;
        event_wait(25);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev001b_sonic_action_4,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_wait(20);
        event_message_close();
        event_talk_stop(event_get_player(0));
        dat_8c1e0958->sibling = sonic_fingers_minus_z;
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -13.62F, 1534.03F, 3348.96F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf900, 0x9300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -10.07F, 1532.66F, 3356.0F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev001b_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(10);
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1093);
        event_wait(20);
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(10);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1094);
        event_message_wait(90, TEXT_3);
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.5F, 1532.23F, 3369.52F);
        event_wait(10);
        event_talk_start(event_get_player(2), TEXT_16);
        event_voice_play(1095);
        event_message_show(TEXT_4);
        event_wait(16);
        event_talk_stop(event_get_player(2));
        event_wait(14);
        event_message_close();
        event_wait(10);
        event_set_pos(event_get_player(3),
                      event_get_player(0)->work->pos.x - 40.0F,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z - 10.0F);
        event_set_ang(event_get_player(3), event_get_player(0)->work->ang.x,
                      0x7000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_look_clear(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -0.13F, 1528.75F, 3367.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1100, 0x1100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -1.25F, 1529.97F, 3365.2F);
        event_wait(45);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1096);
        event_message_wait(30, TEXT_5);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 32);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1097);
        event_message_wait(80, TEXT_6);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 25.39F, 1527.59F, 3344.71F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x480, 0x5d00, 0x500);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 11.44F, 1529.77F,
                             3352.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_play_action(event_get_player(0), &ev001b_sonic_action_5,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(4), &ev001b_e102_action_3,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(2), &ev001b_amy_action_1,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        sound_play_at(1223, 0, 0, 120, -3.0F, 1525.0F, 3390.0F);
        vibration_start(0, 7, 50, 3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1280, 0x5d00, 0x500);
        event_wait(3);
        event_play_action(event_get_player(0), &ev001b_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(4), &ev001b_e102_action_3,
                          &e102_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev001b_amy_action_1,
                          &amy_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x980, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x980, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        sound_stop(1215);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x580, 0x5d00, 0x500);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x480, 0x5d00, 0x500);
        vibration_start(0, 7, 50, 3);
        sound_play_at(1223, 0, 0, 120, -3.0F, 1525.0F, 3390.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -35.19F, 1532.11F, 3359.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf780, 0x1500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -33.26F, 1532.94F,
                             3362.46F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_message_show(TEXT_7);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_voice_play(1098);
        event_talk_stop(event_get_player(3));
        event_talk_start(event_get_player(3), TEXT_19);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf680, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf780, 0x1500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf680, 0x1500, 0);
        event_wait(3);
        event_wait(57);
        sound_stop(1215);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &ev001b_sonic_action_1,
                          &sonic_texlist, 0.4F, 0, 4);
        event_wait(15);
        sound_play_at(1223, 0, 0, 100, -102.0F, 1519.0F, 3000.0F);
        event_talk_stop(event_get_player(3));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 6.01F, 1530.3F, 3369.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x80, 0x2a00, 0);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_voice_play(1099);
        event_message_wait(80, TEXT_8);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.71F, 1530.29F, 3368.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x880, 0x9800, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev001b_e102_action_2,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_21);
        event_voice_play(1100);
        event_message_wait(25, TEXT_9);
        event_message_close();
        event_talk_stop(event_get_player(2));
        sound_play_at(1223, 0, 0, 80, -102.0F, 1519.0F, 3000.0F);
        event_look_clear(event_get_player(0));
        event_wait(5);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 8.0F,
                           0.0F);
        event_play_action(event_get_player(0), &ev001b_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev001b_sonic_action_3,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 7.12F, 1531.76F, 3383.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x80, 0x1100, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x271c);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_22);
        event_voice_play(1101);
        event_message_wait(60, TEXT_10);
        event_message_wait(60, TEXT_11);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_wait(1);
        sound_play_at(1223, 0, 0, 80, -102.0F, 1519.0F, 3000.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 12.35F, 1542.55F, 3384.94F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf580, 0x1100, 0x500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 36.64F, 1558.34F,
                             3439.73F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_run_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 10);
        event_run_to(event_get_player(0),
                     event_get_player(0)->work->pos.x - 80.0F,
                     event_get_player(0)->work->pos.y,
                     event_get_player(0)->work->pos.z - 240.0F);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_wait(40);
        sound_play_at(1223, 0, 0, 50, -102.0F, 1519.0F, 3000.0F);
        event_wait(80);
        break;
    case EVENT_END:
        event_lightning_kill();
        dat_8c1e0958->sibling = sonic_fingers_minus_z;
        event_reset_player(0);
        event_set_pos(event_get_player(0), -42.46F, 1525.69F, 3240.8F);
        event_set_ang(event_get_player(0), 0, 0x8d17, 0);
        event_kill_player(2);
        event_kill_player(3);
        event_kill_player(4);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev001b_textures[] = {
    {"AMY", &amy_texlist},
    {"MILES", &tails_texlist},
    {"E102", &e102_texlist},
    {NULL, NULL},
};
