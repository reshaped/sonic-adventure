/* EV009D, Knuckles' story: Knuckles restores the Master Emerald */
#include <ninja.h>

#include "adv02/master_emerald.h"
#include "ev009d/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/orbs.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 16
#define TEXT_2_LENGTH 43
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 14
#include "ev009d/script/text.inc"

/* set ev009d_fade's speed, if it holds a fade */
/* @hack spelling: the test and the call as one expression, as
 * src/event/play.h's EVENT_KILL has them. SHC then keeps the address of
 * event_flags[FLAG_MASTER_EMERALD] in a stack slot from 0x0cb80070 to
 * 0x0cb807be. as an if, SHC loads the address again, script.c's code ends 12
 * bytes short of the image's, and 911 of 1099 words differ */
#define SET_FADE_SPEED(speed)                                                  \
    (ev009d_fade != NULL && (event_fade_set_speed(ev009d_fade, speed), 0))

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_walk_action;

extern NJS_ACTION ev009d_knuckles_action_0;
extern NJS_ACTION ev009d_knuckles_action_1;
/* @hack type: ev009d_knuckles_action_2 declared as an array, which script.c
 * passes by its name. SHC then keeps -2.87 in FR12 and -158 in FR13, the x and
 * y of Knuckles' place, from 0x0cb80300 to 0x0cb8043a. as one NJS_ACTION passed
 * as &ev009d_knuckles_action_2, SHC holds only -2.87, in FR13, and loads -158
 * for each call. script.c's code then ends 8 bytes short of the image's, and
 * 825 of 1099 words differ */
extern NJS_ACTION ev009d_knuckles_action_2[];
extern NJS_ACTION ev009d_knuckles_action_3;

static struct task *ev009d_fade = NULL;

