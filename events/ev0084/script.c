/* EV0084, Knuckles' story: out of a white flash, Knuckles finds himself in the
 * past. he does not know where he is, yet feels he knows the place */
#include <ninja.h>

#include "ev0084/knuckles_action.h"
#include "ev0084/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 23
#define TEXT_3_LENGTH 41
#define TEXT_4_LENGTH 29
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 3
#define TEXT_7_LENGTH 2
#define TEXT_8_LENGTH 4
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 6
#include "ev0084/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

/* the white flash the event opens with */
static struct task *ev0084_flash = NULL;

void ev0084_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_start(0, SE_TIKAL_CHIME, 90);
        ev0084_flash = event_fade_create(1.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_talk_start(event_get_player(0), TEXT_5);
        event_wait(1);
        event_set_pos(event_get_player(0), 2.46F, 100.5F, -723.85F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_play_action(event_get_player(0), &ev0084_knuckles_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.98F, 110.25F, -710.814F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf100, 0, 0);
        event_wait(29);
        /* the flash clears over the next 60 steps */
        /* @hack spelling: the test and the call as one expression. SHC then
         * keeps event_camera_set_pos's address in R11 (0x0cb800ca). as an if,
         * SHC holds 0 in R11 and the address in R10, script.c's code ends 4
         * bytes short of the image's, and 329 of 415 words differ */
        ev0084_flash != NULL ? event_fade_set_speed(ev0084_flash, -1.0F / 60.0F)
                             : (void)0;
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 3.0F, 113.85F, -701.485F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xf100, 0, 0);
        event_wait(80);
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(8);
        event_talk_start(event_get_player(0), TEXT_6);
        event_wait(80);
        event_talk_start(event_get_player(0), TEXT_7);
        event_voice_play(1549);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.747F, 110.212F, -716.745F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0x4700, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0xfc00, 0xa800, 0);
        event_wait(160);
        event_talk_start(event_get_player(0), TEXT_8);
        event_voice_play(1550);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.579F, 107.016F, -709.53F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0xef00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -0.728F, 106.526F,
                             -715.896F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0100, 0xef00, 0);
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_9);
        event_voice_play(1551);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.345F, 108.173F, -741.109F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 2.345F, 108.173F,
                             -757.108F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0800, 0x8000, 0);
        event_talk_start(event_get_player(0), TEXT_10);
        event_voice_play(1552);
        event_message_show(TEXT_4);
        EVENT_KILL(ev0084_flash);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        if (ev0084_flash != NULL) {
            task_kill(ev0084_flash);
            ev0084_flash = NULL;
        }
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0084_textures[] = {{NULL, NULL}};
