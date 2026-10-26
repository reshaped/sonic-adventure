/* EV0058, Tails' story: two sounds play on a black screen as Tails enters
 * Casinopolis's act 1 */
#include "ev0058/script.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

static struct task *ev0058_fade = NULL;

void ev0058_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_take_control();
        event_wait(1);
        sound_load_file("e_0024.mlt");
        event_sound_init(5);
        event_reset_player(0);
        event_pads_off();
        ev0058_fade = event_fade_create(1.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_set_pos(event_get_player(0), 19.0F, -1695.0F, 2850.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(30);
        event_sound_start(0, 1225, 1800);
        event_wait(45);
        event_sound_start(1, 249, 1800);
        event_wait(100);
        event_sound_stop(0);
        event_sound_stop(1);
        event_set_pos(event_get_player(0), 19.0F, -1695.0F, 2850.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), 19.0F, -1695.0F, 2850.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_sound_stop_all();
        EVENT_KILL(ev0058_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0058_textures[] = {
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
