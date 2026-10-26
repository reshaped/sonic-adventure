/* EV0024: Eggman flies off in the Egg Viper, and Sonic runs after him */
#include <ninja.h>

#include "b_egm3/egg_viper_event.h"
#include "ev0024/script.h"
#include "ev0024/sonic_action.h"
#include "ev0024/wreckage_drop.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 19
#define TEXT_2_LENGTH 34
#define TEXT_3_LENGTH 21
#include "ev0024/script/text.inc"

/* set the speed of the fade in slot, unless slot is NULL */
/* @hack spelling: the test and the call as one expression, as
 * src/event/play.h's EVENT_KILL has them. SHC then loads 3.0F and 0.4F, the
 * wreckage drops' speed and scale, into FR13 and FR14 at 0x0cb804e0 and
 * 0x0cb804e4. it keeps them there for all three drops. as an if, the two trade
 * registers, and 6 of ev0024_script's 2066 words differ */
#define SET_FADE_SPEED(slot, speed)                                            \
    ((slot) != NULL && (event_fade_set_speed((slot), (speed)), 0))

extern NJS_TEXLIST ev0024_wreckage_texlist;

/* Sonic's run and jump, a key a step. the stick's direction runs from 8 down to
 * 2, in 256ths of a turn. it is pushed 234 to 255 out of 255. A is held for the
 * 9 keys from key 29 */
static const struct event_pad_key ev0024_run_keys[41] = {
#include "ev0024/script/ev0024_run_keys.inc"
};

static const struct event_pad_recording ev0024_run = {
    ev0024_run_keys, sizeof(ev0024_run_keys) / sizeof(ev0024_run_keys[0])};

static struct task *ev0024_fade = NULL;
static struct task *ev0024_drop_0 = NULL;
static struct task *ev0024_drop_1 = NULL;
static struct task *ev0024_drop_2 = NULL;

