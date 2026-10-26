/* EV0016, Sonic's story: Sonic catches up with Zero as it carries Amy off, and
 * jumps at it. the Egg Carrier appears overhead, and its beam lifts Zero and
 * Amy away. the Egg Carrier then flies off */
#include <ninja.h>

#include "ev0016/script.h"
#include "ev0016/sonic_action.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/beam.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/prop.h"
#include "shared/sound.h"
#include "shared/zero.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 30
#define TEXT_2_LENGTH 48
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 13
#define TEXT_6_LENGTH 18
#define TEXT_7_LENGTH 1
#define TEXT_8_LENGTH 3
#include "ev0016/script/text.inc"

/* the slot of the event's props that holds the Egg Carrier */
#define EGG_CARRIER 0

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_trot_action;
extern NJS_ACTION sonic_jump_land_action;

extern NJS_TEXLIST ev0016_egg_carrier_texlist;
extern NJS_OBJECT ev0016_egg_carrier_object;
extern NJS_ACTION ev0016_egg_carrier_action;

/* player 0's pad as Sonic jumps at Zero, a key a step. from the 2nd key on, the
 * stick points at -107 to -103, in 256ths of a turn. the 12th to 15th keys hold
 * A, and so do the 27th and 28th */
static const struct event_pad_key ev0016_jump_keys[34] = {
#include "ev0016/script/ev0016_jump_keys.inc"
};

static const struct event_pad_recording ev0016_jump = {
    ev0016_jump_keys, sizeof(ev0016_jump_keys) / sizeof(ev0016_jump_keys[0])};

/* set ev0016_fade's speed, if it holds a fade */
/* @hack spelling: the test and the call as one expression, as
 * src/event/play.h's EVENT_KILL has them. SHC then loads event_clear_motions'
 * address into R8 at 0x0cb80110 and keeps it there for all seven calls. as ifs,
 * SHC loads the address at each call, ev0016_script runs 16 bytes long, and
 * 1163 of its 1241 words differ */
#define SET_FADE_SPEED(speed)                                                  \
    (ev0016_fade != NULL && (event_fade_set_speed(ev0016_fade, speed), 0))

static struct task *ev0016_zero = NULL;
static struct task *ev0016_beam = NULL;
static struct task *ev0016_fade = NULL;

