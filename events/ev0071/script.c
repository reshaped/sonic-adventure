/* EV0071, Amy's story: the bird finds its family, and Zero strikes it down. Amy
 * hits Zero with her hammer, and swears never to forgive Zero */
#include <ninja.h>

#include "enemy/zero.h"
#include "ev0071/bird_textures.h"
#include "ev0071/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 30
#define TEXT_4_LENGTH 33
#define TEXT_5_LENGTH 16
#define TEXT_6_LENGTH 5
#include "ev0071/script/text.inc"

extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_trot_action;

extern NJS_ACTION ev0071_amy_action_0;
/* @hack type: ev0071_amy_action_1 declared as an array, which script.c passes
 * by its name. SHC then loads the blend 8 anew for each of the four plays that
 * pass it, this action's among them (MOV #8 at 0x0cb80de2, 0x0cb80f60,
 * 0x0cb80ffa and 0x0cb81132). as one NJS_ACTION passed as &ev0071_amy_action_1,
 * R12 holds the 8 for all four plays. ev0071_script then runs 4 bytes short,
 * and 440 of its 1760 words differ */
extern NJS_ACTION ev0071_amy_action_1[];
extern NJS_ACTION ev0071_amy_action_2;
extern NJS_ACTION ev0071_amy_action_3;
extern NJS_ACTION ev0071_amy_action_4;
extern NJS_ACTION ev0071_amy_action_5;
extern NJS_ACTION ev0071_amy_action_6;
extern NJS_ACTION ev0071_wing_t_action;

/* a pad recording and its keys, which nothing plays. at all but one of its
 * 261 steps, the stick is pushed at least 235 of 255. its direction spans
 * 85/256 of a turn. no button is pressed */
static const struct event_pad_key ev0071_pad_keys[261] = {
#include "ev0071/script/ev0071_pad_keys.inc"
};
static const struct event_pad_recording ev0071_pad_recording = {
    ev0071_pad_keys, sizeof(ev0071_pad_keys) / sizeof(ev0071_pad_keys[0])};

static struct task *ev0071_bird = NULL;
/* the bird's family: two birds, named for the PVMs of their textures,
 * WING_P and WING_T. their texlists differ only where WING_P's names
 * ld_pink_a and WING_T's ld_gray_a */
static struct task *ev0071_wing_p = NULL;
static struct task *ev0071_wing_t = NULL;
/* the white flash as Amy's hammer hits Zero */
static struct task *ev0071_fade = NULL;

