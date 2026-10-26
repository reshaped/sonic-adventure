/* EV0015, Sonic's story: Sonic looks for Amy and can't find her. he hears her
 * call, and sees Zero carry her off */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev0015/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/zero.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 34
#define TEXT_2_LENGTH 26
#define TEXT_3_LENGTH 23
#define TEXT_4_LENGTH 16
#define TEXT_5_LENGTH 9
#include "ev0015/script/text.inc"

extern NJS_ACTION ev0015_sonic_action_0;
extern NJS_ACTION ev0015_sonic_action_1;
extern NJS_ACTION ev0015_sonic_action_2;
extern NJS_ACTION ev0015_sonic_action_3;
extern NJS_ACTION ev0015_sonic_action_4;

static struct task *ev0015_zero = NULL;

void ev0015_script(int phase)
{
    void (*set_fov)(Sint8 ease, int frames, Angle fov);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_create_player(0, sonic_step, -427.0F, -2.8F, 1113.3F, 0, 0x8000,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(0), -427.0F, -1.0F, 1113.3F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_place_task(&ev0015_zero, event_zero_create, -304.0F, 175.0F,
                         1241.0F, 0, 0, 0);
        event_wait(1);
        event_set_flags(ev0015_zero, EVENT_IGNORE_GROUND);
        event_zero_set_speed(0.8F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -424.7F, -0.3F, 997.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf900, 0x7b00, 0);
        event_set_pos(event_get_player(0), -426.0F, -1.0F, 1007.3F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_play_action(event_get_player(0), &ev0015_sonic_action_3,
                          &sonic_texlist, 0.6F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -424.5F, 0.3F, 996.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0700, 0x7a00, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1054);
        event_message_show(TEXT_1);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 75, -425.0F, 2.0F, 999.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 75, 0x0900, 0x7a00, 0);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_wait(40);
        event_voice_play(1413);
        event_message_show(TEXT_2);
        event_play_action(event_get_player(0), &ev0015_sonic_action_4,
                          &sonic_texlist, 2.7F, 0, 8);
        event_wait_motions(event_get_player(0));
        stream_request_music(MUSIC_EGGROBO);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0x1800, 0x8100, 0);
        event_play_action(event_get_player(0), &ev0015_sonic_action_1,
                          &sonic_texlist, 0.9F, 0, 16);
        /* @hack spelling: event_camera_set_fov called through a local. SHC then
         * keeps its address in R9 for the five calls (MOV.L at 0x0cb802be), and
         * event_talk_stop's in R8. called by name, each call loads
         * event_camera_set_fov's address from the pool, R9 holds
         * event_talk_stop's and R8 event_play_action's instead, and 371 of
         * ev0015_script's 595 words differ */
        set_fov = event_camera_set_fov;
        set_fov(EVENT_EASE_LINEAR, 8, 0x2fa5);
        event_wait(8);
        set_fov(EVENT_EASE_LINEAR, 10, 0x3111);
        event_wait(30);
        event_set_ang(event_get_player(0), 0, 0x3200, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0015_sonic_action_0,
                          &sonic_texlist, 1.3F, 0, 16);
        event_play_action(event_get_player(0), &ev0015_sonic_action_2,
                          &sonic_texlist, 1.1F, EVENT_REPEAT, 8);
        set_fov(EVENT_EASE_SMOOTH, 0, 0x27d2);
        event_look_at(event_get_player(0), -469.0F, 184.0F, 1231.4F);
        event_move(ev0015_zero, -304.0F, 180.0F, 1240.0F, -740.0F, 169.0F,
                   1248.0F, 290);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -416.2F, 28.4F, 1040.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x14a0, 0x8bb0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, -433.0F, -1.6F, 987.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x1380, 0x80c0, 0);
        event_wait(55);
        event_set_ang(event_get_player(0), 0, 0x1000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -521.8F, 124.2F, 1161.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x154f, 0x92f0, 0x0d00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 55, -545.5F, 155.6F, 1182.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 55, 0x104f, 0x92f0, 0x0d00);
        event_wait(55);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1055);
        event_message_show(TEXT_3);
        event_wait(30);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -413.0F, 3.2F, 1033.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1308, 0x7240, 0xfe00);
        event_camera_follow_clear();
        set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_talk_stop(event_get_player(0));
        event_set_pos(event_get_player(0), -428.4F, -1.8F, 1052.8F);
        event_set_ang(event_get_player(0), 0, 0x1000, 0);
        event_look_at_task(event_get_player(0), ev0015_zero, 0.0F, 8.0F, 0.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -413.4F, -1.3F, 1035.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x1307, 0x7240, 0xfe00);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, -434.26F, -0.24F, 1034.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0x1108, 0x9540, 0xfc00);
        event_wait(80);
        set_fov(EVENT_EASE_SMOOTH, 0, 0x3111);
        event_wait(1);
        event_talk_stop(event_get_player(0));
        event_move_stop_all();
        break;
    case EVENT_END:
        event_reset_player(0);
        event_look_clear(event_get_player(0));
        event_kill_task(&ev0015_zero);
        stream_stop_music();
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0015_textures[] = {
    {"AMY_EGGROBO", &event_zero_texlist},
    {NULL, NULL},
};
