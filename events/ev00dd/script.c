/* EV00DD: Big comes back from the past holding Froggy, and the Egg Carrier
 * starts to shake */
#include <ninja.h>

#include "ev00dd/big_action.h"
#include "ev00dd/frog_action.h"
#include "ev00dd/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 32
#define TEXT_2_LENGTH 33
#define TEXT_3_LENGTH 69
#define TEXT_4_LENGTH 19
#include "ev00dd/script/text.inc"

static struct task *ev00dd_frog = NULL;
static struct task *ev00dd_white = NULL;

void ev00dd_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_sound_init(1);
        sound_load_file("e_001B.mlt");
        ev00dd_white = event_fade_create(-0.0012F, 1.0F, 1.0F, 1.0F, 1.0F);
        event_sound_start(0, SE_TIKAL_CHIME, 1800);
        event_sound_fade_volume(0, -120, 120);
        event_set_pos(event_get_player(0), 0.0F, 0.0F, 197.5F);
        event_set_ang(event_get_player(0), 0, 0x7f79, 0x0119);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00dd_big_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_place_actor(&ev00dd_frog, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z,
                          event_get_player(0)->work->ang.x,
                          0x4000 - event_get_player(0)->work->ang.y,
                          event_get_player(0)->work->ang.z);
        event_wait(1);
        event_set_flags(ev00dd_frog, EVENT_IGNORE_GROUND);
        event_set_pos(ev00dd_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 1.5F,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev00dd_frog, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_play_action(ev00dd_frog, &ev00dd_frog_action,
                          &ev00dd_frog_texlist, 1.0F, EVENT_REPEAT, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.2F, 11.12F, 180.49F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0584, 0x908f, 0);
        event_wait(30);
        if (ev00dd_white != NULL) {
            event_fade_set_speed(ev00dd_white, -0.016F);
        }
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -15.38F, 7.61F, 156.82F);
        event_wait(120);
        event_sound_stop(0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -14.18F, 12.96F, 210.49F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xffcb, 0xe8ce, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -47.07F, 12.98F, 191.75F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 180, 0xffcb, 0xeece, 0);
        event_voice_play(1863);
        event_wait(10);
        event_message_wait(140, TEXT_1);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 27.69F, 6.71F, 128.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x04cb, 0x6993, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 18.17F, 9.05F, 145.72F);
        sound_play_at(1223, 0, 0, 120, 0.0F, 0.0F, 197.5F);
        vibration_start(0, 7, 50, 3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x12cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x12cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x12cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x12cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 5, 0x12cb, 0x6993, 0);
        event_wait(3);
        sound_play_at(1223, 0, 0, 80, 0.0F, 0.0F, 197.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x09cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x09cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        sound_play_at(1223, 0, 0, 30, 0.0F, 0.0F, 197.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        sound_play_at(1223, 0, 0, 60, 0.0F, 0.0F, 197.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_voice_play(1864);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x05cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x04cb, 0x6993, 0);
        sound_play_at(1223, 0, 0, 20, 0.0F, 0.0F, 197.5F);
        event_message_wait(60, TEXT_2);
        event_message_close();
        sound_play_at(1223, 0, 0, 10, 0.0F, 0.0F, 197.5F);
        event_voice_play(1865);
        event_message_wait(200, TEXT_3);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -19.45F, 21.63F, 222.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf5cb, 0xed93, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -26.43F, 25.34F, 235.4F);
        event_voice_play(1866);
        event_wait(10);
        event_message_wait(60, TEXT_4);
        event_message_close();
        event_wait(30);
        break;
    case EVENT_END:
        event_sound_stop_all();
        EVENT_KILL(ev00dd_white);
        event_reset_player(0);
        event_kill_task(&ev00dd_frog);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00dd_textures[] = {
    {"FROG", &ev00dd_frog_texlist},
    {NULL, NULL},
};
