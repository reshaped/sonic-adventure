/* EV00D0, Big's story opens: Froggy grows a tail, and runs off with Big's lucky
 * charm */
#include <ninja.h>

#include "chaos/models.h"
#include "ev00d0/emerald_action.h"
#include "ev00d0/emerald_textures.h"
#include "ev00d0/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/chaos0.h"
#include "shared/floor.h"
#include "shared/frog.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/sparkle.h"
#include "shared/sprite.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 30
#define TEXT_4_LENGTH 32
#define TEXT_5_LENGTH 34
#define TEXT_6_LENGTH 26
#define TEXT_7_LENGTH 42
#define TEXT_8_LENGTH 22
#include "ev00d0/script/text.inc"

extern NJS_ACTION ev00d0_big_action_0;
extern NJS_ACTION ev00d0_big_action_1;
extern NJS_ACTION ev00d0_big_action_2;
extern NJS_ACTION ev00d0_big_action_3;
extern NJS_ACTION ev00d0_big_action_4;
extern NJS_ACTION ev00d0_big_action_5;
extern NJS_ACTION ev00d0_big_action_6;
extern NJS_ACTION ev00d0_big_action_7;
extern NJS_ACTION ev00d0_frog_action_0;
extern NJS_ACTION ev00d0_frog_action_1;
extern NJS_ACTION ev00d0_frog_action_2;
extern NJS_ACTION ev00d0_frog_action_3;
extern NJS_ACTION ev00d0_frog_action_4;
extern NJS_ACTION ev00d0_frog_action_5;
extern NJS_TEXLIST event_chaos0_water_texlist;

static struct task *ev00d0_emerald = NULL; /* Big's lucky charm */
static struct task *ev00d0_floor = NULL;
static struct task *ev00d0_frog = NULL;

static NJS_POINT3 ev00d0_sparkle_pos;
static NJS_VECTOR ev00d0_sparkle_speed;

