/* menus: the debug menu titled <SELECT PROGRAM>, and the menu task that
 * grows a menu open, moves its cursor and shrinks it closed */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "debug/editor.h"
#include "debug/fog_editor.h"
#include "debug/seq_editor.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"

/* program_select_step's modes. 1 is skipped: nothing sets program_mode to
 * it */
enum program_mode {
    PROGRAM_SELECT = 0, /* the menu shows while X is held */
    PROGRAM_RUN = 2 /* the chosen program steps */
};

/* a program on the debug menu. init readies it once, and exec steps it
 * until it returns -1. a NULL init ends the table */
struct program {
    char name[16];
    void (*init)(void);
    int (*exec)(void);
};

#define SCALE_STEPS 16 /* to grow a menu open, or to shrink it closed */
#define CHOSEN_STEPS 4 /* between a choice and the close */

/* the item under the cursor, and the cursor, are a fifth larger */
#define CURSOR_SCALE 1.2F

/* the cursor's blink turns this far a step: a whole blink in 90 steps,
 * near enough */
#define BLINK_STEP 0x2d8
/* the cursor's pulse after a move, and its fall in a step */
#define PULSE_MOVED 1.5F
#define PULSE_FALL 0.05F

/* room for 15 disabled items and the negative that ends them */
#define DISABLED_SIZE 16

/* the items' colors, 0xAARRGGBB. the item under the cursor is
 * CURSOR_ITEM_COLOR, but while the menu waits for a choice it blinks
 * between that and its kind's color */
#define ROW_ITEM_COLOR 0xff0000ff
#define COLUMN_ITEM_COLOR 0xff12b4ff
#define CURSOR_ITEM_COLOR 0xffff0000

/* the cursor's colors, 0xRRGGBB under its alpha. a row's cursor runs from
 * yellow at its top to orange at its bottom, and a column's is orange */
#define CURSOR_YELLOW 0xffeb00
#define CURSOR_ORANGE 0xec9800

enum menu_mode {
    MENU_IDLE, /* no menu yet */
    MENU_OPEN, /* growing open */
    MENU_CHOOSE, /* waiting while the pad moves the cursor */
    MENU_CHOSEN, /* chosen, and waiting CHOSEN_STEPS to close */
    MENU_CLOSE, /* shrinking closed */
    MENU_DONE /* closed, with the cursor on the choice */
};

/* the menu task's extra */
struct menu_work {
    const struct menu *menu;
    enum menu_mode mode;
    Uint32 tint; /* the whole menu's color, or 0 to hide it */
    Float scale_x, scale_y; /* the whole menu's, as it grows and shrinks */
    Float pulse; /* the cursor's scale: 1.5 after a move, settling to 1 */
    Uint32 cursor_color; /* the item under the cursor's */
    Uint32 item_color; /* the other items' */
    Angle blink; /* cursor_color follows its cosine */
    Sint16 idle; /* as menu_get_idle gives it */
    Sint16 step; /* steps into the mode */
    Sint8 cursor;
    Uint8 alpha; /* the cursor's, from its pulse */
    Sint8 disabled[DISABLED_SIZE]; /* items to skip, ending in a negative */
};

struct menu_scale {
    Float x, y;
};

struct menu_keys {
    Uint32 prev, next;
};

static void program_select_draw(void);
static Bool menu_list_has(Sint8 *list, Sint8 value);
static void menu_set_disabled(Sint8 *disabled);
static void menu_step(struct task *task);
static void menu_draw(struct task *task);
static void menu_set_cursor(Sint8 cursor);

static struct program program_table[] = {
    {"SEQ EDITOR", seq_editor_init, seq_editor_exec},
    {"DUMMY", NULL, NULL},
};

static enum program_mode program_mode;
static int program_count;
static int program_cursor;
static int program_selected;

void program_select_init(void)
{
    struct program *program = program_table;

    program_count = 0;
    for (; program->init != NULL; program_count++, program++) {
        program->init();
    }
    light_editor_init();
    fog_editor_init();
    program_cursor = 0;
    program_mode = PROGRAM_SELECT;
}

