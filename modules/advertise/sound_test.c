/* the sound test, which the options screen goes on to. it plays the game's
 * music by kind, and the sounds of the effects' banks */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "advertise/back.h"
#include "advertise/buttons.h"
#include "advertise/draw.h"
#include "advertise/font.h"
#include "advertise/menus.h"
#include "advertise/screen.h"
#include "advertise/sound.h"
#include "advertise/sound_test.h"
#include "advertise/sound_test_banks.h"
#include "advertise/sound_test_music.h"
#include "advertise/textures.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "sound/sound.h"
#include "system/pad.h"
#include "system/task.h"

/* SOUND_TEST_MENU's items, as menu_result numbers them. the kinds before
 * KIND_EFFECTS are the kinds of music, and sound_test_music has a list for
 * each */
enum sound_test_kind {
    KIND_CHARACTER,
    KIND_ACTION,
    KIND_ADVENTURE,
    KIND_EVENT,
    KIND_OTHER,
    KIND_JINGLE,
    KIND_EFFECTS,
    KIND_BACK,
    KINDS
};

/* the screen's states. the draw tests them by range, so their order counts */
enum sound_test_state {
    SOUND_TEST_ASK, /* shows SOUND_TEST_MENU */
    SOUND_TEST_CHOOSE, /* waits for its choice */
    SOUND_TEST_DONE, /* back was chosen */
    /* a kind of music's tracks. SOUND_TEST_LIST_OPEN fills the list, and
     * SOUND_TEST_LIST_CLOSE asks the menu again */
    SOUND_TEST_LIST_OPEN,
    SOUND_TEST_LIST,
    SOUND_TEST_LIST_CLOSE,
    /* the effects. SOUND_TEST_BANK_LOAD loads the bank's file, and
     * SOUND_TEST_BANK_CLOSE loads mobile1.mlt again */
    SOUND_TEST_BANK_LOAD,
    SOUND_TEST_BANK,
    SOUND_TEST_BANK_CLOSE
};

/* the effects' three numbers, in their order from the left */
enum sound_test_column { COLUMN_BANK, COLUMN_GROUP, COLUMN_SOUND, COLUMNS };

#define TEST_SLOT 32 /* the first sequence slot */

#define LIST_ROWS 5 /* tracks shown at once */
/* the cursor's row among those shown, from 0 at the top, while the list can
 * scroll */
#define LIST_MARGIN 2

/* the effects' digits, each on a SNDTEST_WINDOW of its own: two for the bank,
 * one for the group and two for the sound */
#define DIGITS 5

/* @hack spelling: red, green and blue each masked and shifted back on its
 * own. SHC then takes each byte out and ORs them together again (SHLR16 and
 * EXTU.B at 0x8c907cb8 and 0x8c907cbc, OR at 0x8c907ccc). as color &
 * 0xffffff, it ANDs once, sound_test_fade_quad and sound_test_fade_corners
 * run shorter, and 76 of 76 words differ */
#define TRANSPARENT(color)                                                     \
    (((((color) >> 16) & 0xff) << 16) | ((((color) >> 8) & 0xff) << 8) |       \
     ((color) & 0xff))

struct sound_test {
    enum screen_mode mode;
    int back;
    int next;
    Float z;
    Float fade;
    Uint32 tint;
    enum sound_test_state state;
    Uint32 cursor_color;
    Float pulse;
    Angle blink;
    Uint8 alpha;
    Uint8 unk_29[7];
    /* how far the list, its box and the effects' windows have faded in, from
     * 0 to 1. a choice of kind sets it to 1, and nothing else changes it */
    Float list_fade;
    Uint8 count;
    Uint8 top; /* the track in the list's top row */
    Uint8 item; /* the track the cursor is on */
    /* the kind chosen, or -1 while the menu waits for a choice. the menu
     * opens with its cursor on it */
    int kind;
    Float unk_3c;
    Sint8 column;
    /* 1 after down and 0 after up: the draw pulses the arrow pressed last */
    Sint8 pressed_down;
    Sint8 bank;
    Sint8 group;
    Sint8 sound;
};

/* each kind's caption in ava_sndtest, by enum sound_test_kind. every kind
 * but back has a box, and each kind of music a list in it */
