/* EV0008, Sonic's story: after the Egg Hornet falls, Eggman snatches the purple
 * Chaos Emerald from Tails. he gives it to Chaos 0, which grows into Chaos 1 */
#include <ninja.h>

#include "chaos/models.h"
#include "ev0008/eggmobile_action_7.h"
#include "ev0008/tails_action_3.h"
#include "ev0008/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "shared/chaos0.h"
#include "shared/chaos1.h"
#include "shared/claw.h"
#include "shared/claw_textures.h"
#include "shared/dust.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/rays.h"
#include "shared/shade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 34
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 20
#define TEXT_6_LENGTH 30
#define TEXT_7_LENGTH 21
#define TEXT_8_LENGTH 69
#define TEXT_9_LENGTH 60
#define TEXT_10_LENGTH 51
#define TEXT_11_LENGTH 58
#define TEXT_12_LENGTH 61
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 61
#define TEXT_15_LENGTH 15
#define TEXT_16_LENGTH 68
#define TEXT_17_LENGTH 52
#define TEXT_18_LENGTH 6
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 12
#include "ev0008/script/text.inc"

/* @hack type: sonic_texlist declared as an array, which script.c passes by its
 * name. SHC then keeps the Egg Mobile's x, 1045.3F, in FR12 (FMOV.S @R0,FR12 at
 * 0x0cb80082). as one NJS_TEXLIST passed as &sonic_texlist, FR12 takes its z,
 * 888.0F, ev0008_script runs 24 bytes long, and 2965 of its 3276 words
 * differ */
extern NJS_TEXLIST sonic_texlist[];
extern NJS_ACTION sonic_trot_action;
/* @hack type: tails_texlist declared as an array, which script.c passes by its
 * name. SHC then keeps 0xfff0 and 0x19e0, the x and y angles that Tails and the
 * emerald both start at, in R9 and R8 (0x0cb80296, 0x0cb8029c). as one
 * NJS_TEXLIST passed as &tails_texlist, 0xfff0 takes R8 and 0x19e0 R9, and 16
 * of ev0008_script's 3276 words differ */
extern NJS_TEXLIST tails_texlist[];
extern NJS_ACTION tails_run_action;
extern NJS_ACTION tails_stand_action;

/* @hack type: ev0008_sonic_action_0 declared as an array, which script.c passes
 * by its name. SHC then keeps the Egg Mobile's x, 1045.3F, in FR12 (FMOV.S
 * @R0,FR12 at 0x0cb80082). as one NJS_ACTION passed as &ev0008_sonic_action_0,
 * SHC loads the x straight into FR4 for event_eggmobile_create, ev0008_script
 * runs 8 bytes long, and 3081 of its 3276 words differ */
extern NJS_ACTION ev0008_sonic_action_0[];
extern NJS_ACTION ev0008_sonic_action_1;
extern NJS_ACTION ev0008_sonic_action_2;
extern NJS_ACTION ev0008_sonic_action_3;
extern NJS_ACTION ev0008_sonic_action_4;
extern NJS_ACTION ev0008_tails_action_0;
extern NJS_ACTION ev0008_tails_action_1;
extern NJS_ACTION ev0008_tails_action_2;
/* @hack type: ev0008_emerald_texlist declared as an array, which script.c
 * passes by its name. SHC then loads the Egg Mobile's z, 888.0F, straight into
 * FR6 (0x0cb80086). as one NJS_TEXLIST passed as &ev0008_emerald_texlist, SHC
 * keeps the z in FR13, ev0008_script runs 16 bytes long, and 3120 of its 3276
 * words differ */
extern NJS_TEXLIST ev0008_emerald_texlist[];
extern NJS_ACTION ev0008_emerald_action_0;
extern NJS_ACTION ev0008_emerald_action_1;
extern NJS_ACTION ev0008_emerald_action_2;
/* @hack type: ev0008_eggmobile_action_0 declared as an array, which script.c
 * passes by its name. SHC then loads Tails' run speed, 0.4F, straight into FR7
 * (0x0cb80580). as one NJS_ACTION passed as &ev0008_eggmobile_action_0, SHC
 * keeps the speed in FR13, ev0008_script runs 8 bytes long, and 2671 of its
 * 3276 words differ */
extern NJS_ACTION ev0008_eggmobile_action_0[];
/* @hack type: ev0008_eggmobile_action_1 declared as an array, which script.c
 * passes by its name. SHC then keeps 973.3F and 823.5F, the x and z Tails is
 * placed at, in FR13 and FR15 (0x0cb802bc, 0x0cb802c0). as one NJS_ACTION
 * passed as &ev0008_eggmobile_action_1, FR15 takes the x, SHC loads the z
 * straight into FR6, ev0008_script runs 24 bytes long, and 2955 of its 3276
 * words differ */
