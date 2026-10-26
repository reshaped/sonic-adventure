/* EV0095, Knuckles' story: Knuckles gathers the last shard of the Master
 * Emerald. Tikal's light draws his eye, and the screen turns white */
#include <ninja.h>

#include "ev0095/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 42
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 2
#define TEXT_5_LENGTH 3
#define TEXT_6_LENGTH 2
#include "ev0095/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

extern NJS_ACTION ev0095_knuckles_action_0;
extern NJS_ACTION ev0095_knuckles_action_1;

static struct event_path_point ev0095_light_points[111] = {
#include "ev0095/script/ev0095_light_points.inc"
};

static struct event_path ev0095_light_path = {111, ev0095_light_points};

static struct task *ev0095_light = NULL;
/* the white fade that ends the event */
static struct task *ev0095_fade = NULL;

struct texture_pvm ev0095_textures[] = {{NULL, NULL}};

void ev0095_script(int phase)
{
    /* @hack spelling: event_wait called through a local. SHC then keeps
     * event_play_action's address in R10 and event_message_show's in R11
     * (0x0cb80132, 0x0cb80174). called by name, event_play_action's moves to
     * R12, event_talk_stop's to R11, event_message_show's to R10,
     * event_voice_play's to R9 and event_talk_start's to R8, and 20 of 419
     * words differ */
    void (*wait)(int frames);
    /* @hack spelling: player 0 through a local, which keeps 0.0 in FR15 and
     * 1.0 in FR14 (0x0cb80064, 0x0cb8026c). passed directly, the two trade
     * registers, as do event_talk_start's and event_voice_play's addresses
     * in R8 and R9 and event_message_show's and event_talk_stop's in R11 and
     * R12, and 26 of 419 words differ */
    struct task *player;
    /* @hack spelling: event_talk_stop called through a local. SHC then keeps
     * its address in R12 (0x0cb8021c). called by name, it moves to R10,
     * event_play_action's to R11 and event_message_show's to R12, and 12 of 419
     * words differ */
    void (*talk_stop)(struct task *task);
    /* @hack spelling: event_message_show called through a local. SHC then also
     * keeps event_play_action's address in R10 and its own in R11. called by
     * name, it builds the same bytes as event_wait called by name, and 20 of
     * 419 words differ */
    void (*message_show)(const char *text);

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
        event_set_pos(event_get_player(0), 636.0F, 200.0F, 0.0F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        ev0095_light =
            event_light_create(event_get_player(0)->work->pos.x + 9.17F,
                               event_get_player(0)->work->pos.y + 32.0F,
                               event_get_player(0)->work->pos.z + 15.19F);
        event_set_flags(ev0095_light, EVENT_IGNORE_GROUND);
        wait = event_wait;
        wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0,
                             event_get_player(0)->work->pos.x + 16.45F,
                             event_get_player(0)->work->pos.y + 7.25F,
                             event_get_player(0)->work->pos.z - 0.75F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_play_action(event_get_player(0), &ev0095_knuckles_action_0,
                          &knuckles_texlist, 0.5F, 0, 16);
        event_play_action(event_get_player(0), &ev0095_knuckles_action_1,
                          &knuckles_texlist, 0.5F, EVENT_REPEAT, 8);
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1635);
        message_show = event_message_show;
        message_show(TEXT_1);
        wait(1);
        event_voice_wait();
        event_message_clear();
        talk_stop = event_talk_stop;
        talk_stop(event_get_player(0));
        wait(1);
        player = event_get_player(0);
        event_talk_start(player, TEXT_5);
        event_voice_play(1636);
        message_show(TEXT_2);
        wait(60);
        event_play_path(ev0095_light, &ev0095_light_path, 0.8F,
                        EVENT_FROM_HERE);
        wait(20);
        talk_stop(event_get_player(0));
        event_message_clear();
        wait(1);
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, 0, 8);
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1637);
        message_show(TEXT_3);
        event_voice_wait();
        talk_stop(event_get_player(0));
        event_message_close();
        wait(40);
        event_look_at_task(event_get_player(0), ev0095_light, 4.0F, -3.0F,
                           3.0F);
        wait(10);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100,
                             event_get_player(0)->work->pos.x + 12.2F,
                             event_get_player(0)->work->pos.y + 8.82F,
                             event_get_player(0)->work->pos.z - 4.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100,
                             event_get_player(0)->work->ang.x - 0x280,
                             0x4178 - event_get_player(0)->work->ang.y,
                             event_get_player(0)->work->ang.z);
        wait(50);
        event_sound_start(0, SE_TIKAL_CHIME, 88);
        ev0095_fade = event_fade_create(1.0F / 60.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        wait(90);
        break;
    case EVENT_END:
        EVENT_KILL(ev0095_fade);
        event_reset_player(0);
        event_clear_paths(ev0095_light);
        EVENT_KILL(ev0095_light);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