void ev0016_script(int phase)
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
        sound_load_file("e_0009.mlt");
        event_set_pos(event_get_player(0), -52.19F, 204.99F, 1020.98F);
        event_set_ang(event_get_player(0), 0, 0xa603, 0);
        event_place_task(&ev0016_zero, event_zero_create, -157.48F, 78.0F,
                         1060.33F, 0, 0xca00, 0);
        event_prop_create(50.0F, -1600.0F, 1050.0F, 0, 0x8000, 0, 0.12F, 0.12F,
                          0.12F, &ev0016_egg_carrier_object,
                          &ev0016_egg_carrier_texlist, EGG_CARRIER);
        event_prop_set_action(EGG_CARRIER, &ev0016_egg_carrier_action, 0);
        ev0016_beam =
            event_beam_create(-176.84F, 78.0F, 941.55F, 0x0700, 0, -0x1500);
        event_wait(1);
        stream_request_music(MUSIC_EGGROBO);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_look_at_task(event_get_player(0), ev0016_zero, 0.0F, 6.0F, 0.0F);
        event_set_flags(ev0016_zero, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0016_zero, 1.2F);
        event_prop_play(EGG_CARRIER, 0, 1.0F, EVENT_PROP_FOREVER);
        event_beam_set_shape(ev0016_beam, 0.23F, 0.0F, 0.23F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev0016_beam, 0.0F, 10.0F, 0.0F, 0.0F, 0.0F, 0);
        event_beam_set_shape(ev0016_beam, 0.0F, 0.0F, 0.0F, 0.001F, 0.0F, 0);
        event_beam_set_shape(ev0016_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.001F, 0);
        event_beam_set_shape(ev0016_beam, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0);
        event_wait(1);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -171.32F, 91.57F, 1056.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x02b2, 0xd623, 0);
        event_camera_follow(ev0016_zero);
        event_message_show(TEXT_1);
        event_move(ev0016_zero, -157.48F, 78.0F, 1060.33F, -176.84F, 78.0F,
                   941.55F, 150);
        event_wait(10);
        event_voice_play(1412);
        event_wait(50);
        event_message_close();
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -210.12F, 76.11F, 917.43F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, ev0016_zero, 0.0F, 6.0F,
                                  0.0F, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 90, 0x2e39);
        event_wait(45);
        event_voice_play(1058);
        event_wait(30);
        event_message_show(TEXT_2);
        event_wait(15);
        event_move_stop(ev0016_zero);
        event_wait(40);
        event_camera_look_clear();
        event_camera_follow_clear();
        event_camera_look_at_task(EVENT_EASE_LINEAR, 15, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, -63.91F, 202.25F, 1014.03F);
        event_wait(40);
        event_message_close();
        event_set_ang(ev0016_zero, 0, 0xd800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -61.02F, 204.43F, 1015.73F);
        event_wait(10);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -44.97F, 224.14F, 1001.23F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -42.35F, 228.91F, 994.06F);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0016_jump);
        event_wait(70);
        event_camera_look_clear();
        event_camera_follow_clear();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -95.34F, 66.11F, 959.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x073a, 0x3b6b, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -141.74F, 68.78F,
                             1014.53F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x063a, 0x0e6b, 0);
        event_wait(30);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(100);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -152.17F, 65.43F, 954.43F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x153a, 0x9168, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -153.25F, 63.94F, 952.06F);
        event_wait(60);
        stream_stop_music();
        ev0016_fade = event_fade_create(0.1F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(6);
        SET_FADE_SPEED(0.0F);
        event_play_action(event_get_player(0), &ev0016_sonic_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_7);
        event_wait(15);
        event_voice_play(1059);
        event_message_show(TEXT_3);
        event_wait(5);
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_wait(10);
        event_set_pos(event_prop_get(EGG_CARRIER), 50.0F, 600.0F, 1050.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -248.82F, 109.03F, 960.87F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf2a3, 0xc5ac, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -270.19F, 116.44F, 963.89F);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -362.24F, 112.42F, 616.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1588, 0x9eb9, 0);
        stream_request_music(MUSIC_EGCARER1);
        event_wait(20);
        SET_FADE_SPEED(-0.01F);
        event_wait(40);
        event_camera_set_fov(EVENT_EASE_LINEAR, 240, 0x3111);
        event_wait(90);
        event_move(event_prop_get(EGG_CARRIER), 50.0F, 600.0F, 1050.0F, 100.0F,
                   650.0F, 1150.0F, 100);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x238e);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -134.08F, 74.16F, 1011.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x03bb, 0x1089, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 45, -154.39F, 74.16F, 1016.64F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0x03bb, 0x0589, 0);
        event_wait(45);
        event_beam_set_mode(ev0016_beam, BEAM_OPEN);
        sound_play_at_for(1226, EVENT_SOUND_ID, 1, 120, 100.0F, 600.0F, 1150.0F,
                          300);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 60, ev0016_zero, 0.0F,
                                  5.5F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -252.04F, 95.9F, 938.24F);
        event_wait(90);
        event_voice_play(1061);
        event_message_show(TEXT_4);
        event_move(ev0016_zero, -176.84F, 78.0F, 941.55F, 232.52F, 800.02F,
                   1089.03F, 300);
        event_wait(30);
        event_message_close();
        event_wait(120);
        event_voice_play(1063);
        event_message_show(TEXT_5);
        event_wait(30);
        event_message_close();
        event_beam_set_mode(ev0016_beam, BEAM_CLOSE);
        event_move_stop(ev0016_zero);
        event_kill_task(&ev0016_zero);
        event_camera_look_clear();
        event_wait(15);
        event_set_pos(event_get_player(0), -181.24F, 64.1F, 958.97F);
        event_set_ang(event_get_player(0), 0xff5e, 0x23db, 0xffce);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), 100.0F, 200.0F, 1150.0F);
        event_wait(30);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -170.5F, 66.88F, 958.86F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e9c, 0x45ea, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, -173.27F, 67.94F, 959.26F);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_8);
        event_wait(15);
        event_voice_play(1060);
        event_message_show(TEXT_6);
        event_wait(5);
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -254.81F, 88.5F, 809.78F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0,
                                  event_prop_get(EGG_CARRIER), 0.0F, 0.0F, 0.0F,
                                  0);
        event_sound_play(1231, 120, 0, 0);
        event_move_turn(event_prop_get(EGG_CARRIER), 100.0F, 600.0F, 1150.0F,
                        600.0F, 1500.0F, 1250.0F, 0, 0x8000, 0, 0, 0x4500,
                        -0x2000, 250);
        event_wait(150);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -21.98F, 204.04F, 921.54F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x3b00, 0x3bc5, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -5.42F, 69.06F, 923.26F);
        event_move_stop(event_prop_get(EGG_CARRIER));
        event_set_pos(event_prop_get(EGG_CARRIER), 50.0F, -1600.0F, 1050.0F);
        event_sound_play(1231, 120, 0, 0);
        event_wait(15);
        event_move_turn(event_prop_get(EGG_CARRIER), 1300.0F, 2500.0F, 750.0F,
                        -1400.0F, 2500.0F, 770.0F, 0, 0x4500, 0, 0, 0x4500, 0,
                        150);
        event_wait(100);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x0800, 0x39c5, 0);
        event_wait(20);
        event_look_clear(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_trot_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), -239.58F, 65.2F, 921.74F, 1.75F,
                           0.06F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, 8.15F, 66.33F, 925.35F);
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_move_stop(event_prop_get(EGG_CARRIER));
        event_wait(60);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_set_pos(event_get_player(0), -239.58F, 65.2F, 921.74F);
        event_set_ang(event_get_player(0), 0xff1c, 0xb366, 0xfe7c);
        event_move_stop_all();
        event_kill_task(&ev0016_zero);
        event_prop_kill(EGG_CARRIER);
        if (ev0016_fade != NULL) {
            task_kill(ev0016_fade);
            ev0016_fade = NULL;
        }
        event_beam_kill(ev0016_beam);
        ev0016_beam = NULL;
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0016_textures[] = {
    {"CAPTUREBEAM", &event_beam_texlist},
    {"AMY_EGGROBO", &event_zero_texlist},
    {"EC_LIGHT", &ev0016_egg_carrier_texlist},
    {NULL, NULL},
};