void ev0071_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0071.mlt");
        event_sound_init(3);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        event_wait(1);
        event_set_pos(event_get_player(0), -151.1F, 733.0F, 585.0F);
        event_set_ang(event_get_player(0), 0xf800, 0xf000, 0);
        /* the bird's angles are Amy's x, y and z, not her angles.
         * event_place_actor's Angle parameters convert each to an integer */
        event_place_actor(&ev0071_bird, event_get_player(0)->work->pos.x + 1.0F,
                          event_get_player(0)->work->pos.y + 100.5F,
                          event_get_player(0)->work->pos.z + 0.1F,
                          event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z);
        event_create_player(2, zero_event_init, 195.8F, 747.3F, 330.0F, 0,
                            0xf000, 0);
        event_wait(1);
        event_set_flags(ev0071_bird, EVENT_IGNORE_GROUND);
        event_place_actor(&ev0071_wing_p, 3.6F, 759.7F, -444.2F, 0, 0, 0);
        event_place_actor(&ev0071_wing_t, 6.6F, 759.7F, -444.2F, 0, 0, 0);
        event_wait(1);
        event_set_pos(ev0071_wing_p, -5.0F, 750.0F, -393.0F);
        event_set_ang(ev0071_wing_p, 0, 0, 0);
        event_set_pos(ev0071_wing_t, 5.0F, 750.0F, -393.0F);
        event_set_ang(ev0071_wing_t, 0, 0, 0);
        event_play_action(ev0071_wing_p, &ev0071_wing_p_action,
                          &ev0071_wing_p_texlist, 3.0F, EVENT_REPEAT, 0);
        event_play_action(ev0071_wing_t, &ev0071_wing_t_action,
                          &ev0071_wing_t_texlist, 3.0F, EVENT_REPEAT, 0);
        event_set_flags(ev0071_wing_p, EVENT_IGNORE_GROUND);
        event_set_flags(ev0071_wing_t, EVENT_IGNORE_GROUND);
        event_set_shadow(ev0071_bird, 0.6F);
        event_set_shadow(ev0071_wing_p, 0.5F);
        event_set_shadow(ev0071_wing_t, 0.5F);
        stream_request_music(MUSIC_EVTBGM04);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -147.7F, 780.4F, -323.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x05b3, 0x7733, 0x0200);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, -150.0F, 747.4F, -319.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x05b3, 0x9033, 0x0200);
        event_set_pos(event_get_player(0), -140.8F, 743.5F, -289.9F);
        event_set_ang(event_get_player(0), 0, 0xfd7c, 0);
        event_wait(1);
        event_set_ang(event_get_player(0), 0, 0xfd7c, 0);
        event_wait(160);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.7F, 748.0F, -261.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1583, 0x08f6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 6.5F, 746.2F, -263.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x1583, 0x0bf6, 0);
        event_set_pos(event_get_player(0), -24.8F, 743.5F, -273.1F);
        event_set_ang(event_get_player(0), 0, 0x440c, 0);
        event_run_to_speed(event_get_player(0), 0.0F, 746.6F, -276.9F, 0.5F,
                           0.06F);
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_set_pos(ev0071_bird, -10.5F, 790.3F, -270.88F);
        event_set_ang(ev0071_bird, 0xfd00, 0x6b00, 0);
        event_play_action(ev0071_bird, &ev0071_bird_action_0,
                          &ev0071_bird_texlist, 2.4F, EVENT_REPEAT, 0);
        event_move(ev0071_bird, -10.5F, 800.3F, -265.88F, -0.5F, 750.0F,
                   -379.4F, 107);
        event_wait(40);
        event_look_at_task(event_get_player(0), ev0071_bird, 0.0F, 0.0F, 0.0F);
        event_wait(100);
        event_look_clear(event_get_player(0));
        event_wait(1);
        event_set_pos(event_get_player(0), 0.0F, 746.6F, -276.9F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.6F, 754.0F, -285.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfc83, 0x72f6, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 30, 1.29F, 754.0F, -284.46F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 30, 0xff83, 0x77f6, 0);
        event_wait(36);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 9, 0.07F, 753.3F, -281.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 9, 0xfa83, 0x7ff6, 0);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 2.9F, 755.4F, -268.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf433, 0x06b6, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, 4.2F, 749.8F, -263.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0a83, 0x06f6, 0);
        stream_stop_music();
        event_play_action(ev0071_bird, &ev0071_bird_action_0,
                          &ev0071_bird_texlist, 1.8F, EVENT_REPEAT, 0);
        event_play_action(ev0071_wing_p, &ev0071_wing_p_action,
                          &ev0071_wing_p_texlist, 3.7F, EVENT_REPEAT, 0);
        event_play_action(ev0071_wing_t, &ev0071_wing_t_action,
                          &ev0071_wing_t_texlist, 3.7F, EVENT_REPEAT, 0);
        event_set_pos(ev0071_wing_p, -5.0F, 770.0F, -393.0F);
        event_set_pos(ev0071_wing_t, 5.0F, 770.0F, -393.0F);
        event_move_turn(ev0071_bird, 0.0F, 750.0F, -385.0F, 0.0F, 803.0F,
                        -340.0F, 0, 0x4000, 0, 0, 0x4000, 0, 130);
        event_move_turn(ev0071_wing_p, -5.0F, 750.0F, -393.0F, -14.0F, 801.2F,
                        -354.0F, 0, 0x4000, 0, 0, 0x4000, 0, 130);
        event_move_turn(ev0071_wing_t, 5.0F, 750.0F, -393.0F, 14.0F, 801.2F,
                        -354.0F, 0, 0x4000, 0, 0, 0x4000, 0, 130);
        stream_request_music(MUSIC_THEAMY);
        event_wait(50);
        event_voice_play(1515);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_message_show(TEXT_1);
        event_play_action(event_get_player(0), &ev0071_amy_action_0,
                          &amy_texlist, 1.2F, EVENT_REPEAT, 16);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, 5.6F, 747.0F, -257.11F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x1083, 0x06f6, 0x0100);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 80, 0x2889);
        event_wait(85);
        event_talk_stop(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 11.55F, 766.38F, -330.76F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, ev0071_bird, 0.0F, 0.0F,
                                  0.0F, 0);
        event_move_turn(ev0071_bird, 0.0F, 795.0F, -345.0F, 0.0F, 765.8F,
                        -313.3F, 0, 0x4000, 0, 0, 0x4000, 0, 100);
        event_move_turn(ev0071_wing_p, -7.0F, 790.0F, -350.0F, -8.0F, 769.2F,
                        -315.0F, 0, 0x4000, 0, 0, 0x4000, 0, 110);
        event_move_turn(ev0071_wing_t, 7.0F, 790.0F, -350.0F, 8.0F, 769.2F,
                        -315.0F, 0, 0x4000, 0, 0, 0x4000, 0, 110);
        event_wait(101);
        event_set_pos(event_get_player(2), 0.4F, 753.3F, -226.0F);
        event_set_ang(event_get_player(2), 0xfe00, 0x3930, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -10.4F, 747.7F, -264.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b00, 0xeba0, 0x0200);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -4.3F, 757.4F, -293.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x0d00, 0xf7a0, 0x0200);
        event_move_turn(ev0071_bird, 0.0F, 765.8F, -313.3F, 0.0F, 765.8F,
                        -303.3F, 0, 0x4000, 0, 0, 0x4000, 0, 100);
        event_wait(40);
        zero_event_grab(&ev0071_bird->work->pos);
        event_wait(50);
        event_set_pos(event_get_player(0), 78.0F, 901.3F, -1180.0F);
        event_set_ang(event_get_player(0), 0xfe00, 0x3930, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &amy_trot_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(19);
        stream_stop_music();
        event_play_action(ev0071_bird, &ev0071_bird_action_1,
                          &ev0071_bird_texlist, 1.0F, 0, 0);
        event_play_action(ev0071_bird, &ev0071_bird_action_2,
                          &ev0071_bird_texlist, 1.0F, EVENT_REPEAT, 0);
        event_sound_play(1223, 120, 0, 0);
        event_wait(20);
        event_move_turn(ev0071_wing_p, -8.0F, 769.2F, -315.0F, -12.0F, 779.2F,
                        -325.0F, 0, 0x4000, 0, 0, 0x4000, 0x4000, 30);
        event_move_turn(ev0071_wing_t, 8.0F, 769.2F, -315.0F, 15.0F, 789.2F,
                        -325.0F, 0, 0x4000, 0, 0x5000, 0x4000, 0, 30);
        event_camera_look_clear();
        event_move_turn(ev0071_bird, 0.0F, 775.8F, -309.3F, -2.0F, 748.1F,
                        -292.3F, 0, 0x4000, 0, 0x4000, 0x4000, 0, 30);
        event_set_pos(event_get_player(0), 0.0F, 746.6F, -244.9F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_run_to_speed(event_get_player(0), -1.5F, 747.2F, -287.9F, 0.9F,
                           0.06F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.4F, 746.2F, -262.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0x08a0, 0x0200);
        event_voice_play(1516);
        event_message_show(TEXT_2);
        event_wait(20);
        event_play_action(ev0071_bird, &ev0071_bird_action_0,
                          &ev0071_bird_texlist, 0.02F, EVENT_REPEAT, 0);
        event_move_turn(ev0071_wing_p, -12.0F, 779.2F, -325.0F, -15.0F, 759.2F,
                        -315.0F, 0, 0x4000, 0x4000, 0, 0x2000, 0, 30);
        event_move_turn(ev0071_wing_t, 15.0F, 789.2F, -325.0F, 12.0F, 762.2F,
                        -319.0F, 0x5000, 0x4000, 0, 0, 0x7000, 0, 45);
        event_wait(30);
        event_message_close();
        event_sound_play(1224, 120, 120, 0);
        event_set_pos(event_get_player(2), 0.4F, 753.3F, -229.0F);
        event_set_ang(event_get_player(2), 0xfe00, 0x8030, 0);
        event_run_wait(event_get_player(0));
        event_play_action(event_get_player(0), ev0071_amy_action_1,
                          &amy_texlist, 0.6F, EVENT_REPEAT, 8);
        stream_request_music(MUSIC_EGGROBO);
        event_sound_play(1224, 90, -40, 0);
        event_run_to_speed(event_get_player(2), 0.4F, 753.3F, -263.0F, 0.05F,
                           0.006F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 2.2F, 749.08F, -299.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0248, 0x8d50, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1517);
        event_message_show(TEXT_3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -6.45F, 749.47F, -296.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0248, 0x9456, 0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, -8.48F, 749.15F, -298.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0648, 0x9256, 0);
        event_wait(90);
        event_play_action(event_get_player(0), &ev0071_amy_action_2,
                          &amy_texlist, 0.6F, EVENT_REPEAT, 8);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1518);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x0873, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -10.45F, 754.5F, -284.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf048, 0xdc56, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -7.9F, 755.79F, -280.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0xf448, 0xeb56, 0);
        event_play_action(event_get_player(0), &ev0071_amy_action_3,
                          &amy_texlist, 0.9F, 0, 8);
        amy_set_hammer_scale(0, 1.3F);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -14.9F, 747.1F, -281.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x0e48, 0xc856, 0x0900);
        event_wait(30);
        event_sound_play(1226, 128, 10, 0);
        ev0071_fade = event_fade_create(0.00011F, 0.8F, 1.0F, 0.99F, 0.99F);
        event_wait(3);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then keeps -393.0 in FR12
         * for the first event_set_pos calls of ev0071_wing_p and ev0071_wing_t
         * (FMOV.S @R0,FR12 at 0x0cb80182). as an if, FR12 holds 750.0 there,
         * ev0071_script runs 8 bytes short, and 1279 of its 1760 words
         * differ */
        ev0071_fade != NULL && (event_fade_set_speed(ev0071_fade, 0.2F), 0);
        event_wait(4);
        EVENT_KILL(ev0071_fade);
        event_play_action(event_get_player(0), &ev0071_amy_action_4,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_set_flags(event_get_player(2), EVENT_IGNORE_GROUND);
        event_wait(1);
        event_move_turn(event_get_player(2), 148.5F, 884.3F, -317.9F, 152.5F,
                        752.0F, -309.56F, 0x8000, 0x2000, 0x7000, 0x7000,
                        0xd000, 0, 130);
        event_wait(20);
        event_message_close();
        event_camera_set_fov(EVENT_EASE_LINEAR, 80, 0x2ccd);
        event_wait(79);
        event_sound_play(1225, 70, 120, 0);
        event_wait(80);
        amy_set_hammer_scale(0, 1.15F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 7.35F, 752.0F, -276.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0548, 0x2d56, 0x0900);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 12.7F, 749.8F, -288.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0648, 0x4456, 0x0800);
        event_set_ang(event_get_player(0), 0, 0x2800, 0);
        event_play_action(event_get_player(0), &ev0071_amy_action_5,
                          &amy_texlist, 0.6F, 0, 16);
        event_play_action(event_get_player(0), &ev0071_amy_action_6,
                          &amy_texlist, 0.6F, EVENT_REPEAT, 16);
        event_wait(120);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_6);
        event_voice_play(1519);
        event_message_show(TEXT_5);
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 7.8F, 750.3F, -287.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0948, 0x4456, 0x0800);
        event_wait(110);
        event_talk_stop(event_get_player(0));
        event_message_close();
        break;
    case EVENT_END:
        amy_set_hammer_scale(0, 0.0F);
        event_sound_stop_all();
        event_move_stop_all();
        event_clear_task(event_get_player(0));
        event_kill_player(2);
        event_kill_task(&ev0071_bird);
        event_kill_task(&ev0071_wing_p);
        event_kill_task(&ev0071_wing_t);
        if (ev0071_fade != NULL) {
            task_kill(ev0071_fade);
            ev0071_fade = NULL;
        }
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0071_textures[] = {
    {"VER2_WING", &ev0071_bird_texlist},
    {"WING_P", &ev0071_wing_p_texlist},
    {"WING_T", &ev0071_wing_t_texlist},
    {NULL, NULL},
};
