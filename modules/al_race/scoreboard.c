/* the Chao race's clock, standings board and records, and the letters and
 * digits they draw with. the file also holds the names the rivals take */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/models.h"
#include "al_race/game.h"
#include "al_race/menu.h"
#include "al_race/race_rival.h"
#include "al_race/racer.h"
#include "al_race/standings.h"
#include "al_race/texlists.h"
#include "chao/chao.h"
#include "draw/draw.h"
#include "system/frame.h"
#include "system/texlists.h"
#include "system/util.h"

/* the font: characters FONT_FIRST to FONT_LAST in alife_window_texlist's
 * texture 0, FONT_COLUMNS to a row. each glyph is FONT_CELL units of u and v on
 * a side */
#define FONT_FIRST 32
#define FONT_LAST 191
#define FONT_COLUMNS 16
#define FONT_CELL 16

/* the names in each of the four tables of rivals' names */
#define RIVAL_NAMES 8

/* the gap between a name's letters, as a share of the letters' size */
#define LETTER_GAP 0.15F

/* the frames of menu_sprites beside the clock's time and the best time */
#define CLOCK_LABEL 7
#define BEST_LABEL 8

/* the bytes of records_mark */
#define MARK_SIZE 8
/* the first place's minutes in a reset record. each place after it takes one
 * more */
#define DEFAULT_MINUTES 8

/* the separator's frame in a digit set, after the ten digits */
#define DIGIT_SEPARATOR 10
/* the frame of lane_frames after the lanes' own, drawn beside the place of a
 * racer at its goal or on the podium */
#define FINISHED_FRAME RACE_LANES
/* the bit of the standings' timer that picks the pointer's frame */
#define POINTER_BLINK 0x20

/* the board's first TOP_PLACES places rest at TOP_SCALE, and the other places
 * at OTHER_SCALE. a top place whose lane changes grows to PASSED_SCALE, then
 * shrinks by SCALE_STEP a step back to rest */
#define TOP_PLACES 3
#define TOP_SCALE 1.15F
#define OTHER_SCALE 0.85F
#define PASSED_SCALE 1.45F
#define SCALE_STEP 0.02F

/* spin grows by SPIN_STEP a step, and returns to 0 once past SPIN_END, two
 * turns. the followed lane's sprite takes spin as its angle only while spin
 * is under SPIN_TURN, one turn */
#define SPIN_STEP 0x222
#define SPIN_TURN NJM_DEG_ANG(360)
#define SPIN_END NJM_DEG_ANG(720)

/* where and how race_time_draw draws a time */
struct time_style {
    Float x; /* x and y place the first digit */
    Float y;
    Float sx; /* the digits' scale */
    Float sy;
    Float digit_step; /* how far x moves from a digit to the next */
    /* how far x moves from a digit to the separator after it, and again from
     * the separator to the next digit */
    Float separator_step;
    int set; /* the digits' set, as race_draw_digit takes it */
};

/* the names race_rival_name_by_type gives a rival of type CHAO_CHILD, and
 * of any other type */
static const Uint8 child_rival_names[RIVAL_NAMES][CHAO_NAME_SIZE] = {
#include "al_race/scoreboard/child_rival_names.inc"
};
static const Uint8 adult_rival_names[RIVAL_NAMES][CHAO_NAME_SIZE] = {
#include "al_race/scoreboard/adult_rival_names.inc"
};
/* the names race_rival_name_by_egg gives the silver egg's rivals, for gold
 * 0, and the gold egg's. race_rival_init passes 0 for its CHAO_PAL rival, the
 * silver egg's Chao, and 1 for its CHAO_GOLD one */
