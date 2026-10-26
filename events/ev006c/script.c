/* EV006C, Amy's story: Amy wakes, wonders whether she was dreaming, and
 * realizes she must get out */
#include <ninja.h>

#include "ev006c/amy_action.h"
#include "ev006c/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 16
#define TEXT_2_LENGTH 27
#define TEXT_3_LENGTH 47
#define TEXT_4_LENGTH 3
#define TEXT_5_LENGTH 4
#define TEXT_6_LENGTH 4
#include "ev006c/script/text.inc"

/* @hack type: amy_stand_action declared as an array, which script.c passes by
 * its name. SHC then keeps event_play_action's address in R9 from 0x0cb800e4
 * on, and event_talk_start's in R8. as one NJS_ACTION passed as
 * &amy_stand_action, the two addresses trade registers, and 8 of 308 words
 * differ */
extern NJS_ACTION amy_stand_action[];

static struct task *ev006c_fade = NULL;

void ev006c_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_sound_init(1);
        ev006c_fade = event_fade_create(-0.0012F, 1.0F, 1.0F, 1.0F, 1.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 290.17F, 15.8F, 1.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0xd690, 0);
        event_set_pos(event_get_player(0), 295.24F, 10.0F, 0.0F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_wait(1);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(29);
        if (ev006c_fade != NULL) {
            event_fade_set_speed(ev006c_fade, -0.016F);
        }
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 277.57F, 15.01F, 10.16F);
        event_wait(60);
        event_sound_stop(0);
        event_play_action(event_get_player(0), &ev006c_amy_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_voice_play(1484);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_message_show(TEXT_1);
        event_wait(40);
        event_talk_stop(event_get_player(0));
        event_wait(40);
        event_play_action(event_get_player(0), amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(15);
        event_voice_play(1485);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_message_wait(100, TEXT_2);
        event_talk_stop(event_get_player(0));
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 303.16F, 26.02F, 17.45F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xee00, 0x1c90, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 319.48F, 37.99F, 36.8F);
        event_wait(15);
        event_voice_play(1486);
        event_talk_start(event_get_player(0), TEXT_6);
        event_message_wait(100, TEXT_3);
        event_talk_stop(event_get_player(0));
        event_wait(15);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        EVENT_KILL(ev006c_fade);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev006c_textures[] = {{NULL, NULL}};
