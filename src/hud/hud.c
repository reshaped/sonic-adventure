/* the hud's rings, lives, time and score. score_step runs the screen that
 * ends a stage: an act's tally, the boss trial's times, or a minigame's */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "draw/draw.h"
#include "hud/board.h"
#include "hud/hud.h"
#include "hud/overlay.h"
#include "main.h"
#include "minicart/score.h"
#include "minicart/times.h"
#include "object/common.h"
#include "player/fishing.h"
#include "player/player.h"
#include "player/race.h"
#include "progress/emblem.h"
#include "save/card.h"
#include "save/minigame.h"
#include "save/save.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg00/mole.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"
#include "vmu/screen.h"

/* hud_texanims' sprites after the ten digits */
#define HUD_COLON 10
#define HUD_LABELS 11 /* the time's and the rings' labels, as one sprite */
/* the first of the heads, one for each character, in character order */
#define HUD_LIVES 12

/* minutes and seconds as steps, 60 to a second, the unit of time_frames */
#define TIME(minutes, seconds) (((minutes) * 3600) + ((seconds) * 60))

/* mission A's limit on the cart: ten minutes, in hundredths of a second */
#define CART_A_TIME 60000

/* the buttons that skip a score screen ahead, any of them held */
#define SKIP_BUTTONS                                                           \
    (PDD_DGT_TX | PDD_DGT_TY | PDD_DGT_ST | PDD_DGT_TA | PDD_DGT_TB)
/* the wait a skip cuts a score screen's wait to. a screen skips only while
 * its wait is longer */
#define SKIP_WAIT 10

/* the steps after points within which more points join the combo */
#define COMBO_WAIT 120
#define COMBO_MOST 500 /* the most points a combo holds */

/* a show task's last step: the lives or the rings show for 181 steps */
#define SHOW_LAST 180

#define SE_EXTRA_LIFE 704
#define SE_TIME_BEEP 779 /* E-102's beep, as his time runs out */
/* a score screen's tick, as each of its rows shows */
#define SE_TICK 5

/* the characters' voices as a score screen ends: for a record, for a
 * mission cleared, and for one failed */
#define VOICE_SONIC_RECORD 0x57e
#define VOICE_SONIC_CLEARED 0x580
#define VOICE_SONIC_FAILED 0x57d
#define VOICE_TAILS_RECORD 0x554
#define VOICE_TAILS_CLEARED 0x556
#define VOICE_TAILS_FAILED 0x553
#define VOICE_KNUCKLES_RECORD 0x543
#define VOICE_KNUCKLES_CLEARED 0x545
#define VOICE_KNUCKLES_FAILED 0x542
#define VOICE_AMY_RECORD 0x503
#define VOICE_AMY_CLEARED 0x502
#define VOICE_AMY_FAILED 0x500
#define VOICE_BIG_RECORD 0x513
#define VOICE_BIG_CLEARED 0x515
#define VOICE_BIG_FAILED 0x512

/* the points a boss adds to an act's points bonus */
#define BOSS_BONUS 1000.0F

/* the points a tally's count moves in a step from a bonus into the score,
 * or the bonus's rest when less is left. the Hedgehog Hammer's count moves
 * its Eggman row the other way, out of the score. the time bonus and Big's
 * fish move TALLY_TIME_STEP, the rings bonus TALLY_RINGS_STEP, and every
 * other bonus TALLY_STEP */
#define TALLY_STEP 46.0F
#define TALLY_TIME_STEP 83.0F
#define TALLY_RINGS_STEP 7.0F

/* every score screen's last two modes, after its own. SCORE_WAIT counts
 * work->wait down to 0, or for the cart up past 30, then moves on.
 * SCORE_END releases the screen's textures and kills its task, and all but
 * the Hedgehog Hammer's and the cart's end the stage too */
#define SCORE_WAIT 19
#define SCORE_END 20

enum combo_mode { COMBO_AIR, COMBO_LANDED };

/* score_mission's steps, in its work's step */
enum mission_step {
    STEP_INIT,
    STEP_SHOW, /* the mission and whether the run cleared it */
    STEP_EMBLEM, /* the emblem count, once a mission is cleared */
    STEP_WAIT,
    STEP_END
};

/* the boss trial screen's modes. from TRIAL_SONIC1 to TRIAL_E102_2, each
 * shows one boss time more than the mode before it in the character's run,
 * which TRIAL_BOSSES starts. Sonic's run is TRIAL_SONIC1 to TRIAL_SONIC5.
 * Tails', Knuckles' and Big's runs take some of his modes, and Amy's and
 * E-102's take only their own */
enum trial_mode {
    TRIAL_INIT,
    TRIAL_WAIT,
    TRIAL_SAVE,
    TRIAL_BOSSES,
    TRIAL_SONIC1,
    TRIAL_SONIC2,
    TRIAL_SONIC3,
    TRIAL_SONIC4,
    TRIAL_SONIC5,
    TRIAL_TAILS3,
    TRIAL_KNUCKLES1,
    TRIAL_AMY1,
    TRIAL_E102_1,
    TRIAL_E102_2,
    TRIAL_TOTAL,
    TRIAL_HOLD
};

/* the Hedgehog Hammer screen's modes. from MOLE_BACK, each draws one row
 * more than the mode before it */
enum mole_mode {
    MOLE_INIT,
    MOLE_WAIT,
    MOLE_BACK,
    MOLE_BONUS1,
    MOLE_BONUS2,
    MOLE_BONUS3,
    MOLE_COUNT,
    MOLE_MISSION
};

enum act_mode {
    ACT_INIT,
    ACT_WAIT,
    ACT_SAVE,
    ACT_BACK,
    ACT_TIME,
    ACT_RINGS,
    ACT_BONUS1,
    ACT_BONUS2,
    ACT_BONUS3,
    ACT_COUNT,
    ACT_MISSION
};

enum board_mode {
    BOARD_INIT,
    BOARD_BACK,
    BOARD_BONUS1,
    BOARD_BONUS2,
    BOARD_BONUS3,
    BOARD_COUNT,
    BOARD_MISSION
};

/* Sky Chase's score screen's modes */
enum shoot_mode {
    SHOOT_INIT,
    SHOOT_BACK,
    SHOOT_SCORE,
    SHOOT_RATE,
    SHOOT_MISSION
};

enum cart_mode {
    CART_INIT,
    CART_WAIT,
    CART_SAVE,
    CART_BACK,
    CART_LAP1,
    CART_LAP2,
    CART_LAP3,
    CART_TIME,
    CART_MISSION
};

/* a time for score_draw_time, and where it goes */
struct score_time {
    Uint32 minutes;
    Uint32 seconds;
    Uint32 hundredths;
    Float x;
    Float y;
};

/* a score screen's work, as its tally lays it out. the count empties
 * bonus1, bonus2 and bonus3 in turn, moving their points from total to
 * score. the Hedgehog Hammer's bonus3 moves the other way, from score to
 * total. score_mission takes the work over after the count */
struct tally_work {
    Sint8 mode;
    Uint8 unk_01[2];
    Uint8 step; /* score_mission's step */
    Uint8 unk_04[4];
    /* the mode's wait, in steps. most modes count it down, and the cart's
     * SCORE_WAIT counts it up */
    Uint32 wait;
    Float score; /* the score as counted so far */
    Float total; /* the points still to count */
    Uint8 unk_14[12];
    Float record; /* 1 when the run set a record, else 0 */
    Uint8 unk_24[8];
    Float bonus1;
    Float bonus2;
    Float bonus3;
};

/* task's work as a tally */
/* @hack spelling: the work read through a cast at its one use in each row
 * draw, which has the draw store score_number.max before it loads task
 * back from the frame for that read (0x8c0919fe to 0x8c091a02 in
 * mole_draw_bonus1); with a local set where it is declared, 480 of the 518
 * words of the 14 row draws that use it differ, and with one set after the
 * draw's call, 70 */
#define TALLY(task) ((struct tally_work *)(task)->work)

/* the same work, as score_mission lays it out after the count */
struct mission_work {
    Sint8 mode;
    Uint8 unk_01[2];
    Uint8 step;
    Uint8 unk_04[2];
    Uint16 timer; /* the steps run since step last changed */
    Sint32 wait; /* the mission shown, then SCORE_WAIT's wait */
    Bool cleared;
    Uint8 unk_10[16];
    Float record;
};

struct trial_work {
    Sint8 mode;
    Sint8 character;
    Uint8 unk_02[6];
    Uint32 wait;
    Sint32 total; /* every boss's time, in steps */
    Sint32 unk_10; /* a copy of total, which this file never reads */
};

/* a show task's extra: the steps it has run */
struct show_extra {
    int steps;
};

#define EXTRA(task) ((struct show_extra *)(task)->extra)

static void draw_digits(NJS_SPRITE *sprite, int value, Uint8 digits);
static void lives_show_step(struct task *task);
static void rings_show_step(struct task *task);
static void combo_add(int points);
static void draw_rings_lives(void);
static void draw_time(Float x, Float y);
static Float time_bonus(void);
static void score_draw_clock(Float x, Float y);
static void score_draw_time(struct score_time time);
static void score_draw_back(Float x, Float y, Float width, Float height);
static int save_stage_index(int character, int stage);
static Bool mission_check_action(int character, int stage, int mission);
static Bool mission_check(int character, int stage, int mission);
static void score_mission(struct task *task);
static void trial_draw(struct task *task);
static void trial_draw_sonic1(struct task *task);
static void trial_draw_knuckles1(struct task *task);
static void trial_draw_sonic2(struct task *task);
static void trial_draw_sonic3(struct task *task);
static void trial_draw_sonic4(struct task *task);
static void trial_draw_sonic5(struct task *task);
static void trial_draw_tails3(struct task *task);
static void trial_draw_amy1(struct task *task);
static void trial_draw_e102_1(struct task *task);
static void trial_draw_e102_2(struct task *task);
static void trial_draw_total(struct task *task);
static void score_trial_step(struct task *task);
static void mole_draw(struct task *task);
static void mole_draw_bonus1(struct task *task);
static void mole_draw_bonus2(struct task *task);
static void mole_draw_bonus3(struct task *task);
static void mole_draw_score(struct task *task);
static void score_mole_step(struct task *task);
static void act_draw(struct task *task);
static void act_draw_rings(struct task *task);
static void act_draw_bonus1(struct task *task);
static void act_draw_bonus2(struct task *task);
static void act_draw_bonus2_big(struct task *task);
static void act_draw_bonus3(struct task *task);
static void act_draw_score(struct task *task);
static void score_act_step(struct task *task);
static void board_draw(struct task *task);
static void board_draw_bonus1(struct task *task);
static void board_draw_bonus2(struct task *task);
static void board_draw_bonus3(struct task *task);
static void board_draw_score(struct task *task);
static void score_board_step(struct task *task);
static Bool mission_check_board(int character, int mission);
static void shoot_draw(struct task *task);
static void shoot_draw_score(struct task *task);
static void shoot_draw_rate(struct task *task);
static void score_shoot_step(struct task *task);
static void cart_draw(struct task *task);
static void cart_draw_lap1(struct task *task);
static void cart_draw_lap2(struct task *task);
static void cart_draw_lap3(struct task *task);
static void cart_draw_time(struct task *task);
static void score_cart_step(struct task *task);
static Bool mission_check_cart(int character, int mission);

static Bool time_running = TRUE;

NJS_TEXLIST hud_digit_texlist = {
#include "hud/hud/hud_digit_texlist.inc"
};
NJS_TEXLIST hud_texlist = {
#include "hud/hud/hud_texlist.inc"
};

static NJS_TEXANIM digit_texanims[HUD_COLON + 1] = {
#include "hud/hud/digit_texanims.inc"
};

/* hud_texlist's sprites: the ten digits and the colon, the labels, a head
 * for each character, and the four ranks */
static NJS_TEXANIM hud_texanims[24] = {
#include "hud/hud/hud_texanims.inc"
};

/* the word MISSION, the letters A to C, and the result, by cleared: 4 for
 * FALSE and 5 for TRUE */
static NJS_TEXANIM mission_texanims[6] = {
#include "hud/hud/mission_texanims.inc"
};

static NJS_SPRITE hud_sprite = {
#include "hud/hud/hud_sprite.inc"
};
/* the big digits' sprite, which no code draws */
static NJS_SPRITE digit_sprite = {
#include "hud/hud/digit_sprite.inc"
};
static NJS_SPRITE mission_sprites[3] = {
#include "hud/hud/mission_sprites.inc"
};

/* the rings' label alone, which a field draws in place of HUD_LABELS */
static NJS_TEXANIM field_label_texanims[1] = {
#include "hud/hud/field_label_texanims.inc"
};
static NJS_SPRITE field_label_sprite = {
#include "hud/hud/field_label_sprite.inc"
};

static struct hud_number score_number = {
    NUMBER_SCALED | NUMBER_CLAMPED,
    NUMBER_LEFT,
    0,
    1000000,
    0,
    {0.0F, 0.0F, 0.0F},
    2.0F,
    0xffffffff,
};
static struct hud_number time_number = {
    NUMBER_SCALED | NUMBER_CLAMPED,
    NUMBER_LEFT,
    0,
    99,
    0,
    {0.0F, 0.0F, 0.0F},
    2.0F,
    0xffffffff,
};

/* a score screen panel's fill, corner and edge */
static NJS_TEXANIM back_texanims[3] = {
#include "hud/hud/back_texanims.inc"
};

/* by character, a bit for each stage whose records a save keeps */
static Uint16 record_stages[CHARACTER_COUNT] = {
    0x07fe, 0x0000, 0x0354, 0x02f0, 0x0000, 0x1408, 0x1426, 0x110a,
};

