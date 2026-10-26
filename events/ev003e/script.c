/* EV003E, Tails' story: Tails dreams of running after Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev003e/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/player.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 1
#define TEXT_2_LENGTH 7
#include "ev003e/script/text.inc"

/* SET_FADE_SPEED sets the speed of the fade in slot, and SET_FADE_DEPTH its
 * depth, if slot holds one */
/* @hack spelling: each macro's test and call as one expression. SHC then keeps
 * event_set_ang's address in R10 from 0x0cb80112 to 0x0cb80a2a. it loads
 * event_fade_set_speed's and event_fade_set_depth's from a pool at each call.
 * as ifs, R10 holds event_fade_set_speed's address, the code runs 12 bytes
 * short, and 1035 of 1316 words differ */
#define SET_FADE_SPEED(slot, speed)                                            \
    ((slot) != NULL && (event_fade_set_speed((slot), (speed)), 0))
#define SET_FADE_DEPTH(slot, depth)                                            \
    ((slot) != NULL && (event_fade_set_depth((slot), (depth)), 0))

extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_stand_action;

extern NJS_ACTION ev003e_tails_action_0;
extern NJS_ACTION ev003e_tails_action_1;
extern NJS_ACTION ev003e_tails_action_2;

/* the path Sonic runs, and Tails after him */
static struct event_path_point ev003e_points[213] = {
#include "ev003e/script/ev003e_points.inc"
};

static struct event_path ev003e_path = {213, ev003e_points};

static struct task *ev003e_amber = NULL;
static struct task *ev003e_black = NULL;
/* Tails' upgrades from before the event takes his rhythm brooch away */
static Sint16 ev003e_upgrades = 0;
static Bool ev003e_upgrades_saved = FALSE;

