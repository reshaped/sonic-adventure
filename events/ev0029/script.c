/* EV0029: the Tornado 2 comes in to land on the Egg Carrier. Tails finds
 * that its changed shape has no landing gear. EV0054.PRS is a copy of
 * EV0029.PRS, byte for byte */
#include <ninja.h>

#include "ev0029/script.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/sound.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 37
#define TEXT_3_LENGTH 15
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 27
#define TEXT_7_LENGTH 11
#include "ev0029/script/text.inc"

/* @hack type: ev0029_tornado_action declared as an array, which script.c
 * passes by its name. SHC then loads event_set_ang's address into R10 at
 * 0x0cb80022, and event_set_pos's into R11 at 0x0cb80024. as one NJS_ACTION
 * passed as &ev0029_tornado_action, the two trade registers, and 6 of
 * ev0029_script's 472 words differ */
extern NJS_ACTION ev0029_tornado_action[];
extern NJS_TEXLIST ev0029_tornado_texlist;

static struct event_path_point ev0029_tornado_points[96] = {
#include "ev0029/script/ev0029_tornado_points.inc"
};

static struct event_path ev0029_tornado_path = {96, ev0029_tornado_points};

static struct task *ev0029_tornado = NULL;

void ev0029_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0029.mlt");
        event_sound_init(2);
        stream_request_music(MUSIC_SCRAMBLE);
        event_set_pos(event_get_player(0), 100.0F, 570.0F, -673.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_reset_player(0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xea35, 0xa1e7, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1145.26F, 1252.15F,
                             -2293.41F);
        event_place_actor(&ev0029_tornado, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y + 12.0F,
                          event_get_player(0)->work->pos.z, 0, 0xd300, 0);
        event_set_flags(ev0029_tornado, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_ang(ev0029_tornado, -0x52b, 0x323d, -0xf04);
        event_set_pos(ev0029_tornado, -1125.7914F, 1232.0995F, -2274.443F);
        event_play_action(ev0029_tornado, ev0029_tornado_action,
                          &ev0029_tornado_texlist, 0.8F, EVENT_REPEAT, 8);
        event_play_path(ev0029_tornado, &ev0029_tornado_path, 0.15F, 0);
        event_camera_follow(ev0029_tornado);
        event_wait(30);
        event_voice_play(1168);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -994.0F, 1207.84F, -2219.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe935, 0xa4e7, 0xfc00);
        event_camera_follow(ev0029_tornado);
        event_wait(10);
        event_voice_play(1169);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_clear();
        event_voice_play(1170);
        event_message_show(TEXT_3);
        event_camera_follow_clear();
        event_wait(30);
        event_voice_play(1171);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_play(1172);
        event_message_show(TEXT_5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -480.07F, 1029.05F,
                             -1917.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe935, 0x1fe7, 0xfe00);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, ev0029_tornado, 0.0F,
                                  0.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -495.8887F, 1025.03F,
                             -1782.9F);
        event_voice_play(1173);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_play(1174);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -65.48F, 704.0F, -1116.462F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, ev0029_tornado, 0.0F,
                                  2.2F, 0.0F, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 147, -40.73F, 702.0F,
                             -1127.2323F);
        event_wait(127);
        event_sound_start(1, 1215, 1800);
        event_sound_fade_volume(1, 120, 1);
        event_wait(20);
        event_sprite_create(ev0029_tornado, 0, 0.0F, -13.0F, 0.0F,
                            &event_sprite_texlists[EVENT_SPRITE_LIGHTNING]);
        event_sprite_set_color(0, 0.5F, 0.3F, 0.5F, 0.6F);
        vibration_start(0, 3, 50, 5);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -16.9F, 713.0F, -1063.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x023b, 0x89cf, 0xff00);
        event_message_close();
        event_sound_stop(1);
        event_sprite_kill(0);
        event_play_action(ev0029_tornado, ev0029_tornado_action,
                          &ev0029_tornado_texlist, 0.1F, EVENT_REPEAT, 8);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -18.0F, 713.68F, -1077.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x003b, 0x85cf, 0xff00);
        event_wait(60);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), 0.0F, 700.0F, -1078.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_clear_paths(ev0029_tornado);
        event_kill_task(&ev0029_tornado);
        event_sprite_kill(0);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0029_textures[] = {
    {"EV_TR2CHANGE_WITH_SONIC", &ev0029_tornado_texlist},
    {NULL, NULL},
};
