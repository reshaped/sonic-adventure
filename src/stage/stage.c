/* the current stage, act and character, and the frame steps that run a
 * stage to its end */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "adv00/start.h"
#include "adv00/station_square.h"
#include "adv0100/egg_carrier.h"
#include "adv0100/sky.h"
#include "adv0130/egg_carrier_inside.h"
#include "adv01obj/start.h"
#include "adv02/mystic_ruins.h"
#include "adv02/start.h"
#include "adv03/past.h"
#include "adv03/start.h"
#include "al_garden00/garden.h"
#include "al_garden01/garden.h"
#include "al_garden02/garden.h"
#include "al_main/card.h"
#include "al_race/race.h"
#include "al_race/sky.h"
#include "b_chaos0/chaos0.h"
#include "b_chaos2/chaos2.h"
#include "b_chaos4/chaos4.h"
#include "b_chaos6/chaos6.h"
#include "b_chaos6/sky.h"
#include "b_chaos7/chaos7.h"
#include "b_e101/e101.h"
#include "b_e101_r/e101_r.h"
#include "b_egm1/eggmobile1.h"
#include "b_egm2/eggmobile2.h"
#include "b_egm3/eggmobile3.h"
#include "b_robo/eggmanrobo.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/debug.h"
#include "draw/fade.h"
#include "draw/light.h"
#include "draw/model.h"
#include "draw/text.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "event/play.h"
#include "hud/hud.h"
#include "hud/menu.h"
#include "hud/overlay.h"
#include "main.h"
#include "minicart/cart.h"
#include "minicart/sky.h"
#include "object/capsule.h"
#include "object/common.h"
#include "object/download.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/savepoint.h"
#include "path/path.h"
#include "player/amy.h"
#include "player/big.h"
#include "player/e102.h"
#include "player/fishing.h"
#include "player/knuckles.h"
#include "player/tails.h"
#include "player/npc.h"
#include "player/player.h"
#include "player/race.h"
#include "player/sonic.h"
#include "player/talk.h"
#include "progress/demo.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/recorder.h"
#include "save/card.h"
#include "save/save.h"
#include "sboard/sky.h"
#include "sboard/sunaboard.h"
#include "shooting/shooting.h"
#include "shooting/tornado.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg00/practice.h"
#include "stg01/beach.h"
#include "stg02/windy.h"
#include "stg03/twinkle.h"
#include "stg04/highway.h"
#include "stg05/mountain.h"
#include "stg06/sky_deck.h"
#include "stg07/ruin.h"
#include "stg08/icecap.h"
#include "stg09/casino.h"
#include "stg10/final.h"
#include "stg12/shelter.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/history.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/thread.h"
#include "system/timer.h"
#include "vmu/screen.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 6
#include "stage/stage/text.inc"

/* the stage an exit list ends at */
#define EXIT_END 255

/* the buttons that, all held with one newly pressed, flip hud_hidden */
#define HUD_BUTTONS (PDD_DGT_TX | PDD_DGT_TY | PDD_DGT_TA | PDD_DGT_TB)

/* the effect the pause menu plays as it opens and as it closes */
#define SE_PAUSE 3
/* the frames the continue screen waits for a choice before it quits */
#define CONTINUE_FRAMES 1200

/* the voices Sonic, Knuckles and Amy clear a stage or a boss with */
#define SE_SONIC_CLEAR 0x57c
#define SE_SONIC_CLEAR_BOSS 0x57f
#define SE_KNUCKLES_CLEAR 0x541
#define SE_KNUCKLES_CLEAR_BOSS 0x544
#define SE_AMY_CLEAR 0x4ff
#define SE_AMY_CLEAR_BOSS 0x501

/* the frames title_card_load shows the title card for, before the stage
 * loads */
#define TITLE_CARD_FRAMES 5

/* station square's act that twinkle park leads back to, and the entrance
 * there that the cart leads to */
#define STATION_SQUARE_TWINKLE_ACT 5
#define CART_ENTRANCE 2

/* the step of its task on which sound_play_later's effect or
 * music_play_later's track starts */
#define LATER_STEPS 5

/* a frame runs one step more when its second step ends this long or
 * longer after the frame began. ACTION_LIMIT holds in an action stage, and
 * FIELD_LIMIT in a field. the times are in thousandths of a vsync, as
 * timer_elapsed gives them */
#define ACTION_LIMIT 1900.0F
#define FIELD_LIMIT 1850.0F

/* a frame of the memory card's error checks the card and the pad only
 * while play_time's bits under this mask are 0. play_time counts steps, so
 * a frame of more than one step can skip past a multiple of 256 */
#define CARD_CHECK_MASK 0xff

/* what a stage does each frame, from its title card to its end */
enum stage_mode {
    STAGE_ENTER, /* fade from black, and load the stage as the black lifts */
    STAGE_ENTER_WAIT,
    STAGE_AGAIN, /* fade from black, and start the act again */
    STAGE_AGAIN_WAIT,
    STAGE_FADE_IN, /* play while the fade from black ends */
    STAGE_CLEAR_FADE, /* every mode ending in _FADE fades to black first */
    STAGE_MISS_FADE,
    STAGE_GAME_OVER_FADE,
    STAGE_GO_TO_FADE,
    STAGE_WAIT,
    STAGE_RESTART_FADE, /* the cart and the Chao race start over, at no cost */
    STAGE_RETRY_FADE,
    STAGE_QUIT_FADE,
    STAGE_ENDING,
    STAGE_PLAY,
    STAGE_PAUSE,
    STAGE_LEAVE,
    STAGE_GAME_OVER,
    STAGE_CONTINUE, /* the choice to go on or quit, after a game over */
    STAGE_QUIT,
    STAGE_COMING_SOON,
    STAGE_TITLE_CARD,
    STAGE_TITLE_CARD_SHOW,
    STAGE_TRIAL_END,
    STAGE_CARD_ERROR
};

/* an event that plays once for a character at a stage and act */
struct stage_event {
    Sint8 stage;
    Sint8 act;
    Sint16 character;
    Sint16 unk_04;
    Sint16 event;
    void (*destructor)(struct task *task); /* of the task that plays it */
};

/* where a character starts in a stage and act */
struct start_point {
    Sint16 stage;
    Sint16 act;
    NJS_POINT3 pos;
    Angle ang_y;
};

/* where a character comes back to a field from a stage */
struct return_point {
    Sint16 stage;
    Sint16 field;
    NJS_POINT3 pos;
    Angle ang_y;
};

/* the colors njSetBackColor takes */
struct back_color {
    Uint32 col1;
    Uint32 col2;
    Uint32 col3;
};

/* where leaving a stage takes a character: first after the first clear, and
 * again after every later one, unless again_stage is 0 */
struct stage_exit {
    Sint8 movie; /* for movie_request */
    Uint8 stage;
    Uint8 first_stage;
    Uint8 first_act;
    Uint8 first_entrance;
    Uint8 again_stage;
    Uint8 again_act;
    Uint8 again_entrance;
};

enum later_kind { LATER_MUSIC, LATER_SOUND };

/* a task that waits, then requests a track or plays an effect */
struct later_extra {
    int steps; /* until it acts, counting that one */
    int number;
    enum later_kind kind;
};

#define LATER(task) ((struct later_extra *)(task)->extra)

/* the camera's paths round each character's clear pose */
extern struct path amy_clear_path;
extern struct path sonic_clear_path;
extern struct path e102_clear_path;
extern struct path knuckles_clear_path;

static Bool stage_start_event(void);
static void event_task_wait(struct task *task);
static void event_task_start(struct task *task);
static void clear_event_end(struct task *task);
static void clear_event_task(struct task *task);
static Bool stage_clear_event(void);
static void action_leave_cleared(void);
static void action_go_next(void);
static void field_go_next(void);
static void stage_reset_counts(void);
static void stage_reset_play(void);
static void stage_load(void);
static void stage_start(void);
static int stage_run_step(void);
static int stage_run_action(void);
static int stage_run_field(void);
static int stage_run_paused(void);
static int stage_run_card_error(void);
static void stage_stop(void);
static void stage_stop_counted(void);
static void stage_unload(void);
static void later_step(struct task *task);
static void clear_wait_step(struct task *task);
static void stage_create_players(void);
static void stage_create_task(void);
static void stage_create_sky(void);
static void stage_keep_previous(void);
static void stage_set_next_return(void);
static void stage_set_next_exit_from(Uint16 stage, Uint16 act);
static Bool stage_set_next_boss(void);

/* the events that play once as a character comes to a stage and act, and
 * those that play once as it clears one. once event 253 is over, its task
 * starts the next entry event, 255 */
static struct stage_event entry_events[] = {
    {STAGE_CASINO, 1, CHARACTER_TAILS, 88, 88, NULL},
    {STAGE_FINAL, 0, CHARACTER_AMY, 0x304, 111, NULL},
    {STAGE_CHAOS0, 0, CHARACTER_SONIC, 1, 43, NULL},
    {STAGE_CHAOS2, 0, CHARACTER_KNUCKLES, 33, 135, NULL},
    {STAGE_CHAOS6, 0, CHARACTER_SONIC, 28, 28, NULL},
    {STAGE_CHAOS6, 1, CHARACTER_KNUCKLES, 0x104, 153, NULL},
    {STAGE_CHAOS6, 0, CHARACTER_BIG, 0x106, 222, NULL},
    /* @bug mismatched-call: stage_start_event, a Bool (void), is the event
     * task's destructor (0x8c1b2d6c, stored at 0x8c04f84c), which task_destroy
     * calls with the task in R4 and its result dropped (JSR @R3 at
     * 0x8c09e13e) */
    {STAGE_CHAOS7, 0, CHARACTER_SONIC, 0x301, 253,
     /* NOLINTNEXTLINE(clang-diagnostic-cast-function-type-*) */
     (void (*)(struct task *))stage_start_event},
    {STAGE_CHAOS7, 0, CHARACTER_SONIC, 0, 255, NULL},
    {STAGE_MOUNTAIN, 0, CHARACTER_SONIC, 1, 384, NULL},
    {STAGE_NONE, 0, 0, 0, 0, NULL},
};

static struct stage_event clear_events[] = {
    {STAGE_BEACH, 1, CHARACTER_TAILS, 49, 49, NULL},
    {STAGE_BEACH, 2, CHARACTER_BIG, 40, 212, NULL},
    {STAGE_BEACH, 0, CHARACTER_E102, 35, 184, NULL},
    {STAGE_MOUNTAIN, 1, CHARACTER_SONIC, 0x309, 23, NULL},
    {STAGE_MOUNTAIN, 1, CHARACTER_TAILS, 0x310, 72, NULL},
    {STAGE_RUIN, 2, CHARACTER_SONIC, 0x308, 32, NULL},
    {STAGE_RUIN, 1, CHARACTER_KNUCKLES, 0x307, 141, NULL},
    {STAGE_SKY_DECK, 2, CHARACTER_KNUCKLES, 0x311, 149, NULL},
    {STAGE_CASINO, 0, CHARACTER_KNUCKLES, 0x302, 131, NULL},
    {STAGE_SHELTER, 1, CHARACTER_AMY, 0x303, 102, NULL},
    {STAGE_SHELTER, 0, CHARACTER_BIG, 0x305, 218, NULL},
    {STAGE_CHAOS0, 0, CHARACTER_SONIC, 1, 2, NULL},
    {STAGE_CHAOS2, 0, CHARACTER_KNUCKLES, 33, 136, NULL},
    {STAGE_CHAOS6, 0, CHARACTER_BIG, 0x106, 223, NULL},
    {STAGE_CHAOS7, 0, CHARACTER_SONIC, 0x301, 254, NULL},
    {STAGE_SUNABOARD, 0, CHARACTER_TAILS, 20, 85, NULL},
    {STAGE_NONE, 0, 0, 0, 0, NULL},
};