void ev009d_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_009D.mlt");
        event_sound_init(3);
        /* the emerald's object sets the same for Knuckles once event 145
         * has played */
        event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_INCOMPLETE;
        event_reset_player(0);
        event_set_pos(event_get_player(0), -3.4F, -247.0F, 2469.6F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_clear_motions(event_get_player(0));
        stream_request_music(MUSIC_MSTCLN);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -5.45F, -246.4F, 2460.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0572, 0x89b3, 0xff00);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x1472, 0x89b3, 0xff00);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -3.7F, -241.9F, 2480.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf150, 0xfd70, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -4.07F, -243.5F, 2499.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0c50, 0xfe70, 0);
        event_wait(60);
        event_run_to_speed(event_get_player(0), -2.5F, -157.0F, 2239.4F, 1.8F,
                           0.5F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90,
                             event_get_player(0)->work->pos.x - 0.7F,
                             event_get_player(0)->work->pos.y + 8.0F,
                             event_get_player(0)->work->pos.z + 18.6F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 40, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_follow(event_get_player(0));
        event_wait(95);
        event_camera_follow_clear();
        event_wait(15);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -39.2F, -156.4F, 2138.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x05a0, 0x9485, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -39.6F, -149.5F, 2138.0F);
        event_wait(100);
        event_play_action(event_get_player(0), &knuckles_walk_action,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -7.37F, -156.0F, 2234.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x03a0, 0xa885, 0);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 40, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_run_to_speed(event_get_player(0), -2.87F, -158.0F, 2201.9F, 0.35F,
                           0.06F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 70, -3.37F, -156.0F, 2234.67F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 70, 0x04f5, 0xfe61, 0);
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 16);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, -3.19F, -155.4F, 2229.67F);
        event_wait(110);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev009d_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), -2.87F, -158.0F, 2201.9F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -7.45F, -153.6F, 2195.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa4f, 0x9b7a, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -5.57F, -150.67F, 2194.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xfc4f, 0x8e7a, 0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -6.6F, -150.4F, 2192.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xfc4f, 0x8e7a, 0);
        event_play_action(event_get_player(0), &ev009d_knuckles_action_1,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(20);
        event_orbs_start(3);
        event_wait(15);
        event_rays_create(-2.87F, -151.0F, 2197.9F, 1, 10, 0.4F, 1.2F,
                          0xff50dc50, 0x00325a32);
        event_wait(45);
        event_sound_play(1225, 128, 0, 0);
        event_wait(20);
        event_play_action(event_get_player(0), ev009d_knuckles_action_2,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &ev009d_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(25);
        event_rays_kill();
        event_wait(20);
        event_sound_start(2, 1224, 1800);
        event_sound_fade_volume(2, -10, 1);
        event_wait(1);
        event_sound_fade_volume(2, 90, 220);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -34.6F, -147.6F, 2162.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x084f, 0xaa7a, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -55.57F, -155.6F, 2156.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x284f, 0xb17a, 0x0e00);
        event_play_action(event_get_player(0), ev009d_knuckles_action_2,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(0), &ev009d_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x21eb, 0xc1ba, 0x0e00);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 44.57F, -20.4F, 2195.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xd54f, 0x1e7a, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -17.45F, -150.9F, 2223.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x044f, 0xf57a, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 30, 0x238e);
        event_wait(40);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x31c7);
        event_wait(60);
        event_sound_stop(2);
        event_sound_play(1225, 128, 0, 0);
        event_sound_play(1224, 128, 0, 0);
        event_rays_create(-3.52F, -136.0F, 2151.0F, 2, 12, 2.0F, 5.0F,
                          0xff50dc50, 0x00325a32);
        /* @bug logic: sets no speed, here or at the next SET_FADE_SPEED:
         * ev009d_fade stays NULL until event_fade_create below makes the fade
         * (JSR at 0x0cb80b44). so each test of ev009d_fade (TST at 0x0cb807ac
         * and 0x0cb807cc) skips its call */
        SET_FADE_SPEED(0.028F);
        event_wait(40);
        event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_WHOLE;
        event_wait(20);
        SET_FADE_SPEED(-0.022F);
        event_wait(20);
        event_rays_kill();
        event_set_pos(event_get_player(0), -2.45F, -158.0F, 2192.4F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -15.1F, -149.7F, 2188.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b4f, 0xf37a, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -17.4F, -154.9F, 2207.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0e4f, 0xf1f5, 0);
        event_play_action(event_get_player(0), &ev009d_knuckles_action_0,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 16);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1662);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_wait(10);
        event_talk_stop(event_get_player(0));
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 3.7F, -153.9F, 2181.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b4f, 0x6b7a, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1663);
        event_message_show(TEXT_2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 1.37F, -152.5F, 2185.9F);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 0.44F, -152.0F, 2187.6F);
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_walk_action,
                          &knuckles_texlist, 0.55F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -5.18F, -155.2F, 2206.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x064f, 0xfbeb, 0);
        /* @bug logic: gives the run a target y of 158, not Knuckles' -158
         * (158.0F at 0x0cb80adc, loaded at 0x0cb80a00 for the JSR at
         * 0x0cb80a0e). a player's run measures its target in x and z alone,
         * so the y changes nothing */
        event_run_to_speed(event_get_player(0), -2.74F, 158.0F, 2246.5F, 0.15F,
                           0.08F);
        event_camera_follow(event_get_player(0));
        event_wait(80);
        event_rays_create(-3.52F, -136.0F, 2151.0F, 1, 4, 1.0F, 3.0F,
                          0xff50dc50, 0x00325a32);
        event_wait(20);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -2.55F, -155.92F, 2280.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0f4f, 0xff7a, 0);
        event_rays_kill();
        event_rays_create(-3.52F, -136.0F, 2151.0F, 1, 8, 1.0F, 6.0F,
                          0xff28aa28, 0x00145014);
        event_wait(30);
        ev009d_fade = event_fade_create(0.02F, 0.02F, 0.7F, 1.0F, 0.7F);
        SET_FADE_SPEED(0.005F);
        event_wait(70);
        SET_FADE_SPEED(-0.01F);
        event_rays_kill();
        event_rays_create(-3.52F, -136.0F, 2151.0F, 1, 8, 1.3F, 7.0F,
                          0xff28aa28, 0x00145014);
        event_wait(20);
        SET_FADE_SPEED(0.01F);
        event_wait(110);
        event_rays_kill();
        event_rays_kill();
        EVENT_KILL(ev009d_fade);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_sound_stop_all();
        event_voice_stop();
        event_rays_kill();
        EVENT_KILL(ev009d_fade);
        event_orbs_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev009d_textures[] = {{NULL, NULL}};