static const Uint8 sound_test_captions[KINDS] = {
    SNDTEST_CHARACTER, SNDTEST_ACTION, SNDTEST_ADVENTURE, SNDTEST_EVENT,
    SNDTEST_OTHER,     SNDTEST_JINGLE, SNDTEST_EFFECTS,   SNDTEST_BACK,
};
static const Float sound_test_list_xs[KIND_EFFECTS] = {
    176.0F, 176.0F, 176.0F, 176.0F, 176.0F, 176.0F,
};
static const Float sound_test_box_widths[KIND_BACK] = {
    400.0F, 400.0F, 400.0F, 400.0F, 400.0F, 400.0F, 352.0F,
};
static const Float sound_test_item_widths[KIND_EFFECTS] = {
    1.313F, 1.313F, 1.313F, 1.313F, 1.313F, 1.313F,
};
static const Float sound_test_window_xs[DIGITS] = {188.0F, 233.0F, 301.0F,
                                                   369.0F, 414.0F};
static const Float sound_test_column_xs[COLUMNS] = {231.0F, 320.0F, 409.0F};

static struct task *sound_test_task;

static void sound_test_up(struct sound_test *work)
{
    switch (work->column) {
    case COLUMN_BANK:
        if (work->bank < SOUND_TEST_BANKS - 1) {
            work->pulse = 1.5F;
            ++work->bank;
            work->group = 0;
            work->sound = 0;
            sound_load_file(sound_test_bank_files[work->bank]);
        }
        break;
    case COLUMN_GROUP:
        if (work->group < sound_test_banks[work->bank]->count - 1) {
            work->pulse = 1.5F;
            ++work->group;
            work->sound = 0;
        }
        break;
    case COLUMN_SOUND:
        if (work->sound <
            sound_test_banks[work->bank]->groups[work->group].count - 1) {
            work->pulse = 1.5F;
            ++work->sound;
        }
        break;
    }
    work->pressed_down = 0;
}

static void sound_test_down(struct sound_test *work)
{
    switch (work->column) {
    case COLUMN_BANK:
        if (work->bank > 0) {
            work->pulse = 1.5F;
            --work->bank;
            work->group = 0;
            work->sound = 0;
            sound_load_file(sound_test_bank_files[work->bank]);
        }
        break;
    case COLUMN_GROUP:
        if (work->group > 0) {
            work->pulse = 1.5F;
            --work->group;
            work->sound = 0;
        }
        break;
    case COLUMN_SOUND:
        if (work->sound > 0) {
            work->pulse = 1.5F;
            --work->sound;
        }
        break;
    }
    work->pressed_down = 1;
}

static void sound_test_open(struct sound_test *work)
{
    textures_load_screen(SCREEN_SOUND_TEST);
    advertise_font_load(&abc_font);
    advertise_font_use(&abc_font);
    advertise_music_start(-1);
    advertise_voice_summary(LINE_SOUND_TEST);
    work->top = 0;
    work->item = 0;
    /* @hack spelling: copy, a copy of work. SHC then copies work to R5 and
     * stores state and kind through it (MOV R14,R5 at 0x8c90784a). without
     * it, SHC stores through R14, the function runs 2 bytes short,
     * sound_test_run moves, and 10 of 31 words differ */
    {
        struct sound_test *copy = work;

        copy->state = SOUND_TEST_ASK;
        copy->kind = KIND_CHARACTER;
    }
}