void program_select_step(void)
{
    struct program *program;
    int result;

    switch (program_mode) {
    case PROGRAM_SELECT:
        if (per_info[0]->on & PDD_DGT_TX) {
            if (per_info[0]->press & PDD_DGT_KU) {
                program_cursor--;
            } else if (per_info[0]->press & PDD_DGT_KD) {
                program_cursor++;
            }
            if (program_cursor < 0) {
                program_cursor = program_count - 1;
            } else if (program_cursor >= program_count) {
                program_cursor = 0;
            }
            if (per_info[0]->press & PDD_DGT_ST) {
                program_selected = program_cursor;
                program_mode = PROGRAM_RUN;
            } else {
                program_select_draw();
                pause_disable();
            }
        } else {
            pause_enable();
        }
        break;
    case PROGRAM_RUN:
        program = &program_table[program_selected];
        result = program->exec();
        if (result == -1) {
            program_mode = PROGRAM_SELECT;
        }
        break;
    }
}

static void program_select_draw(void)
{
    int i;

    njPrintColor(0xff00ff00);
    njPrintC(NJM_LOCATION(10, 6), "<SELECT PROGRAM>");
    njPrintColor(0xffc0c0c0);
    for (i = 0; i < program_count; i++) {
        njPrintC(NJM_LOCATION(10, i + 8), program_table[i].name);
    }
    njPrintColor(0xffff0000);
    njPrintC(NJM_LOCATION(9, program_cursor + 8), ">");
    njPrintColor(0xffffffff);
}

const struct menu_sounds menu_default_sounds = {SE_MENU_OPEN, SE_MENU_CLOSE,
                                                SE_MOVE, SE_CHOOSE, SE_CANCEL};

/* the menu grows past its size to 1.2, falls back to 0.9, and settles at 1 */
static const struct menu_scale menu_open_scales[SCALE_STEPS] = {
#include "hud/menu/menu_open_scales.inc"
};

/* the menu widens and flattens, then narrows and stretches, and is gone */
static const struct menu_scale menu_close_scales[SCALE_STEPS] = {
#include "hud/menu/menu_close_scales.inc"
};

static const struct menu_keys menu_keys[2] = {
    {PDD_DGT_KL, PDD_DGT_KR},
    {PDD_DGT_KU, PDD_DGT_KD},
};

static NJS_TEXNAME row_texnames[4] = {
    {NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0}};
NJS_TEXLIST menu_row_texlist = {
#include "hud/menu/menu_row_texlist.inc"
};
static NJS_TEXNAME column_texnames[3] = {
    {NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0}};
NJS_TEXLIST menu_column_texlist = {
#include "hud/menu/menu_column_texlist.inc"
};

static struct task *menu_task;

/* set the cursor on the first item from cursor on that isn't disabled,
 * wrapping past the last */
static void menu_set_cursor(Sint8 cursor)
{
    struct menu_work *work = menu_task->extra;
    Sint8 i;

    for (i = 0; i < work->menu->count; i++) {
        if (menu_list_has(work->disabled, cursor)) {
            cursor++;
            if (cursor >= work->menu->count) {
                cursor = 0;
            }
        }
    }
    work->cursor = cursor;
}

void menu_cursor_step(Uint8 *alpha, Angle *blink, Float *pulse)
{
    *blink += BLINK_STEP;
    *pulse = *pulse > 1.0F ? *pulse - PULSE_FALL : 1.0F;
    *alpha = (1.0F - njSqrt((*pulse - 1.0F) * 2.0F)) * 255.0F;
}