static const Uint8 silver_rival_names[RIVAL_NAMES][CHAO_NAME_SIZE] = {
#include "al_race/scoreboard/silver_rival_names.inc"
};
static const Uint8 gold_rival_names[RIVAL_NAMES][CHAO_NAME_SIZE] = {
#include "al_race/scoreboard/gold_rival_names.inc"
};
/* the name each place takes in a reset record, from the first */
static const Uint8 default_names[RACE_BEST][CHAO_NAME_SIZE] = {
#include "al_race/scoreboard/default_names.inc"
};
/* the styles of the clock's time, and of the best time beside it */
static const struct time_style clock_style = {
    230.0F, 430.0F, 1.0F, 1.0F, 30.0F, 20.0F, 1,
};
static const struct time_style best_style = {
    465.0F, 430.0F, 0.5F, 0.5F, 15.0F, 10.0F, 0,
};
/* where the clock's two labels go: CLOCK_LABEL, then BEST_LABEL */
static const NJS_POINT2 clock_labels_at[2] = {
    {165.0F, 430.0F},
    {507.0F, 400.0F},
};
/* where the board draws each place */
static const NJS_POINT2 standings_at[RACE_LANES] = {
    {70.0F, 120.0F}, {70.0F, 160.0F}, {70.0F, 200.0F}, {70.0F, 240.0F},
    {70.0F, 270.0F}, {70.0F, 300.0F}, {70.0F, 330.0F}, {70.0F, 360.0F},
};
/* the bytes that open records the race has set up, with no null.
 * race_records_reset_unmarked resets records that don't open with them */
/* NOLINTNEXTLINE(clang-diagnostic-unterminated-string-initialization) */
static const char records_mark[MARK_SIZE] = "\214\255\271\243\160\246\174\213";

/* sets 0 and 1 of the digits race_draw_digit and race_time_draw take: the ten
 * digits in con_regular_texlist, then the separator */
static NJS_TEXANIM digit_set_0[DIGIT_SEPARATOR + 1] = {
#include "al_race/scoreboard/digit_set_0.inc"
};
static NJS_TEXANIM digit_set_1[DIGIT_SEPARATOR + 1] = {
#include "al_race/scoreboard/digit_set_1.inc"
};
/* the board's sprite for each lane, then FINISHED_FRAME */
static NJS_TEXANIM lane_frames[FINISHED_FRAME + 1] = {
#include "al_race/scoreboard/lane_frames.inc"
};
/* the board's pointer, which blinks between these two frames.
 * standings_draw_marker draws the first as the marker */
static NJS_TEXANIM pointer_frames[2] = {
#include "al_race/scoreboard/pointer_frames.inc"
};

/* draw letter in the font, size on a side and centred on x and y. a letter
 * outside FONT_FIRST to FONT_LAST draws as FONT_FIRST */
static void race_draw_letter(Uint8 letter, Float x, Float y, Float size)
{
    NJS_POINT2COL polygon;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR uvs[4];
    Float half;
    int left;
    int top;
    Uint8 glyph;

    half = size * 0.5F;
    points[0].x = x - half;
    points[0].y = y - half;
    points[1].x = x + half;
    points[1].y = y - half;
    points[2].x = x + half;
    points[2].y = y + half;
    points[3].x = x - half;
    points[3].y = y + half;
    colors[0].color = 0xffffffff;
    colors[1].color = 0xffffffff;
    colors[2].color = 0xffffffff;
    colors[3].color = 0xffffffff;
    if (letter >= FONT_FIRST && letter <= FONT_LAST) {
        glyph = letter - FONT_FIRST;
    } else {
        glyph = 0;
    }
    left = (glyph % FONT_COLUMNS) * FONT_CELL;
    top = (glyph / FONT_COLUMNS) * FONT_CELL;
    uvs[0].tex.u = left;
    uvs[0].tex.v = top;
    uvs[1].tex.u = left + FONT_CELL - 1;
    uvs[1].tex.v = top;
    uvs[2].tex.u = left + FONT_CELL - 1;
    uvs[2].tex.v = top + FONT_CELL - 1;
    uvs[3].tex.u = left;
    uvs[3].tex.v = top + FONT_CELL - 1;
    polygon.p = points;
    polygon.col = colors;
    polygon.tex = uvs;
    polygon.num = 4;
    njSetTexture(&alife_window_texlist);
    njSetTextureNum(0);
    njDrawPolygon2D(&polygon, 4, -2.0F,
                    NJD_USE_TEXTURE | NJD_TRANSPARENT | NJD_FILL);
}

static void race_time_clear(struct race_time *time)
{
    time->steps = 0;
    time->seconds = 0;
    time->minutes = 0;
}

static void race_time_set(struct race_time *time, int minutes, int seconds,
                          int frames)
{
    time->steps = frames;
    time->seconds = seconds;
    time->minutes = minutes;
}