static void sound_test_run(struct sound_test *work)
{
    Sint8 sum;

    menu_cursor_step(&work->alpha, &work->blink, &work->pulse);
    work->cursor_color = 0xffff0000;
    switch (work->state) {
    case SOUND_TEST_ASK:
        advertise_menu_show(SOUND_TEST_MENU, work->kind, NULL);
        work->state = SOUND_TEST_CHOOSE;
        /* fall through */
    case SOUND_TEST_CHOOSE:
        /* @hack type: the result cast to Sint8 before kind takes it, which
         * SHC sign-extends from its low byte (EXTS.B at 0x8c9078de); without
         * the cast, 42 of 378 words differ */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        if ((work->kind = (Sint8)menu_result()) >= 0) {
            switch (work->kind) {
            case KIND_BACK:
                screen_go(work->back);
                work->state = SOUND_TEST_DONE;
                break;
            case KIND_EFFECTS:
                work->list_fade = 1.0F;
                work->state = SOUND_TEST_BANK_LOAD;
                break;
            default:
                work->list_fade = 1.0F;
                work->state = SOUND_TEST_LIST_OPEN;
                break;
            }
        }
        break;
    case SOUND_TEST_DONE:
        break;
    case SOUND_TEST_LIST_OPEN:
        work->count = sound_test_music[work->kind].count;
        work->top = 0;
        work->item = 0;
        work->state = SOUND_TEST_LIST;
        break;
    case SOUND_TEST_LIST:
        if (pad_press_repeat[0] & PDD_DGT_KU) {
            if (work->item > 0) {
                work->pulse = 1.5F;
                --work->item;
                if (work->top > 0 && work->item < work->top + LIST_MARGIN) {
                    --work->top;
                }
                se_move();
            }
        } else if (pad_press_repeat[0] & PDD_DGT_KD) {
            if (work->item < work->count - 1) {
                work->pulse = 1.5F;
                ++work->item;
                if (work->item > work->top + LIST_MARGIN &&
                    work->top + LIST_ROWS < work->count) {
                    ++work->top;
                }
                se_move();
            }
        } else if (pad_choose_pressed() && textures_ready == TRUE) {
            advertise_music_start(
                sound_test_music[work->kind].tracks[work->item].music);
        } else if (pad_back_pressed() && textures_ready == TRUE) {
            advertise_music_stop();
            work->state = SOUND_TEST_LIST_CLOSE;
        }
        break;
    case SOUND_TEST_LIST_CLOSE:
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        advertise_menu_popup(SOUND_TEST_MENU, work->kind, NULL);
        work->state = SOUND_TEST_CHOOSE;
        break;
    case SOUND_TEST_BANK_LOAD:
        sound_load_file(sound_test_bank_files[work->bank]);
        work->state = SOUND_TEST_BANK;
        break;
    case SOUND_TEST_BANK:
        sum = work->column + work->bank + work->group + work->sound;
        if ((pad_press_repeat[0] & PDD_DGT_KL) && work->column > 0) {
            --work->column;
        } else if ((pad_press_repeat[0] & PDD_DGT_KR) &&
                   work->column < COLUMNS - 1) {
            ++work->column;
        } else if (pad_press_repeat[0] & PDD_DGT_KU) {
            sound_test_up(work);
        } else if (pad_press_repeat[0] & PDD_DGT_KD) {
            sound_test_down(work);
        } else if (pad_choose_pressed()) {
            sound_slot_stop(TEST_SLOT);
            sound_slot_play(
                sound_test_banks[work->bank]->driver_banks[work->group],
                sound_test_banks[work->bank]
                    ->groups[work->group]
                    .numbers[work->sound],
                0, TEST_SLOT);
        } else if (pad_back_pressed()) {
            work->state = SOUND_TEST_BANK_CLOSE;
        }
        /* @bug logic: the test sees a change only in the sum of the
         * column and the three numbers (stored at 0x8c907ab2, compared at
         * 0x8c907bc4). a step up raises a number by 1 and zeroes the numbers
         * right of it. where those summed to 1, the sum stays as it was, and
         * the step plays no move sound and doesn't stop TEST_SLOT */
        if (work->column + work->bank + work->group + work->sound != sum) {
            se_move();
            sound_slot_stop(TEST_SLOT);
        }
        break;
    case SOUND_TEST_BANK_CLOSE:
        sound_slot_stop(TEST_SLOT);
        sound_load_file("mobile1.mlt");
        /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
        advertise_menu_popup(SOUND_TEST_MENU, work->kind, NULL);
        work->state = SOUND_TEST_CHOOSE;
        break;
    }
}