extern NJS_ACTION ev0008_eggmobile_action_1[];
extern NJS_ACTION ev0008_eggmobile_action_2;
extern NJS_ACTION ev0008_eggmobile_action_3;
extern NJS_ACTION ev0008_eggmobile_action_4;
extern NJS_ACTION ev0008_eggmobile_action_5;
extern NJS_ACTION ev0008_eggmobile_action_6;

/* @hack type: event_eggmobile_texlist declared as an array, which script.c
 * passes by its name. SHC then keeps the Egg Mobile's x, 1045.3F, in FR12
 * (FMOV.S @R0,FR12 at 0x0cb80082). as one NJS_TEXLIST passed as
 * &event_eggmobile_texlist, SHC loads the x straight into FR4 for
 * event_eggmobile_create, ev0008_script runs 4 bytes long, and 2904 of its 3276
 * words differ */
extern NJS_TEXLIST event_eggmobile_texlist[];
extern NJS_TEXLIST event_chaos0_water_texlist;
extern NJS_TEXLIST event_chaos1_texlist;

static struct task *ev0008_emerald = NULL;
static struct task *ev0008_flash = NULL;
/* ev0008_smoke_0, ev0008_smoke_1 and ev0008_smoke_2 hold dust tasks that smoke
 * around the Egg Mobile. the script never creates ev0008_smoke_0, and
 * event_set_pos does nothing with a null task */
static struct task *ev0008_smoke_0 = NULL;
static struct task *ev0008_smoke_1 = NULL;
/* no code uses this word. it keeps ev0008_smoke_2 at its address in the
 * image */
static struct task *dat_0cb89aa8 = NULL;
static struct task *ev0008_smoke_2 = NULL;
static struct task *ev0008_claw_0 = NULL;
static struct task *ev0008_claw_1 = NULL;
static struct task *ev0008_claw_2 = NULL;

/* bands of the screen, in pixels, that the shades darken as Eggman strikes */
static NJS_POINT2 ev0008_top_band[4] = {
#include "ev0008/script/ev0008_top_band.inc"
};
static NJS_POINT2 ev0008_middle_band[4] = {
#include "ev0008/script/ev0008_middle_band.inc"
};
static NJS_POINT2 ev0008_bottom_band[4] = {
#include "ev0008/script/ev0008_bottom_band.inc"
};

static struct task *ev0008_top_shade = NULL;
static struct task *ev0008_middle_shade = NULL;
static struct task *ev0008_bottom_shade = NULL;