void race_time_copy(struct race_time *dst, const struct race_time *src)
{
    int i = 0;
    const char *from = (const char *)src;
    char *to = (char *)dst;

    while (i < (int)sizeof(*dst)) {
        *to = *from;
        to++;
        from++;
        i++;
    }
}

void race_time_copy_to(const struct race_time *src, struct race_time *dst)
{
    int i = 0;
    char *to = (char *)dst;
    const char *from = (const char *)src;

    while (i < (int)sizeof(*dst)) {
        *to = *from++;
        to++;
        i++;
    }
}

int race_time_compare(const struct race_time *left,
                      const struct race_time *right)
{
    int packed[2];
    int i = 0;

    do {
        /* @hack type: the chosen time through an array of one, which chooses
         * it into R14 and copies it to R7 for the loads (0x0cb91692 to
         * 0x0cb91698); through time alone, the function runs 6 bytes short
         * and 49 of 51 words differ */
        const struct race_time(*time)[1];

        if (i == 0) {
            time = (const struct race_time(*)[1])left;
        } else {
            time = (const struct race_time(*)[1])right;
        }
        packed[i] = ((*time)[0].minutes << 16) + ((*time)[0].seconds << 8) +
                    (*time)[0].steps;
        i++;
    } while (i < 2);
    if (packed[0] > packed[1]) {
        return 1;
    } else if (packed[0] < packed[1]) {
        return 0;
    } else {
        return -1;
    }
}

/* not static. SHC expands race_time_swap at its one call, in
 * race_records_enter, and keeps a function's own copy as well only if other
 * files can call it. the image holds race_time_swap's copy, at 0x0cb916e2 */
int race_time_swap(struct race_time *first, struct race_time *second)
{
    struct race_time was;

    was = *first;
    *first = *second;
    *second = was;
    return 1;
}

static void race_time_step(struct race_time *time)
{
    time->steps++;
    if (time->steps >= 60) {
        time->steps = 0;
        time->seconds++;
    }
    if (time->seconds >= 60) {
        time->seconds = 0;
        time->minutes++;
    }
    /* @bug logic: the minutes stop at 100, which two digits can't show
     * (MOV #100,R5 at 0x0cb9173e), while the steps and seconds go on
     * counting and wrapping (0x0cb91714 to 0x0cb91734). race_time_digits
     * gives 100 a tens digit of 10, which race_time_draw draws as the
     * DIGIT_SEPARATOR frame */
    if (time->minutes >= 100) {
        time->minutes = 100;
    }
}

/* write time's minutes, seconds and hundredths into digits[0] to digits[5],
 * each as its tens and then its units */
static void race_time_digits(const struct race_time *time, char *digits)
{
    int hundredths = time->steps * 100 / 60;

    digits[4] = (hundredths + 0.5F) * 0.1F;
    digits[5] = hundredths - (digits[4] * 10);
    digits[2] = (time->seconds + 0.5F) * 0.1F;
    digits[3] = time->seconds - (digits[2] * 10);
    digits[0] = (time->minutes + 0.5F) * 0.1F;
    digits[1] = time->minutes - (digits[0] * 10);
}

static void race_time_draw(const struct race_time *time,
                           const struct time_style *style)
{
    char digits[6];
    NJS_SPRITE sprite;
    int i;

    if (frame_step) {
        return;
    }
    race_time_digits(time, digits);
    sprite.p.z = 0.0F;
    sprite.ang = 0;
    sprite.tlist = &con_regular_texlist;
    if (style->set) {
        sprite.tanim = digit_set_1;
    } else {
        sprite.tanim = digit_set_0;
    }
    sprite.sx = style->sx;
    sprite.sy = style->sy;
    sprite.p.x = style->x;
    sprite.p.y = style->y;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&con_regular_texlist);
    for (i = 0; i < 6; i++) {
        /* @hack type: each digit cast to Uint8, though none is negative,
         * which writes an EXTU.B R5,R5 before each digit's draw, five in the
         * unrolled loop (0x0cb91862 to 0x0cb919b2); without it, the function
         * runs 10 bytes short and 170 of 238 words differ */
        njDrawSprite2D(&sprite, (Uint8)digits[i], -2.0F, NJD_SPRITE_ALPHA);
        if (i != 1 && i != 3) {
            sprite.p.x += style->digit_step;
        } else {
            sprite.p.x += style->separator_step;
            njDrawSprite2D(&sprite, DIGIT_SEPARATOR, -2.0F, NJD_SPRITE_ALPHA);
            sprite.p.x += style->separator_step;
        }
    }
}

