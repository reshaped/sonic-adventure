/* EV009B, Knuckles' story: Knuckles beats Chaos 6, and six Chaos Emeralds float
 * near him. he says it will all be over once he takes the Master Emerald's
 * pieces back to the altar */
#include <ninja.h>

#include "ev009b/green_emerald_action.h"
#include "ev009b/green_emerald_textures.h"
#include "ev009b/script.h"
#include "ev009b/white_emerald_action.h"
#include "ev009b/white_emerald_textures.h"
#include "ev009b/yellow_emerald_textures.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 67
#define TEXT_3_LENGTH 6
#include "ev009b/script/text.inc"

extern NJS_ACTION knuckles_stand_action;

extern NJS_TEXLIST ev009b_blue_emerald_texlist;
extern NJS_ACTION ev009b_blue_emerald_action;
extern NJS_TEXLIST ev009b_purple_emerald_texlist;
extern NJS_ACTION ev009b_purple_emerald_action;
extern NJS_TEXLIST ev009b_sky_emerald_texlist;

static struct task *ev009b_white_emerald = NULL;
static struct task *ev009b_blue_emerald = NULL;
static struct task *ev009b_green_emerald = NULL;
static struct task *ev009b_purple_emerald = NULL;
static struct task *ev009b_sky_emerald = NULL;
static struct task *ev009b_yellow_emerald = NULL;
static struct task *ev009b_fade = NULL;

void ev009b_script(int phase)
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
        event_set_pos(event_get_player(0), 0.0F, 745.38F, -224.35F);
        event_set_ang(event_get_player(0), 0x0317, 0x7f0d, 0x0119);
        event_set_pos(event_get_player(0), 0.0F, 743.94F, -224.59F);
        event_set_ang(event_get_player(0), 0x01c9, 0x7f0d, 0);
        event_place_actor(&ev009b_white_emerald, 32.16F, 773.75F, -231.9F, 0, 0,
                          0);
        event_place_actor(&ev009b_blue_emerald, 32.16F, 773.75F, -231.9F, 0, 0,
                          0);
        event_place_actor(&ev009b_green_emerald, 32.16F, 773.75F, -231.9F, 0, 0,
                          0);
        event_place_actor(&ev009b_purple_emerald, 32.16F, 773.75F, -231.9F, 0,
                          0, 0);
        event_place_actor(&ev009b_sky_emerald, 32.16F, 773.75F, -231.9F, 0, 0,
                          0);
        event_place_actor(&ev009b_yellow_emerald, 32.16F, 773.75F, -231.9F, 0,
                          0, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 8);
        event_set_flags(ev009b_white_emerald, EVENT_IGNORE_GROUND);
        event_set_flags(ev009b_blue_emerald, EVENT_IGNORE_GROUND);
        event_set_flags(ev009b_green_emerald, EVENT_IGNORE_GROUND);
        event_set_flags(ev009b_purple_emerald, EVENT_IGNORE_GROUND);
        event_set_flags(ev009b_sky_emerald, EVENT_IGNORE_GROUND);
        event_set_flags(ev009b_yellow_emerald, EVENT_IGNORE_GROUND);
        event_set_pos(ev009b_white_emerald, event_get_player(0)->work->pos.x,
                      event_get_player(0)->work->pos.y + 5.0F,
                      event_get_player(0)->work->pos.z - 45.0F);
        event_set_ang(ev009b_white_emerald, 0, 0, 0);
        event_set_pos(ev009b_blue_emerald,
                      event_get_player(0)->work->pos.x - 5.0F,
                      event_get_player(0)->work->pos.y + 5.0F,
                      event_get_player(0)->work->pos.z - 48.0F);
        event_set_ang(ev009b_blue_emerald, 0, 0, 0);
        event_set_pos(ev009b_green_emerald,
                      event_get_player(0)->work->pos.x + 7.0F,
                      event_get_player(0)->work->pos.y + 5.0F,
                      event_get_player(0)->work->pos.z - 42.0F);
        event_set_ang(ev009b_green_emerald, 0, 0, 0);
        event_set_pos(ev009b_purple_emerald,
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y + 5.0F,
                      event_get_player(0)->work->pos.z - 44.0F);
        event_set_ang(ev009b_purple_emerald, 0, 0, 0);
        event_set_pos(ev009b_sky_emerald,
                      event_get_player(0)->work->pos.x - 8.0F,
                      event_get_player(0)->work->pos.y + 5.0F,
                      event_get_player(0)->work->pos.z - 52.0F);
        event_set_ang(ev009b_sky_emerald, 0, 0, 0);
        event_set_pos(ev009b_yellow_emerald,
                      event_get_player(0)->work->pos.x + 6.0F,
                      event_get_player(0)->work->pos.y + 5.0F,
                      event_get_player(0)->work->pos.z - 50.0F);
        event_set_ang(ev009b_yellow_emerald, 0, 0, 0);
        event_wait(1);
        event_play_action(ev009b_white_emerald, &ev009b_white_emerald_action,
                          &ev009b_white_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev009b_blue_emerald, &ev009b_blue_emerald_action,
                          &ev009b_blue_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev009b_green_emerald, ev009b_green_emerald_action,
                          &ev009b_green_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev009b_purple_emerald, &ev009b_purple_emerald_action,
                          &ev009b_purple_emerald_texlist, 1.0F, EVENT_REPEAT,
                          0);
        /* the sky and yellow emeralds play the green and purple emeralds'
         * actions with their own texlists. no word in EV009B or 1ST_READ.BIN
         * points at their own actions, in the data after their texlists */
        event_play_action(ev009b_sky_emerald, ev009b_green_emerald_action,
                          &ev009b_sky_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev009b_yellow_emerald, &ev009b_purple_emerald_action,
                          &ev009b_yellow_emerald_texlist, 1.0F, EVENT_REPEAT,
                          0);
        stream_request_music(MUSIC_EGCARER1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.02F, 764.51F, -289.14F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xed1c, 0x980e, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -21.49F, 769.43F,
                             -297.32F);
        event_wait(90);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -1.92F, 745.84F, -234.45F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x141c, 0x950e, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -6.27F, 745.84F, -231.98F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1660);
        event_message_wait(30, TEXT_1);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(45);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1661);
        event_message_wait(140, TEXT_2);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 7.66F, 764.75F, -265.25F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf21c, 0x720e, 0);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 200, 0x31c7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, 62.11F, 839.58F, -501.51F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 300, 0xf71c, 0x760e, 0);
        event_wait(140);
        /* the black fade, from 0.2 to full in 48 steps */
        ev009b_fade = event_fade_create(1.0F / 60.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(90);
        break;
    case EVENT_END:
        event_clear_task(event_get_player(0));
        EVENT_KILL(ev009b_fade);
        event_kill_task(&ev009b_white_emerald);
        event_kill_task(&ev009b_blue_emerald);
        event_kill_task(&ev009b_green_emerald);
        event_kill_task(&ev009b_purple_emerald);
        event_kill_task(&ev009b_sky_emerald);
        event_kill_task(&ev009b_yellow_emerald);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev009b_textures[] = {
    {"M_EM_WHITE", &ev009b_white_emerald_texlist},
    {"M_EM_BLUE", &ev009b_blue_emerald_texlist},
    {"M_EM_GREEN", &ev009b_green_emerald_texlist},
    {"M_EM_PURPLE", &ev009b_purple_emerald_texlist},
    {"M_EM_SKY", &ev009b_sky_emerald_texlist},
    {"M_EM_YELLOW", &ev009b_yellow_emerald_texlist},
    {NULL, NULL},
};