void ev0024_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        sound_load_file("e_0024.mlt");
        event_sound_init(5);
        event_reset_player(0);
        event_camera_take();
        event_pads_off();
        event_take_control();
        stream_request_music(MUSIC_FINALEG2);
        ev0024_fade = event_fade_create(1.0F, 0.0F, 0.0F, 0.0F, 0.0F);
        event_wait(1);
        egg_viper_event_create(-65.99F, -49840.497F, -0.467F, -0x3fff);
        event_set_pos(event_get_player(0), -632.43F, -49934.161F, 0.34F);
        event_set_ang(event_get_player(0), 0, 0x4000, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -650.78F, -49921.64F,
                             -0.75F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf800, 0xbc00, 0);
        event_camera_follow(event_get_player(0));
        SET_FADE_SPEED(ev0024_fade, -0.00625F);
        event_run_to_speed(event_get_player(0), -389.41F, -49940.68F, -3.94F,
                           0.8F, 1.1F);
        event_wait(80);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -350.335F, -49936.27F,
                             -30.29F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xa00, 0x5400, 0);
        event_run_wait(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0024_run);
        event_wait(60);
        event_set_pos(event_get_player(0), -389.3F, -49940.66F, -0.67F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -299.854F, -49939.51F,
                             10.88F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 0, event_get_player(0),
                                  0.0F, 5.0F, 0.0F, 0);
        event_camera_follow(event_get_player(0));
        event_clear_motions(event_get_player(0));
        event_play_pad(0, &ev0024_run);
        event_wait(50);
        event_camera_follow_clear();
        event_wait(30);
        event_camera_look_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -303.81F, -49999.72F,
                             1.456F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xe00, 0x4400, 0);
        event_play_action(event_get_player(0), &ev0024_sonic_action,
                          &sonic_texlist, 1.0F, 0, 8);
        event_wait(80);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -327.017F, -49992.78F,
                             3.267F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1300, 0xbe00, 0);
        event_wait(60);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -82.629F, -49858.243F,
                             -91.05F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xf00, 0x8a00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 60, -128.22F, -49858.24F,
                             -79.631F);
        event_wait(58);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -188.094F, -49821.82F,
                             31.158F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfd00, 0xce00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -177.354F, -49857.9F,
                             30.439F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0xf00, 0xd000, 0);
        event_wait(60);
        event_sound_play(1224, 0, 0, 0);
        egg_viper_event_open();
        egg_viper_event_talk();
        event_wait(60);
        event_voice_play(1143);
        event_message_show(TEXT_1);
        event_wait(1);
        event_voice_wait();
        event_voice_play(1144);
        event_message_show(TEXT_2);
        event_wait(1);
        event_voice_wait();
        egg_viper_event_hush();
        event_wait(5);
        event_message_close();
        egg_viper_event_shut();
        event_sound_play(1224, 0, 0, 0);
        event_wait(80);
        event_sound_start(0, 1226, 1800);
        egg_viper_event_fly();
        event_sound_fade_volume(0, -4, 300);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x1b00, 0xc000, 0);
        event_wait(200);
        event_sound_play(1225, 0, 0, 0);
        ev0024_drop_0 = wreckage_drop_create(10.071F, -49500.231F, 7.274F, 3.0F,
                                             0.4F, 80.0F, 15);
        ev0024_drop_1 = wreckage_drop_create(91.515F, -49500.16F, -16.011F,
                                             3.0F, 0.4F, 50.0F, 30);
        ev0024_drop_2 = wreckage_drop_create(-122.556F, -49500.231F, 174.875F,
                                             3.0F, 0.4F, 50.0F, 10);
        event_voice_play(1145);
        event_message_show(TEXT_3);
        vibration_start(0, 4, 30, 10);
        /* the camera shakes about the angles 0x1b00, 0xc000 and 0. each
         * step it eases toward angles 0x100 to one side of these, then to the
         * other */
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1c00, 0xc100, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1b00, 0xc000, 0xff00);
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1a00, 0xbf00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1b00, 0xc000, 0xff00);
        event_wait(1);
        vibration_start(0, 0, 0, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -325.294F, -49997.575F,
                             -20.613F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1000, 0x8b00, 0);
        event_camera_follow(event_get_player(0));
        event_run_to_speed(event_get_player(0), -198.84F, -50000.0F, -6.05F,
                           1.5F, 2.0F);
        event_wait(1);
        vibration_start(0, 5, 40, 10);
        /* the camera shakes again, about 0x1000, 0x8b00 and 0 */
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        event_wait(1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0xf00, 0x8c00, 0x100);
        event_wait(1);
        vibration_start(0, 0, 0, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 2, 0x1100, 0x8a00, 0xff00);
        egg_viper_event_kill();
        event_run_wait(event_get_player(0));
        SET_FADE_SPEED(ev0024_fade, 0.0125F);
        event_wait(160);
        event_sound_start(1, 705, 1800);
        event_set_pos(event_get_player(0), 260.0F, 100.0F, 0.0F);
        event_wait(10);
        SET_FADE_SPEED(ev0024_fade, -0.016666666F);
        event_wait(30);
        event_play_action(event_get_player(0), &ev0024_sonic_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_follow_clear();
        event_camera_take();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, 271.725F, 15.634F, -10.011F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x700, 0x5b00, 0xff00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 200, 268.664F, 16.318F,
                             -7.531F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 200, 0x700, 0x5e00, 0xff00);
        event_wait(100);
        event_sound_stop(1);
        break;
    case EVENT_END:
        event_sound_stop_all();
        event_camera_release();
        event_pads_on();
        EVENT_KILL(ev0024_fade);
        event_set_pos(event_get_player(0), 260.0F, 13.0F, 0.0F);
        vibration_start(0, 0, 0, 0);
        egg_viper_event_kill();
        EVENT_KILL(ev0024_drop_0);
        EVENT_KILL(ev0024_drop_1);
        EVENT_KILL(ev0024_drop_2);
        event_reset_player(0);
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev0024_textures[] = {
    {"F_EGG_ZANGAI", &ev0024_wreckage_texlist},
    {NULL, NULL},
};
