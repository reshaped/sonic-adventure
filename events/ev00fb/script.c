/* EV00FB: Sonic wakes Tikal at the altar in the past, and she runs off */
#include <ninja.h>

#include "ev00fb/script.h"
#include "ev00fb/tikal_action_5.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 27
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 17
#define TEXT_4_LENGTH 29
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 3
#define TEXT_8_LENGTH 5
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 3
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 1
#define TEXT_13_LENGTH 1
#include "ev00fb/script/text.inc"

extern NJS_ACTION sonic_stand_action;

extern NJS_ACTION ev00fb_sonic_action_0;
extern NJS_ACTION ev00fb_sonic_action_1;
extern NJS_ACTION ev00fb_sonic_action_2;
extern NJS_ACTION ev00fb_tikal_action_0;
extern NJS_ACTION ev00fb_tikal_action_1;
extern NJS_ACTION ev00fb_tikal_action_2;
extern NJS_ACTION ev00fb_tikal_action_3;
extern NJS_ACTION ev00fb_tikal_action_4;

void ev00fb_script(int phase)
{
    /* @hack spelling: event_camera_set_ang called through a local. SHC then
     * keeps its address in R13 and event_camera_set_pos's in R12 (0x0cb8010c,
     * 0x0cb800f6). called by name, the two trade, and 34 of 826 words differ */
    void (*set_ang)(enum event_ease ease, int frames, Angle ang_x, Angle ang_y,
                    Angle ang_z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_TICAL);
        sound_load_file("e_00F8.mlt");
        event_sound_init(5);
        event_set_pos(event_get_player(0), -21.0F, -5.3F, 441.0F);
        event_set_ang(event_get_player(0), 0x04f0, 0x8100, 0);
        event_create_player(2, tikal_player_step, -15.8F, -4.2F, 434.39F,
                            0x04f0, 0x4c00, 0);
        event_wait(1);
        event_talk_start(event_get_player(2), TEXT_7);
        event_play_action(event_get_player(0), &ev00fb_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 16);
        event_play_action(event_get_player(2), &ev00fb_tikal_action_0,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -15.84F, -1.77F, 429.65F);
        set_ang = event_camera_set_ang;
        set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -20.9F, -3.0F, 429.6F);
        set_ang(EVENT_EASE_LINEAR, 100, 0x0a00, 0x8e00, 0x0600);
        event_voice_play(1979);
        event_talk_start(event_get_player(0), TEXT_8);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -22.753F, -2.7F, 430.29F);
        set_ang(EVENT_EASE_LINEAR, 120, 0x0a00, 0x9600, 0);
        event_voice_play(1980);
        event_talk_start(event_get_player(2), TEXT_9);
        event_message_show(TEXT_2);
        event_wait(30);
        event_play_action(event_get_player(2), &ev00fb_tikal_action_1,
                          &tikal_texlist, 0.4F, 0, 4);
        event_voice_wait();
        event_set_pos(event_get_player(0), -21.94F, -4.86F, 443.6F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00fb_tikal_action_4,
                          &tikal_texlist, 0.5F, 0, 4);
        set_ang(EVENT_EASE_LINEAR, 0, 0xf200, 0xf000, 0x0800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -19.5F, -0.337F, 441.088F);
        set_ang(EVENT_EASE_LINEAR, 90, 0xf200, 0xf000, 0x0800);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -19.867F, 0.0F, 441.958F);
        event_voice_play(1981);
        event_talk_start(event_get_player(2), TEXT_10);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait_motions(event_get_player(2));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_play_action(event_get_player(0), &ev00fb_sonic_action_0,
                          &sonic_texlist, 0.3F, EVENT_REPEAT, 0);
        set_ang(EVENT_EASE_LINEAR, 0, 0x135d, 0x6116, 0x00ad);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -13.84F, -4.62F, 436.87F);
        set_ang(EVENT_EASE_LINEAR, 120, 0x145d, 0x8416, 0x00ad);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -22.96F, -3.57F, 434.7F);
        event_voice_play(1982);
        event_talk_start(event_get_player(2), TEXT_11);
        event_message_show(TEXT_4);
        event_wait(80);
        event_message_close();
        event_wait(1);
        event_sound_play(1227, 0, 0, 0);
        event_voice_play(1644);
        event_wait(6);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00fb_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 12);
        set_ang(EVENT_EASE_SMOOTH, 10, 0x025d, 0x8470, 0x0445);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 10, -22.44F, 1.48F, 436.9567F);
        event_message_show(TEXT_5);
        event_clear_motions(event_get_player(2));
        event_wait(15);
        event_message_close();
        event_play_action(event_get_player(2), &ev00fb_tikal_action_2,
                          &tikal_texlist, 0.9F, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -10.647F, -1.3F, 428.7F);
        set_ang(EVENT_EASE_SMOOTH, 0, 0x1600, 0x6a00, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -9.0F, -2.568F, 432.32F);
        set_ang(EVENT_EASE_LINEAR, 50, 0x1e00, 0x4d00, 0xfe00);
        event_set_pos(event_get_player(0), -20.865F, -5.3F, 437.87F);
        event_set_ang(event_get_player(0), 0x06a0, 0x6700, 0);
        event_wait_motions(event_get_player(2));
        event_set_pos(event_get_player(0), -18.74F, -4.9F, 437.0F);
        event_set_ang(event_get_player(0), 0x04f0, 0x83c8, 0);
        event_set_pos(event_get_player(2), -14.0F, -4.2F, 429.4F);
        event_set_ang(event_get_player(2), 0x04f0, 0x8600, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00fb_tikal_action_3,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at(event_get_player(0), -20.6F, 6.338F, 412.669F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -14.88F, -2.0F, 456.0F);
        set_ang(EVENT_EASE_LINEAR, 0, 0x0bf0, 0xfdcc, 0x0500);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -10.0F, 28.0F, 356.0F);
        set_ang(EVENT_EASE_LINEAR, 40, 0x0af0, 0, 0x0300);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.0F, 55.0F, 267.0F);
        set_ang(EVENT_EASE_LINEAR, 0, 0x09f0, 0, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 0.0F, 85.0F, 167.0F);
        set_ang(EVENT_EASE_LINEAR, 40, 0x08f0, 0, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -15.47F, -0.45F, 424.7F);
        set_ang(EVENT_EASE_LINEAR, 0, 0x05f0, 0x77cc, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -16.76F, 10.7F, 428.236F);
        set_ang(EVENT_EASE_LINEAR, 90, 0xdb90, 0x77cc, 0x0600);
        event_play_action(event_get_player(2), &ev00fb_tikal_action_5,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 4);
        event_wait(10);
        event_run_to_speed(event_get_player(2), -28.6F, -1.75F, 417.0F, 0.5F,
                           0.4F);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 5.0F,
                           0.0F);
        event_play_action(event_get_player(0), &ev00fb_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 5);
        event_wait(16);
        event_voice_play(1983);
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_13);
        event_message_show(TEXT_6);
        event_voice_wait();
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), -18.74F, -4.9F, 437.0F);
        event_set_ang(event_get_player(0), 0x04f0, 0x83c8, 0);
        event_reset_player(0);
        event_kill_player(2);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00fb_textures[] = {
    {"TIKAL", &tikal_texlist},
    {NULL, NULL},
};
