/* EV00BD, E-102's story: E-102 walks into E-101's factory. he backs away
 * as he says E-101's name */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00bd/e102_action_2.h"
#include "ev00bd/factory_task.h"
#include "ev00bd/factory_textures.h"
#include "ev00bd/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#include "ev00bd/script/text.inc"

extern NJS_ACTION e102_walk_action;
extern NJS_ACTION e102_stand_action;

extern NJS_ACTION ev00bd_e102_action_0;
extern NJS_ACTION ev00bd_e102_action_1;

static struct task *ev00bd_floor = NULL;
static struct task *ev00bd_fade = NULL;

void ev00bd_script(int phase)
{
    /* @hack spelling: the top of E-102's bob through a local, which SHC
     * keeps in FR14 and 500.5F in FR13 (0x0cb80428, 0x0cb800d0); as a literal
     * at each use, the two trade and 45 of 1088 words differ */
    Float top_y;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00bb.mlt");
        event_sound_init(2);
        stream_request_music(MUSIC_EVTBGM04);
        ev00bd_fade = event_fade_create(-0.01F, 1.0F, 0.0F, 0.0F, 0.0F);
        ev00bd_factory_create();
        ev00bd_floor = event_floor_create(event_get_player(0), 500.0F);
        event_set_pos(event_get_player(0), -130.0F, 500.0F, 0.0F);
        event_set_ang(event_get_player(0), 0, 0x3ed0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -119.9F, 500.5F, 27.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x11c9, 0xe9d6, 0x0500);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, -119.9F, 500.5F, 27.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 110, 0x10c9, 0xe5d6, 0x0500);
        event_play_action(event_get_player(0), &e102_walk_action, &e102_texlist,
                          0.6F, EVENT_REPEAT, 0);
        event_sound_play(1224, 128, 0, 0);
        event_wait(20);
        event_run_to_speed(event_get_player(0), -87.0F, 500.0F, 0.0F, 0.45F,
                           0.1F);
        event_wait(110);
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 0.6F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -93.1F, 502.19F, 46.57F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x03c9, 0xe7d6, 0x0500);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -89.0F, 504.77F, 50.86F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xffc9, 0xf1d6, 0x0500);
        event_wait(50);
        event_play_action(event_get_player(0), &ev00bd_e102_action_0,
                          &e102_texlist, 0.4F, 0, 16);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -80.82F, 518.3F, 52.69F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0xef5e, 0x0143, 0x0500);
        event_wait_motions(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0xfed0, 0);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(105);
        event_play_action(event_get_player(0), &ev00bd_e102_action_1,
                          &e102_texlist, 0.8F, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -79.9F, 522.02F, 1.09F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xdff8, 0x58d6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 7, -79.33F, 522.73F, -1.49F);
        event_wait(40);
        event_set_ang(event_get_player(0), 0, 0x3ed0, 0);
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -96.18F, 501.39F, -11.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x17c9, 0xaad6, 0x1100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 105, -101.57F, 510.28F,
                             -12.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 105, 0x17c9, 0xb7d6, 0x1100);
        event_wait(105);
        event_camera_set_pos(EVENT_EASE_LINEAR, 85, -97.47F, 513.03F, -12.087F);
        event_wait(85);
        top_y = 501.0F;
        event_set_pos(event_get_player(0), -77.6F, top_y, 0.0F);
        event_set_ang(event_get_player(0), 0, 0x3ed0, 0);
        event_play_action(event_get_player(0), &e102_walk_action, &e102_texlist,
                          0.6F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -30.7F, 564.8F, -6.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x03c9, 0xa0d6, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -23.05F, 559.15F, -22.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 87, 0x07c9, 0xa5d6, 0);
        event_wait(86);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 114, 0x12c9, 0xdbd6, 0x0400);
        event_wait(4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, -74.2F, 514.2F, 40.34F);
        event_wait(130);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -103.95F, 500.2F, 23.16F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x11c9, 0xd0d6, 0x0400);
        event_wait(70);
        event_voice_play(1759);
        event_message_show(TEXT_1);
        /* E-102 backs away 0.2 every 2 steps, from x -77.6 to -86.8. his y
         * moves among 501, 500.5 and 500, so he bobs as he goes */
        event_set_pos(event_get_player(0), -77.6F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -77.8F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -78.0F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -78.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -78.4F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -78.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -78.8F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -79.0F, 500.0F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -79.2F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -79.4F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -79.6F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -79.8F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -80.0F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -80.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -80.4F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -80.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -80.8F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -81.0F, 500.0F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -81.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -81.4F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -81.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -81.8F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -82.0F, 500.0F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -82.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -82.4F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -82.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -82.8F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -83.0F, 500.0F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -83.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -83.4F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -83.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -83.8F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -84.0F, 500.0F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -84.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -84.4F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -84.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -84.8F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -85.0F, 500.0F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -85.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -85.4F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -85.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -85.8F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -86.0F, 500.0F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -86.2F, top_y, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -86.4F, top_y, 0.0F);
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 0.4F, EVENT_REPEAT, 12);
        event_wait(2);
        event_set_pos(event_get_player(0), -86.6F, 500.5F, 0.0F);
        event_wait(2);
        event_set_pos(event_get_player(0), -86.8F, 500.5F, 0.0F);
        event_wait(4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -102.3F, 501.04F, 22.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x11c9, 0xd0d6, 0x0400);
        event_wait(90);
        event_play_action(event_get_player(0), &ev00bd_e102_action_2,
                          &e102_texlist, 1.1F, EVENT_REPEAT, 12);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -76.9F, 526.7F, 0.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xdec9, 0x3fd6, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -80.38F, 523.1F, 0.0F);
        event_wait(95);
        event_message_close();
        event_wait(100);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        ev00bd_factory_kill();
        EVENT_KILL(ev00bd_fade);
        event_set_pos(event_get_player(0), 62.8F, 0.0F, 194.0F);
        event_set_ang(event_get_player(0), 0xec98, 0x8b05, 0);
        EVENT_KILL(ev00bd_floor);
        event_sound_stop_all();
        event_wait(1);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00bd_textures[] = {
    {"PVME101FACTORY", &ev00bd_factory_texlist},
    {NULL, NULL},
};
