/* EV0087, Knuckles' story: Knuckles demands the Master Emerald from Eggman, who
 * calls Chaos to fight him. Eggman throws Chaos an emerald, and Chaos becomes
 * Chaos 2 */
#include <ninja.h>

#include "chaos/models.h"
#include "ev0087/chaos2_hold.h"
#include "ev0087/emerald_textures.h"
#include "ev0087/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/chaos0_water_textures.h"
#include "shared/chaos1.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/rays.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 52
#define TEXT_2_LENGTH 17
#define TEXT_3_LENGTH 53
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 56
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 47
#define TEXT_10_LENGTH 38
#define TEXT_11_LENGTH 54
#define TEXT_12_LENGTH 36
#define TEXT_13_LENGTH 32
#define TEXT_14_LENGTH 39
#define TEXT_15_LENGTH 21
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 23
#define TEXT_20_LENGTH 21
#define TEXT_21_LENGTH 15
#include "ev0087/script/text.inc"

extern NJS_ACTION eggman_stand_action;
extern NJS_ACTION dat_8c5a0b78; /* an action that moves eggman_object */

extern NJS_TEXLIST event_chaos1_texlist;

extern NJS_ACTION ev0087_knuckles_action_0;
extern NJS_ACTION ev0087_knuckles_action_1;
extern NJS_ACTION ev0087_knuckles_action_2;
extern NJS_ACTION ev0087_knuckles_action_3;
extern NJS_ACTION ev0087_knuckles_action_4;
extern NJS_ACTION ev0087_eggman_action_0;
extern NJS_ACTION ev0087_eggman_action_1;
extern NJS_ACTION ev0087_eggman_action_2;
extern NJS_ACTION ev0087_emerald_action_0;
extern NJS_ACTION ev0087_emerald_action_1;
extern NJS_ACTION ev0087_emerald_action_2;

/* the white Chaos Emerald, which Eggman throws to Chaos 1 */
static struct task *ev0087_emerald = NULL;
/* the white flash as Chaos 2 forms */
static struct task *ev0087_flash = NULL;

