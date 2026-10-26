/* EV00B1, E-102's story: Eggman sends E-102 to the training room */
#include <ninja.h>

#include "ev00b1/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/eggman.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 49
#define TEXT_2_LENGTH 66
#define TEXT_3_LENGTH 19
#define TEXT_4_LENGTH 51
#include "ev00b1/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_run_action;

extern NJS_ACTION ev00b1_eggman_action_0;
extern NJS_ACTION ev00b1_eggman_action_1;
extern NJS_ACTION ev00b1_eggman_action_2;
extern NJS_ACTION ev00b1_eggman_action_3;
extern NJS_ACTION ev00b1_eggman_action_4;

static struct task *ev00b1_fade = NULL;

void ev00b1_script(int phase)
{
    /* @hack spelling: event_set_ang called through a local. SHC then puts
     * event_camera_set_pos's address in R11 and event_camera_set_ang's in R12
     * (0x0cb80066, 0x0cb80080). called by name, the two trade, and 22 of 592
     * words differ */
    void (*set_ang)(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_THEE102);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 5.23F, 125.07F, -122.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0300, 0x041d, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 4.28F, 110.01F, -79.65F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0c00, 0xff1d, 0);
        event_set_pos(event_get_player(0), -12.82F, 108.77F, -123.55F);
        set_ang = event_set_ang;
        set_ang(event_get_player(0), 0, 0x5236, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(2, eggman_step, -143.89F, 108.75F, 12.23F, 0,
                            0xa6fc, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 34.34F, 108.73F, -140.86F);
        set_ang(event_get_player(2), 0xffe0, 0xcf82, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00b1_eggman_action_0,
                          &eggman_texlist, 1.0F, 0, 8);
        event_wait(30);
        event_voice_play(1678);
        event_message_wait(0, TEXT_1);
        event_play_action(event_get_player(2), &ev00b1_eggman_action_2,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(30);
        event_play_action(event_get_player(2), &ev00b1_eggman_action_0,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -20.04F, 149.33F, -63.24F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xec00, 0xf0f2, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -53.08F, 144.04F, -140.09F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xec00, 0xb7f2, 0);
        event_voice_play(1679);
        event_message_wait(0, TEXT_2);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 59.01F, 129.04F, -134.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf200, 0x3c31, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 43.91F, 129.04F, -162.32F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xf200, 0x61fd, 0);
        event_wait(80);
        event_message_close();
        event_play_action(event_get_player(2), &ev00b1_eggman_action_1,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait(20);
        event_voice_play(1680);
        event_message_wait(0, TEXT_3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 16.14F, 122.15F, -129.85F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0xd58d, 0);
        event_voice_play(1681);
        event_message_wait(0, TEXT_4);
        event_wait(30);
        event_set_pos(event_get_player(0), 0.99F, 108.78F, -115.14F);
        set_ang(event_get_player(0), 0, 0x8307, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1.07F, 112.12F, -100.36F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0600, 0x008d, 0);
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(5);
        event_run_to_speed(event_get_player(0), -0.74F, 108.74F, -155.6F, 0.5F,
                           0.06F);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 16.14F, 122.15F, -129.85F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0xd58d, 0);
        event_play_action(event_get_player(2), &ev00b1_eggman_action_3,
                          &eggman_texlist, 1.0F, 0, 0);
        event_wait_motions(event_get_player(2));
        ev00b1_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_set_pos(event_get_player(0), 0.99F, 108.78F, -115.14F);
        set_ang(event_get_player(0), 0, 0x8307, 0);
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -0.74F, 108.74F, -155.6F, 0.5F,
                           0.06F);
        event_wait(1);
        event_set_pos(event_get_player(2), 28.79F, 108.73F, -151.59F);
        set_ang(event_get_player(2), 0, 0x5005, 0);
        event_play_action(event_get_player(2), &ev00b1_eggman_action_4,
                          &eggman_texlist, 1.0F, 0, 0);
        event_run_to_speed(event_get_player(2), 55.4F, 108.69F, -160.85F, 0.2F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1.04F, 139.1F, -199.49F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe900, 0x808d, 0);
        event_message_close();
        event_wait(90);
        break;
    case EVENT_END:
        EVENT_KILL(ev00b1_fade);
        event_reset_player(2);
        event_kill_player(2);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b1_textures[] = {{NULL, NULL}};
