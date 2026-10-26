/* EV0030: Tails' new plane stalls on a test flight and falls, trailing smoke */
#include <ninja.h>

#include "adv00/texlists.h"
#include "ev0030/plane.h"
#include "ev0030/script.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/clouds.h"
#include "shared/dust.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/picture.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 37
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 29
#define TEXT_5_LENGTH 27
#define TEXT_6_LENGTH 23
#define TEXT_7_LENGTH 21
#define TEXT_8_LENGTH 19
#define TEXT_9_LENGTH 23
#include "ev0030/script/text.inc"

/* the plane's climb and dive, as offsets from where it stands */
static struct event_path_point ev0030_plane_points_0[100] = {
#include "ev0030/script/ev0030_plane_points_0.inc"
};

static struct event_path ev0030_plane_path_0 = {100, ev0030_plane_points_0};

/* the plane's fall, from 465 high to 116 */
static struct event_path_point ev0030_plane_points_1[200] = {
#include "ev0030/script/ev0030_plane_points_1.inc"
};

static struct event_path ev0030_plane_path_1 = {200, ev0030_plane_points_1};

/* the plane's last flight, which climbs from 218 high to 332 and sinks to
 * 180 */
static struct event_path_point ev0030_plane_points_2[105] = {
#include "ev0030/script/ev0030_plane_points_2.inc"
};

static struct event_path ev0030_plane_path_2 = {105, ev0030_plane_points_2};

static struct task *ev0030_sky = NULL;
static struct task *ev0030_fade = NULL;
static struct task *ev0030_plane = NULL;
static struct task *ev0030_smoke = NULL;
/* a second dust task. the script never sets its timer, so it never puffs */
static struct task *ev0030_smoke_2 = NULL;