void ev0087_script(int phase)
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
        sound_load_file("e_0087.mlt");
        stream_request_music(MUSIC_EVTBGM04);
        event_set_pos(event_get_player(0), 0.0F, 0.0F, -26.0F);
        event_set_ang(event_get_player(0), 0, 0x7b00, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0087_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.5F, 2.5F, -4.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0cf1, 0xdfda, 0);
        event_create_player(2, eggman_step, 6.16F, 0.0F, -49.7F, 0, 0x8100, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 6.16F, 0.0F, -49.7F);
        event_set_ang(event_get_player(2), 0, 0x8100, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 9.0F,
                           0.0F);
        event_place_actor(&ev0087_emerald, 28.0F, -15.0F, -50.0F, 0, 0xfb00, 0);
        event_wait(1);
        event_set_flags(ev0087_emerald, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_pos(ev0087_emerald, event_get_player(2)->work->pos.x + 2.5F,
                      event_get_player(2)->work->pos.y - 15.5F,
                      event_get_player(2)->work->pos.z + 2.5F);
        event_set_ang(ev0087_emerald, 0, 0xfb00, 0);
        event_play_action(ev0087_emerald, &ev0087_emerald_action_0,
                          &ev0087_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_follow_weld0(ev0087_emerald, 2, -15.0F, -6.7F, 0.0F, 700);
        /* the Chaos 2 boss waits below the floor, out of sight, until he
         * forms */
        ev0087_chaos2_hold(-7.7F, -50.0F, 55.0F, 0x0800, 0x432d, 0xfa00, 8);
        ev0087_chaos2_set_pos(-7.7F, -50.0F, 55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -25.0F, 55.0F);
        event_wait(1);
        ev0087_chaos2_set_ang(0x0800, 0x432d, 0xfa00);
        event_wait(1);
        ev0087_chaos2_melt();
        /* Chaos 1 starts as a puddle below the floor */
        event_chaos1_create(-7.7F, -20.0F, -55.0F, 0, 0x432d, 0, 80);
        event_wait(1);
        event_chaos1_puddle();
        event_wait(1);
        event_chaos1_set_speed(0.3F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -12.2F, 2.5F, -17.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x09f1, 0xdfda, 0);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 3.0F, 11.6F, -66.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb53, 0x7c1b, 0);
        event_set_pos(event_get_player(2), 11.7F, 0.0F, -53.7F);
        event_set_ang(event_get_player(2), 0, 0x8100, 0);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_3,
                          &knuckles_texlist, 1.4F, 0, 0);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_4,
                          &knuckles_texlist, 1.5F, EVENT_REPEAT, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_15);
        event_voice_play(1564);
        event_message_show(TEXT_1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 7.0F, 11.6F, -65.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xfd53, 0x861b, 0);
        event_wait(40);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_2,
                          &knuckles_texlist, 0.9F, EVENT_REPEAT, 16);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(2), &ev0087_eggman_action_0,
                          &eggman_texlist, 0.7F, 0, 0);
        event_voice_play(1565);
        event_message_show(TEXT_2);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6.3F, 12.1F, -39.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0429, 0xeff4, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 6.9F, 12.5F, -44.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 20, 0x0453, 0xed1b, 0xff00);
        event_set_ang(event_get_player(2), 0, 0xf000, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0087_eggman_action_1,
                          &eggman_texlist, 0.1F, EVENT_REPEAT, 0);
        /* the emerald stops following Eggman. it rocks on its x axis, 14
         * above the floor */
        event_set_pos(ev0087_emerald, 5.2F, 14.0F, -56.7F);
        event_set_ang(ev0087_emerald, 0x1000, 0xfb00, 0);
        event_move_stop_all();
        event_set_ang(event_get_player(0), 0, 0x7000, 0);
        event_voice_play(1566);
        event_message_show(TEXT_3);
        event_move_turn(ev0087_emerald, 5.2F, 14.0F, -56.7F, 4.9F, 14.0F,
                        -58.5F, 0x1000, 0, 0, -0x1000, 0, 0, 18);
        event_wait(10);
        event_wait(8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 5.7F, 11.68F, -45.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0c20, 0xfffd, 0);
        event_move_turn(ev0087_emerald, 4.9F, 14.0F, -58.5F, 4.9F, 14.5F,
                        -57.5F, 0x1000, 0, 0, -0x1000, 0, 0, 40);
        event_wait(45);
        event_move_turn(ev0087_emerald, 4.9F, 14.5F, -57.5F, 6.5F, 14.3F,
                        -55.7F, -0x1000, 0, 0, 0x1000, 0, 0, 75);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 5.7F, 12.6F, -47.86F);
        event_wait(60);
        event_play_action(event_get_player(2), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.9F, 2.8F, -37.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0953, 0x901b, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, -2.4F, 3.8F, -33.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x0953, 0x901b, 0);
        event_set_pos(ev0087_emerald, 8.6F, -14.3F, -70.0F);
        event_set_ang(ev0087_emerald, 0, 0xfb00, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_16);
        event_voice_play(1567);
        event_message_show(TEXT_4);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_1,
                          &knuckles_texlist, 0.7F, EVENT_REPEAT, 8);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_set_pos(ev0087_emerald, 7.0F, -18.0F, -57.6F);
        event_set_ang(ev0087_emerald, 0, 0xfb00, 0);
        event_wait(40);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F,
                           12.0F, 0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -17.7F, 1.8F, -25.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0c53, 0xd01b, 0xfb00);
        event_voice_play(1568);
        event_message_show(TEXT_5);
        event_voice_wait();
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &dat_8c5a0b78, &eggman_texlist,
                          1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -11.7F, 1.0F, -26.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0bf1, 0xe7da, 0xfbe6);
        event_message_show(TEXT_6);
        event_wait(15);
        event_voice_play(1569);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -4.9F, 1.27F, -14.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x0bf1, 0xf6da, 0x02e6);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_3,
                          &knuckles_texlist, 1.3F, 0, 16);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_2,
                          &knuckles_texlist, 1.3F, EVENT_REPEAT, 16);
        event_wait_motions(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        /* the puddle comes up. Chaos 1 then rises from it */
        sound_play_for(1226, 1, 1, 0, 128);
        event_chaos1_set_pos(-7.7F, -8.0F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -7.5F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -7.0F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -6.5F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -6.0F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -5.5F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -5.0F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -4.5F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -4.0F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -3.5F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -3.0F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -2.5F, -55.0F);
        event_wait(1);
        event_chaos1_set_pos(-7.7F, -2.0F, -55.0F);
        event_wait(18);
        event_chaos1_set_pos(-7.7F, -2.0F, -55.0F);
        sound_stop(1226);
        event_chaos1_rise();
        event_chaos1_set_speed(0.3F);
        event_wait(35);
        sound_play_at_for(1223, 2, 1, 400, -7.7F, 1.22F, -55.0F, 128);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_17);
        event_voice_play(1570);
        event_message_show(TEXT_7);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_wait(10);
        event_look_at(event_get_player(0), -18.0F, 12.0F, -63.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -11.05F, 0.3F, -66.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x07f1, 0x8ada, 0xfde6);
        event_chaos1_set_pos(-7.7F, -1.0F, -55.0F);
        event_set_ang(event_get_player(0), 0, 0x9200, 0);
        event_set_ang(event_get_player(2), 0, 0xc000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 290, -10.6F, 17.6F, -64.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 290, 0xf9a1, 0x88da, 0xfde6);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1571);
        event_message_show(TEXT_8);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(1572);
        event_message_show(TEXT_9);
        event_wait(70);
        event_wait(70);
        event_talk_stop(event_get_player(0));
        sound_stop_id(2);
        sound_play(1223, 0, 0, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_voice_play(1573);
        event_message_show(TEXT_10);
        event_wait(110);
        event_talk_stop(event_get_player(0));
        sound_play_at_for(1224, 3, 1, 10, -7.7F, 1.22F, -55.0F, 300);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.3F, 5.0F, -30.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x00f1, 0xaada, 0x00e6);
        sound_stop_id(3);
        sound_play_at_for(1223, 9, 1, 0, -7.7F, 1.22F, -55.0F, 300);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        sound_stop(1224);
        event_voice_play(1574);
        event_message_show(TEXT_11);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -8.5F, 5.1F, -30.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x00f1, 0xaada, 0x00e6);
        event_wait(30);
        event_wait(45);
        event_talk_stop(event_get_player(0));
        event_wait(20);
        event_message_close();
        /* Eggman throws the emerald to Chaos 1 */
        event_move_stop_all();
        event_set_pos(ev0087_emerald, 8.0F, 12.8F, -56.9F);
        event_set_ang(ev0087_emerald, 0, 0xd000, 0);
        event_set_ang(event_get_player(0), 0, 0x9600, 0);
        event_set_pos(event_get_player(2), 13.37F, 0.0F, -52.4F);
        event_set_ang(event_get_player(2), 0, 0xdb00, 0);
        event_play_action(event_get_player(2), &ev0087_eggman_action_2,
                          &eggman_texlist, 0.8F, 0, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.4F, 3.12F, -29.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0ddc, 0xf8da, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30, ev0087_emerald, 0.0F,
                                  0.0F, 0.0F, 0);
        event_move(ev0087_emerald, 8.0F, 12.8F, -56.9F, 5.8F, 12.4F, -53.8F,
                   10);
        event_wait(10);
        event_move(ev0087_emerald, 5.8F, 12.4F, -53.8F, -3.0F, 39.0F, -52.6F,
                   20);
        event_wait(25);
        event_play_action(event_get_player(2), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev0087_emerald, &ev0087_emerald_action_1,
                          &ev0087_emerald_texlist, 0.7F, 0, 0);
        event_play_action(ev0087_emerald, &ev0087_emerald_action_2,
                          &ev0087_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev0087_emerald, -3.0F, 41.0F, -52.6F, -9.0F, 5.0F, -52.6F,
                   55);
        event_wait(45);
        event_camera_look_clear();
        event_wait(10);
        /* Chaos 1 melts as the emerald reaches him. Chaos 2 forms in a white
         * flash */
        sound_play_at_for(1226, 8, 1, 90, -7.7F, 1.22F, -55.0F, 128);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -28.4F, 14.12F, -62.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x00b6, 0xab98, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -25.4F, 4.12F, -63.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x00b6, 0x9f98, 0);
        event_chaos1_melt();
        event_wait(10);
        event_rays_create(ev0087_emerald->work->pos.x,
                          ev0087_emerald->work->pos.y,
                          ev0087_emerald->work->pos.z, 3, 10, 0.4F, 2.0F,
                          0xffffffff, 0x96969696);
        ev0087_flash = event_fade_create(0.04F, 0.1F, 1.0F, 1.0F, 1.0F);
        stream_stop_music();
        stream_request_music(MUSIC_EVTBGM01);
        event_wait(50);
        event_rays_kill();
        if (ev0087_flash != NULL) {
            event_fade_set_speed(ev0087_flash, -0.02F);
        }
        event_kill_task(&ev0087_emerald);
        ev0087_chaos2_set_pos(-7.7F, -8.0F, -55.0F);
        ev0087_chaos2_set_ang(0x0800, 0x432d, 0xfa00);
        event_wait(1);
        event_chaos1_kill();
        ev0087_chaos2_form();
        /* the Chaos 2 boss rises where Chaos 1 stood */
        ev0087_chaos2_set_pos(-7.7F, -7.5F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -7.0F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -6.5F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -6.0F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -5.5F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -5.0F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -4.5F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -4.0F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -3.5F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -3.0F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -2.5F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -2.0F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -1.5F, -55.0F);
        event_wait(1);
        ev0087_chaos2_set_pos(-7.7F, -1.0F, -55.0F);
        event_wait(1);
        ev0087_chaos2_ribbon_rate(3.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.9F, 2.8F, -40.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfef1, 0x17e1, 0x0100);
        event_set_pos(event_get_player(2), 7.54F, 0.0F, -49.2F);
        event_set_ang(event_get_player(2), 0, 0xe000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 12.2F, 1.5F, -25.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x0f08, 0x16da, 0x0100);
        event_wait(40);
        event_voice_play(1575);
        event_message_show(TEXT_12);
        event_play_action(event_get_player(2), &dat_8c5a0b78, &eggman_texlist,
                          1.0F, 0, 0);
        event_wait_motions(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &eggman_stand_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at(event_get_player(2), -18.0F, 19.0F, 63.0F);
        sound_stop(1225);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -7.2F, 5.7F, -28.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x03f1, 0xa7da, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -5.5F, 5.7F, -30.85F);
        event_voice_play(1576);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_21);
        event_message_show(TEXT_13);
        event_wait(55);
        event_talk_stop(event_get_player(0));
        event_wait(20);
        sound_play_at_for(1223, 6, 1, 20, -7.7F, 1.22F, -55.0F, 140);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -15.1F, 1.9F, -16.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x09f1, 0xf5da, 0);
        ev0087_chaos2_action(1.0F);
        sound_play_at(1225, 0, 0, 128, -7.7F, -50.0F, 55.0F);
        event_set_pos(event_get_player(2), 9.7F, 0.0F, -69.17F);
        event_set_ang(event_get_player(2), 0, 0xe000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -6.4F, 1.9F, -13.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x09f1, 0xf8d8, 0);
        event_play_action(event_get_player(0), &ev0087_knuckles_action_0,
                          &knuckles_texlist, 1.4F, EVENT_REPEAT, 20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_20);
        event_voice_play(1577);
        event_message_show(TEXT_14);
        event_wait(130);
        event_talk_stop(event_get_player(0));
        sound_stop(1223);
        sound_stop(1224);
        sound_stop(1225);
        sound_stop(1226);
        event_wait(1);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        sound_stop(1223);
        sound_stop(1224);
        sound_stop(1225);
        sound_stop(1226);
        sound_stop_id(1);
        sound_stop_id(2);
        sound_stop_id(3);
        sound_stop_id(6);
        sound_stop_id(8);
        sound_stop_id(9);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0087_emerald);
        event_chaos1_kill();
        ev0087_chaos2_release();
        event_voice_stop();
        event_rays_kill();
        if (ev0087_flash != NULL) {
            task_kill(ev0087_flash);
            ev0087_flash = NULL;
        }
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0087_textures[] = {
    {"M_EM_WHITE", &ev0087_emerald_texlist},
    {"EV_CHAOS1", &event_chaos1_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};
