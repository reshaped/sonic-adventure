/* the hud's rings, lives, time and score, and the numbers it draws.
 * score_step runs the screen that ends a stage */
#ifndef HUD_HUD_H
#define HUD_HUD_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* hud_number.flags' bits */
#define NUMBER_SCALED 0x01 /* the digits drawn at scale, not at 1 */
/* value held to max, and as many digits as max has */
#define NUMBER_CLAMPED 0x08
#define NUMBER_COLON 0x20 /* a colon in each digit's place */
#define NUMBER_ZEROS 0x40 /* the leading zeros drawn, not left out */

/* hud_number.place's bits. NUMBER_DEPTH holds a depth from 0 to 7, which
 * draws the number that many tenths of priority further back */
#define NUMBER_DEPTH 0x07
/* NUMBER_TOP puts pos at the number's top edge, and NUMBER_BOTTOM at its
 * bottom edge. with neither, pos is at its middle. NUMBER_TOP wins when
 * both are set */
#define NUMBER_TOP 0x10
#define NUMBER_BOTTOM 0x20
/* NUMBER_LEFT puts pos at the number's left edge, and NUMBER_RIGHT two
 * digits' widths left of its right edge. with neither, pos is half a digit
 * left of its middle. NUMBER_LEFT wins when both are set */
#define NUMBER_RIGHT 0x40
#define NUMBER_LEFT 0x80

/* a number for hud_draw_number to draw. its digits are 16 pixels square at
 * a scale of 1 */
struct hud_number {
    Uint8 flags;
    Uint8 place;
    Sint16 unk_02; /* hud_draw_number doesn't read it */
    Uint32 max;
    Uint32 value;
    NJS_POINT3 pos;
    Float scale;
    Uint32 color; /* 0xAARRGGBB */
};

/* a kind of Hedgehog Hammer target: how many the round hit, and what a hit
 * scores */
struct mole_hit {
    int count;
    int points;
};

/* each names a run of con_regular_textures. hud_texlist names its first
 * 24, the hud's own digits, labels, heads and ranks. hud_digit_texlist
 * names the 11 after them, the big digits and their colon */
extern NJS_TEXLIST hud_digit_texlist;
extern NJS_TEXLIST hud_texlist;
/* the kinds of Hedgehog Hammer target, which index mole_hits */
enum mole_kind { KIND_500, KIND_100, KIND_EGGMAN, MOLE_KINDS };
/* the Hedgehog Hammer's round, by kind of target. its module counts the
 * hits, and the score screen totals them */
extern struct mole_hit mole_hits[MOLE_KINDS];

/* the lives lives_reset sets: a new save's, and the fewest a stage starts
 * with */
#define LIVES_START 4

/* reset for a new game: no rings, LIVES_START lives, and trial_count 0 */
void hud_reset(void);
/* set score, bonus_points and hints_used to 0, and clear the combo */
void score_reset(void);
void ring_reset(void);
void lives_reset(void);
/* add count lives, with the extra life's sound when count is above 0.
 * count is cut to a Sint8, and a sum outside 0 to 127 leaves 127 lives. in
 * a field, the lives then show for 181 steps */
void lives_add(Sint16 count);
/* add count rings, and a life for each hundred the rings pass. in a field,
 * the rings then show for 181 steps */
void ring_add(int count);
void score_add(int points);
/* add points to bonus_points, which the act's score screen counts in. the
 * points also join the combo while its window is open, and open it again */
void score_add_bonus(int points);
int ring_count(void);
/* draw the rings and the lives, unless hud_hidden is below 0, the cart
 * runs or Big plays. in a field, the rings show only for 181 steps after a
 * call of ring_add, and the lives after one of lives_add, whatever its
 * count */
void hud_draw(void);
Sint16 lives_count(void);

/* set play_time to 0, and reset the time as time_reset does */
void play_time_reset(void);
/* set the time to 0:00, or for E-102, whose time counts down, to 3:00.
 * time_steps goes to 0 */
void time_reset(void);
void time_start(void);
void time_stop(void);
/* add the frame's steps, frame_step_count, to play_time, once a frame */
void play_time_count(void);
/* the time's step, which counts the time while it runs. time_steps counts
 * every call */
void time_step(void);
int play_time_get(void);
/* draw the time, unless hud_hidden or rings_shown is below 0. Big, Sky
 * Chase and the cart draw no time, and a board played as a minigame draws
 * its score in the time's place */
void hud_draw_timer(void);
/* time_get_whole gets the time's minutes and seconds, and time_set_whole
 * sets them at the start of the second */
void time_get_whole(Uint8 *minutes, Uint8 *seconds);
void time_set_whole(int minutes, int seconds);
void time_set(Uint8 minutes, Uint8 seconds, Uint8 frames);
void time_get(Uint8 *minutes, Uint8 *seconds, Uint8 *frames);
void time_add_seconds(int seconds);
Bool time_is_zero(void);

/* the step of the score screen that ends a stage, for the stage and act
 * playing */
void score_step(struct task *task);
/* draw number in hud_texlist's digits, as its flags and place say. it draws
 * only on a frame's first step, while frame_step is 0 */
void hud_draw_number(struct hud_number *number);
/* set every one of trial_times to 0. trial_count stays as it is */
void trial_times_clear(void);

#endif
