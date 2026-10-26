/* EV00DF: Big has Froggy back from Chaos 6, and Sonic tells him to escape */
#include <ninja.h>

#include "ev00df/script.h"
#include "ev00df/sonic_action_1.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/chaos6.h"
#include "shared/fade.h"
#include "shared/frog.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 41
#define TEXT_3_LENGTH 44
#define TEXT_4_LENGTH 25
#define TEXT_5_LENGTH 40
#define TEXT_6_LENGTH 3
#include "ev00df/script/text.inc"

/* @hack type: Big's actions declared as arrays. SHC then keeps event_wait's
 * address in R10 and event_play_action's in R12 (0x0cb8004e, 0x0cb80128), and
 * moves each 8 a call passes into a scratch register, as at 0x0cb80126. as
 * single objects, SHC keeps 8 in R12, ev00df_script runs 4 bytes short, and 322
 * of 573 words differ */
extern NJS_ACTION ev00df_big_action_0[];
extern NJS_ACTION ev00df_big_action_1[];
extern NJS_ACTION ev00df_big_action_2[];
extern NJS_ACTION ev00df_big_action_3[];
extern NJS_ACTION ev00df_sonic_action_0;
/* @hack type: Froggy's actions declared as arrays. SHC then pools each action's
 * address, as at 0x0cb803b4, and reads the motion 4 bytes past it (MOV.L
 * @(4,R6),R6 at 0x0cb802de). as single objects, the pool words hold the
 * addresses of the actions' motion fields, and 44 of 573 words differ */
extern NJS_ACTION ev00df_frog_action_0[];
extern NJS_ACTION ev00df_frog_action_1[];
extern NJS_ACTION ev00df_frog_action_2[];
extern NJS_ACTION ev00df_frog_action_3[];
extern NJS_TEXLIST ev00df_frog_texlist; /* for the first action Froggy plays */

/* Froggy's model, whose nodes 1, 12 and 13 are event_frog_node1_made,
 * event_frog_node12_made and event_frog_node13_made. the script plays every
 * motion but his first on it */
extern NJS_OBJECT event_frog_made_object;

static struct task *ev00df_frog = NULL;
static struct task *ev00df_black = NULL;

void ev00df_script(int phase)
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
        stream_request_music(MUSIC_THEBIG);
        event_set_pos(event_get_player(0), -6.91F, 746.04F, -539.74F);
        event_set_ang(event_get_player(0), 0xfcd3, 0x8693, 0);
        event_place_task(
            &ev00df_frog, event_frog_create, event_get_player(0)->work->pos.x,
            event_get_player(0)->work->pos.y, event_get_player(0)->work->pos.z,
            event_get_player(0)->work->ang.x,
            0x4000 - event_get_player(0)->work->ang.y,
            event_get_player(0)->work->ang.z);
        event_create_player(2, sonic_step, 97.87F, 746.98F, -488.81F, 0xfd77,
                            0xe5a1, 0xfe16);
        event_chaos6_hold(5.4F, 750.0F, -400.0F, 0, 0x8000, 0, 0x038e);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00df_frog);
        event_play_action(event_get_player(0), ev00df_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(ev00df_frog, ev00df_frog_action_1,
                          &ev00df_frog_texlist, 1.0F, EVENT_REPEAT, 8);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00df_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_set_flags(ev00df_frog, EVENT_IGNORE_GROUND);
        event_frog_set_shape(2);
        event_wait(1);
        event_set_pos(ev00df_frog, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y - 0.5F,
                      event_get_player(0)->work->pos.z);
        event_set_ang(ev00df_frog, event_get_player(0)->work->ang.x,
                      0x4000 - event_get_player(0)->work->ang.y,
                      event_get_player(0)->work->ang.z);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -14.04F, 759.05F, -576.98F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfd00, 0x8800, 0);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00df_frog);
        event_play_action(event_get_player(0), ev00df_big_action_2,
                          &big_texlist, 1.0F, 0, 8);
        event_play_motion(ev00df_frog, &event_frog_made_object,
                          ev00df_frog_action_2->motion, &event_frog_texlist,
                          1.0F, 0, 8);
        event_play_action(event_get_player(0), ev00df_big_action_3,
                          &big_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_motion(ev00df_frog, &event_frog_made_object,
                          ev00df_frog_action_3->motion, &event_frog_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(1);
        event_voice_play(1877);
        event_message_wait(60, TEXT_1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 150, 0x238e);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00df_frog);
        event_play_action(event_get_player(0), ev00df_big_action_0,
                          &big_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_motion(ev00df_frog, &event_frog_made_object,
                          ev00df_frog_action_0->motion, &event_frog_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_voice_play(1879);
        event_message_wait(60, TEXT_2);
        event_message_close();
        event_play_action(event_get_player(2), &ev00df_sonic_action_0,
                          &sonic_texlist, 0.4F, 0, 4);
        event_play_action(event_get_player(2), &ev00df_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 114.27F, 753.5F, -461.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x1e00, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_6);
        event_voice_play(1881);
        event_message_wait(60, TEXT_3);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -40.44F, 768.45F, -576.32F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf800, 0xa300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 240, -63.46F, 774.5F, -596.18F);
        event_voice_play(1882);
        event_message_wait(60, TEXT_4);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_clear_motions(ev00df_frog);
        event_play_action(event_get_player(0), ev00df_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_motion(ev00df_frog, &event_frog_made_object,
                          ev00df_frog_action_1->motion, &event_frog_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_voice_play(1883);
        event_message_wait(60, TEXT_5);
        event_wait(30);
        event_message_close();
        ev00df_black = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        EVENT_KILL(ev00df_black);
        event_chaos6_release();
        event_reset_player(0);
        event_kill_task(&ev00df_frog);
        event_kill_player(2);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00df_textures[] = {
    {"SHAPE_FROG", &event_frog_texlist},
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
