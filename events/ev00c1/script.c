/* EV00C1, E-102's story: E-102 reaches the deck, where Eggman sends him
 * after Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/egg_lift.h"
#include "adv0100/gate.h"
#include "ev00c1/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/amy.h"
#include "player/tails.h"
#include "player/sonic.h"
#include "player/textures.h"
#include "progress/progress.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 33
#define TEXT_2_LENGTH 44
#define TEXT_3_LENGTH 25
#define TEXT_4_LENGTH 31
#define TEXT_5_LENGTH 16
#include "ev00c1/script/text.inc"

extern NJS_ACTION e102_slide_stop_action;
extern NJS_ACTION e102_slide_stand_action;

extern NJS_ACTION ev00c1_sonic_action;
extern NJS_ACTION ev00c1_tails_action;
extern NJS_ACTION ev00c1_amy_action;
extern NJS_TEXLIST event_eggmobile_texlist;

static struct task *ev00c1_fade = NULL;

void ev00c1_script(int phase)
{
    /* @hack spelling: the lift's flag through a pointer, set in the store of
     * LIFT_LOW_NOW, which SHC keeps in a stack word (MOV.L R12,@R15 at
     * 0x0cb802d8, read back at 0x0cb80434); by name, the frame loses the
     * word, the code runs 4 bytes long and 901 of 945 words differ */
    Uint8 *lift_flag;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        /* the screen holds black until the speed set below fades it out */
        ev00c1_fade =
            event_fade_create(-EVENT_FADE_STILL, 1.0F, 0.0F, 0.0F, 0.0F);
        event_reset_player(0);
        event_set_pos(event_get_player(0), -5.6F, 1528.6F, 3504.3F);
        event_set_ang(event_get_player(0), 0, 0x9000, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &e102_slide_stop_action,
                          &e102_texlist, 1.0F, 0, 0);
        event_play_action(event_get_player(0), &e102_slide_stand_action,
                          &e102_texlist, 1.0F, EVENT_REPEAT, 0);
        event_create_player(3, tails_step, 13.0F, 1525.0F, 3385.0F, 0, 0, 0);
        event_create_player(4, sonic_step, 0.0F, 1525.0F, 3380.0F, 0, 0x8000,
                            0);
        event_create_player(5, amy_step, -20.0F, 1525.6F, 3292.66F, 0, 0x4b70,
                            0);
        event_wait(1);
        event_set_pos(event_get_player(3), 13.0F, 1525.0F, 3385.0F);
        event_set_ang(event_get_player(3), 0, 0, 0);
        event_set_pos(event_get_player(4), 0.0F, 1525.8F, 3370.0F);
        event_set_ang(event_get_player(4), 0, 0, 0);
        event_set_pos(event_get_player(5), -9.0F, 1525.6F, 3290.66F);
        event_set_ang(event_get_player(5), 0, 0, 0);
        event_clear_motions(event_get_player(3));
        event_play_action(event_get_player(3), &ev00c1_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(4));
        event_play_action(event_get_player(4), &ev00c1_sonic_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_clear_motions(event_get_player(5));
        event_play_action(event_get_player(5), &ev00c1_amy_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(4), event_get_player(5), 16.0F,
                           16.0F, 0.0F);
        event_look_at_task(event_get_player(5), event_get_player(0), 0.0F,
                           14.0F, 0.0F);
        event_eggmobile_create(18.8F, 1533.1F, 3288.6F, 0, 0x4000, 0);
        event_wait(1);
        event_flags[FLAG_E102_EC_EGG_LIFT] = 1;
        *(lift_flag = &event_flags[FLAG_LIFT]) = LIFT_LOW_NOW;
        stream_request_music(MUSIC_EGCARER1);
        event_wait(20);
        if (ev00c1_fade != NULL) {
            event_fade_set_speed(ev00c1_fade, -0.016F);
        }
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 0.9F, 1530.81F, 3441.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xfed0, 0xfae1, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 85, 0.9F, 1541.81F, 3441.0F);
        event_wait(85);
        /* the camera shakes at the top of its rise. it bobs 1 unit up and
         * back, then 0.2 up and back, a move every 3 steps. its x angle
         * tips with each bob */
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 0.9F, 1542.81F, 3441.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0, 0xfae1, 0);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 0.9F, 1541.81F, 3441.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfed0, 0xfae1, 0);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 0.9F, 1542.01F, 3441.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xff00, 0xfae1, 0);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 0.9F, 1541.81F, 3441.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0xfed0, 0xfae1, 0);
        event_wait(3);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 25, 0xfbd0, 0xfae1, 0);
        event_wait(40);
        *lift_flag = LIFT_HIGH_NOW;
        event_set_pos(event_get_player(0), 0.0F, 1544.4F, 3457.8F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 0.1F, 1547.7F, 3432.9F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xe5cd, 0x7e01, 0);
        event_wait(6);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 40, 0.7F, 1539.4F, 3422.23F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 40, 0x0b5d, 0x7e01, 0);
        event_wait(50);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, 2.18F, 1531.8F, 3408.23F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x0b5d, 0x7e01, 0);
        event_wait(90);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 42.1F, 1527.6F, 3417.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0491, 0x1af2, 0x0200);
        event_set_pos(event_get_player(0), 4.0F, 1546.6F, 3405.3F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_voice_play(0x6e7);
        event_message_show(TEXT_1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, 20.15F, 1528.1F, 3422.8F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0c91, 0x11f2, 0);
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F,
                           11.0F, 0.0F);
        event_look_at_task(event_get_player(4), event_get_player(0), 0.0F,
                           11.0F, 0.0F);
        event_voice_wait();
        event_message_close();
        event_wait(40);
        if (ev00c1_fade != NULL) {
            task_kill(ev00c1_fade);
            ev00c1_fade = NULL;
        }
        event_set_pos(event_get_player(0), 4.0F, 1530.6F, 3405.3F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_play_action(event_get_player(5), &ev00c1_amy_action, &amy_texlist,
                          1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 28.2F, 1551.31F, 3289.3F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xec91, 0x38e1, 0);
        event_voice_play(0x43f);
        event_message_show(TEXT_2);
        event_camera_set_pos(EVENT_EASE_LINEAR, 40, 26.44F, 1550.3F, 3289.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xeb91, 0x38e1, 0);
        event_voice_wait();
        event_look_clear(event_get_player(3));
        event_look_at_task(event_get_player(3), event_get_player(0), 0.0F,
                           15.0F, 0.0F);
        event_look_clear(event_get_player(4));
        event_look_at_task(event_get_player(4), event_get_player(0), 0.0F,
                           15.0F, 0.0F);
        event_voice_play(0x440);
        event_message_show(TEXT_3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 29.87F, 1528.4F, 3367.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0591, 0x63e1, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 90, 6.5F, 1528.4F, 3355.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 90, 0x0591, 0x7be1, 0);
        event_voice_wait();
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 3.62F, 1530.7F, 3370.1F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c7b, 0x806d, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 55, 3.8F, 1534.7F, 3382.5F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 55, 0x0c7b, 0x806d, 0);
        event_voice_play(0x6e8);
        event_message_show(TEXT_4);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, 19.7F, 1526.8F, 3417.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0b22, 0x1214, 0);
        event_voice_play(0x441);
        event_message_show(TEXT_5);
        event_eggmobile_fly_to(18.8F, 1552.1F, 3288.6F, 43);
        event_wait(1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 160, 20.76F, 1535.4F, 3419.2F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 160, 0x0a22, 0x1214, 0);
        event_wait(50);
        event_message_close();
        event_eggmobile_turn(0, -0x4000, 0, 50, 0);
        event_wait(54);
        event_eggmobile_fly_turn(-74.3F, 1570.5F, 3240.9F, 0, -0x4000, 0, 85,
                                 0);
        event_wait(89);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_wait(1);
        event_eggmobile_fly_to(-186.8F, 1605.9F, 2450.4F, 130);
        event_wait(130);
        event_message_close();
        event_wait(30);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_kill_player(5);
        event_kill_player(3);
        event_kill_player(4);
        event_eggmobile_kill();
        EVENT_KILL(ev00c1_fade);
        event_flags[FLAG_LIFT] = LIFT_LOW_NOW;
        event_flags[FLAG_GATE] = GATE_SHUT_NOW;
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00c1_textures[] = {
    {"AMY", &amy_texlist},
    {"MILES", &tails_texlist},
    {"SONIC", &sonic_texlist},
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {NULL, NULL},
};
