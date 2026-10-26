/* EV0142: E-102 reports E-105 rescued, two pictures appear over the screen in
 * turn, and E-101 flies past */
#include <ninja.h>

#include "ev0142/e102_action_2.h"
#include "ev0142/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/picture.h"
#include "shared/scanlines.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 31
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 15
#include "ev0142/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;

extern NJS_ACTION ev0142_e102_action_0;
extern NJS_ACTION ev0142_e102_action_1;
extern NJS_TEXLIST ev0142_memory_texlist;

static struct event_path_point ev0142_e101_points[121] = {
#include "ev0142/script/ev0142_e101_points.inc"
};

static struct event_path ev0142_e101_path = {121, ev0142_e101_points};

/* the picture over the screen, which shows ev0142_memory_texlist's two
 * textures in turn */
static struct task *ev0142_memory = NULL;
static struct task *ev0142_e101 = NULL;

void ev0142_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00C5.mlt");
        event_sound_init(5);
        event_set_pos(event_get_player(0), -2.2F, 733.5F, 1050.6F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_reset_player(0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_stand_action,
                          &e102_texlist, 0.4F, EVENT_REPEAT, 8);
        event_place_actor(&ev0142_e101, -12.45F, 776.22F, -368.58F, 0x0a00,
                          0xe320, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.136F, 734.91F, 1027.548F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c45, 0x8d25, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, -9.136F, 743.0F,
                             1027.548F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0x0c45, 0x8d25, 0);
        event_voice_play(1787);
        event_message_show(TEXT_1);
        event_wait(30);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 0.5F, 0, 8);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 0.3F, EVENT_REPEAT, 8);
        event_wait(32);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, -7.566F, 746.3377F,
                             1040.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x1445, 0x9a25, 0);
        event_wait(30);
        event_voice_play(1788);
        event_message_show(TEXT_2);
        event_wait(40);
        event_play_action(event_get_player(0), &ev0142_e102_action_2,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 32);
        event_wait(45);
        event_message_close();
        ev0142_memory = event_picture_create(&ev0142_memory_texlist);
        event_picture_set_depth(ev0142_memory, -3.0F);
        event_picture_fade(ev0142_memory, 1.0F, 16);
        event_wait(16);
        event_sound_play(1223, 0, 0, 0);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_wait(60);
        event_picture_fade(ev0142_memory, 0.0F, 16);
        event_scanlines_kill();
        event_wait(16);
        event_picture_set_texture(ev0142_memory, 1);
        event_picture_fade(ev0142_memory, 1.0F, 16);
        event_wait(16);
        event_scanlines_create(128, 0, 80, 85, 5);
        event_sound_play(1223, 0, 0, 0);
        event_wait(60);
        event_picture_fade(ev0142_memory, 0.0F, 16);
        event_scanlines_kill();
        event_wait(16);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.748F, 734.38F, 1043.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b45, 0x8f25, 0x0100);
        event_wait(20);
        event_play_action(event_get_player(0), &ev0142_e102_action_0,
                          &e102_texlist, 0.6F, EVENT_REPEAT, 72);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 95, -11.06F, 743.5F, 1033.628F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 95, 0x0b45, 0x9a25, 0x0200);
        event_wait(105);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -12.677F, 743.6F, 1034.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0b45, 0x9d25, 0x0200);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 120, 0x327d);
        event_voice_play(1789);
        event_message_show(TEXT_3);
        event_wait(80);
        event_set_flags(ev0142_e101, EVENT_IGNORE_GROUND);
        event_set_pos(ev0142_e101, -47.7F, 758.6F, 962.9F);
        event_set_ang(ev0142_e101, 0xf9ee, 0x3150, 0);
        event_play_action(ev0142_e101, ev0142_e101_action, &ev0142_e101_texlist,
                          1.0F, EVENT_REPEAT, 8);
        stream_request_music(MUSIC_BOSSE101);
        event_play_action(event_get_player(0), &ev0142_e102_action_1,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 8);
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, -11.78F, 750.433F,
                             1045.66F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 35, 0x0745, 0xb725, 0x0300);
        event_sound_start(0, 1224, 1800);
        event_sound_fade_volume(0, -40, 1);
        event_wait(35);
        event_play_path(ev0142_e101, &ev0142_e101_path, 0.5F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -11.43F, 749.577F, 1051.5F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, ev0142_e101, 0.0F, 0.0F,
                                  0.0F, 0);
        event_wait(71);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -133.06F, 855.0F, 797.57F);
        event_camera_follow(ev0142_e101);
        event_sound_fade_volume(0, -10, 1);
        event_wait(60);
        event_camera_follow_clear();
        event_sound_fade_volume(0, -40, 1);
        event_wait(1);
        event_sound_fade_volume(0, -90, 70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -11.43F, 751.577F, 1051.5F);
        event_wait(70);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -4.064F, 746.52F, 1035.58F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0d45, 0x8425, 0);
        event_play_action(event_get_player(0), &ev0142_e102_action_2,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 32);
        event_wait(20);
        event_voice_play(1795);
        event_message_show(TEXT_4);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, -8.04F, 738.47F, 996.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0x0c9e, 0x8425, 0);
        event_wait(20);
        event_message_close();
        event_wait(80);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), -2.2F, 733.5F, 1050.6F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_reset_player(0);
        event_kill_task(&ev0142_e101);
        EVENT_KILL(ev0142_memory);
        event_sound_stop_all();
        event_scanlines_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0142_textures[] = {
    {"EV_E101KAI", &ev0142_e101_texlist},
    {"ICM0142", &ev0142_memory_texlist},
    {NULL, NULL},
};
