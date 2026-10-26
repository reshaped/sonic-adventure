/* EV00D9, Big's story: Big spots Froggy in Hot Shelter, and means to save him.
 * Big is player 0 */
#include "ev00d9/script.h"
#include "ev00d9/big_action.h"
#include "ev00d9/frog_action.h"
#include "ev00e3/frog.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 24
#include "ev00d9/script/text.inc"

static struct task *ev00d9_frog = NULL;

void ev00d9_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        event_set_pos(event_get_player(0), 704.3F, 2.0F, -649.95F);
        event_set_ang(event_get_player(0), 0, 0x1d6e, 0);
        event_place_task(&ev00d9_frog, ev00e3_frog_create, 610.556F, 21.02F,
                         -852.36F, 0xfa19, 0x71c4, 0);
        event_wait(1);
        event_set_pos(ev00d9_frog, 568.39F, 26.23F, -820.04F);
        event_set_ang(ev00d9_frog, 0, 0xe58f, 0);
        event_wait(1);
        event_set_flags(ev00d9_frog, EVENT_IGNORE_GROUND);
        event_play_action(ev00d9_frog, &ev00d9_frog_action,
                          &ev00d9_frog_texlist, 1.0F, EVENT_REPEAT, 0);
        event_move(ev00d9_frog, 568.39F, 26.23F, -820.04F, 556.44F, 36.61F,
                   -810.39F, 150);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 555.99F, 41.21F, -831.28F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf700, 0xa5c4, 0);
        event_wait(30);
        event_sound_play(718, 0, 0, 0);
        event_wait(30);
        event_sound_play(718, 0, 0, 0);
        event_wait(30);
        event_sound_play(718, 0, 0, 0);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00d9_big_action, &big_texlist,
                          0.8F, 0, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 696.66F, 9.52F, -667.69F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0f19, 0x89c4, 0);
        event_move_stop_all();
        event_set_pos(ev00d9_frog, 568.39F, 26.23F, -820.04F);
        event_set_ang(ev00d9_frog, 0, 0xe58f, 0);
        event_wait(1);
        event_wait_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 632.45F, 25.83F, -755.41F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0x2153, 0);
        event_move(ev00d9_frog, 568.39F, 26.23F, -820.04F, 556.44F, 36.61F,
                   -810.39F, 150);
        stream_request_music(MUSIC_SHELTER1);
        event_voice_play(1844);
        event_message_wait(1, TEXT_1);
        event_set_pos(event_get_player(0), 717.07F, 2.0F, -647.96F);
        event_set_ang(event_get_player(0), 0, 0x9b2e, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, 753.29F, 17.29F, -630.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x0200, 0x1f53, 0);
        event_wait(70);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 704.91F, 15.06F, -664.07F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0019, 0x98c4, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 707.19F, 15.07F, -660.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 150, 0x0019, 0x98c4, 0);
        event_voice_play(1845);
        event_message_wait(0, TEXT_2);
        event_move_stop_all();
        event_kill_task(&ev00d9_frog);
        event_wait(70);
        event_message_close();
        break;
    case EVENT_END:
        event_move_stop_all();
        event_kill_task(&ev00d9_frog);
        event_set_pos(event_get_player(0), 717.07F, 2.0F, -647.96F);
        event_set_ang(event_get_player(0), 0, 0x9b2e, 0);
        event_wait(1);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00d9_textures[] = {
    {"FROG", &ev00d9_frog_texlist},
    {NULL, NULL},
};
