/* EV00C4, E-102's story: E-102 lands, resolves to save the rest of the
 * E-Series, and asks where they are */
#include <ninja.h>

#include "ev00c4/e102_action.h"
#include "ev00c4/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/scanlines.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 46
#include "ev00c4/script/text.inc"

/* @hack type: e102_stand_action declared as an array, which script.c passes by
 * its name. SHC then leaves R8 to the camera angle 0x1160 from 0x0cb8029a. as
 * one NJS_ACTION passed as &e102_stand_action, R8 holds event_sound_stop's
 * address, and 465 of 591 words differ */
extern NJS_ACTION e102_stand_action[];
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;

/* E-102's fall. A is held from key 75 through key 212, and the stick is pushed
 * twice meanwhile, never as far as halfway */
static const struct event_pad_key ev00c4_fall_keys[282] = {
#include "ev00c4/script/ev00c4_fall_keys.inc"
};

static const struct event_pad_recording ev00c4_fall = {
    ev00c4_fall_keys, sizeof(ev00c4_fall_keys) / sizeof(ev00c4_fall_keys[0])};

void ev00c4_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_004e.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_MSTCLN);
        event_set_pos(event_get_player(0), 52.4F, 500.0F, 673.84F);
        event_set_ang(event_get_player(0), 0, 0x25c6, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev00c4_fall);
        event_sound_start(0, 1228, 1800);
        event_wait(1);
        event_sound_fade_pitch(0, 8000, 1);
        event_sound_fade_volume(0, -100, 1);
        event_wait(1);
        event_sound_fade_pitch(0, -8000, 80);
        event_sound_fade_volume(0, 50, 80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 12.21F, 456.16F, 610.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0441, 0x96ef, 0);
        event_wait(38);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 35.66F, 474.81F, 643.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xc741, 0x8bef, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 122.31F, 234.51F, 726.24F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d89, 0x25cb, 0);
        event_sound_stop(0);
        event_sound_start(0, 789, 1800);
        event_wait(50);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 150.29F, 212.56F, 630.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0c04, 0x4acb, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 43.73F, 180.81F, 614.14F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x1404, 0x85cb, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 78.5F, 7.17F, 663.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x3860, 0x4fcb, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 78.5F, 7.17F, 663.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x1160, 0x4fcb, 0);
        event_sound_stop(0);
        event_wait(65);
        vibration_start(0, 3, 5, 1);
        event_sound_play(785, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 5, 78.5F, 7.17F, 663.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1160, 0x4fcb, 0);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 5, 78.5F, 7.17F, 663.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1360, 0x4fcb, 0);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 5, 78.5F, 7.17F, 663.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x1160, 0x4fcb, 0);
        event_sound_play(785, 0, 0, 0);
        event_wait(50);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 74.21F, 10.93F, 671.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff60, 0x87cb, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 74.21F, 10.93F, 671.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xff60, 0xf8cb, 0);
        event_voice_play(1777);
        event_message_wait(0, TEXT_1);
        event_voice_play(1778);
        event_message_wait(0, TEXT_2);
        event_play_action(event_get_player(0), &ev00c4_e102_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 74.21F, 10.93F, 671.18F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xff60, 0x87cb, 0);
        event_wait(30);
        event_voice_play(1779);
        event_message_wait(0, TEXT_3);
        event_wait(30);
        event_scanlines_kill();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 81.52F, 23.55F, 621.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfc08, 0x701d, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 98.32F, 10.72F, 693.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0260, 0x2aee, 0);
        event_wait(50);
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(40);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 8);
        event_wait(8);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 16.59F, 23.15F, 696.74F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfdf2, 0xcc7f, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 19.45F, 23.0F, 695.84F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xfdf2, 0xcc7f, 0);
        event_wait(40);
        event_message_close();
        event_wait(1);
        /* no break: the EVENT_PLAY case runs on into the EVENT_END case.
         * play.c's event_play then calls the script again with EVENT_END,
         * which runs that code a second time */
    case EVENT_END:
        event_scanlines_kill();
        event_sound_stop_all();
        event_set_pos(event_get_player(0), 52.2F, 8.32F, 673.59F);
        event_set_ang(event_get_player(0), 0, 0x5518, 0);
        event_wait(1);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00c4_textures[] = {
    {NULL, NULL},
};
