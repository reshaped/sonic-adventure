/* EV00E3: at Station Square's station, Big spots Froggy, who hops away */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/traffic.h"
#include "ev00e3/frog.h"
#include "ev00e3/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_models.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

extern NJS_TEXLIST ev00e3_frog_texlist;
/* @hack type: Froggy's actions declared as arrays. SHC then keeps
 * event_get_player's address in R12 and event_play_action's in R11 (0x0cb80080,
 * 0x0cb800ec). as single objects, the two trade, and 16 of 426 words differ */
extern NJS_ACTION ev00e3_frog_action_0[];
extern NJS_ACTION ev00e3_frog_action_1[];
extern NJS_ACTION ev00e3_frog_action_2[];
extern NJS_ACTION ev00e3_frog_action_3[];

static struct task *ev00e3_black = NULL;
static struct task *ev00e3_frog = NULL;

void ev00e3_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_S_SQUARE);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb00, 0x1216, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 78.17F, 16.84F, 1492.26F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, 98.75F, 16.84F, 1477.13F);
        event_place_task(&ev00e3_frog, ev00e3_frog_create, -36.79F, 1.7F,
                         1665.08F, 0, 0x9f39, 0);
        event_wait(1);
        event_set_flags(ev00e3_frog, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_clear_motions(ev00e3_frog);
        event_play_action(ev00e3_frog, ev00e3_frog_action_0,
                          &ev00e3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), 62.4F, 0.0F, 1458.08F);
        event_set_ang(event_get_player(0), 0, 0x12fb, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(50);
        event_flags[FLAG_TRAFFIC] = FALSE;
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 87.95F, 17.88F, 1426.17F);
        event_wait(120);
        event_camera_look_clear();
        event_clear_motions(event_get_player(0));
        event_play_action(ev00e3_frog, ev00e3_frog_action_1,
                          &ev00e3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(40);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30, ev00e3_frog, 0.0F,
                                  1.0F, 0.0F, 0);
        event_play_action(ev00e3_frog, ev00e3_frog_action_0,
                          &ev00e3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 47.73F, 2.78F, 1488.19F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x6e16, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 30, 0x0222);
        event_play_action(ev00e3_frog, ev00e3_frog_action_2,
                          &ev00e3_frog_texlist, 1.0F, 0, 8);
        event_wait_motions(ev00e3_frog);
        event_play_action(ev00e3_frog, ev00e3_frog_action_3,
                          &ev00e3_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(50);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0x8d16, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -85.11F, 35.54F, 1563.83F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, ev00e3_frog, 0.0F, 0.0F,
                                  0.0F, 0);
        event_play_action(ev00e3_frog, ev00e3_frog_action_1,
                          &ev00e3_frog_texlist, 0.8F, EVENT_REPEAT, 0);
        event_set_ang(ev00e3_frog, 0, 0xbf39, 0);
        event_wait(1);
        event_move(ev00e3_frog, -36.79F, 1.7F, 1665.08F, -298.69F, 0.0F,
                   1690.44F, 380);
        event_wait(60);
        event_camera_look_clear();
        event_wait(160);
        event_move_stop(ev00e3_frog);
        event_kill_task(&ev00e3_frog);
        event_run_to_speed(event_get_player(0), 44.54F, 0.0F, 1492.41F, 0.0F,
                           3.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf55c, 0x6067, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 71.23F, 20.75F, 1466.94F);
        event_wait(30);
        ev00e3_black = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_reset_player(0);
        event_kill_task(&ev00e3_frog);
        event_flags[FLAG_TRAFFIC] = TRUE;
        EVENT_KILL(ev00e3_black);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00e3_textures[] = {
    {"FROG", &ev00e3_frog_texlist},
    {NULL, NULL},
};