void ev003e_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        ev003e_upgrades = player_get(0)->upgrades;
        ev003e_upgrades_saved = TRUE;
        player_get(0)->upgrades &= ~PLAYER_TAILS_RHYTHM_BROOCH;
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_reset_player(0);
        stream_request_music(MUSIC_EVTBGM00);
        /* the fades are made below, so both slots are NULL and these set no
         * depth */
        SET_FADE_DEPTH(ev003e_black, -1.0F);
        SET_FADE_DEPTH(ev003e_amber, -1.1F);
        ev003e_black = event_fade_create(-0.001F, 0.8F, 0.0F, 0.0F, 0.0F);
        ev003e_amber =
            event_fade_create(-0.0000000001F, 0.36F, 0.6F, 0.44F, 0.16F);
        event_set_pos(event_get_player(0), -955.6F, 9.82F, -181.6F);
        event_set_ang(event_get_player(0), 0xffc0, 0x2800, 0xfcf7);
        event_create_player(2, sonic_step, -701.7F, 4.0F, 100.1F, 0, 0xfa00, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), -779.4F, 1.21F, -106.77F);
        event_set_ang(event_get_player(2), 0, 0x1049, 0);
        SET_FADE_SPEED(ev003e_black, -0.005F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -962.1F, 13.22F, -187.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xa400, 0x0300);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -963.8F, 12.6F, -189.75F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x0933, 0xa057, 0x0300);
        event_wait(100);
        event_play_action(event_get_player(0), &ev003e_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -947.7F, 13.2F, -183.9F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 0.0F, 0.0F, 0);
        event_wait(5);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 45, event_get_player(0),
                                  0.0F, 3.5F, 0.0F, 0xfb00);
        event_run_to_speed(event_get_player(0), -892.5F, 12.4F, -153.0F, 0.32F,
                           0.08F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -867.1F, 38.45F, -159.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf8af, 0x7097, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, -867.1F, 30.57F, -161.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0xf8af, 0x7397, 0);
        SET_FADE_SPEED(ev003e_amber, 0.002F);
        event_wait(50);
        SET_FADE_SPEED(ev003e_amber, -0.0015F);
        event_wait(65);
        SET_FADE_SPEED(ev003e_amber, -0.00000000001F);
        event_wait(20);
        event_run_clear(event_get_player(0));
        event_camera_look_clear();
        event_set_pos(event_get_player(0), -805.2F, 12.7F, 51.57F);
        event_set_ang(event_get_player(0), 0xfd00, 0x1208, 0xfc00);
        event_set_pos(event_get_player(2), -778.7F, 13.77F, 105.55F);
        event_set_ang(event_get_player(2), 0x0b00, 0xea00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -791.6F, 14.0F, 108.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x04af, 0x0997, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, -796.6F, 16.7F, 113.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0x04af, 0x0897, 0);
        event_run_to_speed(event_get_player(0), -799.7F, 12.66F, 104.31F, 0.3F,
                           0.3F);
        SET_FADE_SPEED(ev003e_amber, 0.0027F);
        event_wait(50);
        SET_FADE_SPEED(ev003e_amber, -0.0015F);
        event_wait(60);
        SET_FADE_SPEED(ev003e_amber, -0.00000000001F);
        event_wait(18);
        event_wait(35);
        event_play_action(event_get_player(0), &ev003e_tails_action_1,
                          &tails_texlist, 1.0F, 0, 4);
        event_play_action(event_get_player(0), &ev003e_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_sound_play(699, 128, 0, 0);
        event_run_to_speed(event_get_player(2), -825.4F, 24.33F, 109.9F, 8.0F,
                           4.0F);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_1);
        event_wait(20);
        event_wait(20);
        event_run_clear(event_get_player(2));
        event_set_pos(event_get_player(2), -839.7F, 29.77F, 167.4F);
        event_set_ang(event_get_player(2), 0xfb00, 0xca00, 0);
        event_talk_stop(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -791.48F, 14.0F, 105.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x069e, 0x2c02, 0x0600);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -793.7F, 20.3F, 107.15F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xf39e, 0x1d02, 0x0600);
        event_wait(90);
        event_set_ang(event_get_player(0), 0, 0xf000, 0);
        event_play_action(event_get_player(0), &tails_stand_action,
                          &tails_texlist, 0.4F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -784.2F, 11.7F, 98.36F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b9e, 0x5c02, 0x0400);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -788.2F, 12.6F, 92.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0b9e, 0x5c02, 0x0400);
        event_run_to_speed(event_get_player(2), -1086.18F, 131.7F, 373.9F, 8.0F,
                           2.0F);
        SET_FADE_SPEED(ev003e_amber, 0.003F);
        event_wait(50);
        SET_FADE_SPEED(ev003e_amber, -0.003F);
        event_wait(50);
        SET_FADE_SPEED(ev003e_amber, -0.00000000001F);
        event_set_pos(event_get_player(2), -778.7F, 13.77F, 105.55F);
        event_set_ang(event_get_player(2), 0x0b00, 0xea00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -798.7F, 18.0F, 109.78F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0a9e, 0x0902, 0x0300);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -798.9F, 18.3F, 108.8F);
        event_wait(50);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, -793.6F, 15.31F, 101.37F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0x099e, 0x5e02, 0x0300);
        event_clear_motions(event_get_player(0));
        event_run_to_speed(event_get_player(0), -916.7F, 100.66F, 240.5F, 1.0F,
                           0.2F);
        event_wait(30);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 40, event_get_player(0),
                                  0.0F, 4.5F, 0.0F, 0);
        event_wait(80);
        event_run_clear(event_get_player(0));
        event_set_pos(event_get_player(0), -763.7F, 9.2F, 107.3F);
        event_set_ang(event_get_player(0), 0xfb00, 0xda00, 0);
        event_set_pos(event_get_player(2), -836.7F, 24.77F, 144.9F);
        event_set_ang(event_get_player(2), 0xfb00, 0xe000, 0);
        event_play_action(event_get_player(2), &sonic_trot_action,
                          &sonic_texlist, 1.2F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &tails_run_action,
                          &tails_texlist, 1.3F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(0), event_get_player(2), 0.0F, 5.0F,
                           0.0F);
        event_set_pos(event_get_player(0), -1317.6F, 78.0F, -717.34F);
        event_set_ang(event_get_player(0), 0, 0x9000, 0);
        event_play_path(event_get_player(2), &ev003e_path, 0.6F, 0);
        event_wait(8);
        event_play_path(event_get_player(0), &ev003e_path, 0.6F, 0);
        event_look_clear(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(2)->work->pos.x - 5.0F,
                             event_get_player(2)->work->pos.y + 7.0F,
                             event_get_player(2)->work->pos.z + 28.0F);
        event_camera_follow(event_get_player(2));
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(2),
                                  0.0F, 5.0F, 0.0F, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(2)->work->pos.x + 6.0F,
                             event_get_player(2)->work->pos.y + 8.0F,
                             event_get_player(2)->work->pos.z - 10.0F);
        event_camera_follow(event_get_player(2));
        SET_FADE_SPEED(ev003e_amber, -0.0016F);
        event_wait(100);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->pos.x + 1.0F,
                             event_get_player(0)->work->pos.y + 5.5F,
                             event_get_player(0)->work->pos.z - 10.0F);
        event_camera_follow(event_get_player(0));
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  -2.0F, 5.0F, -2.5F, 0);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(2)->work->pos.x - 4.0F,
                             event_get_player(2)->work->pos.y + 5.0F,
                             event_get_player(2)->work->pos.z - 8.0F);
        event_camera_follow(event_get_player(2));
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(2),
                                  0.0F, 5.0F, 0.0F, 0);
        event_wait(50);
        event_camera_look_clear();
        event_camera_follow_clear();
        event_clear_paths(event_get_player(2));
        event_set_pos(event_get_player(2), -1055.3F, 1.0F, -1433.6F);
        if (ev003e_amber != NULL) {
            event_fade_set_color(ev003e_amber, 1.0F, 1.0F, 1.0F);
        }
        SET_FADE_SPEED(ev003e_amber, 0.002F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1067.2F, 1.48F, -1423.27F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0dc9, 0xd6ab, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, -1078.6F, 0.8F, -1438.27F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0x0bc9, 0xdcab, 0xff00);
        event_run_to_speed(event_get_player(0), -956.0F, 0.0F, -1595.8F, 0.9F,
                           0.2F);
        event_run_to_speed(event_get_player(2), -963.45F, 0.0F, -1598.66F, 0.7F,
                           0.02F);
        event_wait(190);
        SET_FADE_SPEED(ev003e_black, 0.028F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        break;
    case EVENT_END:
        if (ev003e_upgrades_saved) {
            player_get(0)->upgrades = ev003e_upgrades;
            ev003e_upgrades_saved = FALSE;
        }
        event_camera_release();
        event_pads_on();
        stream_stop_music();
        event_reset_player(0);
        EVENT_KILL(ev003e_amber);
        EVENT_KILL(ev003e_black);
        event_kill_player(2);
        event_clear_paths(event_get_player(0));
        event_clear_paths(event_get_player(2));
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev003e_textures[] = {{NULL, NULL}};
