/* EV0092, Knuckles' story: Knuckles follows E-102 through the jungle */
#include <ninja.h>

#include "ev0092/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/e102.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 25
#define TEXT_3_LENGTH 1
#define TEXT_4_LENGTH 2
#include "ev0092/script/text.inc"

extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION knuckles_walk_action;
extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_walk_action;

struct texture_pvm ev0092_textures[] = {
    {"E102", &e102_texlist},
    {NULL, NULL},
};

/* the black fade that ends the event */
static struct task *ev0092_fade = NULL;

void ev0092_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_THEE102);
        event_create_player(2, e102_step, -108.464F, 0.0F, -1699.866F, 0xffef,
                            0x33c1, 0);
        event_wait(1);
        event_play_action(event_get_player(2), &e102_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), -367.72F, 0.0F, -1676.064F);
        event_set_ang(event_get_player(0), 0xffe2, 0x5ed4, 0x001d);
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -380.139F, 7.588F,
                             -1679.489F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0xbeac, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -378.178F, 7.198F,
                             -1679.425F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -84.848F, 15.529F,
                             -1713.205F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0x52ac, 0);
        event_run_to_speed(event_get_player(2), 64.163F, -0.44F, -1688.433F,
                           0.2F, 0.2F);
        event_play_action(event_get_player(2), &e102_walk_action, &e102_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(120);
        event_run_clear(event_get_player(2));
        event_wait(1);
        event_set_pos(event_get_player(2), 64.163F, -0.44F, -1608.433F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 66.853F, 20.636F,
                             -1565.666F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc00, 0, 0);
        event_run_to_speed(event_get_player(2), 71.588F, -0.572F, -1761.806F,
                           0.2F, 0.2F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -380.139F, 7.588F,
                             -1599.489F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_kill_player(2);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_walk_action,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -298.549F, -0.461F, -1725.037F,
                           0.5F, 0.5F);
        event_wait(60);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 35.901F, 6.466F, -1605.867F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xfaac, 0);
        event_set_pos(event_get_player(0), 0.897F, -0.38F, -1587.124F);
        event_set_ang(event_get_player(0), 0xffee, 0x658b, 0xfffd);
        event_wait(1);
        event_run_to_speed(event_get_player(0), 35.269F, -0.451F, -1619.898F,
                           0.7F, 0.7F);
        event_wait(1);
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1627);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1628);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_walk_action,
                          &knuckles_texlist, 0.8F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 71.588F, -0.572F, -1761.806F,
                           0.2F, 0.2F);
        ev0092_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_kill_player(2);
        event_reset_player(0);
        EVENT_KILL(ev0092_fade);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
