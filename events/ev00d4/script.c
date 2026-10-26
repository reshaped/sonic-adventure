/* EV00D4, Big's story: E-102 takes Froggy from Big */
#include <ninja.h>

#include "ev00d4/frog_action.h"
#include "ev00d4/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/big_kaeru_textures.h"
#include "player/e102.h"
#include "player/textures.h"
#include "shared/move.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 35
#define TEXT_4_LENGTH 37
#include "ev00d4/script/text.inc"

extern NJS_ACTION e102_stand_action;
extern NJS_ACTION e102_slide_stand_action;
extern NJS_ACTION e102_walk_action;

extern NJS_ACTION ev00d4_big_action_0;
extern NJS_ACTION ev00d4_big_action_1;
/* @hack type: ev00d4_big_action_2 declared as an array, which script.c passes
 * by its name. SHC then keeps its address in R8 across its four plays from
 * 0x0cb80514. as one NJS_ACTION passed as &ev00d4_big_action_2, its address
 * loads from the pool at each play, and 256 of 766 words differ */
extern NJS_ACTION ev00d4_big_action_2[];
extern NJS_ACTION ev00d4_big_action_3;

/* three pad recordings, without buttons. on port 2, E-102 walks a little way,
 * then turns about and walks off. on port 0, Big runs after him */
static const struct event_pad_key ev00d4_e102_walk_keys[73] = {
#include "ev00d4/script/ev00d4_e102_walk_keys.inc"
};

static const struct event_pad_recording ev00d4_e102_walk = {
    ev00d4_e102_walk_keys,
    sizeof(ev00d4_e102_walk_keys) / sizeof(ev00d4_e102_walk_keys[0])};

static const struct event_pad_key ev00d4_e102_leave_keys[143] = {
#include "ev00d4/script/ev00d4_e102_leave_keys.inc"
};

static const struct event_pad_recording ev00d4_e102_leave = {
    ev00d4_e102_leave_keys,
    sizeof(ev00d4_e102_leave_keys) / sizeof(ev00d4_e102_leave_keys[0])};

static const struct event_pad_key ev00d4_big_run_keys[209] = {
#include "ev00d4/script/ev00d4_big_run_keys.inc"
};

static const struct event_pad_recording ev00d4_big_run = {
    ev00d4_big_run_keys,
    sizeof(ev00d4_big_run_keys) / sizeof(ev00d4_big_run_keys[0])};

static struct task *ev00d4_frog = NULL;