void ev0008_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0008.mlt");
        event_sound_init(4);
        event_reset_player(0);
        event_eggmobile_create(1045.3F, 155.5F, 888.0F, 0, 0xf700, 0);
        event_wait(3);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev0008_eggmobile_action_7, event_eggmobile_texlist,
                          1.1F, EVENT_REPEAT, 0);
        if (ev0008_smoke_1 == NULL) {
            ev0008_smoke_1 = event_dust_create();
        }
        /* @hack spelling: the test and the creation as one || expression. SHC
         * then keeps the Egg Mobile's x, 1045.3F, in FR12 (FMOV.S @R0,FR12 at
         * 0x0cb80082). as an if, SHC loads the x straight into FR4 for
         * event_eggmobile_create, and 1632 of ev0008_script's 3276 words
         * differ */
        ev0008_smoke_2 != NULL || (ev0008_smoke_2 = event_dust_create());
        if (ev0008_smoke_1 != NULL) {
            DUST_WORK(ev0008_smoke_1)->params->scale = 4.0F;
        }
        if (ev0008_smoke_2 != NULL) {
            DUST_WORK(ev0008_smoke_2)->params->scale = 2.5F;
        }
        /* @hack spelling: the test and the color stores as one && expression.
         * SHC then keeps the Egg Mobile's x, 1045.3F, in FR12 (FMOV.S @R0,FR12
         * at 0x0cb80082). as an if, SHC loads the x straight into FR4 for
         * event_eggmobile_create. ev0008_script then runs 24 bytes long, and
         * 3136 of its 3276 words differ */
        ev0008_smoke_1 != NULL &&
            (DUST_WORK(ev0008_smoke_1)->params->color.a = 1.0F,
             DUST_WORK(ev0008_smoke_1)->params->color.r = 0.9F,
             DUST_WORK(ev0008_smoke_1)->params->color.g = 0.9F,
             DUST_WORK(ev0008_smoke_1)->params->color.b = 0.9F);
        /* @hack spelling: the test and the color stores as one && expression.
         * SHC then loads the Egg Mobile's z, 888.0F, straight into FR6
         * (0x0cb80086). as an if, SHC keeps the z in FR13 and loads the x
         * straight into FR4. ev0008_script then runs 16 bytes long, and 3066 of
         * its 3276 words differ */
        ev0008_smoke_2 != NULL &&
            (DUST_WORK(ev0008_smoke_2)->params->color.a = 1.0F,
             DUST_WORK(ev0008_smoke_2)->params->color.r = 0.65F,
             DUST_WORK(ev0008_smoke_2)->params->color.g = 0.65F,
             DUST_WORK(ev0008_smoke_2)->params->color.b = 0.65F);
        event_set_pos(ev0008_smoke_1, 1054.3F, 144.5F, 871.0F);
        event_set_pos(ev0008_smoke_2, 1046.3F, 147.5F, 898.5F);
        if (ev0008_smoke_1 != NULL) {
            DUST_WORK(ev0008_smoke_1)->timer =
                DUST_WORK(ev0008_smoke_1)->interval = 4;
        }
        if (ev0008_smoke_2 != NULL) {
            DUST_WORK(ev0008_smoke_2)->timer =
                DUST_WORK(ev0008_smoke_2)->interval = 5;
        }
        event_set_pos(event_get_player(0), 981.0F, 126.4F, 868.8F);
        event_set_ang(event_get_player(0), 0x18, 0x1b00, 0x3e);
        event_create_player(2, tails_step, 959.5F, 126.5F, 794.93F, 0xfff0,
                            0x19e0, 0x54);
        event_wait(1);
        event_set_pos(event_get_player(2), 973.3F, 126.5F, 823.5F);
        event_set_ang(event_get_player(2), 0x18, 0x0b00, 0x54);
        event_place_actor(&ev0008_emerald, 973.3F, 126.5F, 823.5F, 0xfff0,
                          0x19e0, 0x54);
        event_wait(1);
        event_set_flags(ev0008_emerald, EVENT_IGNORE_GROUND);
        event_eggmobile_fly_turn(1045.3F, 135.5F, 888.0F, 0x0200, 0x5000,
                                 0x2300, 140, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1025.5F, 147.77F, 871.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0xb300, 0xf700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 998.4F, 142.7F, 883.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xfb00, 0xbe00, 0xfc00);
        event_sound_start(3, 1092, 1800);
        event_sound_fade_volume(3, 115, 1);
        event_wait(70);
        event_set_pos(ev0008_smoke_0, 1030.3F, 129.5F, 874.6F);
        event_set_pos(ev0008_smoke_1, 1030.3F, 129.5F, 874.6F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, 957.4F, 129.7F, 877.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x0800, 0xc100, 0xfe00);
        event_sound_play(25, -20, 0, 0);
        event_wait(110);
        event_sound_play(25, -20, 0, 0);
        event_sound_fade_volume(3, 80, 1);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(5);
        event_wait(5);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 957.4F, 127.7F, 872.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x0c00, 0xc000, 0xfe00);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 989.97F, 129.6F, 876.95F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x081c, 0x1e94, 0xfce0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 988.4F, 130.2F, 873.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x061c, 0x1094, 0xfd00);
        event_set_pos(event_get_player(2), 973.3F, 126.5F, 823.5F);
        event_set_ang(event_get_player(2), 0x18, 0x0b00, 0x54);
        event_set_pos(ev0008_emerald, 973.3F, 126.5F, 823.5F);
        event_set_ang(ev0008_emerald, 0xfff0, 0x19e0, 0x54);
        event_play_action(ev0008_emerald, &ev0008_emerald_action_1,
                          ev0008_emerald_texlist, 1.0F, EVENT_REPEAT, 1);
        event_play_action(event_get_player(2), &ev0008_tails_action_1,
                          tails_texlist, 1.0F, EVENT_REPEAT, 1);
        event_run_to_speed(event_get_player(2), 983.8F, 126.5F, 848.7F, 0.4F,
                           0.06F);
        event_wait(1);
        event_follow(ev0008_emerald, 0.0F, 0.0F, 0.0F, 160,
                     event_get_player(2));
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_18);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_19);
        event_voice_play(975);
        event_message_show(TEXT_1);
        event_play_action(event_get_player(0), &ev0008_sonic_action_2,
                          sonic_texlist, 0.5F, 0, 4);
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_wait(20);
        event_talk_stop(event_get_player(0));
        event_clear_motions(event_eggmobile_get_task());
        event_message_close();
        /* EVENT_KILL in place of this if makes ev0008_script 4 bytes longer,
         * and 2854 of its 3276 words differ */
        if (ev0008_smoke_1 != NULL) {
            task_kill(ev0008_smoke_1);
            ev0008_smoke_1 = NULL;
        }
        EVENT_KILL(ev0008_smoke_2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1035.7F, 144.1F, 889.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe813, 0x0a90, 0xf1c1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 20, 1036.2F, 143.5F, 890.17F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 20, 0xf513, 0x0990, 0xf2c1);
        event_wait(25);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 1036.15F, 141.7F, 889.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfa13, 0x1890, 0xecc1);
        if (ev0008_top_shade == NULL) {
            ev0008_top_shade = event_shade_create(0, ev0008_top_band, 4);
        }
        if (ev0008_top_shade != NULL) {
            event_shade_fade(ev0008_top_shade, 117, 6);
        }
        /* @hack spelling: the test and the creation as one || expression. SHC
         * then keeps 0xfff0 and 0x19e0, the x and y angles that Tails and the
         * emerald both start at, in R9 and R8 (0x0cb80296, 0x0cb8029c). as an
         * if, 0xfff0 takes R8 and 0x19e0 R9, ev0008_script runs 8 bytes long,
         * and 2687 of its 3276 words differ */
        ev0008_middle_shade != NULL ||
            (ev0008_middle_shade =
                 event_shade_create(0, ev0008_middle_band, 4));
        if (ev0008_middle_shade != NULL) {
            event_shade_fade(ev0008_middle_shade, 128, 6);
        }
        /* @hack spelling: the test and the creation as one || expression. SHC
         * then keeps 0xfff0 and 0x19e0, the x and y angles that Tails and the
         * emerald both start at, in R9 and R8 (0x0cb80296, 0x0cb8029c). as an
         * if, 0xfff0 takes R8 and 0x19e0 R9, ev0008_script runs 8 bytes long,
         * and 2687 of its 3276 words differ */
        ev0008_bottom_shade != NULL ||
            (ev0008_bottom_shade =
                 event_shade_create(0, ev0008_bottom_band, 4));
        if (ev0008_bottom_shade != NULL) {
            event_shade_fade(ev0008_bottom_shade, 128, 6);
        }
        event_message_show(TEXT_2);
        event_sound_play(1223, 128, 0, 0);
        event_voice_play(976);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 2, 1033.7F, 139.95F, 891.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 2, 0xfa13, 0x1890, 0xecc1);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 10, 989.7F, 131.9F, 858.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 10, 0xfc13, 0x1390, 0x08c1);
        if (ev0008_top_shade != NULL) {
            event_shade_fade(ev0008_top_shade, 0, 16);
        }
        if (ev0008_middle_shade != NULL) {
            event_shade_fade(ev0008_middle_shade, 0, 16);
        }
        if (ev0008_bottom_shade != NULL) {
            event_shade_fade(ev0008_bottom_shade, 0, 16);
        }
        event_wait(14);
        EVENT_KILL(ev0008_top_shade);
        EVENT_KILL(ev0008_middle_shade);
        EVENT_KILL(ev0008_bottom_shade);
        stream_request_music(MUSIC_EGGMAN);
        event_sound_start(1, 1224, 1800);
        event_sound_fade_volume(1, 110, 1);
        event_sound_fade_pan(1, 20, 1);
        ev0008_claw_0 =
            event_claw_create(1046.225F, 134.56F, 885.2F, 0, 0, 0xa000);
        ev0008_claw_1 =
            event_claw_create(1066.73F, 114.058F, 885.2F, 0, 0, 0x2000);
        event_claw_set_rates(ev0008_claw_0, 0x1200, 0x1200);
        event_claw_set_rates(ev0008_claw_1, 0x1200, 0x1200);
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.4F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1031.1F, 132.9F, 900.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1813, 0xe790, 0xfac1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 1031.51F, 129.9F, 897.38F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x3413, 0x0190, 0xecc1);
        event_move(ev0008_claw_0, 1046.225F, 134.56F, 885.2F, 1006.39F, 174.5F,
                   885.2F, 25);
        event_move(ev0008_claw_1, 1066.73F, 114.058F, 885.2F, 1046.225F,
                   134.56F, 885.2F, 25);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(1);
        event_set_pos(ev0008_emerald, 983.8F, 132.5F, 848.7F);
        event_set_ang(ev0008_emerald, 0x18, 0x3000, 0x3e);
        ev0008_claw_2 =
            event_claw_create(990.0F, 138.0F, 866.0F, 0x1300, 0x5000, 0xf000);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 990.0F, 138.0F, 866.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1300, 0x5000, 0xf000);
        event_move(ev0008_claw_2, 990.0F, 138.0F, 866.0F, 988.0F, 128.0F,
                   856.7F, 7);
        event_sound_fade_volume(1, 100, 1);
        event_sound_fade_pan(1, -20, 1);
        event_sound_fade_pitch(1, -1214, 1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 990.64F, 130.66F, 834.57F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x08ad, 0x9a5c, 0xf5c1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 984.67F, 127.18F, 835.78F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x0bad, 0x905c, 0xf4c1);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(3);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(3);
        event_sound_play(1225, 118, 0, 0);
        vibration_start(0, 2, 60, 2);
        event_play_action(event_get_player(0), &ev0008_sonic_action_1,
                          sonic_texlist, 0.5F, EVENT_REPEAT, 1);
        event_play_action(event_get_player(2), &ev0008_tails_action_0,
                          tails_texlist, 1.2F, 0, 1);
        event_play_action(event_get_player(2), &ev0008_tails_action_2,
                          tails_texlist, 1.2F, 0, 1);
        event_play_action(event_get_player(2), &ev0008_tails_action_3,
                          tails_texlist, 1.2F, EVENT_REPEAT, 1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 15, 974.3F, 128.1F, 838.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 15, 0x0813, 0x9e90, 0xebc1);
        event_sound_fade_volume(1, 95, 1);
        event_sound_fade_pan(1, -20, 1);
        event_sound_fade_pitch(1, 1214, 1);
        event_wait(1);
        event_sound_fade_pitch(1, -1214, 110);
        event_play_action(event_get_player(0), &ev0008_sonic_action_3,
                          sonic_texlist, 0.4F, 0, 4);
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.4F, EVENT_REPEAT, 4);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(2);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(2);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(2);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(2);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(2);
        event_claw_set_mode(ev0008_claw_0, CLAW_OPEN);
        event_wait(2);
        event_claw_set_mode(ev0008_claw_0, CLAW_CLOSE);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 20, 962.68F, 127.7F, 850.55F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 20, 0x0a13, 0xb490, 0x07c1);
        event_voice_play(977);
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.4F, EVENT_REPEAT, 4);
        event_clear_motions(ev0008_emerald);
        event_play_action(ev0008_emerald, &ev0008_emerald_action_0,
                          ev0008_emerald_texlist, 1.0F, EVENT_REPEAT, 1);
        event_move(ev0008_claw_2, 987.5F, 131.8F, 857.9F, 998.58F, 170.42F,
                   887.77F, 40);
        event_move(ev0008_emerald, 984.0F, 136.8F, 851.0F, 995.0F, 172.0F,
                   882.78F, 40);
        event_voice_wait();
        event_follow(ev0008_emerald, -5.0F, 16.0F, 1.0F, 50, ev0008_claw_0);
        event_move(ev0008_claw_0, 1006.39F, 174.5F, 885.2F, 1046.225F, 134.56F,
                   885.2F, 45);
        event_move(ev0008_claw_1, 1046.225F, 134.56F, 885.2F, 1066.73F,
                   114.058F, 885.2F, 45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 957.52F, 128.16F, 868.05F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0a13, 0xc590, 0x07c1);
        event_wait(50);
        event_message_close();
        EVENT_KILL(ev0008_claw_0);
        EVENT_KILL(ev0008_claw_1);
        event_move_turn(ev0008_claw_2, 1055.24F, 144.058F, 886.63F, 1055.24F,
                        115.058F, 886.63F, 0, 0x5000, 0x8000, 0, 0xd000, 0x8000,
                        120);
        event_play_action(event_eggmobile_get_task(),
                          &ev0008_eggmobile_action_3, event_eggmobile_texlist,
                          1.1F, EVENT_REPEAT, 0);
        event_play_action(ev0008_emerald, &ev0008_emerald_action_2,
                          ev0008_emerald_texlist, 1.1F, EVENT_REPEAT, 0);
        event_wait(2);
        event_set_ang(event_get_player(0), 0x18, 0x3400, 0x3e);
        event_eggmobile_fly_turn(1045.3F, 135.5F, 888.0F, -0x0200, -0x8000,
                                 -0x2300, 1, 1);
        event_wait(2);
        event_set_pos(ev0008_emerald, 1052.5F, 123.6F, 873.3F);
        event_set_ang(ev0008_emerald, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1046.0F, 156.17F, 875.55F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x04fd, 0x98ff, 0x00c1);
        event_sound_fade_volume(1, 70, 1);
        event_sound_fade_pan(1, -20, 1);
        event_sound_fade_pitch(1, -1440, 1);
        event_wait(1);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 1046.87F, 148.23F,
                             876.48F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x04f9, 0x8bff, 0x00c1);
        event_wait(30);
        event_sound_play(1226, 128, 0, 0);
        event_wait(100);
        EVENT_KILL(ev0008_claw_2);
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.8F, EVENT_REPEAT, 8);
        event_sound_stop(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1046.23F, 143.45F, 881.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0dfd, 0x6fff, 0x00c1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 1046.12F, 143.79F, 881.98F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0dfd, 0x7aff, 0x00c1);
        event_voice_play(978);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_wait(20);
        event_chaos0_create(1091.2F, 121.5F, 866.8F, 0, 0x0800, 0, 80);
        event_wait(1);
        event_chaos0_puddle();
        event_chaos0_slide(1096.2F, 126.3F, 864.8F, 60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 971.8F, 128.3F, 856.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0ab9, 0xa9aa, 0x02c1);
        event_voice_play(979);
        event_message_show(TEXT_4);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, 970.8F, 128.7F, 860.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x08b9, 0xb7aa, 0x03c1);
        event_wait(50);
        event_message_close();
        event_chaos0_slide(1050.0F, 126.3F, 857.8F, 90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1035.2F, 157.3F, 896.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe821, 0xdf0c, 0xffc1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 70, 1026.2F, 157.6F, 883.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 70, 0xe82c, 0xda0c, 0xffc1);
        event_wait(70);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1040.2F, 128.0F, 849.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x06b9, 0x9eaa, 0xf5c1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, 1037.66F, 134.47F,
                             848.68F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 110, 0x0fb9, 0x9aaa, 0xf5c1);
        event_sound_fade_volume(2, 5, 70);
        event_chaos0_rise();
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 975.2F, 130.5F, 860.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x04b9, 0xbcaa, 0xfec1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 12, 968.66F, 129.5F, 859.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 12, 0x07b9, 0xb7aa, 0xfec1);
        event_voice_play(980);
        event_message_show(TEXT_5);
        event_wait(40);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1032.2F, 133.5F, 844.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x03b9, 0x9baa, 0xfec1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 1006.66F, 134.9F, 859.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x03b9, 0xbeaa, 0xfec1);
        event_play_action(event_eggmobile_get_task(),
                          &ev0008_eggmobile_action_4, event_eggmobile_texlist,
                          1.0F, 0, 8);
        event_move_turn(ev0008_emerald, 1052.5F, 123.6F, 873.3F, 1070.5F,
                        130.7F, 847.27F, 0, 0, 0, 0, 0x4000, 0x3000, 55);
        event_wait(50);
        event_rays_create(1051.2F, 135.5F, 857.8F, 3, 10, 0.4F, 2.0F,
                          0xffff64ff, 0x00ff1eff);
        event_wait(8);
        event_sound_stop(2);
        event_kill_task(&ev0008_emerald);
        event_chaos0_melt();
        event_sound_play(1228, 128, 0, 0);
        ev0008_flash = event_fade_create(0.04F, 0.04F, 1.0F, 0.85F, 1.0F);
        event_wait(20);
        event_chaos0_kill();
        event_chaos1_create(1050.0F, 126.3F, 857.8F, 0, 0x0800, 0, 80);
        event_wait(1);
        event_chaos1_puddle();
        event_wait(1);
        event_chaos1_rise();
        event_wait(1);
        event_chaos1_set_speed(0.3F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1039.2F, 126.0F, 867.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1ab9, 0xd6aa, 0xfec1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 1035.66F, 130.4F, 865.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x21b9, 0xd3aa, 0xfec1);
        event_sound_start(2, 1227, 1800);
        event_sound_fade_volume(2, -25, 1);
        event_wait(30);
        event_rays_kill();
        if (ev0008_flash != NULL) {
            event_fade_set_speed(ev0008_flash, -0.02F);
        }
        event_wait(110);
        EVENT_KILL(ev0008_flash);
        event_play_action(event_eggmobile_get_task(),
                          &ev0008_eggmobile_action_5, event_eggmobile_texlist,
                          1.0F, 0, 8);
        event_play_action(event_eggmobile_get_task(), ev0008_eggmobile_action_1,
                          event_eggmobile_texlist, 1.2F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1042.9F, 144.6F, 880.84F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x06b9, 0x8faa, 0xfec1);
        event_voice_play(981);
        event_message_show(TEXT_6);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 1042.9F, 144.6F, 880.84F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0x0cb9, 0x8faa, 0xfec1);
        event_wait(60);
        event_message_close();
        event_eggmobile_fly_turn(1045.3F, 135.5F, 888.0F, 0, 0x4300, 0, 80, 1);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 961.2F, 132.5F, 848.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x02b9, 0xabaa, 0x00c1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 957.66F, 133.4F, 865.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x02b9, 0xb9aa, 0x00c1);
        event_voice_play(982);
        event_play_action(event_eggmobile_get_task(), ev0008_eggmobile_action_1,
                          event_eggmobile_texlist, 0.75F, EVENT_REPEAT, 0);
        event_message_show(TEXT_7);
        event_wait(80);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 1036.2F, 156.68F, 889.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe9b9, 0xee4a, 0x00c1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 1040.26F, 154.2F, 892.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xe9b9, 0xea4a, 0x00c1);
        event_voice_play(983);
        event_message_show(TEXT_8);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1036.2F, 128.5F, 853.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1fb9, 0xb3aa, 0xf2c1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 1042.0F, 127.4F, 865.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0x23b9, 0xe7aa, 0xf63e);
        event_voice_play(984);
        event_message_show(TEXT_9);
        event_voice_wait();
        event_voice_play(985);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(2);
        /* @hack type: the array's address, cast, in place of the array. SHC
         * then keeps 0xfff0 and 0x19e0, the x and y angles that Tails and the
         * emerald both start at, in R9 and R8 (0x0cb80296, 0x0cb8029c). as
         * ev0008_eggmobile_action_0 passed by its name, 0xfff0 takes R8 and
         * 0x19e0 R9, and 8 of ev0008_script's 3276 words differ */
        event_play_action(event_eggmobile_get_task(),
                          (NJS_ACTION *)&ev0008_eggmobile_action_0,
                          event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 0);
        event_message_show(TEXT_10);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1039.7F, 156.6F, 894.56F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe6b9, 0xe0aa, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 1036.8F, 147.18F, 880.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0cb9, 0x9daa, 0);
        event_voice_play(986);
        event_message_show(TEXT_11);
        event_play_action(event_eggmobile_get_task(),
                          &ev0008_eggmobile_action_2, event_eggmobile_texlist,
                          0.9F, 0, 8);
        event_play_action(event_eggmobile_get_task(), ev0008_eggmobile_action_1,
                          event_eggmobile_texlist, 1.0F, EVENT_REPEAT, 8);
        event_voice_wait();
        event_play_action(event_eggmobile_get_task(), ev0008_eggmobile_action_0,
                          event_eggmobile_texlist, 1.3F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1009.0F, 133.4F, 851.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x05b9, 0xadaa, 0xfc00);
        event_voice_play(987);
        event_message_show(TEXT_12);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x05b9, 0xa8aa, 0xfc00);
        event_wait(30);
        event_eggmobile_fly_turn(1045.3F, 135.5F, 888.0F, 0, -0x2000, 0, 60, 0);
        event_wait(3);
        event_voice_wait();
        event_message_close();
        event_wait(3);
        event_clear_motions(event_eggmobile_get_task());
        event_play_action(event_eggmobile_get_task(),
                          &ev0008_eggmobile_action_6, event_eggmobile_texlist,
                          1.0F, 0, 0);
        event_wait(8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1047.0F, 160.48F, 895.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xddb9, 0x0eaa, 0xfc00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 1055.22F, 161.037F,
                             890.36F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0xe5b9, 0x2baa, 0xfc00);
        event_wait(30);
        event_play_action(event_eggmobile_get_task(), ev0008_eggmobile_action_0,
                          event_eggmobile_texlist, 1.3F, EVENT_REPEAT, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1004.0F, 129.4F, 873.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfbb9, 0xbfaa, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 972.6F, 132.7F, 873.1F);
        event_rays_create(1030.2F, 130.86F, 873.8F, 4, 6, 0.8F, 2.5F,
                          0xffffff96, 0x00ffffd2);
        event_play_action(event_get_player(0), &ev0008_sonic_action_4,
                          sonic_texlist, 0.7F, 0, 0);
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.9F, EVENT_REPEAT, 0);
        ev0008_flash = event_fade_create(0.1F, 0.05F, 1.0F, 1.0F, 0.9F);
        event_wait(5);
        event_sound_play(1229, 120, 0, 0);
        event_wait(30);
        event_rays_kill();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 966.54F, 129.0F, 875.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x07b9, 0xc2aa, 0xfd00);
        event_wait(20);
        event_eggmobile_kill();
        event_chaos1_kill();
        /* @hack spelling: the test and the call as one conditional expression.
         * SHC then keeps the Egg Mobile's x, 1045.3F, in FR12 (FMOV.S @R0,FR12
         * at 0x0cb80082). as an if, FR13 takes the x. ev0008_script then runs 8
         * bytes long, and 2687 of its 3276 words differ */
        ev0008_flash != NULL ? event_fade_set_speed(ev0008_flash, -0.04F)
                             : (void)0;
        event_wait(50);
        /* EVENT_KILL in place of this if makes ev0008_script 4 bytes longer,
         * and 2595 of its 3276 words differ */
        if (ev0008_flash != NULL) {
            task_kill(ev0008_flash);
            ev0008_flash = NULL;
        }
        event_wait(10);
        event_sound_stop(2);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 970.6F, 155.4F, 855.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0309, 0xb6aa, 0);
        event_set_pos(event_get_player(2), 968.0F, 126.6F, 858.0F);
        event_set_ang(event_get_player(2), 0, 0x4600, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 110, 967.0F, 147.5F, 855.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 110, 0xe209, 0xb5aa, 0x0400);
        event_play_action(event_get_player(0), &sonic_trot_action,
                          sonic_texlist, 1.0F, EVENT_REPEAT, 1);
        event_play_action(event_get_player(2), &tails_run_action, tails_texlist,
                          1.0F, EVENT_REPEAT, 1);
        event_run_to_speed(event_get_player(0), 1008.2F, 126.6F, 879.1F, 1.0F,
                           0.1F);
        event_run_to_speed(event_get_player(2), 997.0F, 126.6F, 857.94F, 1.0F,
                           0.1F);
        event_run_wait(event_get_player(0));
        event_run_wait(event_get_player(2));
        event_play_action(event_get_player(0), ev0008_sonic_action_0,
                          sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_play_action(event_get_player(2), &tails_stand_action,
                          tails_texlist, 1.0F, EVENT_REPEAT, 1);
        event_wait(20);
        event_set_ang(event_get_player(0), 0, 0x2900, 0);
        event_set_pos(event_get_player(2), 1013.0F, 126.6F, 862.5F);
        event_set_ang(event_get_player(2), 0, 0xfd00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1023.4F, 129.5F, 859.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x09b9, 0x51aa, 0x0300);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_20);
        event_voice_play(988);
        event_message_show(TEXT_13);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 1024.2F, 129.35F, 860.94F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 80, 0x08b9, 0x50aa, 0x0300);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_21);
        event_voice_play(989);
        event_message_show(TEXT_14);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_22);
        event_voice_play(990);
        event_message_show(TEXT_15);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_set_ang(event_get_player(0), 0, 0x1900, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1013.9F, 129.0F, 888.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x15b9, 0x12aa, 0xfe00);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x0bb9, 0x12aa, 0xfe00);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_23);
        event_voice_play(991);
        event_message_show(TEXT_16);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_24);
        event_voice_play(992);
        event_message_show(TEXT_17);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_wait(40);
        event_move_stop_all();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop(2);
        event_sound_stop_all();
        event_reset_player(0);
        event_move_stop_all();
        event_kill_player(2);
        event_eggmobile_kill();
        event_kill_task(&ev0008_emerald);
        event_chaos0_kill();
        event_chaos1_kill();
        event_rays_kill();
        EVENT_KILL(ev0008_flash);
        event_voice_stop();
        event_set_pos(event_get_player(0), 913.0F, 126.2F, 951.2F);
        event_set_ang(event_get_player(0), 0, 0x4c00, 0);
        EVENT_KILL(ev0008_claw_0);
        EVENT_KILL(ev0008_claw_1);
        EVENT_KILL(ev0008_claw_2);
        EVENT_KILL(ev0008_smoke_1);
        EVENT_KILL(ev0008_smoke_2);
        EVENT_KILL(ev0008_top_shade);
        EVENT_KILL(ev0008_middle_shade);
        EVENT_KILL(ev0008_bottom_shade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0008_textures[] = {
    {"M_EM_PURPLE", ev0008_emerald_texlist},
    {"EV_EGGMOBLE0", event_eggmobile_texlist},
    {"MGHAND", &event_claw_texlist},
    {"MILES", tails_texlist},
    {"CHAOS0", &event_chaos0_texlist},
    {"EV_CHAOS1", &event_chaos1_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};
