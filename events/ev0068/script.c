/* EV0068, Amy's story: Tikal comforts a Chao by the water in the past. Chaos 0
 * then rises out of the water */
#include <ninja.h>

#include "chaos/models.h"
#include "ev0068/script.h"
#include "ev0068/tikal_action_3.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/chao.h"
#include "shared/chao_textures.h"
#include "shared/chaos0.h"
#include "shared/chaos0_water_textures.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "shared/wave.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 34
#define TEXT_3_LENGTH 7
#include "ev0068/script/text.inc"

extern NJS_ACTION ev0068_chao_action_1;
extern NJS_ACTION ev0068_tikal_action_0;
extern NJS_ACTION ev0068_tikal_action_1;
extern NJS_ACTION ev0068_tikal_action_2;

/* @hack type: single NJS_ACTIONs declared as arrays. SHC then keeps
 * event_place_task's address in R14 (0x0cb800ba) and event_chao_create's in
 * R13 (0x0cb800be). it keeps ev0068_chao_1's x, -124.4F, in FR14
 * (0x0cb809a2). as a single object alone, ev0068_chao_action_0 makes 257 of
 * ev0068_script's 2274 words differ, and ev0068_chao_action_2 makes 249. as
 * single objects passed by address, R13 and R14 trade values, FR14 takes
 * ev0068_chao_0's x, -126.3F, and 62 of the 2274 differ */
extern NJS_ACTION ev0068_chao_action_0[];
extern NJS_ACTION ev0068_chao_action_2[];

static struct task *ev0068_chao_0 = NULL;
static struct task *ev0068_chao_1 = NULL;
static struct task *ev0068_chao_2 = NULL;
static struct task *ev0068_chao_3 = NULL;
static struct task *ev0068_fade = NULL;

