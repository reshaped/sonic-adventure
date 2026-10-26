/* EV006F, Amy's story: Zero drops down on Amy again, and she runs off with the
 * bird */
#include <ninja.h>

#include "enemy/zero.h"
#include "ev006f/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 19
#define TEXT_3_LENGTH 5
#include "ev006f/script/text.inc"

/* @hack type: amy_stand_action declared as an array, which script.c passes by
 * its name. SHC then loads event_camera_set_pos's address into R9 at 0x0cb8011a
 * and event_camera_set_ang's into R10 at 0x0cb80134. as one NJS_ACTION passed
 * as &amy_stand_action, event_camera_set_pos's goes to R10 and
 * event_camera_set_ang's to R9, and 12 of ev006f_script's 543 words differ */
extern NJS_ACTION amy_stand_action[];
extern NJS_ACTION amy_trot_action;

extern NJS_ACTION ev006f_amy_action_0;
extern NJS_ACTION ev006f_amy_action_1;
extern NJS_ACTION ev006f_amy_action_2;
extern NJS_ACTION ev006f_amy_action_3;
extern NJS_TEXLIST ev006f_bird_texlist;
extern NJS_ACTION ev006f_bird_action;

static struct task *ev006f_bird = NULL;

void ev006f_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        sound_load_file("e_000d.mlt");
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_place_actor(&ev006f_bird, 174.92F, -160.97F, 522.31F, 0, 0x8000,
                          0);
        event_wait(1);
        event_set_pos(ev006f_bird, 174.92F, -155.97F, 522.31F);
        event_set_ang(ev006f_bird, 0, 0x8000, 0);
        event_set_flags(ev006f_bird, EVENT_IGNORE_GROUND);
        event_clear_motions(ev006f_bird);
        event_play_action(ev006f_bird, &ev006f_bird_action,
                          &ev006f_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), 169.87F, -167.01F, 516.03F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 109.88F, -138.07F, 410.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf993, 0x9a66, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 109.31F, -144.0F, 409.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf993, 0x9a66, 0);
        event_wait(1);
        /* this clears the motions of player 1, not of Amy, who is player 0.
         * an empty slot gives a null task, which event_clear_motions
         * ignores */
        event_clear_motions(event_get_player(1));
        event_play_action(event_get_player(0), &ev006f_amy_action_0,
                          &amy_texlist, 0.3F, EVENT_REPEAT, 8);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_2);
        event_create_player(2, zero_event_init, 170.0F, 60.98F, 550.0F, 0,
                            0x5982, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 170.0F, 70.98F, 550.0F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_sound_start(0, 1229, 1800);
        event_sound_fade_pitch(0, 8000, 1);
        event_sound_fade_volume(0, -100, 1);
        event_wait(1);
        event_sound_fade_pitch(0, -8000, 80);
        event_sound_fade_volume(0, 80, 80);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 181.42F, -132.98F, 559.12F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe793, 0x0766, 0);
        event_wait(10);
        event_play_action(event_get_player(0), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(20);
        event_sound_stop(0);
        event_sound_play(297, 0, 0, 0);
        event_wait(40);
        event_play_action(event_get_player(0), &ev006f_amy_action_1,
                          &amy_texlist, 1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 166.25F, -159.4F, 505.99F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf793, 0x8f66, 0);
        event_wait(25);
        event_play_action(event_get_player(0), &ev006f_amy_action_2,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_play_action(event_get_player(0), &ev006f_amy_action_3,
                          &amy_texlist, 1.0F, 0, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1504);
        event_message_wait(0, TEXT_1);
        event_wait_motions(event_get_player(0));
        event_move(ev006f_bird, 174.92F, -155.97F, 522.31F, 175.95F, -155.97F,
                   492.08F, 60);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 177.28F, -162.59F, 480.71F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0293, 0x6e66, 0x0a00);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          2.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 170.0F, -167.0F, 490.0F, 2.0F,
                           0.06F);
        event_wait(40);
        event_message_close();
        event_run_wait(event_get_player(0));
        event_run_clear(event_get_player(0));
        event_talk_stop(event_get_player(0));
        event_move_stop(ev006f_bird);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_move_stop(ev006f_bird);
        event_kill_task(&ev006f_bird);
        event_reset_player(0);
        event_reset_player(2);
        event_run_clear(event_get_player(0));
        event_kill_player(2);
        event_set_pos(event_get_player(0), 170.0F, -167.0F, 490.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev006f_textures[] = {
    {"VER2_WING", &ev006f_bird_texlist},
    {NULL, NULL},
};