void race_rival_name_by_type(Uint8 *name, int type, int index)
{
    const Uint8 *from;
    /* @hack type: name through a pointer to the name array, which copies it
     * to R1, not R6 (0x0cb91a26); through name alone, 3 of 30 words differ */
    Uint8(*to)[CHAO_NAME_SIZE];
    int i = 0;

    while (index >= RIVAL_NAMES) {
        index -= RIVAL_NAMES;
    }
    if (type == CHAO_CHILD) {
        from = child_rival_names[index];
    } else {
        from = adult_rival_names[index];
    }
    to = (Uint8(*)[CHAO_NAME_SIZE])name;
    while (*from) {
        (*to)[i] = *from++;
        i++;
    }
    (*to)[i] = 0;
}

void race_rival_name_by_egg(Uint8 *name, int gold, int index)
{
    const Uint8 *from;
    /* @hack type: name through a pointer to the name array, which copies it
     * to R1, not R6 (0x0cb91a88); through name alone, 3 of 30 words differ */
    Uint8(*to)[CHAO_NAME_SIZE];
    int i = 0;

    while (index >= RIVAL_NAMES) {
        index -= RIVAL_NAMES;
    }
    if (gold == 0) {
        from = silver_rival_names[index];
    } else {
        from = gold_rival_names[index];
    }
    to = (Uint8(*)[CHAO_NAME_SIZE])name;
    while (*from) {
        (*to)[i] = *from++;
        i++;
    }
    (*to)[i] = 0;
}

/* copy the letters of place's default name over the start of name, without
 * the null */
static void race_name_default(Uint8 *name, int place)
{
    int i = 0;
    /* @hack spelling: letter, a cursor of its own over the default name,
     * which the test reads through R5 while the copy indexes it from R14
     * (0x0cb91aa8 to 0x0cb91ab8); as one indexed loop, the function runs 2
     * bytes short and 14 of 20 words differ */
    const Uint8 *letter = default_names[place];

    for (; *letter != 0; i++, letter++) {
        name[i] = default_names[place][i];
    }
}

/* clear name, then copy from's letters into it, to from's null or
 * CHAO_NAME_SIZE letters */
/* @hack type: name as a pointer to the name array, which keeps it in R13
 * while the clearing walks R4 (0x0cb91aca, 0x0cb91ae8); as Uint8 *, 15 of
 * 28 words differ */
static void race_name_copy(Uint8 (*name)[CHAO_NAME_SIZE], Uint8 *from)
{
    int i;

    for (i = 0; i < CHAO_NAME_SIZE; i++) {
        (*name)[i] = 0;
    }
    /* @bug memory: after eight letters with no null, the test
     * reads from[8] (0x0cb91aea), past the name, before it tests i
     * (0x0cb91af4): for race_records_enter's chao->name, the low byte of
     * skills.swim, the field after it */
    for (i = 0; *from != 0 && i < CHAO_NAME_SIZE; i++) {
        (*name)[i] = *from++;
    }
}

/* set name's eighth byte to null, then draw its letters, each size on a
 * side. the first is centred on x and y, and each after it LETTER_GAP times
 * size past the one before */
static void race_name_draw(Uint8 *name, Float x, Float y, Float size)
{
    /* @hack type: name in a struct of its own, home, which SHC keeps at sp+4.
     * it stores name there after y's spill to sp, and loads it back into R14
     * (0x0cb91b14 to 0x0cb91b1e). as a scalar local, name and y trade places,
     * the function runs 4 bytes long and 28 of 39 words differ. with name
     * alone, it runs 4 bytes short and 31 differ */
    struct {
        Uint8 *name;
    } home;
    Uint8 *letter;
    Float step;
    Float extra;

    /* @hack redundant: taking home's address, which reaches it at @(4,R15)
     * (0x0cb91b16, 0x0cb91b1e), not through its address in R5; without it,
     * the function runs 4 bytes long and 33 of 39 words differ */
    (void)&home;
    home.name = name;
    extra = size * LETTER_GAP;
    home.name[7] = 0;
    letter = home.name;
    step = size + extra;
    while (*letter != 0) {
        Uint8 character = *letter;

        letter++;
        race_draw_letter(character, x, y, size);
        x += step;
    }
}