void ev00d0_script(int phase)
{
    /* @hack spelling: Froggy's slot through its address, which leaves Big's x,
     * 1292.83, in FR12 and the camera's x, 1279.35, in FR14 (0x0cb8009e,
     * 0x0cb80488); read by name, the two trade, and 16 of 2199 words and 4
     * pool words differ */
    struct task **frog;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_00D0.mlt");
        event_sound_init(5);
        ev00d0_floor = event_floor_create(event_get_player(0), 131.0F);
        event_set_pos(event_get_player(0), 1292.83F, 130.0F, -725.2F);
        event_set_ang(event_get_player(0), 0, 0xd300, 0);
        event_reset_player(0);
        event_place_task(&ev00d0_frog, event_frog_create,
                         event_get_player(0)->work->pos.x,
                         event_get_player(0)->work->pos.y + 12.0F,
                         event_get_player(0)->work->pos.z, 0, 0xd300, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1700, 0x4300, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1263.32F, 316.32F, -615.8F);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 1273.976F, 245.9F,
                             -617.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x1000, 0x4300, 0);
        event_wait(130);
        event_sprite_create_loop(NULL, 0, 700.0F, 490.0F, -597.0F,
                                 &event_sprite_texlists[EVENT_SPRITE_SPARKLE]);
        event_sprite_set_scale(0, -9.5F, -9.5F);
        event_wait(8);
        event_sprite_kill(0);
        event_wait(10);
        ev00d0_sparkle_pos.x = 700.0F;
        ev00d0_sparkle_pos.y = 490.0F;
        ev00d0_sparkle_pos.z = -597.0F;
        ev00d0_sparkle_speed.x = 0.01F;
        ev00d0_sparkle_speed.y = -3.6F;
        ev00d0_sparkle_speed.z = -0.12F;
        event_sparkle_create_shrinking(&ev00d0_sparkle_pos,
                                       &ev00d0_sparkle_speed, 0.6F, 1.0F);
        event_wait(29);
        event_frog_set_shape(2);
        frog = &ev00d0_frog;
        event_set_flags(*frog, EVENT_IGNORE_GROUND);
        event_set_pos(*frog, 1298.76F, 138.75F, -732.75F);
        event_set_ang(*frog, 0xf500, 0x9500, -0x1000);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_clear_motions(*frog);
        event_play_action(event_get_player(0), &ev00d0_big_action_6,
                          &big_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(*frog, &ev00d0_frog_action_5, &event_frog_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_sound_start(1, 1224, 1800);
        event_sound_fade_volume(1, -110, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 105, 0x0600, 0x4122, 0x00ee);
        event_camera_set_pos(EVENT_EASE_LINEAR, 105, 1290.7F, 135.38F,
                             -619.66F);
        event_wait(1);
        event_sound_fade_volume(1, -40, 59);
        event_wait(59);
        event_sound_fade_volume(1, -70, 90);
        event_wait(40);
        event_sprite_kill(0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1000, 0x4100, 0x0100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 1290.19F, 123.2757F,
                             -619.75F);
        event_place_actor(&ev00d0_emerald, -550.33F, 1.5F, 973.9F, 0, 0, 0);
        event_set_flags(ev00d0_emerald, EVENT_IGNORE_GROUND);
        event_wait(40);
        event_set_pos(ev00d0_emerald, 1290.84F, 126.57F, -685.465F);
        event_set_ang(ev00d0_emerald, 0x2000, 0, 0);
        event_play_action(ev00d0_emerald, &ev00d0_emerald_action,
                          &ev00d0_emerald_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0700, 0xf400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1240.687F, 132.05F,
                             -693.63F);
        event_wait(4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 1275.525F, 132.05F,
                             -683.0768F);
        event_wait(140);
        event_camera_set_pos(EVENT_EASE_LINEAR, 71, 1279.35F, 132.05F, -681.9F);
        event_wait(66);
        vibration_start(0, 4, 50, 8);
        event_play_action(event_get_player(0), &ev00d0_big_action_7,
                          &big_texlist, 1.0F, EVENT_REPEAT, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0900, 0xf400, 0);
        event_wait(2);
        event_sound_stop(1);
        event_sound_play(1228, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, 1279.35F, 134.05F, -681.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfa00, 0xf400, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0700, 0xf400, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfc00, 0xf500, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, 1279.35F, 130.05F, -681.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0600, 0xf400, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfe00, 0xf400, 0);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 1, 1279.35F, 132.05F, -681.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0700, 0xf400, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfd00, 0xf400, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0600, 0xf300, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xff00, 0xf400, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0500, 0xf400, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xff00, 0xf500, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0400, 0xf400, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0, 0xf400, 0);
        event_wait(3);
        event_play_action(event_get_player(0), &ev00d0_big_action_6,
                          &big_texlist, 0.5F, EVENT_REPEAT, 6);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0400, 0xf400, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0x0200, 0xf400, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 190, 1280.3F, 132.28F, -685.8F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 190, 0x238e);
        event_wait(30);
        stream_request_music(MUSIC_EVTBGM04);
        event_move(*frog, 1298.76F, 138.75F, -732.75F, 1298.4325F, 143.0F,
                   -731.9F, 6);
        event_wait(20);
        event_set_ang(*frog, 0xf500, 0xc000, 0x1000);
        event_wait(25);
        event_set_ang(*frog, 0xf500, 0x9500, 0);
        event_wait(18);
        event_play_action(*frog, &ev00d0_frog_action_1, &event_frog_texlist,
                          1.1F, EVENT_REPEAT, 0);
        event_wait(15);
        event_set_ang(*frog, 0xf500, 0xc400, 0x1000);
        event_wait(5);
        event_move(*frog, 1298.4325F, 143.0F, -731.9F, 1293.0F, 131.5F,
                   -730.84F, 5);
        event_wait(10);
        event_move(*frog, 1293.0F, 131.5F, -730.84F, 1257.0F, 130.0F, -728.8F,
                   35);
        event_wait(35);
        event_move_turn(*frog, 1257.0F, 130.0F, -728.8F, 1236.7F, 120.0F,
                        -712.5F, 0xf500, 0xc400, 0x1000, 0, 0x7c00, 0, 1);
        event_play_action(*frog, &ev00d0_frog_action_1, &event_frog_texlist,
                          1.45F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), 1292.83F, 131.0F, -725.2F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1147.6F, 100.8F, -697.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0900, 0xcb00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3555);
        event_wait(1);
        event_set_flags(*frog, EVENT_GROUND | EVENT_SLOPE);
        event_set_radius(*frog, 1.0F);
        event_move(*frog, 1236.7F, 120.0F, -712.5F, 1137.74F, 99.6F, -707.98F,
                   145);
        event_wait(101);
        event_chaos0_create(1081.49F, 97.45F, -711.3F, 0, 0, 0, 1);
        event_wait(4);
        event_chaos0_melt();
        event_camera_set_ang(EVENT_EASE_LINEAR, 122, 0x0300, 0x3300, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 122, 1141.7F, 100.4F, -705.0F);
        event_wait(42);
        event_play_action(*frog, &ev00d0_frog_action_0, &event_frog_texlist,
                          0.5F, EVENT_REPEAT, 0);
        event_wait(10);
        event_set_flags(*frog, EVENT_IGNORE_GROUND);
        event_wait(70);
        event_camera_set_ang(EVENT_EASE_LINEAR, 95, 0x0300, 0x3500, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 95, 1140.753F, 100.4736F,
                             -705.3F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 95, 0x3111);
        event_wait(64);
        event_wait(41);
        event_play_action(*frog, &ev00d0_frog_action_0, &event_frog_texlist,
                          0.1F, EVENT_REPEAT, 0);
        event_chaos0_slide(1123.777F, 96.54F, -708.57F, 44);
        event_sound_start(2, 1223, 1800);
        event_sound_fade_volume(2, -20, 1);
        event_wait(4);
        event_camera_set_ang(EVENT_EASE_LINEAR, 41, 0xf0c0, 0x3ba0, 0xfee0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 41, 1139.344F, 101.58F,
                             -706.3F);
        event_wait(40);
        event_sound_stop(2);
        event_set_pos(*frog, 1137.74F, 99.8F, -707.98F);
        event_chaos0_kill();
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf900, 0xc300, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1080.94F, 106.77F, -703.69F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 45, 0x271c);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, 1134.94F, 101.0F, -707.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0xfb00, 0xc300, 0x0900);
        event_wait(20);
        event_camera_set_ang(EVENT_EASE_LINEAR, 25, 0xfb00, 0xc300, 0xff00);
        event_wait(29);
        stream_stop_music();
        event_set_pos(event_get_player(0), 1292.83F, 131.0F, -725.2F);
        event_look_at(event_get_player(0), 1265.0F, 142.0F, -714.5F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x00b6);
        event_wait(1);
        event_frog_set_shape(FROG_SCRIPT_RESTORE);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x090f, 0xdcf9, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1285.043F, 133.28F,
                             -715.185F);
        event_wait(60);
        event_play_action(*frog, &ev00d0_frog_action_0, &event_frog_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0600, 0xdcf9, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 1274.067F, 135.1F,
                             -706.43F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 160, 0x31c7);
        event_wait(140);
        stream_request_music(MUSIC_THEBIG);
        event_camera_set_ang(EVENT_EASE_LINEAR, 340, 0x0c00, 0xdff9, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 340, 1281.04F, 136.57F,
                             -712.478F);
        event_play_action(event_get_player(0), &ev00d0_big_action_0,
                          &big_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &ev00d0_big_action_1,
                          &big_texlist, 1.0F, EVENT_REPEAT, 16);
        event_wait(380);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0400, 0xe000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 1283.78F, 141.68F,
                             -715.487F);
        event_wait(60);
        event_voice_play(1800);
        event_message_show(TEXT_1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 40, 0x0400, 0xdf00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 1284.5F, 141.778F,
                             -716.173F);
        event_play_action(event_get_player(0), &ev00d0_big_action_1,
                          &big_texlist, 0.8F, EVENT_REPEAT, 16);
        event_message_close();
        event_wait(25);
        event_set_pos(event_get_player(0), 1292.83F, 131.0F, -725.2F);
        event_set_ang(*frog, 0, 0xb000, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf500, 0x4300, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1302.94F, 144.6F, -725.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0xec00, 0x4200, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 1303.6F, 144.6F, -716.83F);
        event_voice_play(1802);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_wait(10);
        event_voice_play(1804);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_wait(10);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 230, 0x0d00, 0x4600, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 1145.97F, 98.877F,
                             -707.019F);
        event_wait(180);
        event_play_action(event_get_player(0), &ev00d0_big_action_4,
                          &big_texlist, 1.0F, EVENT_REPEAT, 20);
        event_play_action(*frog, &ev00d0_frog_action_3, &event_frog_texlist,
                          1.0F, 0, 20);
        event_wait(32);
        event_camera_set_pos(EVENT_EASE_LINEAR, 85, 1145.84F, 98.877F,
                             -709.015F);
        event_wait(25);
        event_set_ang(*frog, 0, 0xa000, 0);
        event_play_action(*frog, &ev00d0_frog_action_4, &event_frog_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_wait(63);
        event_move(*frog, 1137.74F, 99.8F, -707.98F, 1153.1454F, 110.53F,
                   -720.874F, 15);
        event_look_clear(event_get_player(0));
        /* the EVENT_END case kills the floor with EVENT_KILL, play.h's test
         * and kill as one && expression. EVENT_KILL in place of this if makes
         * 1802 of 2199 words differ */
        if (ev00d0_floor != NULL) {
            task_kill(ev00d0_floor);
            ev00d0_floor = NULL;
        }
        event_set_pos(event_get_player(0), 1262.02F, 121.0F, -711.54F);
        event_set_ang(event_get_player(0), 0, 0xbd00, 0);
        event_wait(15);
        event_clear_motions(event_get_player(0));
        event_clear_motions(*frog);
        event_play_action(*frog, &ev00d0_frog_action_1, &event_frog_texlist,
                          1.6F, EVENT_REPEAT, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b00, 0xb800, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1251.25F, 134.0F,
                             -713.8566F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0400, 0xb700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 1234.352F, 127.68F,
                             -712.62F);
        event_set_ang(*frog, 0x1200, 0x3a00, 0);
        event_set_radius(*frog, 1.5F);
        event_set_flags(*frog, EVENT_GROUND | EVENT_SLOPE);
        event_move(*frog, 1229.8F, 112.0F, -702.825F, 1287.5F, 127.56F, -685.8F,
                   80);
        event_voice_play(1806);
        event_message_show(TEXT_4);
        event_wait(10);
        event_play_action(event_get_player(0), &ev00d0_big_action_2,
                          &big_texlist, 0.8F, 0, 8);
        event_wait_motions(event_get_player(0));
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0700, 0x3a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1293.52F, 127.11F, -687.95F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00d0_big_action_4,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(*frog, &ev00d0_frog_action_0, &event_frog_texlist,
                          1.0F, EVENT_REPEAT, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0b00, 0x4700, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 1293.3766F, 127.1445F,
                             -686.0F);
        event_voice_wait();
        event_message_close();
        event_play_action(*frog, &ev00d0_frog_action_2, &event_frog_texlist,
                          1.7F, 0, 1);
        event_frog_set_shape(0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0e00, 0x4700, 0xfd00);
        event_wait(56);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1273.12F, 136.2337F,
                             -711.75F);
        event_kill_task(&ev00d0_emerald);
        event_camera_set_ang(EVENT_EASE_LINEAR, 105, 0x0d00, 0x3400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 105, 1277.12F, 131.7F,
                             -707.49F);
        event_wait(10);
        event_frog_set_shape(6);
        event_play_action(event_get_player(0), &ev00d0_big_action_5,
                          &big_texlist, 1.2F, 0, 4);
        event_wait_motions(event_get_player(0));
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xef00, 0xa100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1257.6F, 144.8876F,
                             -715.4865F);
        event_play_action(event_get_player(0), &ev00d0_big_action_4,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_flags(*frog, EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(0), 1266.3F, 120.4F, -707.2447F);
        event_set_ang(event_get_player(0), 0, 0x2100, 0);
        event_set_pos(*frog, 1287.1F, 129.0F, -685.8F);
        event_set_ang(*frog, 0, 0xae00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 120, 0x29f5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 1259.9F, 142.5F, -713.27F);
        event_voice_play(1809);
        event_message_show(TEXT_5);
        event_wait(85);
        event_play_action(*frog, &ev00d0_frog_action_1, &event_frog_texlist,
                          1.5F, EVENT_REPEAT, 8);
        event_wait(10);
        event_move(*frog, 1287.1F, 129.0F, -685.8F, 1276.16F, 122.44F, -691.94F,
                   10);
        event_message_close();
        event_wait(10);
        event_play_action(event_get_player(0), &ev00d0_big_action_3,
                          &big_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(0), 0, 0xd500, 0);
        event_move(*frog, 1270.58F, 122.44F, -686.3667F, 1236.64F, 120.8F,
                   -704.8F, 70);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0900, 0xc800, 0xfe00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1249.0F, 121.6F, -688.954F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, *frog, 0.0F, 5.0F, 0.0F,
                                  0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 63, 1230.62F, 121.9213F,
                             -701.76F);
        event_wait(30);
        event_set_ang(event_get_player(0), 0, 0xd400, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xd300, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xd200, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xd100, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xd000, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xcf00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xce00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xcd00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xcc00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xcb00, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xca00, 0);
        event_wait(40);
        event_camera_look_clear();
        event_clear_motions(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0xc500, 0);
        event_set_ang(*frog, 0, 0xc100, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1227.526F, 150.844F,
                             -682.3644F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe844, 0xe80c, 0x0100);
        event_move(*frog, 1236.64F, 120.8F, -704.8F, 1204.067F, 115.2F,
                   -704.16F, 60);
        event_run_to(event_get_player(0), 1244.19F, 122.3F, -706.04F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 250, 1220.78F, 145.7F, -682.8F);
        event_voice_play(1812);
        event_message_show(TEXT_6);
        event_wait(1);
        event_voice_play(1814);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1235.94F, 138.73F, -701.22F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf044, 0xd80c, 0xfc00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 1229.0F, 142.1F,
                             -696.5789F);
        event_wait(20);
        event_voice_play(1815);
        event_message_show(TEXT_8);
        event_wait(15);
        event_run_to(event_get_player(0), 1196.19F, 117.3F, -708.9F);
        event_voice_wait();
        event_message_close();
        event_run_clear(event_get_player(0));
        /* this call and the next pass a null task, so both return at once */
        event_set_pos(NULL, 1173.5F, 97.133F, -703.576F);
        event_set_ang(NULL, 0x04bb, 0xc93a, 0xffe7);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        /* this call and the next pass a null task too, so both return at
         * once */
        event_set_pos(NULL, 1168.58F, 97.6F, -710.6F);
        event_set_ang(NULL, 0, 0xb850, 0);
        event_frog_set_shape(FROG_SCRIPT_RESTORE);
        event_move_stop_all();
        event_sprite_kill(0);
        event_wait(1);
        event_chaos0_kill();
        EVENT_KILL(ev00d0_floor);
        event_reset_player(0);
        event_kill_task(&ev00d0_frog);
        event_kill_task(&ev00d0_emerald);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00d0_textures[] = {
    {"SHAPE_FROG", &event_frog_texlist},
    {"M_EM_YELLOW", &ev00d0_emerald_texlist},
    {"CHAOS0", &event_chaos0_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};
