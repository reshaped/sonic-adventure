/* the tutorial's back. each character's back loads one PVM into a texlist
 * of its own. each of its two pages is a picture of textures from that
 * texlist. every back draws its titles and bar from tutorial_title_texlist */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/screen.h"
#include "summary/tutorial.h"
#include "summary/tutorial_back.h"
#include "system/math.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 16
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 8
#define TEXT_24_LENGTH 8
#define TEXT_25_LENGTH 8
#define TEXT_26_LENGTH 8
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 8
#define TEXT_29_LENGTH 8
#include "summary/tutorial_back/text.inc"

/* the textures of tutorial_title_texlist: the bar, then a title for each
 * page */
enum { TITLE_BAR, TITLE_GAME, TITLE_CONTROLS };

/* the pictures of each character's game page and controls page. each
 * array is laid out as struct screen_parts: its count, then each part's
 * texture, x and y */
/* clang-format off */
static const Sint16 sonic_game_parts[] = {
    2,
    0, 0, 0,
    1, 256, 0,
};
static const Sint16 sonic_controls_parts[] = {
    2,
    2, 0, 0,
    3, 256, 0,
};
static const Sint16 tails_game_parts[] = {
    2,
    2, 0, 0,
    3, 256, 0,
};
static const Sint16 tails_controls_parts[] = {
    2,
    0, 0, 0,
    1, 256, 0,
};
static const Sint16 knuckles_game_parts[] = {
    2,
    2, 0, 0,
    3, 256, 0,
};
static const Sint16 knuckles_controls_parts[] = {
    2,
    0, 0, 0,
    1, 256, 0,
};
static const Sint16 amy_game_parts[] = {
    4,
    2, 0, 0,
    3, 256, 0,
    4, 0, 256,
    5, 256, 256,
};
static const Sint16 amy_controls_parts[] = {
    2,
    0, 0, 0,
    1, 256, 0,
};
static const Sint16 big_game_parts[] = {
    2,
    2, 0, 0,
    3, 256, 0,
};
static const Sint16 big_controls_parts[] = {
    2,
    0, 0, 0,
    1, 256, 0,
};
static const Sint16 e102_game_parts[] = {
    2,
    0, 0, 0,
    1, 256, 0,
};
static const Sint16 e102_controls_parts[] = {
    2,
    2, 0, 0,
    3, 256, 0,
};
/* clang-format on */

