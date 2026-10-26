/* EV002A, Sonic's story: Sonic and Tails count the Chaos Emeralds they hold.
 * Sonic is player 0 and Tails player 2 */
#include <ninja.h>

#include "ev002a/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 33
#define TEXT_2_LENGTH 48
#define TEXT_3_LENGTH 22
#define TEXT_4_LENGTH 56
#define TEXT_5_LENGTH 31
#define TEXT_6_LENGTH 18
#define TEXT_7_LENGTH 2
#define TEXT_8_LENGTH 3
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 82
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 2
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 2
#define TEXT_15_LENGTH 17
#include "ev002a/script/text.inc"

extern NJS_ACTION sonic_stand_action;

extern NJS_ACTION ev002a_sonic_action_0;
extern NJS_ACTION ev002a_sonic_action_1;
extern NJS_ACTION ev002a_sonic_action_2;
extern NJS_ACTION ev002a_sonic_action_3;
extern NJS_ACTION ev002a_sonic_action_4;
extern NJS_ACTION ev002a_tails_action_0;
extern NJS_ACTION ev002a_tails_action_1;
extern NJS_ACTION ev002a_tails_action_2;
extern NJS_ACTION ev002a_tails_action_3;
extern NJS_ACTION ev002a_tails_action_4;
extern NJS_ACTION ev002a_tails_action_5;
extern NJS_ACTION ev002a_tails_action_6;

static struct task *ev002a_fade = NULL;

struct texture_pvm ev002a_textures[] = {
    {"MILES", &tails_texlist},
    {NULL, NULL},
};

void ev002a_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        ev002a_fade = event_fade_create(1.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_wait(1);
        event_set_pos(event_get_player(0), -535.4F, -2.0F, 946.5F);
        event_set_ang(event_get_player(0), 0, 0xfa26, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev002a_sonic_action_2,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_6);
        event_create_player(2, tails_step, -526.0F, -2.0F, 941.0F, 0, 0xf200,
                            0);
        event_wait(1);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev002a_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(2), TEXT_6);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -544.9645F, 23.58F, 969.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe511, 0xed00, 0xff00);
        event_wait(20);
        if (ev002a_fade != NULL) {
            event_fade_set_speed(ev002a_fade, -1.0F / 60.0F);
        }
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -540.5F, 15.4F, 960.3F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x238e);
        event_wait(50);
        event_talk_start(event_get_player(0), TEXT_7);
        event_wait(5);
        event_talk_start(event_get_player(2), TEXT_8);
        event_wait(27);
        event_play_action(event_get_player(0), &ev002a_sonic_action_3,
                          &sonic_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(15);
        event_play_action(event_get_player(2), &ev002a_tails_action_1,
                          &tails_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(2));
        event_set_pos(event_get_player(2), -527.3F, -2.0F, 944.7F);
        event_play_action(event_get_player(2), &ev002a_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_play_action(event_get_player(2), &ev002a_tails_action_3,
                          &tails_texlist, 1.0F, 0, 8);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 3.0F,
                           0.0F);
        event_play_action(event_get_player(2), &ev002a_tails_action_4,
                          &tails_texlist, 1.3F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -524.78F, 1.8466F, 956.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0923, 0x1600, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -539.94F, 2.23F, 955.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0x0923, 0xe800, 0xff00);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 3.0F,
                           15.0F);
        event_talk_stop(event_get_player(0));
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_9);
        event_voice_play(997);
        /* @hack redundant: a never-run call of event_talk_stop, which SHC still
         * counts when it allocates registers. SHC then loads event_set_pos's
         * address into R10 at 0x0cb80026, and event_camera_set_pos's into R9 at
         * 0x0cb80128. R10 takes event_talk_stop's at 0x0cb8035e. without it, R9
         * and R10 trade, and 23 of ev002a_script's 676 words differ */
        if (0) {
            event_talk_stop(event_get_player(0));
        }
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_10);
        event_play_action(event_get_player(0), &ev002a_sonic_action_4,
                          &sonic_texlist, 0.4F, 0, 8);
        event_message_close();
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev002a_sonic_action_0,
                          &sonic_texlist, 0.15F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, -544.8F, 0.23F, 962.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 240, 0x0923, 0xe800, 0xff00);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_11);
        event_voice_play(998);
        event_message_wait(1, TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_voice_play(999);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -516.5F, -0.535F, 959.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0923, 0x2400, 0xff00);
        event_look_clear(event_get_player(0));
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_13);
        event_voice_play(1000);
        event_message_show(TEXT_4);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -514.6F, -0.6F, 956.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0923, 0x2900, 0xff00);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_voice_play(1001);
        event_talk_start(event_get_player(2), TEXT_14);
        event_message_show(TEXT_5);
        event_play_action(event_get_player(2), &ev002a_tails_action_5,
                          &tails_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(2), &ev002a_tails_action_6,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev002a_sonic_action_1,
                          &sonic_texlist, 0.6F, EVENT_REPEAT, 12);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, -527.05F, 1.226F, 953.13F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x0c23, 0x2400, 0xff00);
        event_wait(30);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_15);
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), -535.4F, -2.0F, 946.5F);
        event_set_ang(event_get_player(0), 0, 0xfa26, 0);
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev002a_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}
