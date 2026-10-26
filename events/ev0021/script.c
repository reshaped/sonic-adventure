/* EV0021, Sonic's story: a white flash fades to show Sonic in the past. he asks
 * what is going on and where he is */
#include <ninja.h>

#include "ev0021/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 26
#define TEXT_2_LENGTH 21
#define TEXT_3_LENGTH 1
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 2
#include "ev0021/script/text.inc"

extern NJS_ACTION sonic_stand_action;

extern NJS_ACTION ev0021_sonic_action_0;
extern NJS_ACTION ev0021_sonic_action_1;

static struct task *ev0021_flash = NULL;

struct texture_pvm ev0021_textures[] = {{NULL, NULL}};

void ev0021_script(int phase)
{
    /* @hack spelling: event_wait called through a local. SHC then keeps
     * event_wait's address in R13 and event_camera_set_ang's in R12, loaded
     * at 0x0cb800ac and 0x0cb80094. called by name, the two addresses trade
     * registers, and 18 of 373 words differ */
    void (*wait)(int steps);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_sound_start(0, SE_TIKAL_CHIME, 90);
        ev0021_flash = event_fade_create(1.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, -20.0F, 1058.215F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        wait = event_wait;
        wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 90, 0x31c7);
        event_set_pos(event_get_player(0), -0.885F, -71.0F, 1279.62F);
        event_set_ang(event_get_player(0), 0, 0x7c60, 0);
        event_play_action(event_get_player(0), &ev0021_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        if (ev0021_flash != NULL) {
            event_fade_set_speed(ev0021_flash, -1.0F / 60.0F);
        }
        wait(90);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -0.12F, -65.32F, 1208.52F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -0.12F, -65.32F, 1250.52F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 60, 0, 0x8000, 0);
        wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -0.14F, -65.32F, 1264.515F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0, 0x8000, 0);
        wait(60);
        event_talk_start(event_get_player(0), TEXT_3);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 16);
        wait(30);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1136);
        event_message_show(TEXT_1);
        event_play_action(event_get_player(0), &ev0021_sonic_action_1,
                          &sonic_texlist, 0.25F, 0, 8);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0xc800, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0, 0x3a00, 0);
        wait(40);
        event_talk_start(event_get_player(0), TEXT_5);
        event_voice_play(1137);
        event_message_show(TEXT_2);
        wait(50);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -1.92F, -56.32F, 1310.62F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -1.92F, -44.933F,
                             1324.853F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0xfa00, 0, 0);
        wait(100);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_set_pos(event_get_player(0), -0.885F, -71.0F, 1279.62F);
        event_set_ang(event_get_player(0), 0, 0x7c60, 0);
        event_reset_player(0);
        EVENT_KILL(ev0021_flash);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
