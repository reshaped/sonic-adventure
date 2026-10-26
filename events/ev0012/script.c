/* EV0012, Sonic's story: in Station Square, Amy runs up to Sonic. she asks him
 * to guard a little bird that is being chased. she says she'll come along even
 * if he says no */
#include <ninja.h>

#include "ev0012/bird_textures_1.h"
#include "ev0012/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/textures.h"
#include "shared/move.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 27
#define TEXT_5_LENGTH 66
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 34
#define TEXT_8_LENGTH 4
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 7
#define TEXT_11_LENGTH 20
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 1
#define TEXT_14_LENGTH 13
#include "ev0012/script/text.inc"

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_land_action;
extern NJS_ACTION amy_trot_action;
extern NJS_ACTION amy_stop_action;
extern NJS_ACTION amy_stand_action;
extern NJS_ACTION amy_walk_action;

extern NJS_ACTION ev0012_bird_action_2;
extern NJS_ACTION ev0012_sonic_action_0;
extern NJS_ACTION ev0012_sonic_action_1;
extern NJS_ACTION ev0012_sonic_action_2;
extern NJS_ACTION ev0012_amy_action;

static struct task *ev0012_bird = NULL;

void ev0012_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        sound_load_file("e_0012.mlt");
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_create_player(2, amy_step, 605.7F, 0.0F, 1110.8F, 0, 0, 0);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1210.0F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_set_pos(event_get_player(2), -623.2F, 5.6F, 1079.0F);
        event_set_ang(event_get_player(2), 0, 0, 0);
        /* @bug logic: gives the bird angles from Amy's position, here and in
         * its two event_set_ang calls below. the 0x200, 0x9000 and -0x200 the
         * script adds to her coordinates are angle offsets. each coordinate,
         * offset or not, is truncated to an Angle (FTRC at 0x0cb80102 to
         * 0x0cb80122, 0x0cb801de to 0x0cb80208 and 0x0cb80b0a to 0x0cb80b34,
         * before the JSRs at 0x0cb80168, 0x0cb80210 and 0x0cb80b3c) */
        event_place_actor(&ev0012_bird, event_get_player(2)->work->pos.x + 4.8F,
                          event_get_player(2)->work->pos.y + 2.5F,
                          event_get_player(2)->work->pos.z + 0.4F,
                          event_get_player(2)->work->pos.x,
                          event_get_player(2)->work->pos.y,
                          event_get_player(2)->work->pos.z + 0x200);
        event_wait(1);
        event_set_flags(ev0012_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev0012_bird, &ev0012_bird_action_2,
                          &ev0012_bird_texlist_2, 0.8F, EVENT_REPEAT, 0);
        event_set_pos(ev0012_bird, event_get_player(2)->work->pos.x + 4.8F,
                      event_get_player(2)->work->pos.y + 2.5F,
                      event_get_player(2)->work->pos.z + 2.4F);
        event_set_ang(ev0012_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 0x9000,
                      event_get_player(2)->work->pos.z - 0x200);
    /* @hack redundant: a label nothing jumps to. SHC then keeps
     * event_talk_stop's address in R8 for its 12 calls (MOV.L at 0x0cb805ea).
     * without it, each call loads the address from the pool, R8 holds
     * ev0012_bird's address instead, and 1277 of ev0012_script's 1449 words
     * differ */
    sonic: /* NOLINT(clang-diagnostic-unused-label) */
        event_clear_motions(event_get_player(0));
        event_set_pos(event_get_player(0), -534.2F, 0.0F, 1200.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -535.0F, 7.2F, 1191.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0255, 0x81c3, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 50, -535.0F, 5.2F, 1191.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 50, 0x0455, 0x81c3, 0);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(20);
        event_voice_play(1394);
        event_message_show(TEXT_1);
        event_set_pos(event_get_player(2), -525.5F, 0.02F, 1268.9F);
        event_set_ang(event_get_player(2), 0, 0x01c3, 0);
        event_play_action(event_get_player(2), &amy_trot_action, &amy_texlist,
                          1.3F, EVENT_REPEAT, 1);
        event_run_to_speed(event_get_player(2), -528.2F, 0.0F, 1208.0F, 0.65F,
                           0.06F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0012_sonic_action_0,
                          &sonic_texlist, 0.25F, 0, 8);
        event_move(ev0012_bird, -520.7F, 8.0F, 1269.4F, -525.4F, 8.0F, 1210.4F,
                   117);
        event_wait(10);
        stream_request_music(MUSIC_AMY);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, -531.4F, 5.3F, 1191.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0xff55, 0x7bc3, 0);
        event_wait(45);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 5.0F,
                           0.0F);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0012_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 16);
        event_set_ang(event_get_player(0), 0, 0xfd00, 0);
        event_set_ang(event_get_player(0), 0, 0xfe90, 0);
        event_set_pos(event_get_player(2), -525.5F, 0.02F, 1230.9F);
        event_set_ang(event_get_player(2), 0, 0x01c3, 0);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -518.3F, 8.7F, 1194.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfbc6, 0x636d, 0x0200);
        event_run_to_speed(event_get_player(2), -530.2F, 0.0F, 1210.0F, 0.9F,
                           0.1F);
        event_message_close();
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1203.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -523.8F, 1.2F, 1191.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x0ba8, 0x61c3, 0x0300);
        event_run_wait(event_get_player(2));
        event_play_action(event_get_player(0), &ev0012_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stop_action, &amy_texlist,
                          0.7F, 0, 1);
        event_wait(1);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_8);
        event_voice_play(1043);
        event_message_show(TEXT_2);
        event_talk_stop(event_get_player(2));
        event_play_action(event_get_player(0), &ev0012_sonic_action_2,
                          &sonic_texlist, 0.4F, 0, 4);
        event_play_action(event_get_player(0), &ev0012_sonic_action_1,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 4);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_9);
        event_voice_play(1395);
        event_message_show(TEXT_3);
        event_follow(ev0012_bird, 4.5F, 8.0F, 0.2F, 100, event_get_player(2));
        /* Sonic backs away from Amy, 0.2 a step for 20 steps */
        event_wait(2);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1203.0F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1202.8F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1202.6F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1202.4F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1202.2F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1202.0F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1201.8F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1201.6F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1201.4F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1201.2F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1201.0F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1200.8F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1200.6F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1200.4F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1200.2F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1200.0F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1199.8F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1199.6F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1199.4F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1199.2F);
        event_wait(1);
        event_set_pos(event_get_player(0), -534.2F, 0.01F, 1199.0F);
        event_wait_motions(event_get_player(2));
        event_set_ang(event_get_player(2), 0, 0x8a00, 0);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 1);
        event_talk_stop(event_get_player(0));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 4.0F,
                           0.0F);
        event_message_close();
        event_wait(10);
        event_play_action(event_get_player(2), &amy_walk_action, &amy_texlist,
                          0.75F, EVENT_REPEAT, 1);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -528.5F, 2.5F, 1189.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0855, 0x6fc3, 0x0300);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_10);
        event_run_to_speed(event_get_player(2), -531.0F, 0.0F, 1207.0F, 0.5F,
                           0.06F);
        event_voice_play(1396);
        event_message_show(TEXT_4);
        event_wait(15);
        event_play_action(event_get_player(2), &amy_stand_action, &amy_texlist,
                          0.75F, EVENT_REPEAT, 8);
        event_wait(50);
        event_talk_stop(event_get_player(2));
        event_wait(10);
        event_voice_stop();
        event_clear_motions(ev0012_bird);
        event_play_action(ev0012_bird, &ev0012_bird_action_1,
                          &ev0012_bird_texlist_1, 0.7F, EVENT_REPEAT, 0);
        event_set_pos(ev0012_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 1.0F,
                      event_get_player(2)->work->pos.z - 0.6F);
        event_set_ang(ev0012_bird, event_get_player(2)->work->pos.x,
                      event_get_player(2)->work->pos.y + 0x9000,
                      event_get_player(2)->work->pos.z - 0x200);
        event_clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &ev0012_amy_action, &amy_texlist,
                          0.75F, EVENT_REPEAT, 0);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -532.9F, 7.22F, 1200.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfb45, 0x896d, 0);
        event_set_ang(event_get_player(0), 0, 0x0f00, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 12);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_11);
        event_voice_play(1397);
        event_message_show(TEXT_5);
        event_wait(150);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -529.27F, 5.3F, 1203.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0700, 0x25f0, 0);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_12);
        event_camera_set_pos(EVENT_EASE_LINEAR, 8, -526.4F, 2.7F, 1206.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 8, 0x1300, 0x1ef0, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_land_action,
                          &sonic_texlist, 2.0F, 0, 5);
        event_play_action(event_get_player(0), &ev0012_sonic_action_1,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -525.8F, 2.2F, 1207.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x1300, 0x1ef0, 0);
        event_talk_start(event_get_player(0), TEXT_13);
        sound_play_at(1223, 0, 0, 127, -529.2F, 2.01F, 1206.0F);
        event_voice_play(1398);
        event_talk_stop(event_get_player(2));
        event_message_show(TEXT_6);
        event_follow(ev0012_bird, 0.2F, 1.0F, -0.6F, 300, event_get_player(2));
        event_voice_wait();
        event_wait(15);
        event_talk_stop(event_get_player(0));
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -516.6F, 8.11F, 1186.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa55, 0x5a00, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -520.55F, 7.42F, 1189.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xfa55, 0x5e00, 0xfe00);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_14);
        event_voice_play(1399);
        event_message_show(TEXT_7);
        event_wait(20);
        event_talk_stop(event_get_player(2));
        event_wait(60);
        event_play_action(event_get_player(0), &ev0012_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 4);
        event_wait(120);
        sound_stop(1215);
        event_move_stop_all();
        break;
    case EVENT_END:
        event_move_stop_all();
        stream_stop_music();
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_kill_player(2);
        event_move_stop_all();
        event_kill_task(&ev0012_bird);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0012_textures[] = {
    {"VER1_WING", &ev0012_bird_texlist_1},
    {"VER2_WING", &ev0012_bird_texlist_2},
    {"AMY", &amy_texlist},
    {NULL, NULL},
};
