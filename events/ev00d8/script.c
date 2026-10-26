/* EV00D8, Big's story: Big follows Froggy aboard the Egg Carrier, and smells
 * him behind a door */
#include <ninja.h>

#include "ev00d8/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "sound/stream.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 58
#define TEXT_3_LENGTH 25
#define TEXT_4_LENGTH 39
#include "ev00d8/script/text.inc"

/* @hack type: big_stand_action declared as an array, which script.c passes by
 * its name. big_models.h declares it as one NJS_ACTION. SHC then keeps its
 * address in R9 across its five plays from 0x0cb80092. as one NJS_ACTION passed
 * as &big_stand_action, its address loads from the pool at each play, the
 * function runs 8 bytes short, and 285 of 378 words differ */
extern NJS_ACTION big_stand_action[];

void ev00d8_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_wait(1);
        event_reset_player(0);
        event_set_pos(event_get_player(0), 0.0F, 48.5F, -371.0F);
        event_set_ang(event_get_player(0), 0, 0xfea4, 0x0119);
        event_wait(1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 8);
        stream_request_music(MUSIC_EGCARER1);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -123.39F, 53.13F, -384.26F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x064c, 0x9100, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 80, -63.07F, 54.86F, -420.48F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 80, 0x064c, 0x9400, 0);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -56.9F, 54.86F, -423.78F);
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -5.15F, 56.91F, -350.06F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0c4c, 0xe700, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 270, -8.42F, 56.9F, -352.36F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 270, 0x0c4c, 0xe700, 0);
        event_wait(30);
        event_voice_play(1839);
        event_message_wait(60, TEXT_1);
        event_message_close();
        event_voice_play(1840);
        event_wait(10);
        event_message_wait(120, TEXT_2);
        event_message_close();
        event_wait(30);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -6.54F, 60.88F, -361.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0e4c, 0xe500, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -8.55F, 60.88F, -363.92F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0x0e4c, 0xd800, 0);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), big_stand_action, &big_texlist,
                          3.0F, EVENT_REPEAT, 0);
        event_voice_play(1841);
        event_wait(20);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(10);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), big_stand_action, &big_texlist,
                          3.0F, EVENT_REPEAT, 0);
        event_voice_play(1841);
        event_wait(20);
        event_play_action(event_get_player(0), big_stand_action, &big_texlist,
                          1.0F, EVENT_REPEAT, 0);
        event_wait(30);
        event_voice_play(1842);
        event_message_wait(60, TEXT_3);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -16.29F, 73.48F, -436.13F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0xf84c, 0x8300, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x1c72);
        event_camera_set_pos(EVENT_EASE_LINEAR, 135, -16.13F, 85.26F, -433.88F);
        event_wait(10);
        event_voice_play(1843);
        event_message_wait(60, TEXT_4);
        event_message_close();
        event_wait(60);
        break;
    case EVENT_END:
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00d8_textures[] = {{NULL, NULL}};
