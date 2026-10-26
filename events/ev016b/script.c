/* EV016B: Knuckles takes the Shovel Claws */
#include <ninja.h>

#include "ev016b/knuckles_action_2.h"
#include "ev016b/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 58
#define TEXT_2_LENGTH 54
#include "ev016b/script/text.inc"

/* @hack type: ev016b_knuckles_action_0 declared as an array, which script.c
 * passes by its name. SHC then keeps each call's load of the 8 it passes as
 * blend, as the image's MOV #8,R2 at 0x0cb800c2. as one NJS_ACTION passed as
 * &ev016b_knuckles_action_0, SHC holds the 8 in R11. ev016b_script then saves
 * R8 besides R9 to R14. event_camera_follow_clear moves from R12 to R8,
 * event_camera_look_clear from R10 to R12, and event_clear_paths from R11 to
 * R10. 169 of 340 words differ */
extern NJS_ACTION ev016b_knuckles_action_0[];
extern NJS_ACTION ev016b_knuckles_action_1;

static struct event_path_point ev016b_light_points[483] = {
#include "ev016b/script/ev016b_light_points.inc"
};

static struct event_path ev016b_light_path = {483, ev016b_light_points};

static struct task *ev016b_light = NULL;

void ev016b_script(int phase)
{
    /* ev016b_script loads e_000d.mlt whatever phase it is called with. at
     * EVENT_END and EVENT_UNLOAD, after the event is over, it reads the file
     * into WORK_AREA again. every other event that loads a sound file loads it
     * at one phase */
    sound_load_file("e_000d.mlt");

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_voice_play(VOICE_UPGRADE_CHIME);
        event_wait(1);
        event_set_pos(event_get_player(0), 1300.0F, -1.56F, -290.0F);
        event_set_ang(event_get_player(0), 0, 0x38f8, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), ev016b_knuckles_action_0,
                          &knuckles_texlist, 0.1F, 0, 8);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 4.5F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 250, event_get_player(0), 8.0F, 0,
                           0x5555, 0, 0, -0xc71b, 0, 16.0F, 16.0F);
        event_voice_play(475);
        event_message_wait(0, TEXT_1);
        ev016b_light =
            event_light_create(event_get_player(0)->work->pos.x + 7.535F,
                               event_get_player(0)->work->pos.y + 27.0F,
                               event_get_player(0)->work->pos.z + 13.89F);
        event_wait(1);
        event_set_flags(ev016b_light, EVENT_IGNORE_GROUND);
        event_clear_paths(ev016b_light);
        event_play_path(ev016b_light, &ev016b_light_path, 2.0F,
                        EVENT_FROM_HERE);
        event_play_action(event_get_player(0), &ev016b_knuckles_action_1,
                          &knuckles_texlist, 0.2F, EVENT_REPEAT, 8);
        event_wait(35);
        event_wait(80);
        event_play_action(event_get_player(0), &ev016b_knuckles_action_2,
                          &knuckles_texlist, 0.4F, 0, 8);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, 1308.79F, 8.46F, -288.44F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 150, 0xfa3a, 0x38e4, 0);
        event_wait(30);
        event_message_wait(0, TEXT_2);
        event_wait(5);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -80, 80);
        event_wait(100);
        event_message_close();
        event_wait(10);
        event_clear_paths(ev016b_light);
        EVENT_KILL(ev016b_light);
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_clear_paths(ev016b_light);
        event_reset_player(0);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_voice_stop();
        event_message_close();
        EVENT_KILL(ev016b_light);
        event_camera_release();
        event_pads_on();
        /* a second event_camera_release and event_pads_on. they change
         * nothing: the event doesn't have the camera after the first release,
         * and the pads already drive the players */
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev016b_textures[] = {{NULL, NULL}};