Uint32 menu_blend_color(Uint32 from, Uint32 to, Float part)
{
    Uint32 alpha;
    Uint32 red;
    Uint32 green;
    Uint32 blue;

    alpha = ((from >> 24) * (1.0F - part)) + ((to >> 24) * part);
    red =
        (((from >> 16) & 0xff) * (1.0F - part)) + (((to >> 16) & 0xff) * part);
    green =
        (((from >> 8) & 0xff) * (1.0F - part)) + (((to >> 8) & 0xff) * part);
    blue = ((from & 0xff) * (1.0F - part)) + ((to & 0xff) * part);
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

void menu_draw_row_cursor(int alpha, Float x, Float y, Float z, Float scale_x,
                          Float scale_y)
{
    /* @hack type: the cast to Uint32 on top's alpha alone, which has SHC
     * extend alpha once for each color (EXTU.B at 0x8c0abd6c and
     * 0x8c0abd74); with the same cast on both, 37 of 48 words differ */
    Uint32 top = ((Uint32)(Uint8)alpha << 24) | CURSOR_YELLOW;
    Uint32 bottom = ((Uint8)alpha << 24) | CURSOR_ORANGE;

    njSetTexture(&menu_row_texlist);
    screen_quad_colors(top, bottom, top, bottom);
    screen_quad_draw_scaled(QUAD_CENTERED | 3, x, y, z, scale_x, scale_y);
}

/* @hack type: an old-style definition, so that a caller passes alpha as an
 * int and this function reads it as a Uint8; with a prototype taking int
 * alpha, cast as menu_draw_row_cursor casts it, 37 of 39 words differ */
/* clang-format off */
/* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
void menu_draw_column_cursor(alpha, x, y, z, scale_x, scale_y)
    Uint8 alpha;
    Float x, y, z, scale_x, scale_y;
/* clang-format on */
{
    njSetTexture(&menu_column_texlist);
    screen_quad_color(((Uint32)alpha << 24) | CURSOR_ORANGE);
    screen_quad_draw_scaled(QUAD_CENTERED | 2, x, y, z, scale_x, scale_y);
}

void menu_draw_row_item(Uint32 color, Float x, Float y, Float z, Float scale_x,
                        Float scale_y)
{
    njSetTexture(&menu_row_texlist);
    screen_quad_color(color);
    screen_quad_draw_scaled(QUAD_CENTERED | 2, x, y, z - 0.3F, scale_x,
                            scale_y);
    screen_quad_color((color & 0xff000000) | 0xffffff);
    screen_quad_draw_scaled(QUAD_CENTERED | 1, x, y, z - 0.6F, scale_x,
                            scale_y);
}

void menu_draw_column_item(Uint32 color, Float x, Float y, Float z,
                           Float scale_x, Float scale_y)
{
    njSetTexture(&menu_column_texlist);
    screen_quad_color(color);
    screen_quad_draw_scaled(QUAD_CENTERED | 0, x, y, z - 0.3F, scale_x,
                            scale_y);
    screen_quad_color((color & 0xff000000) | 0xffffff);
    screen_quad_draw_scaled(QUAD_CENTERED | 1, x, y, z, scale_x * 1.0460432F,
                            scale_y * 1.1770245F);
}

void menu_draw_box(Float x, Float y, Float z, Float width, Float height)
{
    screen_box_draw(x - (width * 0.5F), y - (height * 0.5F), z, width, height);
}

static void menu_reset(struct menu_work *work)
{
    work->tint = 0;
    work->scale_x = 0.0F;
    work->scale_y = 0.0F;
    work->pulse = 1.0F;
    work->idle = 0;
    work->step = 0;
    menu_set_disabled(NULL);
}

/* copy disabled, a list of items to skip that a negative ends, or clear
 * the list for NULL. the copy has no bound */
static void menu_set_disabled(Sint8 *disabled)
{
    struct menu_work *work = menu_task->extra;
    int i;
    int j = 0;

    if (disabled != NULL) {
        do {
            work->disabled[j++] = *disabled;
        } while (*disabled++ >= 0);
    } else {
        for (i = 0; i < DISABLED_SIZE; i++) {
            work->disabled[i] = -1;
        }
    }
}

/* whether value is in list, which a negative ends */
static Bool menu_list_has(Sint8 *list, Sint8 value)
{
    Bool found = FALSE;

    while (*list >= 0) {
        if (value == *list) {
            found = TRUE;
        }
        list++;
    }
    return found;
}

/* the first item after cursor that isn't disabled, or cursor for none */
static Sint8 menu_next_item(Sint8 *disabled, Sint8 cursor, Sint8 count)
{
    Sint8 start;

    if (cursor == count - 1) {
        return cursor;
    }
    start = cursor;
    do {
        cursor++;
        if (!menu_list_has(disabled, cursor)) {
            return cursor;
        }
    } while (cursor < count - 1);
    return start;
}

/* the last item before cursor that isn't disabled, or cursor for none */
static Sint8 menu_prev_item(Sint8 *disabled, Sint8 cursor)
{
    Sint8 start;

    if (cursor <= 0) {
        return cursor;
    }
    start = cursor;
    do {
        cursor--;
        if (!menu_list_has(disabled, cursor)) {
            return cursor;
        }
    } while (cursor > 0);
    return start;
}

static void menu_draw_frame(struct menu_work *work)
{
    const struct menu *menu = work->menu;

    screen_blend_alpha();
    screen_quad_colors(menu->frame_top, menu->frame_bottom, menu->frame_top,
                       menu->frame_bottom);
    menu_draw_box(menu->x, menu->y, menu->z - 4.0F,
                  menu->frame_w * work->scale_x, menu->frame_h * work->scale_y);
}

/* draw each item's plate and label, then the cursor. the item under the
 * cursor is in cursor_color unless it is disabled. a disabled item is
 * faint, and the cursor over one has half its alpha */
static void menu_draw_items(struct menu_work *work)
{
    const struct menu *menu = work->menu;
    Float x;
    Float y;
    Float scale_x;
    Float scale_y;
    Uint32 color;
    Uint32 tint;
    int alpha;
    Sint8 i;

    for (i = 0; i < menu->count; i++) {
        x = menu->x + (menu->items[i].x * work->scale_x);
        y = menu->y + (menu->items[i].y * work->scale_y);
        if (work->cursor == i) {
            scale_x = work->scale_x * CURSOR_SCALE;
            scale_y = work->scale_y * CURSOR_SCALE;
        } else {
            /* @hack redundant: the scales multiplied by 1.0F, which loads 1
             * with FLDI1 and multiplies by it (0x8c0ac1c4 to 0x8c0ac1ce);
             * assigned as they are, menu.c's code runs 32 bytes shorter and
             * 255 of 312 words differ */
            scale_x = work->scale_x * 1.0F;
            scale_y = work->scale_y * 1.0F;
        }
        if (menu_list_has(work->disabled, i)) {
            color = (work->item_color & 0xffffff) | 0x30000000;
            tint = 0xb0ffffff;
        } else {
            color = work->cursor == i ? work->cursor_color : work->item_color;
            tint = 0xffffffff;
        }
        switch (menu->kind) {
        case MENU_ROW:
            menu_draw_row_item(color, x, y, menu->z - 6.0F,
                               menu->item_scale_x * scale_x,
                               menu->item_scale_y * scale_y);
            break;
        case MENU_COLUMN:
            menu_draw_column_item(color, x, y, menu->z - 6.0F,
                                  menu->item_scale_x * scale_x,
                                  menu->item_scale_y * scale_y);
            break;
        }
        njSetTexture(menu->texlist);
        screen_quad_color(tint);
        screen_quad_draw_scaled(menu->items[i].texture | QUAD_CENTERED, x, y,
                                menu->z - 8.0F, scale_x, scale_y);
    }
    if (work->cursor < menu->count) {
        alpha = menu_list_has(work->disabled, work->cursor) ? work->alpha / 2
                                                            : work->alpha;
        scale_x =
            menu->item_scale_x * work->scale_x * work->pulse * CURSOR_SCALE;
        scale_y =
            menu->item_scale_y * work->scale_y * work->pulse * CURSOR_SCALE;
        switch (menu->kind) {
        case MENU_ROW:
            menu_draw_row_cursor(
                alpha, menu->x + (menu->items[work->cursor].x * work->scale_x),
                menu->y + (menu->items[work->cursor].y * work->scale_y),
                menu->z - 10.0F, scale_x, scale_y);
            break;
        case MENU_COLUMN:
            menu_draw_column_cursor(
                alpha, menu->x + (menu->items[work->cursor].x * work->scale_x),
                menu->y + (menu->items[work->cursor].y * work->scale_y),
                menu->z - 10.0F, scale_x, scale_y);
            break;
        }
    }
}

static void menu_init(struct task *task)
{
    struct menu_work *work = task->extra;

    work->menu = NULL;
    work->mode = MENU_IDLE;
    menu_reset(work);
}

/* step the menu through its modes: grow it open, wait for a choice, pause,
 * and shrink it closed. the cursor's blink and pulse step in every mode */
static void menu_step(struct task *task)
{
    struct menu_work *work = task->extra;
    const struct menu *menu = work->menu;
    Sint8 cursor;
    Uint32 buttons;

    menu_cursor_step(&work->alpha, &work->blink, &work->pulse);
    work->cursor_color = CURSOR_ITEM_COLOR;
    if (menu != NULL) {
        switch (menu->kind) {
        case MENU_ROW:
            work->item_color = ROW_ITEM_COLOR;
            break;
        case MENU_COLUMN:
            work->item_color = COLUMN_ITEM_COLOR;
            break;
        }
    } else {
        work->item_color = COLUMN_ITEM_COLOR;
    }
    switch (work->mode) {
    case MENU_IDLE:
        menu_reset(work);
        break;
    case MENU_OPEN:
        work->tint = 0xffffffff;
        work->scale_x = menu_open_scales[work->step].x;
        work->scale_y = menu_open_scales[work->step].y;
        work->step++;
        if (work->step >= SCALE_STEPS) {
            work->blink = 0;
            work->mode = MENU_CHOOSE;
        }
        break;
    case MENU_CHOOSE:
        work->tint = 0xffffffff;
        work->scale_x = work->scale_y = 1.0F;
        switch (menu->kind) {
        case MENU_ROW:
            work->cursor_color =
                menu_blend_color(ROW_ITEM_COLOR, CURSOR_ITEM_COLOR,
                                 (njCos(work->blink) + 1.0F) * 0.5F);
            break;
        case MENU_COLUMN:
            work->cursor_color =
                menu_blend_color(COLUMN_ITEM_COLOR, CURSOR_ITEM_COLOR,
                                 (njCos(work->blink) + 1.0F) * 0.5F);
            break;
        }
        cursor = work->cursor;
        if ((pad_press_repeat[0] & menu_keys[menu->kind].prev) &&
            work->cursor > 0) {
            work->cursor = menu_prev_item(work->disabled, work->cursor);
        } else if ((pad_press_repeat[0] & menu_keys[menu->kind].next) &&
                   work->cursor < menu->count - 1) {
            work->cursor =
                menu_next_item(work->disabled, work->cursor, menu->count);
            /* @hack spelling: the next test, of A and start, reads the presses
             * into buttons, which loads them into R5 there (MOV.L @R4,R5 at
             * 0x8c0ac5fe) and keeps them for the test of B and X after it
             * (0x8c0ac684); read from pad_press_repeat[0] in each test, 15
             * of 349 words differ */
            /* @hack spelling: the next two tests each AND the presses with
             * each button's mask and OR the two (0x8c0ac600 to 0x8c0ac608,
             * 0x8c0ac680 to 0x8c0ac68a); with one mask for both buttons in
             * each test, 336 of 349 words differ */
        } else if ((buttons = pad_press_repeat[0],
                    (buttons & PDD_DGT_TA) | (buttons & PDD_DGT_ST))) {
            if (!menu_list_has(work->disabled, work->cursor)) {
                work->step = 0;
                work->mode = MENU_CHOSEN;
                if (menu->sounds != NULL) {
                    sound_play(work->cursor == menu->cancel
                                   ? menu->sounds->cancel
                                   : menu->sounds->select,
                               0, 0, 0);
                }
            }
        } else if ((buttons & PDD_DGT_TB) | (buttons & PDD_DGT_TX)) {
            work->cursor = menu->cancel;
            work->step = 0;
            work->mode = MENU_CHOSEN;
            if (menu->sounds != NULL) {
                sound_play(menu->sounds->cancel, 0, 0, 0);
            }
        }
        if (cursor != work->cursor) {
            work->idle = 0;
            work->pulse = PULSE_MOVED;
            if (menu->sounds != NULL) {
                sound_play(menu->sounds->move, 0, 0, 0);
            }
        } else if (work->idle < 0x7fff) {
            work->idle++;
        }
        break;
    case MENU_CHOSEN:
        work->step++;
        if (work->step >= CHOSEN_STEPS) {
            work->step = 0;
            work->mode = MENU_CLOSE;
            if (menu->sounds != NULL) {
                sound_play(menu->sounds->close, 0, 0, 0);
            }
        }
        break;
    case MENU_CLOSE:
        work->tint = 0xffffffff;
        work->scale_x = menu_close_scales[work->step].x;
        work->scale_y = menu_close_scales[work->step].y;
        work->step++;
        if (work->step >= SCALE_STEPS) {
            menu_reset(work);
            work->mode = MENU_DONE;
        }
        break;
    case MENU_DONE:
        break;
    }
}

/* draw the frame, the items and the cursor, then call the menu's own
 * draw. nothing draws while tint is 0, or while both scales are */
static void menu_draw(struct task *task)
{
    struct menu_work *work = task->extra;
    struct menu_view view;

    if (work->tint == 0) {
        return;
    }
    if (work->scale_x == 0.0F && work->scale_y == 0.0F) {
        return;
    }
    screen_quad_tint(work->tint);
    menu_draw_frame(work);
    menu_draw_items(work);
    if (work->menu->draw != NULL) {
        view.x = work->menu->x;
        view.y = work->menu->y;
        view.z = work->menu->z;
        view.scale_x = work->scale_x;
        view.scale_y = work->scale_y;
        view.cursor = work->cursor;
        work->menu->draw(&view);
    }
}

struct task *menu_create(void)
{
    struct task *task;
    struct menu_work *work;

    /* @hack spelling: the assignments in the test, which tests task_create's
     * result in R0 before its copy to R14 (TST R0,R0 at 0x8c0ac81c);
     * assigned ahead of the test, 2 of 27 words differ */
    if ((menu_task = task = task_create(TASK_EXTRA, 3, menu_step)) != NULL) {
        work = heap_alloc(sizeof(struct menu_work));
        /* @bug memory: the work replaces the 16-byte extra task_create gave
         * the task, which leaks (MOV.L R0,@(44,R14) at 0x8c0ac82a) */
        task->extra = work;
        menu_init(task);
        task->step = menu_step;
        task->draw = menu_draw;
    }
    return task;
}

void menu_kill(void)
{
    if (menu_task != NULL) {
        task_kill(menu_task);
    }
}

void menu_show(const struct menu *menu, Sint8 cursor, Sint8 *disabled)
{
    struct menu_work *work;

    /* @hack spelling: the first store through menu_task->extra, not work, which
     * keeps menu_task's address in R14 across the calls (0x8c0ac85a to
     * 0x8c0ac866); with work set once here for every store, 18 of 29 words
     * differ */
    ((struct menu_work *)menu_task->extra)->menu = menu;
    menu_set_disabled(disabled);
    menu_set_cursor(cursor);
    menu_set_idle(0);
    work = menu_task->extra;
    work->blink = 0;
    work->mode = MENU_CHOOSE;
}

void menu_popup(const struct menu *menu, Sint8 cursor, Sint8 *disabled)
{
    /* @hack spelling: both stores through menu_task->extra, which loads
     * menu_task and its extra again for the second (0x8c0ac8a0 to
     * 0x8c0ac8a8); through a local, 40 of 43 words differ */
    ((struct menu_work *)menu_task->extra)->menu = menu;
    ((struct menu_work *)menu_task->extra)->mode = MENU_OPEN;
    menu_set_disabled(disabled);
    menu_set_cursor(cursor);
    menu_set_idle(0);
    if (menu->sounds != NULL) {
        sound_play(menu->sounds->open, 0, 0, 0);
    }
}

void menu_popup_default(const struct menu *menu)
{
    menu_popup(menu, 0, NULL);
}

int menu_result(void)
{
    struct menu_work *work = menu_task->extra;

    if (work->mode >= MENU_DONE) {
        return work->cursor;
    }
    return -1;
}

Bool menu_close(void)
{
    struct menu_work *work = menu_task->extra;

    /* @hack redundant: the else after the return, which stores step through R1,
     * work plus 38 (0x8c0ac962 to 0x8c0ac968); as two ifs, SHC stores
     * through @(R0,R4), the function runs 2 bytes short and 10 of 21 words
     * differ */
    if (work->mode < MENU_CHOOSE) {
        return FALSE;
    } else if (work->mode == MENU_CHOOSE) {
        work->step = CHOSEN_STEPS;
        work->mode = MENU_CHOSEN;
    }
    return TRUE;
}

Sint16 menu_get_idle(void)
{
    struct menu_work *work = menu_task->extra;

    return work->idle;
}

void menu_set_idle(Sint16 idle)
{
    struct menu_work *work = menu_task->extra;

    work->idle = idle;
}
