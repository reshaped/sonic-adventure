/* EV0044, Tails' story: Tails finds himself in the past */
#include "ev0044/script.h"
#include "ev0044/tails_action.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 42
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 6
#include "ev0044/script/text.inc"

void ev0044_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_000d.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), -11.58F, 9.99F, 1354.12F);
        event_set_ang(event_get_player(0), 0, 0x8020, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -13.38F, 14.09F, 1345.51F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0x91dc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 250, -15.19F, 14.09F, 1346.36F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 250, 0x0700, 0x91dc, 0);
        event_white_start_hold(0, 30, 60);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_wait(50);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_wait(7);
        event_voice_play(1253);
        event_message_wait(1, TEXT_1);
        event_wait(140);
        event_sound_stop(0);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.63F, 24.15F, 1318.76F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xffdc, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -1.85F, 13.04F, 1382.74F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0700, 0xffdc, 0);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0044_tails_action,
                          &tails_texlist, 0.2F, EVENT_REPEAT, 0);
        event_voice_play(1254);
        event_message_wait(0, TEXT_2);
        event_wait(90);
        event_message_close();
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_reset_player(0);
        event_white_kill();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0044_textures[] = {{NULL, NULL}};
