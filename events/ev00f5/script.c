/* EV00F5: Knuckles tells Sonic and Tails that the emeralds were stolen and
 * Chaos still lives, and Tikal's light comes to Sonic */
#include <ninja.h>
#include <sg_xpt.h>

#include "ev00f5/eggman_action_1.h"
#include "ev00f5/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/eggman.h"
#include "player/knuckles.h"
#include "player/tails.h"
#include "player/textures.h"
#include "shared/eggmobile.h"
#include "shared/fade.h"
#include "shared/light.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 21
#define TEXT_2_LENGTH 46
#define TEXT_3_LENGTH 36
#define TEXT_4_LENGTH 38
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 30
#define TEXT_7_LENGTH 28
#define TEXT_8_LENGTH 42
#define TEXT_9_LENGTH 54
#define TEXT_10_LENGTH 28
#define TEXT_11_LENGTH 19
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 3
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 2
#define TEXT_18_LENGTH 4
#define TEXT_19_LENGTH 5
#define TEXT_20_LENGTH 6
#define TEXT_21_LENGTH 1
#define TEXT_22_LENGTH 2
#define TEXT_23_LENGTH 1
#include "ev00f5/script/text.inc"

/* the node of Sonic's model whose sibling is his -z fingers */
extern NJS_OBJECT dat_8c1e0958;
extern NJS_OBJECT sonic_fingers_minus_z;
extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_sprint_action;
extern NJS_ACTION sonic_skid_action;
extern NJS_ACTION sonic_jump_land_action;
extern NJS_ACTION tails_sprint_action;
extern NJS_ACTION tails_skid_action;
extern NJS_ACTION tails_stand_action;

extern NJS_OBJECT ev00f5_sonic_fingers;
extern NJS_ACTION ev00f5_sonic_action_0;
extern NJS_ACTION ev00f5_sonic_action_1;
extern NJS_ACTION ev00f5_sonic_action_2;
extern NJS_ACTION ev00f5_sonic_action_3;
extern NJS_ACTION ev00f5_sonic_action_4;
extern NJS_ACTION ev00f5_sonic_action_5;
extern NJS_ACTION ev00f5_tails_action;
extern NJS_ACTION ev00f5_knuckles_action_0;
extern NJS_ACTION ev00f5_knuckles_action_1;
extern NJS_ACTION ev00f5_knuckles_action_2;
extern NJS_ACTION ev00f5_knuckles_action_3;
extern NJS_ACTION ev00f5_eggman_action_0;
extern NJS_TEXLIST event_eggmobile_texlist;

/* @hack spelling: the sibling as (&dat_8c1e0958)->sibling. SHC then pools the
 * node's address for each of the three stores and stores 48 bytes past it,
 * as MOV.L R2,@(48,R3) at 0x0cb80ddc does. as dat_8c1e0958.sibling, the pool
 * words hold the address of the node's sibling field, and 84 of 1557 words
 * differ */
#define SET_SONIC_FINGERS(fingers) ((&dat_8c1e0958)->sibling = &(fingers))

