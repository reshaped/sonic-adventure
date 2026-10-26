/* E-102's time bonuses, seconds shown on a sign and then added to his time,
 * and the strips his lock-ons' cubes draw */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "hud/hud.h"
#include "player/e102.h"
#include "player/e102_bonus.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 13
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 11
#include "player/e102_bonus/text.inc"

#define SE_BONUS_GOES 22

/* in a bonus's work flags: its draw shows its sign. the time bonus sets it
 * on each step it shows, and a volley bonus from its first hit on */
#define BONUS_SHOWN 0x100

/* where a bonus's sign shows on the screen, and how far it moves each step
 * as it shrinks away */
#define BONUS_X 540.0F
#define BONUS_Y 120.0F
#define BONUS_LEAVE_X (-14.0F)
#define BONUS_LEAVE_Y (-3.0F)

/* the steps of a sign's pulse, and the sign's scale till it shrinks away. a
 * volley bonus's sign pulses at each hit, and the time bonus's when its ready
 * is set */
#define BONUS_PULSE 15
#define BONUS_SCALE 1.15F

/* the steps the time bonus holds its sign after its last pulse */
#define HOLD_STEPS (4 * 60)

/* the steps a volley bonus waits for a hit before it ends, and the steps
 * its sign then waits before it shrinks away */
#define HIT_WAIT (5 * 60)
#define LEAVE_PAUSE 60

#define HIT_SECONDS 5 /* the most seconds a hit is worth */
/* as a volley bonus leaves, up to LATE_HITS shots still to land count as
 * hits */
#define LATE_HITS 6

#define CUBE_EDGES 12

enum time_bonus_mode {
    TIME_BONUS_INIT,
    TIME_BONUS_IDLE,
    TIME_BONUS_RESTART,
    TIME_BONUS_COUNT,
    TIME_BONUS_HOLD,
    TIME_BONUS_LEAVE
};

enum bonus_mode {
    BONUS_INIT,
    BONUS_HIT,
    BONUS_WAIT,
    BONUS_PULSING,
    BONUS_END,
    BONUS_PAUSE,
    BONUS_LEAVE
};

/* a tally: the task that adds a bonus's seconds to the time */
struct tally_vars {
    Uint8 unk_00[4];
    Sint16 seconds; /* seconds left to add */
    Sint16 delay; /* steps until the next second */
};

#define TALLY(work) ((struct tally_vars *)&(work)->vars)

struct time_bonus_extra {
    Sint8 ready; /* set when seconds wait in pending */
    Sint8 pending; /* seconds still to count onto the sign */
    Sint16 count; /* the seconds on the sign */
    Sint16 pulse; /* steps left of the sign's pulse */
    Sint16 timer;
};

struct bonus_extra {
    Sint32 pulse; /* steps left of the sign's pulse */
    Sint32 timer;
    Sint32 waited; /* steps it has waited for a hit */
    Sint32 worth; /* what the last hit was worth, in seconds */
    Sint32 total; /* the seconds on the sign */
};

static void tally_step(struct task *task);
static void time_bonus_step(struct task *task);

static NJS_TEXNAME e102time_textures[] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
};

NJS_TEXLIST e102time_texlist = {
#include "player/e102_bonus/e102time_texlist.inc"
};

/* the digits 0 to 9 and the colon, 24 pixels square, then the three
 * signs, 64 */
static NJS_TEXANIM e102time_texanims[14] = {
#include "player/e102_bonus/e102time_texanims.inc"
};

NJS_SPRITE e102time_sprite = {
#include "player/e102_bonus/e102time_sprite.inc"
};

/* the time bonus while it runs, else NULL */
static struct task *time_bonus_task = NULL;
/* the volley bonus that new hits go to, or NULL. each volley bonus takes it on
 * its first step. each one sets it to NULL as it ends, even if a newer one has
 * taken it since */
static struct task *volley_bonus = NULL;

