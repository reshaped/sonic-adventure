/* EV009F, Knuckles' story: Knuckles stays on the island to guard the Master
 * Emerald */
#include <ninja.h>

#include "adv02/master_emerald.h"
#include "ev009f/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/sparkle.h"
#include "shared/sprite.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 44
#define TEXT_2_LENGTH 37
#define TEXT_3_LENGTH 52
#define TEXT_4_LENGTH 35
#define TEXT_5_LENGTH 40
#include "ev009f/script/text.inc"

extern NJS_ACTION ev009f_knuckles_action_0;

/* the sparkle's start and speed. event_sparkle_create_shrinking copies
 * both, and nothing reads them after the call */
static NJS_POINT3 ev009f_sparkle_pos;
static NJS_VECTOR ev009f_sparkle_speed;

void ev009f_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_set_pos(event_get_player(0), -3.0F, -157.083F, 2251.0F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_reset_player(0);
        event_play_action(event_get_player(0), &ev009f_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_5);
        event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_WHOLE;
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 158.28F, -74.886F,
                             2620.085F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf100, 0x0800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -25.4F, -78.26F, 2629.17F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0xf100, 0xfe00, 0);
        event_wait(30);
        event_voice_play(1664);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_wait(25);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2e39);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.27F, -139.6F, 2262.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0300, 0xf9f7, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -6.39F, -148.39F,
                             2261.825F);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, -6.4688F, -154.37F,
                             2261.374F);
        event_wait(10);
        event_voice_play(1665);
        event_message_show(TEXT_2);
        event_wait(120);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -49.12F, -122.21F,
                             2084.614F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0x9500, 0xff00);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x1200, 0x9500, 0xff00);
        event_wait(50);
        event_voice_play(1666);
        event_message_show(TEXT_3);
        event_wait(90);
        event_sprite_create_loop(NULL, 0, 120.0F, 60.0F, 2140.0F,
                                 &event_sprite_texlists[EVENT_SPRITE_SPARKLE]);
        event_sprite_set_scale(0, -9.0F, -9.0F);
        event_wait(8);
        event_sprite_kill(0);
        event_wait(10);
        ev009f_sparkle_pos.x = 120.0F;
        ev009f_sparkle_pos.y = 60.0F;
        ev009f_sparkle_pos.z = 2140.0F;
        ev009f_sparkle_speed.x = -2.0F;
        ev009f_sparkle_speed.y = -3.0F;
        ev009f_sparkle_speed.z = 3.0F;
        event_sparkle_create_shrinking(&ev009f_sparkle_pos,
                                       &ev009f_sparkle_speed, 1.0F, 0.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x1700, 0x9500, 0xff00);
        event_wait(20);
        event_voice_play(1667);
        event_message_show(TEXT_4);
        event_wait(150);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sprite_kill(0);
        event_wait(1);
        event_reset_player(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev009f_textures[] = {{NULL, NULL}};