static void sound_test_draw_arrow(Uint32 from, Uint32 to, Float x, Float y,
                                  Float z, Float scale, Bool flip)
{
    int flags = flip ? QUAD_FLIP_V : 0;

    njSetTexture(&ava_sndtest_texlist);
    screen_quad_colors(from, to, from, to);
    screen_quad_draw_scaled(QUAD_CENTERED | SNDTEST_ARROW | flags, x, y,
                            z - 0.6F, scale, scale);
    screen_quad_color(0xffffffff);
    screen_quad_draw_scaled(QUAD_CENTERED | SNDTEST_ARROW_FRAME | flags, x, y,
                            z - 0.3F, scale, scale);
    screen_quad_draw_scaled(QUAD_CENTERED | SNDTEST_ARROW_SMALL | flags, x, y,
                            z, scale, scale);
}

static void sound_test_fade_quad(Uint32 color, Float fade)
{
    Uint32 faded = menu_blend_color(TRANSPARENT(color), color, fade);

    screen_quad_colors(color, faded, color, faded);
}

static void sound_test_fade_corners(Uint32 color0, Uint32 color1, Uint32 color2,
                                    Uint32 color3, Float fade)
{
    Uint32 faded1;
    Uint32 faded3;

    faded1 = menu_blend_color(TRANSPARENT(color1), color1, fade);
    faded3 = menu_blend_color(TRANSPARENT(color3), color3, fade);
    screen_quad_colors(color0, faded1, color2, faded3);
}

static void sound_test_draw_list(struct sound_test *work)
{
    int i;
    int rows = work->count > LIST_ROWS ? LIST_ROWS : work->count;
    Float y;
    Uint32 color;

    for (i = 0; i < rows; ++i) {
        y = (i * 32.0F) + 202.0F;
        sound_test_fade_quad(0xffffffff, work->list_fade);
        advertise_font_draw_string(
            sound_test_music[work->kind].tracks[work->top + i].title,
            sound_test_list_xs[work->kind], (i * 32.0F) + 194.0F,
            work->z - 12.0F);
        if (work->item == work->top + i) {
            /* @hack type: alpha passed as a Sint8. SHC then passes the byte
             * as MOV.B loads it, in the JSR's slot (0x8c907e1c). as the
             * Uint8, SHC adds an EXTU.B there, and 5 of 188 words differ */
            /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
            menu_draw_column_cursor(
                (Sint8)work->alpha, 320.0F, y, work->z - 14.0F,
                work->pulse * sound_test_item_widths[work->kind],
                work->pulse * 0.375F);
            color = work->cursor_color;
        } else {
            color = 0xff12b4ff;
        }
        menu_draw_column_item(color, 320.0F, y, work->z - 10.0F,
                              sound_test_item_widths[work->kind], 0.375F);
    }
    if (work->top > 0) {
        scroll_arrow_draw(TRUE, 320.0F, 170.0F, work->z - 12.0F);
    }
    if (work->top + LIST_ROWS < work->count) {
        scroll_arrow_draw(FALSE, 320.0F, 370.0F, work->z - 12.0F);
    }
}

static void sound_test_draw_box(struct sound_test *work)
{
    sound_test_fade_corners(0x78002e67, 0x78117bff, 0x78002e67, 0x78117bff,
                            work->list_fade);
    menu_draw_box(320.0F, 264.0F, work->z - 6.0F,
                  sound_test_box_widths[work->kind], 336.0F);
    menu_draw_column_item(0xff12b4ff, 320.0F, 124.0F, work->z - 8.0F, 0.813F,
                          0.375F);
    njSetTexture(&ava_sndtest_texlist);
    sound_test_fade_quad(0xffffffff, work->list_fade);
    /* @hack type: the caption read through a cast of the table's address
     * plus kind. SHC then adds kind to the address (ADD R3,R4 at 0x8c907fd0)
     * and loads the byte from the sum (MOV.B @R4,R4 at 0x8c907fd6). as
     * sound_test_captions[work->kind], SHC loads it by MOV.B @(R0,R3), the
     * function runs 2 bytes short, the functions after it move, and 45 of 84
     * words differ */
    screen_quad_draw(QUAD_CENTERED |
                         *(const Uint8 *)(sound_test_captions + work->kind),
                     320.0F, 124.0F, work->z - 10.0F);
    screen_quad_draw(SNDTEST_AB, 155.0F, 388.0F, work->z - 10.0F);
}