void ev00d4_script(int phase)
{
    /* @hack spelling: event_clear_motions called through a local. SHC then
     * keeps its address in R9 from 0x0cb8008c. called by name, its address
     * loads from the pool at each call, the function runs 4 bytes short, and
     * 488 of 766 words differ */
    void (*clear_motions)(struct task *task);
    /* @hack spelling: event_set_ang called through a local. R10 holds its
     * address either way. SHC then keeps event_set_pos's address in R8 from
     * 0x0cb80066 for its four calls. called by name, event_set_pos's address
     * loads from the pool at each call, and 570 of 766 words differ */
    void (*set_ang)(struct task *task, Angle ang_x, Angle ang_y, Angle ang_z);

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_00d4.mlt");
        event_wait(1);
        event_reset_player(0);
        event_set_pos(event_get_player(0), 998.3F, 78.0F, 777.3F);
        set_ang = event_set_ang;
        set_ang(event_get_player(0), 0x019f, 0x0320, 0);
        clear_motions = event_clear_motions;
        clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00d4_big_action_1,
                          &big_texlist, 0.8F, EVENT_REPEAT, 0);
        /* here and below, Froggy's angles come from a player's position,
         * not from the player's angles. each is a coordinate, with any offset
         * added, converted to an Angle */
        event_place_actor(&ev00d4_frog, event_get_player(0)->work->pos.x - 6.1F,
                          event_get_player(0)->work->pos.y + 9.5F,
                          event_get_player(0)->work->pos.z + 3.0F,
                          event_get_player(0)->work->pos.x - 0x4000,
                          event_get_player(0)->work->pos.y - 0x4000,
                          event_get_player(0)->work->pos.z);
        event_wait(1);
        event_set_flags(ev00d4_frog, EVENT_IGNORE_GROUND);
        event_set_pos(ev00d4_frog, event_get_player(0)->work->pos.x - 5.0F,
                      event_get_player(0)->work->pos.y + 7.8F,
                      event_get_player(0)->work->pos.z + 6.7F);
        set_ang(ev00d4_frog, event_get_player(0)->work->pos.x - 0x1000,
                event_get_player(0)->work->pos.y + 0x2800,
                event_get_player(0)->work->pos.z);
        event_play_action(ev00d4_frog, &ev00d4_frog_action, &big_kaeru_texlist,
                          2.0F, EVENT_REPEAT, 0);
        event_follow(ev00d4_frog, -7.0F, 2.0F, 3.0F, 200, event_get_player(0));
        event_create_player(2, e102_step, 1027.0F, 76.0F, 857.3F, 0xf900,
                            0x9000, 0);
        event_wait(1);
        event_set_pos(event_get_player(2), 1027.0F, 73.0F, 857.3F);
        set_ang(event_get_player(2), 0xf900, 0x9000, 0);
        clear_motions(event_get_player(2));
        event_play_action(event_get_player(2), &e102_stand_action,
                          &e102_texlist, 1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 1005.97F, 84.91F, 840.5F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 7.0F, 0.0F, 0);
        event_run_to_speed(event_get_player(0), 1012.7F, 79.5F, 809.9F, 0.3F,
                           0.3F);
        event_wait(60);
        event_play_pad(2, &ev00d4_e102_walk);
        event_wait(20);
        event_move_stop(ev00d4_frog);
        event_wait(10);
        sound_play(1223, 0, 0, 0);
        event_wait(10);
        event_camera_look_clear();
        event_follow(ev00d4_frog, -13.0F, 3.2F, -15.0F, 600,
                     event_get_player(2));
        event_wait(30);
        sound_stop(1215);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 1005.13F, 83.2F, 838.6F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0342, 0xf2c0, 0);
        event_play_action(event_get_player(0), &ev00d4_big_action_0,
                          &big_texlist, 0.8F, EVENT_REPEAT, 16);
        event_wait(40);
        event_voice_play(1839);
        event_message_show(TEXT_1);
        event_wait(50);
        set_ang(event_get_player(2), 0, 0x2300, 0);
        event_play_action(event_get_player(2), &e102_slide_stand_action,
                          &e102_texlist, 2.5F, EVENT_REPEAT, 0);
        event_move_stop(ev00d4_frog);
        event_set_pos(ev00d4_frog, event_get_player(2)->work->pos.x - 3.9F,
                      event_get_player(2)->work->pos.y + 2.8F,
                      event_get_player(2)->work->pos.z - 3.9F);
        set_ang(ev00d4_frog, event_get_player(2)->work->pos.x,
                event_get_player(2)->work->pos.y + 0x3300,
                event_get_player(2)->work->pos.z);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, 1012.7F, 79.1F, 841.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x0cbf, 0x0bc4, 0);
        event_play_action(event_get_player(0), ev00d4_big_action_2,
                          &big_texlist, 2.3F, EVENT_REPEAT, 2);
        event_wait(50);
        event_play_action(event_get_player(0), ev00d4_big_action_2,
                          &big_texlist, 0.4F, EVENT_REPEAT, 12);
        set_ang(event_get_player(0), 0x0800, 0xa400, 0);
        event_wait(10);
        event_play_action(event_get_player(0), ev00d4_big_action_2,
                          &big_texlist, 0.2F, EVENT_REPEAT, 8);
        event_voice_play(1832);
        event_message_show(TEXT_2);
        event_wait(20);
        event_play_action(event_get_player(0), ev00d4_big_action_2,
                          &big_texlist, 0.1F, EVENT_REPEAT, 8);
        event_wait(65);
        event_follow(ev00d4_frog, -15.0F, 6.8F, -16.5F, 600,
                     event_get_player(2));
        event_play_pad(2, &ev00d4_e102_leave);
        sound_stop(1216);
        event_wait(10);
        event_wait(10);
        event_move_stop(ev00d4_frog);
        event_follow(ev00d4_frog, -7.0F, 4.2F, -10.5F, 600,
                     event_get_player(2));
        clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00d4_big_action_3,
                          &big_texlist, 2.0F, EVENT_REPEAT, 4);
        clear_motions(event_get_player(2));
        event_camera_set_fov(EVENT_EASE_SMOOTH, 60, 0x2eef);
        event_wait(1);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 30, event_get_player(0),
                                  0.0F, 6.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, 1032.0F, 85.3F, 836.5F);
        event_voice_play(1833);
        event_message_show(TEXT_3);
        event_play_pad(0, &ev00d4_big_run);
        event_voice_play(1834);
        event_message_show(TEXT_4);
        event_wait(20);
        event_camera_look_clear();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 110, 0x1100, 0x20c1, 0);
        event_move_stop_all();
        event_play_action(event_get_player(2), &e102_walk_action, &e102_texlist,
                          2.0F, EVENT_REPEAT, 0);
        event_wait(120);
        event_wait(1);
        break;
    case EVENT_END:
        event_move_stop_all();
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        event_kill_player(2);
        event_kill_task(&ev00d4_frog);
        event_voice_stop();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00d4_textures[] = {
    {"E102", &e102_texlist},
    {NULL, NULL},
};