/* each character's back PVM, by story */
static char *back_files[TUTORIALS] = {
    "tutobg_sonic", "tutobg_tails", "tutobg_knuckles",
    "tutobg_amy",   "tutobg_big",   "tutobg_e102",
};
Uint32 tutorial_back_colors[TUTORIALS] = {
    0xff8cc8ff, 0xffffffb4, 0xffffbed2, 0xffffbed2, 0xffdcc8ff, 0xffbebebe,
};
static NJS_TEXNAME tutorial_title_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST tutorial_title_texlist = {
#include "summary/tutorial_back/tutorial_title_texlist.inc"
};
static NJS_TEXNAME sonic_back_textures[4] = {
    {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},
    {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},
};
static NJS_TEXLIST sonic_back_texlist = {
#include "summary/tutorial_back/sonic_back_texlist.inc"
};
static NJS_TEXNAME tails_back_textures[4] = {
    {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},
    {TEXT_10, 0, 0},
    {TEXT_11, 0, 0},
};
static NJS_TEXLIST tails_back_texlist = {
#include "summary/tutorial_back/tails_back_texlist.inc"
};
static NJS_TEXNAME knuckles_back_textures[4] = {
    {TEXT_12, 0, 0},
    {TEXT_13, 0, 0},
    {TEXT_14, 0, 0},
    {TEXT_15, 0, 0},
};
static NJS_TEXLIST knuckles_back_texlist = {
#include "summary/tutorial_back/knuckles_back_texlist.inc"
};
static NJS_TEXNAME amy_back_textures[6] = {
    {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
    {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0},
};
static NJS_TEXLIST amy_back_texlist = {
#include "summary/tutorial_back/amy_back_texlist.inc"
};
static NJS_TEXNAME e102_back_textures[4] = {
    {TEXT_22, 0, 0},
    {TEXT_23, 0, 0},
    {TEXT_24, 0, 0},
    {TEXT_25, 0, 0},
};
static NJS_TEXLIST e102_back_texlist = {
#include "summary/tutorial_back/e102_back_texlist.inc"
};
static NJS_TEXNAME big_back_textures[4] = {
    {TEXT_26, 0, 0},
    {TEXT_27, 0, 0},
    {TEXT_28, 0, 0},
    {TEXT_29, 0, 0},
};
static NJS_TEXLIST big_back_texlist = {
#include "summary/tutorial_back/big_back_texlist.inc"
};
static NJS_TEXLIST *back_texlists[TUTORIALS] = {
    &sonic_back_texlist, &tails_back_texlist, &knuckles_back_texlist,
    &amy_back_texlist,   &big_back_texlist,   &e102_back_texlist,
};
static const struct screen_parts *game_pages[TUTORIALS] = {
    (const struct screen_parts *)sonic_game_parts,
    (const struct screen_parts *)tails_game_parts,
    (const struct screen_parts *)knuckles_game_parts,
    (const struct screen_parts *)amy_game_parts,
    (const struct screen_parts *)big_game_parts,
    (const struct screen_parts *)e102_game_parts,
};
static const struct screen_parts *controls_pages[TUTORIALS] = {
    (const struct screen_parts *)sonic_controls_parts,
    (const struct screen_parts *)tails_controls_parts,
    (const struct screen_parts *)knuckles_controls_parts,
    (const struct screen_parts *)amy_controls_parts,
    (const struct screen_parts *)big_controls_parts,
    (const struct screen_parts *)e102_controls_parts,
};

void tutorial_whiten_back_color(int unused)
{
    njSetBackColor(0xffffffff, 0xffffffff, 0xffffffff);
}

NJS_TEXLIST *tutorial_back_load(int story)
{
    char *file = back_files[story];
    NJS_TEXLIST *texlist = back_texlists[story];

    texture_load_pvm(file, texlist);
    return texlist;
}

/* draw the page shown and its title. the top left of the page's picture
 * is at work->pos. during a turn, the two fade in as the page before it and
 * its title fade out. the bar and a green band behind the titles draw last */
static void tutorial_back_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct tutorial_back *back = task->extra;
    NJS_ARGB argb;
    int titles[2] = {TITLE_CONTROLS, TITLE_GAME}; /* by page */
    Uint32 shown_color;
    Uint32 before_color;
    Float x;
    Float y;
    Float z = 100.0F;
    Float band_y;

    screen_blend_alpha();
    argb.a = back->blend;
    argb.r = 1.0F;
    argb.g = 1.0F;
    argb.b = 1.0F;
    shown_color = color_multiply(back->color, color_from_argb_unit(&argb));
    argb.a = 1.0F - back->blend;
    before_color = color_multiply(back->color, color_from_argb_unit(&argb));
    njSetTexture(back->texlist);
    screen_quad_tint(0xffffffff);
    if (back->blend >= 1.0F) {
        screen_quad_color(0xffffffff);
        screen_parts_draw(back->pages[back->shown], work->pos.x, work->pos.y,
                          work->pos.z);
    } else {
        screen_quad_color(shown_color);
        screen_parts_draw(back->pages[back->shown], work->pos.x, work->pos.y,
                          work->pos.z - 10.0F);
        screen_quad_color(before_color);
        screen_parts_draw(back->pages[back->before], work->pos.x, work->pos.y,
                          work->pos.z);
    }
    njSetTexture(&tutorial_title_texlist);
    /* @hack spelling: x, y and band_y, locals that hold 32, 42 and 32. x and y
     * place the titles, and band_y and y give the band's top and height. SHC
     * keeps 32 in FR12, 42 in FR14 and z in FR13 (0x8c9013c0, 0x8c9013c4 and
     * 0x8c9012fa). without it, with literals in their place, 42 takes FR12,
     * 32 FR13 and z FR14, and 12 of 194 words differ */
    band_y = x = 32.0F;
    y = 42.0F;
    if (back->blend >= 1.0F) {
        screen_quad_color(0xffffffff);
        screen_quad_draw(titles[back->shown], x, y, z - 10.0F);
    } else {
        screen_quad_color(shown_color);
        screen_quad_draw(titles[back->shown], x, y, z - 10.0F);
        screen_quad_color(before_color);
        screen_quad_draw(titles[back->before], x, y, z - 20.0F);
    }
    screen_quad_color(0xffffffff);
    screen_quad_draw(TITLE_BAR, 288.0F, 400.0F, z);
    screen_band_draw(0.0F, band_y, z, 565.0F, y);
}

static void tutorial_back_step(struct task *task)
{
    struct task_work *work = task->work;
    struct tutorial_back *back = task->extra;

    switch (work->mode) {
    case BACK_STILL:
        back->blend = 1.0F;
        break;
    case BACK_TURN_START:
        back->blend = 0.0F;
        work->mode = BACK_TURNING;
        break;
    case BACK_TURNING:
        back->blend += back->speed;
        if (back->blend >= 1.0F) {
            back->blend = 1.0F;
            work->mode = BACK_STILL;
        }
        break;
    }
}

struct task *tutorial_back_create(int level, int story, NJS_POINT3 *pos)
{
    /* where the back sits when pos is null */
    NJS_POINT3 place = {16.0F, 176.0F, 20000.0F};
    struct task *task;
    struct tutorial_back *back;

    task = task_create(TASK_WORK, level, tutorial_back_step);
    if (task) {
        task->extra = heap_calloc(1, sizeof(struct tutorial_back));
        back = task->extra;
        if (pos) {
            task->work->pos = *pos;
        } else {
            task->work->pos = place;
        }
        task->work->scl.x = 1.0F;
        back->pages[BACK_CONTROLS] = controls_pages[story];
        back->pages[BACK_GAME] = game_pages[story];
        back->texlist = back_texlists[story];
        back->color = 0xffffffff;
        back->blend = 1.0F;
        back->speed = 0.1F;
        task->draw = tutorial_back_draw;
    }
    return task;
}

void tutorial_back_set_mode(struct task *task, int mode)
{
    task->work->mode = mode;
}

void tutorial_back_turn(struct task *task)
{
    task->work->mode = BACK_TURN_START;
}
