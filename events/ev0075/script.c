/* EV0075, Amy's story: Zero flies off from Station Square with Amy and her
 * bird, and Sonic runs a short way after them */
#include <ninja.h>

#include "ev0075/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/zero.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 24
#define TEXT_2_LENGTH 5
#include "ev0075/script/text.inc"

extern NJS_ACTION sonic_land_action;
/* @hack type: sonic_trot_action declared as an array, which script.c passes
 * by its name. SHC then loads event_wait's address into R13 at 0x0cb80088,
 * and &ev0075_zero into R12 at 0x0cb800e8. as one NJS_ACTION passed as
 * &sonic_trot_action, the two loads swap registers, and 27 of
 * ev0075_script's 493 words differ */
extern NJS_ACTION sonic_trot_action[];
extern NJS_ACTION sonic_stand_action;

/* the fade to black over the event's last 90 steps */
static struct task *ev0075_fade = NULL;
static struct task *ev0075_zero = NULL;

void ev0075_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), -588.0F, 0.0F, 1110.0F);
        event_create_player(2, sonic_step, -427.0F, -2.8F, 1113.3F, 0, 0x8000,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(2), -409.0F, 0.0F, 1060.3F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        ev0075_fade =
            event_fade_create(EVENT_FADE_STILL, 0.0F, 0.0F, 0.0F, 0.0F);
        event_place_task(&ev0075_zero, event_zero_create, -115.0F, 90.0F,
                         1242.8F, 0, 0x0700, 0xef00);
        event_wait(1);
        event_set_flags(ev0075_zero, EVENT_IGNORE_GROUND);
        event_zero_set_speed(1.1F);
        stream_request_music(MUSIC_EGGROBO);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -261.5F, 176.0F, 1291.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xef10, 0xc720, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -266.5F, 156.4F, 1313.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xf1ca, 0xc7d0, 0xfd00);
        event_move(ev0075_zero, -110.0F, 103.0F, 1247.0F, -226.6F, 143.0F,
                   1304.7F, 64);
        event_wait(64);
        event_move_stop(ev0075_zero);
        event_set_pos(ev0075_zero, -330.0F, 170.0F, 1245.8F);
        event_set_ang(ev0075_zero, 0, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -300.5F, 175.8F, 1227.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 0, 0x360b);
        event_wait(1);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, ev0075_zero, -3.0F,
                                  22.0F, -35.0F, 0);
        event_wait(1);
        event_camera_follow(ev0075_zero);
        event_move(ev0075_zero, -330.0F, 182.0F, 1245.8F, -740.0F, 181.0F,
                   1248.0F, 290);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x1555);
        event_wait(60);
        event_set_pos(ev0075_zero, -343.3F, 182.0F, 1221.8F);
        event_set_ang(ev0075_zero, 0, 0x4000, 0xef00);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 35, event_get_player(2),
                                  0.0F, 5.0F, 0.0F, 0);
        event_voice_play(1413);
        event_message_show(TEXT_1);
        event_wait(15);
        event_play_action(event_get_player(2), &sonic_land_action,
                          &sonic_texlist, 1.7F, 0, 8);
        event_wait_motions(event_get_player(2));
        event_set_ang(event_get_player(2), 0, 0xd200, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), sonic_trot_action,
                          &sonic_texlist, 0.9F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(2), ev0075_zero, 0.0F, 5.0F, 0.0F);
        event_run_to_speed(event_get_player(2), -421.8F, 1.0F, 1068.0F, 2.5F,
                           1.1F);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_2);
        event_wait(3);
        event_wait(60);
        event_talk_stop(event_get_player(2));
        event_set_ang(event_get_player(2), 0, 0xd000, 0);
        event_play_action(event_get_player(2), &sonic_stand_action,
                          &sonic_texlist, 1.2F, EVENT_REPEAT, 12);
        event_camera_look_clear();
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, -666.5F, 150.0F, 1273.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0xfd0f, 0xc020, 0x0100);
        event_message_close();
        if (ev0075_fade != NULL) {
            event_fade_set_speed(ev0075_fade, 0.025F);
        }
        event_wait(90);
        event_talk_stop(event_get_player(0));
        event_move_stop_all();
        event_camera_follow_clear();
        event_camera_look_clear();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_kill_player(2);
        event_move_stop_all();
        event_kill_task(&ev0075_zero);
        EVENT_KILL(ev0075_fade);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 0, 0x3111);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0075_textures[] = {
    {"AMY_EGGROBO", &event_zero_texlist},
    {NULL, NULL},
};