/* mission_times' rows: the characters with a time for mission A */
enum times_row {
    TIMES_SONIC,
    TIMES_KNUCKLES,
    TIMES_AMY,
    TIMES_E102,
    TIMES_ROWS
};

/* mission A's times by stage, in steps: the times Sonic, Knuckles and Amy
 * must finish under, and the time E-102 must have left. a stage the
 * character doesn't play has 0 */
static int mission_times[TIMES_ROWS][STAGE_SHELTER + 1] = {
    {0, TIME(2, 0), TIME(3, 0), TIME(3, 0), TIME(2, 30), TIME(3, 0), TIME(5, 0),
     TIME(4, 30), TIME(4, 0), TIME(5, 0), TIME(4, 30), 0, 0},
    {0, 0, 0, 0, TIME(1, 0), TIME(1, 0), TIME(2, 0), TIME(1, 0), 0, TIME(1, 0),
     0, 0, 0},
    {0, 0, 0, TIME(2, 0), 0, 0, 0, 0, 0, 0, TIME(2, 30), 0, TIME(6, 30)},
    {0, TIME(3, 0), TIME(2, 30), 0, 0, TIME(3, 0), 0, 0, 0, 0, TIME(2, 30), 0,
     TIME(2, 0)},
};

/* the label of each boss's row, and of the total's */
static NJS_TEXANIM trial_texanims[11] = {
#include "hud/hud/trial_texanims.inc"
};
/* the boss trial's rows */
static NJS_SPRITE trial_sprites[6] = {
#include "hud/hud/trial_sprites.inc"
};

/* the Hedgehog Hammer's rows */
static NJS_TEXANIM mole_texanims[4] = {
#include "hud/hud/mole_texanims.inc"
};
static NJS_SPRITE mole_sprites[4] = {
#include "hud/hud/mole_sprites.inc"
};
/* an act's rows, and Big's fish in place of the time bonus */
static NJS_TEXANIM act_texanims[7] = {
#include "hud/hud/act_texanims.inc"
};
static NJS_SPRITE act_sprites[7] = {
#include "hud/hud/act_sprites.inc"
};
/* a board's rows */
static NJS_TEXANIM board_texanims[4] = {
#include "hud/hud/board_texanims.inc"
};
static NJS_SPRITE board_sprites[4] = {
#include "hud/hud/board_sprites.inc"
};
/* Sky Chase's rows */
static NJS_TEXANIM shoot_texanims[3] = {
#include "hud/hud/shoot_texanims.inc"
};
static NJS_SPRITE shoot_sprites[3] = {
#include "hud/hud/shoot_sprites.inc"
};
/* the cart's rows */
static NJS_TEXANIM cart_texanims[4] = {
#include "hud/hud/cart_texanims.inc"
};
static NJS_SPRITE cart_sprites[4] = {
#include "hud/hud/cart_sprites.inc"
};

struct mole_hit mole_hits[MOLE_KINDS];
/* the steps left for more points to join the combo, and its points */
static int combo_timer;
static int combo;
static int dat_8c7a2338;
/* the angle of the ring count's blink while there are no rings */
static Angle ring_blink;
/* steps to E-102's next beep, as his time runs out */
static Sint8 beep_timer;

/* draw value in digits places, leading zeros included, from the sprite's
 * place rightward, 16 pixels apart. it draws no more than 5 places, and
 * holds value to the most that fits them. the sprite is left on the last
 * digit. it draws nothing while hud_hidden is below 0 */
