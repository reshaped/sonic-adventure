/* EV0038, Tails' story: Knuckles knocks Tails back twice, and Tails refuses to
 * hand over his emerald */
#include <ninja.h>

#include "ev0038/knuckles_action_2.h"
#include "ev0038/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/knuckles.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/shapes.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 43
#define TEXT_5_LENGTH 20
#define TEXT_6_LENGTH 3
#define TEXT_7_LENGTH 4
#define TEXT_8_LENGTH 4
#include "ev0038/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION knuckles_stand_action;

extern NJS_ACTION ev0038_tails_action_0;
extern NJS_ACTION ev0038_tails_action_1;
extern NJS_ACTION ev0038_knuckles_action_0;
extern NJS_ACTION ev0038_knuckles_action_1;

/* Knuckles' punches: B is held at keys 0 to 6, 15 to 21 and 33 to 48 */
static const struct event_pad_key ev0038_punch_keys[49] = {
#include "ev0038/script/ev0038_punch_keys.inc"
};

static const struct event_pad_recording ev0038_punch = {
    ev0038_punch_keys,
    sizeof(ev0038_punch_keys) / sizeof(ev0038_punch_keys[0])};

void ev0038_script(int phase)
{
    /* @hack redundant: tails, set once and never read. SHC then loads
     * Knuckles' x into FR12 and his z into FR13 (0x0cb8009c, 0x0cb800a0). it
     * loads event_play_action's address into R11 (0x0cb8012a), and
     * event_camera_set_pos's into R12 (0x0cb80220). without it, FR12 and FR13
     * hold his z and y, R11 and R12 trade, and 41 of 1051 words differ */
    struct task *tails;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_set_pos(tails = event_get_player(0), 34.94F, 60.89F, 427.09F);
        event_set_ang(event_get_player(0), 0x0ed9, 0x79ce, 0xfbf7);
        event_create_player(2, knuckles_step, 87.87F, 72.71F, 252.82F, 0xff96,
                            0xdef7, 18);
        event_create_player(3, sonic_step, 39.39F, 51.38F, 450.68F, 0x0ed9,
                            0x8521, 0xfbf7);
        event_wait(1);
        event_set_pos(event_get_player(0), 34.94F, 60.89F, 427.09F);
        event_set_ang(event_get_player(0), 0x0ed9, 0x79ce, 0xfbf7);
        event_set_pos(event_get_player(2), 87.87F, 72.71F, 252.82F);
        event_set_ang(event_get_player(2), 0xff96, 0xdef7, 18);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_run_action,
                          &tails_texlist, 2.5F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &sonic_trot_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        event_wait(1);
        stream_request_music(MUSIC_THEKNKLS);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 112.26F, 76.19F, 252.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x008c, 0x6e8a, 0);
        event_run_to_speed(event_get_player(0), 57.17F, 73.1F, 286.54F, 1.75F,
                           0.06F);
        event_run_to_speed(event_get_player(3), 56.67F, 73.1F, 315.3F, 1.75F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 88.67F, 76.19F, 242.29F);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 86.85F, 76.19F, 241.46F);
        event_wait(15);
        event_clear_motions(event_get_player(0));
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(0), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(3), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0xff6e, 0x62d6, 0xffa2);
        event_set_ang(event_get_player(3), 0xff6e, 0x6600, 0xffa2);
        event_wait(20);
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 83.58F, 73.74F, 264.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0276, 0xf930, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, 83.53F, 78.73F, 264.29F);
        event_voice_play(1220);
        event_message_wait(45, TEXT_1);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 83.52F, 79.73F, 264.35F);
        event_wait(20);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 57.37F, 77.57F, 284.06F);
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_set_ang(event_get_player(3), 0x0091, 0x99ba, 0xff68);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_pad(2, &ev0038_punch);
        event_wait(20);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x071c);
        event_wait(15);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 48.73F, 76.14F, 269.72F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0xf800);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0038_tails_action_1,
                          &tails_texlist, 2.0F, 0, 4);
        event_wait(5);
        event_move(event_get_player(0), event_get_player(0)->work->pos.x,
                   event_get_player(0)->work->pos.y + 0.8F,
                   event_get_player(0)->work->pos.z,
                   event_get_player(0)->work->pos.x - 30.0F,
                   event_get_player(0)->work->pos.y + 0.8F,
                   event_get_player(0)->work->pos.z + 30.0F, 15);
        event_wait(15);
        event_set_ang(event_get_player(0), 0xff6e, 0x62d6, 0xffa2);
        event_play_action(event_get_player(0), &ev0038_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(15);
        event_set_pos(event_get_player(0), 57.11F, 73.1F, 286.96F);
        event_set_pos(event_get_player(2), 87.87F, 72.71F, 252.82F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_set_pos(event_get_player(3), 86.49F, 72.12F, 369.01F);
        event_set_ang(event_get_player(3), 0x0091, 0x89dc, 0xff68);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 43.51F, 76.07F, 310.44F);
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 49.62F, 76.8F, 300.13F);
        event_clear_motions(event_get_player(2));
        event_play_pad(2, &ev0038_punch);
        event_wait(35);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0038_tails_action_1,
                          &tails_texlist, 2.0F, 0, 4);
        event_wait(5);
        event_move(event_get_player(0), event_get_player(0)->work->pos.x,
                   event_get_player(0)->work->pos.y + 0.8F,
                   event_get_player(0)->work->pos.z,
                   event_get_player(0)->work->pos.x - 30.0F,
                   event_get_player(0)->work->pos.y + 0.8F,
                   event_get_player(0)->work->pos.z + 30.0F, 15);
        event_wait(15);
        event_set_ang(event_get_player(0), 0xff6e, 0x5a00, 0xffa2);
        event_play_action(event_get_player(0), &ev0038_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(15);
        event_set_ang(event_get_player(2), 0, 0xd500, 0);
        event_play_action(event_get_player(2), &ev0038_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 45, 60.82F, 75.91F, 286.24F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 45, 0x0618, 0x65aa, 0xfeb3);
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1221);
        event_message_wait(45, TEXT_2);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 24.1F, 80.57F, 345.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff18, 0xf135, 0xfeb3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 17.72F, 80.79F, 343.08F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xff18, 0xeb35, 0xfeb3);
        event_set_pos(event_get_player(2), 54.47F, 73.5F, 296.95F);
        event_play_action(event_get_player(2), &ev0038_knuckles_action_1,
                          &knuckles_texlist, 1.0F, 0, 2);
        event_play_action(event_get_player(2), &ev0038_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(2), TEXT_7);
        event_voice_play(1596);
        event_message_wait(60, TEXT_3);
        event_talk_stop(event_get_player(2));
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_8);
        event_voice_play(1597);
        event_message_wait(90, TEXT_4);
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 47.92F, 79.74F, 266.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0118, 0x7a32, 0xfeb3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 48.92F, 79.55F, 259.81F);
        event_voice_play(1222);
        event_talk_start(event_get_player(0), TEXT_6);
        event_message_wait(50, TEXT_5);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(40);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(3);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0038_textures[] = {
    {"SONIC", &sonic_texlist},
    {"KNUCKLES", &knuckles_texlist},
    {NULL, NULL},
};
