/* EV0028, Sonic's story: Sonic and Tails meet Knuckles in Mystic Ruins.
 * Knuckles swings at Sonic and demands Sonic's emerald. Sonic is player 0,
 * Knuckles player 2 and Tails player 3 */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0028/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/knuckles.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/shapes.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 43
#define TEXT_5_LENGTH 24
#define TEXT_6_LENGTH 1
#define TEXT_7_LENGTH 3
#include "ev0028/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION knuckles_stand_action;

extern NJS_ACTION ev0028_sonic_action_0;
extern NJS_ACTION ev0028_sonic_action_1;
extern NJS_ACTION ev0028_sonic_action_2;
extern NJS_ACTION ev0028_sonic_action_3;
extern NJS_ACTION ev0028_sonic_action_4;
extern NJS_ACTION ev0028_sonic_action_5;
extern NJS_ACTION ev0028_knuckles_action_0;
extern NJS_ACTION ev0028_knuckles_action_1;
extern NJS_ACTION ev0028_knuckles_action_2;
extern NJS_ACTION ev0028_tails_action;

/* Knuckles' punches at Sonic, a key a step. B is held for keys 0 to 6, 15 to 21
 * and 33 to 48. the stick is let go throughout */
static const struct event_pad_key ev0028_punch_keys[49] = {
#include "ev0028/script/ev0028_punch_keys.inc"
};

static const struct event_pad_recording ev0028_punch = {
    ev0028_punch_keys,
    sizeof(ev0028_punch_keys) / sizeof(ev0028_punch_keys[0])};

