/* EV0063, Amy's story: Amy catches up with Sonic at Twinkle Park as Zero closes
 * in */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/twinkle_elevator.h"
#include "enemy/zero.h"
#include "ev0063/amy_action_1.h"
#include "ev0063/bird_action.h"
#include "ev0063/script.h"
#include "event/event.h"
#include "event/play.h"
#include "object/hint.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 22
#define TEXT_2_LENGTH 45
#define TEXT_3_LENGTH 32
#define TEXT_4_LENGTH 51
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 15
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 69
#define TEXT_9_LENGTH 32
#define TEXT_10_LENGTH 27
#define TEXT_11_LENGTH 27
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 5
#define TEXT_15_LENGTH 4
#define TEXT_16_LENGTH 2
#define TEXT_17_LENGTH 1
#define TEXT_18_LENGTH 6
#define TEXT_19_LENGTH 4
#define TEXT_20_LENGTH 4
#define TEXT_21_LENGTH 4
#include "ev0063/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_ready_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION amy_trot_action;
extern NJS_ACTION amy_stop_action;
extern NJS_ACTION amy_stand_action;

extern NJS_ACTION ev0063_sonic_action_0;
extern NJS_ACTION ev0063_sonic_action_1;
extern NJS_ACTION ev0063_sonic_action_2;
extern NJS_ACTION ev0063_sonic_action_3;
extern NJS_ACTION ev0063_sonic_action_4;
extern NJS_ACTION ev0063_sonic_action_5;
extern NJS_ACTION ev0063_sonic_action_6;
extern NJS_ACTION ev0063_sonic_action_7;
extern NJS_ACTION ev0063_sonic_action_8;
extern NJS_ACTION ev0063_amy_action_0;

/* port 0's keys as Amy heads for the park, one a step. the stick is pushed one
 * way for 85 steps, then little or not at all for 3. it is then pushed the
 * opposite way for 15 steps, and let go for the last 65 */
static const struct event_pad_key ev0063_walk_keys[168] = {
#include "ev0063/script/ev0063_walk_keys.inc"
};

static const struct event_pad_recording ev0063_walk = {
    ev0063_walk_keys, sizeof(ev0063_walk_keys) / sizeof(ev0063_walk_keys[0])};

static struct task *ev0063_fade = NULL;
static struct task *ev0063_bird = NULL;
/* FLAG_HINTS_IN_EVENTS as the event found it. EVENT_END puts it back while
 * ev0063_hints_saved is TRUE */
static Uint8 ev0063_hints_before = 0;
static Uint8 ev0063_hints_saved = FALSE;

