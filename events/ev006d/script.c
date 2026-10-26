/* EV006D, Amy's story: Tails sets Amy down. she sees the bird's locket, and
 * sets off for the Mystic Ruins to find its family */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev006d/amy_action_9.h"
#include "ev006d/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/textures.h"
#include "shared/move.h"
#include "shared/picture.h"
#include "shared/prop.h"
#include "shared/shapes.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 31
#define TEXT_2_LENGTH 41
#define TEXT_3_LENGTH 26
#define TEXT_4_LENGTH 63
#define TEXT_5_LENGTH 54
#define TEXT_6_LENGTH 24
#define TEXT_7_LENGTH 17
#define TEXT_8_LENGTH 49
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 19
#define TEXT_11_LENGTH 31
#define TEXT_12_LENGTH 43
#define TEXT_13_LENGTH 73
#define TEXT_14_LENGTH 42
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 1
#define TEXT_22_LENGTH 3
#define TEXT_23_LENGTH 6
#define TEXT_24_LENGTH 3
#define TEXT_25_LENGTH 8
#define TEXT_26_LENGTH 1
#define TEXT_27_LENGTH 2
#define TEXT_28_LENGTH 7
#define TEXT_29_LENGTH 4
#define TEXT_30_LENGTH 3
#define TEXT_31_LENGTH 6
#include "ev006d/script/text.inc"

/* the locket's prop slot */
#define LOCKET 1

extern NJS_ACTION amy_stand_action;
extern NJS_ACTION tails_run_action;

extern NJS_ACTION ev006d_amy_action_0;
extern NJS_ACTION ev006d_amy_action_1;
/* @hack type: ev006d_amy_action_2 declared as an array, which script.c passes
 * by its name. SHC then loads the blend 8 anew for each of the three plays that
 * pass it, this action's among them (MOV #8,R2 at 0x0cb8058a, 0x0cb805b6 and
 * 0x0cb806dc). as one NJS_ACTION passed as &ev006d_amy_action_2, R10 holds the
 * 8 for all three plays. ev006d_script then runs 8 bytes long, and 938 of its
 * 1575 words differ */
extern NJS_ACTION ev006d_amy_action_2[];
extern NJS_ACTION ev006d_amy_action_3;
extern NJS_ACTION ev006d_amy_action_4;
extern NJS_ACTION ev006d_amy_action_5;
extern NJS_ACTION ev006d_amy_action_6;
extern NJS_ACTION ev006d_amy_action_7;
extern NJS_ACTION ev006d_amy_action_8;
extern NJS_ACTION ev006d_tails_action_0;
extern NJS_ACTION ev006d_tails_action_1;
extern NJS_ACTION ev006d_tails_action_2;
extern NJS_ACTION ev006d_tails_action_3;

/* the path Tails flies down with Amy held above him. the bird starts along
 * it 40 steps after him */
static struct event_path_point ev006d_flight_points[102] = {
#include "ev006d/script/ev006d_flight_points.inc"
};
static struct event_path ev006d_flight = {102, ev006d_flight_points};