static void race_name_swap(Uint8 *first, Uint8 *second)
{
    Uint8 name[CHAO_NAME_SIZE];
    /* @hack spelling: first and second copied to one and two, which keeps them
     * in R12 and R14 (0x0cb91b5c, 0x0cb91b56), not in the frame; without
     * them, the function runs 4 bytes short and 23 of 26 words differ */
    Uint8 *one = first;
    Uint8 *two = second;

    njMemCopy(name, one, CHAO_NAME_SIZE);
    njMemCopy(one, two, CHAO_NAME_SIZE);
    njMemCopy(two, name, CHAO_NAME_SIZE);
}

void race_clock_clear(struct race_clock *clock)
{
    race_time_clear(&clock->time);
}

void race_clock_set_best(struct race_clock *clock, struct race_records *records,
                         int kind)
{
    clock->best = records->times[kind];
}

void race_clock_step(struct race_clock *clock)
{
    race_time_step(&clock->time);
}

void race_clock_draw(struct race_clock *clock)
{
    NJS_SPRITE sprite;
    int i;
    int frame;

    race_time_draw(&clock->time, &clock_style);
    if (clock->best) {
        race_time_draw(clock->best, &best_style);
    }
    sprite.p.z = 0.0F;
    sprite.ang = 0;
    sprite.tlist = &dat_0cbc089c;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&con_regular_texlist);
    for (i = 0; i < 2; i++) {
        sprite.tanim = (NJS_TEXANIM *)menu_sprites;
        sprite.sx = 1.0F;
        sprite.sy = 1.0F;
        sprite.p.x = clock_labels_at[i].x;
        sprite.p.y = clock_labels_at[i].y;
        if (i == 0) {
            frame = CLOCK_LABEL;
        } else {
            frame = BEST_LABEL;
        }
        njDrawSprite2D(&sprite, frame, -2.0F, NJD_SPRITE_ALPHA);
    }
}

void standings_init(struct race_standings *standings)
{
    int i;

    for (i = 0; i < RACE_LANES; i++) {
        standings->lanes[i] = i;
        standings->unk_08[i] = 0;
        if (i < TOP_PLACES) {
            standings->scales[i] = TOP_SCALE;
        } else {
            standings->scales[i] = OTHER_SCALE;
        }
    }
    standings->spin = 0;
    standings->marker_state = 0;
    standings->marker_lane = 1;
}

void standings_update(struct race_standings *standings, struct racer *racers,
                      int lane)
{
    int leaders[TOP_PLACES];
    int i;
    int j;
    int displaced;
    struct racer *racer;
    struct racer *other;
    Float rest;

    for (i = 0; i < TOP_PLACES; i++) {
        leaders[i] = standings->lanes[i];
    }
    for (i = 0; i < RACE_LANES - 1; i++) {
        racer = &racers[standings->lanes[i]];
        for (j = i + 1; j <= RACE_LANES - 1; j++) {
            other = &racers[standings->lanes[j]];
            if (racer_at_goal(racer)) {
                break;
            } else if (racer_at_goal(other) ||
                       (!racer_at_goal(other) &&
                        racer->distance < other->distance)) {
                displaced = standings->lanes[i];
                standings->lanes[i] = standings->lanes[j];
                standings->lanes[j] = displaced;
            }
        }
    }
    for (i = 0; i < TOP_PLACES; i++) {
        if (leaders[i] != standings->lanes[i]) {
            standings->scales[i] = PASSED_SCALE;
        }
    }
    for (i = 0; i < RACE_LANES; i++) {
        if (i < TOP_PLACES) {
            rest = TOP_SCALE;
        } else {
            rest = OTHER_SCALE;
        }
        if (standings->scales[i] > rest) {
            standings->scales[i] -= SCALE_STEP;
            if (standings->scales[i] < rest) {
                standings->scales[i] = rest;
            }
        }
    }
    standings->spin += SPIN_STEP;
    if (standings->spin > SPIN_END) {
        standings->spin = 0;
    }
    standings->timer++;
}

int standings_lane(struct race_standings *standings, int rank)
{
    if (rank >= 0 && rank < RACE_LANES) {
        return standings->lanes[rank];
    }
    return -1;
}

/* not static, though nothing calls it: SHC drops a static function nothing in
 * its file calls. the image holds it at 0x0cb92060 */
