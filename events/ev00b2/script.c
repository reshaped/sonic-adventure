/* EV00B2, E-102's story: Eggman praises E-102 */
#include <ninja.h>

#include "ev00b2/e102_action_0.h"
#include "ev00b2/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/dat_8c5a644c.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 23
#include "ev00b2/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION eggman_walk_action;

void ev00b2_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_THEE102);
        event_set_pos(event_get_player(0), -125.1F, 0.31F, -16.18F);
        event_set_ang(event_get_player(0), 0, 0xe5ff, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(2, eggman_step, -162.8F, 0.21F, 22.26F, 0, 0x5982,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(2), -140.36F, 0.27F, 5.44F);
        event_set_ang(event_get_player(2), 0, 0x5982, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -117.88F, 21.81F, -14.69F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf1a5, 0x5c50, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &dat_8c5a644c, &eggman_texlist,
                          1.0F, 0, 0);
        event_voice_play(1682);
        event_message_wait(0, TEXT_1);
        event_wait(1);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 30);
        event_wait(40);
        event_set_pos(event_get_player(2), -140.36F, 0.27F, 5.44F);
        event_set_ang(event_get_player(2), 0, 0x5982, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &eggman_walk_action,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -134.55F, 11.18F, -35.08F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x80f2, 0);
        event_run_to_speed(event_get_player(2), -0.74F, 0.42F, 4.72F, 0.3F,
                           0.06F);
        event_voice_play(1683);
        event_message_wait(0, TEXT_2);
        event_wait(30);
        event_play_action(event_get_player(0), &ev00b2_e102_action_0,
                          &e102_texlist, 1.0F, 0, 16);
        event_wait(10);
        event_wait_motions(event_get_player(0));
        event_set_pos(event_get_player(0), -125.1F, 0.31F, -16.18F);
        event_set_ang(event_get_player(0), 0, 0x3a55, 0);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -88.38F, 16.06F, -20.04F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfa00, 0x50f2, 0);
        event_message_close();
        event_wait(50);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), -125.1F, 0.31F, -16.18F);
        event_set_ang(event_get_player(0), 0, 0x3a55, 0);
        event_wait(1);
        event_reset_player(0);
        event_reset_player(2);
        event_kill_player(2);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b2_textures[] = {
    {"EGGMAN", &eggman_texlist},
    {NULL, NULL},
};