static void draw_digits(NJS_SPRITE *sprite, int value, Uint8 digits)
{
    int limit;
    int i;
    /* @hack type: volatile on digit, which stores each digit to the
     * frame and loads it back for njDrawSprite2D (MOV.L R0,@R15 at
     * 0x8c08ef16, MOV.L @R15,R5 at 0x8c08ef1a); as a plain int, 136 of 139
     * words differ */
    volatile int digit;

    if (hud_hidden < 0) {
        return;
    }
    if (value >= 100000) {
        value = 99999;
    }
    limit = 1;
    for (i = 0; i < digits; i++) {
        limit *= 10;
    }
    limit--;
    if (value > limit) {
        value = limit;
    }
    if (digits >= 5) {
        digit = value / 10000;
        njDrawSprite2D(sprite, digit, -1.5F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite->p.x += 16.0F;
    }
    if (digits >= 4) {
        digit = value % 10000 / 1000;
        njDrawSprite2D(sprite, digit, -1.501F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite->p.x += 16.0F;
    }
    if (digits >= 3) {
        digit = value % 1000 / 100;
        njDrawSprite2D(sprite, digit, -1.502F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite->p.x += 16.0F;
    }
    if (digits >= 2) {
        digit = value % 100 / 10;
        njDrawSprite2D(sprite, digit, -1.503F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite->p.x += 16.0F;
    }
    if (digits >= 1) {
        digit = value % 10;
        njDrawSprite2D(sprite, digit, -1.504F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
}

void hud_reset(void)
{
    ring_reset();
    lives_reset();
    trial_count = 0;
}

static void score_clear(void)
{
    score = 0;
    bonus_points = 0;
    /* @hack redundant: a return where the function ends. in score_reset, which
     * SHC expands it into, each store follows its address's load, by turns
     * in R3 and R2 (0x8c08f032 to 0x8c08f048); without it, combo_clear's
     * loads move ahead of these stores, and 8 of score_reset's 12 words
     * differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

static void combo_clear(void)
{
    combo_timer = 0;
    combo = 0;
    /* @hack redundant: a return where the function ends. in score_reset, it
     * keeps the load of hints_used's address in R3, between combo_timer's store
     * and combo's (0x8c08f042), and in combo_air_step, the 120 in R2
     * (0x8c08f1e4); without it, that load moves ahead of every store, and 9
     * of the 43 words of the two differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void score_reset(void)
{
    score_clear();
    combo_clear();
    hints_used = 0;
}

void ring_reset(void)
{
    rings = 0;
    /* @hack redundant: a return where the function ends. SHC expands this
     * function and lives_reset into hud_reset. with lives_reset's return,
     * this one has hud_reset load all three addresses before its first
     * store, trial_count's into R0 (0x8c08f020 to 0x8c08f02a); without it,
     * 8 of the 13 words of ring_reset and hud_reset differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void lives_reset(void)
{
    lives = LIVES_START;
    /* @hack redundant: a return where the function ends, which steers hud_reset
     * as ring_reset's does; without it, 7 of the 13 words of lives_reset
     * and hud_reset differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* hold lives_shown at 0 for 181 steps, which shows the lives in a field,
 * then kill the task */
static void lives_show_step(struct task *task)
{
    if (EXTRA(task)->steps++ > SHOW_LAST) {
        task_kill(task);
    } else {
        lives_shown = 0;
    }
}

void lives_add(Sint16 count)
{
    if (count > 0) {
        sound_play(SE_EXTRA_LIFE, 0, 0, 0);
    }
    lives += (Sint8)count;
    /* a Sint8 past 127 wraps below 0, as does a loss at 0. either way the
     * lives become 127 */
    if (lives < 0) {
        lives = 127;
    }
    if (stage_group() == STAGE_GROUP_FIELD) {
        task_create(TASK_EXTRA, 6, lives_show_step);
    }
}

/* lives_show_step for the rings */
static void rings_show_step(struct task *task)
{
    if (EXTRA(task)->steps++ > SHOW_LAST) {
        task_kill(task);
    } else {
        rings_shown = 0;
    }
}

void ring_add(int count)
{
    Sint16 old = rings;
    int old_hundreds;
    int new_hundreds;

    rings += count;
    old_hundreds = old / 100;
    new_hundreds = rings / 100;
    if (old_hundreds < new_hundreds) {
        lives_add(new_hundreds - old_hundreds);
    }
    if (stage_group() == STAGE_GROUP_FIELD) {
        task_create(TASK_EXTRA, 6, rings_show_step);
    }
}

/* nothing calls fun_8c08f1b2 or combo_air_step: no image holds either's
 * address. neither is static, since SHC leaves out a static function that
 * nothing calls */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void fun_8c08f1b2(struct task *task)
{
    task_kill(task);
}

/* hold the combo's window open while player 0 is in the air. once they
 * land, clear the combo, and kill the task on the next step */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void combo_air_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case COMBO_AIR:
        if (player_work[0]->flags & PLAYER_ON_GROUND) {
            work->mode = COMBO_LANDED;
            combo_clear();
        } else {
            combo_timer = COMBO_WAIT;
            /* @hack spelling: a goto where a break would do, which gives the
             * function one exit for every case. task_kill's call is then a
             * JSR, with PR saved (0x8c08f1b8, 0x8c08f1ec); with a break, it
             * is a JMP, and all 31 words differ */
            goto done;
        }
        break;
    case COMBO_LANDED:
        task_kill(task);
        break;
    }
done:;
}

/* add points to the combo, to at most COMBO_MOST, if they come within
 * COMBO_WAIT steps of the last. points after the window closes set the
 * combo to 0 without counting. either way, the window opens again */
static void combo_add(int points)
{
    int value;

    if (combo_timer > 0) {
        value = combo + points;
        if (value > COMBO_MOST) {
            value = COMBO_MOST;
        }
        combo = value;
    } else {
        combo = 0;
    }
    combo_timer = COMBO_WAIT;
}

void score_add(int points)
{
    score += points;
}

void score_add_bonus(int points)
{
    combo_add(points);
    bonus_points += points;
}

int ring_count(void)
{
    return rings;
}

/* draw the labels and the ring count, unless rings_shown is below 0. the
 * count blinks while there are no rings. then draw the character's head
 * and the lives, unless lives_shown is below 0 */
static void draw_rings_lives(void)
{
    NJS_SPRITE *sprite = &hud_sprite;
    Float sine;
    Float bright;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    sprite->sx = sprite->sy = 1.0F;
    if (rings_shown >= 0) {
        /* a board played as a minigame draws its own labels */
        if (board_active() && menu_state.trial[0] == MENU_TRIAL_MINI_GAME) {
            hud_draw_board_labels();
        } else {
            switch (stage_get_stage_act() >> 8) {
            /* the adventure fields */
            case STAGE_STATION_SQUARE:
            case STAGE_STATION_SQUARE_2:
            case STAGE_STATION_SQUARE_3:
            case STAGE_EGG_CARRIER:
            case STAGE_EGG_CARRIER_2:
            case STAGE_EGG_CARRIER_3:
            case STAGE_EGG_CARRIER_INSIDE:
            case STAGE_MYSTIC_RUINS:
            case STAGE_PAST:
                njDrawSprite2D(&field_label_sprite, 0, -2.0F, NJD_SPRITE_ALPHA);
                break;
            default:
                sprite->p.x = 16.0F;
                sprite->p.y = 32.0F;
                njDrawSprite2D(sprite, HUD_LABELS, -2.0F, NJD_SPRITE_ALPHA);
                break;
            }
        }
        sprite->p.x = 46.0F;
        sprite->p.y = 52.0F;
        if (rings == 0) {
            ring_blink += 0x400;
            sine = njSin(ring_blink);
            bright = 1.0F - (sine * sine);
        } else {
            bright = 1.0F;
        }
        draw_set_material(1.0F, 1.0F, bright, bright);
        draw_digits(sprite, rings, 3);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    if (lives_shown >= 0) {
        sprite->p.x = 16.0F;
        sprite->p.y = 400.0F;
        njDrawSprite2D(sprite, get_character() + HUD_LIVES, -2.0F,
                       NJD_SPRITE_ALPHA);
        sprite->p.x = 49.0F;
        sprite->p.y = 416.0F;
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        draw_digits(sprite, lives, 2);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
}

/* in a field, the show tasks set rings_shown and lives_shown to 0 each
 * step, and each draw sets them back to -1 */
void hud_draw(void)
{
    if (hud_hidden < 0) {
        return;
    }
    if (cart_active()) {
        return;
    }
    if (get_character() == CHARACTER_BIG) {
        return;
    }
#ifdef GCC
    /* the port's 16:9 squeezes the rings and the lives about the screen's
     * left edge */
    port_wide_anchor = WIDE_LEFT;
#endif
    draw_rings_lives();
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
    if (stage_group() == STAGE_GROUP_FIELD) {
        rings_shown = -1;
        lives_shown = -1;
    } else {
        rings_shown = lives_shown = hud_hidden;
    }
}

Sint16 lives_count(void)
{
    return lives;
}

void play_time_reset(void)
{
    play_time = 0;
    time_reset();
    /* @hack redundant: a return where the function ends, which ends it in a BRA
     * to time_reset, the next function (0x8c08f534); without it, SHC lets
     * the function fall into time_reset, 2 of its 4 words differ, and the
     * functions after it move */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void time_reset(void)
{
    time_frames = 0;
    time_seconds = 0;
    time_minutes = 0;
    if (get_character() == CHARACTER_E102) {
        time_minutes = 3;
        time_seconds = 0;
    }
    time_steps = 0;
}

void time_start(void)
{
    time_running = TRUE;
}

void time_stop(void)
{
    time_running = FALSE;
}

void play_time_count(void)
{
    play_time += frame_step_count;
}

/* while the time runs, it counts up to 99 minutes, 59 seconds and 59
 * steps, and E-102's counts down to 0. with under ten seconds left on his,
 * SE_TIME_BEEP plays every two seconds, and with five or fewer, every
 * second. a running time also counts the combo's window down */
void time_step(void)
{
    int seconds;

    if (time_running) {
        if (get_character() == CHARACTER_E102) {
            if (--time_frames < 0) {
                time_frames = 59;
                if (--time_seconds < 0) {
                    time_seconds = 59;
                    if (--time_minutes < 0) {
                        time_frames = time_seconds = time_minutes = 0;
                    }
                }
            }
            seconds = (time_minutes * 60) + time_seconds;
            if (seconds < 10 && seconds > 0) {
                beep_timer--;
                if (beep_timer <= 0) {
                    sound_play(SE_TIME_BEEP, 0, 0, 0);
                    if (seconds <= 5) {
                        beep_timer = 60;
                    } else {
                        beep_timer = 120;
                    }
                }
            }
        } else {
            if (++time_frames >= 60) {
                time_frames = 0;
                if (++time_seconds >= 60) {
                    time_seconds = 0;
                    if (++time_minutes >= 100) {
                        time_minutes = 99;
                        time_seconds = 59;
                        time_frames = 59;
                    }
                }
            }
        }
        if (combo_timer-- < 0) {
            combo_timer = 0;
        }
    }
    time_steps++;
}

int play_time_get(void)
{
    return play_time;
}

void hud_draw_timer(void)
{
    if (hud_hidden < 0 || rings_shown < 0) {
        return;
    }
    if (get_character() == CHARACTER_BIG) {
        return;
    }
    switch (stage_get_stage_act() >> 8) {
    case STAGE_SHOOTING1:
    case STAGE_SHOOTING2:
        return;
    }
    if (cart_active()) {
        return;
    }
#ifdef GCC
    /* the port's 16:9 squeezes the time, or a board's score, about the
     * screen's left edge */
    port_wide_anchor = WIDE_LEFT;
#endif
    if (board_active() && menu_state.trial[0] == MENU_TRIAL_MINI_GAME) {
        hud_draw_board_score(score);
    } else {
        draw_time(64.0F, 33.0F);
    }
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}

/* draw the time as minutes, seconds and hundredths, from x, y. in E-102's
 * last 30 seconds his time pulses from white to yellow, and in his last
 * 15, from yellow to red */
static void draw_time(Float x, Float y)
{
    NJS_SPRITE sprite;
    Float green;
    Float blue;
    Uint32 hundredths;
    Sint16 digit;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    njSetTexture(&hud_texlist);
    sprite.tlist = &hud_texlist;
    sprite.tanim = hud_texanims;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = x;
    sprite.p.y = y;
    if (time_minutes <= 99) {
        green = 1.0F;
        blue = 1.0F;
        if (get_character() == CHARACTER_E102) {
            if (time_minutes < 1 && time_seconds < 30) {
                blue = njAbs(njSin(time_frames * (65536.0F / 60.0F)));
                if (time_seconds < 15) {
                    green = blue;
                    blue = 0.0F;
                } else {
                    green = 1.0F;
                }
            }
        }
        draw_set_material(1.0F, 1.0F, green, blue);
        digit = time_minutes / 10;
        njDrawSprite2D(&sprite, digit, -2.001F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite.p.x += 16.0F;
        digit = time_minutes % 10;
        njDrawSprite2D(&sprite, digit, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, HUD_COLON, -2.001F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite.p.x += 16.0F;
        digit = time_seconds / 10;
        njDrawSprite2D(&sprite, digit, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite.p.x += 16.0F;
        digit = time_seconds % 10;
        njDrawSprite2D(&sprite, digit, -2.001F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, HUD_COLON, -2.001F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        hundredths = time_frames * (100.0F * (1.0F / 60.0F));
        sprite.p.x += 16.0F;
        digit = hundredths / 10;
        njDrawSprite2D(&sprite, digit, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        sprite.p.x += 16.0F;
        digit = hundredths % 10;
        njDrawSprite2D(&sprite, digit, -2.001F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    } else {
        /* past 99 minutes, 99:59:99 */
        njDrawSprite2D(&sprite, 9, -2.001F, NJD_SPRITE_ALPHA);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, 9, -2.0F, NJD_SPRITE_ALPHA);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, HUD_COLON, -2.001F, NJD_SPRITE_ALPHA);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, 5, -2.0F, NJD_SPRITE_ALPHA);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, 9, -2.001F, NJD_SPRITE_ALPHA);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, HUD_COLON, -2.001F, NJD_SPRITE_ALPHA);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, 9, -2.0F, NJD_SPRITE_ALPHA);
        sprite.p.x += 16.0F;
        njDrawSprite2D(&sprite, 9, -2.001F, NJD_SPRITE_ALPHA);
    }
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
}

void time_get_whole(Uint8 *minutes, Uint8 *seconds)
{
    *seconds = time_seconds;
    *minutes = time_minutes;
}

void time_set_whole(int minutes, int seconds)
{
    time_seconds = seconds;
    time_minutes = minutes;
    time_frames = 0;
}

void time_set(Uint8 minutes, Uint8 seconds, Uint8 frames)
{
    time_seconds = seconds;
    time_minutes = minutes;
    time_frames = frames;
}

void time_get(Uint8 *minutes, Uint8 *seconds, Uint8 *frames)
{
    *seconds = time_seconds;
    *minutes = time_minutes;
    *frames = time_frames;
}

void time_add_seconds(int seconds)
{
    seconds += time_seconds;
    while (seconds >= 60) {
        seconds -= 60;
        time_minutes++;
    }
    time_seconds = seconds;
}

Bool time_is_zero(void)
{
    return time_frames == 0 && time_seconds == 0 && time_minutes == 0;
}

/* the time bonus. under a minute it is 5000. from a minute on, it is 5000
 * less 10 for every second of the whole time, and not below 0. E-102 gets
 * 25 for each second he has left, or 5000 with three minutes or more
 * left */
static Float time_bonus(void)
{
    Float bonus = 5000.0F;
    Float used;

    if (get_character() == CHARACTER_E102) {
        if (time_minutes < 3) {
            bonus = ((time_minutes * 60) + time_seconds) * 25.0F;
        }
    } else {
        used = ((time_minutes * 60) + time_seconds) * 10.0F;
        if (time_minutes != 0) {
            bonus -= used;
            if (bonus < 0.0F) {
                bonus = 0.0F;
            }
        }
    }
    return bonus;
}

/* draw the time as a score screen's clock from x, y: minutes, seconds and
 * hundredths */
static void score_draw_clock(Float x, Float y)
{
    Float width = time_number.scale * 16.0F;

    time_number.max = 99;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    time_number.value = time_minutes;
    time_number.pos.x = x;
    time_number.pos.y = y;
    hud_draw_number(&time_number);
    x += width * 2.0F;
    time_number.flags |= NUMBER_COLON;
    time_number.max = 9;
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    x += width;
    time_number.flags |= NUMBER_ZEROS;
    time_number.flags &= ~NUMBER_COLON;
    time_number.max = 59;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    time_number.value = time_seconds;
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    x += width * 2.0F;
    time_number.flags |= NUMBER_COLON;
    time_number.max = 9;
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    x += width;
    time_number.flags &= ~NUMBER_COLON;
    time_number.max = 99;
    time_number.value = time_frames * (100.0F * (1.0F / 60.0F));
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    time_number.flags &= ~NUMBER_ZEROS;
}

/* draw time as score_draw_clock draws the time */
static void score_draw_time(struct score_time time)
{
    Float x = time.x;
    Float width = time_number.scale * 16.0F;

    time_number.max = 99;
    time_number.value = time.minutes;
    time_number.pos.x = x;
    time_number.pos.y = time.y;
    hud_draw_number(&time_number);
    x += width * 2.0F;
    time_number.flags |= NUMBER_COLON;
    time_number.max = 9;
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    x += width;
    time_number.flags |= NUMBER_ZEROS;
    time_number.flags &= ~NUMBER_COLON;
    time_number.max = 59;
    time_number.value = time.seconds;
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    x += width * 2.0F;
    time_number.flags |= NUMBER_COLON;
    time_number.max = 9;
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    x += width;
    time_number.flags &= ~NUMBER_COLON;
    time_number.max = 99;
    time_number.value = time.hundredths;
    time_number.pos.x = x;
    hud_draw_number(&time_number);
    time_number.flags &= ~NUMBER_ZEROS;
}

/* draw a panel centered on x, y: a fill of width by height 32-pixel
 * cells, in a frame 32 pixels wide */
static void score_draw_back(Float x, Float y, Float width, Float height)
{
    NJS_ARGB color;
    NJS_SPRITE sprite;
    Float half_width;
    Float half_height;
    Float right;
    Float top;
    Float left;
    Float bottom;
    Float sx;
    Float sy;

    half_width = (width + 1.0F) * 16.0F;
    half_height = (height + 1.0F) * 16.0F;
    left = x - half_width;
    right = x + half_width;
    top = y - half_height;
    bottom = y + half_height;
    sx = width * 4.0F;
    sy = height * 4.0F;
    color.a = -1.0F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    sprite.p.x = x;
    sprite.p.y = y;
    sprite.p.z = 0.0F;
    sprite.sx = sx;
    sprite.sy = sy;
    sprite.ang = 0;
    sprite.tlist = &score_back_texlist;
    sprite.tanim = back_texanims;
    njDrawSprite2D(&sprite, 0, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.x = right;
    sprite.p.y = bottom;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    njDrawSprite2D(&sprite, 1, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.x = left;
    sprite.ang = NJM_DEG_ANG(90);
    njDrawSprite2D(&sprite, 1, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.y = top;
    sprite.ang = NJM_DEG_ANG(180);
    njDrawSprite2D(&sprite, 1, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.x = right;
    sprite.ang = NJM_DEG_ANG(270);
    njDrawSprite2D(&sprite, 1, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.x = x;
    sprite.p.y = bottom;
    sprite.sx = width;
    sprite.ang = 0;
    njDrawSprite2D(&sprite, 2, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.x = left;
    sprite.p.y = y;
    sprite.sx = height;
    sprite.ang = NJM_DEG_ANG(90);
    njDrawSprite2D(&sprite, 2, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.x = x;
    sprite.p.y = top;
    sprite.sx = width;
    sprite.ang = NJM_DEG_ANG(180);
    njDrawSprite2D(&sprite, 2, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    sprite.p.x = right;
    sprite.p.y = y;
    sprite.sx = height;
    sprite.ang = NJM_DEG_ANG(270);
    njDrawSprite2D(&sprite, 2, -1.5F, NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    color.a = color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
}

/* the index of the character's records of stage in a save's stage
 * records, such as stage_scores. stage must have its bit in
 * record_stages[character] */
static int save_stage_index(int character, int stage)
{
    int i;
    int count = 0;
    Uint16 bits;

    for (i = 0; i < character + 1; i++) {
        if (character == i) {
            bits = 0xffff >> (15 - stage);
            bits &= record_stages[i];
        } else {
            bits = record_stages[i];
        }
        while (bits) {
            if (bits & 1) {
                count++;
            }
            bits = bits >> 1;
        }
    }
    count--;
    return count;
}

/* whether the character cleared mission on an action stage. C is the
 * clear itself. B is 50 rings or more, or for Knuckles no hint taken, or
 * for Big a fish over 1000. A is a time in mission_times, or for Big a fish
 * over 2000. Tails' A is his race, which he has won if this runs. Eggman and
 * Tikal take Sonic's checks */
static Bool mission_check_action(int character, int stage, int mission)
{
    int time;
    int result;

    switch (mission) {
    case MISSION_A:
        time = TIME(time_minutes, time_seconds) + time_frames;
        /* @hack spelling: each of the four reads of mission_times stored
         * in result before its test, which builds the read's address in R4,
         * result's register, and loads through it (0x8c090140 to
         * 0x8c090146); tested in place, the stage's offset and the table's
         * address take R13 and R12, which the function then saves, and 94
         * of 120 words differ */
        switch (character) {
        /* @hack spelling: default ahead of CHARACTER_SONIC, so that the chain
         * of tests, matching no other character, runs on into Sonic's case
         * (0x8c090140). with default after it, the chain ends in a BRA to
         * that case, hud.c's code runs 96 bytes longer and 77 of 120 words
         * differ */
        default:
        case CHARACTER_SONIC:
            result = mission_times[TIMES_SONIC][stage];
            if (time < result) {
                result = TRUE;
            } else {
                result = FALSE;
            }
            break;
        case CHARACTER_TAILS:
            result = TRUE;
            break;
        case CHARACTER_KNUCKLES:
            result = mission_times[TIMES_KNUCKLES][stage];
            if (time < result) {
                result = TRUE;
            } else {
                result = FALSE;
            }
            break;
        case CHARACTER_AMY:
            result = mission_times[TIMES_AMY][stage];
            if (time < result) {
                result = TRUE;
            } else {
                result = FALSE;
            }
            break;
        case CHARACTER_E102:
            result = mission_times[TIMES_E102][stage];
            if (time < result) {
                result = FALSE;
            } else {
                result = TRUE;
            }
            break;
        case CHARACTER_BIG:
            if (biggest_fish > 2000) {
                result = TRUE;
            } else {
                result = FALSE;
            }
            break;
        }
        break;
    case MISSION_B:
        switch (character) {
        default:
            if (rings < 50) {
                result = FALSE;
            } else {
                result = TRUE;
            }
            break;
        case CHARACTER_BIG:
            if (biggest_fish > 1000) {
                result = TRUE;
            } else {
                result = FALSE;
            }
            break;
        case CHARACTER_KNUCKLES:
            if (hints_used != 0) {
                result = FALSE;
            } else {
                result = TRUE;
            }
            break;
        }
        break;
    case MISSION_C:
        result = TRUE;
        break;
    default:
        result = FALSE;
        break;
    }
    return result;
}

/* whether the character cleared mission on a minigame. the boards and the
 * cart have checks of their own. elsewhere B is any score. A is a score of
 * at least 3000 on the Hedgehog Hammer, 8000 and 20000 on Sky Chase's two
 * acts, and 9000 on any other */
static Bool mission_check(int character, int stage, int mission)
{
    int needed;
    Bool result;

    switch (stage) {
    case STAGE_ICECAP:
        if (game_mode != GAME_MODE_TRIAL) {
            break;
        }
        /* fall through */
    case STAGE_SUNABOARD:
        return mission_check_board(character, mission);
        /* @hack redundant: a break after a return, which SHC writes no jump
         * for. it has the cart's case below load mission_check_cart's address
         * into R3 for its JMP (0x8c090240, 0x8c090244); without it, R2
         * holds the address, and 5 of 65 words differ */
        break;
    case STAGE_CART:
        return mission_check_cart(character, mission);
    }
    switch (mission) {
    case MISSION_A:
        switch (stage) {
        case STAGE_PRACTICE:
            needed = 3000;
            break;
        case STAGE_SHOOTING1:
            needed = 8000;
            break;
        case STAGE_SHOOTING2:
            needed = 20000;
            break;
        default:
            needed = 9000;
            break;
        }
        break;
    case MISSION_B:
        needed = 0;
        break;
    default:
        needed = 999999;
        break;
    }
    if (needed <= score) {
        result = TRUE;
    } else {
        result = FALSE;
    }
    return result;
}

/* the steps after a tally, in work->step. it picks the easiest mission
 * whose emblem the character hasn't won, from C, or from B in a minigame.
 * outside a minigame, it shows that mission with whether the run cleared
 * it. the character's voice says how the run went, and a mission cleared
 * wins its emblem and shows the count. it ends in SCORE_WAIT */
static void score_mission(struct task *task)
{
    struct mission_work *work = (struct mission_work *)task->work;
    int character;
    int stage;
    int mission;
    Bool cleared;
    Bool record;
    NJS_ARGB color;

    switch (work->step) {
    case STEP_INIT:
        character = get_character();
        stage = stage_get_stage_act() >> 8;
        record = FALSE;
        if (work->record > 0.0F) {
            record = TRUE;
        }
        /* a minigame's missions stop at B */
        switch (stage) {
        case STAGE_PRACTICE:
        case STAGE_CART:
        case STAGE_SHOOTING1:
        case STAGE_SHOOTING2:
        case STAGE_SUNABOARD:
            mission = MISSION_B;
            break;
        default:
            mission = MISSION_C;
            break;
        }
        switch (mission) {
        default:
        case MISSION_C:
            if (emblem_check_mission(&saves[save_slot], character, stage,
                                     mission) == 0) {
                break;
            }
            mission--;
            /* fall through */
        case MISSION_B:
            if (emblem_check_mission(&saves[save_slot], character, stage,
                                     mission) == 0) {
                break;
            }
            mission--;
            /* fall through */
        case MISSION_A:
            if (emblem_check_mission(&saves[save_slot], character, stage,
                                     mission) != 0) {
                mission--;
            }
        }
        work->wait = mission;
        if (mission < 0) {
            work->step = STEP_END;
            mission = 0;
        } else {
            work->step = STEP_SHOW;
            work->timer = 0;
            mission_card_create_score();
            /* with a mission shown, the voice says whether the run cleared
             * it, not that it set a record */
            record = FALSE;
        }
        switch (stage) {
        case STAGE_PRACTICE:
        case STAGE_CART:
        case STAGE_SHOOTING1:
        case STAGE_SHOOTING2:
        case STAGE_SUNABOARD:
            cleared = mission_check(character, stage, mission);
            break;
        default:
            cleared = mission_check_action(character, stage, mission);
            break;
        }
        /* the voice says how the run went: a record, a mission cleared, or
         * one failed. Sky Chase's first act, the cart and the Hedgehog
         * Hammer play none, and E-102 has none */
        if (stage != STAGE_SHOOTING1 && stage != STAGE_CART &&
            stage != STAGE_PRACTICE) {
            /* @hack spelling: record compared with TRUE, CMP/EQ #1
             * (0x8c090496). record is set only to FALSE or TRUE above; as a
             * plain test, 3 of 498 words differ */
            if (record == TRUE) {
                switch (character) {
                case CHARACTER_SONIC:
                    sound_play(VOICE_SONIC_RECORD, 0, 0, 0);
                    break;
                case CHARACTER_TAILS:
                    sound_play(VOICE_TAILS_RECORD, 0, 0, 0);
                    break;
                case CHARACTER_KNUCKLES:
                    sound_play(VOICE_KNUCKLES_RECORD, 0, 0, 0);
                    break;
                case CHARACTER_AMY:
                    sound_play(VOICE_AMY_RECORD, 0, 0, 0);
                    break;
                case CHARACTER_E102:
                    break;
                case CHARACTER_BIG:
                    sound_play(VOICE_BIG_RECORD, 0, 0, 0);
                    break;
                }
                /* @hack spelling: cleared compared with TRUE, CMP/EQ #1
                 * (0x8c090562). the mission checks return only TRUE or
                 * FALSE; as a plain test, 35 of 498 words differ */
            } else if (cleared == TRUE) {
                switch (character) {
                case CHARACTER_SONIC:
                    sound_play(VOICE_SONIC_CLEARED, 0, 0, 0);
                    break;
                case CHARACTER_TAILS:
                    sound_play(VOICE_TAILS_CLEARED, 0, 0, 0);
                    break;
                case CHARACTER_KNUCKLES:
                    sound_play(VOICE_KNUCKLES_CLEARED, 0, 0, 0);
                    break;
                case CHARACTER_AMY:
                    sound_play(VOICE_AMY_CLEARED, 0, 0, 0);
                    break;
                case CHARACTER_E102:
                    break;
                case CHARACTER_BIG:
                    sound_play(VOICE_BIG_CLEARED, 0, 0, 0);
                    break;
                }
            } else {
                switch (character) {
                case CHARACTER_SONIC:
                    sound_play(VOICE_SONIC_FAILED, 0, 0, 0);
                    break;
                case CHARACTER_TAILS:
                    sound_play(VOICE_TAILS_FAILED, 0, 0, 0);
                    break;
                case CHARACTER_KNUCKLES:
                    sound_play(VOICE_KNUCKLES_FAILED, 0, 0, 0);
                    break;
                case CHARACTER_AMY:
                    sound_play(VOICE_AMY_FAILED, 0, 0, 0);
                    break;
                case CHARACTER_E102:
                    break;
                case CHARACTER_BIG:
                    sound_play(VOICE_BIG_FAILED, 0, 0, 0);
                    break;
                }
            }
        }
        /* @hack spelling: cleared compared with TRUE, CMP/EQ #1
         * (0x8c09068c). the mission checks return only TRUE or FALSE; as a
         * plain test, 22 of 498 words differ */
        /* @bug logic: mission >= 0 always holds, since a run with no
         * mission left sets mission to 0 above (CMP/PZ R13 at 0x8c090690).
         * such a run sets mission A's emblem again, which it has won */
        if (cleared == TRUE && mission >= 0) {
            emblem_set_mission(&saves[save_slot], character, stage, mission);
        }
        work->cleared = cleared;
        work->timer = 0;
        break;
    case STEP_SHOW:
        cleared = work->cleared;
        switch (stage_get_stage_act() >> 8) {
        case STAGE_PRACTICE:
        case STAGE_CART:
        case STAGE_SHOOTING1:
        case STAGE_SHOOTING2:
        case STAGE_SUNABOARD:
            work->timer = 150;
            break;
        default:
            mission = work->wait;
            color.a = -0.5F;
            color.r = color.g = color.b = 1.0F;
            njSetConstantMaterial(&color);
            njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
            njColorBlendingMode(NJD_DESTINATION_COLOR,
                                NJD_COLOR_BLENDING_INVSRCALPHA);
            njDrawSprite2D(&mission_sprites[0], 0, -1.15F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
            njDrawSprite2D(&mission_sprites[1], mission + 1, -1.15F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
            njDrawSprite2D(&mission_sprites[2], cleared + 4, -1.15F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
            color.a = 0.0F;
            color.r = color.g = color.b = 0.0F;
            njSetConstantMaterial(&color);
            score_draw_back(SCREEN_MIDDLE_X, SCREEN_MIDDLE_Y, 10.0F, 7.0F);
            break;
        }
        /* @hack type: 150.0F, a float, which converts the timer and
         * compares it in FR3 (FLOAT FPUL,FR3 at 0x8c0907da, FCMP/GT at
         * 0x8c0907dc); against 150, 241 of 498 words differ */
        if (++work->timer > 150.0F) {
            switch (cleared) {
            default:
            case FALSE:
                work->step = STEP_END;
                break;
            case TRUE:
                work->step = STEP_EMBLEM;
                work->timer = 0;
                stream_destroy();
                emblem_start_count_display();
                stream_reset();
                break;
            }
        }
        break;
    case STEP_EMBLEM:
        /* @hack type: 30.0F, a float, which converts the timer and compares
         * it in FR3 (FLOAT FPUL,FR3 at 0x8c09081a, FCMP/GT at 0x8c09081c);
         * against 30, hud.c's code runs 32 bytes shorter and 66 of 498
         * words differ */
        if (++work->timer > 30.0F) {
            work->step = STEP_WAIT;
            work->timer = 0;
        }
        break;
    case STEP_WAIT:
        if (++work->timer > 60) {
            work->step = STEP_END;
            work->timer = 0;
        }
        break;
    case STEP_END:
        work->mode = SCORE_WAIT;
        work->wait = 2;
        break;
    default:
        work->mode = SCORE_END;
        break;
    }
}

/* draw the boss trial's screen: a row for each of the character's
 * bosses, and the total's row, with no times */
static void trial_draw(struct task *task)
{
    struct trial_work *work = (struct trial_work *)task->work;
    NJS_ARGB color;

    color.a = -0.5F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    switch (work->character) {
    case CHARACTER_SONIC:
        njDrawSprite2D(&trial_sprites[0], 0, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[1], 1, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[2], 2, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[3], 3, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[4], 4, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        break;
    case CHARACTER_TAILS:
        njDrawSprite2D(&trial_sprites[1], 1, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[2], 2, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[3], 5, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        break;
    case CHARACTER_KNUCKLES:
        njDrawSprite2D(&trial_sprites[1], 6, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[2], 2, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[3], 3, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        break;
    case CHARACTER_AMY:
        njDrawSprite2D(&trial_sprites[2], 7, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        break;
    case CHARACTER_E102:
        njDrawSprite2D(&trial_sprites[1], 8, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        njDrawSprite2D(&trial_sprites[3], 9, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        break;
    case CHARACTER_BIG:
        njDrawSprite2D(&trial_sprites[2], 3, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        break;
    }
    njDrawSprite2D(&trial_sprites[5], 10, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    color.a = 0.0F;
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
    score_draw_back(SCREEN_MIDDLE_X, 224.0F, 15.0F, 6.0F);
    score_draw_back(SCREEN_MIDDLE_X, 384.0F, 15.0F, 0.0F);
}

/* each draw below calls an earlier draw, then adds one boss time, the
 * next in trial_times, in its row. each is named for the mode that calls
 * it, and some choose the draw they call, the time and the row by
 * work->character */
static void trial_draw_sonic1(struct task *task)
{
    struct score_time time;

    trial_draw(task);
    time.minutes = trial_times[0].minutes;
    time.seconds = trial_times[0].seconds;
    time.hundredths = trial_times[0].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 128.0F;
    score_draw_time(time);
}

static void trial_draw_knuckles1(struct task *task)
{
    struct score_time time;

    trial_draw(task);
    time.minutes = trial_times[0].minutes;
    time.seconds = trial_times[0].seconds;
    time.hundredths = trial_times[0].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 176.0F;
    score_draw_time(time);
}

static void trial_draw_sonic2(struct task *task)
{
    struct trial_work *work = (struct trial_work *)task->work;
    struct score_time time;
    int i;

    if (work->character == CHARACTER_TAILS) {
        trial_draw(task);
        i = 0;
    } else {
        trial_draw_sonic1(task);
        i = 1;
    }
    time.minutes = trial_times[i].minutes;
    time.seconds = trial_times[i].seconds;
    time.hundredths = trial_times[i].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 176.0F;
    score_draw_time(time);
}

static void trial_draw_sonic3(struct task *task)
{
    struct trial_work *work = (struct trial_work *)task->work;
    struct score_time time;
    int i;

    switch (work->character) {
    case CHARACTER_TAILS:
        trial_draw_sonic2(task);
        i = 1;
        break;
    case CHARACTER_KNUCKLES:
        trial_draw_knuckles1(task);
        i = 1;
        break;
    default:
        trial_draw_sonic2(task);
        i = 2;
        break;
    }
    time.minutes = trial_times[i].minutes;
    time.seconds = trial_times[i].seconds;
    time.hundredths = trial_times[i].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 224.0F;
    score_draw_time(time);
}

static void trial_draw_sonic4(struct task *task)
{
    struct trial_work *work = (struct trial_work *)task->work;
    struct score_time time;
    int i;

    switch (work->character) {
    case CHARACTER_KNUCKLES:
        trial_draw_sonic3(task);
        i = 2;
        time.y = 272.0F;
        break;
    case CHARACTER_BIG:
        trial_draw(task);
        i = 0;
        time.y = 224.0F;
        break;
    default:
        trial_draw_sonic3(task);
        i = 3;
        time.y = 272.0F;
        break;
    }
    time.minutes = trial_times[i].minutes;
    time.seconds = trial_times[i].seconds;
    time.hundredths = trial_times[i].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    score_draw_time(time);
}

static void trial_draw_sonic5(struct task *task)
{
    struct score_time time;

    trial_draw_sonic4(task);
    time.minutes = trial_times[4].minutes;
    time.seconds = trial_times[4].seconds;
    time.hundredths = trial_times[4].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 320.0F;
    score_draw_time(time);
}

static void trial_draw_tails3(struct task *task)
{
    struct score_time time;

    trial_draw_sonic3(task);
    time.minutes = trial_times[2].minutes;
    time.seconds = trial_times[2].seconds;
    time.hundredths = trial_times[2].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 272.0F;
    score_draw_time(time);
}

static void trial_draw_amy1(struct task *task)
{
    struct score_time time;

    trial_draw(task);
    time.minutes = trial_times[0].minutes;
    time.seconds = trial_times[0].seconds;
    time.hundredths = trial_times[0].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 224.0F;
    score_draw_time(time);
}

static void trial_draw_e102_1(struct task *task)
{
    struct score_time time;

    trial_draw(task);
    time.minutes = trial_times[0].minutes;
    time.seconds = trial_times[0].seconds;
    time.hundredths = trial_times[0].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 176.0F;
    score_draw_time(time);
}

static void trial_draw_e102_2(struct task *task)
{
    struct score_time time;

    trial_draw_e102_1(task);
    time.minutes = trial_times[1].minutes;
    time.seconds = trial_times[1].seconds;
    time.hundredths = trial_times[1].frames * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 272.0F;
    score_draw_time(time);
}

/* draw every boss time, and the total under them */
static void trial_draw_total(struct task *task)
{
    struct trial_work *work = (struct trial_work *)task->work;
    struct score_time time;
    int rest;

    switch (work->character) {
    case CHARACTER_SONIC:
        trial_draw_sonic5(task);
        break;
    case CHARACTER_TAILS:
        trial_draw_tails3(task);
        break;
    case CHARACTER_KNUCKLES:
    case CHARACTER_BIG:
        trial_draw_sonic4(task);
        break;
    case CHARACTER_AMY:
        trial_draw_amy1(task);
        break;
    case CHARACTER_E102:
        trial_draw_e102_2(task);
    }
    time.minutes = work->total / 3600;
    rest = work->total % 3600;
    time.seconds = rest / 60;
    time.hundredths = rest % 60 * (100.0F / 60.0F);
    time.x = 288.0F;
    time.y = 384.0F;
    score_draw_time(time);
}

/* the boss trial's score screen, after each boss. it keeps the boss's
 * time in trial_times, and until the character's last boss it ends there.
 * after the last, the times' total takes its place among the save's best
 * totals for the character, least first, or for E-102 most left first. the
 * screen then shows each time and the total */
static void score_trial_step(struct task *task)
{
    struct trial_work *work = (struct trial_work *)task->work;
    struct save *save;
    struct save_times *best;
    struct save_time *time;
    int times[SAVE_BEST + 1]; /* the save's best, then this run's */
    int index;
    int count;
    int rest;
    int swap;
    int i;

    if ((per_info[0]->on & SKIP_BUTTONS) && work->wait > SKIP_WAIT &&
        work->mode > TRIAL_SAVE && work->mode < SCORE_WAIT) {
        work->wait = SKIP_WAIT;
    }
    switch (work->mode) {
    case TRIAL_INIT:
        /* NOLINTBEGIN(bugprone-signed-char-misuse) */
        trial_times[trial_count].minutes = time_minutes;
        trial_times[trial_count].seconds = time_seconds;
        trial_times[trial_count].frames = time_frames;
        /* NOLINTEND(bugprone-signed-char-misuse) */
        trial_count++;
        work->wait = 180;
        work->mode = TRIAL_WAIT;
        work->character = get_character();
        vmu_request(VMU_REQUEST_CLEAR);
        stream_destroy();
        texture_load_pvm("SCORE_BACK", &score_back_texlist);
        texture_load_pvm("SCORE_BOSS", &score_boss_texlist);
        stream_reset();
        break;
    case TRIAL_WAIT:
        if (work->wait-- > 0) {
            break;
        }
        /* how many bosses each character fights */
        switch (work->character) {
        case CHARACTER_SONIC:
            count = 5;
            break;
        case CHARACTER_TAILS:
        case CHARACTER_KNUCKLES:
            count = 3;
            break;
        case CHARACTER_E102:
            count = 2;
            break;
        case CHARACTER_AMY:
        case CHARACTER_BIG:
            count = 1;
            break;
        default:
            count = 0;
            /* the last case needs no break. one here would load trial_count
             * into R2 (0x8c0913f2), not R3, and 2 of 731 words would
             * differ */
        }
        if (trial_count < count) {
            work->mode = SCORE_END;
            break;
        }
        stream_request_music(MUSIC_RNDCLEAR);
        work->mode = TRIAL_SAVE;
        work->wait = 60;
        break;
    case TRIAL_SAVE:
        work->mode = TRIAL_BOSSES;
        work->wait = 60;
        times[SAVE_BEST] = 0;
        for (i = 0; i < TRIAL_TIMES; i++) {
            times[SAVE_BEST] +=
                TIME(trial_times[i].minutes, trial_times[i].seconds) +
                trial_times[i].frames;
        }
        work->unk_10 = work->total = times[SAVE_BEST];
        /* the save's trial times skip Eggman and Tikal, who have no trial */
        index = work->character; /* NOLINT(bugprone-signed-char-misuse) */
        save = &saves[save_slot];
        if (work->character >= CHARACTER_TAILS) {
            index--;
        }
        if (work->character >= CHARACTER_AMY) {
            index--;
        }
        best = &save->trial_times[index];
        for (i = 0; i < SAVE_BEST; i++) {
            time = &best->time[i];
            times[i] = TIME(time->minutes, time->seconds) + time->frames;
        }
        for (i = SAVE_BEST; i > 0; i--) {
            if (work->character == CHARACTER_E102) {
                if (times[i - 1] < times[i]) {
                    swap = times[i - 1];
                    times[i - 1] = times[i];
                    times[i] = swap;
                }
            } else {
                if (times[i - 1] > times[i]) {
                    swap = times[i - 1];
                    times[i - 1] = times[i];
                    times[i] = swap;
                }
            }
        }
        for (i = 0; i < SAVE_BEST; i++) {
            time = &best->time[i];
            time->minutes = times[i] / 3600;
            rest = times[i] % 3600;
            time->seconds = rest / 60;
            time->frames = rest % 60;
        }
        break;
    case TRIAL_BOSSES:
        if (work->wait-- <= 0) {
            switch (work->character) {
            default:
            case CHARACTER_SONIC:
                work->mode = TRIAL_SONIC1;
                break;
            case CHARACTER_TAILS:
                work->mode = TRIAL_SONIC2;
                break;
            case CHARACTER_KNUCKLES:
                work->mode = TRIAL_KNUCKLES1;
                break;
            case CHARACTER_AMY:
                work->mode = TRIAL_AMY1;
                break;
            case CHARACTER_E102:
                work->mode = TRIAL_E102_1;
                break;
            case CHARACTER_BIG:
                work->mode = TRIAL_SONIC4;
                break;
            }
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw(task);
        break;
    case TRIAL_SONIC1:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_SONIC2;
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_sonic1(task);
        break;
    case TRIAL_SONIC2:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_SONIC3;
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_sonic2(task);
        break;
    case TRIAL_SONIC3:
        if (work->wait-- <= 0) {
            switch (work->character) {
            default:
            case CHARACTER_SONIC:
            case CHARACTER_KNUCKLES:
                work->mode = TRIAL_SONIC4;
                break;
            case CHARACTER_TAILS:
                work->mode = TRIAL_TAILS3;
                break;
            }
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_sonic3(task);
        break;
    case TRIAL_SONIC4:
        if (work->wait-- <= 0) {
            switch (work->character) {
            default:
                work->mode = TRIAL_SONIC5;
                work->wait = 30;
                break;
            case CHARACTER_KNUCKLES:
            case CHARACTER_BIG:
                work->mode = TRIAL_TOTAL;
                work->wait = 60;
                break;
            }
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_sonic4(task);
        break;
    case TRIAL_SONIC5:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_TOTAL;
            work->wait = 60;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_sonic5(task);
        break;
    case TRIAL_TAILS3:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_TOTAL;
            work->wait = 60;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_tails3(task);
        break;
    case TRIAL_KNUCKLES1:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_SONIC3;
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_knuckles1(task);
        break;
    case TRIAL_AMY1:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_TOTAL;
            work->wait = 60;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_amy1(task);
        break;
    case TRIAL_E102_1:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_E102_2;
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_e102_1(task);
        break;
    case TRIAL_E102_2:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_TOTAL;
            work->wait = 60;
            sound_play(SE_TICK, 0, 0, 0);
        }
        trial_draw_e102_2(task);
        break;
    case TRIAL_TOTAL:
        if (work->wait-- <= 0) {
            work->mode = TRIAL_HOLD;
            work->wait = 60;
        }
        trial_draw_total(task);
        break;
    case TRIAL_HOLD:
        trial_draw_total(task);
        /* fall through */
    case SCORE_WAIT:
        if (work->wait-- <= 0) {
            work->mode = SCORE_END;
        }
        break;
    case SCORE_END:
        njReleaseTexture(&score_back_texlist);
        njReleaseTexture(&score_boss_texlist);
        stage_end(STAGE_END_CLEAR);
        task_kill(task);
        break;
    }
}

/* draw the Hedgehog Hammer's score screen. its rows are the points from
 * the 100s and the 500s hit, what hits on Eggman took, and the score */
static void mole_draw(struct task *task)
{
    NJS_ARGB color;

    color.a = -0.5F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawSprite2D(&mole_sprites[0], 0, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&mole_sprites[1], 1, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&mole_sprites[2], 2, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&mole_sprites[3], 3, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    color.a = 0.0F;
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
    score_draw_back(SCREEN_MIDDLE_X, 224.0F, 15.0F, 3.0F);
    score_draw_back(SCREEN_MIDDLE_X, 336.0F, 15.0F, 0.0F);
}

/* each draw below calls the one above it, then draws one row's number */
static void mole_draw_bonus1(struct task *task)
{
    mole_draw(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus1;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 176.0F;
    hud_draw_number(&score_number);
}

static void mole_draw_bonus2(struct task *task)
{
    mole_draw_bonus1(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus2;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 224.0F;
    hud_draw_number(&score_number);
}

static void mole_draw_bonus3(struct task *task)
{
    mole_draw_bonus2(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus3;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 272.0F;
    score_number.color = 0xffff0000;
    hud_draw_number(&score_number);
    score_number.color = 0xffffffff;
}

static void mole_draw_score(struct task *task)
{
    mole_draw_bonus3(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->score;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 336.0F;
    hud_draw_number(&score_number);
}

/* the Hedgehog Hammer's tally: the 100s' and the 500s' points count up
 * into the score, and what hits on Eggman took counts back down */
static void score_mole_step(struct task *task)
{
    struct tally_work *work = (struct tally_work *)task->work;

    /* a skip button counts the rest in at once, and moves on to the
     * missions */
    if ((per_info[0]->on & SKIP_BUTTONS) && work->wait > SKIP_WAIT &&
        work->mode > MOLE_WAIT && work->mode < MOLE_MISSION) {
        work->mode = MOLE_MISSION;
        work->wait = SKIP_WAIT;
        work->score += work->total;
    }
    switch (work->mode) {
    case MOLE_INIT:
        work->wait = 120;
        /* the points of the 100s and the 500s hit, and what the score lost
         * to hits on Eggman */
        work->bonus1 = mole_hits[KIND_100].points * mole_hits[KIND_100].count;
        work->bonus2 = mole_hits[KIND_500].points * mole_hits[KIND_500].count;
        work->total = score;
        /* @hack spelling: the difference times -1.0F, which loads 1 with
         * FLDI1, multiplies by it and negates (0x8c091cc8 to 0x8c091ccc).
         * a product with -1.0F only flips the sign; as -(total - bonus1 -
         * bonus2), 108 of 406 words differ */
        work->bonus3 = (work->total - work->bonus1 - work->bonus2) * -1.0F;
        work->score = 0.0F;
        work->mode = MOLE_WAIT;
        stream_destroy();
        texture_load_pvm("SCORE_BACK", &score_back_texlist);
        texture_load_pvm("SCORE_MOLE", &score_mole_texlist);
        stream_reset();
        vmu_request(VMU_REQUEST_CLEAR);
        break;
    case MOLE_WAIT:
        if (work->wait-- <= 0) {
            work->mode = MOLE_BACK;
            work->wait = 60;
        }
        break;
    case MOLE_BACK:
        if (work->wait-- <= 0) {
            work->mode = MOLE_BONUS1;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        mole_draw(task);
        break;
    case MOLE_BONUS1:
        if (work->wait-- <= 0) {
            work->mode = MOLE_BONUS2;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        mole_draw_bonus1(task);
        break;
    case MOLE_BONUS2:
        if (work->wait-- <= 0) {
            work->mode = MOLE_BONUS3;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        mole_draw_bonus2(task);
        break;
    case MOLE_BONUS3:
        if (work->wait-- <= 0) {
            work->mode = MOLE_COUNT;
            work->wait = 180;
            work->score = 0.0F;
        }
        mole_draw_bonus3(task);
        break;
    case MOLE_COUNT:
        if (work->total > 0.0F || work->bonus3 > 0.0F) {
            /* a step moves points from the first bonus left. the loop's test
             * of the skip buttons never holds: the skip test above reads them
             * first, and ends the count */
            do {
                if (work->bonus1 != 0.0F) {
                    if (work->bonus1 < TALLY_STEP) {
                        work->score += work->bonus1;
                        work->total -= work->bonus1;
                        work->bonus1 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_STEP;
                        work->total -= TALLY_STEP;
                        work->bonus1 -= TALLY_STEP;
                    }
                } else if (work->bonus2 != 0.0F) {
                    if (work->bonus2 < TALLY_STEP) {
                        work->score += work->bonus2;
                        work->total -= work->bonus2;
                        work->bonus2 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_STEP;
                        work->total -= TALLY_STEP;
                        work->bonus2 -= TALLY_STEP;
                    }
                } else if (work->bonus3 != 0.0F) {
                    if (work->bonus3 < TALLY_STEP) {
                        work->score -= work->bonus3;
                        work->total += work->bonus3;
                        work->bonus3 = 0.0F;
                        break;
                    } else {
                        work->score -= TALLY_STEP;
                        work->total += TALLY_STEP;
                        work->bonus3 -= TALLY_STEP;
                    }
                } else {
                    work->wait = 60;
                    work->mode = MOLE_MISSION;
                    break;
                }
            } while (per_info[0]->on & SKIP_BUTTONS);
            /* @bug logic: the tick waits for wait to reach a multiple of
             * 8, but the count holds wait at 180, and its end sets 60. the
             * tally never ticks (TST R1,R5 at 0x8c091f48) */
            if (!(work->wait & 7)) {
                sound_play(SE_TICK, 0, 0, 0);
            }
        } else if (work->wait-- <= 0) {
            work->wait = 60;
            work->mode = MOLE_MISSION;
            work->step = STEP_INIT;
        }
        mole_draw_score(task);
        break;
    case MOLE_MISSION:
        score_mission(task);
        break;
    case SCORE_WAIT:
        if (work->wait-- <= 0) {
            work->mode = SCORE_END;
        }
        break;
    case SCORE_END:
        mole_score_done();
        njReleaseTexture(&score_back_texlist);
        njReleaseTexture(&score_mole_texlist);
        task_kill(task);
        break;
    }
}

/* draw an act's score screen. its rows are the time and rings, the points
 * bonus, the time bonus or Big's fish, the rings bonus and the score */
static void act_draw(struct task *task)
{
    NJS_ARGB color;

    color.a = -0.5F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawSprite2D(&act_sprites[0], 0, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&act_sprites[1], 1, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&act_sprites[2], 2, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    if (get_character() == CHARACTER_BIG) {
        njDrawSprite2D(&act_sprites[6], 6, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    } else {
        njDrawSprite2D(&act_sprites[3], 3, -1.15F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    }
    njDrawSprite2D(&act_sprites[4], 4, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&act_sprites[5], 5, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    color.a = 0.0F;
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
    score_draw_back(SCREEN_MIDDLE_X, 128.0F, 15.0F, 1.5F);
    score_draw_back(SCREEN_MIDDLE_X, 272.0F, 15.0F, 3.0F);
    score_draw_back(SCREEN_MIDDLE_X, 384.0F, 15.0F, 0.0F);
}

/* each draw below calls an earlier one, then adds its row's number.
 * act_draw_rings adds the clock too, the row ACT_TIME draws in its own
 * case */
static void act_draw_rings(struct task *task)
{
    act_draw(task);
    score_draw_clock(288.0F, 96.0F);
    score_number.max = 1000000;
    score_number.value = rings;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 156.0F;
    hud_draw_number(&score_number);
}

static void act_draw_bonus1(struct task *task)
{
    act_draw_rings(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus1;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 228.0F;
    hud_draw_number(&score_number);
}

static void act_draw_bonus2(struct task *task)
{
    act_draw_bonus1(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus2;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 274.0F;
    hud_draw_number(&score_number);
}

/* act_draw_bonus2 again, for Big's screen, where the row is his fish */
static void act_draw_bonus2_big(struct task *task)
{
    act_draw_bonus1(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus2;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 274.0F;
    hud_draw_number(&score_number);
}

static void act_draw_bonus3(struct task *task)
{
    if (get_character() == CHARACTER_BIG) {
        act_draw_bonus2_big(task);
    } else {
        act_draw_bonus2(task);
    }
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus3;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 320.0F;
    hud_draw_number(&score_number);
}

static void act_draw_score(struct task *task)
{
    act_draw_bonus3(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->score;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 384.0F;
    hud_draw_number(&score_number);
}

/* @hack spelling: bonus_points as a helper SHC expands, whose result it
 * copies from FR3 to FR4 for the store (FMOV FR3,FR4 at 0x8c092486);
 * written in place, 94 of score_act_step's 819 words differ */
static Float points_bonus(void)
{
    return bonus_points;
}

/* @hack spelling: the rings bonus as a helper SHC expands, which multiplies
 * into FR4, not the FR3 the conversion leaves (FMUL FR2,FR4 at 0x8c0925dc
 * and 0x8c092f8a); written in place, 286 of the 1319 words of
 * score_act_step and score_board_step differ */
static Float rings_bonus(void)
{
    return rings * 10.0F;
}

/* an act's tally: the points bonus, the time bonus or Big's fish and the
 * rings bonus count into the score, then the act's own points. an action
 * stage saves the run's score, time and rings, each where it beats the
 * save's best */
static void score_act_step(struct task *task)
{
    struct tally_work *work = (struct tally_work *)task->work;
    struct save *save;
    struct save_time *best;
    int stage;
    int character;
    int index;
    int saved;
    int current;
    int old;

    if ((per_info[0]->on & SKIP_BUTTONS) && work->wait > SKIP_WAIT &&
        work->mode > ACT_SAVE && work->mode < ACT_MISSION) {
        work->wait = SKIP_WAIT;
        work->score += work->total;
        work->total = 0.0F;
        work->mode = ACT_MISSION;
    }
    switch (work->mode) {
    case ACT_INIT:
        work->wait = 180;
        work->bonus1 = points_bonus();
        stage = stage_get_stage_act() >> 8;
        if (stage > STAGE_ACTION_LAST && stage <= STAGE_BOSS_LAST) {
            work->bonus1 += BOSS_BONUS;
        }
        /* on the stages below, while Sonic or Tails lacks mission C's
         * emblem there, each run's time gets ten seconds more. score_mission
         * wins that emblem after this, so the run that first clears the
         * stage gets them too. the time bonus, the clock and the best time
         * saved all count them */
        switch (get_character()) {
        case CHARACTER_BIG:
            work->bonus2 = fishing_score + 500.0F;
            break;
        case CHARACTER_SONIC:
            switch (stage) {
            case STAGE_BEACH:
            case STAGE_WINDY:
            case STAGE_RUIN:
            case STAGE_ICECAP:
            case STAGE_CASINO:
            case STAGE_FINAL:
                if (emblem_check_mission(&saves[save_slot], CHARACTER_SONIC,
                                         stage, MISSION_C) == 0) {
                    time_add_seconds(10);
                }
                /* a break here would add a jump to a shared call of
                 * time_bonus, and 40 of 819 words would differ */
            }
            work->bonus2 = time_bonus();
            break;
        case CHARACTER_TAILS:
            switch (stage) {
            case STAGE_WINDY:
            case STAGE_ICECAP:
            case STAGE_CASINO:
                if (emblem_check_mission(&saves[save_slot], CHARACTER_TAILS,
                                         stage, MISSION_C) == 0) {
                    time_add_seconds(10);
                }
            }
            work->bonus2 = time_bonus();
            break;
        default:
            work->bonus2 = time_bonus();
            break;
        }
        work->bonus3 = rings_bonus();
        work->total = work->bonus1 + work->bonus2 + work->bonus3 + score;
        /* @hack type: the total converted as a Uint32, which tests it
         * against 2^31 before its FTRC (FCMP/GT at 0x8c092602, FTRC at
         * 0x8c09260a); converted as an int, 92 of 819 words differ */
        score = (Uint32)work->total;
        work->score = 0.0F;
        work->mode = ACT_WAIT;
        stream_destroy();
        texture_load_pvm("SCORE_BACK", &score_back_texlist);
        texture_load_pvm("SCORE_ACT", &score_act_texlist);
        texture_load_pvm("SCORE_RESULT", &score_result_texlist);
        stream_reset();
        vmu_request(VMU_REQUEST_CLEAR);
        break;
    case ACT_WAIT:
        if (work->wait-- <= 0) {
            /* Tails lost his race: the stage ends with STAGE_END_RETRY,
             * with no score and no mission */
            if (get_character() == CHARACTER_TAILS &&
                race_outcome() != RACE_WON) {
                /* @hack redundant: a call of stage_get_stage_act, which has
                 * no effect, and whose result nothing reads (JSR at
                 * 0x8c092678). the call shapes the registers from the
                 * function's start, where per_info's address takes R7
                 * (0x8c0923cc); without it, R3 takes it and 254 of 819 words
                 * differ */
                stage_get_stage_act();
                njReleaseTexture(&score_back_texlist);
                njReleaseTexture(&score_act_texlist);
                njReleaseTexture(&score_result_texlist);
                stage_end(STAGE_END_RETRY);
                task_kill(task);
                /* @hack spelling: a return where a break would do, which gives
                 * this arm an exit of its own, ending in a JMP to task_kill
                 * (0x8c092694 to 0x8c0926aa); as a break, the arm shares
                 * SCORE_END's exit, hud.c's code runs 32 bytes shorter and
                 * 560 of 819 words differ */
                return;
            } else {
                work->mode = ACT_SAVE;
                work->wait = 60;
            }
        }
        break;
    case ACT_SAVE:
        work->mode = ACT_BACK;
        stage = stage_get_stage_act() >> 8;
        /* a shorter time beats the saved one, or for E-102 more time left.
         * the save keeps no time of Big's */
        if (stage != STAGE_PRACTICE && stage <= STAGE_ACTION_LAST) {
            save = &saves[save_slot];
            character = get_character();
            index = save_stage_index(character, stage);
            old = save->stage_scores[index];
            if (old < score) {
                work->record = 1.0F;
                save->stage_scores[index] = score;
            } else {
                work->record = 0.0F;
            }
            if (character != CHARACTER_BIG) {
                best = &save->stage_times[index];
                saved = TIME(best->minutes, best->seconds) + best->frames;
                current = TIME(time_minutes, time_seconds) + time_frames;
                if (character == CHARACTER_E102) {
                    if (saved < current) {
                        best->minutes = time_minutes;
                        best->seconds = time_seconds;
                        best->frames = time_frames;
                    }
                } else if (saved > current) {
                    best->minutes = time_minutes;
                    best->seconds = time_seconds;
                    best->frames = time_frames;
                }
            }
            old = save->stage_rings[index];
            if (old < rings) {
                save->stage_rings[index] = rings;
            }
            /* @hack redundant: a call of score_draw_clock and a break that
             * never run. SHC drops the block, but it still shapes the step's
             * registers: sound_play's address takes R10, score's R11 and
             * njReleaseTexture's R9 (0x8c092412 to 0x8c09241e), and the rest
             * follow. without it, 45 of 819 words differ */
            if (0) {
                score_draw_clock(SCREEN_MIDDLE_X, 320.0F);
                break;
            }
        }
        break;
    case ACT_BACK:
        if (work->wait-- <= 0) {
            work->mode = ACT_TIME;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        act_draw(task);
        break;
    case ACT_TIME:
        if (work->wait-- <= 0) {
            work->mode = ACT_RINGS;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        /* @hack spelling: the two calls in one comma expression, which has the
         * case call score_draw_clock with a JSR, then branch to the step's
         * shared exit (0x8c0928ac, 0x8c0928b0). as two statements, the case
         * ends in a JMP to score_draw_clock and an exit of its own, hud.c's
         * code runs 32 bytes longer and 368 of 819 words differ */
        act_draw(task), score_draw_clock(288.0F, 96.0F);
        break;
    case ACT_RINGS:
        if (work->wait-- <= 0) {
            work->mode = ACT_BONUS1;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        act_draw_rings(task);
        break;
    case ACT_BONUS1:
        if (work->wait-- <= 0) {
            work->mode = ACT_BONUS2;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        act_draw_bonus1(task);
        break;
    case ACT_BONUS2:
        if (work->wait-- <= 0) {
            work->mode = ACT_BONUS3;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        if (get_character() == CHARACTER_BIG) {
            act_draw_bonus2_big(task);
        } else {
            act_draw_bonus2(task);
        }
        break;
    case ACT_BONUS3:
        if (work->wait-- <= 0) {
            work->mode = ACT_COUNT;
            work->wait = 60;
            work->score = 0.0F;
        }
        act_draw_bonus3(task);
        break;
    case ACT_COUNT:
        /* a step moves points from the first bonus left. once none is
         * left, the rest of total, the act's own points, goes in at once.
         * the loop's test of the skip buttons never holds: the skip test
         * above reads them first, and ends the count */
        if (work->total > 0.0F) {
            do {
                if (work->bonus1 != 0.0F) {
                    if (work->bonus1 < TALLY_STEP) {
                        work->score += work->bonus1;
                        work->total -= work->bonus1;
                        work->bonus1 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_STEP;
                        work->total -= TALLY_STEP;
                        work->bonus1 -= TALLY_STEP;
                    }
                } else if (work->bonus2 != 0.0F) {
                    if (work->bonus2 < TALLY_TIME_STEP) {
                        work->score += work->bonus2;
                        work->total -= work->bonus2;
                        work->bonus2 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_TIME_STEP;
                        work->total -= TALLY_TIME_STEP;
                        work->bonus2 -= TALLY_TIME_STEP;
                    }
                } else if (work->bonus3 != 0.0F) {
                    if (work->bonus3 < TALLY_RINGS_STEP) {
                        work->score += work->bonus3;
                        work->total -= work->bonus3;
                        work->bonus3 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_RINGS_STEP;
                        work->total -= TALLY_RINGS_STEP;
                        work->bonus3 -= TALLY_RINGS_STEP;
                    }
                } else {
                    work->wait = 60;
                    work->mode = ACT_MISSION;
                    work->score += work->total;
                    work->total = 0.0F;
                    break;
                }
            } while (per_info[0]->on & SKIP_BUTTONS);
            /* @bug logic: the beep waits for wait to reach a multiple of
             * 8, but the count holds wait at 60, and its end sets 60 again.
             * the tally never beeps (TST #7,R0 at 0x8c092b18) */
            if (!(work->wait & 7)) {
                sound_play(SE_BEEP, 0, 0, 0);
            }
        } else if (work->wait-- <= 0) {
            work->wait = 60;
            work->mode = ACT_MISSION;
        }
        act_draw_score(task);
        break;
    case ACT_MISSION:
        stage = stage_get_stage_act() >> 8;
        /* a boss has no missions */
        if (stage > STAGE_ACTION_LAST && stage <= STAGE_BOSS_LAST) {
            work->mode = SCORE_WAIT;
        } else {
            score_mission(task);
        }
        break;
    case SCORE_WAIT:
        if (work->wait-- <= 0) {
            work->mode = SCORE_END;
        }
        break;
    case SCORE_END:
        stage_end(STAGE_END_CLEAR);
        njReleaseTexture(&score_back_texlist);
        njReleaseTexture(&score_act_texlist);
        njReleaseTexture(&score_result_texlist);
        task_kill(task);
        break;
    }
}

/* draw a board's score screen: rows for the score, the time bonus, the
 * rings bonus, and the total */
static void board_draw(struct task *task)
{
    NJS_ARGB color;

    color.a = -0.5F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawSprite2D(&board_sprites[0], 0, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&board_sprites[1], 1, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&board_sprites[2], 2, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&board_sprites[3], 3, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    color.a = 0.0F;
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
    score_draw_back(SCREEN_MIDDLE_X, 224.0F, 15.0F, 3.0F);
    score_draw_back(SCREEN_MIDDLE_X, 336.0F, 15.0F, 0.0F);
}

/* each draw below calls the one above it, then draws one row's number */
static void board_draw_bonus1(struct task *task)
{
    board_draw(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus1;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 176.0F;
    hud_draw_number(&score_number);
}

static void board_draw_bonus2(struct task *task)
{
    board_draw_bonus1(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus2;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 224.0F;
    hud_draw_number(&score_number);
}

static void board_draw_bonus3(struct task *task)
{
    board_draw_bonus2(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->bonus3;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 272.0F;
    hud_draw_number(&score_number);
}

static void board_draw_score(struct task *task)
{
    board_draw_bonus3(task);
    score_number.max = 1000000;
    score_number.value = TALLY(task)->score;
    score_number.pos.x = SCREEN_MIDDLE_X;
    score_number.pos.y = 336.0F;
    hud_draw_number(&score_number);
}

/* a board's tally counts the board's score, the time bonus and the rings
 * bonus into the bottom row. at its start their sum takes its place among
 * the board's best scores. only Sand Hill's board goes on to missions */
static void score_board_step(struct task *task)
{
    struct tally_work *work = (struct tally_work *)task->work;
    int stage;
    int rank;

    if ((per_info[0]->on & SKIP_BUTTONS) && work->wait > SKIP_WAIT &&
        work->mode < BOARD_MISSION && work->mode != BOARD_INIT) {
        work->wait = SKIP_WAIT;
        work->score += work->total;
        work->total = 0.0F;
    }
    switch (work->mode) {
    case BOARD_INIT:
        work->wait = 60;
        work->bonus1 = score;
        work->bonus2 = time_bonus();
        work->bonus3 = rings_bonus();
        work->total = work->bonus1 + work->bonus2 + work->bonus3;
        /* @hack type: the total converted as a Uint32, which tests it
         * against 2^31 before its FTRC (FCMP/GT at 0x8c092fa6, FTRC at
         * 0x8c092fae); converted as an int, hud.c's code runs 32 bytes
         * shorter and 401 of 500 words differ */
        score = (Uint32)work->total;
        /* @hack redundant: the stage masked by STAGE_OF before its shift,
         * which ANDs it and shifts it with SHAD (0x8c092fc0, 0x8c092fc2).
         * stage_get_stage_act returns a Uint16, and the shift drops the low
         * byte the mask clears, so the mask changes nothing; as
         * stage_get_stage_act() >> 8, SHC shifts with SHLR8, hud.c's code
         * runs 32 bytes shorter and 391 of 500 words differ */
        stage = stage_get_stage_act();
        stage = STAGE_OF(stage);
        switch ((Uint8)stage) {
        case STAGE_ICECAP:
            rank = board_record_icecap_score();
            break;
        case STAGE_SUNABOARD:
            rank = board_record_sunaboard_score();
            break;
        default:
            rank = 3;
            break;
        }
        work->record = rank == 0 ? 1.0F : 0.0F;
        work->score = 0.0F;
        work->mode = BOARD_BACK;
        stream_destroy();
        texture_load_pvm("SCORE_BACK", &score_back_texlist);
        texture_load_pvm("SCORE_BOARD", &score_board_texlist);
        texture_load_pvm("SCORE_RESULT", &score_result_texlist);
        stream_reset();
        vmu_request(VMU_REQUEST_CLEAR);
        break;
    case BOARD_BACK:
        if (work->wait-- <= 0) {
            work->mode = BOARD_BONUS1;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        board_draw(task);
        break;
    case BOARD_BONUS1:
        if (work->wait-- <= 0) {
            work->mode = BOARD_BONUS2;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        board_draw_bonus1(task);
        break;
    case BOARD_BONUS2:
        if (work->wait-- <= 0) {
            work->mode = BOARD_BONUS3;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        board_draw_bonus2(task);
        break;
    case BOARD_BONUS3:
        if (work->wait-- <= 0) {
            work->mode = BOARD_COUNT;
            work->wait = 180;
            work->score = 0.0F;
        }
        board_draw_bonus3(task);
        break;
    case BOARD_COUNT:
        if (work->total > 0.0F) {
            /* a step moves points from the first bonus left. the loop's test
             * of the skip buttons never holds: the skip test above reads them
             * first, and ends the count */
            do {
                if (work->bonus1 != 0.0F) {
                    if (work->bonus1 < TALLY_STEP) {
                        work->score += work->bonus1;
                        work->total -= work->bonus1;
                        work->bonus1 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_STEP;
                        work->total -= TALLY_STEP;
                        work->bonus1 -= TALLY_STEP;
                    }
                } else if (work->bonus2 != 0.0F) {
                    if (work->bonus2 < TALLY_TIME_STEP) {
                        work->score += work->bonus2;
                        work->total -= work->bonus2;
                        work->bonus2 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_TIME_STEP;
                        work->total -= TALLY_TIME_STEP;
                        work->bonus2 -= TALLY_TIME_STEP;
                    }
                } else if (work->bonus3 != 0.0F) {
                    if (work->bonus3 < TALLY_RINGS_STEP) {
                        work->score += work->bonus3;
                        work->total -= work->bonus3;
                        work->bonus3 = 0.0F;
                        break;
                    } else {
                        work->score += TALLY_RINGS_STEP;
                        work->total -= TALLY_RINGS_STEP;
                        work->bonus3 -= TALLY_RINGS_STEP;
                    }
                } else {
                    work->wait = 60;
                    /* score_board_step has no case for ACT_MISSION, but this
                     * never runs. a board's total is the sum of its bonuses,
                     * all whole numbers, so it reaches 0 with them */
                    work->mode = ACT_MISSION;
                    break;
                }
            } while (per_info[0]->on & SKIP_BUTTONS);
            /* @bug logic: the tick waits for wait to reach a multiple of
             * 8, but the count holds wait at 180, and its end sets 60. the
             * tally never ticks (TST #7,R0 at 0x8c093282) */
            if (!(work->wait & 7)) {
                sound_play(SE_TICK, 0, 0, 0);
            }
        } else if (work->wait-- <= 0) {
            work->wait = 60;
            /* @hack redundant: the stage masked by STAGE_OF before its
             * shift, which ANDs it and shifts it with SHAD (0x8c0932ac,
             * 0x8c0932ae). stage_get_stage_act returns a Uint16, and the
             * shift drops the low byte the mask clears, so the mask changes
             * nothing; as stage_get_stage_act() >> 8, SHC shifts with SHLR8,
             * hud.c's code runs 32 bytes shorter and 96 of 500 words differ */
            stage = stage_get_stage_act();
            stage = STAGE_OF(stage);
            switch ((Uint8)stage) {
            case STAGE_ICECAP:
            default:
                work->mode = SCORE_WAIT;
                break;
            case STAGE_SUNABOARD:
                work->mode = BOARD_MISSION;
                break;
            }
        }
        board_draw_score(task);
        break;
    case BOARD_MISSION:
        score_mission(task);
        break;
    case SCORE_WAIT:
        if (work->wait-- <= 0) {
            work->mode = SCORE_END;
        }
        break;
    case SCORE_END:
        /* Tails' board is a race, and a loss ends it with STAGE_END_MISS */
        if (get_character() != CHARACTER_TAILS) {
            stage_end(STAGE_END_CLEAR);
        } else if (race_outcome() == RACE_WON) {
            stage_end(STAGE_END_CLEAR);
        } else {
            stage_end(STAGE_END_MISS);
        }
        njReleaseTexture(&score_back_texlist);
        njReleaseTexture(&score_board_texlist);
        njReleaseTexture(&score_result_texlist);
        task_kill(task);
        break;
    }
}

/* whether the character cleared mission on a board. A is 10000 points or
 * more, and B any score. only Sonic and Tails have them */
static Bool mission_check_board(int character, int mission)
{
    Bool result = FALSE;

    switch (character) {
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
        switch (mission) {
        case MISSION_A:
            if (score >= 10000) {
                result = TRUE;
            }
            break;
        case MISSION_B:
            result = TRUE;
            break;
        }
        break;
    }
    return result;
}

/* draw Sky Chase's score screen: rows for the score, and for the share
 * of targets hit */
static void shoot_draw(struct task *task)
{
    NJS_ARGB color;

    color.a = -0.5F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawSprite2D(&shoot_sprites[0], 0, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&shoot_sprites[1], 1, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&shoot_sprites[2], 2, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    color.a = 0.0F;
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
    score_draw_back(SCREEN_MIDDLE_X, 224.0F, 15.0F, 4.0F);
}

/* each draw below calls the one above it, then draws one row's number */
static void shoot_draw_score(struct task *task)
{
    shoot_draw(task);
    score_number.max = 99999;
    score_number.value = TALLY(task)->bonus1;
    score_number.pos.x = 352.0F;
    score_number.pos.y = 192.0F;
    hud_draw_number(&score_number);
}

static void shoot_draw_rate(struct task *task)
{
    struct tally_work *work = (struct tally_work *)task->work;

    shoot_draw_score(task);
    score_number.max = 100;
    score_number.value = work->bonus2;
    score_number.pos.x = 384.0F;
    score_number.pos.y = 272.0F;
    hud_draw_number(&score_number);
}

/* Sky Chase's score screen. at its start the score takes its place among
 * the save's best for the act, which it keeps for Sonic and for Tails */
static void score_shoot_step(struct task *task)
{
    struct tally_work *work = (struct tally_work *)task->work;
    struct save *save;
    struct save_scores *best;
    int scores[SAVE_BEST + 1]; /* the save's best, then this run's */
    int character;
    int stage;
    int swap;
    int i;

    if ((per_info[0]->on & SKIP_BUTTONS) && work->wait > SKIP_WAIT &&
        work->mode > SHOOT_INIT && work->mode < SHOOT_MISSION) {
        work->wait = SKIP_WAIT;
        work->score += work->total;
    }
    switch (work->mode) {
    case SHOOT_INIT:
        vmu_request(VMU_REQUEST_CLEAR);
        stream_destroy();
        texture_load_pvm("SCORE_BACK", &score_back_texlist);
        texture_load_pvm("SCORE_SHOOT", &score_shoot_texlist);
        stream_reset();
        work->wait = 60;
        work->bonus1 = score;
        work->bonus2 = shoot_hit_rate;
        work->mode = SHOOT_BACK;
        save = &saves[save_slot];
        character = get_character();
        if (character == CHARACTER_TAILS) {
            character--;
        }
        stage = stage_get_stage_act() >> 8;
        switch (stage) {
        default:
        case STAGE_SHOOTING1:
            best = &save->shooting1_scores[character];
            break;
        case STAGE_SHOOTING2:
            best = &save->shooting2_scores[character];
            break;
        }
        for (i = 0; i < SAVE_BEST; i++) {
            scores[i] = best->score[i];
        }
        scores[SAVE_BEST] = work->bonus1;
        for (i = SAVE_BEST; i > 0; i--) {
            if (scores[i - 1] < scores[i]) {
                swap = scores[i - 1];
                scores[i - 1] = scores[i];
                scores[i] = swap;
            }
        }
        for (i = 0; i < SAVE_BEST; i++) {
            best->score[i] = scores[i];
        }
        break;
    case SHOOT_BACK:
        if (work->wait-- <= 0) {
            work->mode = SHOOT_SCORE;
            work->wait = 20;
            sound_play(SE_TICK, 0, 0, 0);
        }
        shoot_draw(task);
        break;
    case SHOOT_SCORE:
        if (work->wait-- <= 0) {
            work->mode = SHOOT_RATE;
            work->wait = 180;
            sound_play(SE_TICK, 0, 0, 0);
        }
        shoot_draw_score(task);
        break;
    case SHOOT_RATE:
        if (work->wait-- <= 0) {
            work->mode = SHOOT_MISSION;
            work->wait = 60;
            work->step = STEP_INIT;
        }
        shoot_draw_rate(task);
        break;
    case SHOOT_MISSION:
        score_mission(task);
        break;
    case SCORE_WAIT:
        if (work->wait-- <= 0) {
            work->mode = SCORE_END;
        }
        break;
    case SCORE_END:
        stage_end(STAGE_END_CLEAR);
        njReleaseTexture(&score_back_texlist);
        njReleaseTexture(&score_shoot_texlist);
        task_kill(task);
        break;
    }
}

/* draw the cart's score screen: rows for each of the three laps' times,
 * and for the run's */
static void cart_draw(struct task *task)
{
    NJS_ARGB color;

    color.a = -0.5F;
    color.r = color.g = color.b = 1.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawSprite2D(&cart_sprites[0], 0, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&cart_sprites[1], 1, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&cart_sprites[2], 2, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    njDrawSprite2D(&cart_sprites[3], 3, -1.15F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    color.a = 0.0F;
    color.r = color.g = color.b = 0.0F;
    njSetConstantMaterial(&color);
    score_draw_back(SCREEN_MIDDLE_X, 224.0F, 15.0F, 3.0F);
    score_draw_back(SCREEN_MIDDLE_X, 336.0F, 15.0F, 0.0F);
}

/* each draw below calls the one above it, then draws one row's time */
/* NOLINTBEGIN(bugprone-signed-char-misuse) */
static void cart_draw_lap1(struct task *task)
{
    struct score_time time;
    struct cart_time lap;

    cart_draw(task);
    lap = cart_get_lap(0);
    time.minutes = lap.minutes;
    time.seconds = lap.seconds;
    time.hundredths = lap.hundredths;
    time.x = 288.0F;
    time.y = 176.0F;
    score_draw_time(time);
}

static void cart_draw_lap2(struct task *task)
{
    struct score_time time;
    struct cart_time lap;

    cart_draw_lap1(task);
    lap = cart_get_lap(1);
    time.minutes = lap.minutes;
    time.seconds = lap.seconds;
    time.hundredths = lap.hundredths;
    time.x = 288.0F;
    time.y = 224.0F;
    score_draw_time(time);
}

static void cart_draw_lap3(struct task *task)
{
    struct score_time time;
    struct cart_time lap;

    cart_draw_lap2(task);
    lap = cart_get_lap(2);
    time.minutes = lap.minutes;
    time.seconds = lap.seconds;
    time.hundredths = lap.hundredths;
    time.x = 288.0F;
    time.y = 272.0F;
    score_draw_time(time);
}

static void cart_draw_time(struct task *task)
{
    struct score_time time;
    struct cart_time run;

    cart_draw_lap3(task);
    run = cart_get_time();
    time.minutes = run.minutes;
    time.seconds = run.seconds;
    time.hundredths = run.hundredths;
    time.x = 288.0F;
    time.y = 336.0F;
    score_draw_time(time);
}
/* NOLINTEND(bugprone-signed-char-misuse) */

/* the cart's score screen. cart_record_time puts the run among the act's
 * best times, and only act 0 goes on to missions */
static void score_cart_step(struct task *task)
{
    struct tally_work *work = (struct tally_work *)task->work;
    int stage_act;
    int rank;

    if ((per_info[0]->on & SKIP_BUTTONS) && work->wait > SKIP_WAIT &&
        work->mode > CART_SAVE && work->mode < CART_MISSION) {
        stage_act = stage_get_stage_act();
        switch ((Uint8)stage_act) {
        case 0:
            work->mode = CART_MISSION;
            break;
        default:
            work->mode = SCORE_WAIT;
            break;
        }
        work->wait = SKIP_WAIT;
    }
    switch (work->mode) {
    case CART_INIT:
        stream_destroy();
        texture_load_pvm("SCORE_BACK", &score_back_texlist);
        texture_load_pvm("SCORE_CART", &score_cart_texlist);
        stream_reset();
        work->wait = 120;
        work->mode = CART_WAIT;
        vmu_request(VMU_REQUEST_CLEAR);
        break;
    case CART_WAIT:
        if (work->wait-- <= 0) {
            work->wait = 60;
            work->mode = CART_SAVE;
        }
        break;
    case CART_SAVE:
        rank = cart_record_time();
        work->record = rank == 0 ? 1.0F : 0.0F;
        work->mode = CART_BACK;
        break;
    case CART_BACK:
        if (work->wait-- <= 0) {
            work->mode = CART_LAP1;
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        cart_draw(task);
        break;
    case CART_LAP1:
        if (work->wait-- <= 0) {
            work->mode = CART_LAP2;
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        cart_draw_lap1(task);
        break;
    case CART_LAP2:
        if (work->wait-- <= 0) {
            work->mode = CART_LAP3;
            work->wait = 30;
            sound_play(SE_TICK, 0, 0, 0);
        }
        cart_draw_lap2(task);
        break;
    case CART_LAP3:
        if (work->wait-- <= 0) {
            work->mode = CART_TIME;
            work->wait = 180;
            sound_play(SE_TICK, 0, 0, 0);
        }
        cart_draw_lap3(task);
        break;
    case CART_TIME:
        if (work->wait-- <= 0) {
            stage_act = stage_get_stage_act();
            switch ((Uint8)stage_act) {
            case 0:
                work->mode = CART_MISSION;
                break;
            default:
                work->mode = SCORE_WAIT;
                break;
            }
            work->wait = 10;
        }
        cart_draw_time(task);
        break;
    case CART_MISSION:
        score_mission(task);
        break;
    case SCORE_WAIT:
        if (work->wait++ > 30) {
            work->mode = SCORE_END;
            cart_score_done();
        }
        break;
    case SCORE_END:
        njReleaseTexture(&score_back_texlist);
        njReleaseTexture(&score_cart_texlist);
        task_kill(task);
        break;
    }
}

/* whether the character cleared mission on the cart. A is a run of
 * CART_A_TIME or less, and B any run. Eggman and Tikal have none */
static Bool mission_check_cart(int character, int mission)
{
    Bool result = FALSE;
    struct cart_time run;
    int time;

    switch (character) {
    case CHARACTER_SONIC:
    case CHARACTER_TAILS:
    case CHARACTER_KNUCKLES:
    case CHARACTER_AMY:
    case CHARACTER_E102:
    case CHARACTER_BIG:
        switch (mission) {
        case MISSION_A:
            run = cart_get_time();
            time = (((run.minutes * 60) + run.seconds) * 100) + run.hundredths;
            if (time <= CART_A_TIME) {
                result = TRUE;
            }
            break;
        case MISSION_B:
            result = TRUE;
            break;
        }
        break;
    }
    return result;
}

/* run the score screen of the stage and act playing. a boss in
 * GAME_MODE_TRIAL has the boss trial's, and an act with no screen of its
 * own has the act's tally */
void score_step(struct task *task)
{
    Uint16 stage;

    stage = stage_get_stage_act();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    switch (stage) {
    case STAGE_ACT(STAGE_PRACTICE, 0):
        score_mole_step(task);
        break;
    case STAGE_ACT(STAGE_SHOOTING1, 0):
    case STAGE_ACT(STAGE_SHOOTING2, 0):
        score_shoot_step(task);
        break;
    case STAGE_ACT(STAGE_CART, 0):
    case STAGE_ACT(STAGE_CART, 1):
    case STAGE_ACT(STAGE_CART, 2):
    case STAGE_ACT(STAGE_CART, 3):
    case STAGE_ACT(STAGE_CART, 4):
    case STAGE_ACT(STAGE_CART, 5):
        score_cart_step(task);
        break;
    case STAGE_ACT(STAGE_SUNABOARD, 0):
        score_board_step(task);
        break;
    /* Ice Cap's board, a minigame when the trial menu's minigame entry
     * plays it */
    case STAGE_ACT(STAGE_ICECAP, 2):
        if (game_mode == GAME_MODE_TRIAL &&
            menu_state.trial[0] == MENU_TRIAL_MINI_GAME) {
            score_board_step(task);
        } else {
            score_act_step(task);
        }
        break;
    case STAGE_ACT(STAGE_CHAOS0, 0):
    case STAGE_ACT(STAGE_CHAOS2, 0):
    case STAGE_ACT(STAGE_CHAOS4, 0):
    case STAGE_ACT(STAGE_CHAOS6, 0):
    case STAGE_ACT(STAGE_CHAOS6, 1):
    case STAGE_ACT(STAGE_CHAOS7, 0):
    case STAGE_ACT(STAGE_EGGMOBILE1, 0):
    case STAGE_ACT(STAGE_EGGMOBILE2, 0):
    case STAGE_ACT(STAGE_EGGMOBILE3, 0):
    case STAGE_ACT(STAGE_EGGMANROBO, 0):
    case STAGE_ACT(STAGE_E101, 0):
    case STAGE_ACT(STAGE_E101_R, 0):
        if (game_mode == GAME_MODE_TRIAL) {
            score_trial_step(task);
        } else {
            score_act_step(task);
        }
        break;
    default:
        score_act_step(task);
        break;
    }
}

/* once task's score screen has reached SCORE_WAIT, set its work's submode
 * to -1 and return TRUE. it returns FALSE before then. nothing calls it: no
 * image holds its address. it isn't static, since SHC leaves out a static
 * function that nothing calls */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
Bool score_mark_ended(struct task *task)
{
    if (task->work->mode >= SCORE_WAIT) {
        task->work->submode = -1;
        return TRUE;
    }
    return FALSE;
}

void hud_draw_number(struct hud_number *number)
{
    NJS_SPRITE *sprite;
    Uint32 value;
    Uint32 color;
    Uint32 max;
    int place;
    int flags;
    int count;
    int digits;
    Uint32 digit;
    Bool draw;
    Float priority;
    Float x;
    Float y;
    Float alpha;
    Float red;
    Float green;
    Float blue;

    /* @hack spelling: flags, place and sprite set ahead of frame_step's test,
     * which loads them before it (0x8c09412a to 0x8c094132); after the
     * test, 15 of 280 words differ */
    flags = number->flags;
    place = number->place;
    sprite = &hud_sprite;
    if (frame_step) {
        return;
    }
    value = number->value;
    if (flags & NUMBER_CLAMPED) {
        max = number->max;
        if (value > max) {
            value = max;
        }
    }
    digits = 1;
    if (flags & NUMBER_CLAMPED) {
        count = number->max;
    } else {
        count = number->value;
    }
    while (count >= 10) {
        /* @hack spelling: count = count / 10, not /=, which copies count to
         * R1 before __divls's call and loads 10 in its delay slot
         * (0x8c094168, 0x8c09416c); with /=, the two trade places and 2 of
         * 280 words differ */
        count = count / 10;
        digits++;
    }
    if (flags & NUMBER_SCALED) {
        sprite->sx = sprite->sy = number->scale;
    } else {
        sprite->sx = sprite->sy = 1.0F;
    }
    if (sprite->sx > 1.0F) {
        njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    } else {
        njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    }
    priority = -1.1F - ((place & NUMBER_DEPTH) * 0.1F);
    if (place & NUMBER_TOP) {
        y = 0.0F;
    } else if (place & NUMBER_BOTTOM) {
        y = -(sprite->sy * 16.0F);
    } else {
        y = -(sprite->sy * 8.0F);
    }
    if (place & NUMBER_LEFT) {
        x = sprite->sx * (digits - 1) * 16.0F;
    } else if (place & NUMBER_RIGHT) {
        x = sprite->sx * 16.0F;
    } else {
        x = sprite->sx * (digits - 1) * 8.0F;
    }
    color = number->color;
    blue = (color & 0xff) * (1.0F / 255.0F);
    color >>= 8;
    green = (color & 0xff) * (1.0F / 255.0F);
    color >>= 8;
    red = (color & 0xff) * (1.0F / 255.0F);
    alpha = (color >> 8 & 0xff) * (1.0F / 255.0F);
    draw_set_material(alpha, red, green, blue);
    sprite->p.x = number->pos.x + x;
    sprite->p.y = number->pos.y + y;
    draw = TRUE;
    for (; digits > 0; digits--) {
        if (flags & NUMBER_COLON) {
            digit = HUD_COLON;
        } else {
            digit = value % 10;
            value = value / 10;
        }
        /* @hack spelling: draw compared with TRUE, CMP/EQ #1 (0x8c0942f4).
         * draw is set only to TRUE or FALSE; as a plain test, 17 of 280
         * words differ */
        if (draw == TRUE) {
            njDrawSprite2D(sprite, digit, priority,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        }
        if ((flags & NUMBER_ZEROS) || value) {
            draw = TRUE;
        } else {
            draw = FALSE;
        }
        sprite->p.x -= sprite->sx * 16.0F;
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
}

void trial_times_clear(void)
{
    int i;

    for (i = 0; i < TRIAL_TIMES; i++) {
        trial_times[i].minutes = 0;
        trial_times[i].seconds = 0;
        trial_times[i].frames = 0;
    }
}