void ev0030_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0003.mlt");
        event_sound_init(5);
        ev0030_sky = event_picture_create(&ss_bg_texlist);
        event_picture_set_texture(ev0030_sky, 0);
        event_picture_set_depth(ev0030_sky, -790.0F);
        if (ev0030_smoke == NULL) {
            ev0030_smoke = event_dust_create();
        }
        if (ev0030_smoke_2 == NULL) {
            ev0030_smoke_2 = event_dust_create();
        }
        if (ev0030_smoke != NULL) {
            DUST_WORK(ev0030_smoke)->params->scale = 1.2F;
        }
        if (ev0030_smoke != NULL) {
            DUST_WORK(ev0030_smoke)->params->rate = 0.09F;
        }
        /* @hack spelling: the test and the color stores as one && expression.
         * SHC then moves 0.3F through FPUL into FR3 for the r, g and b stores
         * (LDS R2,FPUL at 0x0cb80110). as an if, SHC reads 0.3F straight from
         * the pool into FR4, ev0030_script runs 4 bytes long, and 1587 of its
         * 1713 words differ */
        ev0030_smoke != NULL &&
            (DUST_WORK(ev0030_smoke)->params->color.a = 0.9F,
             DUST_WORK(ev0030_smoke)->params->color.r = 0.3F,
             DUST_WORK(ev0030_smoke)->params->color.g = 0.3F,
             DUST_WORK(ev0030_smoke)->params->color.b = 0.3F);
        event_set_pos(ev0030_smoke, 313.0F, 933.35F, 752.26F);
        event_place_task(&ev0030_plane, ev0030_plane_create, 400.0F, 470.0F,
                         400.0F, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 103.7F, 247.7F, 1002.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0xfb00, 0);
        event_sound_start(0, 1223, 1800);
        event_sound_fade_volume(0, -65, 1);
        event_move(ev0030_plane, 400.0F, 470.0F, 400.0F, 180.0F, 470.0F, 400.0F,
                   180);
        event_wait(180);
        event_move_stop_all();
        event_picture_fade(ev0030_sky, 1.0F, 1);
        event_set_pos(ev0030_plane, 321.0424F, 933.84F, 750.8F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 323.21F, 941.0F, 762.335F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf100, 0, 0);
        event_sound_fade_volume(0, -30, 1);
        event_clouds_start(2.2F, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, 315.8F, 941.0F, 759.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0xf100, 0xed00, 0);
        event_wait(50);
        event_voice_play(1175);
        event_wait(5);
        event_message_show(TEXT_1);
        event_wait(35);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, 314.95F, 942.0F, 767.857F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0xf800, 0xed00, 0);
        event_voice_play(1176);
        event_message_show(TEXT_2);
        event_voice_wait();
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 307.97F, 930.9F, 760.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0xdf00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 95, 305.66F, 930.9F, 759.0F);
        event_wait(110);
        ev0030_plane_play(1, 2.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x1100, 0xdf00, 0);
        event_sound_start(2, 1215, 1800);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x0500, 0xdf00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x0f00, 0xdf00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x0900, 0xdf00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x0d00, 0xdf00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x0a00, 0xdf00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x0c00, 0xdf00, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 4, 0x0a00, 0xdf00, 0);
        event_sound_stop(2);
        event_wait(37);
        ev0030_plane_play(3, 0.1F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x216c);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 316.17F, 940.122F, 760.044F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf900, 0xe500, 0);
        event_wait(1);
        event_voice_play(1177);
        event_message_show(TEXT_3);
        ev0030_plane_play(3, 1.4F);
        event_voice_wait();
        event_message_close();
        stream_request_music(MUSIC_CIRCUIT);
        event_picture_set_depth(ev0030_sky, -700.0F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 323.93F, 940.143F, 750.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xee00, 0x4100, 0);
        ev0030_plane_play(3, 0.2F);
        event_wait(20);
        event_sound_start(1, 1216, 1800);
        event_sound_fade_volume(1, -30, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xee00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf000, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xec00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xef00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xed00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf000, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xec00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xef00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xed00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xec00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xef00, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xec00, 0x4100, 0);
        event_sound_stop(1);
        event_wait(37);
        if (ev0030_smoke != NULL) {
            DUST_WORK(ev0030_smoke)->timer = DUST_WORK(ev0030_smoke)->interval =
                6;
        }
        event_clouds_stop();
        event_sound_start(1, 1216, 1800);
        event_sound_fade_volume(1, 0, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf500, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xe400, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf500, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xe400, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf400, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xe500, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf500, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xe500, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 4, 0xf400, 0x4100, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe500, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf500, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe600, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf500, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe600, 0x4100, 0);
        event_wait(2);
        event_voice_play(1178);
        event_message_show(TEXT_4);
        /* @hack spelling: the test and the stores as one && expression. SHC
         * then keeps ev0030_plane's address in R11, loaded at 0x0cb80022. as an
         * if, it goes in R9 instead, ev0030_script runs 44 bytes short, and
         * 1423 of its 1713 words differ */
        ev0030_smoke != NULL && (DUST_WORK(ev0030_smoke)->timer =
                                     DUST_WORK(ev0030_smoke)->interval = 3);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe300, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf900, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe200, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe300, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf900, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe200, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf800, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe300, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf900, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe200, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xf900, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe200, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfa00, 0x4100, 0);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe100, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfa00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe100, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfa00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe100, 0x4100, 0);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe000, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe000, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe000, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe000, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe000, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe000, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfb00, 0x4100, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xe000, 0x4100, 0);
        event_wait(2);
        if (ev0030_smoke != NULL) {
            DUST_WORK(ev0030_smoke)->timer = DUST_WORK(ev0030_smoke)->interval =
                2;
        }
        event_clouds_start(1.0F, 5);
        event_wait(15);
        event_follow(ev0030_smoke, 0.0F, 0.0F, 0.0F, 9999, ev0030_plane);
        ev0030_plane_play(3, 1.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 323.7F, 931.4F, 835.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0, 0);
        event_sound_fade_pan(0, -100, 40);
        event_play_path(ev0030_plane, &ev0030_plane_path_0, 1.5F,
                        EVENT_FROM_HERE);
        event_wait(5);
        event_voice_play(1179);
        event_wait(28);
        event_voice_stop();
        event_clear_paths(ev0030_plane);
        event_wait(6);
        event_sound_fade_pan(0, 100, 1);
        event_wait(1);
        event_sound_fade_pan(0, -100, 50);
        event_set_ang(ev0030_plane, 0, 0x4000, 0x1000);
        event_move(ev0030_plane, 400.0F, 1000.0F, 820.0F, 250.0F, 850.0F,
                   790.0F, 50);
        event_wait(3);
        event_voice_play(1179);
        event_wait(28);
        event_voice_stop();
        event_wait(20);
        event_sound_fade_pan(0, 100, 50);
        event_set_ang(ev0030_plane, 0, -0x2300, 0x1000);
        event_move(ev0030_plane, 200.0F, 850.0F, 650.0F, 370.0F, 960.0F, 880.0F,
                   55);
        event_voice_play(1179);
        event_message_show(TEXT_5);
        event_wait(35);
        event_move_stop_all();
        event_clouds_stop();
        EVENT_KILL(ev0030_sky);
        if (ev0030_smoke != NULL) {
            DUST_WORK(ev0030_smoke)->timer = DUST_WORK(ev0030_smoke)->interval =
                1;
        }
        event_follow(ev0030_smoke, 0.0F, 0.0F, 0.0F, 9999, ev0030_plane);
        event_play_path(ev0030_plane, &ev0030_plane_path_1, 1.0F, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 201, 0x3111);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 309.97F, 1.68F, 1241.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1321, 0xe6af, 0);
        event_sound_fade_volume(0, -40, 1);
        event_sound_fade_volume(1, -20, 1);
        event_wait(90);
        event_voice_play(1180);
        event_message_show(TEXT_6);
        event_wait(91);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 519.7286F, 144.2F, 1048.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe521, 0x29af, 0xe900);
        event_camera_follow(ev0030_plane);
        event_sound_fade_volume(0, -55, 45);
        event_sound_fade_volume(1, -45, 45);
        event_move(ev0030_plane, 501.64F, 116.427F, 1041.0998F, 391.21F,
                   28.1624F, 1047.48F, 45);
        event_voice_play(1181);
        event_message_show(TEXT_7);
        event_wait(26);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 382.74F, 23.245F, 1048.732F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x41af, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 233.2F, 10.2F, 1061.0F);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, 225.658F, 0.8085F,
                             1086.586F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0x1500, 0x75af, 0);
        event_camera_follow_clear();
        event_voice_play(1182);
        event_message_show(TEXT_8);
        event_set_ang(ev0030_plane, -0x2926, -0x2e6d, 0x2ae0);
        event_move(ev0030_plane, 202.06F, -5.74F, 1061.336F, 201.5277F, 170.58F,
                   1295.58F, 50);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 198.558F, 174.8555F,
                             1290.742F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xc3c7, 0x9cbd, 0x7400);
        event_move(ev0030_plane, 201.5277F, 170.58F, 1295.58F, 193.363F,
                   304.837F, 1324.21F, 50);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 51, 190.72F, 309.978F,
                             1319.3353F);
        event_wait(52);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x38e4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 314.945F, 0.4F, 1580.83F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x12c7, 0x50bd, 0);
        event_sound_fade_volume(0, -95, 100);
        event_sound_fade_volume(1, -95, 100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 320.8F, 0.4F, 1522.637F);
        event_play_path(ev0030_plane, &ev0030_plane_path_2, 0.9F, 0);
        event_voice_play(1183);
        event_message_show(TEXT_9);
        event_voice_wait();
        event_message_close();
        event_sound_play(1225, 0, 0, 0);
        ev0030_fade = event_fade_create(1.0F / 60.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_wait(70);
        event_sound_stop(0);
        event_sound_stop(1);
        event_sound_stop(2);
        if (ev0030_smoke != NULL) {
            DUST_WORK(ev0030_smoke)->timer = DUST_WORK(ev0030_smoke)->interval =
                0;
        }
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_move_stop_all();
        event_clear_paths(ev0030_plane);
        event_reset_player(0);
        event_kill_task(&ev0030_plane);
        event_clouds_stop();
        EVENT_KILL(ev0030_smoke);
        EVENT_KILL(ev0030_smoke_2);
        EVENT_KILL(ev0030_sky);
        event_sound_stop_all();
        EVENT_KILL(ev0030_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0030_textures[] = {
    {"M_HEAD_1", &ev0030_head_texlist},
    {"M_TR_P", &ev0030_plane_texlist},
    {"EV_ECCLOUD", &event_clouds_texlist},
    {NULL, NULL},
};
