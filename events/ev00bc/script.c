/* EV00BC, E-102's story: at the Egg Carrier's rotary, E-102 asks himself
 * if this is the place and walks on as it turns */
#include "ev00bc/script.h"
#include "adv0130/rotary.h"
#include "event/play.h"
#include "progress/progress.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#include "ev00bc/script/text.inc"

/* E-102's walk, a key a step: the stick alone, with no buttons, let go for the
 * last 9 keys */
/* clang-format off */
static const struct event_pad_key ev00bc_walk_keys[166] = {
    {0, 73, 153}, {0, 73, 153}, {0, 77, 153}, {0, 77, 150}, {0, 77, 156},
    {0, 77, 156}, {0, 77, 156}, {0, 77, 156}, {0, 77, 158}, {0, 77, 154},
    {0, 77, 154}, {0, 77, 154}, {0, 77, 157}, {0, 77, 157}, {0, 77, 157},
    {0, 77, 154}, {0, 77, 154}, {0, 77, 154}, {0, 77, 154}, {0, 77, 154},
    {0, 77, 158}, {0, 77, 152}, {0, 79, 150}, {0, 70, 156}, {0, 70, 159},
    {0, 70, 159}, {0, 70, 157}, {0, 70, 159}, {0, 69, 150}, {0, 68, 153},
    {0, 67, 153}, {0, 67, 155}, {0, 66, 157}, {0, 67, 157}, {0, 67, 157},
    {0, 66, 151}, {0, 66, 151}, {0, 66, 151}, {0, 66, 157}, {0, 66, 159},
    {0, 66, 157}, {0, 66, 159}, {0, 65, 159}, {0, 66, 159}, {0, 66, 159},
    {0, 67, 161}, {0, 67, 61}, {0, 67, 159}, {0, 67, 161}, {0, 67, 161},
    {0, 71, 154}, {0, 71, 156}, {0, 71, 154}, {0, 71, 154}, {0, 71, 154},
    {0, 71, 154}, {0, 71, 154}, {0, 71, 154}, {0, 71, 154}, {0, 71, 154},
    {0, 71, 154}, {0, 71, 154}, {0, 72, 163}, {0, 73, 186}, {0, 74, 188},
    {0, 78, 220}, {0, 83, 236}, {0, 84, 238}, {0, 84, 238}, {0, 83, 236},
    {0, 83, 236}, {0, 83, 236}, {0, 83, 238}, {0, 83, 238}, {0, 83, 238},
    {0, 83, 236}, {0, 83, 238}, {0, 83, 238}, {0, 83, 238}, {0, 83, 238},
    {0, 83, 238}, {0, 83, 238}, {0, 83, 236}, {0, 83, 238}, {0, 83, 238},
    {0, 85, 235}, {0, 85, 237}, {0, 85, 237}, {0, 84, 236}, {0, 83, 236},
    {0, 77, 240}, {0, 73, 247}, {0, 68, 252}, {0, 67, 0}, {0, 47, 241},
    {0, 40, 239}, {0, 40, 238}, {0, 40, 238}, {0, 39, 240}, {0, 39, 240},
    {0, 40, 239}, {0, 41, 240}, {0, 41, 237}, {0, 39, 239}, {0, 26, 240},
    {0, 16, 244}, {0, 11, 246}, {0, 7, 251}, {0, 5, 253}, {0, 2, 255},
    {0, 0, 255}, {0, -3, 0}, {0, 2, 255}, {0, 1, 255}, {0, 0, 255}, {0, 0, 255},
    {0, -1, 255}, {0, -1, 255}, {0, -1, 255}, {0, -1, 255}, {0, -1, 255},
    {0, -1, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255}, {0, 0, 255},
    {0, 0, 255}, {0, 1, 254}, {0, 1, 254}, {0, 1, 254}, {0, 2, 255},
    {0, 3, 255}, {0, 4, 0}, {0, -3, 0}, {0, 0, 255}, {0, 1, 255}, {0, 4, 252},
    {0, 9, 248}, {0, 11, 245}, {0, 13, 244}, {0, 15, 245}, {0, 15, 241},
    {0, 15, 241}, {0, 15, 241}, {0, 15, 241}, {0, 16, 244}, {0, 18, 242},
    {0, 18, 240}, {0, 19, 242}, {0, 20, 240}, {0, 23, 240}, {0, 26, 242},
    {0, 27, 241}, {0, 27, 240}, {0, 27, 240}, {0, 27, 240}, {0, 30, 240},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0},
};
/* clang-format on */

static const struct event_pad_recording ev00bc_walk = {
    ev00bc_walk_keys, sizeof(ev00bc_walk_keys) / sizeof(ev00bc_walk_keys[0])};

void ev00bc_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00bb.mlt");
        event_sound_init(2);
        stream_request_music(MUSIC_THEE102);
        event_set_pos(event_get_player(0), 0.0F, -5.0F, 26.37F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -0.7F, 51.4F, -8.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf900, 0x7f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 0.2F, 48.1F, -46.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xfb00, 0x7f00, 0);
        event_wait(100);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 0.3F, 18.5F, -50.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xff00, 0x7f00, 0);
        event_wait(100);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_camera_set_pos(EVENT_EASE_SMOOTH, 170, 44.0F, 12.7F, -9.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 170, 0xff00, 0x6b00, 0);
        event_wait(100);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_voice_play(1758);
        event_message_show(TEXT_1);
        event_wait(50);
        event_message_close();
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_wait(30);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_play_pad(0, &ev00bc_walk);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -12.6F, 18.3F, 232.3F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 10.0F, 0.0F, 0);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_wait(40);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_wait(50);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_wait(40);
        event_sound_play(1224, 128, 0, 0);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.5F, 15.7F, 15.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x8600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -2.5F, 11.7F, 15.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0200, 0x8100, 0);
        event_set_pos(event_get_player(0), -6.1F, 6.5F, -193.6F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(80);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -2.6F, 52.7F, 21.54F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xf400, 0x8000, 0);
        event_wait(80);
        event_flags[FLAG_ROTARY] |= ROTARY_TURN;
        event_wait(80);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_wait(1);
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00bc_textures[] = {{NULL, NULL}};
