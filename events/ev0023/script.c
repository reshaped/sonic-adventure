/* EV0023, Sonic's story: Sonic wakes in the jungle and wonders whether the past
 * was a dream. Eggman flies over, and Sonic swears he won't get away this
 * time */
#include <ninja.h>

#include "ev0023/script.h"
#include "ev0023/sonic_action_4.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 25
#define TEXT_2_LENGTH 19
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 34
#define TEXT_6_LENGTH 22
#define TEXT_7_LENGTH 20
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 3
#define TEXT_10_LENGTH 3
#define TEXT_11_LENGTH 1
#define TEXT_12_LENGTH 3
#include "ev0023/script/text.inc"

extern NJS_ACTION sonic_trot_action;

extern NJS_TEXLIST event_eggmobile_texlist;
extern NJS_ACTION ev0023_sonic_action_0;
extern NJS_ACTION ev0023_sonic_action_1;
extern NJS_ACTION ev0023_sonic_action_2;
extern NJS_ACTION ev0023_sonic_action_3;

static struct task *ev0023_fade = NULL;

struct texture_pvm ev0023_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {NULL, NULL},
};

void ev0023_script(int phase)
{
    /* @hack spelling: event_talk_start called through a local. SHC then keeps
     * its address in R9 and event_play_action's in R8, loaded at 0x0cb8010e
     * and 0x0cb800ba. called by name, the two addresses trade registers, and
     * 16 of 822 words differ */
    void (*talk_start)(struct task *task, const char *text);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0007.mlt");
        event_sound_init(3);
        ev0023_fade = event_fade_create(1.0F, 1.0F, 1.0F, 1.0F, 1.0F);
        event_sound_start(2, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(2, -120, 120);
        event_set_pos(event_get_player(0), -518.4F, -0.0F, -867.63F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_play_action(event_get_player(0), &ev0023_sonic_action_3,
                          &sonic_texlist, 0.35F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -519.3F, 1.46F, -863.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0xff00, 0);
        event_eggmobile_create(-574.455F, 114.0F, -620.8F, 0, 0xc800, 0);
        talk_start = event_talk_start;
        talk_start(event_get_player(0), TEXT_7);
        event_wait(30);
        if (ev0023_fade != NULL) {
            event_fade_set_speed(ev0023_fade, -1.0F / 60.0F);
        }
        event_wait(35);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        talk_start(event_get_player(0), TEXT_8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -522.135F, 0.797F,
                             -855.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x0900, 0xfb00, 0xfe00);
        event_wait(110);
        event_sound_stop(2);
        event_voice_play(1138);
        talk_start(event_get_player(0), TEXT_9);
        event_message_show(TEXT_1);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 185, -512.54F, 1.565F,
                             -857.646F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 185, 0x0900, 0x1700, 0x0300);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        event_play_action(event_get_player(0), &ev0023_sonic_action_4,
                          &sonic_texlist, 0.4F, EVENT_REPEAT, 24);
        event_wait(5);
        event_voice_play(1139);
        talk_start(event_get_player(0), TEXT_10);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_eggmobile_fly_to(-721.476F, 117.0F, -1141.117F, 190);
        event_wait(10);
        event_sound_start(0, 1225, 1800);
        event_sound_play(1226, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -586.75F, 140.0F,
                             -672.0244F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xdf00, 0x0200, 0xf800);
        event_wait(20);
        stream_request_music(MUSIC_EVTBGM01);
        event_play_action(event_get_player(0), &ev0023_sonic_action_2,
                          &sonic_texlist, 0.6F, EVENT_REPEAT, 0);
        event_wait(15);
        event_set_ang(event_get_player(0), 0x0100, 0xa400, 0);
        event_voice_play(933);
        event_message_show(TEXT_3);
        event_wait(5);
        event_sound_fade_volume(0, -30, 80);
        event_sound_fade_pan(0, -60, 80);
        event_set_pos(event_get_player(0), -482.4F, 0.0F, -808.4F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -472.03F, 3.0F, -810.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0900, 0x4300, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -473.0F, 3.0F, -805.5647F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0900, 0x2900, 0xfe00);
        event_message_close();
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 1.9F, EVENT_REPEAT, 0);
        event_wait(10);
        event_run_to_speed(event_get_player(0), -606.4F, 9.0F, -837.5F, 2.0F,
                           1.0F);
        event_wait(40);
        event_sound_fade_volume(0, 0, 50);
        event_sound_fade_pan(0, 0, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -712.37F, 129.826F,
                             -1136.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf600, 0x7500, 0x0200);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xf600, 0x7000, 0x0200);
        event_look_at(event_get_player(0), -721.476F, 157.0F, -1141.117F);
        event_wait(48);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -620.832F, 25.8856F,
                             -865.546F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe800, 0x9400, 0x0200);
        event_camera_follow(event_get_player(0));
        event_wait(1);
        event_run_to_speed(event_get_player(0), -721.268F, 15.843F, -878.164F,
                           1.3F, 2.0F);
        event_wait(20);
        event_voice_play(1140);
        talk_start(event_get_player(0), TEXT_11);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -700.478F, 20.26F, -856.56F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0900, 0xef00, 0);
        event_eggmobile_fly_turn(-643.5F, 127.0F, -1375.9F, 0, -0x2000, 0, 110,
                                 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -727.76F, 18.3324F,
                             -869.233F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0d00, 0xf500, 0x0300);
        event_run_wait(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -731.9F, 17.5878F, -876.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0900, 0xf500, 0x0300);
        event_run_to_speed(event_get_player(0), -724.0F, 14.443F, -885.378F,
                           1.3F, 2.5F);
        event_run_wait(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &ev0023_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -735.2F, 16.0F, -887.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0900, 0xb900, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -732.385F, 16.0F,
                             -891.94F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0900, 0xa900, 0);
        event_wait(10);
        event_voice_play(1141);
        talk_start(event_get_player(0), TEXT_12);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_play_action(event_get_player(0), &ev0023_sonic_action_0,
                          &sonic_texlist, 1.0F, 0, 4);
        event_play_action(event_get_player(0), &ev0023_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_voice_play(1142);
        talk_start(event_get_player(0), TEXT_11);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_set_pos(event_get_player(0), -724.0F, 14.44F, -885.54F);
        event_set_ang(event_get_player(0), 0xfb52, 0x8cc3, 0xfcc5);
        event_reset_player(0);
        if (ev0023_fade != NULL) {
            task_kill(ev0023_fade);
            ev0023_fade = NULL;
        }
        event_eggmobile_kill();
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