void ev0063_script(int phase)
{
    /* @hack spelling: event_talk_stop called through a local. SHC then keeps
     * 1.5 in FR14 (0x0cb80134) and 3.0 in FR13 (0x0cb80810). called by name,
     * 1.5 and 3.0 trade registers, and 17 of ev0063_script's 2420 words
     * differ */
    void (*talk_stop)(struct task *task);
    /* @hack spelling: player 0 through a local, for one call. SHC then
     * keeps 1.5 in FR14 (0x0cb80134) and 3.0 in FR13 (0x0cb80810). read in
     * place, 1.5 and 3.0 trade registers, and 17 of ev0063_script's 2420 words
     * differ */
    struct task *amy;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        ev0063_hints_before = event_flags[FLAG_HINTS_IN_EVENTS];
        ev0063_hints_saved = TRUE;
        event_flags[FLAG_HINTS_IN_EVENTS] = TRUE;
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_reset_player(2);
        sound_load_file("e_0064.mlt");
        event_sound_init(2);
        event_create_player(2, sonic_step, 323.0F, 0.0F, 1639.0F, 0, 0x7400, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 329.4F, 0.0F, 1617.0F);
        event_set_ang(event_get_player(2), 0, 0x3a00, 0);
        event_set_pos(event_get_player(0), 333.5F, 0.0F, 1545.8F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_play_action(event_get_player(2), &ev0063_sonic_action_5,
                          &sonic_texlist, 0.45F, EVENT_REPEAT, 0);
        /* @bug logic: gives the bird angles from Amy's position, here and in
         * its eight event_set_ang calls below. seven of those calls add an
         * angle, from 0x1400 to 0xc000, to her y. each prototype converts her
         * position implicitly to Angle (FTRC at 0x0cb80140 to 0x0cb80160
         * before this call's JSR at 0x0cb8019c, and at 0x0cb80276 to
         * 0x0cb8029a before the first event_set_ang's JSR at 0x0cb8029e). her
         * x and z tilt the bird by about 2 and 9 degrees. her y, 0 or 50, is
         * too small a turn to see */
        event_place_actor(&ev0063_bird, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y + 2.5F,
                          event_get_player(0)->work->pos.z + 1.5F,
                          event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z);
        event_wait(1);
        event_set_flags(ev0063_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev0063_bird, &ev0063_bird_action,
                          &ev0063_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 8.0F,
                      event_get_player(0)->work->pos.z + 1.5F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z);
        event_create_player(3, zero_event_init, 395.1F, 12.0F, 1201.0F, 0,
                            0xc000, 0);
        event_wait(1);
        event_set_pos(event_get_player(3), 395.1F, 12.0F, 1200.0F);
        event_set_ang(event_get_player(3), 0, 0xc000, 0);
        event_flags[FLAG_ELEVATOR] = ELEVATOR_TO_BOTTOM;
        ev0063_fade =
            event_fade_create(EVENT_FADE_STILL, 0.0F, 0.0F, 0.0F, 0.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 326.5F, 14.8F, 1540.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf33a, 0x84b9, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 326.5F, 9.2F, 1540.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xf73a, 0x84b9, 0xff00);
        amy = event_get_player(0);
        event_clear_motions(amy);
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          1.5F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0),
                           event_get_player(2)->work->pos.x + 10.0F,
                           event_get_player(2)->work->pos.y,
                           event_get_player(2)->work->pos.z - 4.0F, 0.6F, 0.4F);
        event_move(ev0063_bird, 330.7F, 2.7F, 1545.0F,
                   event_get_player(2)->work->pos.x + 3.2F,
                   event_get_player(2)->work->pos.y + 8.3F,
                   event_get_player(2)->work->pos.z - 9.0F, 125);
        event_wait(1);
        event_voice_play(1402);
        event_message_show(TEXT_1);
        event_wait(40);
        event_play_action(event_get_player(2), &ev0063_sonic_action_6,
                          &sonic_texlist, 0.75F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 351.9F, 2.8F, 1610.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x06b5, 0x0ec7, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 65, 0x06b5, 0x40c7, 0);
        event_set_ang(event_get_player(2), 0, 0x3400, 0);
        talk_stop = event_talk_stop;
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(1403);
        event_message_show(TEXT_2);
        talk_stop(event_get_player(0));
        event_run_wait(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0xea00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 346.5F, 2.8F, 1621.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x06b5, 0x27db, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stop_action, &amy_texlist,
                          0.8F, 0, 8);
        event_wait_motions(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(20);
        event_wait(40);
        event_message_close();
        event_set_ang(event_get_player(2), 0, 0x3000, 0);
        event_set_ang(event_get_player(0), 0, 0xd000, 0);
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x + 6.0F,
                      event_get_player(0)->work->pos.y + 4.0F,
                      event_get_player(0)->work->pos.z + 1.0F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 0x9000,
                      event_get_player(0)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 337.4F, 2.3F, 1594.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfeb5, 0x7bc7, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 336.4F, 4.5F, 1593.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0xfeb5, 0x7cc7, 0);
        event_wait(30);
        event_play_action(event_get_player(2), &ev0063_sonic_action_1,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 8);
        event_wait(35);
        event_look_at_task(event_get_player(0), event_get_player(3), -20.0F,
                           10.0F, 0.0F);
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_wait(30);
        event_voice_play(1404);
        event_message_show(TEXT_3);
        event_wait(10);
        talk_stop(event_get_player(0));
        event_set_pos(event_get_player(3), 331.8F, 12.0F, 1336.2F);
        event_set_ang(event_get_player(3), 0, 0, 0);
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x + 7.0F,
                      event_get_player(0)->work->pos.y + 3.0F,
                      event_get_player(0)->work->pos.z + 0.8F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 0xa000,
                      event_get_player(0)->work->pos.z);
        event_run_to_speed(event_get_player(3), 326.8F, 12.0F, 1547.54F, 0.5F,
                           0.1F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 334.8F, 4.1F, 1626.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d90, 0x002d, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 334.8F, 1.27F, 1625.6F);
        stream_request_music(MUSIC_EGGROBO);
        event_set_ang(event_get_player(2), 0, 0x7d00, 0);
        event_set_ang(event_get_player(0), 0, 0x9200, 0);
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_14);
        event_wait(60);
        talk_stop(event_get_player(0));
        event_clear_motions(event_get_player(2));
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x + 6.0F,
                      event_get_player(0)->work->pos.y + 3.0F,
                      event_get_player(0)->work->pos.z + 2.2F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 0x4000,
                      event_get_player(0)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 344.1F, 4.3F, 1596.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf9b5, 0x52c7, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 346.1F, 6.7F, 1616.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x01b5, 0x3ec7, 0);
        event_set_pos(event_get_player(0), 338.9F, 0.0F, 1617.2F);
        event_set_ang(event_get_player(0), 0, 0x4500, 0);
        event_set_pos(event_get_player(2), 329.4F, 0.0F, 1608.2F);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), 363.1F, 22.8F, 1612.2F);
        event_look_clear(event_get_player(2));
        talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1046);
        event_message_show(TEXT_4);
        event_play_action(event_get_player(2), &ev0063_sonic_action_0,
                          &sonic_texlist, 0.5F, 0, 8);
        event_play_action(event_get_player(2), &ev0063_sonic_action_7,
                          &sonic_texlist, 0.425F, 0, 8);
        event_play_action(event_get_player(2), &ev0063_sonic_action_0,
                          &sonic_texlist, 0.25F, 0, 8);
        event_play_action(event_get_player(2), &ev0063_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_wait(90);
        talk_stop(event_get_player(2));
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_voice_play(1405);
        event_message_show(TEXT_5);
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        talk_stop(event_get_player(0));
        event_wait(10);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 5.0F,
                           0.0F);
        event_play_action(event_get_player(0), &ev0063_amy_action_0,
                          &amy_texlist, 1.5F, EVENT_REPEAT, 8);
        talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_16);
        event_voice_play(1047);
        event_message_show(TEXT_6);
        event_wait(30);
        talk_stop(event_get_player(2));
        event_message_close();
        stream_stop_music();
        stream_request_music(MUSIC_THEAMY);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 352.5F, 44.78F, 1607.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x00b5, 0xc0c7, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 313.11F, 15.53F, 1605.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0cb5, 0xbf2c, 0x0100);
        event_set_pos(event_get_player(0), 366.98F, 0.0F, 1575.95F);
        event_set_ang(event_get_player(0), 0, 0x454c, 0);
        event_look_at(event_get_player(0), 360.0F, 9.5F, 1587.0F);
        event_set_ang(event_get_player(2), 0, 0x5000, 0);
        event_look_clear(event_get_player(2));
        event_play_action(event_get_player(2), &ev0063_sonic_action_8,
                          &sonic_texlist, 0.6F, 0, 8);
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.2F, EVENT_REPEAT, 8);
        talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_17);
        event_voice_play(1048);
        event_message_show(TEXT_7);
        event_wait(120);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 311.0F, 5.8F, 1603.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0eb5, 0xb9c7, 0);
        event_wait(15);
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x + 4.0F,
                      event_get_player(0)->work->pos.y + 3.0F,
                      event_get_player(0)->work->pos.z + 2.0F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 0x4000,
                      event_get_player(0)->work->pos.z);
        event_wait(15);
        event_play_action(event_get_player(0), &ev0063_amy_action_0,
                          &amy_texlist, 1.1F, EVENT_REPEAT, 16);
        event_wait(15);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.1F, EVENT_REPEAT, 16);
        talk_stop(event_get_player(2));
        event_wait(55);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x08b5, 0xc3c7, 0);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 361.09F, 6.15F, 1570.15F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1fb5, 0xabc7, 0x1300);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 356.1F, 0.53F, 1567.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0x1fb5, 0xabc7, 0x1600);
        event_set_ang(event_get_player(2), 0, 0x6500, 0);
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_18);
        event_voice_play(1406);
        event_message_show(TEXT_8);
        event_wait(170);
        talk_stop(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(2));
        event_play_action(event_get_player(2), &ev0063_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 357.1F, 10.2F, 1609.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf2b5, 0x2cc7, 0x0200);
        event_set_ang(event_get_player(2), 0, 0x4000, 0);
        event_set_pos(event_get_player(0), 345.4F, 0.0F, 1596.95F);
        event_set_ang(event_get_player(0), 0, 0xdb00, 0);
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x + 5.5F,
                      event_get_player(0)->work->pos.y + 3.0F,
                      event_get_player(0)->work->pos.z + 5.0F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 0xc000,
                      event_get_player(0)->work->pos.z);
        event_follow_turn(ev0063_bird, 4.5F, 3.0F, 5.0F, 0, 0x5000, 0, 200,
                          event_get_player(0));
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 5.0F,
                           0.0F);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 5.0F,
                           0.0F);
        event_set_pos(event_get_player(3), 326.8F, 0.0F, 1330.54F);
        event_play_action(event_get_player(0), &ev0063_amy_action_0,
                          &amy_texlist, 1.5F, EVENT_REPEAT, 8);
        talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(1407);
        event_message_show(TEXT_9);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 359.4F, 4.3F, 1611.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0xfeb5, 0x26c7, 0x0200);
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          1.1F, EVENT_REPEAT, 8);
        event_play_pad(0, &ev0063_walk);
        talk_stop(event_get_player(0));
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x + 4.5F,
                      event_get_player(0)->work->pos.y + 3.0F,
                      event_get_player(0)->work->pos.z + 5.0F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 0x8000,
                      event_get_player(0)->work->pos.z);
        talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_20);
        event_voice_play(1049);
        event_message_show(TEXT_10);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0063_sonic_action_4,
                          &sonic_texlist, 0.6F, 0, 8);
        event_wait_motions(event_get_player(2));
        event_play_action(event_get_player(2), &sonic_ready_action,
                          &sonic_texlist, 1.2F, EVENT_REPEAT, 0);
        event_wait(45);
        event_play_action(event_get_player(2), &ev0063_sonic_action_3,
                          &sonic_texlist, 0.5F, 0, 0);
        event_play_action(event_get_player(2), &sonic_ready_action,
                          &sonic_texlist, 1.5F, EVENT_REPEAT, 0);
        talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_21);
        event_voice_play(1050);
        event_message_show(TEXT_11);
        talk_stop(event_get_player(2));
        event_wait(40);
        event_play_action(event_get_player(2), &sonic_trot_action,
                          &sonic_texlist, 1.2F, EVENT_REPEAT, 8);
        event_wait(25);
        event_run_to_speed(
            event_get_player(2), event_get_player(0)->work->pos.x - 5.0F,
            event_get_player(0)->work->pos.y,
            event_get_player(0)->work->pos.z - 5.0F, 0.7F, 0.06F);
        event_play_action(event_get_player(0), &ev0063_amy_action_0,
                          &amy_texlist, 1.5F, EVENT_REPEAT, 8);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 341.5F, 0.99F, 1587.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0ffe, 0xafb9, 0xff00);
        /* @bug logic: gives Sonic angles from Amy's position, as the bird's
         * calls do. the prototype converts it implicitly to Angle (FTRC at
         * 0x0cb81522 to 0x0cb81542, before the JSR at 0x0cb81552) */
        event_set_ang(event_get_player(2), event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y,
                      event_get_player(0)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 318.7F, 59.0F, 1579.8F);
        event_wait(30);
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.0F, 0, 8);
        event_sound_start(1, 1223, 1800);
        event_sound_fade_volume(1, 110, 1);
        event_sound_fade_pan(1, -20, 1);
        event_flags[FLAG_ELEVATOR] = ELEVATOR_GO;
        event_wait(60);
        event_sound_fade_volume(1, -50, 30);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 411.28F, 63.2F, 1671.4F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_sound_stop(1);
        event_set_pos(event_get_player(0), 399.8F, 50.0F, 1650.54F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_set_pos(ev0063_bird, event_get_player(0)->work->pos.x + 8.5F,
                      event_get_player(0)->work->pos.y + 3.0F,
                      event_get_player(0)->work->pos.z - 4.0F);
        event_set_ang(ev0063_bird, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 0x1400,
                      event_get_player(0)->work->pos.z);
        event_follow(ev0063_bird, 5.5F, 3.0F, -4.0F, 200, event_get_player(0));
        event_play_action(event_get_player(0), &ev0063_amy_action_1,
                          &amy_texlist, 1.2F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 409.28F, 70.2F, 1667.4F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          1.1F, EVENT_REPEAT, 16);
        event_wait(5);
        event_run_to_speed(event_get_player(0), 401.3F, 50.0F, 1742.8F, 0.8F,
                           0.6F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 409.5F, 56.4F, 1655.0F);
        event_message_close();
        if (ev0063_fade != NULL) {
            event_fade_set_speed(ev0063_fade, 0.02F);
        }
        event_wait(60);
        event_run_wait(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_camera_look_clear();
        event_move_stop_all();
        break;
    case EVENT_END:
        if (ev0063_hints_saved) {
            event_flags[FLAG_HINTS_IN_EVENTS] = ev0063_hints_before;
            ev0063_hints_saved = FALSE;
        }
        event_reset_player(0);
        event_reset_player(2);
        event_reset_player(3);
        event_voice_stop();
        event_sound_stop_all();
        event_move_stop(ev0063_bird);
        event_move_stop_all();
        event_kill_player(2);
        event_kill_player(3);
        event_kill_task(&ev0063_bird);
        if (ev0063_fade != NULL) {
            task_kill(ev0063_fade);
            ev0063_fade = NULL;
        }
        event_set_pos(event_get_player(0), 547.5F, 50.5F, 1771.6F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0063_textures[] = {
    {"SONIC", &sonic_texlist},
    {"VER2_WING", &ev0063_bird_texlist},
    {"EGGROB", &zero_texlist},
    {NULL, NULL},
};