static NJS_TEXNAME ev006d_locket_textures[6] = {
    {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0},
    {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
};
static NJS_TEXLIST ev006d_locket_texlist = {
#include "ev006d/script/ev006d_locket_texlist.inc"
};
static NJS_MATERIAL ev006d_locket_7_materials[1] = {
#include "ev006d/script/ev006d_locket_7_materials.inc"
};
static Sint16 ev006d_locket_7_meshes0[9] = {
#include "ev006d/script/ev006d_locket_7_meshes0.inc"
};
static NJS_COLOR ev006d_locket_7_uvs0[8] = {
#include "ev006d/script/ev006d_locket_7_uvs0.inc"
};
static NJS_MESHSET ev006d_locket_7_meshsets[1] = {
#include "ev006d/script/ev006d_locket_7_meshsets.inc"
};
static NJS_POINT3 ev006d_locket_7_points[6] = {
#include "ev006d/script/ev006d_locket_7_points.inc"
};
static NJS_VECTOR ev006d_locket_7_normals[6] = {
#include "ev006d/script/ev006d_locket_7_normals.inc"
};
static NJS_MODEL ev006d_locket_7_model = {
#include "ev006d/script/ev006d_locket_7_model.inc"
};
static NJS_OBJECT ev006d_locket_7 = {
#include "ev006d/script/ev006d_locket_7.inc"
};
static NJS_MATERIAL ev006d_locket_6_materials[1] = {
#include "ev006d/script/ev006d_locket_6_materials.inc"
};
static Sint16 ev006d_locket_6_meshes0[9] = {
#include "ev006d/script/ev006d_locket_6_meshes0.inc"
};
static NJS_COLOR ev006d_locket_6_uvs0[8] = {
#include "ev006d/script/ev006d_locket_6_uvs0.inc"
};
static NJS_MESHSET ev006d_locket_6_meshsets[1] = {
#include "ev006d/script/ev006d_locket_6_meshsets.inc"
};
static NJS_POINT3 ev006d_locket_6_points[6] = {
#include "ev006d/script/ev006d_locket_6_points.inc"
};
static NJS_VECTOR ev006d_locket_6_normals[6] = {
#include "ev006d/script/ev006d_locket_6_normals.inc"
};
static NJS_MODEL ev006d_locket_6_model = {
#include "ev006d/script/ev006d_locket_6_model.inc"
};
static NJS_OBJECT ev006d_locket_6 = {
#include "ev006d/script/ev006d_locket_6.inc"
};
static NJS_MATERIAL ev006d_locket_5_materials[1] = {
#include "ev006d/script/ev006d_locket_5_materials.inc"
};
static Sint16 ev006d_locket_5_meshes0[12] = {
#include "ev006d/script/ev006d_locket_5_meshes0.inc"
};
static NJS_COLOR ev006d_locket_5_uvs0[10] = {
#include "ev006d/script/ev006d_locket_5_uvs0.inc"
};
static NJS_MESHSET ev006d_locket_5_meshsets[1] = {
#include "ev006d/script/ev006d_locket_5_meshsets.inc"
};
static NJS_POINT3 ev006d_locket_5_points[6] = {
#include "ev006d/script/ev006d_locket_5_points.inc"
};
static NJS_VECTOR ev006d_locket_5_normals[6] = {
#include "ev006d/script/ev006d_locket_5_normals.inc"
};
static NJS_MODEL ev006d_locket_5_model = {
#include "ev006d/script/ev006d_locket_5_model.inc"
};
static NJS_OBJECT ev006d_locket_5 = {
#include "ev006d/script/ev006d_locket_5.inc"
};
static NJS_MATERIAL ev006d_locket_4_materials[1] = {
#include "ev006d/script/ev006d_locket_4_materials.inc"
};
static Sint16 ev006d_locket_4_meshes0[10] = {
#include "ev006d/script/ev006d_locket_4_meshes0.inc"
};
static NJS_COLOR ev006d_locket_4_uvs0[8] = {
#include "ev006d/script/ev006d_locket_4_uvs0.inc"
};
static NJS_MESHSET ev006d_locket_4_meshsets[1] = {
#include "ev006d/script/ev006d_locket_4_meshsets.inc"
};
static NJS_POINT3 ev006d_locket_4_points[5] = {
#include "ev006d/script/ev006d_locket_4_points.inc"
};
static NJS_VECTOR ev006d_locket_4_normals[5] = {
#include "ev006d/script/ev006d_locket_4_normals.inc"
};
static NJS_MODEL ev006d_locket_4_model = {
#include "ev006d/script/ev006d_locket_4_model.inc"
};
static NJS_OBJECT ev006d_locket_4 = {
#include "ev006d/script/ev006d_locket_4.inc"
};
static NJS_MATERIAL ev006d_locket_3_materials[1] = {
#include "ev006d/script/ev006d_locket_3_materials.inc"
};
static Sint16 ev006d_locket_3_meshes0[10] = {
#include "ev006d/script/ev006d_locket_3_meshes0.inc"
};
static NJS_COLOR ev006d_locket_3_uvs0[8] = {
#include "ev006d/script/ev006d_locket_3_uvs0.inc"
};
static NJS_MESHSET ev006d_locket_3_meshsets[1] = {
#include "ev006d/script/ev006d_locket_3_meshsets.inc"
};
static NJS_POINT3 ev006d_locket_3_points[5] = {
#include "ev006d/script/ev006d_locket_3_points.inc"
};
static NJS_VECTOR ev006d_locket_3_normals[5] = {
#include "ev006d/script/ev006d_locket_3_normals.inc"
};
static NJS_MODEL ev006d_locket_3_model = {
#include "ev006d/script/ev006d_locket_3_model.inc"
};
static NJS_OBJECT ev006d_locket_3 = {
#include "ev006d/script/ev006d_locket_3.inc"
};
static NJS_MATERIAL ev006d_locket_2_materials[2] = {
#include "ev006d/script/ev006d_locket_2_materials.inc"
};
static Sint16 ev006d_locket_2_meshes0[23] = {
#include "ev006d/script/ev006d_locket_2_meshes0.inc"
};
static Sint16 ev006d_locket_2_meshes1[15] = {
#include "ev006d/script/ev006d_locket_2_meshes1.inc"
};
static NJS_COLOR ev006d_locket_2_uvs0[20] = {
#include "ev006d/script/ev006d_locket_2_uvs0.inc"
};
static NJS_COLOR ev006d_locket_2_uvs1[12] = {
#include "ev006d/script/ev006d_locket_2_uvs1.inc"
};
static NJS_MESHSET ev006d_locket_2_meshsets[2] = {
#include "ev006d/script/ev006d_locket_2_meshsets.inc"
};
static NJS_POINT3 ev006d_locket_2_points[12] = {
#include "ev006d/script/ev006d_locket_2_points.inc"
};
static NJS_VECTOR ev006d_locket_2_normals[12] = {
#include "ev006d/script/ev006d_locket_2_normals.inc"
};
static NJS_MODEL ev006d_locket_2_model = {
#include "ev006d/script/ev006d_locket_2_model.inc"
};
static NJS_OBJECT ev006d_locket_2 = {
#include "ev006d/script/ev006d_locket_2.inc"
};
static NJS_MATERIAL ev006d_locket_1_materials[2] = {
#include "ev006d/script/ev006d_locket_1_materials.inc"
};
static Sint16 ev006d_locket_1_meshes0[79] = {
#include "ev006d/script/ev006d_locket_1_meshes0.inc"
};
static Sint16 ev006d_locket_1_meshes1[9] = {
#include "ev006d/script/ev006d_locket_1_meshes1.inc"
};
static NJS_COLOR ev006d_locket_1_uvs0[68] = {
#include "ev006d/script/ev006d_locket_1_uvs0.inc"
};
static NJS_COLOR ev006d_locket_1_uvs1[8] = {
#include "ev006d/script/ev006d_locket_1_uvs1.inc"
};
static NJS_MESHSET ev006d_locket_1_meshsets[2] = {
#include "ev006d/script/ev006d_locket_1_meshsets.inc"
};
static NJS_POINT3 ev006d_locket_1_points[28] = {
#include "ev006d/script/ev006d_locket_1_points.inc"
};
static NJS_VECTOR ev006d_locket_1_normals[28] = {
#include "ev006d/script/ev006d_locket_1_normals.inc"
};
static NJS_MODEL ev006d_locket_1_model = {
#include "ev006d/script/ev006d_locket_1_model.inc"
};
static NJS_OBJECT ev006d_locket_1 = {
#include "ev006d/script/ev006d_locket_1.inc"
};
static NJS_OBJECT ev006d_locket_object = {
#include "ev006d/script/ev006d_locket_object.inc"
};
static NJS_MKEY_F ev006d_locket_pos0[2] = {
#include "ev006d/script/ev006d_locket_pos0.inc"
};
static NJS_MKEY_A ev006d_locket_ang0[2] = {
#include "ev006d/script/ev006d_locket_ang0.inc"
};
static NJS_MKEY_F ev006d_locket_pos1[2] = {
#include "ev006d/script/ev006d_locket_pos1.inc"
};
static NJS_MKEY_F ev006d_locket_pos2[2] = {
#include "ev006d/script/ev006d_locket_pos2.inc"
};
static NJS_MKEY_A ev006d_locket_ang2[11] = {
#include "ev006d/script/ev006d_locket_ang2.inc"
};
static NJS_MKEY_F ev006d_locket_pos3[2] = {
#include "ev006d/script/ev006d_locket_pos3.inc"
};
static NJS_MKEY_F ev006d_locket_pos4[2] = {
#include "ev006d/script/ev006d_locket_pos4.inc"
};
static NJS_MKEY_F ev006d_locket_pos5[2] = {
#include "ev006d/script/ev006d_locket_pos5.inc"
};
static NJS_MKEY_A ev006d_locket_ang5[2] = {
#include "ev006d/script/ev006d_locket_ang5.inc"
};
static NJS_MKEY_F ev006d_locket_pos6[2] = {
#include "ev006d/script/ev006d_locket_pos6.inc"
};
static NJS_MKEY_A ev006d_locket_ang6[2] = {
#include "ev006d/script/ev006d_locket_ang6.inc"
};
static NJS_MKEY_F ev006d_locket_pos7[2] = {
#include "ev006d/script/ev006d_locket_pos7.inc"
};
static NJS_MKEY_A ev006d_locket_ang7[2] = {
#include "ev006d/script/ev006d_locket_ang7.inc"
};
static NJS_MDATA2 ev006d_locket_mdata[8] = {
#include "ev006d/script/ev006d_locket_mdata.inc"
};
static NJS_MOTION ev006d_locket_motion = {
#include "ev006d/script/ev006d_locket_motion.inc"
};
static NJS_ACTION ev006d_locket_action = {
#include "ev006d/script/ev006d_locket_action.inc"
};

static struct task *ev006d_bird = NULL;
/* the pictures of ICM006D's textures 0 and 1, over the screen once the
 * camera turns to the locket. the script makes the pair twice, and the
 * second pair replaces the first in these slots. the script never fades or
 * kills the first pair, which stays clear, as every new picture is */
static struct task *ev006d_picture_0 = NULL;
static struct task *ev006d_picture_1 = NULL;
/* no code uses this word. it keeps ev006d_carry at its address in the
 * image */
static Uint32 dat_0cb849b8 = 0;
static struct task *ev006d_carry = NULL;

/* set Amy's pos and ang.y to Tails' plus the task's. Amy is player 0, and
 * Tails player 3. nothing checks that either player still exists, so both
 * must outlive the task */
static void ev006d_carry_step(struct task *task)
{
    struct task_work *tails = event_get_player(3)->work;
    struct task_work *amy = event_get_player(0)->work;
    struct task_work *work = task->work;

    amy->pos.x = tails->pos.x + work->pos.x;
    amy->pos.y = tails->pos.y + work->pos.y;
    amy->pos.z = tails->pos.z + work->pos.z;
    amy->ang.y = tails->ang.y + work->ang.y;
}

/* make ev006d_carry, the task that holds Amy 5 above Tails. its ang.y is
 * one read of Tails' ang.y less another, which is 0, so Amy takes Tails'
 * ang.y */
static void ev006d_carry_create(void)
{
    struct task_work *tails;
    struct task_work *tails_again;
    struct task_work *work;

    ev006d_carry = task_create(TASK_WORK, 5, ev006d_carry_step);
    tails = event_get_player(3)->work;
    tails_again = event_get_player(3)->work;
    work = ev006d_carry->work;
    work->pos.x = 0.0F;
    work->pos.y = 5.0F;
    work->pos.z = 0.0F;
    work->ang.y = tails_again->ang.y - tails->ang.y;
}

void ev006d_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_clear_task(event_get_player(0));
        sound_load_file("e_0064.mlt");
        event_sound_init(5);
        event_camera_take();
        event_pads_off();
        event_take_control();
        ev006d_picture_0 = event_picture_create(&ev006d_picture_texlist);
        event_picture_set_texture(ev006d_picture_0, 0);
        event_picture_set_depth(ev006d_picture_0, -5.0F);
        ev006d_picture_1 = event_picture_create(&ev006d_picture_texlist);
        event_picture_set_texture(ev006d_picture_1, 1);
        event_picture_set_depth(ev006d_picture_1, -3.0F);
        stream_request_music(MUSIC_THEAMY);
        event_create_player(3, tails_step, 340.01F, 0.01F, 1650.25F, 0, 0x4300,
                            0);
        event_set_flags(event_get_player(3), EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(0), 340.01F, 0.01F, 1630.25F);
        event_set_ang(event_get_player(0), 0, 0x7800, 0);
        /* here and below, the bird's angles are Amy's x, y and z, not her
         * angles. event_place_actor's Angle parameters convert each to an
         * integer */
        event_place_actor(&ev006d_bird, 340.01F, 0.01F, 1630.25F,
                          event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z);
        event_set_flags(ev006d_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev006d_bird, ev006d_bird_action_0,
                          &ev006d_bird_texlist_2, 1.0F, EVENT_REPEAT, 0);
        event_wait(1);
        event_set_shadow(ev006d_bird, 0.5F);
        event_wait(1);
        event_shapes_set_firm(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -10.22F, 40.72F, 1504.89F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfe00, 0x7700, 0);
        TAILS_FLAGS(player_work[3]) |= TAILS_TAIL_SPIN;
        ev006d_carry_create();
        event_wait(20);
        event_set_flags(event_get_player(0), EVENT_IGNORE_GROUND);
        event_set_flags(event_get_player(3), EVENT_IGNORE_GROUND);
        event_play_action(event_get_player(0), &ev006d_amy_action_0,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(3), &ev006d_tails_action_0,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_path(event_get_player(3), &ev006d_flight, 0.2F, 0);
        event_wait(40);
        event_play_path(ev006d_bird, &ev006d_flight, 0.2F, 0);
        event_wait(450);
        event_clear_paths(event_get_player(3));
        ev006d_picture_0 = event_picture_create(&ev006d_picture_texlist);
        event_picture_set_texture(ev006d_picture_0, 0);
        event_picture_set_depth(ev006d_picture_0, -5.0F);
        ev006d_picture_1 = event_picture_create(&ev006d_picture_texlist);
        event_picture_set_texture(ev006d_picture_1, 1);
        event_picture_set_depth(ev006d_picture_1, -3.0F);
        TAILS_FLAGS(player_work[3]) &= ~TAILS_TAIL_SPIN;
        TAILS_FLAGS(player_work[3]) &= ~TAILS_TAIL_STILL;
        if (ev006d_carry != NULL) {
            task_kill(ev006d_carry);
            ev006d_carry = NULL;
        }
        event_clear_paths(ev006d_bird);
        event_set_flags(event_get_player(0), EVENT_GROUND | EVENT_SLOPE);
        event_set_pos(event_get_player(0), -18.4F, 0.0F, 1534.46F);
        event_set_ang(event_get_player(0), 0, 0x7800, 0);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_set_flags(event_get_player(3), EVENT_GROUND | EVENT_SLOPE);
        event_set_pos(event_get_player(3), -11.85F, 0.0F, 1545.28F);
        event_set_ang(event_get_player(3), 0, 0x4300, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev006d_tails_action_1,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_place_actor(&ev006d_bird, -23.28F, 9.0F, 1531.51F,
                          event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z);
        event_set_flags(ev006d_bird, EVENT_IGNORE_GROUND);
        event_play_action(ev006d_bird, ev006d_bird_action_0,
                          &ev006d_bird_texlist_2, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -2.62F, 9.3F, 1473.18F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0x7800, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -10.22F, 7.38F, 1511.38F);
        event_play_action(event_get_player(0), &ev006d_amy_action_1,
                          &amy_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &amy_stand_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(60);
        event_play_action(event_get_player(0), ev006d_amy_action_2,
                          &amy_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(0), &ev006d_amy_action_3,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_play_action(event_get_player(3), &ev006d_tails_action_2,
                          &tails_texlist, 0.8F, 0, 0);
        event_play_action(event_get_player(3), &ev006d_tails_action_3,
                          &tails_texlist, 1.0F, 0, 0);
        event_wait(1);
        event_wait_motions(event_get_player(3));
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &tails_run_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(3), 183.27F, 0.0F, 1405.26F, 0.4F,
                           0.0F);
        event_wait(15);
        event_play_action(event_get_player(0), &ev006d_amy_action_4,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 8);
        event_wait(120);
        event_talk_start(event_get_player(0), TEXT_21);
        event_wait(30);
        event_talk_stop(event_get_player(0));
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 180, -15.93F, 2.92F, 1519.62F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 180, 0x0300, 0x7800, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev006d_amy_action_5,
                          &amy_texlist, 0.5F, EVENT_REPEAT, 32);
        event_wait(60);
        event_talk_start(event_get_player(0), TEXT_22);
        event_voice_play(1487);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_23);
        event_voice_play(1488);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_kill_player(3);
        event_wait(1);
        event_move(ev006d_bird, -23.28F, 9.0F, 1531.51F, -16.4F, 4.0F, 1534.46F,
                   60);
        event_talk_start(event_get_player(0), TEXT_24);
        event_voice_play(1489);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev006d_amy_action_7,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_place_actor(&ev006d_bird, -17.4F, 0.0F, 1533.46F,
                          event_get_player(0)->work->pos.x,
                          event_get_player(0)->work->pos.y,
                          event_get_player(0)->work->pos.z);
        event_wait(1);
        event_play_action(ev006d_bird, &ev006d_bird_action_1,
                          &ev006d_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        event_run_to_speed(event_get_player(0), 115.86F, 0.00531F, 1522.62F,
                           0.2F, 0.1F);
        event_follow(ev006d_bird, 1.0F, 3.0F, 1.0F, 48, event_get_player(0));
        event_wait(40);
        event_message_close();
        event_wait(10);
        event_follow(ev006d_bird, 0.0F, 1.0F, 0.0F, 480, event_get_player(0));
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 15.409F, 9.65F, 1541.035F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa00, 0x3d00, 0);
        event_camera_follow(event_get_player(0));
        event_wait(40);
        event_talk_start(event_get_player(0), TEXT_25);
        event_voice_play(1490);
        event_message_show(TEXT_4);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_camera_follow_clear();
        event_wait(30);
        event_talk_start(event_get_player(0), TEXT_25);
        event_voice_play(1491);
        event_message_wait(100, TEXT_5);
        event_talk_stop(event_get_player(0));
        event_wait(40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, 86.92F, 7.55F, 1535.65F);
        event_wait(60);
        event_run_clear(event_get_player(0));
        event_play_action(event_get_player(0), &ev006d_amy_action_8,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev006d_bird, &ev006d_bird_action_2,
                          &ev006d_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        event_prop_create(-109.708F, 10.005F, 1486.616F, 0, 0, 0, 1.0F, 1.0F,
                          1.0F, &ev006d_locket_object, &ev006d_locket_texlist,
                          LOCKET);
        event_prop_set_action(LOCKET, &ev006d_locket_action, 0);
        event_wait(1);
        event_talk_start(event_get_player(0), TEXT_26);
        event_voice_play(1492);
        event_message_show(TEXT_6);
        event_wait(10);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -113.545F, 10.0F, 1486.52F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xbf00, 0);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x1c72);
        event_picture_fade(ev006d_picture_0, 1.0F, 1);
        event_prop_play(LOCKET, 0, 1e-5F, 1);
        event_wait(7);
        event_prop_stop(LOCKET);
        event_wait(60);
        event_picture_fade(ev006d_picture_0, 0.8F, 40);
        event_wait(1);
        event_picture_fade(ev006d_picture_1, 1.0F, 40);
        event_wait(60);
        event_prop_kill(LOCKET);
        event_picture_fade(ev006d_picture_0, 0.0F, 40);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 102.383F, 7.523F, 1506.658F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_picture_fade(ev006d_picture_1, 0.0F, 40);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 30, 0x31c7);
        event_wait(1);
        event_play_action(ev006d_bird, &ev006d_bird_action_1,
                          &ev006d_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(0), &ev006d_amy_action_7,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_follow(ev006d_bird, 0.0F, 0.5F, 0.0F, 360, event_get_player(0));
        event_wait(1);
        event_run_to_speed(event_get_player(0), 115.86F, 0.00531F, 1522.62F,
                           0.1F, 2.0F);
        event_wait(29);
        event_talk_start(event_get_player(0), TEXT_27);
        event_voice_play(1493);
        event_message_show(TEXT_7);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_28);
        event_voice_play(1494);
        event_message_show(TEXT_8);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_sound_play(1228, 0, 0, 0);
        event_message_show(TEXT_9);
        event_wait(60);
        event_run_wait(event_get_player(0));
        event_set_ang(event_get_player(0), 0, 0x1863, 0);
        event_set_ang(ev006d_bird, 0, 0x1863, 0);
        event_follow(ev006d_bird, 0.0F, 0.5F, 0.0F, 120, event_get_player(0));
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 121.319F, 9.699F, 1536.274F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf59e, 0x10c0, 0);
        event_play_action(event_get_player(0), &ev006d_amy_action_9,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(ev006d_bird, &ev006d_bird_action_3,
                          &ev006d_bird_texlist_1, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_27);
        event_voice_play(1495);
        event_message_show(TEXT_10);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_27);
        event_voice_play(1496);
        event_message_show(TEXT_11);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_29);
        event_voice_play(1497);
        event_message_show(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_30);
        event_wait(120);
        event_talk_start(event_get_player(0), TEXT_31);
        event_voice_play(1498);
        event_message_show(TEXT_13);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_clear();
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 157.76F, 17.271F, 1566.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0600, 0x1a00, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev006d_amy_action_6,
                          &amy_texlist, 1.0F, EVENT_REPEAT, 0);
        event_kill_task(&ev006d_bird);
        event_run_to_speed(event_get_player(0), 82.67F, 0.0F, 1486.185F, 0.3F,
                           0.3F);
        event_voice_play(1499);
        event_message_show(TEXT_14);
        event_wait(1);
        event_voice_wait();
        event_shapes_clear_firm_players(event_get_player(0));
        event_wait(60);
        break;
    case EVENT_END:
        if (event_get_player(3) != NULL) {
            TAILS_FLAGS(player_work[3]) &= ~TAILS_TAIL_SPIN;
            TAILS_FLAGS(player_work[3]) &= ~TAILS_TAIL_STILL;
        }
        event_sound_stop_all();
        EVENT_KILL(ev006d_picture_0);
        EVENT_KILL(ev006d_picture_1);
        event_set_pos(event_get_player(0), 82.67F, 0.0F, 1486.185F);
        event_clear_paths(event_get_player(3));
        event_set_flags(event_get_player(0), EVENT_GROUND | EVENT_SLOPE);
        event_set_flags(event_get_player(3), EVENT_GROUND | EVENT_SLOPE);
        if (ev006d_carry != NULL) {
            task_kill(ev006d_carry);
            ev006d_carry = NULL;
        }
        event_clear_paths(ev006d_bird);
        event_clear_paths(event_get_player(3));
        event_shapes_clear_firm_players(event_get_player(0));
        event_reset_player(3);
        event_reset_player(0);
        event_kill_player(3);
        event_move_stop_all();
        event_prop_kill(LOCKET);
        event_kill_task(&ev006d_bird);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev006d_textures[] = {
    {"VER1_WING", &ev006d_bird_texlist_1},
    {"VER2_WING", &ev006d_bird_texlist_2},
    {"LOCKET", &ev006d_locket_texlist},
    {"MILES", &tails_texlist},
    {"ICM006D", &ev006d_picture_texlist},
    {NULL, NULL},
};
