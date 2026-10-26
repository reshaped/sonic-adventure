/* EV0022, Sonic's story: Tikal stands at the altar among the Chao. she asks why
 * it came to this, and runs off to stop it. Sonic is player 0 and Tikal player
 * 2, as event_get_player numbers them */
#include <ninja.h>

#include "ev0022/chao_action_2.h"
#include "ev0022/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 41
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 24
#define TEXT_5_LENGTH 3
#define TEXT_6_LENGTH 1
#define TEXT_7_LENGTH 3
#include "ev0022/script/text.inc"

extern NJS_ACTION ev0022_sonic_action_0;
extern NJS_ACTION ev0022_sonic_action_1;
extern NJS_ACTION ev0022_tikal_action_0;
extern NJS_ACTION ev0022_tikal_action_1;
extern NJS_ACTION ev0022_tikal_action_2;
extern NJS_ACTION ev0022_tikal_action_3;

/* @hack type: ev0022_chao_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps its address in R14 for the five calls that play
 * it, loaded at 0x0cb8021a. as one NJS_ACTION passed as &ev0022_chao_action_0,
 * SHC loads its address from the pool at each call, and keeps the calls' blend
 * of 4 in R9. ev0022_script then runs 8 bytes short, and 807 of 991 words
 * differ */
extern NJS_ACTION ev0022_chao_action_0[];
/* @hack type: ev0022_chao_action_1 declared as an array, which script.c passes
 * by its name. SHC then keeps its address in R11 for the four calls that play
 * it, loaded at 0x0cb80232. as one NJS_ACTION passed as &ev0022_chao_action_1,
 * SHC loads its address from the pool at each call. ev0022_script then runs 4
 * bytes short, and 779 of 991 words differ */
extern NJS_ACTION ev0022_chao_action_1[];

static struct task *ev0022_chao_0 = NULL;
static struct task *ev0022_chao_1 = NULL;
static struct task *ev0022_chao_2 = NULL;
static struct task *ev0022_chao_3 = NULL;
static struct task *ev0022_chao_4 = NULL;
static struct task *ev0022_chao_5 = NULL;
static struct task *ev0022_chao_6 = NULL;
static struct task *ev0022_chao_7 = NULL;
static struct task *ev0022_chao_8 = NULL;
static struct task *ev0022_chao_9 = NULL;
static struct task *ev0022_fade = NULL;

struct texture_pvm ev0022_textures[] = {
    {"TIKAL", &tikal_texlist},
    {"EV_ALIFE", &event_chao_texlist},
    {NULL, NULL},
};

