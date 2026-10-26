/* EV00DB, Big's story: Big arrives in the past in a flash of white and asks
 * where he is */
#include "ev00db/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_models.h"
#include "player/textures.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 13
#include "ev00db/script/text.inc"

void ev00db_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_set_pos(event_get_player(0), 0.18F, -47.05F, 1393.05F);
        event_set_ang(event_get_player(0), 0, 0x8917, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.96F, -32.38F, 1378.44F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd00, 0x88dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -3.39F, -32.23F, 1376.49F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xfd00, 0x88dc, 0);
        event_white_start_hold(0, 30, 60);
        event_wait(10);
        event_sound_fade_volume(0, -120, 120);
        event_wait(80);
        event_sound_stop(0);
        event_voice_play(1850);
        event_message_wait(0, TEXT_1);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.35F, -36.92F, 1351.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0300, 0xffdc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 13.74F, -37.45F, 1426.85F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0x0300, 0xffdc, 0);
        event_voice_play(1851);
        event_message_wait(0, TEXT_2);
        event_wait(70);
        event_message_close();
        break;
    case EVENT_END:
        event_sound_stop_all();
        /* the event resets player 0 twice, with this call and the next */
        event_reset_player(0);
        event_reset_player(0);
        event_white_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00db_textures[] = {{NULL, NULL}};
