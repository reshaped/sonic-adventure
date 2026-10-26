/* the debug menu: a page of items, each starting a task as the menu's
 * child */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "debug/editor.h"
#include "debug/menu.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/task_desc.h"

enum debug_menu_mode {
    DEBUG_MENU_CLOSED,
    DEBUG_MENU_OPEN,
    DEBUG_MENU_RUNNING /* the chosen item's task, until it ends */
};

struct debug_menu {
    Sint8 mode;
    Sint8 count;
    Sint8 selection;
    Sint16 column;
    Sint16 row;
    struct debug_menu_item *items;
};

#define MENU(task) ((struct debug_menu *)(task)->motion)

static void debug_menu_sound(Sint8 result);
static void debug_menu_init(struct task *task, struct debug_menu_page *page);
static void debug_menu_step(struct task *task);
static void debug_menu_draw(struct task *task);
static void debug_menu_destroy(struct task *task);
static enum debug_menu_result debug_menu_choose(struct task *task);

/* the menu's task. the step draws the menu, so the task has no draw */
static const struct task_desc debug_menu_desc = {
    sizeof(struct debug_menu),
    0,
    (void (*)(struct task *, void *))debug_menu_init,
    debug_menu_step,
    NULL,
    debug_menu_destroy};

/* each result's sound, or -1 for none */
static const int debug_menu_sounds[DEBUG_MENU_RESULTS] = {
    -1, SE_ICE_BREAK, SE_CHOOSE, SE_MENU_CLOSE, SE_MOVE, SE_MENU_OPEN};

struct task *debug_menu_create(struct debug_menu_page *page)
{
    struct task *task;

    task = task_create_desc(0, &debug_menu_desc, page);
    return task;
}

Sint8 debug_menu_select(struct task *task, int *selection, int count,
                        enum debug_menu_result (*choose)(struct task *task))
{
    Uint32 press = debug_pad_press();
    Uint32 repeat = debug_pad_repeat();
    int at = *selection;
    int last = count - 1;
    enum debug_menu_result result = DEBUG_MENU_IDLE;

    if (press & PDD_DGT_TB) {
        result = DEBUG_MENU_CLOSE;
    } else if (press & PDD_DGT_TA) {
        result = choose(task);
    } else if (repeat & PDD_DGT_KD) {
        if (at < last) {
            at++;
        } else {
            at = 0;
        }
    } else if (repeat & PDD_DGT_KU) {
        if (at > 0) {
            at--;
        } else {
            at = last;
        }
    }
    if (at != *selection) {
        result = DEBUG_MENU_MOVE;
        *selection = at;
    }
    debug_menu_sound(result);
    return result;
}

Uint32 debug_pad_on(void)
{
    return per_info[3]->on;
}

Uint32 debug_pad_press(void)
{
    return per_info[3]->press;
}

Uint32 debug_pad_repeat(void)
{
    return pad_press_repeat[0];
}

static void debug_menu_sound(Sint8 result)
{
    int se;

    if (result >= 0 && result < DEBUG_MENU_RESULTS) {
        se = debug_menu_sounds[result];
        if (se >= 0) {
            sound_play(se, 0, 0, 0);
        }
    }
}

/* @hack type: the page taken as its own type, which debug_menu_desc casts
 * to init's; taking a void pointer, SHC copies the page to R6 before the
 * null test (TST R5,R5 at 0x8c0a740a) and 32 of 34 words differ */
static void debug_menu_init(struct task *task, struct debug_menu_page *page)
{
    int count = 0;
    struct debug_menu_item *items = NULL;
    struct debug_menu_item *item;
    struct debug_menu *menu;

    if (page != NULL) {
        items = page->items;
        for (item = items; item->desc != NULL; item++) {
            count++;
        }
    }
    if (count <= 0) {
        task_kill(task);
        return;
    }
    menu = MENU(task);
    menu->mode = DEBUG_MENU_CLOSED;
    menu->count = count;
    menu->selection = 0;
    menu->column = page->column;
    menu->row = page->row;
    menu->items = items;
}

static void debug_menu_step(struct task *task)
{
    Uint32 on = debug_pad_on();
    Uint32 press = debug_pad_press();
    struct debug_menu *menu;
    int selection;
    Sint8 result;

    if (edit_mode) {
        return;
    }
    if (on & PDD_DGT_TX) {
        return;
    }
    menu = MENU(task);
    switch (menu->mode) {
    case DEBUG_MENU_CLOSED:
        if ((on & PDD_DGT_TY) && (press & PDD_DGT_ST)) {
            debug_menu_sound(DEBUG_MENU_START);
            pad_disable();
            pause_disable();
            menu->mode = DEBUG_MENU_OPEN;
        }
        break;
    case DEBUG_MENU_OPEN:
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        selection = menu->selection;
        result =
            debug_menu_select(task, &selection, menu->count, debug_menu_choose);
        menu->selection = selection;
        switch (result) {
        case DEBUG_MENU_DONE:
        case DEBUG_MENU_START:
            menu->mode = DEBUG_MENU_RUNNING;
            /* fall through */
        case DEBUG_MENU_IDLE:
        case DEBUG_MENU_NOTHING:
        case DEBUG_MENU_MOVE:
            pause_disable();
            break;
        case DEBUG_MENU_CLOSE:
            pad_enable();
            pause_enable();
            menu->mode = DEBUG_MENU_CLOSED;
            break;
        }
        break;
    case DEBUG_MENU_RUNNING:
        task_step_children(task);
        if (task->child == NULL) {
            menu->mode = DEBUG_MENU_OPEN;
        }
        pause_disable();
        break;
    }
    debug_menu_draw(task);
}

/* draw the open menu, on a frame's first step only */
static void debug_menu_draw(struct task *task)
{
    struct debug_menu *menu;
    struct debug_menu_item *item;
    const char *label;
    int column;
    int row;

    if (frame_step != 0) {
        return;
    }
    menu = MENU(task);
    switch (menu->mode) {
    case DEBUG_MENU_CLOSED:
        break;
    case DEBUG_MENU_OPEN:
        column = menu->column;
        row = menu->row;
        njPrintColor(0xff00ff00);
        njPrintC(NJM_LOCATION(column, row), "<Main menu>");
        row++;
        njPrintColor(0xffff0000);
        njPrintC(NJM_LOCATION(column, row + menu->selection), ">");
        column++;
        njPrintColor(0xffffffff);
        for (item = menu->items; item->desc != NULL; item++, row++) {
            label = item->label;
            if (label != NULL) {
                njPrintC(NJM_LOCATION(column, row), label);
            }
        }
        break;
    case DEBUG_MENU_RUNNING:
        break;
    }
}

static void debug_menu_destroy(struct task *task)
{
    pad_enable();
    pause_enable();
}

static enum debug_menu_result debug_menu_choose(struct task *task)
{
    struct debug_menu *menu = MENU(task);
    struct debug_menu_item *item = &menu->items[menu->selection];
    const struct task_desc *desc = item->desc;
    struct task *child;
    enum debug_menu_result result = DEBUG_MENU_NOTHING;

    if (desc != NULL) {
        child = task_create_desc_child(task, desc, item->arg);
        if (child != NULL) {
            result = DEBUG_MENU_START;
        }
    }
    return result;
}
