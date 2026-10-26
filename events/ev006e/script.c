/* EV006E, Amy's story: Amy and the bird come upon a bridge she doesn't
 * remember. she means to see where it leads */
#include <ninja.h>

#include "ev006e/amy_action_3.h"
#include "ev006e/bird_action.h"
#include "ev006e/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 29
#define TEXT_2_LENGTH 30
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 19
#define TEXT_5_LENGTH 3
#define TEXT_6_LENGTH 2
#define TEXT_7_LENGTH 2
#include "ev006e/script/text.inc"

extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_trot_action;

/* @hack type: ev006e_amy_action_0 declared as an array, which script.c passes
 * by its name. SHC then loads event_wait's address into R14 at 0x0cb80022 and
 * event_get_player's into R13 at 0x0cb80026. as one NJS_ACTION passed as
 * &ev006e_amy_action_0, event_wait's goes to R13 and event_get_player's to R14,
 * and their pool words at 0x0cb80058 and 0x0cb8005c swap. 64 of ev006e_script's
 * 399 words then differ */
extern NJS_ACTION ev006e_amy_action_0[];
extern NJS_ACTION ev006e_amy_action_1;
extern NJS_ACTION ev006e_amy_action_2;

static struct task *ev006e_bird = NULL;

void ev006e_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_THEAMY);
        event_place_actor(&ev006e_bird, 74.42F, 7.0F, -1743.58F, 0, 0x5351, 0);
        event_wait(1);
        event_set_flags(ev006e_bird, EVENT_IGNORE_GROUND);
        event_set_pos(ev006e_bird, 79.22F, 6.01F, -1635.81F);
        event_set_ang(ev006e_bird, 0, 0x9e6c, 0);
        event_clear_motions(ev006e_bird);
        event_play_action(ev006e_bird, &ev006e_bird_action,
                          &ev006e_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), 73.04F, -0.41F, -1625.85F);
        event_set_ang(event_get_player(0), 0, 0x85be, 0);
        event_wait(1);
        event_run_to_speed(event_get_player(0), 72.8F, -0.41F, -1636.2F, 1.0F,
                           0.06F);
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 56.27F, 6.69F, -1611.07F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff28, 0xee6c, 0);
        event_wait(8);
        event_play_action(event_get_player(0), ev006e_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 67.96F, 5.95F, -1645.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xff28, 0x976c, 0);
        event_wait(50);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1500);
        event_message_wait(0, TEXT_1);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -13.79F, 36.59F, -1577.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf828, 0xed6c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -13.6F, 38.79F, -1577.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xf828, 0xf1e6, 0);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 67.96F, 5.95F, -1645.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff28, 0x976c, 0);
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1501);
        event_message_wait(0, TEXT_2);
        event_play_action(event_get_player(0), &ev006e_amy_action_1,
                          &amy_texlist, 1.0F, 0, 16);
        event_wait(40);
        event_play_action(event_get_player(0), &ev006e_amy_action_2,
                          &amy_texlist, 1.0F, 0, 8);
        event_wait(30);
        event_play_action(event_get_player(0), &ev006e_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_message_close();
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(60);
        event_message_wait(0, TEXT_3);
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_7);
        event_voice_play(1502);
        event_message_wait(0, TEXT_4);
        event_wait(60);
        event_message_close();
        event_wait(10);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_kill_task(&ev006e_bird);
        event_set_pos(event_get_player(0), 72.8F, -0.41F, -1636.2F);
        event_set_ang(event_get_player(0), 0, 0x85be, 0);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev006e_textures[] = {
    {"VER2_WING", &ev006e_bird_texlist},
    {NULL, NULL},
};
