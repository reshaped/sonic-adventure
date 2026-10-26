/* EV00B9, E-102's story: E-102 arrives in the past, scans it under a green
 * tint, and finds no record of where he is */
#include "ev00b9/script.h"
#include "event/play.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 29
#define TEXT_4_LENGTH 13
#include "ev00b9/script/text.inc"

/* set the fade's speed, alpha or color, if there is a fade */
/* @hack spelling: each macro's test and call as one expression. SHC then keeps
 * 0.0F in FR15 (FLDI0 FR15 at 0x0cb80088) and event_voice_play's address in R8
 * (0x0cb8028e). as ifs, FR14 and FR15 trade, R8 takes event_message_show's
 * address, and 31 of ev00b9_script's 667 words differ */
#define SET_FADE_SPEED(speed)                                                  \
    (ev00b9_fade != NULL && (event_fade_set_speed(ev00b9_fade, speed), 0))
#define SET_FADE_ALPHA(alpha)                                                  \
    (ev00b9_fade != NULL && (event_fade_set_alpha(ev00b9_fade, alpha), 0))
#define SET_FADE_COLOR(red, green, blue)                                       \
    (ev00b9_fade != NULL &&                                                    \
     (event_fade_set_color(ev00b9_fade, red, green, blue), 0))

static struct task *ev00b9_fade = NULL;

void ev00b9_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_take_control();
        ev00b9_fade = event_fade_create(-0.0012F, 1.0F, 1.0F, 1.0F, 1.0F);
        event_wait(1);
        event_pads_off();
        event_set_pos(event_get_player(0), 6.7F, -40.5F, 607.9F);
        event_wait(1);
        event_camera_take();
        event_set_pos(event_get_player(0), 0.0F, -46.0F, 1396.5F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        /* the sound file of EV00BA, the event that follows */
        sound_load_file("e_00ba.mlt");
        event_sound_init(3);
        event_sound_start(1, SE_TIKAL_CHIME, 90);
        event_wait(30);
        SET_FADE_SPEED(-0.016F);
        event_set_pos(event_get_player(0), 0.0F, -46.0F, 1396.5F);
        event_set_ang(event_get_player(0), 0, 0x8000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 21.8F, -44.0F, 1375.7F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0f00, 0x6300, 0x0300);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 140, 3.4F, -42.2F, 1358.97F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 140, 0x0f00, 0x7e00, 0xfd00);
        event_wait(140);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -8.25F, -45.8F, 1383.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1300, 0x9a82, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 130, -8.46F, -34.7F, 1383.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 130, 0x0900, 0x9d82, 0xff00);
        event_wait(130);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -23.78F, -31.11F, 1400.89F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0100, 0xc382, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 35, -23.399F, -34.16F,
                             1402.81F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 35, 0x0100, 0xcd82, 0xff00);
        event_wait(35);
        event_camera_set_pos(EVENT_EASE_LINEAR, 130, -19.12F, -34.82F, 1424.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 130, 0x0100, 0xf482, 0xff00);
        event_voice_play(1714);
        event_message_show(TEXT_1);
        event_wait(120);
        event_camera_set_pos(EVENT_EASE_LINEAR, 120, -5.32F, -30.36F, 1433.4F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 120, 0x0100, 0xfa82, 0xff00);
        event_wait(150);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 5.14F, -27.0F, 1386.5F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf900, 0x6a80, 0xff00);
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_LINEAR, 3, 4.15F, -27.33F, 1388.23F);
        event_voice_play(1715);
        event_message_show(TEXT_2);
        event_sound_play(13, 90, 0, 0);
        event_wait(8);
        event_sound_play(21, 120, 0, 0);
        event_wait(20);
        event_message_close();
        /* the green tint set below holds while E-102 scans: added to an
         * alpha of 0.3, a speed of 1e-12 rounds away */
        SET_FADE_SPEED(1e-12F);
        SET_FADE_ALPHA(0.3F);
        SET_FADE_COLOR(0.1F, 0.7F, 0.2F);
        event_sound_play(14, 128, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 35.3F, -47.86F, 1318.37F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0, 0xe680, 0x0700);
        event_camera_set_pos(EVENT_EASE_LINEAR, 9, 39.993F, -47.864F, 1311.89F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 9, 0, 0xe680, 0xff00);
        event_wait(30);
        event_sound_play(14, 128, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -36.9F, -5.3F, 1343.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0x2080, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 8, -141.9F, 47.3F, 1241.0F);
        event_wait(34);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 0.0F, -27.3F, 1389.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0500, 0, 0);
        event_sound_play(14, 128, 0, 0);
        event_camera_set_fov(EVENT_EASE_LINEAR, 7, 0x2fa5);
        event_wait(7);
        event_camera_set_fov(EVENT_EASE_LINEAR, 6, 0x3111);
        event_wait(7);
        event_camera_set_fov(EVENT_EASE_LINEAR, 13, 0x2b61);
        event_wait(15);
        event_camera_set_fov(EVENT_EASE_LINEAR, 60, 0x1c72);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 30, 0x1b06);
        event_wait(40);
        event_sound_play(22, 128, 0, 0);
        SET_FADE_SPEED(-0.008F);
        event_wait(60);
        event_camera_set_fov(EVENT_EASE_LINEAR, 1, 0x31c7);
        event_wait(1);
        EVENT_KILL(ev00b9_fade);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -141.9F, 47.3F, 1241.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0e00, 0x2080, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -8.25F, -45.8F, 1383.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1300, 0x9a82, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -8.46F, -34.7F, 1383.4F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 100, 0x0900, 0x9d82, 0xff00);
        event_wait(15);
        event_voice_play(1716);
        event_message_show(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_wait(20);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 28.7F, -32.6F, 1361.9F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0b00, 0x60f0, 0x0500);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 160, 60.85F, -47.3F, 1330.8F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 160, 0x0b00, 0x5ff3, 0x0500);
        event_wait(18);
        event_voice_play(1717);
        event_message_show(TEXT_4);
        event_wait(170);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_reset_player(0);
        EVENT_KILL(ev00b9_fade);
        event_voice_stop();
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00b9_textures[] = {{NULL, NULL}};
