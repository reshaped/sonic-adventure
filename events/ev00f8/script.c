/* EV00F8: in the past, Tikal speaks of the seven emeralds, and a green light
 * swells at her */
#include <ninja.h>

#include "ev00f8/script.h"
#include "ev00f8/sprite.h"
#include "ev00f8/tikal_action_4.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/rays.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 32
#define TEXT_2_LENGTH 40
#define TEXT_3_LENGTH 42
#define TEXT_4_LENGTH 47
#define TEXT_5_LENGTH 47
#define TEXT_6_LENGTH 28
#define TEXT_7_LENGTH 4
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 4
#include "ev00f8/script/text.inc"

/* ev00f8_sprite slots for a green light at Tikal that swells, and a faint
 * flare that spins above it */
#define SWELL 0
#define FLARE 1

extern NJS_ACTION ev00f8_sonic_action;
extern NJS_ACTION ev00f8_tikal_action_0;
extern NJS_ACTION ev00f8_tikal_action_1;
extern NJS_ACTION ev00f8_tikal_action_2;
extern NJS_ACTION ev00f8_tikal_action_3;

/* @hack spelling: ev00f8_sprite_lens_texlist is
 * ev00f8_sprite_texlists[EVENT_SPRITE_LENS], declared under a name of its own
 * at the element's address. with the name, each ev00f8_sprite_create call
 * loads the address from the pool word at 0x0cb806fc. as
 * &ev00f8_sprite_texlists[EVENT_SPRITE_LENS], SHC loads it once and keeps it
 * on the stack for the two calls, ev00f8_script runs 12 bytes long, and 652
 * of 669 words differ */
extern NJS_TEXLIST ev00f8_sprite_lens_texlist;

static struct task *ev00f8_flash = NULL;

void ev00f8_script(int phase)
{
    /* @hack spelling: every held sound starts through sound_start, a local. SHC
     * then keeps event_sound_start's address in R12 and 1800 in R11
     * (0x0cb804ee, 0x0cb804ea). called directly, the two trade, and 8 of 669
     * words differ */
    void (*sound_start)(int slot, int se, int frames);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00f8.mlt");
        event_sound_init(3);
        event_set_pos(event_get_player(0), -4.28F, 88.0F, 70.0F);
        event_set_ang(event_get_player(0), 0, 0x7d00, 0);
        event_create_player(2, tikal_player_step, 0.0F, 90.0F, 39.0F, 0, 0x8000,
                            0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -72.85F, 100.0F, 80.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xef00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -3.44F, 100.0F, 90.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0, 0x0200, 0);
        event_wait(1);
        stream_request_music(MUSIC_TICAL);
        event_set_pos(event_get_player(0), -4.28F, 88.0F, 70.0F);
        event_set_ang(event_get_player(0), 0, 0x7d00, 0);
        event_play_action(event_get_player(0), &ev00f8_sonic_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(2), &ev00f8_tikal_action_0,
                          &tikal_texlist, 0.8F, EVENT_REPEAT, 0);
        event_voice_play(1958);
        event_message_show(TEXT_1);
        event_wait(1);
        event_set_pos(event_get_player(0), -4.28F, 88.0F, 70.0F);
        event_set_ang(event_get_player(0), 0, 0x7d00, 0);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -6.6F, 92.55F, 45.04F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0xed00, 0x0200);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 5.838F, 92.55F, 48.33F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x1700, 0x0500, 0xfd00);
        event_voice_play(1959);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.45F, 94.1F, 33.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0x9400, 0x0300);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 3.38F, 95.325F, 34.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0800, 0x6600, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 2.44F, 93.425F, 34.02F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x1700, 0x6c00, 0xfd00);
        event_message_show(TEXT_3);
        event_voice_play(1960);
        event_talk_start(event_get_player(2), TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(2), &ev00f8_tikal_action_1,
                          &tikal_texlist, 0.5F, 0, 8);
        event_play_action(event_get_player(2), &ev00f8_tikal_action_0,
                          &tikal_texlist, 0.6F, EVENT_REPEAT, 14);
        event_voice_play(1961);
        event_talk_start(event_get_player(2), TEXT_8);
        event_message_close();
        event_wait(110);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 2.12F, 95.5F, 34.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0c00, 0x6c00, 0xfd00);
        event_voice_play(1962);
        event_talk_start(event_get_player(2), TEXT_9);
        event_message_show(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_voice_play(1963);
        event_talk_start(event_get_player(2), TEXT_10);
        event_play_action(event_get_player(2), &ev00f8_tikal_action_2,
                          &tikal_texlist, 1.0F, 0, 12);
        event_play_action(event_get_player(2), &ev00f8_tikal_action_3,
                          &tikal_texlist, 1.2F, 0, 2);
        event_play_action(event_get_player(2), &ev00f8_tikal_action_4,
                          &tikal_texlist, 0.4F, EVENT_REPEAT, 0);
        event_message_show(TEXT_6);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 25, 4.68F, 93.2F, 30.33F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 25, 0x1100, 0x6c00, 0xfd00);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_rays_create(0.0F, 96.0F, 30.0F, 15, 20, 0.03F, 0.02F, 0x88ffffff,
                          0x88ffffff);
        sound_start = event_sound_start;
        sound_start(0, 1225, 1800);
        sound_start(1, 1226, 1800);
        sound_start(2, 1228, 1800);
        event_sound_fade_volume(0, 120, 1);
        event_sound_fade_volume(1, 120, 1);
        event_sound_fade_volume(2, 120, 1);
        event_wait(30);
        ev00f8_sprite_create(event_get_player(2), SWELL, 2.3F, 0.0F, -1.5F,
                             &ev00f8_sprite_lens_texlist, 0);
        ev00f8_sprite_create(event_get_player(2), FLARE, 2.3F, 3.5F, -1.5F,
                             &ev00f8_sprite_lens_texlist, 0);
        event_wait(1);
        ev00f8_sprite_set_scale(SWELL, 0.01F, 0.01F);
        ev00f8_sprite_set_scale(FLARE, 0.9F, 0.9F);
        ev00f8_sprite_turn(FLARE, 0, 0x0222);
        ev00f8_sprite_set_color(FLARE, 0.1F, 0.7F, 0.7F, 0.7F);
        ev00f8_sprite_set_color(SWELL, 1.0F, 0.2F, 1.0F, 0.2F);
        event_wait(6);
        ev00f8_sprite_set_growth(SWELL, 0.22F, 0.22F);
        event_message_close();
        event_wait(35);
        ev00f8_sprite_kill(SWELL);
        ev00f8_sprite_kill(FLARE);
        ev00f8_flash = event_fade_create(0.1F, 0.1F, 0.7F, 1.0F, 0.7F);
        event_wait(10);
        event_sound_stop(0);
        event_sound_stop(1);
        event_sound_stop(2);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev00f8_flash);
        event_sound_stop_all();
        ev00f8_sprite_kill(SWELL);
        ev00f8_sprite_kill(FLARE);
        event_rays_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f8_textures[] = {
    {"TIKAL", &tikal_texlist},
    {NULL, NULL},
};
