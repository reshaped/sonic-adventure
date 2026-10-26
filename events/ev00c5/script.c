/* EV00C5, E-102's story: with E-103 and E-104 rescued, E-102 suspects E-105 and
 * E-101 are still on the Egg Carrier */
#include <ninja.h>

#include "ev00c5/picture_textures.h"
#include "ev00c5/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/picture.h"
#include "shared/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 57
#define TEXT_2_LENGTH 52
#define TEXT_3_LENGTH 35
#include "ev00c5/script/text.inc"

extern NJS_ACTION e102_run_action;
/* @hack type: e102_stand_action declared as an array, which script.c passes by
 * its name. SHC then keeps its address in R9 from 0x0cb800c0 for all four
 * plays. as one NJS_ACTION passed as &e102_stand_action, R9 holds the blend 8
 * instead, and 404 of 492 words differ */
extern NJS_ACTION e102_stand_action[];
extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;

extern NJS_ACTION ev00c5_e102_action_0;
extern NJS_ACTION ev00c5_e102_action_1;

static struct task *ev00c5_picture = NULL;

void ev00c5_script(int phase)
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
        stream_request_music(MUSIC_THEE102);
        event_set_pos(event_get_player(0), -1955.29F, -350.96F, 1652.74F);
        event_set_ang(event_get_player(0), 0, 0x382f, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_run_action, &e102_texlist,
                          1.0F, 0, 0);
        event_wait(1);
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_run_to_speed(event_get_player(0), -1938.34F, -352.37F, 1656.72F,
                           0.8F, 0.06F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1865.67F, -296.15F,
                             1611.39F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xec16, 0x5e8c, 0);
        event_voice_play(1780);
        event_message_wait(0, TEXT_1);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1905.32F, -337.4F,
                             1686.67F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd16, 0x398c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 280, -1898.54F, -337.4F,
                             1644.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 280, 0xfd16, 0x398c, 0);
        event_wait(130);
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 8);
        event_wait(1);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(80);
        event_voice_play(1781);
        event_message_wait(0, TEXT_2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1923.25F, -334.98F,
                             1667.65F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd16, 0x418c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -1923.66F, -334.98F,
                             1656.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xfd16, 0x418c, 0);
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 24);
        event_wait(100);
        event_play_action(event_get_player(0), &ev00c5_e102_action_0,
                          &e102_texlist, 1.0F, 0, 16);
        event_wait(1);
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(120);
        event_message_close();
        event_wait(20);
        event_sound_start(1, 1223, 1800);
        ev00c5_picture = event_picture_create(&ev00c5_picture_texlist);
        event_picture_fade(ev00c5_picture, 1.0F, 16);
        event_wait(46);
        event_picture_fade(ev00c5_picture, 0.0F, 16);
        event_wait(16);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, -1910.84F, -353.59F,
                             1653.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0x1216, 0x418c, 0);
        event_play_action(event_get_player(0), &ev00c5_e102_action_1,
                          &e102_texlist, 0.2F, 0, 8);
        event_voice_play(1782);
        event_message_wait(0, TEXT_3);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1950.78F, -335.74F,
                             1680.91F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xff4a, 0xe1d8, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -1948.08F, -335.81F,
                             1677.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xff4a, 0xe1d8, 0);
        event_set_pos(event_get_player(0), -1924.08F, -353.19F, 1647.25F);
        event_set_ang(event_get_player(0), 0, 0x587d, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), e102_stand_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 16);
        event_wait(90);
        event_message_close();
        break;
    case EVENT_END:
        event_reset_player(0);
        event_set_pos(event_get_player(0), -1924.08F, -353.19F, 1647.25F);
        event_set_ang(event_get_player(0), 0, 0x587d, 0);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        EVENT_KILL(ev00c5_picture);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00c5_textures[] = {
    {"ICM00C5", &ev00c5_picture_texlist},
    {NULL, NULL},
};
