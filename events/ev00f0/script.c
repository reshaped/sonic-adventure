/* EV00F0: the last story opens on the crashed Tornado 2 in the jungle by
 * night */
#include <ninja.h>

#include "ev00f0/script.h"
#include "event/play.h"
#include "shared/fade.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

/* the crashed Tornado 2's prop slot */
#define TORNADO 0

extern NJS_OBJECT ev00f0_tornado_object;
extern NJS_TEXLIST ev00f0_tornado_texlist;

static struct task *ev00f0_black = NULL;

void ev00f0_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_00F3.mlt");
        event_sound_init(2);
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_EVTBGM04);
        event_prop_create(1226.0F, 99.0F, -904.66F, 0x1000, 0xd000, 0x0400,
                          1.0F, 1.0F, 1.0F, &ev00f0_tornado_object,
                          &ev00f0_tornado_texlist, TORNADO);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1159.26F, 108.24F, -944.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0400, 0x9f00, 0);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 1209.636F, 95.4F,
                             -904.302F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 1210.322F, 95.497F,
                             -903.581F);
        ev00f0_black = event_fade_create(1.0F / 90.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(80);
        event_sound_play(1223, 0, 0, 0);
        event_wait(20);
        event_sound_play(1224, 0, 0, 0);
        /* the black clears for one step, then covers the screen again */
        if (ev00f0_black != NULL) {
            event_fade_set_speed(ev00f0_black, -1.0F);
        }
        event_wait(1);
        if (ev00f0_black != NULL) {
            event_fade_set_speed(ev00f0_black, 1.0F);
        }
        event_wait(60);
        EVENT_KILL(ev00f0_black);
        event_prop_kill(TORNADO);
        break;
    case EVENT_END:
        event_prop_kill(TORNADO);
        EVENT_KILL(ev00f0_black);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f0_textures[] = {
    {"TR2CRASH", &ev00f0_tornado_texlist},
    {NULL, NULL},
};
