/* EV0031, Tails' story: Sonic comes upon Tails where smoke rises */
#include <ninja.h>

#include "ev0031/prop.h"
#include "ev0031/script.h"
#include "ev0031/sonic_action_1.h"
#include "event/event.h"
#include "event/play.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "shared/dust.h"
#include "shared/fade.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 17
#define TEXT_2_LENGTH 2
#define TEXT_3_LENGTH 1
#include "ev0031/script/text.inc"

extern NJS_ACTION ev0031_tails_action_0;
extern NJS_ACTION ev0031_tails_action_1;
extern NJS_ACTION ev0031_tails_action_2;
extern NJS_ACTION ev0031_sonic_action_0;

static struct task *ev0031_fade = NULL;
static struct task *ev0031_smoke = NULL;
static struct task *ev0031_prop = NULL;

void ev0031_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_place_task(&ev0031_prop, ev0031_prop_create, 6893.95F, 1.03F,
                         -2203.15F, 0, 0xc000, 0);
        event_create_player(2, sonic_step, 6420.0F, 1.33F, -2150.0F, 0, 0x4000,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(0), 6888.95F, 1.03F, -2203.15F);
        event_set_ang(event_get_player(0), 0, 0xc000, 0);
        event_play_action(event_get_player(0), &ev0031_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_1);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F,
                           -3.0F, 0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6852.08F, 9.0F, -2203.845F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1b1f, 0xbf3c, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0xfd1f, 0xbf3c, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 6865.03F, 8.08F,
                             -2203.595F);
        /* @hack spelling: the test and the creation as one && expression. SHC
         * then keeps 1.0F in FR12 (FLDI1 FR12 at 0x0cb800c6). as an if, SHC
         * loads 1.0F at each use and saves no FR12, ev0031_script runs 4 bytes
         * short, and 356 of its 521 words differ */
        ev0031_smoke == NULL && (ev0031_smoke = event_dust_create());
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then keeps 1.0F in FR12 (FLDI1 FR12 at 0x0cb800c6). as an if, SHC
         * loads 1.0F at each use and saves no FR12, ev0031_script runs 4 bytes
         * short, and 356 of its 521 words differ */
        ev0031_smoke != NULL && (DUST_WORK(ev0031_smoke)->timer =
                                     DUST_WORK(ev0031_smoke)->interval = 10);
        /* @hack spelling: the test and the store as one && expression. SHC then
         * keeps 1.0F in FR12 (FLDI1 FR12 at 0x0cb800c6). as an if, SHC loads
         * 1.0F at each use and saves no FR12, ev0031_script runs 4 bytes short,
         * and 356 of its 521 words differ */
        ev0031_smoke != NULL && (DUST_WORK(ev0031_smoke)->params->scale = 1.2F);
        /* @hack spelling: the test and the color stores as one && expression.
         * SHC then keeps -2203.15F, the z of the prop and Tails, in FR13
         * (FMOV.S @R0,FR13 at 0x0cb8005e) and event_set_pos's address in R8
         * (0x0cb8009e). as an if, they go in FR12 and R9, and 252 of
         * ev0031_script's 521 words differ */
        ev0031_smoke != NULL &&
            (DUST_WORK(ev0031_smoke)->params->color.a = 1.0F,
             DUST_WORK(ev0031_smoke)->params->color.r = 0.2F,
             DUST_WORK(ev0031_smoke)->params->color.g = 0.2F,
             DUST_WORK(ev0031_smoke)->params->color.b = 0.2F);
        event_wait(1);
        event_set_pos(ev0031_smoke, 6888.95F, 1.03F, -2203.15F);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6882.188F, 5.55F,
                             -2199.1343F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf71f, 0xe03c, 0);
        /* @hack spelling: the test and the stores as one && expression. as an
         * if, -2203.15F and 1.0F trade FR13 and FR12 (0x0cb8005e, 0x0cb800c6),
         * and 26 of ev0031_script's 521 words differ */
        ev0031_smoke != NULL && (DUST_WORK(ev0031_smoke)->timer =
                                     DUST_WORK(ev0031_smoke)->interval = 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_2);
        event_play_action(event_get_player(0), &ev0031_tails_action_1,
                          &tails_texlist, 0.2F, 0, 8);
        event_play_action(event_get_player(0), &ev0031_tails_action_2,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        stream_request_music(MUSIC_THESONIC);
        event_set_pos(event_get_player(2), 6880.56F, 1.03F, -2203.39F);
        event_set_ang(event_get_player(2), 0, 0x4000, 0);
        event_play_action(event_get_player(2), &ev0031_sonic_action_0,
                          &sonic_texlist, 0.5F, 0, 8);
        event_play_action(event_get_player(2), &ev0031_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6886.737F, 3.11F, -2203.44F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf21f, 0x423c, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x091f, 0x423c, 0);
        event_wait(52);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6887.572F, 3.335F, -2197.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff1f, 0x033c, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 6887.072F, 3.335F,
                             -2197.0F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6886.737F, 3.11F, -2203.44F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x091f, 0x423c, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1e1f, 0x423c, 0);
        event_wait(80);
        event_talk_start(event_get_player(2), TEXT_3);
        event_wait(30);
        EVENT_KILL(ev0031_smoke);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 6894.12F, 17.92F, -2065.67F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 3.0F, 0.0F, 0);
        ev0031_fade = event_fade_create(1.0F / 60.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(80);
        event_camera_look_clear();
        event_wait(10);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev0031_prop);
        EVENT_KILL(ev0031_smoke);
        EVENT_KILL(ev0031_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0031_textures[] = {
    {"SONIC", &sonic_texlist},
    {NULL, NULL},
};