static struct event_path_point ev00f5_light_points[123] = {
    {0, -34, 0, {-41.69421F, -247.2999F, 2621.178F}},
    {0, -34, 0, {-42.03262F, -247.2999F, 2621.4622F}},
    {0, -34, 0, {-42.37103F, -247.2999F, 2621.7464F}},
    {0, -34, 0, {-42.70945F, -247.2999F, 2622.0311F}},
    {0, -34, 0, {-43.04786F, -247.2999F, 2622.3155F}},
    {0, -34, 0, {-43.38626F, -247.2999F, 2622.5997F}},
    {0, -34, 0, {-43.72468F, -247.2999F, 2622.8843F}},
    {7, -40, 0, {-44.06309F, -247.2999F, 2623.1685F}},
    {13, -48, 0, {-44.44708F, -247.44521F, 2623.3265F}},
    {13, -50, 0, {-44.84801F, -247.57094F, 2623.4634F}},
    {16, -51, 0, {-45.2432F, -247.72922F, 2623.5813F}},
    {17, -52, 0, {-45.63557F, -247.90056F, 2623.691F}},
    {18, -53, 0, {-46.02489F, -248.08426F, 2623.7913F}},
    {19, -53, 0, {-46.40909F, -248.28255F, 2623.8826F}},
    {21, -54, 0, {-46.79043F, -248.48813F, 2623.9705F}},
    {22, -54, 0, {-47.16479F, -248.70902F, 2624.0506F}},
    {23, -54, 0, {-47.53286F, -248.9408F, 2624.129F}},
    {25, -53, 0, {-47.8931F, -249.18335F, 2624.211F}},
    {26, -51, 0, {-48.23948F, -249.44138F, 2624.3042F}},
    {28, -51, 0, {-48.57077F, -249.71831F, 2624.3985F}},
    {30, -50, 0, {-48.89098F, -250.00771F, 2624.4935F}},
    {31, -50, 0, {-49.20266F, -250.30547F, 2624.591F}},
    {32, -48, 0, {-49.50257F, -250.61342F, 2624.6944F}},
    {33, -47, 0, {-49.79237F, -250.92896F, 2624.8025F}},
    {34, -46, 0, {-50.07475F, -251.24965F, 2624.9156F}},
    {34, -44, 0, {-50.34529F, -251.57773F, 2625.0364F}},
    {35, -42, 0, {-50.60489F, -251.911F, 2625.1663F}},
    {35, -33, 0, {-50.85575F, -252.24541F, 2625.3096F}},
    {33, -15, 0, {-51.03381F, -252.56281F, 2625.5513F}},
    {32, -2, 0, {-51.09729F, -252.87444F, 2625.8567F}},
    {37, 5, 0, {-51.08113F, -253.1907F, 2626.164F}},
    {42, 8, 0, {-51.04137F, -253.5601F, 2626.401F}},
    {44, 10, 0, {-51.00098F, -253.95078F, 2626.6036F}},
    {44, 12, 0, {-50.95045F, -254.34212F, 2626.8025F}},
    {44, 14, 0, {-50.88733F, -254.73148F, 2627.0022F}},
    {43, 16, 0, {-50.81303F, -255.1178F, 2627.204F}},
    {42, 18, 0, {-50.72715F, -255.49994F, 2627.4085F}},
    {40, 19, 0, {-50.62798F, -255.87552F, 2627.6192F}},
    {36, 19, 0, {-50.51234F, -256.23813F, 2627.8438F}},
    {32, 19, 0, {-50.38025F, -256.56257F, 2628.1121F}},
    {29, 20, 0, {-50.2332F, -256.8611F, 2628.4031F}},
    {28, 22, 0, {-50.06895F, -257.14621F, 2628.698F}},
    {27, 24, 0, {-49.88747F, -257.4202F, 2628.9935F}},
    {25, 26, 0, {-49.6882F, -257.6837F, 2629.2872F}},
    {24, 28, 0, {-49.46926F, -257.93693F, 2629.576F}},
    {24, 32, 0, {-49.22754F, -258.18274F, 2629.8521F}},
    {22, 39, 0, {-48.94847F, -258.42154F, 2630.0967F}},
    {20, 45, 0, {-48.61719F, -258.63285F, 2630.2984F}},
    {19, 50, 0, {-48.25874F, -258.83362F, 2630.4605F}},
    {18, 54, 0, {-47.8796F, -259.0273F, 2630.5784F}},
    {21, 60, 0, {-47.4893F, -259.21943F, 2630.6551F}},
    {-101, 62, -128, {-47.12561F, -259.46595F, 2630.6673F}},
    {-101, 55, -128, {-46.78712F, -259.74592F, 2630.621F}},
    {-105, 50, -128, {-46.43678F, -259.99552F, 2630.521F}},
    {-113, 47, -128, {-46.07338F, -260.202F, 2630.3792F}},
    {-125, 47, -128, {-45.68639F, -260.29706F, 2630.197F}},
    {121, 47, -128, {-45.27922F, -260.2484F, 2630.033F}},
    {116, 45, -128, {-44.88653F, -260.144F, 2629.8602F}},
    {111, 42, -128, {-44.52108F, -259.9865F, 2629.6685F}},
    {106, 38, -128, {-44.1907F, -259.78275F, 2629.4581F}},
    {102, 32, -128, {-43.90625F, -259.53614F, 2629.2281F}},
    {100, 23, -128, {-43.68913F, -259.25712F, 2628.9656F}},
    {100, 17, -128, {-43.53419F, -258.97574F, 2628.6622F}},
    {99, 14, -128, {-43.40259F, -258.68924F, 2628.3526F}},
    {97, 11, -128, {-43.30035F, -258.39274F, 2628.0411F}},
    {96, 7, -128, {-43.23092F, -258.08573F, 2627.731F}},
    {95, 4, -128, {-43.18406F, -257.77216F, 2627.424F}},
    {94, 0, -128, {-43.1664F, -257.45051F, 2627.121F}},
    {93, -4, -128, {-43.18755F, -257.12037F, 2626.8284F}},
    {92, -9, -128, {-43.23708F, -256.78486F, 2626.546F}},
    {91, -16, -128, {-43.32291F, -256.44233F, 2626.2811F}},
    {90, -24, -128, {-43.45261F, -256.0879F, 2626.052F}},
    {89, -29, -128, {-43.61247F, -255.72841F, 2625.8514F}},
    {90, -33, -128, {-43.78903F, -255.36951F, 2625.664F}},
    {90, -36, -128, {-43.99133F, -255.01523F, 2625.4935F}},
    {91, -39, -128, {-44.20748F, -254.66518F, 2625.3331F}},
    {92, -42, -128, {-44.44005F, -254.32018F, 2625.1834F}},
    {93, -44, -128, {-44.69021F, -253.98282F, 2625.047F}},
    {94, -46, -128, {-44.95125F, -253.65018F, 2624.918F}},
    {95, -48, -128, {-45.23217F, -253.3288F, 2624.804F}},
    {96, -50, -128, {-45.52013F, -253.0116F, 2624.6951F}},
    {98, -52, -128, {-45.82893F, -252.7095F, 2624.602F}},
    {100, -54, -128, {-46.14753F, -252.41672F, 2624.5118F}},
    {105, -58, -128, {-46.4907F, -252.1494F, 2624.437F}},
    {110, -61, -128, {-46.87887F, -251.94077F, 2624.4068F}},
    {-14, -63, 0, {-47.2849F, -251.76677F, 2624.398F}},
    {-11, -61, 0, {-47.70254F, -251.62227F, 2624.404F}},
    {-9, -60, 0, {-48.12876F, -251.50647F, 2624.4222F}},
    {-7, -60, 0, {-48.56037F, -251.41521F, 2624.4493F}},
    {-6, -60, 0, {-48.99526F, -251.3427F, 2624.4805F}},
    {-6, -62, 0, {-49.42987F, -251.2646F, 2624.4947F}},
    {-4, -62, 0, {-49.86598F, -251.19391F, 2624.501F}},
    {-128, -63, -128, {-50.30669F, -251.1636F, 2624.5F}},
    {-122, -62, -128, {-50.74757F, -251.18718F, 2624.4876F}},
    {-113, -55, -128, {-51.18078F, -251.26685F, 2624.4617F}},
    {-101, -45, -128, {-51.53967F, -251.46886F, 2624.3277F}},
    {-86, -44, -128, {-51.79865F, -251.7865F, 2624.1685F}},
    {49, -28, 0, {-51.95507F, -252.19F, 2624.1155F}},
    {34, -1, 0, {-51.98228F, -252.53441F, 2624.3777F}},
    {30, 0, 0, {-51.99372F, -252.83858F, 2624.698F}},
    {29, 1, 0, {-51.99364F, -253.13245F, 2625.0276F}},
    {28, 3, 0, {-51.98442F, -253.42006F, 2625.3636F}},
    {27, 5, 0, {-51.95287F, -253.69704F, 2625.7063F}},
    {27, 6, 0, {-51.91328F, -253.97081F, 2626.0511F}},
    {26, 8, 0, {-51.85619F, -254.23763F, 2626.399F}},
    {26, 10, 0, {-51.78294F, -254.49921F, 2626.7471F}},
    {25, 12, 0, {-51.69793F, -254.7572F, 2627.096F}},
    {25, 14, 0, {-51.58837F, -255.01021F, 2627.4415F}},
    {26, 18, 0, {-51.45672F, -255.26668F, 2627.7764F}},
    {27, 24, 0, {-51.286F, -255.5388F, 2628.0794F}},
    {26, 28, 0, {-51.08171F, -255.80893F, 2628.3631F}},
    {25, 31, 0, {-50.84959F, -256.0697F, 2628.6343F}},
    {24, 34, 0, {-50.58966F, -256.31736F, 2628.892F}},
    {22, 36, 0, {-50.30989F, -256.5535F, 2629.139F}},
    {21, 38, 0, {-50.01139F, -256.7773F, 2629.3755F}},
    {19, 41, 0, {-49.69322F, -256.9861F, 2629.6001F}},
    {15, 44, 0, {-49.35384F, -257.17466F, 2629.812F}},
    {6, 50, 0, {-48.98177F, -257.2999F, 2629.994F}},
    {0, 54, 0, {-48.55399F, -257.2999F, 2630.105F}},
    {0, 54, 0, {-48.12621F, -257.2999F, 2630.217F}},
    {0, 54, 0, {-47.69843F, -257.2999F, 2630.3282F}},
    {0, 54, 0, {-47.27065F, -257.2999F, 2630.4393F}},
    {0, 54, 0, {-46.84286F, -257.2999F, 2630.5511F}},
};