static NJS_POINT3 cube_corners[8] = {
#include "player/e102_bonus/cube_corners.inc"
};
static NJS_COLOR cube_red[8] = {
#include "player/e102_bonus/cube_red.inc"
};
static NJS_COLOR cube_yellow[8] = {
#include "player/e102_bonus/cube_yellow.inc"
};
static Sint16 cube_edges[CUBE_EDGES * 2] = {
#include "player/e102_bonus/cube_edges.inc"
};

/* his lock-ons' cubes, 10 across, which e102_make_strips copies into his
 * record: the inner red and the outer yellow */
static struct lockon_strip inner_cube = {
    cube_corners, cube_red, cube_edges, CUBE_EDGES, {NULL, NULL, NULL, 0}};
static struct lockon_strip outer_cube = {
    cube_corners, cube_yellow, cube_edges, CUBE_EDGES, {NULL, NULL, NULL, 0}};

void e102_draw_two_digits(int number)
{
    njDrawSprite2D(&e102time_sprite, number / 10, -2.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    e102time_sprite.p.x += 24.0F;
    njDrawSprite2D(&e102time_sprite, number % 10, -2.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    e102time_sprite.p.x += 24.0F;
}

void e102_time_bonus_create(struct task *parent)
{
    time_bonus_task = task_create_child(TASK_WORK, time_bonus_step, parent);
}

/* number in yellow at scale, centered on pos->x a little above pos->y. it
 * draws from the last digit leftward, and 0 draws nothing */
static void draw_number(int number, NJS_POINT3 *pos, Float scale)
{
    Float width = scale * 24.0F;

    e102time_sprite.p.x = pos->x;
    if (number > 99) {
        e102time_sprite.p.x += width;
    } else if (number > 9) {
        e102time_sprite.p.x += 0.5F * width;
    }
    e102time_sprite.p.y = pos->y - (width * 0.3F);
    e102time_sprite.sy = scale;
    e102time_sprite.sx = scale;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 1.0F, 1.0F, 0.0F);
    while (number > 0) {
        njDrawSprite2D(&e102time_sprite, number % 10, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        e102time_sprite.p.x -= width;
        number /= 10;
    }
    e102time_sprite.sy = 1.0F;
    e102time_sprite.sx = 1.0F;
}

/* add a bonus's seconds to the time, one every four steps with a beep.
 * once they're in, it sets the time running */
static void tally_step(struct task *task)
{
    struct task_work *work = task->work;

    if (TALLY(work)->delay > 0) {
        TALLY(work)->delay--;
    } else {
        TALLY(work)->delay = 3;
        if (TALLY(work)->seconds > 0) {
            TALLY(work)->seconds--;
            time_add_seconds(1);
            sound_play(SE_BEEP, 0, 0, 0);
        }
        if (!TALLY(work)->seconds) {
            e102_time_runs = TRUE;
            task_destroy(task);
        }
    }
}

/* draw the sign and its seconds, on a frame's first step while BONUS_SHOWN is
 * set. the sign grows at 6, 15 and 25 seconds, and a pulse swells it by up to
 * half. at 100 seconds or more, the sign draws 1.15 times the size of its
 * digits */
static void time_bonus_draw(struct task *task)
{
    struct bonus_work *work = BONUS(task);
    struct time_bonus_extra *extra = task->extra;
    Float scale;
    Float size;

    if (frame_step != 0) {
        return;
    }
    if (!(work->flags & BONUS_SHOWN)) {
        return;
    }
    if (extra->count >= 25) {
        scale = 1.75F;
    } else if (extra->count >= 15) {
        scale = 1.5F;
    } else if (extra->count >= 6) {
        scale = 1.25F;
    } else {
        scale = 1.0F;
    }
    scale = scale + (scale * njSin(extra->pulse * 0x8000 / BONUS_PULSE) * 0.5F);
    scale *= work->scale;
    size = scale;
    if (extra->count >= 100) {
        size *= 1.15F;
    }
    e102time_sprite.sy = size;
    e102time_sprite.sx = size;
    e102time_sprite.p.x = work->pos.x;
    e102time_sprite.p.y = work->pos.y;
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njDrawSprite2D(&e102time_sprite, E102TIME_GETSECBG, -3.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    draw_number(extra->count, &work->pos, scale);
}

static void time_bonus_destroy(struct task *task)
{
    time_bonus_task = NULL;
}

/* the time bonus waits for its extra's ready to be set, which stops the time
 * and starts a pulse of BONUS_PULSE steps. in the pulse, the pending seconds
 * count onto the sign, one every two steps. HOLD_STEPS after the last pulse,
 * the sign shrinks away. once it's gone, or ready is set again as it goes, a
 * tally adds its seconds to the time */
static void time_bonus_step(struct task *task)
{
    struct bonus_work *work = BONUS(task);
    struct time_bonus_extra *extra = task->extra;
    struct task *tally;

    work->flags &= ~BONUS_SHOWN;
    switch (work->mode) {
    case TIME_BONUS_INIT:
        extra = task->extra = heap_calloc(1, sizeof(struct time_bonus_extra));
        work->unk_08 = 0;
        task->draw = time_bonus_draw;
        task->destructor = time_bonus_destroy;
        work->scale = BONUS_SCALE;
        work->mode = TIME_BONUS_IDLE;
        /* fall through */
    case TIME_BONUS_IDLE:
        if (extra->ready > 0) {
            e102_time_runs = FALSE;
            extra->count = 0;
            work->mode = TIME_BONUS_RESTART;
            work->pos.x = BONUS_X;
            work->pos.y = BONUS_Y;
        }
        break;
    case TIME_BONUS_RESTART:
        extra->pulse = BONUS_PULSE;
        work->scale = BONUS_SCALE;
        extra->timer = 0;
        work->mode = TIME_BONUS_COUNT;
        /* fall through */
    case TIME_BONUS_COUNT:
        if (extra->pending > 0) {
            extra->timer--;
            if (extra->timer <= 0) {
                extra->timer = 2;
                extra->pending--;
                extra->count++;
            }
        }
        extra->pulse--;
        if (extra->pulse <= 0) {
            extra->ready = 0;
            extra->timer = HOLD_STEPS;
            work->mode = TIME_BONUS_HOLD;
        }
        break;
    case TIME_BONUS_HOLD:
        if (extra->ready > 0) {
            work->mode = TIME_BONUS_RESTART;
            break;
        }
        extra->timer--;
        if (extra->timer <= 0) {
            sound_play(SE_BONUS_GOES, 0, 0, 0);
            work->shrink = -(work->scale * (1.0F / 30.0F));
            work->mode = TIME_BONUS_LEAVE;
        }
        break;
    case TIME_BONUS_LEAVE:
        work->pos.x += BONUS_LEAVE_X;
        work->pos.y += BONUS_LEAVE_Y;
        work->scale += work->shrink;
        if (work->scale <= 0.0F || extra->ready > 0) {
            tally = task_create(TASK_WORK, 1, tally_step);
            TALLY(tally->work)->seconds = extra->count;
            work->mode = TIME_BONUS_IDLE;
        }
        break;
    }
    if (work->mode > TIME_BONUS_IDLE && hud_hidden >= 0) {
        work->flags |= BONUS_SHOWN;
        time_bonus_draw(task);
    }
}

void e102_bonus_hit(Sint8 unused)
{
    struct task *task;

    if (volley_bonus == NULL) {
        task = task_create(TASK_WORK, 2, e102_bonus_step);
        if (task == NULL) {
            time_add_seconds(1);
            sound_play(SE_BEEP, 0, 0, 0);
            return;
        }
    } else {
        task = volley_bonus;
    }
    BONUS(task)->volley[VOLLEY_HITS]++;
}

void e102_bonus_count(struct task *bonus)
{
    if (bonus == NULL) {
        if (volley_bonus == NULL) {
            bonus = task_create(TASK_WORK, 2, e102_bonus_step);
            if (bonus == NULL) {
                time_add_seconds(1);
                sound_play(SE_BEEP, 0, 0, 0);
                return;
            }
        } else {
            bonus = volley_bonus;
        }
        BONUS(bonus)->volley[VOLLEY_SHOTS]++;
    }
    BONUS(bonus)->volley[VOLLEY_HITS]++;
}

/* a volley bonus's sign, drawn as time_bonus_draw draws the time bonus's */
static void bonus_draw(struct task *task)
{
    struct bonus_work *work = BONUS(task);
    struct bonus_extra *extra = task->extra;
    Float scale;
    Float size;

    if (frame_step != 0) {
        return;
    }
    if (!(work->flags & BONUS_SHOWN)) {
        return;
    }
    if (extra->total >= 25) {
        scale = 1.75F;
    } else if (extra->total >= 15) {
        scale = 1.5F;
    } else if (extra->total >= 6) {
        scale = 1.25F;
    } else {
        scale = 1.0F;
    }
    scale = scale + (scale * njSin(extra->pulse * 0x8000 / BONUS_PULSE) * 0.5F);
    scale *= work->scale;
    size = scale;
    if (extra->total >= 100) {
        size *= 1.15F;
    }
    e102time_sprite.sy = size;
    e102time_sprite.sx = size;
    e102time_sprite.p.x = work->pos.x;
    e102time_sprite.p.y = work->pos.y;
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawSprite2D(&e102time_sprite, E102TIME_GETSECBG, -3.0F,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    draw_number(extra->total, &work->pos, scale);
}

static void bonus_destroy(struct task *task)
{
    volley_bonus = NULL;
    e102_time_runs = TRUE;
}

/* a volley bonus stops the time while it runs. it counts a hit only while a
 * shot is still to land, and then takes one off each count. each hit it counts
 * is worth a second more than the last, up to HIT_SECONDS. after HIT_WAIT steps
 * with no hit, or once no shot is left to land, the sign waits LEAVE_PAUSE
 * steps and shrinks away. then up to LATE_HITS shots still to land count as
 * hits, and a tally adds the seconds to the time */
void e102_bonus_step(struct task *task)
{
    struct bonus_work *work = BONUS(task);
    struct bonus_extra *extra = task->extra;
    struct task *tally;
    int left;

    e102_time_runs = FALSE;
    switch (work->mode) {
    case BONUS_INIT:
        volley_bonus = task;
        /* @hack spelling: the result tested through task->extra, and extra
         * loaded from it after, which tests the result in R3 (0x8c0de5ce) and
         * loads extra again from the task (0x8c0de5de). assigned to extra and
         * tested there, the function runs 32 bytes long, and 219 of 235 words
         * differ */
        task->extra = heap_calloc(1, sizeof(struct bonus_extra));
        if (task->extra == NULL) {
            task_destroy(task);
            return;
        }
        extra = task->extra;
        task->draw = bonus_draw;
        task->destructor = bonus_destroy;
        work->pos.x = BONUS_X;
        work->pos.y = BONUS_Y;
        extra->waited = 0;
        work->mode = BONUS_WAIT;
        /* fall through */
    case BONUS_WAIT:
        if (work->volley[VOLLEY_SHOTS] > 0 && work->volley[VOLLEY_HITS] > 0) {
            work->mode = BONUS_HIT;
        }
        extra->waited++;
        if (extra->waited >= HIT_WAIT) {
            work->mode = BONUS_END;
        }
        break;
    case BONUS_HIT:
        work->flags |= BONUS_SHOWN;
        work->volley[VOLLEY_HITS]--;
        work->volley[VOLLEY_SHOTS]--;
        extra->waited = 0;
        extra->worth =
            extra->worth + 1 < HIT_SECONDS ? extra->worth + 1 : HIT_SECONDS;
        extra->total += extra->worth;
        extra->pulse = BONUS_PULSE;
        extra->timer = 0;
        work->scale = BONUS_SCALE;
        work->mode = BONUS_PULSING;
        /* fall through */
    case BONUS_PULSING:
        extra->pulse--;
        if (extra->pulse > 0) {
            break;
        }
        if (work->volley[VOLLEY_SHOTS] > 0) {
            work->mode = BONUS_WAIT;
        } else {
            work->mode = BONUS_END;
        }
        break;
    case BONUS_END:
        extra->timer = LEAVE_PAUSE;
        work->shrink = -(work->scale * (1.0F / 30.0F));
        work->mode = BONUS_PAUSE;
        /* fall through */
    case BONUS_PAUSE:
        extra->timer--;
        if (extra->timer > 0) {
            break;
        }
        work->mode = BONUS_LEAVE;
        break;
    case BONUS_LEAVE:
        work->pos.x += BONUS_LEAVE_X;
        work->pos.y += BONUS_LEAVE_Y;
        work->scale += work->shrink;
        if (work->scale > 0.0F) {
            break;
        }
        left = work->volley[VOLLEY_SHOTS] < LATE_HITS
                   ? work->volley[VOLLEY_SHOTS]
                   : LATE_HITS;
        /* @hack spelling: left-- at the top of the body, which the image runs
         * in the delay slot of the BT/S that picks worth (0x8c0de72a). as a for
         * loop's step, it comes after the pick, and 8 of 235 words differ */
        while (left > 0) {
            left--;
            extra->worth =
                extra->worth + 1 < HIT_SECONDS ? extra->worth + 1 : HIT_SECONDS;
            extra->total += extra->worth;
        }
        tally = task_create(TASK_WORK, 1, tally_step);
        TALLY(tally->work)->seconds = extra->total;
        task_destroy(task);
        return;
    /* @hack redundant: a default that only breaks, whose deleted jump keeps
     * hud_hidden's address in R2 and its byte in R3 (0x8c0de762,
     * 0x8c0de764). without it, the two trade and 3 of 235 words differ */
    default:
        break;
    }
    if (hud_hidden >= 0) {
        bonus_draw(task);
    }
}

/* build strip->lines on the heap: for each segment, the two points its
 * indices name, with their colors */
static void strip_make(struct lockon_strip *strip)
{
    NJS_POINT3COL *lines = &strip->lines;
    Sint16 *indices = strip->indices;
    NJS_POINT3 *points = strip->points;
    NJS_COLOR *colors = strip->colors;
    int count = strip->count * 2;
    NJS_POINT3 *point;
    NJS_COLOR *color;
    int index;
    int i;

    point = lines->p = heap_alloc(count * sizeof(NJS_POINT3));
    color = lines->col = heap_alloc(count * sizeof(NJS_COLOR));
    lines->tex = NULL;
    lines->num = count;
    for (i = 0; i < count; i++) {
        index = *indices++;
        *point++ = points[index];
        *color++ = colors[index];
    }
}

/* SHC writes its body into e102_free_strips, its one caller. the image also
 * keeps this copy, at 0x8c0de844, which nothing calls. it isn't static, since
 * SHC would expand a static at its calls and write no copy of its own */
void e102_free_strip(struct lockon_strip *strip)
{
    heap_free(strip->lines.p);
    heap_free(strip->lines.col);
}

void e102_draw_strip(struct lockon_strip *strip, Uint32 attr)
{
    njDrawLine3D(&strip->lines, strip->count, attr);
}

void e102_make_strips(struct lockon_strip *inner, struct lockon_strip *outer)
{
    *inner = inner_cube;
    strip_make(inner);
    *outer = outer_cube;
    strip_make(outer);
}

void e102_free_strips(struct lockon_strip *inner, struct lockon_strip *outer)
{
    e102_free_strip(inner);
    e102_free_strip(outer);
}
