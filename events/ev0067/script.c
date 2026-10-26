/* EV0067, Amy's story: Amy arrives in the past and asks where she is */
#include "ev0067/script.h"
#include "event/event.h"
#include "event/play.h"
#include "shared/fade.h"
#include "shared/move.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 34
#define TEXT_3_LENGTH 3
#define TEXT_4_LENGTH 7
#include "ev0067/script/text.inc"

static struct task *ev0067_fade = NULL;
/* a point for Amy to look at: an actor that plays no motion, so it isn't
 * drawn */
static struct task *ev0067_target = NULL;

struct texture_pvm ev0067_textures[] = {{NULL, NULL}};

void ev0067_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        sound_load_file("e_0068.mlt");
        event_sound_init(3);
        event_reset_player(0);
        event_place_actor(&ev0067_target, 0.0F, 10.0F, 1371.7F, 0, 0, 0);
        event_wait(1);
        event_set_flags(ev0067_target, EVENT_IGNORE_GROUND);
        event_set_pos(ev0067_target, 0.0F, 10.0F, 1371.7F);
        event_set_ang(ev0067_target, 0, 0, 0);
        event_sound_start(1, SE_TIKAL_CHIME, 90);
        ev0067_fade = event_fade_create(-0.0012F, 1.0F, 1.0F, 1.0F, 1.0F);
        event_wait(30);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then keeps the
         * camera's x angle, 0x0f00, in R9 (0x0cb80130) and its z angle,
         * 0xfd00, in R8 (0x0cb80150). as an if, the two registers trade, and
         * 21 of ev0067_script's 494 words differ */
        ev0067_fade != NULL && (event_fade_set_speed(ev0067_fade, -0.016F), 0);
        event_set_pos(event_get_player(0), 0.0F, 10.0F, 1396.5F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_look_at_task(event_get_player(0), ev0067_target, 0.0F, 0.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 21.8F, 12.0F, 1375.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0x6300, 0x0300);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, 3.4F, 10.2F, 1358.97F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x0f00, 0x7e00, 0xfd00);
        event_wait(140);
        EVENT_KILL(ev0067_fade);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.68F, 13.1F, 1381.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0x7e00, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, 0.45F, 14.3F, 1385.89F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0f00, 0x8000, 0xfd00);
        event_move(ev0067_target, 0.0F, -42.0F, 1371.7F, 20.0F, -42.0F, 1378.0F,
                   10);
        event_wait(12);
        event_move(ev0067_target, 20.0F, -42.0F, 1378.0F, 0.0F, -42.0F, 1371.7F,
                   8);
        event_wait(8);
        event_move(ev0067_target, 0.0F, -42.0F, 1371.7F, -20.0F, -42.0F,
                   1378.0F, 10);
        event_wait(12);
        event_move(ev0067_target, -20.0F, -42.0F, 1378.0F, 0.0F, -42.0F,
                   1371.7F, 8);
        event_wait(8);
        event_move(ev0067_target, 0.0F, -42.0F, 1371.7F, 20.0F, -42.0F, 1378.0F,
                   10);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_3);
        event_voice_play(1451);
        event_message_show(TEXT_1);
        event_wait(22);
        event_move(ev0067_target, 20.0F, -42.0F, 1378.0F, 0.0F, -42.0F, 1371.7F,
                   8);
        event_wait(8);
        event_look_clear(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -11.5F, 20.7F, 1429.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0700, 0xfb00, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, 10.3F, 20.3F, 1431.6F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 130, 0x0700, 0x0200, 0xfd00);
        event_wait(130);
        event_look_at_task(event_get_player(0), ev0067_target, 0.0F, 0.0F,
                           0.0F);
        event_move(ev0067_target, 0.0F, -25.0F, 1326.0F, 0.0F, -25.0F, 1326.0F,
                   30);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 11.4F, 40.95F, 1380.2F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xde00, 0x6600, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, -5.9F, 18.9F, 1381.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0xf600, 0x9100, 0xfd00);
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(0), TEXT_4);
        event_voice_play(1452);
        event_message_show(TEXT_2);
        event_wait(190);
        event_talk_stop(event_get_player(0));
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_sound_stop_all();
        event_move_stop_all();
        event_reset_player(0);
        EVENT_KILL(ev0067_fade);
        event_voice_stop();
        break;
    case EVENT_UNLOAD:
        break;
    }
}