static struct event_path ev00f5_light_path = {123, ev00f5_light_points};

static struct task *ev00f5_white = NULL;
static struct task *ev00f5_fade = NULL;
static struct task *ev00f5_light = NULL;

void ev00f5_script(int phase)
{
    /* @hack spelling: every voice and message goes through play_voice and
     * show_message, locals. SHC then keeps event_voice_play's address in R9
     * and event_message_show's in R8 (0x0cb804aa, 0x0cb804be). called
     * directly, event_message_show's address comes from a pool at each call,
     * ev00f5_script runs 28 bytes long, and 1124 of 1557 words differ */
    void (*play_voice)(int voice);
    void (*show_message)(const char *text);
    /* @hack spelling: the y and z of the point by Knuckles that Sonic and Tails
     * look at come from look_y and look_z, locals. SHC then keeps -264.8 in
     * FR13 and 2642.0 in FR12 (0x0cb802e4, 0x0cb802e8). written in place,
     * ev00f5_script runs 36 bytes long, and 1461 of 1557 words differ */
    Float look_y;
    Float look_z;

    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        ev00f5_fade = event_fade_create(1.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        sound_load_file("e_0009.mlt");
        event_sound_init(2);
        stream_request_music(MUSIC_EVTBGM04);
        event_set_pos(event_get_player(0), -244.944F, -247.1F, 2515.9F);
        event_set_ang(event_get_player(0), 0xf800, 0x2c00, 0);
        event_reset_player(0);
        event_play_action(event_get_player(0), &sonic_sprint_action,
                          &sonic_texlist, 2.5F, EVENT_REPEAT, 0);
        event_create_player(2, tails_step, -265.36F, -247.087F, 2518.8F, 0xf800,
                            0x2c00, 0);
        event_reset_player(2);
        event_create_player(3, eggman_step, -33.747F, -262.47F, 2623.17F, 0,
                            0xb00, 0);
        event_reset_player(3);
        event_create_player(4, knuckles_step, -45.57F, -262.47F, 2642.33F, 0,
                            0x8000, 0);
        event_reset_player(4);
        ev00f5_light = event_light_create(-2.56F, -145.623F, 2153.42F);
        event_wait(1);
        event_play_action(event_get_player(2), &tails_sprint_action,
                          &tails_texlist, 1.9F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(3), &ev00f5_eggman_action_0,
                          &eggman_texlist, 1.0F, EVENT_REPEAT, 0);
        event_play_action(event_get_player(4), &ev00f5_knuckles_action_0,
                          &knuckles_texlist, 0.9F, EVENT_REPEAT, 0);
        if (ev00f5_fade != NULL) {
            event_fade_set_speed(ev00f5_fade, -1.0F / 30.0F);
        }
        event_eggmobile_create(-19.97F, -263.867F, 2682.6653F, 0x100, 0x96b0,
                               0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -234.6867F, -243.638F,
                             2518.9354F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x500, 0x3700, 0x300);
        event_wait(1);
        event_camera_follow(event_get_player(0));
        event_wait(1);
        event_run_to_speed(event_get_player(2), -63.69F, -263.9F, 2627.734F,
                           1.3F, 1.5F);
        event_run_to_speed(event_get_player(0), -47.1F, -261.0F, 2626.77F, 1.4F,
                           1.4F);
        look_y = -264.8F;
        look_z = 2642.0F;
        event_look_at(event_get_player(0), -42.3F, look_y, look_z);
        event_look_at(event_get_player(2), -30.6658F, -261.8F, 2627.0264F);
        event_wait(48);
        event_camera_follow_clear();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -22.1F, -260.4F, 2650.58F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x100, 0x2c00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 120, -18.346F, -260.4F,
                             2643.53F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 120, 0x100, 0x2c00, 0);
        event_wait(77);
        event_play_action(event_get_player(0), &sonic_skid_action,
                          &sonic_texlist, 1.0F, 0, 8);
        event_play_action(event_get_player(2), &tails_skid_action,
                          &tails_texlist, 1.0F, 0, 8);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(2), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_at(event_get_player(0), -30.6658F, -261.8F, 2627.0264F);
        event_look_at(event_get_player(2), -42.3F, look_y, look_z);
        event_play_action(event_get_player(4), &ev00f5_knuckles_action_0,
                          &knuckles_texlist, 1.2F, EVENT_REPEAT, 8);
        play_voice = event_voice_play;
        play_voice(1915);
        event_talk_start(event_get_player(4), TEXT_15);
        show_message = event_message_show;
        show_message(TEXT_1);
        event_look_at(event_get_player(0), -42.3F, look_y, look_z);
        event_voice_wait();
        event_look_clear(event_get_player(0));
        event_set_ang(event_get_player(0), 0x800, 0xd00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -35.127F, -264.64F, 2641.0F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xa00, 0x1f00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 150, -38.0244F, -264.64F,
                             2643.74F);
        event_play_action(event_get_player(0), &ev00f5_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        play_voice(1916);
        event_talk_start(event_get_player(0), TEXT_16);
        show_message(TEXT_2);
        event_wait(23);
        event_play_action(event_get_player(0), &ev00f5_sonic_action_2,
                          &sonic_texlist, 0.75F, 0, 8);
        event_look_at_task(event_get_player(0), event_get_player(3), 0.0F, 0.0F,
                           0.0F);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev00f5_sonic_action_1,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_talk_start(event_get_player(4), TEXT_17);
        event_message_close();
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -51.0568F, -260.9F,
                             2636.7764F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xed00, 0xa300, 0);
        event_play_action(event_get_player(4), &ev00f5_knuckles_action_1,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(4), &ev00f5_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        play_voice(1917);
        event_talk_start(event_get_player(4), TEXT_18);
        show_message(TEXT_3);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(4));
        event_wait(25);
        play_voice(1918);
        event_talk_start(event_get_player(4), TEXT_19);
        show_message(TEXT_4);
        event_wait(1);
        /* the light's work is events/shared/light.c's struct light_work,
         * which keeps size where task_work keeps scl.z */
        if (ev00f5_light != NULL) {
            ev00f5_light->work->scl.z = 3.5F;
        }
        event_set_flags(ev00f5_light, EVENT_IGNORE_GROUND);
        event_set_pos(event_get_player(2), -59.447F, -262.275F, 2625.9F);
        event_set_ang(event_get_player(2), 0x800, 0x1f00, 0);
        event_look_at(event_get_player(2), -42.3F, -260.0F, look_z);
        event_voice_wait();
        event_set_ang(event_get_player(2), 0x800, 0x1f00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -45.282F, -265.71F,
                             2638.166F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1c00, 0x1ab0, 0xfe00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 35, -43.624F, -262.437F,
                             2635.245F);
        event_play_action(event_get_player(0), &ev00f5_sonic_action_1,
                          &sonic_texlist, 1.5F, 0, 1);
        event_play_action(event_get_player(0), &ev00f5_sonic_action_1,
                          &sonic_texlist, 0.15F, EVENT_REPEAT, 8);
        event_play_action(event_get_player(2), &ev00f5_tails_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 4.0F,
                           0.0F);
        event_talk_start(event_get_player(2), TEXT_20);
        play_voice(1919);
        event_talk_start(event_get_player(0), TEXT_21);
        show_message(TEXT_5);
        event_wait(1);
        event_voice_wait();
        event_wait(10);
        event_message_close();
        event_clear_motions(event_get_player(0));
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -22.9F, -262.263F, 2632.3F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xff00, 0x29b0, 0xfe00);
        event_play_action(event_get_player(3), &ev00f5_eggman_action_1,
                          &eggman_texlist, 1.0F, 0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 140, -24.029F, -264.524F,
                             2636.048F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 140, 0x1c00, 0x1eb0, 0xfe00);
        event_wait(40);
        play_voice(1920);
        show_message(TEXT_6);
        event_wait(30);
        event_play_action(event_get_player(0), &ev00f5_sonic_action_2,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 8);
        event_look_at_task(event_get_player(2), event_get_player(3), 0.0F,
                           10.0F, 0.0F);
        event_wait_motions(event_get_player(3));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 40);
        event_run_to_speed(event_get_player(3), -21.63F, -266.69F, 2659.7F,
                           1.5F, 2.3F);
        event_play_action(event_get_player(4), &ev00f5_knuckles_action_3,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_ang(event_get_player(4), 0, 0x5000, 0);
        event_set_ang(event_get_player(2), 0x800, 0xc00, 0);
        event_wait(10);
        event_message_close();
        play_voice(1921);
        show_message(TEXT_7);
        event_wait(30);
        event_kill_player(3);
        event_clear_motions(event_get_player(0));
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -62.36F, -256.835F,
                             2614.588F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x100, 0x96b0, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -59.63F, -260.5F, 2619.0F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xf00, 0x96b0, 0xff00);
        event_sound_start(0, 1227, 1800);
        event_eggmobile_fly_turn(-13.0F, -217.033F, 2676.63F, 0x1000, -0x41b0,
                                 -0x1000, 60, 1);
        event_wait(22);
        event_message_close();
        event_wait(20);
        event_eggmobile_set_jets(EGGMOBILE_JETS_ALL);
        event_wait(2);
        event_eggmobile_fly_to(123.0F, -277.6F, 2903.85F, 50);
        event_sound_fade_volume(0, -70, 50);
        event_wait(30);
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 8);
        event_look_clear(event_get_player(0));
        event_wait(10);
        event_eggmobile_kill();
        event_sound_stop(0);
        event_set_ang(event_get_player(2), 0x884, 0x1600, 0x44);
        event_look_clear(event_get_player(2));
        event_look_at_task(event_get_player(2), event_get_player(4), 0.0F, 0.0F,
                           0.0F);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -40.25F, -264.8667F,
                             2644.32F);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xc00, 0x24b0, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -36.9F, -265.7F, 2643.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0xc00, 0x28b0, 0xfb00);
        event_play_action(event_get_player(4), &ev00f5_knuckles_action_1,
                          &knuckles_texlist, 1.0F, 0, 16);
        event_play_action(event_get_player(4), &ev00f5_knuckles_action_2,
                          &knuckles_texlist, 1.0F, EVENT_REPEAT, 0);
        play_voice(1922);
        event_talk_start(event_get_player(4), TEXT_19);
        show_message(TEXT_8);
        event_wait(10);
        event_camera_set_pos(EVENT_EASE_LINEAR, 200, -37.9F, -265.2342F,
                             2641.1F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 200, 0xc00, 0x29b0, 0xfb00);
        event_look_at_task(event_get_player(0), event_get_player(4), 0.0F, 0.0F,
                           0.0F);
        event_voice_wait();
        event_talk_stop(event_get_player(4));
        event_wait(20);
        play_voice(1923);
        event_talk_start(event_get_player(4), TEXT_15);
        show_message(TEXT_9);
        event_wait(20);
        event_voice_wait();
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &ev00f5_sonic_action_3,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 16);
        SET_SONIC_FINGERS(ev00f5_sonic_fingers);
        play_voice(1924);
        event_talk_start(event_get_player(0), TEXT_22);
        show_message(TEXT_10);
        event_wait(11);
        event_play_action(event_get_player(0), &ev00f5_sonic_action_0,
                          &sonic_texlist, 0.45F, 0, 1);
        event_voice_wait();
        SET_SONIC_FINGERS(sonic_fingers_minus_z);
        event_talk_stop(event_get_player(0));
        event_wait(5);
        play_voice(1925);
        event_talk_start(event_get_player(0), TEXT_23);
        show_message(TEXT_11);
        event_wait(1);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_look_at_task(event_get_player(2), event_get_player(0), 0.0F, 0.0F,
                           0.0F);
        event_play_action(event_get_player(2), &tails_stand_action,
                          &tails_texlist, 1.0F, EVENT_REPEAT, 8);
        play_voice(1926);
        show_message(TEXT_12);
        event_wait(1);
        event_voice_wait();
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -42.2F, -257.68F, 2642.7F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x300, 0x2bb0, 0xfb00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 90, -50.7F, -263.32F, 2635.73F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 90, 0x1300, 0xeeb0, 0xfb00);
        event_play_path(ev00f5_light, &ev00f5_light_path, 1.3F, 0);
        event_sound_start(1, SE_TIKAL_CHIME, 1800);
        event_wait(30);
        event_look_at_task(event_get_player(0), ev00f5_light, 0.0F, 0.0F, 0.0F);
        play_voice(1927);
        show_message(TEXT_13);
        event_wait(50);
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &ev00f5_sonic_action_4,
                          &sonic_texlist, 1.0F, 0, 4);
        event_play_action(event_get_player(0), &ev00f5_sonic_action_5,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 3);
        event_wait(5);
        ev00f5_white = event_fade_create(0.025F, 0.1F, 1.0F, 1.0F, 1.0F);
        play_voice(995);
        show_message(TEXT_14);
        event_sound_fade_volume(1, -90, 60);
        event_wait(20);
        event_message_close();
        event_wait(20);
        /* @hack redundant: four waits that never run. SHC writes no code for
         * them, but counts the 0xc00 they pass when it allocates registers. it
         * then keeps event_voice_play's address in R9 and event_message_show's
         * in R8 (0x0cb804aa, 0x0cb804be). without them, the two addresses trade
         * registers and 30 of 1557 words differ */
        if (0) {
            event_wait(0xc00);
            event_wait(0xc00);
            event_wait(0xc00);
            event_wait(0xc00);
        }
        event_sound_stop(1);
        break;
    case EVENT_END:
        event_camera_release();
        event_pads_on();
        event_voice_stop();
        SET_SONIC_FINGERS(sonic_fingers_minus_z);
        event_clear_paths(ev00f5_light);
        event_reset_player(0);
        event_kill_player(2);
        event_kill_player(3);
        event_kill_player(4);
        EVENT_KILL(ev00f5_light);
        event_eggmobile_kill();
        EVENT_KILL(ev00f5_white);
        EVENT_KILL(ev00f5_fade);
        event_sound_stop_all();
        break;
    case EVENT_UNLOAD:
        break;
    }
}

struct texture_pvm ev00f5_textures[] = {
    {"EV_EGGMOBLE0", &event_eggmobile_texlist},
    {"KNUCKLES", &knuckles_texlist},
    {"EGGMAN", &eggman_texlist},
    {"MILES", &tails_texlist},
    {NULL, NULL},
};