static void sound_test_draw_numbers(struct sound_test *work)
{
    int bank;
    int group;
    int sound;
    int i;
    Uint32 left;
    Uint32 right;
    Float z;
    Float up_scale;
    Float down_scale;
    const Float *x;

    z = work->z - 14.0F;
    bank = work->bank % 100;
    new16no_draw_all(bank / 10, 207.0F, 264.1F, z, 1.2F, 1.6F);
    new16no_draw_all(bank % 10, 252.0F, 264.1F, z, 1.2F, 1.6F);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    group = work->group;
    new16no_draw_all(group % 10, 320.0F, 264.1F, z, 1.2F, 1.6F);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    sound = work->sound;
    new16no_draw_all(sound / 10, 388.0F, 264.1F, z, 1.2F, 1.6F);
    new16no_draw_all(sound % 10, 433.0F, 264.1F, z, 1.2F, 1.6F);
    for (i = 0; i < COLUMNS; ++i) {
        if (work->column == i) {
            if (work->pressed_down) {
                up_scale = 1.0F;
                down_scale = work->pulse;
            } else {
                up_scale = work->pulse;
                down_scale = 1.0F;
            }
            left = 0xffff7100;
            right = 0xffffc600;
        } else {
            left = 0xff00ffdb;
            right = 0xff00c3ff;
            up_scale = 1.0F;
            down_scale = 1.0F;
        }
        /* @hack spelling: the column's x read through x, a pointer into
         * sound_test_column_xs. SHC then keeps the pointer in R10 for both
         * arrows (ADD R3,R10 at 0x8c908206, FMOV.S @R10,FR4 at 0x8c90820a).
         * as sound_test_column_xs[i], SHC keeps the address on the stack,
         * and 59 of 170 words differ */
        x = sound_test_column_xs + i;
        sound_test_draw_arrow(left, right, *x, 193.0F, work->z - 10.0F,
                              up_scale, FALSE);
        sound_test_draw_arrow(right, left, *x, 335.0F, work->z - 10.0F,
                              down_scale, TRUE);
    }
    /* @hack redundant: a block that never runs, of three stores of 193.0 and
     * a test of work. SHC then keeps the digits' floats in FR12, FR13 and FR14
     * (0x8c908102 to 0x8c908106), and the arrows' colors in R11 and R12
     * (0x8c908200). without it, those registers rotate and trade, and 76 of
     * 170 words differ */
    if (0) {
        work->z = 193.0F;
        work->z = 193.0F;
        work->z = 193.0F;
        if (work) {
        }
    }
}

static void sound_test_draw_windows(struct sound_test *work)
{
    unsigned int i;

    njSetTexture(&ava_sndtest_texlist);
    sound_test_fade_quad(0xffffffff, work->list_fade);
    screen_quad_draw(QUAD_BOTTOM | SNDTEST_WINDOW_LARGE, 231.0F, 264.0F,
                     work->z - 8.0F);
    screen_quad_draw(QUAD_FLIP_V | QUAD_TOP | SNDTEST_WINDOW_LARGE, 231.0F,
                     264.0F, work->z - 8.0F);
    screen_quad_draw(QUAD_BOTTOM | SNDTEST_WINDOW_SMALL, 320.0F, 264.0F,
                     work->z - 8.0F);
    screen_quad_draw(QUAD_FLIP_V | QUAD_TOP | SNDTEST_WINDOW_SMALL, 320.0F,
                     264.0F, work->z - 8.0F);
    screen_quad_draw(QUAD_BOTTOM | SNDTEST_WINDOW_LARGE, 409.0F, 264.0F,
                     work->z - 8.0F);
    screen_quad_draw(QUAD_FLIP_V | QUAD_TOP | SNDTEST_WINDOW_LARGE, 409.0F,
                     264.0F, work->z - 8.0F);
    sound_test_fade_corners(0x7800297c, 0x781864ff, 0x7800297c, 0x781864ff,
                            work->list_fade);
    for (i = 0; i < DIGITS; ++i) {
        screen_quad_draw(SNDTEST_WINDOW, sound_test_window_xs[i], 235.0F,
                         work->z - 10.0F);
    }
}