void ev0068_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_TICAL);
        sound_load_file("e_0068.mlt");
        event_sound_init(5);
        /* Chaos 0 waits as his puddle, out of sight far below the water */
        event_chaos0_create(-66.0F, -602.3F, 234.6F, 0x3000, 0xa200, 0, 360);
        event_chaos0_puddle();
        event_create_player(2, tikal_player_step, 0.0F, 0.0F, 511.5F, 0x4f0,
                            0xb350, 0);
        event_place_task(&ev0068_chao_0, event_chao_create, -123.4F, 0.0F,
                         313.45F, 0xfd00, 0xa200, 0);
        event_place_task(&ev0068_chao_1, event_chao_create, -139.0F, 0.0F,
                         346.6F, 0xfd00, 0x1000, 0);
        event_place_task(&ev0068_chao_2, event_chao_create, -145.0F, 0.0F,
                         363.6F, 0xfd00, 0x1500, 0);
        event_place_task(&ev0068_chao_3, event_chao_create, -135.0F, 0.0F,
                         358.8F, 0xfd00, 0x1300, 0);
        event_wait(1);
        event_set_flags(ev0068_chao_0, EVENT_IGNORE_GROUND);
        event_set_flags(ev0068_chao_1, EVENT_IGNORE_GROUND);
        event_set_flags(ev0068_chao_2, EVENT_IGNORE_GROUND);
        event_set_flags(ev0068_chao_3, EVENT_IGNORE_GROUND);
        event_wait(1);
        event_reset_player(0);
        event_set_pos(event_get_player(0), -66.8F, -0.01F, 400.35F);
        event_set_ang(event_get_player(0), 0x4f0, 0xaf00, 0);
        event_set_pos(event_get_player(2), -80.0F, 0.0F, 369.5F);
        event_set_ang(event_get_player(2), 0, 0xbf00, 0);
        event_look_at(event_get_player(2), -115.6F, 1.5F, 368.8F);
        event_set_pos(ev0068_chao_0, -107.4F, 1.5F, 361.25F);
        event_set_ang(ev0068_chao_0, 0, 0xcc00, 0xfc00);
        event_set_pos(ev0068_chao_1, -113.0F, 1.5F, 366.6F);
        event_set_ang(ev0068_chao_1, 0, 0xc800, 0xfc00);
        event_set_pos(ev0068_chao_2, -117.0F, 1.5F, 379.6F);
        event_set_ang(ev0068_chao_2, 0, 0xbb00, 0xfc00);
        event_set_pos(ev0068_chao_3, -109.0F, 1.5F, 374.8F);
        event_set_ang(ev0068_chao_3, 0, 0xbb00, 0xfc00);
        event_play_action(ev0068_chao_0, ev0068_chao_action_0,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_1, ev0068_chao_action_0,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_2, ev0068_chao_action_0,
                          &event_chao_texlist, 0.6F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_3, ev0068_chao_action_0,
                          &event_chao_texlist, 1.0F, EVENT_REPEAT, 0);
        event_chao_set_eyes(ev0068_chao_0, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev0068_chao_1, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev0068_chao_2, CHAO_EYES_PLAIN);
        event_chao_set_eyes(ev0068_chao_3, CHAO_EYES_PLAIN);
        event_set_shadow(ev0068_chao_0, 0.6F);
        event_set_shadow(ev0068_chao_1, 0.6F);
        event_set_shadow(ev0068_chao_2, 0.6F);
        event_set_shadow(ev0068_chao_3, 0.6F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -69.33F, 5.8F, 409.84F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x200, 0xa00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -68.0F, 5.75F, 410.34F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x200, 0x1300, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -66.22F, 3.55F, 413.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x400, 0x1300, 0);
        event_wait(100);
        event_set_pos(ev0068_chao_2, -128.0F, 1.5F, 371.87F);
        event_set_ang(ev0068_chao_2, 0, 0xbb00, 0xfd00);
        event_set_pos(ev0068_chao_3, -129.0F, 1.5F, 374.8F);
        event_set_ang(ev0068_chao_3, 0, 0xbb00, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -129.66F, 0.75F, 379.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf00, 0xe800, 0xfd00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, -123.94F, 0.73F, 385.21F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0xf00, 0xe000, 0);
        event_wait(60);
        event_set_pos(ev0068_chao_3, -101.0F, 1.5F, 377.8F);
        event_set_ang(ev0068_chao_3, 0, 0xbb00, 0xfd00);
        event_wait(10);
        event_set_pos(ev0068_chao_2, -100.0F, 1.5F, 382.86F);
        event_set_ang(ev0068_chao_2, 0, 0xbb00, 0xfd00);
        event_move(ev0068_chao_2, -100.0F, 1.5F, 382.86F, -104.0F, 1.5F,
                   372.86F, 140);
        event_wait(90);
        event_camera_set_ang(EVENT_EASE_LINEAR, 55, 0xf00, 0xdd00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, -121.54F, 0.87F, 383.14F);
        event_wait(55);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -85.48F, 3.86F, 371.88F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0xde00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 25, -84.12F, 3.86F, 373.22F);
        event_voice_play(1453);
        event_message_show(TEXT_1);
        event_play_action(event_get_player(2), &ev0068_tikal_action_0,
                          &tikal_texlist, 0.4F, 0, 4);
        event_wait_motions(event_get_player(2));
        event_set_pos(event_get_player(0), -66.8F, -0.01F, 420.35F);
        event_set_ang(event_get_player(0), 0x4f0, 0xaf00, 0);
        event_set_pos(event_get_player(2), -100.0F, 0.0F, 369.6F);
        event_set_ang(event_get_player(2), 0, 0xbf00, 0);
        event_set_pos(ev0068_chao_2, -95.0F, 0.86F, 370.0F);
        event_set_ang(ev0068_chao_1, 0, 0x4c00, 0x1100);
        event_set_ang(ev0068_chao_3, 0, 0x4300, 0x1000);
        event_play_action(ev0068_chao_0, ev0068_chao_action_2,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_1, ev0068_chao_action_2,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_2, ev0068_chao_action_2,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_3, ev0068_chao_action_2,
                          &event_chao_texlist, 0.6F, EVENT_REPEAT, 0);
        event_move(ev0068_chao_0, -107.4F, 1.5F, 361.25F, -109.3F, 1.45F,
                   360.79F, 38);
        event_move_turn(ev0068_chao_1, -113.0F, 1.5F, 366.6F, -118.4F, 1.35F,
                        365.3F, 0, 0xc00, 0x1100, 0, 0xc00, 0x3f00, 42);
        event_move(ev0068_chao_2, -104.0F, 1.5F, 379.86F, -108.8F, 1.45F,
                   380.0F, 45);
        event_move(ev0068_chao_3, -104.0F, 1.5F, 377.8F, -117.33F, 1.45F,
                   374.58F, 40);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -127.84F, 1.841F, 370.92F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xa00, 0xc300, 0);
        event_message_close();
        event_sound_play(1226, 10, 80, 0);
        event_wait(10);
        event_sound_play(1227, 0, -20, 0);
        event_wait(20);
        event_sound_play(1227, 0, -20, 0);
        event_wait(30);
        event_play_action(ev0068_chao_0, ev0068_chao_action_0,
                          &event_chao_texlist, 0.8F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_1, ev0068_chao_action_0,
                          &event_chao_texlist, 0.7F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_2, ev0068_chao_action_0,
                          &event_chao_texlist, 0.6F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_3, ev0068_chao_action_0,
                          &event_chao_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(ev0068_chao_0, -126.3F, 1.45F, 359.79F);
        event_set_pos(ev0068_chao_1, -124.4F, 1.45F, 365.3F);
        event_set_pos(ev0068_chao_2, -123.0F, 1.45F, 375.3F);
        event_set_pos(ev0068_chao_3, -119.83F, 1.45F, 367.78F);
        event_set_ang(ev0068_chao_0, 0, 0xc800, 0xfc00);
        event_set_ang(ev0068_chao_1, 0, 0xc800, 0xfc00);
        event_set_ang(ev0068_chao_2, 0, 0xb700, 0xfd00);
        event_set_ang(ev0068_chao_3, 0, 0xba00, 0xfc00);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -108.12F, 6.42F, 370.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x800, 0xc500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -107.12F, 5.42F, 370.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x800, 0xc800, 0);
        event_talk_stop(event_get_player(2));
        event_talk_start(event_get_player(2), TEXT_3);
        event_voice_play(1454);
        event_message_wait(60, TEXT_2);
        event_voice_wait();
        /* this stops player 0's mouth, not Tikal's. hers moves on until
         * TEXT_3's letters run out */
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -96.04F, 10.0F, 376.97F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf400, 0x2400, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -102.1F, 7.97F, 374.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xf400, 0x2f00, 0);
        event_sound_play(1224, 50, 0, 0);
        event_wait(100);
        event_set_pos(event_get_player(0), -66.8F, -0.01F, 400.35F);
        event_set_ang(event_get_player(0), 0x4f0, 0x9000, 0);
        event_set_pos(ev0068_chao_0, -126.3F, 1.45F, 359.79F);
        event_set_pos(ev0068_chao_1, -124.4F, 1.45F, 365.3F);
        event_set_pos(ev0068_chao_2, -123.0F, 1.45F, 375.3F);
        event_set_pos(ev0068_chao_3, -117.83F, 1.45F, 367.78F);
        event_set_ang(ev0068_chao_0, 0, 0xcc00, 0xfc00);
        event_set_ang(ev0068_chao_1, 0, 0xca00, 0xfc00);
        event_set_ang(ev0068_chao_2, 0, 0xb000, 0xfd00);
        event_set_ang(ev0068_chao_3, 0, 0xb900, 0xfc00);
        event_move(ev0068_chao_3, -117.83F, 1.45F, 367.78F, -112.33F, 1.45F,
                   368.0F, 50);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -106.84F, 9.6F, 369.44F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe300, 0x3600, 0);
        event_wait(51);
        event_play_action(ev0068_chao_3, ev0068_chao_action_0,
                          &event_chao_texlist, 1.0F, EVENT_REPEAT, 0);
        event_sound_play(1223, 60, 0, 0);
        event_wait(21);
        event_sound_play(1223, 80, 0, 0);
        event_set_pos(ev0068_chao_3, -105.5F, 1.45F, 369.6F);
        event_set_ang(ev0068_chao_3, 0, 0xb900, 0xfc00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -90.4F, 2.68F, 364.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1000, 0x5300, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 75, -87.45F, 6.47F, 368.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 75, 0xf700, 0x4200, 0);
        event_play_action(event_get_player(2), &ev0068_tikal_action_1,
                          &tikal_texlist, 1.1F, 0, 0);
        event_sound_play(1223, 100, 0, 0);
        event_move(ev0068_chao_3, -105.5F, 1.45F, 369.6F, -105.5F, 2.6F, 369.6F,
                   50);
        event_wait(75);
        event_clear_motions(event_get_player(2));
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -104.45F, 9.16F, 378.54F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xec00, 0xdd00, 0x400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -113.2F, 12.49F, 379.54F);
        event_set_pos(ev0068_chao_3, -102.2F, 4.3F, 369.6F);
        event_set_ang(ev0068_chao_3, 0, 0x3f00, 0);
        event_play_action(event_get_player(2), &ev0068_tikal_action_2,
                          &tikal_texlist, 0.5F, EVENT_REPEAT, 0);
        event_play_action(ev0068_chao_3, &ev0068_chao_action_1,
                          &event_chao_texlist, 0.5F, EVENT_REPEAT, 0);
        event_chao_set_eyes(ev0068_chao_3, CHAO_EYES_PLAIN);
        event_move(ev0068_chao_0, -126.3F, 1.45F, 359.79F, -109.0F, 1.45F,
                   361.1F, 90);
        event_move(ev0068_chao_1, -124.4F, 1.45F, 365.3F, -112.2F, 1.45F,
                   367.0F, 100);
        event_move(ev0068_chao_2, -123.0F, 1.45F, 375.3F, -110.0F, 1.45F,
                   375.76F, 80);
        event_wait(120);
        event_set_pos(ev0068_chao_0, -106.0F, 1.45F, 361.1F);
        event_set_pos(ev0068_chao_1, -113.2F, 1.45F, 367.0F);
        event_set_pos(ev0068_chao_2, -111.0F, 1.45F, 375.76F);
        event_set_ang(ev0068_chao_0, 0, 0xbc00, 0xfc00);
        event_set_ang(ev0068_chao_1, 0, 0xca00, 0xfc00);
        event_set_ang(ev0068_chao_2, 0, 0xb000, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -119.08F, 1.6F, 369.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x700, 0xdd00, 0x400);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -118.6F, 7.37F, 377.95F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0xf700, 0xd900, 0x400);
        event_wait(20);
        event_sound_play(1223, 100, 120, 0);
        event_wait(40);
        event_sound_play(1223, 110, -100, 0);
        event_wait(20);
        event_sound_play(1223, 100, -20, 0);
        event_wait(30);
        event_sound_play(1223, 90, 80, 0);
        event_wait(10);
        event_sound_play(1223, 20, 30, 0);
        event_wait(10);
        event_sound_play(1223, 90, -90, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -87.29F, 35.42F, 257.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe100, 0xd600, 0xf900);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 80, -91.3F, 31.1F, 258.32F);
        event_sound_play(1225, 110, -100, 0);
        event_wave_emitter_create(-73.0F, -0.6F, 223.6F, 0.25F, 1.0F, 6, 18, 1);
        event_wait(30);
        event_sound_play(1225, 80, -80, 0);
        event_wave_emitter_create(-73.0F, -0.6F, 223.6F, 0.2F, 1.0F, 7, 22, 1);
        event_wait(30);
        event_sound_play(1225, 100, -90, 0);
        event_wave_emitter_create(-73.0F, -1.3F, 223.6F, 0.2F, 1.0F, 6, 18, 2);
        event_wait(30);
        event_sound_play(1225, 110, -20, 0);
        event_wave_emitter_create(-66.0F, -2.3F, 234.6F, 0.3F, 1.0F, 6, 19, 3);
        event_wait(25);
        event_wave_emitter_create(-66.0F, -2.3F, 234.6F, 0.3F, 1.0F, 6, 19, 3);
        event_sound_play(1225, 80, 10, 0);
        event_wait(40);
        event_set_pos(event_get_player(2), -100.0F, 0.0F, 349.6F);
        event_set_ang(event_get_player(2), 0, 0xb300, 0);
        event_set_pos(ev0068_chao_0, -126.3F, 1.45F, 339.79F);
        event_set_pos(ev0068_chao_1, -124.4F, 1.45F, 345.3F);
        event_set_pos(ev0068_chao_2, -123.0F, 1.45F, 355.3F);
        event_set_ang(ev0068_chao_0, 0, 0xcc00, 0xfc00);
        event_set_ang(ev0068_chao_1, 0, 0xca00, 0xfc00);
        event_set_ang(ev0068_chao_2, 0, 0xb000, 0xfd00);
        event_set_pos(ev0068_chao_3, -101.8F, 4.2F, 349.6F);
        event_set_ang(ev0068_chao_3, 0, 0xb300, 0);
        event_set_ang(ev0068_chao_0, 0, 0x9400, 0xfc00);
        event_set_ang(ev0068_chao_1, 0, 0x9100, 0xfc00);
        event_set_ang(ev0068_chao_2, 0, 0x7000, 0xfd00);
        event_set_pos(ev0068_chao_3, -104.5F, 1.45F, 372.6F);
        event_set_ang(ev0068_chao_3, 0, 0x9f00, 0);
        event_set_pos(event_get_player(2), -100.0F, 0.0F, 369.6F);
        event_set_ang(event_get_player(2), 0, 0x8000, 0);
        event_clear_motions(event_get_player(2));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -95.2F, 6.13F, 364.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x800, 0x5d87, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 4, -95.9F, 6.32F, 365.4F);
        event_voice_play(1455);
        event_play_action(event_get_player(2), &ev0068_tikal_action_3,
                          &tikal_texlist, 1.1F, 0, 0);
        event_look_clear(event_get_player(2));
        event_wait(30);
        event_look_at(event_get_player(2), -63.4F, 18.49F, 300.3F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -77.0F, 11.37F, 324.65F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa00, 0x6400, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -60.92F, 15.04F, 305.92F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -119.15F, 3.95F, 364.87F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x100, 0x9f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 30, -104.9F, 2.15F, 352.19F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 30, 0x700, 0x9400, 0xff00);
        event_chaos0_rise();
        event_wait(30);
        event_clear_motions(event_get_player(2));
        event_look_clear(event_get_player(2));
        event_set_pos(event_get_player(2), -65.1F, 0.0F, 301.4F);
        event_set_ang(event_get_player(2), 0, 0x6b00, 0);
        event_sound_start(1, 1226, 1800);
        event_sound_fade_pan(1, 30, 1);
        event_sound_fade_volume(1, 60, 1);
        event_wait(1);
        event_sound_fade_volume(1, 128, 140);
        event_chaos0_set_pos(-42.1F, -3.0F, 238.26F);
        event_chaos0_set_ang(0, 0x4900, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -32.1F, 0.013F, 222.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x88c, 0x6eb9, 0x200);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x238e);
        event_wait(4);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -35.8F, 3.7F, 231.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0, 0x73b9, 0x200);
        /* Chaos 0 rises 0.1 a step from the -3.0 set above to -0.8. the
         * script skips -1.0 and sets -0.8 twice */
        event_chaos0_set_pos(-42.1F, -2.9F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.8F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.7F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.6F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.5F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.4F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.3F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.2F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.1F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -2.0F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.9F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.8F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.7F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.6F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.5F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.4F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.3F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.2F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -1.1F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -0.9F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -0.8F, 238.26F);
        event_wait(1);
        event_chaos0_set_pos(-42.1F, -0.8F, 238.26F);
        event_wait(75);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x31c7);
        event_wait(1);
        event_set_pos(event_get_player(2), -100.0F, 0.0F, 369.6F);
        event_set_ang(event_get_player(2), 0, 0x6b00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -71.2F, 16.26F, 314.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa00, 0x6c00, 0x100);
        event_camera_set_pos(EVENT_EASE_LINEAR, 28, -95.7F, 9.3F, 360.66F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfa00, 0x6c00, 0xff00);
        event_sound_start(2, SE_TIKAL_CHIME, 90);
        event_wait(30);
        event_sound_fade_volume(1, -20, 80);
        event_set_pos(event_get_player(0), -66.8F, -0.01F, 400.35F);
        event_set_ang(event_get_player(0), 0x4f0, 0x8200, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -69.6F, 6.49F, 376.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0x8500, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 32, -67.55F, 6.49F, 393.77F);
        ev0068_fade = event_fade_create(0.016F, 0.0F, 1.0F, 1.0F, 1.0F);
        event_wait(90);
        event_chaos0_kill();
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_voice_stop();
        event_move_stop_all();
        event_reset_player(0);
        event_kill_player(2);
        EVENT_KILL(ev0068_fade);
        event_kill_task(&ev0068_chao_0);
        event_kill_task(&ev0068_chao_1);
        event_kill_task(&ev0068_chao_2);
        event_kill_task(&ev0068_chao_3);
        event_chaos0_kill();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0068_textures[] = {
    {"TIKAL", &tikal_texlist},
    {"EV_ALIFE", &event_chao_texlist},
    {"WAVE7_WA", &event_wave_texlist},
    {"CHAOS0", &event_chaos0_texlist},
    {"EV_CHAOS0_MANJU", &event_chaos0_water_texlist},
    {"CHAOS_SURFACE", &chaos_surface_texlist},
    {NULL, NULL},
};