void ev0028_script(int phase)
{
    /* @hack spelling: Knuckles' z, 252.82, through a local. SHC then loads
     * it into FR12 at 0x0cb80090 and keeps it there for all three uses. as
     * the literal at each use, ev0028_script runs 36 bytes short, and 951 of
     * its 1035 words differ */
    Float knuckles_z;
    /* @hack spelling: Knuckles' y, 72.71, through a local. SHC then loads it
     * into FR13 at 0x0cb80094 and keeps it there for all four uses. as the
     * literal at each use, 35 of ev0028_script's 1035 words differ */
    Float knuckles_y;
    /* @hack spelling: event_clear_motions called through a local, assigned just
     * before its first call. SHC then loads its address into R10 at
     * 0x0cb80158, and event_set_ang's into R9 at 0x0cb8007c. called by name,
     * the two trade R9 and R10, and 26 of ev0028_script's 1035 words
     * differ */
    void (*clear_motions)(struct task *task);
    /* @hack spelling: event_set_ang called through a local, assigned just
     * before its first call. SHC then loads its address into R9 at 0x0cb8007c.
     * called by name, ev0028_script runs 4 bytes long, and 966 of its 1035
     * words differ */
    void (*set_ang)(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_set_pos(event_get_player(0), 34.94F, 60.89F, 427.09F);
        set_ang = event_set_ang;
        set_ang(event_get_player(0), 0x0ed9, 0x79ce, 0xfbf7);
        knuckles_z = 252.82F;
        knuckles_y = 72.71F;
        event_create_player(2, knuckles_step, 87.87F, knuckles_y, knuckles_z,
                            0xff6e, 0xdef7, 0xffa2);
        event_wait(1);
        event_create_player(3, tails_step, event_get_player(0)->work->pos.x,
                            event_get_player(0)->work->pos.y,
                            event_get_player(0)->work->pos.z + 30.0F,
                            event_get_player(0)->work->ang.x,
                            0x4000 - event_get_player(0)->work->ang.y,
                            event_get_player(0)->work->ang.z);
        event_set_pos(event_get_player(0), 34.94F, 60.89F, 427.09F);
        set_ang(event_get_player(0), 0x0ed9, 0x79ce, 0xfbf7);
        event_set_pos(event_get_player(2), 87.87F, knuckles_y, knuckles_z);
        set_ang(event_get_player(2), 0xff6e, 0xdef7, 0xffa2);
        clear_motions = event_clear_motions;
        clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &tails_run_action,
                          &tails_texlist, 2.5F, EVENT_REPEAT, 0);
        stream_request_music(MUSIC_THEKNKLS);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 112.26F, 75.0F, 252.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x038c, 0x6e8a, 0);
        event_run_to_speed(event_get_player(0), 55.93F, 73.16F, 289.96F, 1.75F,
                           0.06F);
        event_run_to_speed(event_get_player(3), 55.76F, 73.23F, 320.75F, 1.75F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 88.67F, 74.2F, 242.29F);
        event_wait(110);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 86.85F, 74.2F, 241.46F);
        event_wait(13);
        clear_motions(event_get_player(0));
        clear_motions(event_get_player(3));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(3), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        set_ang(event_get_player(0), 0xff6e, 0x600c, 0xffa2);
        set_ang(event_get_player(3), 0xff6e, 0x600c, 0xffa2);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 86.88F, 74.95F, 230.49F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x058c, 0x788a, 0);
        event_wait(20);
        event_shapes_set_firm(event_get_player(0));
        event_shapes_set_firm(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 87.6F, 74.4F, 226.6F);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 83.98F, 73.85F, 261.87F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x058c, 0xf58a, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 83.98F, 78.0F, 261.87F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 45, 0x2aab);
        event_voice_play(1165);
        event_message_show(TEXT_1);
        event_wait(45);
        event_message_close();
        event_camera_look_at_task(EVENT_EASE_LINEAR, 45, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 45, 78.03F, 74.47F, 286.9F);
        clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev0028_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(3), event_get_player(2), 0.0F, 6.0F,
                           0.0F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_set_pos(event_get_player(0), 55.93F, 73.16F, 289.96F);
        event_set_pos(event_get_player(2), 87.87F, knuckles_y, knuckles_z);
        clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 80.05F, 98.81F, 315.63F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0);
        clear_motions(event_get_player(2));
        event_play_pad(2, &ev0028_punch);
        event_wait(20);
        event_play_action(event_get_player(0), &ev0028_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev0028_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(event_get_player(0), event_get_player(0)->work->pos.x,
                   event_get_player(0)->work->pos.y,
                   event_get_player(0)->work->pos.z,
                   event_get_player(0)->work->pos.x - 25.0F,
                   event_get_player(0)->work->pos.y,
                   event_get_player(0)->work->pos.z - 25.0F, 40);
        event_wait(10);
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_LINEAR, 30, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_wait(50);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 42.43F, 76.95F, 258.59F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x03a9, 0x580f, 0);
        event_set_pos(event_get_player(2), 42.96F, knuckles_y, 301.29F);
        set_ang(event_get_player(2), 0xff6e, 0x89c2, 0xffa2);
        set_ang(event_get_player(3), 0xff6e, 0x89c2, 0xffa2);
        event_play_action(event_get_player(2), &ev0028_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1166);
        event_wait(15);
        event_message_show(TEXT_2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 46.81F, 81.33F, 241.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xfca9, 0x730f, 0);
        event_play_action(event_get_player(0), &ev0028_sonic_action_3,
                          &sonic_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(0));
        set_ang(event_get_player(0), 0xff6e, 0x0cf2, 0xffa2);
        event_play_action(event_get_player(0), &ev0028_sonic_action_2,
                          &sonic_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev0028_sonic_action_4,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_6);
        event_voice_play(1596);
        event_message_show(TEXT_3);
        event_wait(5);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_wait(5);
        event_shapes_clear_firm_players(event_get_player(0));
        event_shapes_clear_firm_players(event_get_player(2));
        event_play_action(event_get_player(2), &ev0028_knuckles_action_1,
                          &knuckles_texlist, 1.0F, 0, 2);
        event_play_action(event_get_player(2), &ev0028_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev0028_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 16);
        event_talk_start(event_get_player(2), TEXT_7);
        event_voice_play(1597);
        event_message_show(TEXT_4);
        event_wait(70);
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 66.42F, 79.57F, 306.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfda9, 0x200f, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 60.74F, 79.57F, 311.63F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0xfda9, 0x1d0f, 0);
        event_message_show(TEXT_5);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_wait(10);
        event_voice_play(1167);
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_6);
        event_wait(32);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_wait(90);
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

struct texture_pvm ev0028_textures[] = {
    {"MILES", &tails_texlist},
    {"KNUCKLES", &knuckles_texlist},
    {NULL, NULL},
};