int standings_index_of(const Uint8 *entries, int key)
{
    int i;

    for (i = 0; i < 8; i++) {
        /* @hack type: entries plus i as integers, which adds them in R3 and
         * loads through it (0x0cb92064 to 0x0cb92068), not @(R0,R4); as
         * entries[i], the function runs 2 bytes short and 14 of 16 words
         * differ */
        if (*(const Uint8 *)((unsigned int)entries + i) == key) {
            return i;
        }
    }
    return -1;
}

void standings_draw(struct race_standings *standings, struct racer *racers,
                    int lane)
{
    NJS_SPRITE sprite;
    int i;
    int pointer;
    int finished;

    if (frame_step) {
        return;
    }
#ifdef GCC
    /* the port's 16:9 squeezes the board about the screen's left edge */
    port_wide_anchor = WIDE_LEFT;
#endif
    njSetTexture(&dat_0cbc089c);
    sprite.p.z = 0.0F;
    sprite.tlist = &dat_0cbc089c;
    sprite.tanim = lane_frames;
#ifdef GCC
    /* GCC can't see that every path sets pointer: lanes holds each lane
     * once, so the loop meets the lane the race follows */
    pointer = 0;
#endif
    for (i = 0; i < RACE_LANES; i++) {
        finished = 0;
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        sprite.sx = standings->scales[i];
        sprite.sy = standings->scales[i];
        sprite.p.x = standings_at[i].x;
        sprite.p.y = standings_at[i].y;
        sprite.ang = 0;
        if (standings->lanes[i] == lane) {
            pointer = i;
            if (standings->spin < SPIN_TURN &&
                racer_is_active(&racers[standings->lanes[i]])) {
                sprite.ang = standings->spin;
            }
            if (racer_at_goal(&racers[standings->lanes[i]]) ||
                racer_at_podium(&racers[standings->lanes[i]])) {
                finished = 1;
            } else {
                draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
            }
        } else {
            if (racer_at_goal(&racers[standings->lanes[i]]) ||
                racer_at_podium(&racers[standings->lanes[i]])) {
                finished = 1;
            } else {
                draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
            }
        }
        njDrawSprite2D(&sprite, standings->lanes[i],
                       -2.0F + (0.001F * standings->scales[i]),
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR | NJD_SPRITE_ANGLE);
        if (finished == 1) {
            sprite.p.x += 44.0F;
            njDrawSprite2D(&sprite, FINISHED_FRAME, -2.0F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR |
                               NJD_SPRITE_ANGLE);
        }
    }
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    sprite.p.z = 0.0F;
    sprite.tanim = pointer_frames;
    sprite.p.x = 40.0F;
    sprite.p.y = standings_at[pointer].y;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.ang = NJM_DEG_ANG(270);
    if (standings->timer & POINTER_BLINK) {
        i = 1;
    } else {
        i = 0;
    }
    njDrawSprite2D(&sprite, i, -2.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR | NJD_SPRITE_ANGLE);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}

