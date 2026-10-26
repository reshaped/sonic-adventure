/* EV00FE: after the last fight, Chaos sees the Chao at peace and leaves with
 * Tikal, and Sonic sets off after Eggman */
#include <ninja.h>

#include "ev00fe/bird_action.h"
#include "ev00fe/bird_textures.h"
#include "ev00fe/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/big.h"
#include "player/big_models.h"
#include "player/knuckles.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/textures.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "shared/chaos0.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/floor.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/white.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 34
#define TEXT_2_LENGTH 59
#define TEXT_3_LENGTH 32
#define TEXT_4_LENGTH 66
#define TEXT_5_LENGTH 29
#define TEXT_6_LENGTH 15
#define TEXT_7_LENGTH 34
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 3
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 5
#include "ev00fe/script/text.inc"

/* set the speed of the fade in slot, unless slot is NULL */
/* @hack spelling: the test and the call as one expression, as
 * src/event/play.h's EVENT_KILL has them. as an if, ev00fe_script runs 8 bytes
 * short, and 2989 of 3498 words differ */
#define SET_SPEED(slot, speed)                                                 \
    ((slot) != NULL && (event_fade_set_speed((slot), (speed)), 0))

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION tails_stand_action;
extern NJS_ACTION knuckles_stand_action;
extern NJS_ACTION amy_stand_action;

/* an action of Tikal's, in TIKAL_PROG.PRS */
extern NJS_ACTION dat_0cb2d478;

extern NJS_TEXLIST event_eggmobile_texlist;

extern NJS_ACTION ev00fe_chao_action_0;
extern NJS_ACTION ev00fe_chao_action_1;
extern NJS_ACTION ev00fe_chao_action_2;
extern NJS_ACTION ev00fe_chao_action_3;
extern NJS_ACTION ev00fe_tikal_action_0;
extern NJS_ACTION ev00fe_tikal_action_1;
extern NJS_ACTION ev00fe_tikal_action_2;
extern NJS_ACTION ev00fe_tails_action;

/* port 0's keys for Sonic's two jumps, one key a step. A is held for 8 steps
 * and released for 8, then held for 8 and released for 37 */
static const struct event_pad_key ev00fe_jump_keys[61] = {
#include "ev00fe/script/ev00fe_jump_keys.inc"
};

static const struct event_pad_recording ev00fe_jumps = {
    ev00fe_jump_keys, sizeof(ev00fe_jump_keys) / sizeof(ev00fe_jump_keys[0])};

static struct task *ev00fe_chao_0 = NULL;
static struct task *ev00fe_chao_1 = NULL;
static struct task *ev00fe_chao_2 = NULL;
static struct task *ev00fe_chao_3 = NULL;
static struct task *ev00fe_chao_4 = NULL;
static struct task *ev00fe_chao_5 = NULL;
static struct task *ev00fe_chao_6 = NULL;
/* Amy's bird */
static struct task *ev00fe_bird = NULL;
static struct task *ev00fe_sonic_floor_0 = NULL;
static struct task *ev00fe_tails_floor_0 = NULL;
static struct task *ev00fe_knuckles_floor_0 = NULL;
static struct task *ev00fe_amy_floor = NULL;
static struct task *ev00fe_big_floor = NULL;
static struct task *ev00fe_tikal_floor = NULL;
static struct task *ev00fe_sonic_floor_1 = NULL;
static struct task *ev00fe_tails_floor_1 = NULL;
static struct task *ev00fe_knuckles_floor_1 = NULL;
static struct task *ev00fe_sonic_floor_2 = NULL;
static struct task *ev00fe_sonic_floor_3 = NULL;
static struct task *ev00fe_fade_out = NULL;
/* the white fade as Tikal and Chaos leave. it grows mostly before they rise
 * away, and clears as they go */
static struct task *ev00fe_white = NULL;
/* the black fade the event opens on, which clears over 180 steps */
static struct task *ev00fe_fade_in = NULL;

struct texture_pvm ev00fe_textures[] = {
    {NULL, NULL},
};

static struct texture_pvm ev00fe_cast_textures[10];

