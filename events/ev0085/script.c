/* EV0085, Knuckles' story: in the past, Tikal pleads with her father
 * Pachacamac, who wants the seven Chaos Emeralds */
#include <ninja.h>

#include "ev0085/pachacamac_textures.h"
#include "ev0085/script.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 17
#define TEXT_2_LENGTH 47
#define TEXT_3_LENGTH 27
#define TEXT_4_LENGTH 30
#define TEXT_5_LENGTH 54
#define TEXT_6_LENGTH 27
#define TEXT_7_LENGTH 54
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 2
#define TEXT_10_LENGTH 2
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 3
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 2
#include "ev0085/script/text.inc"

extern NJS_ACTION ev0085_pachacamac_action_0;
extern NJS_ACTION ev0085_pachacamac_action_1;
extern NJS_ACTION ev0085_tikal_action_0;
extern NJS_ACTION ev0085_tikal_action_1;
extern NJS_ACTION ev0085_tikal_action_2;

static struct task *ev0085_pachacamac = NULL;
/* the white fade that ends the event */
static struct task *ev0085_fade = NULL;

void ev0085_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_sound_init(1);
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), 0.0F, 0.0F, 973.08F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_create_player(2, tikal_player_step, 5.138F, 0.0F, 1212.25F, 0,
                            0xe800, 0);
        event_wait(1);
        event_place_actor(
            &ev0085_pachacamac, event_get_player(2)->work->pos.x - 10.0F,
            event_get_player(2)->work->pos.y + 7.0F,
            event_get_player(2)->work->pos.z + 10.0F, 0, 0x3800, 0);
        event_play_action(ev0085_pachacamac, &ev0085_pachacamac_action_1,
                          &ev0085_pachacamac_texlist, 1.0F, EVENT_REPEAT, 4);
        event_set_flags(ev0085_pachacamac, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0085_pachacamac, 1.0F);
        event_play_action(event_get_player(2), &ev0085_tikal_action_1,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 4);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, 8.654F, 1237.928F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x038e);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x31c7);
        event_wait(120);
        event_talk_start(event_get_player(2), TEXT_9);
        event_voice_play(1553);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.331F, 4.3F, 1228.047F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0d00, 0x3c00, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0d00, 0x3000, 0);
        event_voice_play(1554);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_wait(30);
        event_voice_play(1555);
        event_message_show(TEXT_3);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.27F, 7.654F, 1218.332F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xe500, 0);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 1.5F, 7.654F, 1216.755F);
        event_voice_play(1556);
        event_talk_start(event_get_player(2), TEXT_10);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0085_tikal_action_0,
                          &tikal_texlist, 0.4F, 0, 0);
        event_play_action(event_get_player(2), &ev0085_tikal_action_2,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.793F, 4.848F, 1238.687F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0600, 0x0300, 0);
        event_set_ang(event_get_player(2), 0, 0xe800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -18.92F, 6.277F,
                             1206.072F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0400, 0xa700, 0);
        event_voice_play(1557);
        event_talk_start(event_get_player(2), TEXT_11);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(30);
        event_voice_play(1558);
        event_talk_start(event_get_player(2), TEXT_12);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 3.63F, 6.134F, 1234.204F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0400, 0x1b00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 7.331F, 12.1F, 1230.95F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0xf600, 0x2900, 0);
        event_voice_play(1559);
        event_talk_start(event_get_player(2), TEXT_13);
        event_message_show(TEXT_7);
        event_wait(1);
        event_play_action(ev0085_pachacamac, &ev0085_pachacamac_action_0,
                          &ev0085_pachacamac_texlist, 0.5F, 0, 4);
        event_play_action(ev0085_pachacamac, &ev0085_pachacamac_action_1,
                          &ev0085_pachacamac_texlist, 1.0F, EVENT_REPEAT, 4);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1.5F, 7.654F, 1216.755F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xe500, 0);
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1560);
        event_message_show(TEXT_8);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.296F, 2.093F, 1015.216F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0600, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x4e39);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 0.296F, 7.375F, 979.605F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 100, 0x31c7);
        event_wait(30);
        event_sound_start(0, SE_TIKAL_CHIME, 88);
        ev0085_fade = event_fade_create(1.0F / 60.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        EVENT_KILL(ev0085_fade);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0085_pachacamac);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0085_textures[] = {
    {"EV_K_PATYA", &ev0085_pachacamac_texlist},
    {"TIKAL", &tikal_texlist},
    {NULL, NULL},
};
