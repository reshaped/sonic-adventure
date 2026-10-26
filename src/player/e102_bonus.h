/* E-102's time bonuses, seconds shown on a sign and then added to his time,
 * and the strips his lock-ons' cubes draw */
#ifndef PLAYER_E102_BONUS_H
#define PLAYER_E102_BONUS_H
#include <ninja.h>
#include <sg_xpt.h>

struct lockon_strip;
struct task;

/* the work of a bonus, the time bonus's or a volley bonus's */
struct bonus_work {
    Sint8 mode;
    Uint8 unk_01[3];
    Sint16 flags;
    Uint8 unk_06[2];
    Sint32 unk_08;
    /* @hack type: a volley bonus's two counts as an array, which VOLLEY_SHOTS
     * and VOLLEY_HITS index. e102_bonus_step's test of both reaches them
     * through one ADD #12 (0x8c0de5fa), and work takes R13. as two fields,
     * work and extra trade R13 and R14, and 80 of 235 words differ */
    Sint8 volley[2];
    Uint8 unk_0e[18];
    NJS_POINT3 pos; /* its sign's place on the screen */
    Float scale; /* its sign's */
    Float shrink; /* added to scale each step as it leaves, so negative */
};

#define BONUS(task) ((struct bonus_work *)(task)->work)

#define VOLLEY_SHOTS 0 /* shots still to land */
#define VOLLEY_HITS 1 /* hits not yet counted */

/* E102TIME's textures, and the sprite that draws them. a texanim index of
 * 0 to 9 draws that digit, and the E102TIME_ indexes below draw the rest */
extern NJS_TEXLIST e102time_texlist;
extern NJS_SPRITE e102time_sprite;

#define E102TIME_COLON 10
#define E102TIME_GETSECBG 11
#define E102TIME_LOCKON0 12
#define E102TIME_LASTSEC 13

/* draw number, 0 to 99, as two digits at e102time_sprite's position, and
 * move the position past them */
void e102_draw_two_digits(int number);
/* make the time bonus, a child of parent. setting its extra's ready makes
 * it count the seconds in pending onto its sign, and later add them to the
 * time */
void e102_time_bonus_create(struct task *parent);
/* count a hit toward the running volley bonus, or a new one if none runs.
 * if none can be made, a second goes onto the time at once. unused is
 * ignored */
void e102_bonus_hit(Sint8 unused);
/* count a hit toward bonus. with bonus NULL, it counts toward the running
 * volley bonus, or a new one if none runs, and as a shot of its own too. if
 * none can be made, a second goes onto the time at once */
void e102_bonus_count(struct task *bonus);
/* the step of a volley bonus. e102_shoot starts one for each volley, with
 * VOLLEY_SHOTS set */
void e102_bonus_step(struct task *task);
/* draw the count segments of strip's lines, with attr */
void e102_draw_strip(struct lockon_strip *strip, Uint32 attr);
/* copy the red cube's strip into inner and the yellow cube's into outer,
 * and build their lines on the heap */
void e102_make_strips(struct lockon_strip *inner, struct lockon_strip *outer);
/* free the lines e102_make_strips built for both strips */
void e102_free_strips(struct lockon_strip *inner, struct lockon_strip *outer);
/* free the lines e102_make_strips built for strip */
void e102_free_strip(struct lockon_strip *strip);

#endif