static struct start_point sonic_starts[] = {
    {STAGE_BEACH, 0, {-9.0F, 16.0F, 4.0F}, 0x6000},
    {STAGE_BEACH, 1, {-986.5F, 1145.0F, -2856.0F}, 0x4000},
    {STAGE_BEACH, 2, {5815.0F, 5.0F, 1136.0F}, 0},
    {STAGE_WINDY, 0, {1230.0F, -333.0F, -140.0F}, 0},
    {STAGE_WINDY, 1, {42.0F, 380.0F, -20.0F}, 0},
    {STAGE_WINDY, 2, {1096.0F, 205.0F, -1251.0F}, 0},
    {STAGE_TWINKLE, 0, {-2000.0F, 1770.0F, -985.0F}, 0x4000},
    {STAGE_TWINKLE, 1, {520.0F, 975.0F, 1620.0F}, 0xc000},
    {STAGE_HIGHWAY, 0, {-673.0F, -10.0F, 5.0F}, 0},
    {STAGE_HIGHWAY, 1, {-50.0F, 42.0F, 0.0F}, 0},
    {STAGE_HIGHWAY, 2, {72.0F, 26.0F, 192.0F}, 0xc000},
    {STAGE_MOUNTAIN, 0, {18.0F, 36.0F, 636.0F}, 0},
    {STAGE_MOUNTAIN, 1, {-78.0F, 831.0F, 1919.0F}, 0},
    {STAGE_SKY_DECK, 0, {0.0F, -400.0F, 60.0F}, 0xc000},
    {STAGE_SKY_DECK, 1, {327.0F, -74.0F, 2535.0F}, 0},
    {STAGE_SKY_DECK, 2, {-696.0F, -86.0F, 135.0F}, 0},
    {STAGE_RUIN, 0, {-264.0F, 73.0F, -168.0F}, 0},
    {STAGE_RUIN, 1, {220.0F, 73.0F, -1423.0F}, 0x4000},
    {STAGE_RUIN, 2, {270.0F, 10.0F, 0.0F}, 0x8000},
    {STAGE_ICECAP, 0, {1580.0F, 84.0F, 1074.0F}, 0x8000},
    {STAGE_ICECAP, 1, {1060.0F, 336.0F, 280.0F}, 0},
    {STAGE_ICECAP, 2, {120.0F, 375.0F, -40.0F}, 0x8800},
    {STAGE_CASINO, 0, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_CASINO, 1, {19.0F, -1695.0F, 2850.0F}, 0},
    {STAGE_CASINO, 2, {137.0F, 35.0F, 64.0F}, 0},
    {STAGE_CASINO, 3, {-430.0F, -205.0F, 100.0F}, 0},
    {STAGE_FINAL, 0, {170.0F, -165.0F, 490.0F}, 0},
    {STAGE_FINAL, 1, {1060.0F, 595.0F, -383.0F}, 0xc000},
    {STAGE_FINAL, 2, {1071.7F, 240.0F, -518.0F}, 0x8000},
    {STAGE_FINAL, 3, {2811.0F, -405.0F, -1001.0F}, 0},
    {STAGE_FINAL, 4, {2383.0F, -3125.0F, -509.0F}, 0},
    {STAGE_FINAL, 5, {-335.0F, 10.0F, 0.0F}, 0},
    {STAGE_CHAOS0, 0, {268.0F, 0.0F, 377.0F}, 0},
    {STAGE_CHAOS4, 0, {-37.0F, 92.0F, -110.0F}, 0},
    {STAGE_CHAOS6, 0, {0.0F, 744.0F, -568.0F}, 0},
    {STAGE_CHAOS7, 0, {-16.0F, 150.0F, -667.0F}, 0xb92c},
    {STAGE_EGGMOBILE1, 0, {812.0F, 140.0F, 839.0F}, 0},
    {STAGE_EGGMOBILE3, 0, {260.0F, 50.0F, 0.0F}, 0},
    {STAGE_STATION_SQUARE, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 3, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 4, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_2, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_2, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_2, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_2, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_3, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_3, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_3, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_3, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_EGG_CARRIER, 0, {0.0F, 700.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 1, {0.0F, 650.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 2, {85.0F, 1541.0F, 3392.0F}, 0},
    {STAGE_CART, 0, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 1, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 2, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 3, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 4, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 5, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_SHOOTING1, 0, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_SHOOTING2, 0, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_SUNABOARD, 0, {1645.0F, -9.0F, 219.0F}, 0xc000},
    {STAGE_SUNABOARD, 1, {0.0F, -30.0F, 0.0F}, 0xc000},
    {STAGE_MYSTIC_RUINS, 0, {-21.0F, 214.0F, 1221.0F}, 0},
    {STAGE_MYSTIC_RUINS, 1, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_MYSTIC_RUINS, 2, {-1134.8F, 354.0F, 406.7F}, 0},
    {STAGE_MYSTIC_RUINS, 3, {2.11F, 108.68F, 172.29F}, 0xc826},
    {STAGE_ALIFE_RACE, 0, {2052.0F, 5.0F, 0.0F}, 0x8000},
    {STAGE_ALIFE_RACE, 1, {0.0F, 75.0F, 0.0F}, 0},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct start_point tails_starts[] = {
    {STAGE_WINDY, 2, {1093.0F, -158.0F, -1254.0F}, 0},
    {STAGE_HIGHWAY, 0, {-673.0F, -10.0F, 5.0F}, 0},
    {STAGE_MOUNTAIN, 1, {-397.0F, 1039.0F, 3141.0F}, 0},
    {STAGE_SKY_DECK, 0, {0.0F, -400.0F, 60.0F}, 0xc000},
    {STAGE_SKY_DECK, 1, {327.0F, -74.0F, 2535.0F}, 0},
    {STAGE_SKY_DECK, 2, {-1237.0F, 15.0F, 97.0F}, 0},
    {STAGE_ICECAP, 2, {120.0F, 375.0F, -40.0F}, 0x8800},
    {STAGE_CASINO, 1, {19.0F, -1695.0F, 2850.0F}, 0},
    {STAGE_CHAOS4, 0, {-37.0F, 92.0F, -110.0F}, 0},
    {STAGE_EGGMOBILE1, 0, {812.0F, 139.0F, 840.0F}, 0},
    {STAGE_EGGMOBILE2, 0, {-480.0F, 19.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_2, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_2, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_2, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_2, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_3, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_3, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_3, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_3, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_EGG_CARRIER, 0, {0.0F, 700.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 1, {0.0F, 650.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 2, {85.0F, 1541.0F, 3392.0F}, 0},
    {STAGE_CART, 0, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 1, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 2, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 3, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 4, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 5, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_SUNABOARD, 0, {0.0F, -30.0F, 0.0F}, 0xc000},
    {STAGE_SUNABOARD, 1, {1645.0F, -9.0F, 219.0F}, 0xc000},
    {STAGE_MYSTIC_RUINS, 0, {-21.0F, 214.0F, 1221.0F}, 0},
    {STAGE_MYSTIC_RUINS, 1, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_MYSTIC_RUINS, 2, {-1134.8F, 354.0F, 406.7F}, 0},
    {STAGE_MYSTIC_RUINS, 3, {2.11F, 108.68F, 172.29F}, 0xc826},
    {STAGE_ALIFE_RACE, 0, {2052.0F, 5.0F, 0.0F}, 0x8000},
    {STAGE_ALIFE_RACE, 1, {0.0F, 75.0F, 0.0F}, 0},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct start_point knuckles_starts[] = {
    {STAGE_HIGHWAY, 2, {-230.0F, 150.0F, -1740.0F}, 0x4000},
    {STAGE_MOUNTAIN, 2, {-3147.0F, 764.0F, -1358.0F}, -16384},
    {STAGE_SKY_DECK, 0, {0.0F, -410.0F, 360.0F}, 0},
    {STAGE_SKY_DECK, 1, {327.0F, -74.0F, 2535.0F}, 0},
    {STAGE_SKY_DECK, 2, {674.0F, 207.0F, 12.0F}, 0x8000},
    {STAGE_RUIN, 0, {-264.0F, -23.0F, -168.0F}, 0},
    {STAGE_RUIN, 1, {7482.0F, -2622.0F, 908.0F}, 0},
    {STAGE_RUIN, 2, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_CHAOS2, 0, {13.0F, 0.0F, 76.0F}, 0},
    {STAGE_CHAOS4, 0, {-37.0F, 92.0F, -110.0F}, 0},
    {STAGE_CHAOS6, 1, {0.0F, 744.0F, -568.0F}, 0},
    {STAGE_STATION_SQUARE, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_2, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_2, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_2, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_2, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_3, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_3, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_3, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_3, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_EGG_CARRIER, 0, {0.0F, 700.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 1, {0.0F, 650.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 2, {85.0F, 1541.0F, 3392.0F}, 0},
    {STAGE_CART, 0, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 1, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 2, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 3, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 4, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 5, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_MYSTIC_RUINS, 0, {-21.0F, 214.0F, 1221.0F}, 0},
    {STAGE_MYSTIC_RUINS, 1, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_MYSTIC_RUINS, 2, {-1134.8F, 354.0F, 406.7F}, 0},
    {STAGE_MYSTIC_RUINS, 3, {2.11F, 108.68F, 172.29F}, 0xc826},
    {STAGE_ALIFE_RACE, 0, {2052.0F, 5.0F, 0.0F}, 0x8000},
    {STAGE_ALIFE_RACE, 1, {0.0F, 75.0F, 0.0F}, 0},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct start_point big_starts[] = {
    {STAGE_BEACH, 2, {6090.0F, 30.0F, 1000.0F}, 0},
    {STAGE_TWINKLE, 1, {230.0F, 80.0F, -538.0F}, 0xc000},
    {STAGE_ICECAP, 3, {1260.0F, 224.0F, -38.0F}, 0},
    {STAGE_CHAOS6, 0, {0.0F, 744.0F, -568.0F}, 0},
    {STAGE_STATION_SQUARE, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_2, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_2, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_2, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_2, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_3, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_3, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_3, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_3, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_SHELTER, 0, {0.0F, 236.0F, 51.5F}, 0x4000},
    {STAGE_SHELTER, 1, {215.0F, 278.0F, -398.0F}, 0},
    {STAGE_SHELTER, 2, {-19.0F, 252.0F, -75.0F}, 0},
    {STAGE_EGG_CARRIER, 0, {0.0F, 700.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 1, {0.0F, 650.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 2, {85.0F, 1541.0F, 3392.0F}, 0},
    {STAGE_CART, 0, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 1, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 2, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 3, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 4, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 5, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_MYSTIC_RUINS, 0, {-21.0F, 214.0F, 1221.0F}, 0},
    {STAGE_MYSTIC_RUINS, 1, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_MYSTIC_RUINS, 2, {-1134.8F, 354.0F, 406.7F}, 0},
    {STAGE_MYSTIC_RUINS, 3, {2.11F, 108.68F, 172.29F}, 0xc826},
    {STAGE_ALIFE_RACE, 0, {2052.0F, 5.0F, 0.0F}, 0x8000},
    {STAGE_ALIFE_RACE, 1, {0.0F, 75.0F, 0.0F}, 0},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct start_point amy_starts[] = {
    {STAGE_TWINKLE, 1, {723.0F, 70.0F, -358.0F}, 0xc000},
    {STAGE_TWINKLE, 2, {75.0F, 0.0F, -390.0F}, 0x4000},
    {STAGE_FINAL, 0, {170.0F, -165.0F, 490.0F}, 0xc000},
    {STAGE_FINAL, 1, {1060.0F, 581.0F, -383.0F}, 0},
    {STAGE_FINAL, 2, {1078.0F, 15.0F, -830.0F}, 0},
    {STAGE_FINAL, 3, {2811.0F, -405.0F, -1001.0F}, 0},
    {STAGE_FINAL, 4, {2383.0F, -3125.0F, -509.0F}, 0},
    {STAGE_FINAL, 5, {-335.0F, 10.0F, 0.0F}, 0},
    {STAGE_SHELTER, 0, {0.0F, 236.0F, 51.5F}, 0x4000},
    {STAGE_SHELTER, 1, {215.0F, 278.0F, -398.0F}, 0},
    {STAGE_SHELTER, 2, {-19.0F, 252.0F, -75.0F}, 0},
    {STAGE_EGGMANROBO, 0, {-131.0F, 745.0F, -385.0F}, 0},
    {STAGE_STATION_SQUARE, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_2, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_2, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_2, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_2, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_3, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_3, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_3, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_3, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_EGG_CARRIER, 0, {0.0F, 700.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 1, {0.0F, 650.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 2, {85.0F, 1541.0F, 3392.0F}, 0},
    {STAGE_CART, 0, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 1, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 2, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 3, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 4, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 5, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_MYSTIC_RUINS, 0, {-21.0F, 214.0F, 1221.0F}, 0},
    {STAGE_MYSTIC_RUINS, 1, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_MYSTIC_RUINS, 2, {-1134.8F, 354.0F, 406.7F}, 0},
    {STAGE_MYSTIC_RUINS, 3, {2.11F, 108.68F, 172.29F}, 0xc826},
    {STAGE_ALIFE_RACE, 0, {2052.0F, 5.0F, 0.0F}, 0x8000},
    {STAGE_ALIFE_RACE, 1, {0.0F, 75.0F, 0.0F}, 0},
    {STAGE_PRACTICE, 0, {-55.0F, 0.0F, -31.0F}, 0},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct start_point e102_starts[] = {
    {STAGE_WINDY, 0, {-10.0F, -102.0F, -10.0F}, 0},
    {STAGE_WINDY, 1, {694.0F, -113.0F, -205.0F}, 0},
    {STAGE_WINDY, 2, {0.0F, 700.0F, 0.0F}, 0},
    {STAGE_MOUNTAIN, 1, {-78.0F, 831.0F, 1919.0F}, 0},
    {STAGE_FINAL, 0, {187.0F, -75.0F, 957.0F}, 0},
    {STAGE_FINAL, 1, {1060.0F, 581.0F, -383.0F}, 0},
    {STAGE_FINAL, 2, {46.5F, -3240.6F, -224.5F}, 0},
    {STAGE_FINAL, 3, {2811.0F, -405.0F, -1001.0F}, 0},
    {STAGE_FINAL, 4, {2383.0F, -3125.0F, -509.0F}, 0},
    {STAGE_FINAL, 5, {-335.0F, 10.0F, 0.0F}, 0},
    {STAGE_SHELTER, 0, {0.0F, 236.0F, -51.5F}, 0},
    {STAGE_SHELTER, 1, {215.0F, 278.0F, -398.0F}, 0},
    {STAGE_SHELTER, 2, {-20.0F, 486.0F, 73.0F}, 0xc000},
    {STAGE_E101, 0, {70.0F, 0.0F, 65.0F}, 0xa000},
    {STAGE_E101_R, 0, {-131.0F, 745.0F, -385.0F}, 0},
    {STAGE_STATION_SQUARE, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_2, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_2, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_2, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_2, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_STATION_SQUARE_3, 0, {280.0F, 0.0F, 840.0F}, 0},
    {STAGE_STATION_SQUARE_3, 1, {-500.0F, 0.0F, 1000.0F}, 0},
    {STAGE_STATION_SQUARE_3, 2, {37.0F, -110.0F, 1066.0F}, 0},
    {STAGE_STATION_SQUARE_3, 5, {717.0F, 50.0F, 1720.0F}, 0},
    {STAGE_EGG_CARRIER, 0, {0.0F, 700.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 1, {0.0F, 650.0F, -1000.0F}, 0},
    {STAGE_EGG_CARRIER, 2, {85.0F, 1541.0F, 3392.0F}, 0},
    {STAGE_CART, 0, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 1, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 2, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 3, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 4, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_CART, 5, {1523.0F, 9.0F, 84.0F}, 0},
    {STAGE_MYSTIC_RUINS, 0, {-21.0F, 214.0F, 1221.0F}, 0},
    {STAGE_MYSTIC_RUINS, 1, {0.0F, 0.0F, 0.0F}, 0},
    {STAGE_MYSTIC_RUINS, 2, {-1134.8F, 354.0F, 406.7F}, 0},
    {STAGE_MYSTIC_RUINS, 3, {2.11F, 108.68F, 172.29F}, 0xc826},
    {STAGE_ALIFE_RACE, 0, {2052.0F, 5.0F, 0.0F}, 0x8000},
    {STAGE_ALIFE_RACE, 1, {0.0F, 75.0F, 0.0F}, 0},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

/* each character's bosses in order, as stage and act pairs, ending at
 * STAGE_NONE. a boss picked from the menus goes on to the next */
static Sint16 sonic_bosses[] = {STAGE_CHAOS0,     0, STAGE_EGGMOBILE1, 0,
                                STAGE_CHAOS4,     0, STAGE_CHAOS6,     0,
                                STAGE_EGGMOBILE3, 0, STAGE_NONE};
static Sint16 tails_bosses[] = {STAGE_EGGMOBILE1, 0, STAGE_CHAOS4, 0,
                                STAGE_EGGMOBILE2, 0, STAGE_NONE};
static Sint16 knuckles_bosses[] = {STAGE_CHAOS2, 0, STAGE_CHAOS4, 0,
                                   STAGE_CHAOS6, 1, STAGE_NONE};
static Sint16 amy_bosses[] = {STAGE_EGGMANROBO, 0, STAGE_NONE};
static Sint16 e102_bosses[] = {STAGE_E101, 0, STAGE_E101_R, 0, STAGE_NONE};
static Sint16 big_bosses[] = {STAGE_CHAOS6, 0, STAGE_NONE};

/* where each character comes back to the field after each stage it
 * quits */
static struct return_point sonic_returns[] = {
    {STAGE_BEACH, STAGE_STATION_SQUARE, {-492.8F, 0.0F, 2053.8F}, 0xdba9},
    {STAGE_TWINKLE, STAGE_STATION_SQUARE, {785.3F, 50.0F, 1771.7F}, 0x7ab7},
    {STAGE_HIGHWAY, STAGE_STATION_SQUARE, {347.8F, 0.0F, 1370.3F}, 0x7fb9},
    {STAGE_CASINO, STAGE_STATION_SQUARE, {-572.5F, -2.5F, 939.5F}, 0x1f17},
    {STAGE_WINDY, STAGE_MYSTIC_RUINS, {644.9F, 59.1F, -155.0F}, 0x6000},
    {STAGE_ICECAP, STAGE_MYSTIC_RUINS, {-1290.0F, 27.5F, 315.0F}, 0xec68},
    {STAGE_MOUNTAIN, STAGE_MYSTIC_RUINS, {-1945.7F, -351.5F, 1646.6F}, 0x05f9},
    {STAGE_RUIN, STAGE_MYSTIC_RUINS, {-515.0F, 89.0F, -1054.0F}, 0x4000},
    {STAGE_FINAL, STAGE_MYSTIC_RUINS, {133.4F, 108.4F, -7.2F}, 0x70ef},
    {STAGE_SKY_DECK, STAGE_EGG_CARRIER, {0.0F, 655.0F, 146.0F}, 0xc000},
    {STAGE_SUNABOARD, STAGE_MYSTIC_RUINS, {-1492.0F, 40.8F, -66.6F}, 0x036c},
    {STAGE_CART, STAGE_STATION_SQUARE, {757.7F, 50.0F, 1747.0F}, 0x4000},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct return_point tails_returns[] = {
    {STAGE_HIGHWAY, STAGE_STATION_SQUARE, {347.8F, 0.0F, 1370.3F}, 0x7fb9},
    {STAGE_CASINO, STAGE_STATION_SQUARE, {-572.5F, -2.5F, 939.5F}, 0x1f17},
    {STAGE_WINDY, STAGE_MYSTIC_RUINS, {644.9F, 59.1F, -155.0F}, 0x6000},
    {STAGE_ICECAP, STAGE_MYSTIC_RUINS, {-1290.0F, 27.5F, 315.0F}, 0xec68},
    {STAGE_SKY_DECK, STAGE_EGG_CARRIER, {0.0F, 655.0F, 146.0F}, 0xc000},
    {STAGE_SUNABOARD, STAGE_MYSTIC_RUINS, {-1492.0F, 40.8F, -66.6F}, 0x036c},
    {STAGE_CART, STAGE_STATION_SQUARE, {757.7F, 50.0F, 1747.0F}, 0x4000},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct return_point knuckles_returns[] = {
    {STAGE_HIGHWAY, STAGE_STATION_SQUARE, {272.0F, 4.0F, 294.9F}, 0x3d0c},
    {STAGE_CASINO, STAGE_STATION_SQUARE, {-572.5F, -2.5F, 939.5F}, 0x1f17},
    {STAGE_MOUNTAIN, STAGE_MYSTIC_RUINS, {-1945.7F, -351.5F, 1646.6F}, 0x05f9},
    {STAGE_RUIN, STAGE_MYSTIC_RUINS, {-515.9F, 16.6F, -1446.0F}, 0xc000},
    {STAGE_SKY_DECK, STAGE_EGG_CARRIER, {290.0F, 17.0F, 0.0F}, 0x8000},
    {STAGE_CART, STAGE_STATION_SQUARE, {757.7F, 50.0F, 1747.0F}, 0x4000},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct return_point amy_returns[] = {
    {STAGE_TWINKLE, STAGE_STATION_SQUARE, {785.3F, 50.0F, 1771.7F}, 0x7ab7},
    {STAGE_FINAL, STAGE_MYSTIC_RUINS, {133.4F, 108.4F, -7.2F}, 0x70ef},
    {STAGE_SHELTER, STAGE_EGG_CARRIER, {0.0F, 14.0F, 160.0F}, 0xc000},
    {STAGE_CART, STAGE_STATION_SQUARE, {757.7F, 50.0F, 1747.0F}, 0x4000},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct return_point big_returns[] = {
    {STAGE_BEACH, STAGE_STATION_SQUARE, {-492.8F, 0.0F, 2053.8F}, 0xdba9},
    {STAGE_TWINKLE, STAGE_STATION_SQUARE, {785.3F, 50.0F, 1771.7F}, 0x7ab7},
    {STAGE_SHELTER, STAGE_EGG_CARRIER, {0.0F, 14.0F, 160.0F}, 0xc000},
    {STAGE_ICECAP, STAGE_MYSTIC_RUINS, {-1290.0F, 27.5F, 315.0F}, 0xec68},
    {STAGE_CART, STAGE_STATION_SQUARE, {757.7F, 50.0F, 1747.0F}, 0x4000},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct return_point e102_returns[] = {
    {STAGE_BEACH, STAGE_STATION_SQUARE, {-492.8F, 0.0F, 2053.8F}, 0xdba9},
    {STAGE_SHELTER, STAGE_EGG_CARRIER, {0.0F, 14.0F, 160.0F}, 0xc000},
    {STAGE_FINAL, STAGE_MYSTIC_RUINS, {-0.5F, 108.8F, -138.1F}, 0x4537},
    {STAGE_WINDY, STAGE_MYSTIC_RUINS, {644.9F, 59.1F, -155.0F}, 0x6000},
    {STAGE_MOUNTAIN, STAGE_MYSTIC_RUINS, {-1945.7F, -351.5F, 1646.6F}, 0x05f9},
    {STAGE_CART, STAGE_STATION_SQUARE, {757.7F, 50.0F, 1747.0F}, 0x4000},
    {STAGE_NONE, 0, {0.0F, 0.0F, 0.0F}, 0},
};

static struct return_point *returns[CHARACTER_COUNT] = {
    sonic_returns, NULL,        tails_returns, knuckles_returns,
    NULL,          amy_returns, e102_returns,  big_returns,
};

/* how the act ends, for the stage's step to act on */
static Sint16 stage_end_kind = STAGE_END_NONE;
static Sint16 pause_enabled = 1; /* 1 lets start open the pause menu */
/* where the player comes back to the field, or NULL; stage_place_player
 * uses it once */
static struct return_point *return_point = NULL;

/* each stage's task step, in its module, or NULL */
static void (*stage_steps[STAGE_COUNT])(struct task *task) = {
    practice_step,
    beach_step,
    windy_step,
    twinkle_step,
    highway_step,
    mountain_step,
    sky_deck_step,
    ruin_step,
    icecap_step,
    casino_step,
    final_step,
    NULL,
    shelter_step,
    NULL,
    NULL,
    chaos0_step,
    chaos2_step,
    chaos4_step,
    chaos6_step,
    chaos7_step,
    eggmobile1_step,
    eggmobile2_step,
    eggmobile3_step,
    eggmanrobo_step,
    e101_step,
    e101_r_step,
    station_square_step,
    station_square_step,
    station_square_step,
    egg_carrier_step,
    NULL,
    NULL,
    egg_carrier_inside_step,
    mystic_ruins_step,
    past_step,
    cart_step,
    shooting_act1_step,
    shooting_act2_step,
    sunaboard_step,
    station_square_garden_step,
    egg_carrier_garden_step,
    mystic_ruins_garden_step,
    alife_race_step,
};

static struct back_color back_colors[STAGE_COUNT] = {
#include "stage/stage/back_colors.inc"
};

/* each stage's sky task step, in its module, or NULL */
static void (*sky_steps[STAGE_COUNT])(struct task *task) = {
    NULL,
    beach_sky_step,
    windy_sky_step,
    twinkle_sky_step,
    highway_sky_step,
    mountain_sky_step,
    sky_deck_sky_step,
    ruin_sky_step,
    icecap_sky_step,
    NULL,
    final_sky_step,
    NULL,
    shelter_sky_step,
    NULL,
    NULL,
    chaos0_sky_step,
    chaos2_sky_step,
    chaos4_sky_step,
    chaos6_sky_step,
    chaos7_sky_step,
    eggmobile1_sky_step,
    eggmobile2_sky_step,
    eggmobile3_sky_step,
    eggmanrobo_sky_step,
    e101_sky_step,
    e101_r_sky_step,
    station_square_sky_step,
    station_square_sky_step,
    station_square_sky_step,
    egg_carrier_sky_step,
    NULL,
    NULL,
    egg_carrier_inside_sky_step,
    mystic_ruins_sky_step,
    past_sky_step,
    cart_sky_step,
    NULL,
    NULL,
    sunaboard_sky_step,
    NULL,
    NULL,
    NULL,
    race_sky_step,
};

static struct stage_exit sonic_exits[] = {
    {0, STAGE_CHAOS0, STAGE_STATION_SQUARE, 4, 0, 0, 0, 0},
    {0, STAGE_BEACH, STAGE_STATION_SQUARE, 4, 2, STAGE_STATION_SQUARE, 4, 2},
    {0, STAGE_EGGMOBILE1, STAGE_MYSTIC_RUINS, 0, 0, 0, 0, 0},
    {0, STAGE_WINDY, STAGE_MYSTIC_RUINS, 0, 3, STAGE_MYSTIC_RUINS, 0, 3},
    {0, STAGE_CASINO, STAGE_STATION_SQUARE, 1, 1, STAGE_STATION_SQUARE, 1, 1},
    {0, STAGE_ICECAP, STAGE_MYSTIC_RUINS, 1, 2, STAGE_MYSTIC_RUINS, 1, 2},
    {MOVIE_SA6, STAGE_CHAOS4, STAGE_MYSTIC_RUINS, 0, 0, 0, 0, 0},
    {0, STAGE_TWINKLE, STAGE_STATION_SQUARE, 5, 1, STAGE_STATION_SQUARE, 5, 1},
    {0, STAGE_MOUNTAIN, STAGE_SHOOTING2, 0, 0, STAGE_MYSTIC_RUINS, 1, 1},
    {0, STAGE_HIGHWAY, STAGE_STATION_SQUARE, 0, 0, STAGE_STATION_SQUARE, 0, 0},
    {0, STAGE_SKY_DECK, STAGE_EGG_CARRIER_INSIDE, 1, 0,
     STAGE_EGG_CARRIER_INSIDE, 1, 0},
    {0, STAGE_CHAOS6, STAGE_EGG_CARRIER, 0, 0, 0, 0, 0},
    {0, STAGE_RUIN, STAGE_PAST, 2, 1, STAGE_MYSTIC_RUINS, 2, 1},
    {0, STAGE_FINAL, STAGE_EGGMOBILE3, 0, 0, STAGE_MYSTIC_RUINS, 3, 3},
    {0, STAGE_EGGMOBILE3, STAGE_MYSTIC_RUINS, 0, 0, 0, 0, 0},
    {0, STAGE_SHOOTING1, STAGE_STATION_SQUARE, 4, 0, 0, 0, 0},
    {0, STAGE_SHOOTING2, STAGE_EGG_CARRIER, 0, 0, 0, 0, 0},
#ifdef GCC
    /* keeps Sonic's exit from Sand Hill: Tails' row, which the image's walk
     * reads on to, as the tag in stage_set_next_exit_from says. GCC's walk
     * ends at EXIT_END, so this row copies Tails' */
    {0, STAGE_SUNABOARD, STAGE_PAST, 0, 0, STAGE_MYSTIC_RUINS, 2, 1},
#endif
    {0, EXIT_END, 0, 0, 0, 0, 0, 0},
};

static struct stage_exit tails_exits[] = {
    {0, STAGE_BEACH, STAGE_STATION_SQUARE, 4, 0, 0, 0, 0},
    {0, STAGE_EGGMOBILE1, STAGE_MYSTIC_RUINS, 0, 0, 0, 0, 0},
    {0, STAGE_WINDY, STAGE_MYSTIC_RUINS, 0, 3, STAGE_MYSTIC_RUINS, 0, 3},
    {0, STAGE_CASINO, STAGE_STATION_SQUARE, 1, 1, STAGE_STATION_SQUARE, 1, 1},
    {0, STAGE_ICECAP, STAGE_MYSTIC_RUINS, 1, 2, STAGE_MYSTIC_RUINS, 1, 2},
    {MOVIE_SA6, STAGE_CHAOS4, STAGE_MYSTIC_RUINS, 0, 5, 0, 0, 0},
    {0, STAGE_MOUNTAIN, STAGE_SHOOTING2, 0, 0, 0, 0, 0},
    {0, STAGE_SKY_DECK, STAGE_EGG_CARRIER_INSIDE, 1, 0,
     STAGE_EGG_CARRIER_INSIDE, 1, 0},
    {0, STAGE_HIGHWAY, STAGE_STATION_SQUARE, 1, 0, STAGE_STATION_SQUARE, 0, 0},
    {0, STAGE_EGGMOBILE2, STAGE_STATION_SQUARE, 1, 0, 0, 0, 0},
    {0, STAGE_SUNABOARD, STAGE_PAST, 0, 0, STAGE_MYSTIC_RUINS, 2, 1},
    {0, STAGE_SHOOTING1, STAGE_MYSTIC_RUINS, 2, 0, 0, 0, 0},
    {0, STAGE_SHOOTING2, STAGE_EGG_CARRIER, 0, 0, 0, 0, 0},
    {0, EXIT_END, 0, 0, 0, 0, 0, 0},
};

static struct stage_exit knuckles_exits[] = {
    {0, STAGE_HIGHWAY, STAGE_STATION_SQUARE, 0, 0, STAGE_STATION_SQUARE, 0, 0},
    {0, STAGE_CASINO, STAGE_PAST, 0, 1, STAGE_STATION_SQUARE, 1, 1},
    {0, STAGE_CHAOS2, STAGE_STATION_SQUARE, 4, 5, STAGE_STATION_SQUARE, 4, 5},
    {0, STAGE_MOUNTAIN, STAGE_MYSTIC_RUINS, 1, 1, STAGE_MYSTIC_RUINS, 1, 1},
    {MOVIE_SA6, STAGE_CHAOS4, STAGE_MYSTIC_RUINS, 0, 5, 0, 0, 0},
    {0, STAGE_RUIN, STAGE_PAST, 1, 0, STAGE_MYSTIC_RUINS, 2, 2},
    {0, STAGE_SKY_DECK, STAGE_PAST, 2, 0, STAGE_EGG_CARRIER, 5, 1},
    {0, STAGE_CHAOS6, STAGE_EGG_CARRIER, 0, 0, 0, 0, 0},
    {0, EXIT_END, 0, 0, 0, 0, 0, 0},
};

static struct stage_exit amy_exits[] = {
    {0, STAGE_TWINKLE, STAGE_STATION_SQUARE, 5, 1, STAGE_STATION_SQUARE, 5, 1},
    {0, STAGE_SHELTER, STAGE_PAST, 0, 0, STAGE_EGG_CARRIER, 5, 0},
    {0, STAGE_FINAL, STAGE_MYSTIC_RUINS, 3, 0, STAGE_MYSTIC_RUINS, 3, 0},
    {0, STAGE_EGGMANROBO, STAGE_EGG_CARRIER, 0, 0, 0, 0, 0},
    {0, EXIT_END, 0, 0, 0, 0, 0, 0},
};

static struct stage_exit e102_exits[] = {
    {0, STAGE_FINAL, STAGE_MYSTIC_RUINS, 3, 0, STAGE_MYSTIC_RUINS, 3, 0},
    {0, STAGE_E101, STAGE_MYSTIC_RUINS, 3, 0, 0, 0, 0},
    {0, STAGE_BEACH, STAGE_PAST, 1, 0, STAGE_STATION_SQUARE, 4, 1},
    {0, STAGE_WINDY, STAGE_MYSTIC_RUINS, 0, 0, STAGE_MYSTIC_RUINS, 0, 0},
    {0, STAGE_MOUNTAIN, STAGE_MYSTIC_RUINS, 1, 1, STAGE_MYSTIC_RUINS, 1, 1},
    {0, STAGE_SHELTER, STAGE_EGG_CARRIER_INSIDE, 1, 1, STAGE_EGG_CARRIER_INSIDE,
     1, 1},
    {0, STAGE_E101_R, STAGE_EGG_CARRIER, 0, 0, 0, 0, 0},
    {0, EXIT_END, 0, 0, 0, 0, 0, 0},
};

static struct stage_exit big_exits[] = {
    {0, STAGE_TWINKLE, STAGE_STATION_SQUARE, 5, 1, STAGE_STATION_SQUARE, 5, 1},
    {0, STAGE_ICECAP, STAGE_MYSTIC_RUINS, 1, 2, STAGE_MYSTIC_RUINS, 1, 2},
    {MOVIE_SA8, STAGE_BEACH, STAGE_EGG_CARRIER_INSIDE, 1, 0,
     STAGE_STATION_SQUARE, 4, 1},
    {0, STAGE_SHELTER, STAGE_PAST, 1, 0, STAGE_EGG_CARRIER_INSIDE, 1, 1},
    {0, STAGE_CHAOS6, STAGE_EGG_CARRIER, 0, 0, 0, 0, 0},
    {0, EXIT_END, 0, 0, 0, 0, 0, 0},
};

static struct stage_exit *exits[CHARACTER_COUNT] = {
    sonic_exits, NULL,      tails_exits, knuckles_exits,
    NULL,        amy_exits, e102_exits,  big_exits,
};

int event_ending;
Sint16 stage_mode;
Sint16 stage_previous;
Sint16 act_previous;
Sint16 stage_current;
Sint16 act_current;
Sint16 stage_loaded;
Sint16 act_loaded;
Sint16 stage_next;
Sint16 act_next;
Sint16 entrance_next;
Uint16 character_current;
Sint16 stage_retries;
Sint16 pause_choice;
Sint16 pause_hidden;
Sint16 continue_choice;
Sint16 continue_timer;
struct particle stage_particles[PARTICLE_COUNT];
int dat_8c75b734;
struct task *stage_task;
Uint16 collision_item_count;
Uint16 collision_enemy_count;
Sint8 debug_hidden;
Sint8 hud_hidden;
Sint8 lives_shown;
Sint8 rings_shown;
Sint8 lives;
Sint8 time_minutes;
Sint8 time_seconds;
Sint8 time_frames;
Sint16 rings;
int play_time;
int time_steps;
int score;
int bonus_points;
int trial_count;
Float shoot_hit_rate;
int hints_used;
#ifdef GCC
Bool stage_limit_cleared;
#endif
struct trial_time trial_times[TRIAL_TIMES];
struct mirror_list mirrors;
NJS_VECTOR scratch_vector;
NJS_VECTOR scratch_vector2;
NJS_VECTOR scratch_vector3;
Uint8 dat_8c75d5dc[12];
NJS_POINT3 scratch_point;
NJS_POINT3 scratch_point2;
NJS_MATRIX scratch_matrix;
Uint8 dat_8c75d640[192];
struct segment_query scratch_segment_query;
NJS_VECTOR world_gravity;
Angle world_gravity_ang_x;
Angle world_gravity_ang_z;
struct story_record *player_record;
int dat_8c75d758;
Sint8 debug_unlocked;

void pause_enable(void)
{
    pause_enabled = 1;
}

void pause_disable(void)
{
    pause_enabled = 0;
}

void boot_new_game(void)
{
    int found;

    set_character(CHARACTER_SONIC);
    play_time_reset();
    hud_reset();
    progress_init();
    messages_shown = 0;
    stage_begin();
    save_reset_all();
    /* @hack spelling: the result through found, which is tested with
     * TST R3,R4 against a 1 in R3 (0x8c04f6be to 0x8c04f6c2); tested in
     * place, with TST #1,R0, 14 of 37 words differ */
    found = save_file_exists();
    if (found & 1) {
        save_read();
    } else {
        /* @hack redundant: an empty else, which keeps debug_unlocked's address
         * in R2 and the 0 in R1 (0x8c04f6cc, 0x8c04f6ce); without it, R1 and R2
         * trade and 3 of 37 words differ */
    }
    debug_unlocked = 0;
    srand(0xdead0cab);
}

/* wait out an event. once it lets go of control, the task ends, and the
 * pads and the time start again. the pause is enabled again too, but in
 * Chaos 7 */
static void event_task_wait(struct task *task)
{
    if (!event_in_control()) {
        task_destroy(task);
        pad_enable();
        if (stage_current != STAGE_CHAOS7) {
            pause_enabled = 1;
        }
        time_start();
    }
}

/* play the event the task's timer holds, read unsigned, then wait it out */
static void event_task_start(struct task *task)
{
    stream_destroy();
    event_load((Uint16)task->work->timer);
    event_start();
    task->step = event_task_wait;
}

/* in a demo with an event, start the demo's event task. else start a task
 * that plays the stage's entry event for the character, if it hasn't
 * played. a stage picked from the menus plays none. return TRUE if a task
 * starts */
static Bool stage_start_event(void)
{
    struct stage_event *event = entry_events;
    struct task *task;

    if (demo_playing) {
        demo_event_ended = 0;
        if (demo_event >= 0) {
            task_create(0, 0, demo_event_start);
            return TRUE;
        }
    }
    if (game_mode == GAME_MODE_TRIAL) {
        return FALSE;
    }
    for (; event->stage != STAGE_NONE; event++) {
        if (event->stage == stage_current && event->act == act_current &&
            event->character == character_current) {
            if (event_was_played(event->event)) {
                /* @hack spelling: an empty arm, which stores the timer through
                 * R3 (0x8c04f846, 0x8c04f848); with the test inverted, through
                 * R2, and 2 of 76 words differ */
            } else {
                task = task_create(TASK_WORK, 0, event_task_start);
                task->work->timer = event->event;
                task->destructor = event->destructor;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void clear_event_end(struct task *task)
{
    stage_end(STAGE_END_CLEAR);
}

/* a task's first step while the clear's event plays: wait it out, then
 * end the act with a clear */
static void clear_event_task(struct task *task)
{
    task->step = event_task_wait;
    task->destructor = clear_event_end;
}

/* start the clear's event for the stage, act and character, if it hasn't
 * played, with a task that clears the act once it's over. return TRUE if
 * it starts. a stage picked from the menus plays none */
static Bool stage_clear_event(void)
{
    struct stage_event *event = clear_events;

    if (game_mode == GAME_MODE_TRIAL) {
        return FALSE;
    }
    for (; event->stage != STAGE_NONE; event++) {
        if (event->stage == stage_current && event->act == act_current &&
            event->character == character_current) {
            if (!event_was_played(event->event)) {
                stream_destroy();
                event_load(event->event);
                event_start();
                task_create(0, 0, clear_event_task);
                event_ending = -1;
                return TRUE;
            }
        }
    }
    return FALSE;
}

enum stage_group stage_group(void)
{
    /* @hack type: the stage in an int, which SHC compares as MOV.W loads it
     * (0x8c04f964, 0x8c04f966); as a Sint16, it sign-extends it again and 21
     * of 23 words differ */
    int stage = stage_current;
    enum stage_group group;

    if (stage <= STAGE_ACTION_LAST) {
        group = STAGE_GROUP_ACTION;
    } else if (stage <= STAGE_BOSS_LAST) {
        group = STAGE_GROUP_ACTION;
    } else if (stage <= STAGE_PAST) {
        group = STAGE_GROUP_FIELD;
    } else if (stage <= STAGE_SUNABOARD) {
        group = STAGE_GROUP_ACTION;
    } else {
        group = STAGE_GROUP_FIELD;
    }
    return group;
}

enum stage_group stage_begin(void)
{
    stage_mode = STAGE_TITLE_CARD;
    stage_end_kind = STAGE_END_NONE;
    movie_request = 0;
    world_reset_gravity();
    pause_choice = PAUSE_CONTINUE;
    pause_hidden = 0;
    return_point = NULL;
    if (saves[save_slot].flags & SAVE_NO_MESSAGES) {
        messages_shown = 0;
    } else {
        messages_shown = 1;
    }
    if (lives_count() < LIVES_START) {
        lives_reset();
    }
    return stage_group();
}

/* SHC expands each function from here to fade_from_black at every call */
/* NOLINTBEGIN(readability-redundant-control-flow) */
static void start_play(void)
{
    stage_start();
    /* @hack redundant: a return where the function ends, after which
     * stage_step_field loads fade_in_start's address into R3 (0x8c050b82);
     * without it, stage_step_action runs 32 bytes long and 1250 of the two
     * steps' 2134 words differ */
    return;
}

static void stop_play(void)
{
    stage_stop();
    /* @hack redundant: a return where the function ends, after which
     * forget_end's expansion in field_go_next stores the 0 in R0 through R1
     * (0x8c0509be to 0x8c0509c6); without it, R0 and R1 trade and 87 of the 975
     * words of field_go_next and stage_step_field differ */
    return;
}

static void stop_ended(void)
{
    stage_stop_counted();
    stage_end_kind = STAGE_END_NONE;
    /* @hack redundant: a return where the function ends, after which
     * action_leave_cleared keeps stage_current's address in R0 and
     * story_mark_cleared's in R3 (0x8c05081c, 0x8c050820); without it, they
     * take R1 and R2, and 179 of the 1328 words of the three functions that
     * expand it differ */
    return;
}

static void forget_end(void)
{
    stage_end_kind = STAGE_END_NONE;
    /* @hack redundant: a return where the function ends, which has each
     * expansion store the 0 before the next call's address loads, as
     * action_go_next's does before fade_in_start's (0x8c050896 to 0x8c05089c);
     * without it, the load comes first, and 153 of the 2272 words of the four
     * functions that expand it differ */
    return;
}

static void reset_steps(void)
{
    stage_mode = STAGE_TITLE_CARD;
    stage_end_kind = STAGE_END_NONE;
    /* @hack redundant: a return where the function ends, which has
     * stage_step_action's expansion store the mode itself and branch past the
     * shared stores at 0x8c0507f2 (MOV.W R10,@R14 and BRA at 0x8c050546,
     * 0x8c050548); without it, the expansion branches to them, and 55 of the
     * two steps' 2134 words differ */
    return;
}

static void pause_sound(void)
{
    sound_play(SE_PAUSE, 0, 0, 0);
    /* @hack redundant: a return where the function ends, after which
     * stage_step_action loads pause_choice's address into R2 and its value into
     * R3 (0x8c050004, 0x8c050006); without it, R2 and R3 trade and 10 of the
     * two steps' 2134 words differ */
    return;
}

static void pause_restart(void)
{
    stage_mode = STAGE_PLAY;
    stage_end(STAGE_END_RESTART);
    /* @hack redundant: a return where the function ends, after which each
     * step's next arm loads history_write's address into R1 (0x8c050060 in
     * stage_step_action, 0x8c050fe0 in stage_step_field); without it, into R3,
     * and 137 of the two steps' 2134 words differ */
    return;
}

static Sint16 end_kind(void)
{
    return stage_end_kind;
}

static void stop_time_and_pausing(void)
{
    time_stop();
    pause_enabled = 0;
    /* @hack redundant: a return where the function ends, which has the quit's
     * arm branch from its pad_disable call to the time_stop call at 0x8c04fcd2
     * (BRA at 0x8c04fcae); without it, SHC writes the call out in that arm,
     * stage_step_action runs 6 bytes long and 62 of its 1235 words differ */
    return;
}

static enum stage_group group(void)
{
    return stage_group();
}

static void save_now(void)
{
    save_write_wait();
    /* @hack redundant: a return where the function ends, after which
     * stage_step_action loads stage_unload's address into R3 and
     * stage_keep_previous's into R2 (0x8c05051a, 0x8c050520); without it, R2
     * and R3 trade and 10 of stage_step_action's 1235 words differ */
    return;
}

static void step_tasks(void)
{
    task_step_all();
    /* @hack redundant: a return where the function ends, which lets the arms of
     * stage_step_action that set the mode share the store at 0x8c050782,
     * each setting R2 in its branch's slot (BRA at 0x8c04fb90, 0x8c0504e6);
     * without it, each arm stores its own, and 111 of the two steps' 2134
     * words differ */
    return;
}

static void cover(NJS_COLOR *color)
{
    fade_set_cover(color);
    /* @hack redundant: a return where the function ends, after which
     * stage_step_field loads fade_out_step's address into R3 (0x8c05117a);
     * without it, into R2, and 2 of stage_step_field's 899 words differ */
    return;
}

static void fade_from_black(void)
{
    fade_in_start();
    /* @hack redundant: a return where the function ends, which keeps the
     * shooting stages' own call of texture_load_regular in stage_load
     * (0x8c051654 to 0x8c051658); without it, their arm branches into the
     * default arm's call, with the 2 in the branch's slot, and 81 of 142 words
     * differ */
    return;
}
/* NOLINTEND(readability-redundant-control-flow) */

static void load_textures(int character)
{
    texture_load_character(character);
    texture_load_regular(0);
}

Sint16 stage_step_action(void)
{
    Sint16 result;
    /* @hack spelling: the title card's results through status, which copies
     * title_card_load's to R4 and sign-extends it from there (0x8c04fb04,
     * 0x8c04fb06); tested in place, SHC skips the copy and 181 of 1235 words
     * differ */
    Sint16 status;
    Uint8 stage;
    Uint8 act;
    NJS_COLOR color;

    switch (stage_mode) {
    case STAGE_TITLE_CARD:
        /* @hack redundant: the breaks in the arms, which leave stage_mode++ in
         * R2 (0x8c04fb10 to 0x8c04fb16); without them, R3, and 5 of 1235 words
         * differ */
        if (demo_playing) {
            stage_mode = STAGE_ENTER;
            break;
        } else {
            color.argb.b = color.argb.g = color.argb.r = color.argb.a = 0;
            fade_set_cover(&color);
            status = title_card_load(TITLE_CARD_FRAMES);
            if (status == -1) {
                stage_mode = STAGE_ENTER;
                break;
            } else {
                stage_mode++;
            }
        }
        break;
    case STAGE_TITLE_CARD_SHOW:
        status = title_card_draw(0);
        if (status == 1) {
            draw_off = FALSE;
            stage_load();
            start_play();
            fade_in_start();
            mission_card_create();
            /* run the fade through its hold on black at once, so the black
             * lifts from this frame */
            while (fade_in_step() != 0) {
            }
            /* @hack redundant: stage_start_event's result tested, though both
             * arms set STAGE_FADE_IN, which stores the mode in a BRA's slot
             * after TST R0,R0 (0x8c04fb52 to 0x8c04fb5a); called alone, 86 of
             * 1235 words differ */
            if (stage_start_event()) {
                stage_mode = STAGE_FADE_IN;
            } else {
                stage_mode = STAGE_FADE_IN;
            }
        }
        break;
    case STAGE_ENTER:
        draw_off = FALSE;
        fade_in_start();
        stage_mode = STAGE_ENTER_WAIT;
        /* fall through */
    case STAGE_ENTER_WAIT:
        if (fade_in_step() == 0) {
            stage_load();
            start_play();
            mission_card_create();
            /* @hack redundant: stage_start_event's result tested, though both
             * arms set STAGE_FADE_IN, as in STAGE_TITLE_CARD_SHOW (0x8c04fb8c
             * to 0x8c04fb92); called alone, 85 of 1235 words differ */
            if (stage_start_event()) {
                stage_mode = STAGE_FADE_IN;
            } else {
                stage_mode = STAGE_FADE_IN;
            }
        }
        break;
    case STAGE_AGAIN:
        fade_in_start();
        stage_mode = STAGE_AGAIN_WAIT;
        /* fall through */
    case STAGE_AGAIN_WAIT:
        if (fade_in_step() == 0) {
            start_play();
            stage_mode = STAGE_FADE_IN;
        }
        break;
    case STAGE_FADE_IN:
        result = fade_in_step();
        stage_run_action();
        if (result != 0) {
            stage_mode = STAGE_PLAY;
            if (!event_in_control()) {
                pad_enable();
                time_start();
                if (recording_kind == RECORDING_OFF) {
                    pause_enabled = 1;
                }
            }
        }
        break;
    case STAGE_PLAY:
        stage_run_action();
        if (stage_mode == STAGE_CARD_ERROR) {
            sound_play(SE_PAUSE, 0, 0, 0);
            pause_choice = PAUSE_CONTINUE;
            sound_pause();
            sound_mute();
            break;
        }
        /* outside a demo, quitting a level, the cart or sunaboard leads back
         * to the field */
        if (end_kind() == STAGE_END_QUIT) {
            forget_end();
            if ((stage_current <= STAGE_ACTION_LAST ||
                 stage_current == STAGE_CART ||
                 stage_current == STAGE_SUNABOARD) &&
                !demo_playing) {
                stage_set_next_return();
                stage_mode = STAGE_GO_TO_FADE;
            } else {
                stage_mode = STAGE_QUIT_FADE;
            }
            fade_out_start();
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_CLEAR) {
            forget_end();
            /* the last boss leaves for the ending */
            if (stage_current == STAGE_CHAOS7) {
                draw_off = TRUE;
                stage_mode = STAGE_ENDING;
                fade_out_start();
                pad_disable();
                time_stop();
                pause_enabled = 0;
            } else {
                stage_mode = STAGE_CLEAR_FADE;
                /* after an event, while event_ending is set, leave at once,
                 * without a fade */
                if (event_ending) {
                    action_leave_cleared();
                    break;
                }
                fade_out_start();
                pad_disable();
                stop_time_and_pausing();
            }
        } else if (end_kind() == STAGE_END_MISS) {
            /* a boss picked from the menus takes no lives, and the menus
             * have no game over */
            forget_end();
            if (game_mode == GAME_MODE_TRIAL &&
                stage_current > STAGE_ACTION_LAST &&
                stage_current <= STAGE_BOSS_LAST) {
                stage_mode = STAGE_MISS_FADE;
                fade_out_start();
            } else if (lives_count() <= 0) {
                if (game_mode == GAME_MODE_TRIAL) {
                    stage_mode = STAGE_MISS_FADE;
                    fade_out_start();
                } else {
                    stage_mode = STAGE_GAME_OVER_FADE;
                    game_over_start();
                }
            } else {
                stage_mode = STAGE_MISS_FADE;
                fade_out_start();
            }
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_GO_TO) {
            forget_end();
            stage_mode = STAGE_GO_TO_FADE;
            fade_out_start();
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_RESTART) {
            forget_end();
            if (stage_current == STAGE_CART) {
                stage_mode = STAGE_RESTART_FADE;
                fade_out_start();
            } else if (game_mode != GAME_MODE_TRIAL && lives_count() <= 0) {
                stage_mode = STAGE_GAME_OVER_FADE;
                game_over_start();
            } else {
                stage_mode = STAGE_MISS_FADE;
                fade_out_start();
            }
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_RETRY) {
            forget_end();
            if (lives_count() <= 0) {
                stage_mode = STAGE_GAME_OVER_FADE;
                game_over_start();
            } else {
                stage_mode = STAGE_RETRY_FADE;
                fade_out_start();
            }
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if ((per_info[0]->press & PDD_DGT_ST) || demo_event_ended) {
            if (recording_kind != RECORDING_OFF) {
                if (demo_event >= 0 && demo_event_ended) {
                    stop_ended();
                    stage_mode = STAGE_QUIT;
                } else {
                    stage_end(STAGE_END_QUIT);
                }
                if (!demo_event_ended) {
                    recording_kind = RECORDING_OFF;
                }
            } else if (pad_records[0].on & PDD_DGT_ST) {
                if (pause_enabled == 1 && !event_in_control()) {
                    sound_play(SE_PAUSE, 0, 0, 0);
                    pause_choice = PAUSE_CONTINUE;
                    stage_mode = STAGE_PAUSE;
                    sound_pause();
                    sound_mute();
                }
            }
        } else {
            /* both triggers, one newly pressed, name the next frame's file.
             * the four face buttons, all held with one newly pressed, flip
             * hud_hidden, which stage_run_step sets again at the next step */
            if (((per_info[0]->press & PDD_DGT_TL) &&
                 (per_info[0]->on & PDD_DGT_TR)) ||
                ((per_info[0]->on & PDD_DGT_TL) &&
                 (per_info[0]->press & PDD_DGT_TR))) {
                debug_name_frame();
            } else if ((per_info[0]->press & HUD_BUTTONS) &&
                       (per_info[0]->on & HUD_BUTTONS) == HUD_BUTTONS) {
                hud_hidden = ~hud_hidden;
            }
        }
        break;
    case STAGE_PAUSE:
        stage_run_paused();
        if (per_info[0]->press & PDD_DGT_ST) {
            pause_sound();
            /* @hack type: pause_choice read as a Uint16, which loads its
             * address into R2 and its value into R3 (0x8c050004, 0x8c050006);
             * as a Sint16, R2 and R3 trade and 6 of 1235 words differ */
            if (*(Uint16 *)&pause_choice == PAUSE_CONTINUE) {
                stage_mode = STAGE_PLAY;
            } else if (pause_choice == PAUSE_RESTART) {
                pause_restart();
            } else {
                history_write();
                if (game_mode == GAME_MODE_TRIAL) {
                    stage_stop_counted();
                    stage_mode = STAGE_TRIAL_END;
                } else if (act_current >= 1 && stage_current == STAGE_CART) {
                    stage_stop_counted();
                    stage_mode = STAGE_QUIT;
                } else if (stage_current <= STAGE_ACTION_LAST ||
                           stage_current == STAGE_CART ||
                           stage_current == STAGE_SUNABOARD) {
                    /* a level, the cart or sunaboard leads back to the
                     * field */
                    stage_mode = STAGE_PLAY;
                    stage_set_next_return();
                    /* @hack spelling: the stage and act through locals, which
                     * the image keeps in the frame for the call (0x8c0500b8,
                     * 0x8c0500bc); passed straight, 41 of 1235 words differ */
                    /* @hack spelling: the act read first, which loads
                     * stage_next's address into R1 and act_next's into R3
                     * (0x8c0500b2, 0x8c0500b4); read the other way round, 10
                     * of 1235 words differ */
                    act = act_next;
                    stage = stage_next;
                    stage_fade_to(stage, act);
                    download_reset();
                } else {
                    stage_stop_counted();
                    stage_mode = STAGE_QUIT;
                }
            }
            /* with the pads recorded or played back, the act starts over,
             * at no cost */
            if (recording_kind == RECORDING_OFF) {
                recording_on = FALSE;
            } else {
                lives_add(1);
                stage_end(STAGE_END_MISS);
            }
            if (pause_choice != PAUSE_CONTINUE) {
                sound_end_act();
            } else {
                sound_resume(15);
            }
            sound_unmute();
        }
        break;
    case STAGE_CLEAR_FADE:
        result = fade_out_step();
        stage_run_action();
        /* the cart's later acts leave for the menus, and a boss picked from
         * the menus goes on to the next */
        if (result != 0) {
            history_write();
            stop_ended();
            story_mark_cleared(stage_current);
            if (act_current >= 1 && stage_current == STAGE_CART) {
                stage_mode = STAGE_QUIT;
            } else if (game_mode == GAME_MODE_TRIAL) {
                if (stage_set_next_boss()) {
                    stage_mode = STAGE_LEAVE;
                } else {
                    stage_mode = STAGE_TRIAL_END;
                }
            } else {
                stage_set_next_exit();
                stage_mode = STAGE_LEAVE;
            }
            save_update_write_wait();
        }
        break;
    case STAGE_MISS_FADE:
        result = fade_out_step();
        stage_run_action();
        if (result != 0) {
            stop_ended();
            if (game_mode == GAME_MODE_TRIAL &&
                stage_current > STAGE_ACTION_LAST &&
                stage_current <= STAGE_BOSS_LAST) {
                stage_mode = STAGE_AGAIN;
            } else if (game_mode == GAME_MODE_TRIAL) {
                if (lives_count() == 0) {
                    stage_mode = STAGE_TRIAL_END;
                } else {
                    lives_add(-1);
                    stage_mode = STAGE_AGAIN;
                }
            } else {
                lives_add(-1);
                stage_mode = STAGE_AGAIN;
            }
            stage_retries++;
            ring_reset();
        }
        break;
    case STAGE_RETRY_FADE:
        result = fade_out_step();
        stage_run_action();
        /* start the act over as it is, without loading it */
        if (result != 0) {
            stop_ended();
            lives_add(-1);
            if (game_mode == GAME_MODE_TRIAL) {
                stage_mode = STAGE_TRIAL_END;
            } else {
                stage_mode = STAGE_FADE_IN;
            }
            stage_retries++;
            task_destroy_all();
            restart_clear();
            world_reset_gravity();
            time_reset();
            time_stop();
            if (character_current == CHARACTER_KNUCKLES) {
                knuckles_pick_pieces();
            } else if (character_current == CHARACTER_BIG) {
                fishing_reset();
            }
            heap_init();
            task_init();
            task_create(TASK_WORK, TASK_LEVEL_FIRST, placement_manager_step);
            stage_create_sky();
            stage_create_task();
            ring_reset();
            start_play();
            fade_in_start();
            while (fade_in_step() != 0) {
            }
        }
        break;
    case STAGE_GAME_OVER_FADE:
        result = game_over_step(continue_choice);
        stage_run_action();
        if (result != 0) {
            stop_ended();
            stage_mode = STAGE_GAME_OVER;
            ring_reset();
        }
        break;
    case STAGE_RESTART_FADE:
        result = fade_out_step();
        stage_run_action();
        if (result != 0) {
            stop_ended();
            if (stage_current != STAGE_CART) {
                lives_add(-1);
                stage_retries++;
            }
            ring_reset();
            if (stage_current == STAGE_CASINO) {
                casino_rings_deposited = 0;
            }
            stage_mode = STAGE_AGAIN;
        }
        break;
    case STAGE_GO_TO_FADE:
        result = fade_out_step();
        stage_run_action();
        if (result != 0) {
            action_go_next();
        }
        break;
    case STAGE_WAIT:
        if (fade_in_step() == 0) {
            stage_mode = STAGE_FADE_IN;
        }
        break;
    case STAGE_QUIT_FADE:
        result = fade_out_step();
        stage_run_action();
        if (result != 0) {
            stop_ended();
            if (demo_playing) {
                stage_mode = STAGE_QUIT;
            } else {
                stage_mode = STAGE_GAME_OVER;
            }
        }
        break;
    case STAGE_ENDING:
        stop_ended();
        stage_unload();
        if (player_record != NULL) {
            player_record->section = -1;
            event_flag_set(FLAG_CLEAR_LAST);
            save_now();
        }
        stage_mode = STAGE_TITLE_CARD;
        return STAGE_STEP_STAFF_ROLL;
    case STAGE_LEAVE:
        /* go on to the next stage, with its title card. a movie played
         * first skips the card. so does a way out of the past or a boss, or
         * into the base */
        stage_unload();
        stage_go_to_next();
        /* @hack spelling: the three tests after movie_request's, each with
         * an arm of its own that sets STAGE_ENTER, which keeps
         * stage_previous in R12 and stage_current's address in R11 (MOV
         * R0,R12 at 0x8c050554, MOV.W @R11,R0 at 0x8c050556); as one test of
         * ors, 70 of 1235 words differ */
        if (movie_request) {
            if (movie_play(movie_request) < 0) {
                reset_steps();
                return STAGE_STEP_RESET;
            }
            stage_mode = STAGE_ENTER;
        } else if (stage_previous == STAGE_PAST) {
            stage_mode = STAGE_ENTER;
        } else if (stage_current == STAGE_MYSTIC_RUINS &&
                   act_current == MYSTIC_RUINS_BASE) {
            stage_mode = STAGE_ENTER;
        } else if (stage_previous > STAGE_ACTION_LAST &&
                   stage_previous <= STAGE_BOSS_LAST) {
            stage_mode = STAGE_ENTER;
        } else {
            stage_mode = STAGE_TITLE_CARD;
        }
        return group();
    case STAGE_GAME_OVER:
        stage_unload();
        hud_reset();
        stage_mode = STAGE_CONTINUE;
        continue_choice = 0;
        continue_timer = CONTINUE_FRAMES;
        heap_init();
        task_init();
        history_write();
        vmu_request(VMU_REQUEST_GAME_OVER);
        stream_reset();
        stream_request_interlude(MUSIC_CONTINUE);
        /* fall through */
    case STAGE_CONTINUE:
        if (PAD_BUTTONS(pad_press_repeat[0], pad_choose_buttons)) {
            sound_kill_all();
            task_destroy_all();
            game_over_end();
            /* @hack redundant: the else after the return, which loads
             * stage_loaded's address into R2 and act_current's into R3
             * (0x8c050628, 0x8c05062a); without it, act_loaded's loads ahead
             * of act_current's and 20 of 1235 words differ */
            if (continue_choice) {
                stage_mode = STAGE_TITLE_CARD;
                return STAGE_STEP_QUIT;
            } else {
                stage_current = stage_loaded;
                act_current = act_loaded;
                stage_mode = STAGE_TITLE_CARD;
                return 0;
            }
        }
        if (continue_timer-- <= 0) {
            sound_kill_all();
            task_destroy_all();
            game_over_end();
            stage_mode = STAGE_TITLE_CARD;
            return STAGE_STEP_QUIT;
        }
        if (pad_press_repeat[0] &
            (PDD_DGT_TL | PDD_DGT_TR | PDD_DGT_KR | PDD_DGT_KL)) {
            /* @hack spelling: continue_choice = continue_choice ^ 1, which
             * copies its address to R1 and stores through it (0x8c0506cc to
             * 0x8c0506d0); as ^=, SHC stores through R2, and 58 of 1235 words
             * differ */
            continue_choice = continue_choice ^ 1;
        }
        game_over_step(continue_choice);
        pad_scan();
        frame_step = 0;
        step_tasks();
        break;
    case STAGE_QUIT:
        stage_unload();
        stage_mode = STAGE_TITLE_CARD;
        continue_choice = 0;
        return STAGE_STEP_QUIT;
    case STAGE_COMING_SOON:
        result = coming_soon_step();
        if (result != 0) {
            task_destroy_all();
            stage_go_to_next();
            stage_mode = STAGE_TITLE_CARD;
            return STAGE_STEP_QUIT;
        }
        break;
    case STAGE_TRIAL_END:
        stage_unload();
        stage_mode = STAGE_TITLE_CARD;
        return STAGE_STEP_QUIT;
    case STAGE_CARD_ERROR:
        stage_run_card_error();
        break;
    default:
        stage_mode = STAGE_LEAVE;
        break;
    }
    if (pad_quit_pressed(0)) {
        switch (stage_mode) {
        case STAGE_AGAIN:
        case STAGE_AGAIN_WAIT:
        case STAGE_FADE_IN:
        case STAGE_CLEAR_FADE:
        case STAGE_MISS_FADE:
        case STAGE_GO_TO_FADE:
        case STAGE_QUIT_FADE:
        case STAGE_PLAY:
        case STAGE_PAUSE:
            stage_stop_counted();
            stage_unload();
            break;
        case STAGE_LEAVE:
        case STAGE_GAME_OVER:
        case STAGE_QUIT:
            stage_unload();
            break;
        case STAGE_GAME_OVER_FADE:
            stage_stop_counted();
            stage_unload();
            break;
        case STAGE_CONTINUE:
            game_over_end();
            break;
        }
        reset_steps();
        return STAGE_STEP_RESET;
    }
    return 0;
}

/* leave a cleared stage, for its exit or the next boss */
static void action_leave_cleared(void)
{
    stop_ended();
    story_mark_cleared(stage_current);
    if (game_mode == GAME_MODE_TRIAL) {
        stage_set_next_boss();
    } else {
        stage_set_next_exit();
    }
    stage_mode = STAGE_LEAVE;
}

/* on to the next act in place, or leave for the next stage */
static void action_go_next(void)
{
    Sint16 delta;

    stream_destroy();
    if (stage_next == stage_current) {
        delta = act_next - act_current;
        world_change_act(delta);
        placement_change_act(delta);
        camera_change_act(delta);
        stage_change_act(delta);
        stage_place_player(player_work[0]);
        player_set_mode(0, PLAYER_MODE_RESET);
        forget_end();
        fade_in_start();
        while (fade_in_step() != 0) {
        }
        stage_mode = STAGE_FADE_IN;
    } else {
        stop_ended();
        stage_mode = STAGE_LEAVE;
    }
}

static void field_go_next(void)
{
    Sint16 delta;

    stream_destroy();
    if (stage_next == stage_current) {
        delta = act_next - act_current;
        world_change_act(delta);
        placement_change_act(delta);
        camera_change_act(delta);
        /* @hack spelling: stage_keep_previous's two stores written out,
         * which loads stage_change_act's address into R1 and
         * stage_place_player's into R3 (0x8c05097e, 0x8c050988); called, 10
         * of 76 words differ */
        stage_previous = stage_current;
        act_previous = act_current;
        stage_change_act(delta);
        stage_place_player(player_work[0]);
        player_set_mode(0, PLAYER_MODE_RESET);
        forget_end();
        fade_in_start();
        while (fade_in_step() != 0) {
        }
        stage_mode = STAGE_FADE_IN;
    } else {
        stop_play();
        forget_end();
        stage_mode = STAGE_LEAVE;
        history_write();
    }
}

Sint16 stage_step_field(void)
{
    Sint16 result;
    /* @hack spelling: the title card's results through status, which copies
     * title_card_load's to R4 and sign-extends it from there (0x8c050b12,
     * 0x8c050b14); tested in place, SHC skips the copy and 176 of 899 words
     * differ */
    Sint16 status;
    Sint16 delta;
    int found;
    NJS_COLOR color;
    Uint32 press;

    switch (stage_mode) {
    case STAGE_TITLE_CARD:
        /* no title card in the past, at the base, for Knuckles until
         * event 128 has played, or in a demo */
        /* @hack redundant: the breaks in the arms, which leave stage_mode++ in
         * R2 (0x8c050b1e to 0x8c050b24); without them, R3, and 5 of 899 words
         * differ */
        if (stage_current == STAGE_PAST ||
            (character_current == CHARACTER_KNUCKLES &&
             !event_was_played(128)) ||
            (stage_current == STAGE_MYSTIC_RUINS &&
             act_current == MYSTIC_RUINS_BASE) ||
            demo_playing) {
            stage_mode = STAGE_ENTER;
            break;
        } else {
            color.argb.b = color.argb.g = color.argb.r = color.argb.a = 0;
            fade_set_cover(&color);
            status = title_card_load(TITLE_CARD_FRAMES);
            if (status == -1) {
                stage_mode = STAGE_ENTER;
                break;
            } else {
                stage_mode++;
            }
        }
        break;
    case STAGE_TITLE_CARD_SHOW:
        status = title_card_draw(0);
        if (status == 1) {
            draw_off = FALSE;
            stage_load();
            story_start_act();
            start_play();
            fade_in_start();
            mission_card_create();
            while (fade_in_step() != 0) {
            }
            /* @hack redundant: stage_start_event's result tested, though both
             * arms set STAGE_FADE_IN, each storing 4 from a branch's slot
             * (0x8c050b9e to 0x8c050baa); called alone, the step's code ends
             * 64 bytes earlier and 788 of 899 words differ */
            if (stage_start_event()) {
                stage_mode = STAGE_FADE_IN;
            } else {
                stage_mode = STAGE_FADE_IN;
            }
        }
        break;
    case STAGE_ENTER:
        draw_off = FALSE;
        fade_in_start();
        stage_mode = STAGE_ENTER_WAIT;
        /* fall through */
    case STAGE_ENTER_WAIT:
        /* not while a save is still being written */
        if (!card_saving) {
            if (fade_in_step() == 0) {
                stage_load();
                story_start_act();
                start_play();
                while (fade_in_step() != 0) {
                }
                stage_start_event();
                stage_mode = STAGE_FADE_IN;
            }
        }
        break;
    case STAGE_AGAIN:
        fade_in_start();
        stage_mode = STAGE_AGAIN_WAIT;
        /* fall through */
    case STAGE_AGAIN_WAIT:
        if (fade_in_step() == 0) {
            story_start_act();
            start_play();
            stage_mode = STAGE_FADE_IN;
        }
        break;
    case STAGE_FADE_IN:
        result = fade_in_step();
        story_step();
        stage_run_field();
        if (result != 0) {
            stage_mode = STAGE_PLAY;
            if (!event_in_control()) {
                pad_enable();
            }
            time_start();
            pause_enabled = 1;
        }
        break;
    case STAGE_PLAY:
        story_step();
        if (story_ended) {
            stage_end(STAGE_END_CLEAR);
        }
        stage_run_field();
        if (stage_mode == STAGE_CARD_ERROR) {
            sound_play(SE_PAUSE, 0, 0, 0);
            pause_choice = PAUSE_CONTINUE;
            sound_pause();
            sound_mute();
            break;
        }
        if (end_kind() == STAGE_END_QUIT) {
            forget_end();
            stage_mode = STAGE_QUIT_FADE;
            fade_out_start();
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_CLEAR) {
            forget_end();
            if (!story_ended) {
                if (event_ending) {
                    stage_mode = STAGE_LEAVE;
                    stop_play();
                } else {
                    stage_mode = STAGE_CLEAR_FADE;
                    fade_out_start();
                }
            } else {
                draw_off = TRUE;
                stage_mode = STAGE_ENDING;
            }
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_MISS) {
            forget_end();
            if (lives_count() > 0) {
                fade_out_start();
                stage_mode = STAGE_MISS_FADE;
            } else {
                game_over_start();
                stage_mode = STAGE_GAME_OVER_FADE;
            }
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_GO_TO) {
            forget_end();
            stage_mode = STAGE_GO_TO_FADE;
            if (event_ending) {
                field_go_next();
            } else {
                /* @hack redundant: the mode set again, which the image stores
                 * in the delay slot of the call to fade_out_start (0x8c050d9e
                 * to 0x8c050da2); without it, 6 of 899 words differ */
                stage_mode = STAGE_GO_TO_FADE;
                fade_out_start();
            }
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if (end_kind() == STAGE_END_RESTART &&
                   stage_current == STAGE_ALIFE_RACE) {
            forget_end();
            stage_mode = STAGE_RESTART_FADE;
            fade_out_start();
            pad_disable();
            time_stop();
            pause_enabled = 0;
        } else if ((per_info[0]->press & PDD_DGT_ST) || demo_event_ended) {
            if (recording_kind != RECORDING_OFF) {
                if (demo_event >= 0 && demo_event_ended) {
                    stop_play();
                    forget_end();
                    stage_mode = STAGE_QUIT;
                } else {
                    stage_end(STAGE_END_QUIT);
                }
                if (!demo_event_ended) {
                    recording_kind = RECORDING_OFF;
                }
            } else if (pad_records[0].on & PDD_DGT_ST) {
                if (pause_enabled == 1 && !event_in_control()) {
                    sound_play(SE_PAUSE, 0, 0, 0);
                    pause_choice = PAUSE_CONTINUE;
                    stage_mode = STAGE_PAUSE;
                    sound_pause();
                    sound_mute();
                }
            }
        } else {
            /* @hack spelling: the first test's press through a local, which
             * keeps the mask in R7 and the press in R6 (0x8c050eba to
             * 0x8c050ec0); read from the pad, R6 and R7 trade and 6 of 899
             * words differ */
            press = per_info[0]->press;
            if (((press & PDD_DGT_TL) && (per_info[0]->on & PDD_DGT_TR)) ||
                ((per_info[0]->on & PDD_DGT_TL) && (press & PDD_DGT_TR))) {
                debug_name_frame();
            } else if ((per_info[0]->press & HUD_BUTTONS) &&
                       (per_info[0]->on & HUD_BUTTONS) == HUD_BUTTONS) {
                hud_hidden = ~hud_hidden;
            }
        }
        break;
    case STAGE_PAUSE:
        stage_run_paused();
        if (per_info[0]->press & PDD_DGT_ST) {
            pause_sound();
            /* @hack type: pause_choice read as a Uint16, as in
             * stage_step_action (0x8c050f82, 0x8c050f84); as a Sint16, 6 of
             * 899 words differ */
            if (*(Uint16 *)&pause_choice == PAUSE_CONTINUE) {
                stage_mode = STAGE_PLAY;
            } else if (pause_choice == PAUSE_RESTART) {
                pause_restart();
            } else {
                history_write();
                stop_play();
                stage_mode = STAGE_QUIT;
                if (stage_current >= STAGE_ALIFE_SS &&
                    stage_current <= STAGE_ALIFE_RACE) {
                    alife_save_wait();
                }
            }
            if (recording_kind == RECORDING_OFF) {
                recording_on = FALSE;
            } else {
                lives_add(1);
                stage_end(STAGE_END_MISS);
            }
            if (pause_choice != PAUSE_CONTINUE) {
                sound_end_act();
            } else {
                sound_resume(15);
            }
            sound_unmute();
        }
        break;
    case STAGE_CLEAR_FADE:
        result = fade_out_step();
        story_step();
        stage_run_field();
        if (result != 0) {
            history_write();
            stop_play();
            forget_end();
            stage_mode = STAGE_LEAVE;
        }
        break;
    case STAGE_MISS_FADE:
        result = fade_out_step();
        stage_run_field();
        if (result != 0) {
            stop_play();
            forget_end();
            lives_add(-1);
            stage_mode = STAGE_AGAIN;
            ring_reset();
        }
        break;
    case STAGE_GAME_OVER_FADE:
        result = game_over_step(continue_choice);
        stage_run_field();
        if (result != 0) {
            stop_play();
            forget_end();
            stage_mode = STAGE_GAME_OVER;
            ring_reset();
        }
        break;
    case STAGE_GO_TO_FADE:
        result = fade_out_step();
        story_step();
        stage_run_field();
        if (result != 0) {
            field_go_next();
        }
        break;
    case STAGE_QUIT_FADE:
        result = fade_out_step();
        stage_run_field();
        if (result != 0) {
            stop_play();
            forget_end();
            stage_mode = STAGE_QUIT;
        }
        break;
    case STAGE_ENDING:
        stop_play();
        forget_end();
        stage_unload();
        stage_mode = STAGE_TITLE_CARD;
        color.color = 0;
        cover(&color);
        return STAGE_STEP_STAFF_ROLL;
    case STAGE_RESTART_FADE:
        result = fade_out_step();
        stage_run_field();
        /* the race starts over from the act it loaded with */
        if (result != 0) {
            stop_play();
            forget_end();
            delta = act_loaded - act_current;
            placement_change_act(delta);
            stage_change_act(delta);
            stage_mode = STAGE_AGAIN;
        }
        break;
    case STAGE_LEAVE:
        stage_unload();
        stage_go_to_next();
        /* @hack spelling: the two tests after movie_request's, each with an
         * arm of its own that sets STAGE_ENTER, which keeps STAGE_TITLE_CARD
         * in R11 and fade_in_step's address in R10 (0x8c050a54, 0x8c050a60);
         * as one test of ors, 27 of 899 words differ */
        if (movie_request) {
            if (movie_play(movie_request) < 0) {
                reset_steps();
                return STAGE_STEP_RESET;
            }
            stage_mode = STAGE_ENTER;
        } else if (stage_previous == STAGE_PAST) {
            stage_mode = STAGE_ENTER;
        } else if (stage_current == STAGE_MYSTIC_RUINS &&
                   act_current == MYSTIC_RUINS_BASE) {
            stage_mode = STAGE_ENTER;
        } else {
            stage_mode = STAGE_TITLE_CARD;
        }
        return group();
    case STAGE_GAME_OVER:
        stage_unload();
        stage_mode = STAGE_CONTINUE;
        continue_choice = 0;
        continue_timer = CONTINUE_FRAMES;
        heap_init();
        task_init();
        history_write();
        vmu_request(VMU_REQUEST_GAME_OVER);
        stream_reset();
        stream_request_interlude(MUSIC_CONTINUE);
        /* fall through */
    case STAGE_CONTINUE:
        if (PAD_BUTTONS(pad_press_repeat[0], pad_choose_buttons)) {
            sound_kill_all();
            task_destroy_all();
            game_over_end();
            /* going on starts the story again from the save */
            /* @hack redundant: the else after the return, which loads
             * hud_reset's address into R2, progress_init's into R3 and
             * save_file_exists' into R2 (0x8c0512c0 to 0x8c0512cc); without it,
             * R2 and R3 trade and 10 of 899 words differ */
            if (continue_choice) {
                stage_mode = STAGE_TITLE_CARD;
                return STAGE_STEP_QUIT;
            } else {
                hud_reset();
                progress_init();
                /* @hack spelling: the result through found, which keeps
                 * continue_choice's address in R8 and pause_enabled's in R9
                 * for the step (MOV.W R3,@R9 at 0x8c050c54); tested in place,
                 * R8 and R9 trade and 48 of 899 words differ */
                found = save_file_exists();
                if (found & 1) {
                    save_read();
                }
                save_load();
                if (lives_count() < LIVES_START) {
                    lives_reset();
                }
                story_enable();
                story_set_character(character_current);
                story_set_next_stage(player_record->section);
                stage_mode = STAGE_TITLE_CARD;
                return 0;
            }
        }
        if (continue_timer-- <= 0) {
            sound_kill_all();
            task_destroy_all();
            game_over_end();
            stage_mode = STAGE_TITLE_CARD;
            return STAGE_STEP_QUIT;
        }
        if (pad_press_repeat[0] &
            (PDD_DGT_TL | PDD_DGT_TR | PDD_DGT_KR | PDD_DGT_KL)) {
            continue_choice ^= 1;
        }
        game_over_step(continue_choice);
        pad_scan();
        frame_step = 0;
        step_tasks();
        break;
    case STAGE_QUIT:
        stage_unload();
        stage_mode = STAGE_TITLE_CARD;
        continue_choice = 0;
        return STAGE_STEP_QUIT;
    case STAGE_CARD_ERROR:
        stage_run_card_error();
        break;
    default:
        stage_mode = STAGE_LEAVE;
        break;
    }
    if (pad_quit_pressed(0)) {
        switch (stage_mode) {
        case STAGE_AGAIN:
        case STAGE_AGAIN_WAIT:
        case STAGE_FADE_IN:
        case STAGE_CLEAR_FADE:
        case STAGE_MISS_FADE:
        case STAGE_GO_TO_FADE:
        case STAGE_QUIT_FADE:
        case STAGE_PLAY:
        case STAGE_PAUSE:
            stop_play();
            /* fall through */
        case STAGE_LEAVE:
        case STAGE_GAME_OVER:
        case STAGE_QUIT:
            stage_unload();
            break;
        case STAGE_GAME_OVER_FADE:
            stop_play();
            stage_unload();
            break;
        case STAGE_CONTINUE:
            game_over_end();
            break;
        }
        reset_steps();
        return STAGE_STEP_RESET;
    }
    return 0;
}

void stage_end(Sint16 end)
{
    if (end == STAGE_END_CLEAR && stage_clear_event()) {
        return;
    }
    stage_end_kind = end;
}

static void stage_reset_counts(void)
{
    stage_retries = 0;
    restart_clear();
    world_reset_gravity();
    if (stage_current <= STAGE_ACTION_LAST ||
        stage_current == STAGE_SUNABOARD) {
        ring_reset();
    }
}

static void stage_reset_play(void)
{
    pause_enabled = 0;
    debug_hidden = -1;
    /* a recording and its playback start from the same random numbers */
    if (recording_kind == RECORDING_OFF) {
        recording_on = FALSE;
    } else {
        recording_on = TRUE;
        srand(0);
        if (recording_kind == RECORDING_RECORD) {
            recording_mode = RECORDER_RECORD;
        } else {
            recording_mode = RECORDER_PLAY;
        }
    }
    recording_next = DEMO_RECORDING;
    recording_buttons = 0;
    dat_8c751454 = (void *)0x0cbc0000;
    pad_enable_player(3); /* 3, like any number but 0 and 1, means both */
    ring_reset_stagger();
    score_reset();
}

static void stage_load(void)
{
    Uint16 stage;

    stage_loaded = stage_current;
    act_loaded = act_current;
    njSetFogColor(0);
    njGenerateFogTable3(fog_table, -7000.0F, -8000.0F);
    njSetFogTable(fog_table);
    sound_load_stage();
    loader_read_stage_files();
    world_compute_bounds();
    /* @hack spelling: the stage taken in two statements, which gives the
     * function a frame word (ADD #-4,R15 at 0x8c0515e8); in one, it has none
     * and 64 of 142 words differ */
    stage = stage_get_stage_act();
    stage = STAGE_OF(stage);
    /* the shooting stages have their own textures, and no character */
    switch (stage) {
    case STAGE_SHOOTING1:
    case STAGE_SHOOTING2:
        texture_release_regular();
        texture_load_regular(2);
        break;
    default:
        texture_release_regular();
        texture_release_characters();
        texture_release_all();
        fade_from_black();
        load_textures(character_current);
        /* @hack redundant: a break where the switch ends, which loads
         * texture_load_stage's address into R2 (0x8c0516e6); without it, R3,
         * and 2 of 142 words differ */
        break;
    }
    texture_load_stage();
    texture_load_act(STAGE_ACT(stage_current, act_current));
    world_load_act(stage_current, act_current);
    njGarbageTexture(texmemlist, TEXTURE_COUNT);
    stage_reset_counts();
    pause_enabled = 0;
    time_reset();
    time_stop();
    if (character_current == CHARACTER_KNUCKLES) {
        knuckles_pick_pieces();
    } else if (character_current == CHARACTER_BIG) {
        fishing_reset();
    }
    heap_init();
    task_init();
    task_create(TASK_WORK, TASK_LEVEL_FIRST, placement_manager_step);
    history_restart();
}

static void stage_start(void)
{
    struct task *task;

    frame_pacing_init(2, 2);
    stage_reset_play();
    thread_init();
    camera_create();
    task_create(TASK_WORK, 0, light_stage_step);
    stage_create_sky();
    stage_create_task();
    task_create(TASK_WORK, 1, world_land_step);
    stage_create_players();
    task_create(TASK_WORK, 1, death_zone_step);
    object_init();
    animal_icons_create();
    talk_create_manager();
    program_select_init();
    collision_mesh_init();
    path_create_manager();
    collision_clear_lists();
    particle_init(stage_particles, PARTICLE_COUNT);
    particle_create_pools();
    download_start();
    event_ending = 0;
    if (demo_playing) {
        texture_load_pvm("PRESSSTART", &press_start_texlist);
        task = task_create(TASK_WORK, 1, press_start_step);
        task->destructor = press_start_destroy;
    }
}

/* a step of play: the pads, the tasks, the collisions and the particles.
 * after them, hud_hidden becomes -1 while an event is in control, and 0
 * otherwise. it returns 0 */
static int stage_run_step(void)
{
    int i;

    pad_scan();
    thread_yield();
    task_step_all();
    timer_mark();
    collision_collide_all();
    if (debug_page == 1) {
        njPrintC(NJM_LOCATION(1, 24), "C_Colli");
        njPrintD(NJM_LOCATION((frame_step * 5) + 10, 24), (int)timer_lap(), 4);
    }
    particle_run();
    timer_mark();
    collision_mesh_update();
    if (debug_page == 1) {
        njPrintC(NJM_LOCATION(1, 25), "CRangeIn");
        njPrintD(NJM_LOCATION((frame_step * 5) + 10, 25), (int)timer_lap(), 4);
    }
    if (debug_page == 2 && frame_step == 0) {
        for (i = 0; i < 20; i++) {
            njPrintD(NJM_LOCATION(3, i + 6), i * 16, 3);
            njPrintD(NJM_LOCATION(7, i + 6), draw_debug_counts[i], 4);
        }
    }
    if (!event_in_control()) {
        if (stage_group() != STAGE_GROUP_FIELD) {
            time_step();
        }
        hud_hidden = 0;
    } else {
        hud_hidden = -1;
    }
    if (game_mode == GAME_MODE_TRIAL && stage_current <= STAGE_BOSS_LAST &&
        stage_current > STAGE_ACTION_LAST) {
        lives_shown = -1;
    }
    return 0;
}

/* a frame of play: frame_step_count steps, and one more when the second
 * ends ACTION_LIMIT or more after the frame began. per_info's presses then
 * hold every step's, and the hud and its timer draw. it returns 0 */
static int stage_run_action(void)
{
    int extra;
    Float limit;
    Uint32 press0;
    Uint32 press1;

    press0 = per_info[0]->press;
    press1 = per_info[1]->press;
    extra = 0;
#ifdef GCC
    /* each frame starts with its limit uncleared, as the image loads
     * 1900 into FR15 for each frame (0x8c051b1a) */
    stage_limit_cleared = FALSE;
#endif
    /* @hack spelling: 1900 through limit, which SHC keeps in FR15 for the
     * loop (MOVA at 0x8c051b0a); compared as a constant, 80 of 87 words
     * differ */
    /* @hack spelling: limit set in the loop's start, which loads 1900 after the
     * pads' records (0x8c051b0a); set before the loop, SHC loads it earlier,
     * from another place in the pool, and 11 of 87 words differ */
    for (frame_step = 0, limit = ACTION_LIMIT;
         frame_step < frame_step_count + extra;) {
        stage_run_step();
#ifdef GCC
        present_step();
        /* keeps the shipped extra step a cone moving over a floor gives a
         * frame at 30 fps. in the image, cone_run's slope z lands on the FR15
         * that holds limit. a floor's z is an angle within half a turn: as a
         * Float, one of 0 or more is 0 or a denormal, which the FPU takes
         * for 0 under FPSCR.DN, and a negative one is a NaN. the FCMP/GT at
         * 0x8c051b2c finds neither over the time, so the frame takes its
         * extra step, as GCC's does with a limit of 0.0F */
        if (stage_limit_cleared) {
            limit = 0.0F;
        }
#endif
        if (frame_step == 1 && timer_elapsed() >= limit) {
            extra++;
        }
        frame_step++;
        if (frame_step < frame_step_count + extra) {
            frame_wait_step(frame_step_count - frame_step + extra);
            press0 |= per_info[0]->press;
            press1 |= per_info[1]->press;
        }
    }
    per_info[0]->press = press0;
    per_info[1]->press = press1;
#ifdef GCC
    /* a split frame's second picture, which the hud draws over */
    present_second();
#endif
    hud_draw();
    hud_draw_timer();
    return 0;
}

/* a frame of a field, as stage_run_action runs one, but with FIELD_LIMIT
 * for the extra step, and with no timer drawn. it returns 0 */
static int stage_run_field(void)
{
    int extra;
    Float limit;
    Uint32 press0;
    Uint32 press1;

    press0 = per_info[0]->press;
    press1 = per_info[1]->press;
    extra = 0;
    /* @hack spelling: 1850 through limit, which SHC keeps in FR15 for the
     * loop (MOVA at 0x8c051bb8); compared as a constant, 77 of 84 words
     * differ */
    /* @hack spelling: limit set in the loop's start, which loads 1850 after the
     * pads' records (0x8c051bb8); set before the loop, SHC loads it earlier,
     * and 10 of 84 words differ */
    for (frame_step = 0, limit = FIELD_LIMIT;
         frame_step < frame_step_count + extra;) {
        stage_run_step();
#ifdef GCC
        present_step();
#endif
        if (frame_step == 1 && timer_elapsed() >= limit) {
            extra++;
        }
        frame_step++;
        if (frame_step < frame_step_count + extra) {
            frame_wait_step(frame_step_count - frame_step + extra);
            press0 |= per_info[0]->press;
            press1 |= per_info[1]->press;
        }
    }
    per_info[0]->press = press0;
    per_info[1]->press = press1;
#ifdef GCC
    /* a split frame's second picture, which the hud draws over */
    present_second();
#endif
    hud_draw();
    return 0;
}

/* a frame of the pause menu: the tasks draw without stepping, and up and
 * down move the choice. x and y held together hide the menu. the fields,
 * the gardens, and a player on a last life get no restart. it returns 0 */
static int stage_run_paused(void)
{
    pad_scan();
    frame_step = 0;
    task_draw_all();
    hud_draw();
    if ((per_info[0]->on & (PDD_DGT_TX | PDD_DGT_TY)) ==
        (PDD_DGT_TX | PDD_DGT_TY)) {
        pause_hidden = -1;
    } else {
        pause_hidden = 0;
    }
    if ((stage_current >= STAGE_STATION_SQUARE &&
         stage_current <= STAGE_PAST) ||
        (stage_current >= STAGE_ALIFE_SS && stage_current <= STAGE_ALIFE_MR) ||
        lives_count() <= 0) {
        if (pad_press_repeat[0] & (PDD_DGT_KU | PDD_DGT_KD)) {
            if (pause_choice != PAUSE_CONTINUE) {
                pause_choice = PAUSE_CONTINUE;
            } else {
                pause_choice = PAUSE_QUIT;
            }
        }
        if (pause_hidden >= 0) {
            pause_draw_no_restart(pause_choice != PAUSE_CONTINUE);
        }
    } else {
        if (pad_press_repeat[0] & PDD_DGT_KU) {
            pause_choice--;
        } else if (pad_press_repeat[0] & PDD_DGT_KD) {
            pause_choice++;
        }
        if (pause_choice >= PAUSE_CHOICES) {
            pause_choice = PAUSE_CONTINUE;
        } else if (pause_choice < PAUSE_CONTINUE) {
            pause_choice = PAUSE_QUIT;
        }
        if (pause_hidden >= 0) {
            pause_draw(pause_choice);
        }
    }
    hud_draw_timer();
    particle_draw_all();
    return 0;
}

/* a frame of the memory card's error over the paused stage. it goes back
 * to the pause menu once the card's saves match the game's. with the game
 * going on without the card, it goes back once a pad is in. until then the
 * screen names the error. it returns 0 */
static int stage_run_card_error(void)
{
    struct save *save;

    pad_scan();
    frame_step = 0;
    task_draw_all();
    hud_draw();
    pause_draw_no_choices();
    /* a frame that doesn't check shows the error for no pad */
    if (!(play_time & CARD_CHECK_MASK)) {
        /* @hack spelling: save_disabled compared with FALSE, which loads its
         * address into R4 (0x8c051e12); tested bare, R0, and 42 of 97 words
         * differ */
        if (save_disabled != FALSE && pad_ports[0] != -1) {
            stage_mode = STAGE_PAUSE;
        } else if (!save_disabled) {
            save = save_peek();
            if (save != NULL && save[0].checksum == saves[0].checksum &&
                save[1].checksum == saves[1].checksum &&
                save[2].checksum == saves[2].checksum) {
                stage_mode = STAGE_PAUSE;
            } else {
                text_print(NJM_LOCATION(14, 15), TEXT_1);
                text_print(NJM_LOCATION(17, 17), TEXT_2);
            }
        } else {
            text_print(NJM_LOCATION(14, 15), TEXT_3);
            text_print(NJM_LOCATION(17, 17), TEXT_4);
        }
    } else {
        text_print(NJM_LOCATION(14, 15), TEXT_5);
        text_print(NJM_LOCATION(17, 17), TEXT_6);
    }
    hud_draw_timer();
    particle_draw_all();
    return 0;
}

static void stage_stop(void)
{
    sound_end_act();
    sound_kill_all();
    thread_destroy_all();
    task_destroy_levels_0_to_6();
    particle_free_all();
}

/* stop the act as stage_stop does. in STAGE_CLEAR_FADE or STAGE_ENDING, it
 * then counts the character's clear of the stage, unless the stage is a
 * field or a Chao stage. a counted clear sets stage_last_cleared. it also
 * sets these clears:
 * - a twinkle park clear gives the character a cart clear if it has none;
 * - while Sonic, Amy and Big have each cleared twinkle park, every counted
 *   clear sets Knuckles', Tails' and E-102's cart clears to 1;
 * - a sunaboard clear gives Sonic a sunaboard clear while he has none and
 *   Tails has exactly one */
static void stage_stop_counted(void)
{
    Uint8 count;

    stage_stop();
    if ((stage_mode != STAGE_CLEAR_FADE && stage_mode != STAGE_ENDING) ||
        (stage_current > STAGE_BOSS_LAST && stage_current != STAGE_SHOOTING1 &&
         stage_current != STAGE_SHOOTING2 && stage_current != STAGE_SUNABOARD &&
         stage_current != STAGE_CART)) {
        return;
    }
    count = stage_clears[character_current][stage_current];
    /* @bug logic: count is a byte, so the test against 256 always
     * holds: CMP/GE at 0x8c051fd6 never sets T, and the store at 0x8c051fde
     * writes a 256th clear as 0 */
    /* NOLINTNEXTLINE(clang-diagnostic-tautological-*) */
    if (count < 256) {
        count++;
    }
    stage_clears[character_current][stage_current] = count;
    if (stage_current == STAGE_TWINKLE) {
        count = stage_clears[character_current][STAGE_CART];
        if (count == 0) {
            count = 1;
        }
        stage_clears[character_current][STAGE_CART] = count;
    }
    if (stage_clears[CHARACTER_SONIC][STAGE_TWINKLE] != 0 &&
        stage_clears[CHARACTER_AMY][STAGE_TWINKLE] != 0 &&
        stage_clears[CHARACTER_BIG][STAGE_TWINKLE] != 0) {
        stage_clears[CHARACTER_KNUCKLES][STAGE_CART] =
            stage_clears[CHARACTER_TAILS][STAGE_CART] =
                stage_clears[CHARACTER_E102][STAGE_CART] = 1;
    }
    if (stage_current == STAGE_SUNABOARD) {
        if (stage_clears[CHARACTER_SONIC][STAGE_SUNABOARD] == 0 &&
            stage_clears[CHARACTER_TAILS][STAGE_SUNABOARD] == 1) {
            stage_clears[CHARACTER_SONIC][STAGE_SUNABOARD] = 1;
        }
    }
    stage_last_cleared = stage_current;
}

static void stage_unload(void)
{
    Uint16 stage;

    task_destroy_all();
    texture_release_stage();
    texture_release_act(STAGE_ACT(stage_current, act_current));
    world_release_act(stage_current, act_current);
    download_end();
    /* @hack spelling: the stage taken in two statements, which gives the
     * function a frame word (ADD #-4,R15 at 0x8c0520a6); in one, it has none
     * and 52 of 58 words differ */
    stage = stage_get_stage_act();
    stage = STAGE_OF(stage);
    switch (stage) {
    case STAGE_SHOOTING1:
    case STAGE_SHOOTING2:
        texture_release_regular();
        texture_load_regular(0);
        break;
    }
}

static void later_step(struct task *task)
{
    struct later_extra *later = task->extra;

    later->steps--;
    if (later->steps > 0) {
        return;
    }
    switch (later->kind) {
    case LATER_MUSIC:
        stream_request_music(later->number);
        break;
    case LATER_SOUND:
        sound_play(later->number, 0, 0, 0);
        break;
    }
    task_destroy(task);
}

void sound_play_later(int se)
{
    struct task *task;

    if ((task = task_create(TASK_EXTRA, 1, later_step)) != NULL) {
        LATER(task)->steps = LATER_STEPS;
        LATER(task)->number = se;
        LATER(task)->kind = LATER_SOUND;
    }
}

void music_play_later(int music)
{
    struct task *task;

    if ((task = task_create(TASK_EXTRA, 1, later_step)) != NULL) {
        LATER(task)->steps = LATER_STEPS;
        LATER(task)->number = music;
        LATER(task)->kind = LATER_MUSIC;
    }
}

void stage_clear(void)
{
    pad_disable_player(0);
    pause_enabled = 0;
    time_stop();
    switch (character_current) {
    case CHARACTER_TAILS:
        if (race_outcome() == RACE_WON) {
            task_create(0, 3, clear_wait_step);
        } else {
            player_set_mode(0, PLAYER_MODE_CLEAR);
            tails_clear();
            overlay_stage_clear();
            task_create(TASK_WORK, 1, score_step);
        }
        break;
    case CHARACTER_KNUCKLES:
        player_set_mode(0, PLAYER_MODE_CLEAR);
        camera_start_path(&knuckles_clear_path, CAMERA_PATH_HOLD,
                          CLEAR_PATH_STEPS);
        overlay_stage_clear();
        task_create(TASK_WORK, 1, score_step);
        /* below STAGE_CHAOS0, the stage is a level */
        /* @hack spelling: music_play_later in each arm, which gives each arm
         * its own call of sound_play_later (BSR at 0x8c052310 and
         * 0x8c05231e); with one call after the if, the first arm branches
         * into the second's and 11 of 236 words differ */
        if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
            stream_destroy();
            sound_play_later(SE_KNUCKLES_CLEAR);
            music_play_later(MUSIC_RNDCLEAR);
        } else {
            stream_destroy();
            sound_play_later(SE_KNUCKLES_CLEAR_BOSS);
            music_play_later(MUSIC_RNDCLEAR);
        }
        break;
    case CHARACTER_AMY:
        if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
            player_set_mode(0, PLAYER_MODE_CLEAR);
            overlay_stage_clear();
            task_create(TASK_WORK, 1, score_step);
            stream_destroy();
            sound_play_later(SE_AMY_CLEAR);
            music_play_later(MUSIC_RNDCLEAR);
        } else {
            task_create(0, 3, clear_wait_step);
        }
        break;
    case CHARACTER_BIG:
        player_set_mode(0, PLAYER_MODE_CLEAR);
        /* a fixed camera on Big. it looks at the center of his first
         * collision shape. it stands at that center less pos, once pos is
         * turned the way he faces */
        {
            NJS_POINT3 pos = {-36.0729F, 5.7132F, -1.5176F};
            NJS_POINT3 offset;

            player_turn_to_world(player_work[0], &pos);
            offset = player_work[0]->collision->shapes->center;
            /* @hack spelling: the view copied a float at a time, for the
             * image's FMOV.S copies (0x8c052408 to 0x8c052440), which keeps
             * overlay_stage_clear's address in R8 (0x8c05223e); as struct
             * copies, SHC copies in MOV.L words and 173 of 236 words differ */
            camera_override_view.target.x = offset.x;
            camera_override_view.target.y = offset.y;
            camera_override_view.target.z = offset.z;
            njSubVector(&offset, &pos);
            camera_override_view.pos.x = offset.x;
            camera_override_view.pos.y = offset.y;
            camera_override_view.pos.z = offset.z;
            camera_override(camera_step_fixed, 1, CAMERA_LOOK_AT);
            task_create(TASK_WORK, 1, score_step);
            if (stage_get_stage_act() < STAGE_ACT(STAGE_CHAOS0, 0)) {
                stream_destroy();
            }
            music_play_later(MUSIC_RNDCLEAR);
        }
        break;
    default:
        task_create(0, 3, clear_wait_step);
        stream_destroy();
        break;
    }
}

/* a task's step: the rest of stage_clear, once the player is on the
 * ground. the player is player 1 while it is on the capsule's button */
static void clear_wait_step(struct task *task)
{
    struct task_work *work;

    work = player_get_task(0)->work;
    if (player_work[1] != NULL && capsule_get() != NULL &&
        capsule_get_player() == 1) {
        work = player_work[1];
    }
    if (!(work->flags & PLAYER_ON_GROUND)) {
        return;
    }
    player_set_mode(0, PLAYER_MODE_CLEAR);
    if (character_current == CHARACTER_SONIC) {
        camera_start_path(&sonic_clear_path, CAMERA_PATH_HOLD,
                          CLEAR_PATH_STEPS);
        if (stage_current <= STAGE_ACTION_LAST) {
            sound_play_later(SE_SONIC_CLEAR);
        } else {
            sound_play_later(SE_SONIC_CLEAR_BOSS);
        }
    } else if (character_current == CHARACTER_TAILS) {
        tails_clear();
    } else if (character_current == CHARACTER_E102) {
        camera_start_path(&e102_clear_path, CAMERA_PATH_HOLD, CLEAR_PATH_STEPS);
    } else if (character_current == CHARACTER_AMY) {
        camera_start_path(&amy_clear_path, CAMERA_PATH_HOLD, CLEAR_PATH_STEPS);
        sound_play_later(SE_AMY_CLEAR_BOSS);
    }
    overlay_stage_clear();
    task_create(TASK_WORK, 1, score_step);
    music_play_later(MUSIC_RNDCLEAR);
    task_kill(task);
}

/* make player 0's task for character_current, and place it with
 * stage_place_player. for player 1, it calls npc_create while Sonic plays,
 * which starts Tails following him. it then calls race_create, which starts
 * a race where the act has one. character_current must be one of the six
 * playable */
static void stage_create_players(void)
{
    struct task *task;
    void (*step)(struct task *task);
    NJS_POINT3 pos; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    Sint16 i;

    player_clear_slots();
#ifdef GCC
    /* GCC can't see that character_current is one of the six playable, as
     * the comment above says it must be: the test for each sets task */
    task = NULL;
#endif
    for (i = 0; i < 2; i++) {
        if (i == 0) {
            if (character_current == CHARACTER_SONIC) {
                if (stage_current == STAGE_SHOOTING1 ||
                    stage_current == STAGE_SHOOTING2) {
                    step = shooting_player_step;
                } else {
                    step = sonic_step;
                }
                task =
                    task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, step);
                PLAYER_VARS(task->work)->character = character_current;
            }
            if (character_current == CHARACTER_KNUCKLES) {
                task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                                   knuckles_step);
                PLAYER_VARS(task->work)->character = character_current;
            }
            if (character_current == CHARACTER_TAILS) {
                if (stage_current == STAGE_SHOOTING1 ||
                    stage_current == STAGE_SHOOTING2) {
                    step = shooting_player_step;
                } else {
                    step = tails_step;
                }
                task =
                    task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1, step);
                PLAYER_VARS(task->work)->character = character_current;
            }
            if (character_current == CHARACTER_BIG) {
                task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                                   big_step);
                PLAYER_VARS(task->work)->character = character_current;
            }
            if (character_current == CHARACTER_AMY) {
                task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                                   amy_step);
                PLAYER_VARS(task->work)->character = character_current;
            }
            if (character_current == CHARACTER_E102) {
                task = task_create(TASK_MOTION | TASK_WORK | TASK_FORCE, 1,
                                   e102_step);
                PLAYER_VARS(task->work)->character = character_current;
            }
            PLAYER_VARS(task->work)->player = i;
            player_work[i] = task->work;
            player_motion[i] = task->motion;
            stage_place_player(task->work);
            /* @hack redundant: the start copied to pos and never read, which
             * the image copies into the frame at @R15 (0x8c052736 to
             * 0x8c052746); without it, the function runs 18 bytes short and
             * 54 of 162 words differ */
            pos = task->work->pos;
        } else {
            if (character_current == CHARACTER_SONIC && i == 1) {
                npc_create(task);
            }
            race_create(character_current, i, stage_current);
        }
    }
}

static void stage_create_task(void)
{
    if (stage_steps[stage_current] != NULL) {
        stage_task = task_create(TASK_WORK, 0, stage_steps[stage_current]);
    } else {
        stage_task = NULL;
    }
    /* @hack redundant: a return where the function ends, which gives the if's
     * arm an RTS of its own (0x8c0527a4 to 0x8c0527aa); without it, the arm
     * branches to the else's, and 4 of 26 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void stage_create_sky(void)
{
    struct back_color *color = &back_colors[stage_current];

    njSetBackColor(color->col1, color->col2, color->col3);
    if (sky_steps[stage_current] != NULL) {
        task_create(TASK_WORK, 2, sky_steps[stage_current]);
    }
}

void set_character(Sint16 character)
{
    int index;

    texture_release_character(character_current);
    /* the save keeps each character's lives in a list that skips Tikal and
     * Eggman */
    index = character_current;
    if (index > CHARACTER_TIKAL) {
        index--;
    }
    if (index > CHARACTER_EGGMAN) {
        index--;
    }
    saves[save_slot].lives[index] = lives;
    character_current = character;
    loader_read_character_motions(character_current);
    texture_load_character(character_current);
    index = character_current;
    if (index > CHARACTER_TIKAL) {
        index--;
    }
    if (index > CHARACTER_EGGMAN) {
        index--;
    }
    lives = saves[save_slot].lives[index];
    if (lives_count() < LIVES_START) {
        lives_reset();
    }
    chao_reset();
}

void set_character_number(Sint16 character)
{
    character_current = character;
}

int get_character(void)
{
    return character_current;
}

int stage_get_retries(void)
{
    return stage_retries;
}

static void stage_keep_previous(void)
{
    stage_previous = stage_current;
    act_previous = act_current;
    /* @hack redundant: a return where the function ends. it has
     * stage_go_to_next load stage_next's address before the expansion stores
     * act_previous (0x8c052a68, 0x8c052a6a), and the three getters below keep
     * the stage's address in R2 (0x8c052ac0 in stage_get_stage_act); without
     * it, the load comes after the store, R2 and R3 trade in the getters, and 7
     * of the 37 words of the function and its two callers differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* lead back to the previous stage and act, at the character's return point
 * for this stage, or with return_point NULL where it has none. from twinkle
 * park, a return point leads to STATION_SQUARE_TWINKLE_ACT in place of the
 * previous act */
static void stage_set_next_return(void)
{
    struct return_point *list;

    stage_next = stage_previous;
    act_next = act_previous;
    list = returns[character_current];
    return_point = NULL;
    for (; list->stage != STAGE_NONE; list++) {
        if (list->stage == stage_current) {
            return_point = list;
            if (stage_current == STAGE_TWINKLE) {
                act_next = STATION_SQUARE_TWINKLE_ACT;
            }
            return;
        }
    }
}

void stage_set_stage_act(Uint8 stage, Uint8 act)
{
    stage_keep_previous();
    stage_current = stage;
    act_current = act;
}

void stage_set_next_stage_act(Uint8 stage, Uint8 act)
{
    stage_next = stage;
    act_next = act;
}

void stage_fade_to(Uint8 stage, Uint8 act)
{
    stage_end(STAGE_END_GO_TO);
    stage_next = stage;
    act_next = act;
    /* @hack redundant: a return where the function ends, after which
     * stage_step_action loads download_reset's address into R2, as its next
     * arm loads stage_stop_counted's (0x8c0500d0, 0x8c0500dc); without it,
     * into R3, and 25 of the 1255 words of the function and
     * stage_step_action differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void stage_restart(void)
{
    stage_end(STAGE_END_RESTART);
}

void stage_go_to_next(void)
{
    stage_keep_previous();
    stage_current = stage_next;
    act_current = act_next;
    /* @hack redundant: a return where the function ends, which has each step
     * load movie_request's address into R3 after the expansion sets
     * act_current (0x8c0511cc in stage_step_field); without it, the load
     * comes first, into R0, and 132 of the 2149 words of the function and
     * the two steps differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

Uint16 stage_get_stage_act(void)
{
    return STAGE_ACT(stage_current, act_current);
}

Uint16 stage_get_previous_stage_act(void)
{
    return STAGE_ACT(stage_previous, act_previous);
}

Uint16 stage_get_next_stage_act(void)
{
    return STAGE_ACT(stage_next, act_next);
}

void stage_change_act(Sint16 by)
{
    texture_release_act(STAGE_ACT(stage_current, act_current));
    world_release_act(stage_current, act_current);
    download_end();
    act_current += by;
    if (act_current >= STAGE_ACTS) {
        act_current = 0;
    }
    if (stage_current != STAGE_MYSTIC_RUINS && stage_current != STAGE_PAST) {
        light_load_act();
    }
    texture_load_act(STAGE_ACT(stage_current, act_current));
    world_load_act(stage_current, act_current);
    download_start();
    story_steps = 0;
}

void stage_set_entrance(Sint8 entrance)
{
    entrance_next = entrance; /* NOLINT(bugprone-signed-char-misuse) */
}

Sint8 stage_get_entrance(void)
{
    return entrance_next;
}

void stage_place_player(struct task_work *work)
{
    int character;
    struct start_point *start;
    Uint8 minutes;
    Uint8 seconds;
    int total_seconds;

    if (restart_keep_this_act()) {
        restart_restore(&work->pos, &work->ang);
    } else {
        if (act_loaded == act_current) {
            time_reset();
        }
        /* @hack spelling: a switch of one case on a copy of character_current,
         * which loads the character into R4 and tests it in R0 (0x8c052ba6
         * to 0x8c052bae); as an if, or a switch on character_current, it
         * loads into R0 and 24 of 209 words differ */
        character = character_current;
        switch (character) {
        case CHARACTER_E102:
            time_set_whole(3, 0);
            break;
        /* @hack redundant: an empty default, which keeps the words copied from
         * return_point in R2 and its address in R3 (0x8c052bc2 to
         * 0x8c052bcc); without it, R2 and R3 trade and 13 of 209 words
         * differ */
        default:
            break;
        }
        if (return_point != NULL) {
            work->ang.y = return_point->ang_y;
            work->pos = return_point->pos;
            return_point = NULL;
        } else if (stage_current == STAGE_STATION_SQUARE ||
                   stage_current == STAGE_STATION_SQUARE_2 ||
                   stage_current == STAGE_STATION_SQUARE_3) {
            station_square_place_player(work);
        } else if (stage_current == STAGE_EGG_CARRIER ||
                   stage_current == STAGE_EGG_CARRIER_INSIDE) {
            egg_carrier_place_player(work);
        } else if (stage_current == STAGE_MYSTIC_RUINS) {
            mystic_ruins_place_player(work);
        } else if (stage_current == STAGE_PAST) {
            past_place_player(work);
        } else {
#ifdef GCC
            /* GCC can't see that the player's character is one of the six
             * playable, the cases, as stage_create_players gives it */
            start = NULL;
#endif
            /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
            switch (PLAYER_VARS(work)->character) {
            case CHARACTER_SONIC:
                start = sonic_starts;
                break;
            case CHARACTER_TAILS:
                start = tails_starts;
                break;
            case CHARACTER_KNUCKLES:
                start = knuckles_starts;
                break;
            case CHARACTER_BIG:
                start = big_starts;
                break;
            case CHARACTER_AMY:
                start = amy_starts;
                break;
            case CHARACTER_E102:
                start = e102_starts;
                break;
            }
            for (;;) {
                if (start->stage == stage_current &&
                    start->act == act_current) {
                    work->ang.y = start->ang_y;
                    work->pos = start->pos;
                    break;
                }
                if (start->stage == STAGE_NONE) {
                    work->ang.y = 0;
                    work->pos.x = 0.0F;
                    work->pos.y = 0.0F;
                    work->pos.z = 0.0F;
                    break;
                }
                start++;
            }
        }
    }
    player_set_history(PLAYER_VARS(work)->player, 0, &work->pos, NULL);
    /* @hack spelling: a switch of one case on a copy of character_current,
     * which keeps return_point's address in R5 and its value in R4 (0x8c052bb8,
     * 0x8c052bba); as an if, R4 and R5 trade and 48 of 209 words differ */
    character = character_current;
    switch (character) {
    case CHARACTER_E102:
        time_get_whole(&minutes, &seconds);
        /* @hack spelling: the seconds through total_seconds, which also
         * keeps return_point's address in R5 and its value in R4 (0x8c052bb8,
         * 0x8c052bba); compared in place, 12 of 209 words differ */
        total_seconds = (minutes * 60) + seconds;
        if (total_seconds < 60) {
            time_set_whole(1, 0);
        }
        break;
    }
    if (PLAYER_VARS(work)->player == 0 && stage_current <= STAGE_ACTION_LAST) {
        restart_save(&work->pos, &work->ang);
    }
}

/* stage_set_next_exit's work, for stage. act is ignored */
static void stage_set_next_exit_from(Uint16 stage, Uint16 act)
{
    struct stage_exit *entry;
    int request;

    if (stage == STAGE_CART) {
        stage_next = STAGE_STATION_SQUARE;
        act_next = STATION_SQUARE_TWINKLE_ACT;
        entrance_next = CART_ENTRANCE;
        return;
    }
    entry = exits[character_current];
    stage_next = STAGE_STATION_SQUARE;
    act_next = 0;
    entrance_next = 0;
    /* @bug memory: for a stage its list lacks, the walk reads on past EXIT_END,
     * the list's end (CMP/EQ R9,R0 at 0x8c052e80, R9 0xffff). it tests for
     * 0xffff, which no Uint8 stage equals, so it ends only at the stage's
     * exit. D holds the lists in the characters' order, so it reads on into
     * the next character's, as Sonic's Sand Hill takes Tails' row at
     * 0x8c1b4bd0, and past big_exits' into the data after them */
    /* NOLINTNEXTLINE(clang-diagnostic-tautological-*) */
    while (entry->stage != 0xffff) {
#ifdef GCC
        /* repairs the read past the list the tag names: GCC's walk ends at
         * EXIT_END */
        if (entry->stage == EXIT_END) {
            break;
        }
#endif
        if (entry->stage == stage) {
            if (stage_clears[character_current][stage] == 1) {
                stage_next = entry->first_stage;
                act_next = entry->first_act;
                entrance_next = entry->first_entrance;
            } else if (entry->again_stage != 0) {
                stage_next = entry->again_stage;
                act_next = entry->again_act;
                entrance_next = entry->again_entrance;
            } else {
                stage_next = entry->first_stage;
                act_next = entry->first_act;
                entrance_next = entry->first_entrance;
            }
            if (player_record != NULL) {
                player_record->stage_act = STAGE_ACT(stage_next, act_next);
                player_record->entrance = entrance_next;
                save_write_wait();
            }
            break;
        }
        entry++;
        request = entry->movie; /* NOLINT(bugprone-signed-char-misuse) */
        switch (character_current) {
        case CHARACTER_SONIC:
            if (event_flags[FLAG_CLEAR_SONIC]) {
                request = 0;
            }
            break;
        case CHARACTER_TAILS:
            if (event_flags[FLAG_CLEAR_TAILS]) {
                request = 0;
            }
            break;
        case CHARACTER_KNUCKLES:
            if (event_flags[FLAG_CLEAR_KNUCKLES]) {
                request = 0;
            }
            break;
        case CHARACTER_AMY:
            if (event_flags[FLAG_CLEAR_AMY]) {
                request = 0;
            }
            break;
        case CHARACTER_E102:
            if (event_flags[FLAG_CLEAR_E102]) {
                request = 0;
            }
            break;
        case CHARACTER_BIG:
            if (event_flags[FLAG_CLEAR_BIG]) {
                request = 0;
            }
            break;
        }
        movie_request = request;
    }
}

void stage_set_next_exit(void)
{
    stage_set_next_exit_from(stage_current, act_current);
}

/* set the next stage and act to the boss after this one in the character's
 * list, and return TRUE. return FALSE where no boss follows. from a stage
 * that isn't a boss, it sets station square's act 0 and returns FALSE */
static Bool stage_set_next_boss(void)
{
    Sint16 *walk;

    if (stage_current <= STAGE_ACTION_LAST || stage_current > STAGE_BOSS_LAST) {
        stage_next = STAGE_STATION_SQUARE;
        act_next = 0;
        return FALSE;
    }
#ifdef GCC
    /* GCC can't see that character_current is one of the six playable, the
     * cases */
    walk = NULL;
#endif
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    switch (character_current) {
    case CHARACTER_SONIC:
        walk = sonic_bosses;
        break;
    case CHARACTER_TAILS:
        walk = tails_bosses;
        break;
    case CHARACTER_KNUCKLES:
        walk = knuckles_bosses;
        break;
    case CHARACTER_AMY:
        walk = amy_bosses;
        break;
    case CHARACTER_E102:
        walk = e102_bosses;
        break;
    case CHARACTER_BIG:
        walk = big_bosses;
        break;
    }
    for (; walk[0] != STAGE_NONE; walk += 2) {
        if (walk[0] == stage_current) {
            if (walk[2] != STAGE_NONE) {
                stage_next = walk[2];
                act_next = walk[3];
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

Bool stage_paused(void)
{
    return stage_mode == STAGE_PAUSE;
}

Bool stage_playing(void)
{
    return stage_mode == STAGE_PLAY;
}

#ifdef GCC
Bool stage_in_play(void)
{
    return stage_mode == STAGE_PLAY || stage_mode == STAGE_FADE_IN;
}
#endif