void standings_draw_marker(struct race_standings *standings, int marker_lane,
                           NJS_POINT3 *pos)
{
    NJS_SPRITE sprite;
    Float drag;
    Float pull;
    Float gap;
    int kick;

    if (standings->marker_lane != marker_lane) {
        standings->marker_state = 0;
        standings->marker_lane = marker_lane;
    }
    switch (standings->marker_state) {
    case 0:
        standings->marker_y = pos->y;
        standings->marker_speed = 0.0F;
        standings->marker_state = 1;
        break;
    /* @hack redundant: case 1, which only breaks, for which the switch tests
     * marker_state against 1 (CMP/EQ #1 at 0x0cb92820); without it, 118 of
     * 211 words differ */
    case 1:
        break;
    }
    if (_builtin_fabsf(standings->marker_speed) < 0.1F) {
        kick = (int)RANDOM(0.0F, 180.0F);
        if (kick == 0) {
            standings->marker_speed += 0.05F;
        } else if (kick == 1) {
            standings->marker_speed -= 0.05F;
        }
    }
    gap = pos->y - standings->marker_y;
    if (_builtin_fabsf(gap) > 4.0F) {
        drag = 0.06F;
    } else {
        drag = 0.02F;
    }
    pull = (pos->y - standings->marker_y) * 0.15F;
    standings->marker_speed += pull;
    standings->marker_speed -= standings->marker_speed * drag;
    if (standings->marker_speed > 0.25F) {
        standings->marker_speed = 0.25F;
    } else if (standings->marker_speed < -0.25F) {
        standings->marker_speed = -0.25F;
    }
    standings->marker_y += standings->marker_speed;
    gap = pos->y - standings->marker_y;
    if (gap > 5.0F) {
        standings->marker_y = pos->y - 5.0F;
    } else if (gap < -5.0F) {
        standings->marker_y = pos->y + 5.0F;
    }
    sprite.p.x = pos->x;
    sprite.p.y = standings->marker_y;
    sprite.p.z = pos->z;
    sprite.ang = 0;
    sprite.tlist = &dat_0cbc089c;
    sprite.tanim = pointer_frames;
    sprite.sx = 0.1F;
    sprite.sy = 0.1F;
    njSetTexture(&dat_0cbc089c);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(0.8F, 1.0F, 1.0F, 0.6F);
    if (!frame_step) {
        njDrawSprite3D(&sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* set records up: copy mark's MARK_SIZE bytes, clear the flags, and give each
 * kind's best times their default minutes. each best time's name takes its
 * place's default name's letters over its start */
static void race_records_reset(struct race_records *records, const char *mark)
{
    char *to;
    int i;
    int kind;
    int place;

    records->flags = 0;
    to = (char *)records->mark;
    for (i = 0; i < MARK_SIZE; i++) {
        *to = *mark;
        to++;
        mark++;
    }
    for (kind = 0; kind < CHAO_RACES; kind++) {
        for (place = 0; place < RACE_BEST; place++) {
            race_time_set(&records->times[kind][place], place + DEFAULT_MINUTES,
                          0, 0);
            race_name_default(records->names[kind][place], place);
        }
    }
}

void race_records_reset_unmarked(struct race_records *records)
{
    int i;
    char *current = (char *)records->mark;
    const char *expected = records_mark;

    for (i = 0; i < MARK_SIZE; i++, expected++, current++) {
        if (*expected != *current) {
            race_records_reset(records, records_mark);
            return;
        }
    }
}

void race_records_enter(struct race_records *records, struct race_time *time,
                        struct chao *chao, int kind)
{
    int place;

    if (race_time_compare(&records->times[kind][RACE_BEST - 1], time) != 1) {
        return;
    }
    records->times[kind][RACE_BEST - 1] = *time;
    race_name_copy(&records->names[kind][RACE_BEST - 1], chao->name);
    for (place = RACE_BEST - 1; place >= 1; place--) {
        if (race_time_compare(&records->times[kind][place - 1],
                              &records->times[kind][place]) != 1) {
            break;
        }
        race_time_swap(&records->times[kind][place - 1],
                       &records->times[kind][place]);
        race_name_swap(records->names[kind][place - 1],
                       records->names[kind][place]);
    }
}

void race_records_draw(struct race_records *records, int kind)
{
    struct time_style style;
    int place;
    int frame;

    style.x = 200.0F;
    style.y = 278.0F;
    style.sx = 0.6F;
    style.sy = 0.6F;
    style.digit_step = 22.0F;
    style.separator_step = 15.0F;
    style.set = 1;
    for (place = 0, frame = 3; place < RACE_BEST; place++) {
        race_time_draw(&records->times[kind][place], &style);
        race_name_draw(records->names[kind][place], style.x + 170.0F, style.y,
                       14.0F);
        menu_draw_sprite_at(style.x - 60.0F, style.y, 1.0F, frame);
        style.y += 25.0F;
        frame++;
    }
}

void race_draw_digit(struct race_digit *digit, Int value, int set)
{
    NJS_SPRITE sprite;

    sprite.p.z = 0.0F;
    sprite.ang = 0;
    sprite.tlist = &con_regular_texlist;
    if (set) {
        sprite.tanim = digit_set_1;
    } else {
        sprite.tanim = digit_set_0;
    }
    sprite.sx = digit->sx;
    sprite.sy = digit->sy;
    sprite.p.x = digit->x;
    sprite.p.y = digit->y;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&con_regular_texlist);
    draw_set_material(digit->color.a, digit->color.r, digit->color.g,
                      digit->color.b);
    njDrawSprite2D(&sprite, value, -2.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
