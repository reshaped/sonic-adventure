/* EV00B3, E-102's story: Eggman sends E-102 through his training
 * again */
#include <ninja.h>

#include "ev00b3/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 37
#define TEXT_3_LENGTH 46
#include "ev00b3/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_run_action;

extern NJS_ACTION ev00b3_e102_action_0;
extern NJS_ACTION ev00b3_e102_action_1;
extern NJS_ACTION ev00b3_e102_action_2;
extern NJS_ACTION ev00b3_eggman_action_0;
extern NJS_ACTION ev00b3_eggman_action_1;
extern NJS_ACTION ev00b3_eggman_action_2;
extern NJS_ACTION ev00b3_eggman_action_3;

static struct task *ev00b3_fade = NULL;

void ev00b3_script(int phase)
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
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 25.56F, 144.56F, -102.07F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xed00, 0x07f2, 0);
        event_set_pos(event_get_player(0), -1.77F, 108.68F, -173.04F);
        event_set_ang(event_get_player(0), 0, 0xfd7d, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -0.52F, 108.71F, -162.05F, 0.5F,
                           0.06F);
        event_create_player(2, eggman_step, -143.89F, 108.75F, 12.23F, 0,
                            0xa6fc, 0);
        event_wait(1);
        event_reset_player(2);
        event_set_pos(event_get_player(2), 34.34F, 108.73F, -140.86F);
        event_set_ang(event_get_player(2), 0, 0xa700, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00b3_eggman_action_3,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(20);
        event_run_clear(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_set_pos(event_get_player(0), 0.79F, 108.73F, -157.86F);
        event_set_ang(event_get_player(0), 0xffe0, 0x1eb8, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 19.28F, 125.16F, -150.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf800, 0xa7f2, 0);
        event_play_action(event_get_player(2), &ev00b3_eggman_action_0,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(1);
        event_voice_play(1684);
        event_message_wait(0, TEXT_1);
        event_wait(1);
        /* the camera shakes between two points, 3 steps each way */
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 26.61F, 123.41F, -145.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0xa7f2, 0);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 19.28F, 125.16F, -150.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0xa7f2, 0);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 26.61F, 123.41F, -145.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0xa7f2, 0);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 19.28F, 125.16F, -150.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0xa7f2, 0);
        event_wait(3);
        event_play_action(event_get_player(0), &ev00b3_e102_action_0,
                          &e102_texlist, 1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 28.02F, 128.33F, -136.29F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf200, 0x22f2, 0);
        event_wait(5);
        event_voice_play(1685);
        event_message_wait(0, TEXT_2);
        event_play_action(event_get_player(2), &ev00b3_eggman_action_1,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        /* the camera jumps back to the first point, then shakes between
         * it and a point 2 units off, 10 steps each way */
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 26.61F, 123.41F, -145.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf800, 0xa7f2, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 24.98F, 123.79F, -146.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0xf800, 0xa7f2, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 26.61F, 123.41F, -145.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0xf800, 0xa7f2, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 24.98F, 123.79F, -146.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0xf800, 0xa7f2, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 26.61F, 123.41F, -145.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0xf800, 0xa7f2, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 24.98F, 123.79F, -146.82F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0xf800, 0xa7f2, 0);
        event_wait(10);
        event_message_close();
        event_play_action(event_get_player(0), &ev00b3_e102_action_1,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 28.02F, 128.33F, -136.29F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf200, 0x22f2, 0);
        event_wait(50);
        event_voice_play(1686);
        event_message_wait(0, TEXT_3);
        event_wait(10);
        event_play_action(event_get_player(2), &ev00b3_eggman_action_2,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 18.04F, 120.27F, -133.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfc00, 0xdbf2, 0);
        event_wait(30);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 26.09F, 123.79F, -148.48F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0xff00, 0x2af2, 0);
        event_play_action(event_get_player(0), &ev00b3_e102_action_2,
                          &e102_texlist, 0.5F, 0, 0);
        event_wait(40);
        ev00b3_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1.58F, 126.0F, -200.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff00, 0x80f2, 0);
        event_set_pos(event_get_player(0), 0.75F, 108.76F, -148.38F);
        event_set_ang(event_get_player(0), 0, 0x7d24, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -1.05F, 108.67F, -192.56F, 0.5F,
                           0.06F);
        event_wait(50);
        break;
    case EVENT_END:
        EVENT_KILL(ev00b3_fade);
        event_kill_player(2);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b3_textures[] = {
    {"EGGMAN", &eggman_texlist},
    {NULL, NULL},
};