static void sound_test_draw_title(struct sound_test *work)
{
    njSetTexture(&ava_sndtest_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(SNDTEST_TITLE, 32.0F, 40.0F, work->z - 14.0F);
    screen_band_draw(0.0F, 32.0F, work->z - 8.0F, 565.0F, 42.0F);
}

static void sound_test_init(struct task *task, struct screen_links *links)
{
    struct sound_test *work = task->extra;

    work->back = links->back;
    work->next = links->next;
    work->z = 56000.0F;
    /* @hack spelling: copy, a copy of work. SHC then holds work in R1 and 0
     * in R4 (MOV.L @(44,R4),R1 at 0x8c90839e, MOV #0,R4 at 0x8c9083a0).
     * without it, work takes R4 and 0 takes R5, and 16 of 27 words differ */
    {
        struct sound_test *copy = work;

        copy->mode = SCREEN_HIDDEN;
        copy->bank = 0;
        copy->group = 0;
        copy->sound = 0;
        copy->column = COLUMN_BANK;
        copy->pressed_down = 0;
        copy->unk_3c = 0.0F;
        copy->state = SOUND_TEST_ASK;
        copy->kind = KIND_CHARACTER;
    }
}

static void sound_test_step(struct task *task)
{
    struct sound_test *work = task->extra;

    if (ADVERTISE_WORK()->screen == SCREEN_SOUND_TEST &&
        work->mode == SCREEN_HIDDEN) {
        sound_test_open(work);
        work->fade = 0.0F;
        work->mode = SCREEN_OPENING;
    }
    if (ADVERTISE_WORK()->last_screen == SCREEN_SOUND_TEST &&
        work->mode == SCREEN_SHOWN) {
        work->fade = 0.0F;
        work->mode = SCREEN_CLOSING;
    }
    switch (work->mode) {
    case SCREEN_HIDDEN:
        work->tint = 0;
        break;
    case SCREEN_OPENING:
        work->tint = color_fade_in(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            work->mode = SCREEN_SHOWN;
        }
        break;
    case SCREEN_SHOWN:
        work->tint = 0xffffffff;
        ADVERTISE_WORK()->shown = SCREEN_SOUND_TEST;
        sound_test_run(work);
        break;
    case SCREEN_CLOSING:
        work->tint = color_fade_out(work->fade);
        work->fade += 0.1F;
        if (work->fade >= 1.0F) {
            textures_switch(SCREEN_SOUND_TEST);
            advertise_font_release(&abc_font);
            ADVERTISE_WORK()->closed = 1;
            work->mode = SCREEN_HIDDEN;
        }
        break;
    }
}

static void sound_test_draw(struct task *task)
{
    struct sound_test *work = task->extra;

    if (work->tint) {
        screen_blend_alpha();
        screen_quad_tint(work->tint);
        sound_test_draw_title(work);
        if (work->state >= SOUND_TEST_LIST_OPEN) {
            sound_test_draw_box(work);
        }
        if (work->state >= SOUND_TEST_LIST_OPEN &&
            work->state <= SOUND_TEST_LIST_CLOSE) {
            sound_test_draw_list(work);
        }
        if (work->state >= SOUND_TEST_BANK_LOAD) {
            sound_test_draw_windows(work);
            if (work->state <= SOUND_TEST_BANK_CLOSE) {
                sound_test_draw_numbers(work);
            }
        }
        back_draw(work->z - 4.0F);
    }
}

struct task *sound_test_create(struct screen_links *links)
{
    struct task *task;
    struct sound_test *work;

    /* @hack spelling: the task assigned in the test. SHC then tests it in R0
     * (TST R0,R0 at 0x8c9085c4), and copies it to R14 after. assigned first,
     * it is copied to R14 and tested there, and 2 of 32 words differ */
    if ((sound_test_task = task = task_create(0, 3, sound_test_step)) != NULL) {
        work = heap_calloc(1, sizeof(struct sound_test));
        task->extra = work;
        sound_test_init(task, links);
        task->step = sound_test_step;
        task->draw = sound_test_draw;
    }
    return task;
}

void sound_test_destroy(void)
{
    if (sound_test_task) {
        task_destroy(sound_test_task);
    }
}
