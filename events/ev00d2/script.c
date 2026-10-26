/* EV00D2, Big's story: Big spots Froggy in Station Square, and Froggy runs
 * under a car */
#include <ninja.h>

#include "ev00d2/big_action.h"
#include "ev00d2/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_models.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 29
#include "ev00d2/script/text.inc"

extern NJS_ACTION ev00d2_frog_action_0;
extern NJS_ACTION ev00d2_frog_action_1;

static struct task *ev00d2_frog = NULL;

void ev00d2_script(int phase)
{
    /* @hack spelling: event_camera_set_ang called through a local. SHC then
     * puts its address in R12 and event_camera_set_pos's in R11 (0x0cb800dc,
     * 0x0cb800c2). called by name, the two trade, and 18 of 515 words differ */
    void (*set_ang)(enum event_ease ease, int steps, Angle ang_x, Angle ang_y,
                    Angle ang_z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_0042.mlt");
        event_sound_init(1);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        stream_request_music(MUSIC_S_SQUARE);
        event_set_pos(event_get_player(0), 484.97F, 0.01F, 884.87F);
        event_set_ang(event_get_player(0), 0, 0xd745, 0);
        event_wait(1);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 32);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 463.97F, 15.14F, 886.1F);
        set_ang = event_camera_set_ang;
        set_ang(EVENT_EASE_LINEAR, 0, 0xff19, 0xc0c4, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 468.96F, 15.03F, 885.89F);
        set_ang(EVENT_EASE_LINEAR, 10, 0xff19, 0xc0c4, 0);
        event_wait(15);
        event_place_actor(&ev00d2_frog, 431.62F, 1.51F, 926.68F, 0, 0x6585, 0);
        event_wait(1);
        event_set_flags(ev00d2_frog, EVENT_IGNORE_GROUND);
        event_play_action(ev00d2_frog, &ev00d2_frog_action_0,
                          &ev00d2_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 501.46F, 14.35F, 854.72F);
        set_ang(EVENT_EASE_LINEAR, 0, 0xfc19, 0x60c4, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 449.68F, 7.18F, 908.5F);
        set_ang(EVENT_EASE_LINEAR, 30, 0xfc19, 0x60c4, 0);
        event_wait(40);
        event_play_action(ev00d2_frog, &ev00d2_frog_action_1,
                          &ev00d2_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(50);
        event_sound_play(1224, 0, 0, 0);
        event_move(ev00d2_frog, 431.62F, 1.51F, 926.68F, 344.7F, 1.51F, 994.21F,
                   300);
        event_wait(30);
        event_sound_play(1224, 0, 0, 0);
        event_wait(30);
        event_sound_play(1224, 0, 0, 0);
        event_wait(30);
        event_sound_play(1224, 0, 0, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 469.22F, 15.12F, 882.45F);
        set_ang(EVENT_EASE_LINEAR, 0, 0xfc19, 0xb6c4, 0);
        event_voice_play(1819);
        event_message_wait(20, TEXT_1);
        event_message_close();
        event_move_stop(ev00d2_frog);
        event_kill_task(&ev00d2_frog);
        event_run_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &big_run_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 20);
        event_wait(2);
        event_run_to_speed(event_get_player(0), 369.59F, 0.01F, 1003.5F, 0.5F,
                           0.06F);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 469.22F, 15.12F, 882.45F);
        set_ang(EVENT_EASE_LINEAR, 80, 0xfc19, 0x5dc4, 0);
        event_wait(160);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(13);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 409.08F, 39.91F, 960.88F);
        set_ang(EVENT_EASE_LINEAR, 0, 0xe952, 0x5d33, 0);
        event_set_pos(event_get_player(0), 369.65F, 0.01F, 1003.42F);
        event_set_ang(event_get_player(0), 0, 0x7146, 0);
        event_wait(1);
        event_voice_play(1820);
        event_message_wait(0, TEXT_2);
        event_play_action(event_get_player(0), &ev00d2_big_action, &big_texlist,
                          0.5F, EVENT_REPEAT, 30);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 409.26F, 58.67F, 915.86F);
        set_ang(EVENT_EASE_LINEAR, 100, 0xe3d3, 0x621f, 0);
        event_wait(30);
        event_voice_play(1821);
        event_message_wait(0, TEXT_3);
        event_wait(10);
        event_play_action(event_get_player(0), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 32);
        event_wait(130);
        event_message_close();
        event_wait(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_task(&ev00d2_frog);
        event_set_pos(event_get_player(0), 369.65F, 0.01F, 1003.42F);
        event_set_ang(event_get_player(0), 0, 0x838e, 0);
        event_wait(1);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00d2_textures[] = {
    {"FROG", &ev00d2_frog_texlist},
    {NULL, NULL},
};