void ev0022_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_reset_player(0);
        sound_load_file("e_0022.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_place_task(&ev0022_chao_0, event_chao_create, -37.633F, 1.0F,
                         404.723F, 0x2000, 0, 0);
        event_place_task(&ev0022_chao_1, event_chao_create, 43.805F, 1.023F,
                         460.182F, 0, 0x4000, 0);
        event_place_task(&ev0022_chao_2, event_chao_create, 44.573F, 1.023F,
                         322.835F, 0, 0, 0);
        event_place_task(&ev0022_chao_3, event_chao_create, -32.633F, 1.0F,
                         399.723F, 0, 0xc000, 0);
        event_place_task(&ev0022_chao_4, event_chao_create, 38.805F, 1.023F,
                         463.182F, 0, 0x1600, 0);
        event_place_task(&ev0022_chao_5, event_chao_create, 37.573F, 1.023F,
                         315.835F, 0, 0, 0);
        event_place_task(&ev0022_chao_6, event_chao_create, -40.633F, 1.0F,
                         400.723F, 0, 0x3000, 0);
        event_place_task(&ev0022_chao_7, event_chao_create, 50.805F, 1.023F,
                         450.182F, 0, 0xf000, 0);
        event_place_task(&ev0022_chao_8, event_chao_create, 47.573F, 1.023F,
                         325.835F, 0xa000, 0, 0);
        event_place_task(&ev0022_chao_9, event_chao_create, -30.633F, 1.0F,
                         395.723F, 0x3000, 0, 0);
        event_wait(1);
        event_play_action(ev0022_chao_0, ev0022_chao_action_0,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_1, ev0022_chao_action_1,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_2, &ev0022_chao_action_2,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_3, ev0022_chao_action_1,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_4, ev0022_chao_action_0,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_5, ev0022_chao_action_1,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_6, ev0022_chao_action_0,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_7, ev0022_chao_action_1,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_8, ev0022_chao_action_0,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_play_action(ev0022_chao_9, ev0022_chao_action_0,
                          &event_chao_texlist, 0.01F, EVENT_REPEAT, 4);
        event_set_pos(event_get_player(0), -2.772F, -48.864F, 674.891F);
        event_set_ang(event_get_player(0), 0x0500, 0x9a80, 0xff90);
        event_create_player(2, tikal_player_step, -15.8F, -4.2F, 434.39F, 0,
                            0x8000, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), -15.8F, -4.2F, 434.39F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        stream_request_music(MUSIC_EVTBGM04);
        event_sound_start(0, 1223, 1800);
        event_sound_fade_volume(0, -16, 0);
        event_sound_start(1, 1224, 1800);
        event_play_action(event_get_player(0), &ev0022_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(2), &ev0022_tikal_action_1,
                          &tikal_texlist, 1.0F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 3.56F, 1.068F, 472.074F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0xae00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 15.1068F, 1.06864F,
                             447.666F);
        event_wait(160);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 90.94758F, 16.57842F,
                             357.992F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0x0a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 56.50751F, 14.13921F,
                             358.29428F);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 42.91408F, 16.1368F,
                             361.649F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xfe00, 0xfa00, 0);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -29.4228F, 2.87091F,
                             415.43176F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x0c00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, -17.5211F, 2.87091F,
                             454.66626F);
        event_wait(160);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, -14.14F, 2.66F, 451.116F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0300, 0x0500, 0);
        event_wait(60);
        event_voice_play(1132);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_wait(60);
        event_play_action(event_get_player(2), &ev0022_tikal_action_2,
                          &tikal_texlist, 0.8F, 0, 4);
        event_wait(10);
        event_voice_play(1133);
        event_talk_start(event_get_player(2), TEXT_5);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_sound_fade_volume(0, -100, 80);
        event_sound_fade_volume(1, -100, 80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -13.607F, -39.803F,
                             669.815F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf900, 0xab00, 0);
        event_clear_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, -8.463F, -40.829F,
                             672.728F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -17.837F, 4.429F, 448.622F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa00, 0xf900, 0);
        event_voice_play(1134);
        event_talk_start(event_get_player(2), TEXT_6);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(2));
        event_sound_play(1225, 0, 0, 0);
        event_wait(30);
        event_play_action(event_get_player(2), &ev0022_tikal_action_3,
                          &tikal_texlist, 0.6F, 0, 8);
        event_talk_start(event_get_player(2), TEXT_7);
        event_voice_play(1135);
        event_message_show(TEXT_4);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0500, 0xf900, 0);
        event_wait(16);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_wait(1);
        event_play_action(event_get_player(0), &ev0022_sonic_action_0,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0022_tikal_action_0,
                          &tikal_texlist, 0.8F, 0, 0);
        event_wait(20);
        event_clear_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -9.69F, 4.796F, 427.153F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf600, 0x6400, 0);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_wait(30);
        event_run_to(event_get_player(2), 0.0F, 91.0F, 46.73F);
        event_wait(20);
        event_set_pos(event_get_player(0), -5.26F, 7.5F, 253.037F);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1.974F, 108.45F, 107.15F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xee00, 0, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 100, 0x471c);
        event_set_pos(event_get_player(2), 1.85F, 91.0F, 89.983F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_camera_follow(event_get_player(2));
        event_run_wait(event_get_player(2));
        event_clear_motions(event_get_player(2));
        event_wait(30);
        event_camera_follow_clear();
        event_wait(30);
        event_set_pos(event_get_player(0), -2.772F, -48.864F, 674.891F);
        event_set_ang(event_get_player(0), 0x0500, 0x9a80, 0xff90);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -33.125F, -31.933F,
                             648.801F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf600, 0xa200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -9.406F, -39.951F,
                             670.298F);
        event_camera_set_fov(EVENT_EASE_LINEAR, 110, 0x31c7);
        event_wait(50);
        event_sound_stop(0);
        event_sound_stop(1);
        event_sound_start(0, SE_TIKAL_CHIME, 88);
        ev0022_fade = event_fade_create(1.0F / 60.0F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_kill_player(2);
        event_wait(90);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_kill_task(&ev0022_chao_0);
        event_kill_task(&ev0022_chao_1);
        event_kill_task(&ev0022_chao_2);
        event_kill_task(&ev0022_chao_3);
        event_kill_task(&ev0022_chao_4);
        event_kill_task(&ev0022_chao_5);
        event_kill_task(&ev0022_chao_6);
        event_kill_task(&ev0022_chao_7);
        event_kill_task(&ev0022_chao_8);
        event_kill_task(&ev0022_chao_9);
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev0022_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}