void ev00fe_script(int phase)
{
    struct texture_pvm *pvm;
    /* @hack spelling: sound effect 1224 through a local, which the image
     * keeps in R11 for its 57 plays (0x0cb80764); without it, ev00fe_script
     * runs 36 bytes long and 3240 of 3498 words differ */
    int sound;
    /* @hack spelling: ev00fe_chao_action_3 through a local, which loads its
     * address into R12 once for the seven Chao (0x0cb8058a); without it, the
     * function runs 4 bytes short and 2908 of 3498 words differ */
    NJS_ACTION *chao_action;

    switch (phase) {
    case EVENT_LOAD:
        /* eff_regular_texlist stays released until EVENT_UNLOAD loads it
         * again. ev00fe_cast_textures load here, since ev00fe_textures is
         * empty */
        njReleaseTexture(&eff_regular_texlist);
        hud_hidden = -1;
        for (pvm = ev00fe_cast_textures; pvm->texlist != NULL; pvm++) {
            if (pvm->name != NULL) {
                texture_load_pvm(pvm->name, pvm->texlist);
            } else {
                njLoadTexture(pvm->texlist);
            }
        }
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        ev00fe_fade_in = event_fade_create(1.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        /* @hack redundant: a call of event_fade_set_speed that never runs, and
         * an empty else. SHC writes no code for either. it still counts the
         * call's 1.0F when it allocates registers, which gives FR14 to 1.0F
         * (0x0cb800cc) and FR13 to the image's other floats, such as 0.5F
         * (0x0cb803ac). with the call, the else puts player_get's address in
         * R3, not R2 (0x0cb800e0); without the else, 2 of 3498 words differ.
         * without the call, whether or not the else stays, FR13 and FR14 trade
         * throughout and 43 of 3498 words differ */
        if (0) {
            event_fade_set_speed(NULL, 1.0F);
        } else {
        }
        event_wait(1);
        player_get(0)->upgrades &= ~PLAYER_SONIC_SUPER;
        event_reset_player(0);
        sound_load_file("e_00fe.mlt");
        event_sound_init(1);
        event_set_pos(event_get_player(0), 696.79F, 156.0F, -762.0F);
        event_set_ang(event_get_player(0), 0, 0x6754, 0);
        event_wait(1);
        event_create_player(2, tikal_player_step, 1015.0F, 84.0F, -825.0F, 0,
                            0xe500, 0);
        event_create_player(4, tails_step, 703.0F, 156.0F, -755.0F, 0, 0x67c7,
                            0);
        event_create_player(5, knuckles_step, 689.16F, 156.0F, -770.25F, 0,
                            0x6754, 0);
        event_chaos0_create(720.0F, 96.0F, -840.0F, 0, 0xa000, 0, 60);
        event_place_task(&ev00fe_chao_0, event_chao_create, 740.0F, 101.3F,
                         -1030.57F, 0, 0x4000, 0);
        event_place_task(&ev00fe_chao_1, event_chao_create, 732.0F, 101.3F,
                         -1035.0F, 0, 0x4000, 0);
        event_place_task(&ev00fe_chao_2, event_chao_create, 745.0F, 101.3F,
                         -1040.57F, 0, 0x4000, 0);
        event_place_task(&ev00fe_chao_3, event_chao_create, 848.67F, 101.3F,
                         -788.0F, 0, 0xe800, 0);
        event_place_task(&ev00fe_chao_4, event_chao_create, 856.0F, 101.3F,
                         -781.0F, 0, 0xe800, 0);
        event_place_task(&ev00fe_chao_5, event_chao_create, 861.67F, 100.3F,
                         -775.0F, 0, 0xe800, 0);
        event_place_task(&ev00fe_chao_6, event_chao_create, 869.67F, 100.3F,
                         -795.0F, 0, 0xe800, 0);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        ev00fe_sonic_floor_0 = event_floor_create(event_get_player(0), 156.0F);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        ev00fe_tails_floor_0 = event_floor_create(event_get_player(4), 156.0F);
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(5), &knuckles_stand_action,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        ev00fe_knuckles_floor_0 =
            event_floor_create(event_get_player(5), 156.0F);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &dat_0cb2d478, &tikal_texlist,
                          0.5F, EVENT_REPEAT, 0);
        event_chaos0_set_scale(0.6F, 0.6F, 0.6F);
        event_wait(1);
        event_chaos0_play7();
        event_set_flags(ev00fe_chao_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fe_chao_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fe_chao_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fe_chao_3, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fe_chao_4, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fe_chao_5, EVENT_IGNORE_GROUND);
        event_set_flags(ev00fe_chao_6, EVENT_IGNORE_GROUND);
        event_look_at_task(event_get_player(0), event_chaos0_get_task(), 0.0F,
                           50.0F, 0.0F);
        event_look_at_task(event_get_player(4), event_chaos0_get_task(), 0.0F,
                           50.0F, 0.0F);
        event_look_at_task(event_get_player(5), event_chaos0_get_task(), 0.0F,
                           50.0F, 0.0F);
        stream_request_music(MUSIC_EVTBGM00);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x2aab);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 758.24F, 245.88F, -843.28F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0500, 0x3f00, 0);
        event_wait(45);
        SET_SPEED(ev00fe_fade_in, -1.0F / 180.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, 740.74F, 103.95F, -843.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0x0500, 0x3f00, 0);
        chao_action = &ev00fe_chao_action_3;
        event_play_action(ev00fe_chao_0, chao_action, &event_chao_texlist, 0.5F,
                          EVENT_REPEAT, 0);
        event_wait(8);
        event_play_action(ev00fe_chao_1, chao_action, &event_chao_texlist, 0.5F,
                          EVENT_REPEAT, 0);
        event_wait(8);
        event_play_action(ev00fe_chao_2, chao_action, &event_chao_texlist, 0.5F,
                          EVENT_REPEAT, 0);
        event_wait(8);
        event_play_action(ev00fe_chao_3, chao_action, &event_chao_texlist, 0.5F,
                          EVENT_REPEAT, 0);
        event_wait(8);
        event_play_action(ev00fe_chao_4, chao_action, &event_chao_texlist, 0.5F,
                          EVENT_REPEAT, 0);
        event_wait(8);
        event_play_action(ev00fe_chao_5, chao_action, &event_chao_texlist, 0.5F,
                          EVENT_REPEAT, 0);
        event_wait(8);
        event_play_action(ev00fe_chao_6, chao_action, &event_chao_texlist, 0.5F,
                          EVENT_REPEAT, 0);
        event_wait(8);
        event_wait(140);
        EVENT_KILL(ev00fe_fade_in);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 743.78F, 106.38F, -812.77F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x1d00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 726.15F, 106.38F,
                             -807.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0, 0x0414, 0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 725.15F, 106.38F, -807.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0, 0xff14, 0);
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 738.95F, 103.38F, -978.02F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0, 0x0400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 746.9F, 103.38F, -978.9F);
        event_wait(5);
        sound = 1224;
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(15);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(5);
        event_sound_play(sound, 100, 0, 0);
        event_set_ang(event_chaos0_get_task(), 0, 0x7900, 0);
        event_chaos0_play9();
        event_wait(45);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 706.84F, 107.95F, -845.32F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0xad00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 709.09F, 107.95F, -849.78F);
        event_sound_play(sound, 50, 0, 0);
        event_wait(10);
        event_sound_play(sound, 0, 0, 0);
        event_wait(15);
        event_sound_play(sound, 0, 0, 0);
        event_wait(10);
        event_sound_play(sound, 0, 0, 0);
        event_wait(20);
        event_sound_play(sound, 0, 0, 0);
        event_wait(15);
        event_sound_play(sound, 0, 0, 0);
        event_wait(10);
        event_sound_play(sound, 0, 0, 0);
        event_wait(10);
        event_sound_play(sound, 50, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 809.26F, 101.56F, -802.55F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0200, 0xac00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, 817.29F, 101.56F,
                             -817.54F);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(15);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(15);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(15);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(15);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 50, 0, 0);
        event_set_pos(ev00fe_chao_0, 730.0F, 101.0F, -846.0F);
        event_set_pos(ev00fe_chao_1, 736.0F, 101.0F, -838.0F);
        event_set_pos(ev00fe_chao_2, 727.0F, 101.0F, -834.0F);
        event_set_ang(ev00fe_chao_0, 0, 0x3500, 0);
        event_set_ang(ev00fe_chao_1, 0, 0x5000, 0);
        event_set_ang(ev00fe_chao_2, 0, 0x5500, 0);
        event_set_pos(ev00fe_chao_3, 723.0F, 101.0F, -852.0F);
        event_set_pos(ev00fe_chao_4, 716.0F, 101.0F, -848.5F);
        event_set_pos(ev00fe_chao_5, 730.0F, 101.0F, -855.0F);
        event_set_pos(ev00fe_chao_6, 736.0F, 101.0F, -847.0F);
        event_set_ang(ev00fe_chao_3, 0, 0, 0);
        event_set_ang(ev00fe_chao_4, 0, 0xe000, 0);
        event_set_ang(ev00fe_chao_5, 0, 0x3000, 0);
        event_set_ang(ev00fe_chao_6, 0, 0x3500, 0);
        event_clear_motions(ev00fe_chao_0);
        event_clear_motions(ev00fe_chao_1);
        event_clear_motions(ev00fe_chao_2);
        event_clear_motions(ev00fe_chao_3);
        event_clear_motions(ev00fe_chao_4);
        event_clear_motions(ev00fe_chao_5);
        event_clear_motions(ev00fe_chao_6);
        event_play_action(ev00fe_chao_0, &ev00fe_chao_action_0,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev00fe_chao_1, &ev00fe_chao_action_1,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev00fe_chao_2, &ev00fe_chao_action_2,
                          &event_chao_texlist, 0.6F, EVENT_REPEAT, 0);
        event_play_action(ev00fe_chao_3, &ev00fe_chao_action_0,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev00fe_chao_4, &ev00fe_chao_action_1,
                          &event_chao_texlist, 0.9F, EVENT_REPEAT, 0);
        event_play_action(ev00fe_chao_5, &ev00fe_chao_action_2,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev00fe_chao_6, &ev00fe_chao_action_0,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_set_ang(event_chaos0_get_task(), 0, 0xa000, 0);
        event_chaos0_play7();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 727.65F, 115.83F, -822.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xed00, 0x0700, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 150, 734.97F, 137.41F,
                             -780.33F);
        event_sound_play(sound, 100, 0, 0);
        event_wait(10);
        event_sound_play(sound, 100, 0, 0);
        event_wait(15);
        event_sound_play(sound, 90, 0, 0);
        event_wait(10);
        event_sound_play(sound, 90, 0, 0);
        event_wait(15);
        event_sound_play(sound, 80, 0, 0);
        event_wait(10);
        event_sound_play(sound, 80, 0, 0);
        event_wait(15);
        event_sound_play(sound, 70, 0, 0);
        event_wait(10);
        event_sound_play(sound, 70, 0, 0);
        event_wait(15);
        event_sound_play(sound, 60, 0, 0);
        event_wait(10);
        event_sound_play(sound, 60, 0, 0);
        event_wait(15);
        event_sound_play(sound, 50, 0, 0);
        event_wait(10);
        event_sound_play(sound, 50, 0, 0);
        event_wait(15);
        event_sound_play(sound, 50, 0, 0);
        event_chaos0_play9();
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0400, 0x7400, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 736.32F, 102.57F, -883.86F);
        event_sound_play(sound, 40, 0, 0);
        event_wait(10);
        event_sound_play(sound, 40, 0, 0);
        event_wait(15);
        event_sound_play(sound, 50, 0, 0);
        event_wait(10);
        event_sound_play(sound, 50, 0, 0);
        event_wait(10);
        event_sound_play(sound, 40, 0, 0);
        event_camera_set_ang(EVENT_EASE_LINEAR, 45, 0x10bb, 0x7400, 0);
        event_sound_play(sound, 60, 0, 0);
        event_wait(10);
        event_sound_play(sound, 60, 0, 0);
        event_wait(15);
        event_sound_play(sound, 50, 0, 0);
        event_wait(10);
        event_sound_play(sound, 50, 0, 0);
        event_wait(15);
        event_sound_play(sound, 60, 0, 0);
        event_wait(10);
        event_sound_play(sound, 60, 0, 0);
        event_wait(15);
        event_sound_play(sound, 50, 0, 0);
        event_wait(10);
        event_sound_play(sound, 50, 0, 0);
        event_wait(5);
        event_sound_play(sound, 60, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 712.46F, 131.56F, -821.25F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1300, 0x7500, 0);
        event_sound_play(sound, -20, 0, 0);
        event_wait(10);
        event_sound_play(sound, -20, 0, 0);
        event_wait(5);
        event_sound_play(sound, -20, 0, 0);
        event_wait(10);
        event_sound_play(sound, -20, 0, 0);
        event_wait(5);
        event_sound_play(sound, -20, 0, 0);
        event_wait(10);
        event_sound_play(sound, -20, 0, 0);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 715.15F, 146.57F, -785.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1200, 0x5d00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 450, 727.18F, 146.57F,
                             -771.15F);
        event_talk_start(event_get_player(5), TEXT_9);
        event_wait(5);
        event_voice_play(2049);
        event_message_show(TEXT_1);
        event_voice_wait();
        event_talk_stop(event_get_player(5));
        event_wait(15);
        event_talk_start(event_get_player(4), TEXT_10);
        event_wait(5);
        event_voice_play(2050);
        event_message_show(TEXT_2);
        event_wait(2);
        event_talk_stop(event_get_player(4));
        event_wait(15);
        event_talk_start(event_get_player(4), TEXT_11);
        event_voice_wait();
        event_talk_stop(event_get_player(4));
        event_message_close();
        event_wait(60);
        event_chaos0_play7();
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 718.78F, 114.91F, -815.12F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf500, 0xf700, 0);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 280, 713.92F, 121.04F,
                             -793.49F);
        event_voice_play(2051);
        stream_stop_music();
        event_wait(1);
        stream_request_music(MUSIC_EVTBGM03);
        event_message_show(TEXT_3);
        event_wait(60);
        event_message_close();
        event_white_start_hold(60, 15, 60);
        event_wait(65);
        event_set_pos(event_get_player(2), 740.0F, 100.0F, -870.0F);
        event_set_ang(event_get_player(2), 0, 0xe500, 0);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 731.24F, 100.78F, -847.45F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0900, 0xf200, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 733.3F, 102.09F, -852.96F);
        event_wait(100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 758.79F, 103.9F, -884.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0300, 0x5c00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 180, 767.4F, 103.9F, -875.48F);
        event_voice_play(2052);
        event_message_show(TEXT_4);
        event_wait(180);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 734.96F, 101.28F, -855.53F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e00, 0xf500, 0);
        event_talk_start(event_get_player(2), TEXT_12);
        event_wait(5);
        event_voice_play(2053);
        event_message_show(TEXT_5);
        event_wait(2);
        event_talk_stop(event_get_player(2));
        event_wait(15);
        event_talk_start(event_get_player(2), TEXT_10);
        event_voice_wait();
        event_talk_stop(event_get_player(2));
        event_message_close();
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 733.85F, 101.74F, -843.37F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x1300, 0x4500, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 120, 0x2000);
        event_wait(120);
        event_set_ang(event_get_player(2), 0, 0xf500, 0);
        event_play_action(event_get_player(2), &ev00fe_tikal_action_2,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 32);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 718.42F, 115.0F, -791.13F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(2),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 719.49F, 133.53F,
                             -794.99F);
        event_wait(50);
        event_talk_start(event_get_player(2), TEXT_12);
        event_wait(5);
        event_voice_play(2054);
        event_message_show(TEXT_6);
        event_wait(20);
        event_talk_stop(event_get_player(2));
        event_wait(25);
        event_message_close();
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 756.84F, 100.04F, -894.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0900, 0x6b00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 759.11F, 120.53F, -898.4F);
        event_wait(100);
        ev00fe_white =
            event_fade_create(1.0F / 300.0F, 0.01F, 1.0F, 1.0F, 1.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 722.06F, 102.98F, -795.72F);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 0, event_get_player(2),
                                  0.0F, 4.0F, -4.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, 688.28F, 102.98F,
                             -842.08F);
        event_set_shadow(event_get_player(2), 0.1F);
        player_get(2)->shadow.scale = 0.0F;
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev00fe_tikal_action_0,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 32);
        event_set_pos(event_get_player(2), 730.0F, 105.0F, -850.0F);
        event_set_ang(event_get_player(2), 0, 0xe500, 0);
        ev00fe_tikal_floor = event_floor_create(event_get_player(2), 105.0F);
        event_set_pos(ev00fe_chao_0, 722.0F, 101.0F, -835.0F);
        event_set_pos(ev00fe_chao_5, 720.0F, 101.0F, -858.0F);
        event_set_pos(ev00fe_chao_6, 731.0F, 101.0F, -837.0F);
        event_set_ang(ev00fe_chao_0, 0, 0x5500, 0);
        event_set_ang(ev00fe_chao_5, 0, -0x0500, 0);
        event_set_ang(ev00fe_chao_6, 0, 0x5500, 0);
        event_wait(60);
        SET_SPEED(ev00fe_white, 0.0002F);
        event_wait(100);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 708.39F, 107.32F, -847.21F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x087c, 0xc149, 0);
        event_play_action(event_get_player(2), &ev00fe_tikal_action_1,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 32);
        event_chaos0_play10();
        event_wait(75);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 726.39F, 107.05F, -859.99F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0474, 0x7349, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 736.46F, 103.45F, -891.2F);
        event_wait(100);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(4));
        event_look_clear(event_get_player(5));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 762.78F, 102.89F, -817.72F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x057c, 0x2749, 0);
        event_sound_start(0, 1225, 1800);
        event_sound_fade_volume(0, -128, 1);
        event_wait(1);
        event_sound_fade_volume(0, 120, 180);
        event_wait(29);
        event_camera_look_at_task(EVENT_EASE_LINEAR, 120, event_get_player(2),
                                  8.0F, 6.0F, 0.0F, 0);
        event_move(event_get_player(2), event_get_player(2)->work->pos.x,
                   event_get_player(2)->work->pos.y,
                   event_get_player(2)->work->pos.z,
                   event_get_player(2)->work->pos.x - 10.0F,
                   event_get_player(2)->work->pos.y + 100.0F,
                   event_get_player(2)->work->pos.z, 300);
        event_wait(15);
        event_move(event_chaos0_get_task(),
                   event_chaos0_get_task()->work->pos.x,
                   event_chaos0_get_task()->work->pos.y,
                   event_chaos0_get_task()->work->pos.z,
                   event_chaos0_get_task()->work->pos.x + 10.0F,
                   event_chaos0_get_task()->work->pos.y + 100.0F,
                   event_chaos0_get_task()->work->pos.z, 400);
        event_wait(200);
        event_white_start_hold(60, 15, 60);
        SET_SPEED(ev00fe_white, -1.0F / 120.0F);
        event_wait(61);
        EVENT_KILL(ev00fe_tikal_floor);
        event_move_stop(event_get_player(2));
        event_move_stop(event_chaos0_get_task());
        event_kill_player(2);
        event_chaos0_kill();
        event_sound_fade_volume(0, -100, 60);
        event_kill_task(&ev00fe_chao_0);
        event_kill_task(&ev00fe_chao_1);
        event_kill_task(&ev00fe_chao_2);
        event_kill_task(&ev00fe_chao_3);
        event_kill_task(&ev00fe_chao_4);
        event_kill_task(&ev00fe_chao_5);
        event_kill_task(&ev00fe_chao_6);
        event_camera_look_clear();
        event_wait(70);
        event_sound_stop(0);
        event_look_clear(event_get_player(0));
        event_look_clear(event_get_player(4));
        event_look_clear(event_get_player(5));
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1555);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 728.98F, 130.96F, -796.22F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x194c, 0x5816, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 220, 742.35F, 130.96F, -776.3F);
        event_wait(115);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 708.15F, 151.67F, -772.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x194c, 0x5816, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, 710.38F, 151.67F,
                             -768.88F);
        event_wait(30);
        event_look_at(event_get_player(0), 893.71F, 400.0F, -1298.07F);
        event_eggmobile_create(893.71F, 350.0F, -1298.07F, 0, 0x3400, 0);
        event_wait(10);
        event_clear_motions(event_eggmobile_get_task());
        event_wait(10);
        event_eggmobile_set_bob(0x0100, 0.5F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 690.26F, 157.64F, -732.03F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x074c, 0xf516, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 686.82F, 229.47F,
                             -719.49F);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 910.17F, 370.72F, -1211.57F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfa4c, 0x0716, 0);
        event_wait(15);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, 916.71F, 389.2F,
                             -1006.85F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x004c, 0xfc16, 0);
        event_eggmobile_fly_turn(893.71F, 352.0F, -1298.07F, 0, -0x8000, 0, 30,
                                 1);
        event_wait(35);
        event_eggmobile_fly_turn(1386.8F, 600.0F, -2868.25F, 0, 0, 0, 300, 1);
        EVENT_KILL(ev00fe_sonic_floor_0);
        EVENT_KILL(ev00fe_tails_floor_0);
        EVENT_KILL(ev00fe_knuckles_floor_0);
        event_wait(1);
        event_set_pos(event_get_player(0), 691.79F, 150.0F, -762.0F);
        event_set_pos(event_get_player(4), 698.0F, 150.0F, -755.0F);
        event_set_pos(event_get_player(5), 684.16F, 150.0F, -770.25F);
        ev00fe_sonic_floor_1 = event_floor_create(event_get_player(0), 150.0F);
        ev00fe_tails_floor_1 = event_floor_create(event_get_player(4), 150.0F);
        ev00fe_knuckles_floor_1 =
            event_floor_create(event_get_player(5), 150.0F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 691.06F, 155.17F, -771.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x094c, 0x8716, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 691.56F, 155.85F, -768.35F);
        event_eggmobile_set_mode(EGGMOBILE_STILL);
        event_wait(10);
        event_eggmobile_kill();
        event_wait(160);
        event_create_player(6, amy_step, 671.54F, 150.0F, -533.45F, 0, 0x5700,
                            0);
        event_create_player(7, big_step, 717.56F, 150.0F, -585.12F, 0, 0x4d00,
                            0);
        event_place_actor(&ev00fe_bird, 671.54F, 150.0F, -533.45F, 0, 0x5700,
                          0);
        event_wait(1);
        event_clear_motions(event_get_player(6));
        event_play_action(event_get_player(6), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        ev00fe_amy_floor = event_floor_create(event_get_player(6), 150.0F);
        event_clear_motions(event_get_player(7));
        event_play_action(event_get_player(7), &big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        ev00fe_big_floor = event_floor_create(event_get_player(7), 150.0F);
        event_set_flags(ev00fe_bird, EVENT_IGNORE_GROUND);
        event_set_pos(ev00fe_bird, 669.54F, 158.0F, -538.45F);
        event_set_ang(ev00fe_bird, 0, 0x5700, 0);
        event_clear_motions(ev00fe_bird);
        event_play_action(ev00fe_bird, &ev00fe_bird_action,
                          &ev00fe_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 738.09F, 158.31F, -581.88F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x094c, 0x3616, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 677.48F, 153.45F, -534.47F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x124c, 0x3e16, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 677.39F, 153.45F, -532.47F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 698.66F, 157.17F, -761.76F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf94c, 0x7c16, 0);
        event_set_ang(event_get_player(5), 0, 0x4754, 0);
        EVENT_KILL(ev00fe_amy_floor);
        EVENT_KILL(ev00fe_big_floor);
        event_kill_player(6);
        event_kill_player(7);
        event_kill_task(&ev00fe_bird);
        stream_stop_music();
        event_talk_start(event_get_player(4), TEXT_10);
        event_wait(5);
        event_voice_play(2055);
        event_message_show(TEXT_7);
        event_wait(60);
        event_talk_stop(event_get_player(4));
        EVENT_KILL(ev00fe_sonic_floor_1);
        event_wait(15);
        event_look_at(event_get_player(4), 691.79F, 156.0F, -762.0F);
        event_look_clear(event_get_player(0));
        event_set_pos(event_get_player(0), 691.79F, 135.0F, -762.0F);
        event_set_ang(event_get_player(0), 0, 0xea86, 0);
        ev00fe_sonic_floor_2 = event_floor_create(event_get_player(0), 135.0F);
        event_wait(30);
        event_message_close();
        event_set_ang(event_get_player(4), 0, 0x87c7, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 689.37F, 179.43F, -784.01F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xde4c, 0x8416, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 300, 688.76F, 186.06F,
                             -790.08F);
        event_look_clear(event_get_player(4));
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev00fe_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(15);
        stream_request_music(MUSIC_THESONIC);
        event_voice_play(2056);
        event_message_show(TEXT_8);
        event_wait(50);
        event_message_close();
        event_wait(30);
        EVENT_KILL(ev00fe_sonic_floor_2);
        event_wait(1);
        event_set_pos(event_get_player(0), -402.65F, 556.86F, -264.29F);
        event_clear_motions(event_get_player(0));
        ev00fe_sonic_floor_3 = event_floor_create(event_get_player(0), 522.0F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -324.32F, 534.34F, -300.59F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x084c, 0x6516, 0);
        event_wait(45);
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev00fe_jumps);
        event_wait(10);
        EVENT_KILL(ev00fe_sonic_floor_3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 300, -34.67F, 600.22F,
                             -673.36F);
        event_wait(45);
        event_sound_play(729, 100, 0, 0);
        event_set_pos(event_get_player(0), 764.99F, 100.0F, -814.88F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_wait(255);
        ev00fe_fade_out =
            event_fade_create(1.0F / 120.0F, 0.2F, 0.0F, 0.0F, 0.0F);
        event_wait(120);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_move_stop_all();
        EVENT_KILL(ev00fe_fade_out);
        EVENT_KILL(ev00fe_white);
        EVENT_KILL(ev00fe_fade_in);
        EVENT_KILL(ev00fe_sonic_floor_0);
        EVENT_KILL(ev00fe_tails_floor_0);
        EVENT_KILL(ev00fe_knuckles_floor_0);
        EVENT_KILL(ev00fe_amy_floor);
        EVENT_KILL(ev00fe_big_floor);
        EVENT_KILL(ev00fe_tikal_floor);
        EVENT_KILL(ev00fe_sonic_floor_1);
        EVENT_KILL(ev00fe_tails_floor_1);
        EVENT_KILL(ev00fe_knuckles_floor_1);
        EVENT_KILL(ev00fe_sonic_floor_2);
        EVENT_KILL(ev00fe_sonic_floor_3);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(4);
        event_kill_player(5);
        event_kill_player(6);
        event_kill_player(7);
        event_eggmobile_kill();
        event_kill_task(&ev00fe_bird);
        event_kill_task(&ev00fe_chao_0);
        event_kill_task(&ev00fe_chao_1);
        event_kill_task(&ev00fe_chao_2);
        event_kill_task(&ev00fe_chao_3);
        event_kill_task(&ev00fe_chao_4);
        event_kill_task(&ev00fe_chao_5);
        event_kill_task(&ev00fe_chao_6);
        event_chaos0_kill();
        event_white_kill();
        event_set_pos(event_get_player(0), 764.99F, 100.0F, -814.88F);
        event_set_ang(event_get_player(0), 0, 0, 0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        for (pvm = ev00fe_cast_textures; pvm->texlist != NULL; pvm++) {
            njReleaseTexture(pvm->texlist);
        }
        texture_load_pvm("EFF_REGULAR", &eff_regular_texlist);
        /* con_regular_texlist is loaded again, though EVENT_LOAD released
         * only eff_regular_texlist */
        texture_load_pvm("CON_REGULAR", &con_regular_texlist);
        break;
    }
}

static struct texture_pvm ev00fe_cast_textures[10] = {
    {"CHAOS0", &event_chaos0_texlist},
    {"TIKAL", &tikal_texlist},
    {"EV_ALIFE", &event_chao_texlist},
    {"MILES", &tails_texlist},
    {"KNUCKLES", &knuckles_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"WING_P", &ev00fe_bird_texlist},
    {"BIG", &big_texlist},
    {"AMY", &amy_texlist},
    {NULL, NULL},
};
