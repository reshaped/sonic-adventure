/* EV00F6: Sonic arrives in the past as a white flash fades, and knows the
 * place from before */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00f6/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 23
#define TEXT_3_LENGTH 22
#define TEXT_4_LENGTH 2
#define TEXT_5_LENGTH 7
#include "ev00f6/script/text.inc"

extern NJS_ACTION sonic_jump_land_action;

extern NJS_ACTION ev00f6_sonic_action_0;
extern NJS_ACTION ev00f6_sonic_action_1;

static struct task *ev00f6_white = NULL;

void ev00f6_script(int phase)
{
    /* @hack spelling: every camera position is set through camera_set_pos, a
     * local. SHC then keeps event_play_action's address in R8 (0x0cb800c0)
     * and the ang_z 0xfd00 in R9 (0x0cb80102). called directly, the two
     * trade, and 8 of 318 words differ */
    void (*camera_set_pos)(enum event_ease ease, int steps, Float x, Float y,
                           Float z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_init(1);
        ev00f6_white = event_fade_create(1.0F, 0.1F, 1.0F, 1.0F, 1.0F);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_set_pos(event_get_player(0), 15.1F, -43.46F, 1447.05F);
        event_set_ang(event_get_player(0), 0xfca0, 0x8c00, 0xfed0);
        event_reset_player(0);
        event_talk_start(event_get_player(0), TEXT_3);
        event_play_action(event_get_player(0), &ev00f6_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x6500, 0);
        camera_set_pos = event_camera_set_pos;
        camera_set_pos(EVENT_EASE_SMOOTH, 0, 19.9F, -37.6F, 1441.1F);
        event_wait(30);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0300, 0x7900, 0xfd00);
        camera_set_pos(EVENT_EASE_LINEAR, 80, 16.475F, -37.6F, 1439.2F);
        if (ev00f6_white != NULL) {
            event_fade_set_speed(ev00f6_white, -1.0F / 60.0F);
        }
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_4);
        event_wait(50);
        event_play_action(event_get_player(0), &ev00f6_sonic_action_0,
                          &sonic_texlist, 0.4F, EVENT_REPEAT, 8);
        event_voice_play(1928);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_message_show(TEXT_1);
        event_wait(45);
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_voice_wait();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0x2600, 0xfd00);
        camera_set_pos(EVENT_EASE_SMOOTH, 0, 22.05F, -40.624F, 1450.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0300, 0x0300, 0xfd00);
        camera_set_pos(EVENT_EASE_LINEAR, 90, 20.08F, -40.7F, 1453.16F);
        event_wait(10);
        event_message_show(TEXT_2);
        event_voice_play(1929);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_wait(15);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_set_pos(event_get_player(0), 15.1F, -43.46F, 1447.05F);
        event_set_ang(event_get_player(0), 0xfca0, 0x8c00, 0xfed0);
        EVENT_KILL(ev00f6_white);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f6_textures[] = {{NULL, NULL}};
