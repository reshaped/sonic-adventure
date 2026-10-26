/* EV00F9: Sonic comes to beside Tails, who tells him the last Chaos Emerald
 * should be in the Tornado 2 */
#include <ninja.h>

#include "ev00f9/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 22
#define TEXT_2_LENGTH 14
#define TEXT_3_LENGTH 29
#define TEXT_4_LENGTH 49
#define TEXT_5_LENGTH 34
#define TEXT_6_LENGTH 48
#define TEXT_7_LENGTH 15
#define TEXT_8_LENGTH 51
#define TEXT_9_LENGTH 2
#define TEXT_10_LENGTH 5
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 2
#define TEXT_13_LENGTH 3
#define TEXT_14_LENGTH 3
#define TEXT_15_LENGTH 3
#include "ev00f9/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_stand_action;

extern NJS_ACTION ev00f9_sonic_action;
extern NJS_ACTION ev00f9_tails_action_0;
extern NJS_ACTION ev00f9_tails_action_1;

static struct task *ev00f9_white = NULL;
static struct task *ev00f9_flash = NULL;

void ev00f9_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_init(1);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        ev00f9_white = event_fade_create(1.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_set_pos(event_get_player(0), -47.2327F, -263.3F, 2626.66F);
        event_set_ang(event_get_player(0), 0x0800, 0x0d00, 0);
        event_reset_player(0);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at(event_get_player(0), -45.22F, -260.0F, 2632.0F);
        /* @bug logic: this look never holds (JSR at 0x0cb80110). Tails is
         * made on the next line (JSR at 0x0cb8012c), and with no Tails,
         * event_look_at_task does nothing. a Tails made already would lose
         * the look to the event_reset_player(2) after the create */
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 6.0F,
                           0.0F);
        event_create_player(2, tails_step, -45.228F, -263.1676F, 2639.0665F,
                            0x0884, 0x8600, 0x0044);
        event_reset_player(2);
        event_wait(1);
        event_play_action(event_get_player(2), &tails_stand_action,
                          &tails_texlist, 0.7F, EVENT_REPEAT, 0);
        event_voice_play(1964);
        event_message_show(TEXT_1);
        event_wait(10);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then keeps 0.0F in
         * FR13 (0x0cb800f8). as an if, SHC keeps it in no register,
         * ev00f9_script runs 16 bytes short, and 585 of 705 words differ */
        ev00f9_white != NULL &&
            (event_fade_set_speed(ev00f9_white, -1.0F / 120.0F), 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -47.595F, -261.857F,
                             2635.345F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1b00, 0xfd00, 0x1600);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -47.363F, -259.4F,
                             2632.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x1b00, 0xfd00, 0xf200);
        event_wait(50);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1b00, 0xfd00, 0x0f00);
        event_wait(30);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x1b00, 0xfd00, 0x0800);
        event_message_close();
        event_look_clear(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_9);
        event_voice_play(1965);
        event_message_show(TEXT_2);
        event_wait(20);
        event_talk_start(event_get_player(0), TEXT_10);
        event_wait(1);
        event_talk_start(event_get_player(2), TEXT_11);
        event_voice_wait();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x6aab);
        ev00f9_flash = event_fade_create(0.5F, 0.01F, 1.0F, 1.0F, 1.0F);
        event_wait(1);
        /* @hack spelling: the test and the call as one expression, as
         * EVENT_KILL has them. SHC then keeps 0x0800 in R11, event_voice_play's
         * address in R9 and event_message_show's in R10 (0x0cb80024,
         * 0x0cb80152, 0x0cb8015a). as an if, the three move to R9, R10 and R11
         * in that order, ev00f9_script runs 4 bytes short, and 580 of 705
         * words differ */
        ev00f9_flash != NULL && (event_fade_set_speed(ev00f9_flash, 0.0F), 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -45.99F, -259.466F,
                             2633.88F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x8600, 0x0100);
        event_camera_set_fov(EVENT_EASE_LINEAR, 80, 0x3111);
        if (ev00f9_flash != NULL) {
            event_fade_set_speed(ev00f9_flash, -1.0F / 90.0F);
        }
        event_wait(83);
        event_voice_play(1967);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_12);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_wait(20);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -48.6F, -263.5F, 2643.36F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1100, 0xfa00, 0x0300);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -52.8F, -257.1F, 2644.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0xfb00, 0xe900, 0x0300);
        event_voice_play(1970);
        event_talk_start(event_get_player(2), TEXT_13);
        event_message_show(TEXT_4);
        event_wait(20);
        event_look_at(event_get_player(0), -51.0F, -260.0F, 2641.0F);
        event_wait(5);
        event_look_at(event_get_player(0), -37.0F, -260.0F, 2642.0F);
        event_wait(15);
        event_look_at(event_get_player(0), -51.0F, -260.0F, 2641.0F);
        event_wait(10);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_voice_wait();
        event_wait(20);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1968);
        event_message_show(TEXT_5);
        event_wait(50);
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -55.57F, -260.4F, 2623.68F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0xa700, 0x0300);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 250, -57.3F, -260.18F, 2626.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 250, 0x0500, 0xac00, 0x0700);
        event_voice_play(1972);
        event_talk_start(event_get_player(2), TEXT_14);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(0), &ev00f9_sonic_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 12);
        event_voice_play(1973);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_talk_start(event_get_player(2), TEXT_15);
        event_voice_play(1974);
        event_message_show(TEXT_8);
        event_wait(20);
        event_play_action(event_get_player(2), &ev00f9_tails_action_0,
                          &tails_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(2), &ev00f9_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_voice_wait();
        event_message_close();
        event_sound_stop(0);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), -47.2327F, -263.3F, 2626.66F);
        event_set_ang(event_get_player(0), 0x0800, 0x0d00, 0);
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev00f9_white);
        EVENT_KILL(ev00f9_flash);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f9_textures[] = {
    {"MILES", &tails_texlist},
    {NULL, NULL},
};
