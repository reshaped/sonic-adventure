/* EV00E0: after Chaos 6, Big wonders whether he can work what he has found,
 * then runs off */
#include <ninja.h>

#include "ev00e0/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 27
#define TEXT_3_LENGTH 16
#include "ev00e0/script/text.inc"

extern NJS_ACTION ev00e0_big_action_0;
extern NJS_ACTION ev00e0_big_action_1;
extern NJS_TEXLIST ev00e0_frog_texlist;
extern NJS_ACTION ev00e0_frog_action;

static struct task *ev00e0_frog = NULL;
static struct task *ev00e0_black = NULL;

void ev00e0_script(int phase)
{
    /* @hack redundant: a block that never runs, doubling phase. with the
     * never-run message close below, it keeps the image's registers, among
     * them event_clear_motions's address in R9 (0x0cb80158). spelled phase
     * += phase, it builds as no block does. without it, 229 of 438 words
     * differ */
    if (0) {
        phase = phase + phase;
    }
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        stream_request_music(MUSIC_THEBIG);
        event_set_pos(event_get_player(0), -21.21F, 699.25F, -1071.66F);
        event_set_ang(event_get_player(0), 0, 0x65f4, 0);
        event_wait(1);
        event_place_actor(&ev00e0_frog, event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z,
                          event_get_player(0)->work->ang.x,
                          0x4000 - event_get_player(0)->work->ang.y,
                          event_get_player(0)->work->ang.z);
        event_wait(1);
        event_set_flags(ev00e0_frog, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_set_pos(ev00e0_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 1.5F,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev00e0_frog, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00e0_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 8);
        event_clear_motions(ev00e0_frog);
        event_play_action(ev00e0_frog, &ev00e0_frog_action,
                          &ev00e0_frog_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 12.0F, 0.0F, 0);
        event_camera_orbit(EVENT_EASE_LINEAR, 160, event_get_player(0), 12.0F,
                           0, 0x9c72, 0, 0, 0x4e39, 0, 60.0F, 90.0F);
        event_wait(5);
        event_voice_play(1884);
        event_wait(1);
        event_voice_play(1885);
        event_message_wait(60, TEXT_1);
        event_message_close();
        /* @hack redundant: a message close that never runs. with the
         * never-run doubling of phase above, it keeps event_message_close's
         * address in R10 (0x0cb80268), event_clear_motions's in R9
         * (0x0cb80158) and event_play_action's in R8 (0x0cb80164). without
         * it, each moves to another of the three registers, and 12 of 438
         * words differ */
        if (0) {
            event_message_close();
        }
        event_wait(30);
        event_voice_play(1886);
        event_message_wait(60, TEXT_2);
        event_message_close();
        event_wait(30);
        event_camera_follow_clear();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -14.22F, 713.75F, -1081.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x6700, 0);
        event_wait(10);
        event_voice_play(1887);
        event_message_wait(40, TEXT_3);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 27.99F, 791.21F, -1167.11F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xde75, 0x7000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 50.62F, 855.01F,
                             -1221.75F);
        event_wait(60);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00e0_big_action_0,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -15.0F, 699.23F, -1080.81F,
                           0.5F, 0.06F);
        event_follow(ev00e0_frog, 0.0F, 0.0F, 0.0F, 30, event_get_player(0));
        ev00e0_black = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_move_stop_all();
        EVENT_KILL(ev00e0_black);
        event_reset_player(0);
        event_kill_task(&ev00e0_frog);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00e0_textures[] = {
    {"FROG", &ev00e0_frog_texlist},
    {NULL, NULL},
};
