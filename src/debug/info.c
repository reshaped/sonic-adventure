/* the debug info menu: one item, which turns the debug info on and off */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/info.h"
#include "debug/menu.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"

struct debug_info_menu {
    Sint8 count;
    Sint8 selection;
    Sint16 column;
    Sint16 row;
};

#define INFO_MENU(task) ((struct debug_info_menu *)(task)->motion)

static void debug_info_menu_init(struct task *task, void *arg);
static void debug_info_menu_step(struct task *task);
static void debug_info_menu_draw(struct task *task);
static enum debug_menu_result debug_info_menu_choose(struct task *task);

/* the info menu's task, which nothing creates. init takes the menu's
 * column and row from a debug_menu_page as its arg. the step draws the
 * menu, so the task has no draw */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct task_desc debug_info_menu_desc = {
    sizeof(struct debug_info_menu), 0,    debug_info_menu_init,
    debug_info_menu_step,           NULL, NULL};

static Sint8 debug_info = 1;

Bool debug_info_on(void)
{
    return debug_info;
}

static void debug_info_menu_init(struct task *task, void *arg)
{
    struct debug_info_menu *menu = INFO_MENU(task);
    struct debug_menu_page *page = arg;

    menu->count = 1;
    menu->selection = 0;
    menu->column = page->column;
    menu->row = page->row;
}

static void debug_info_menu_step(struct task *task)
{
    struct debug_info_menu *menu = INFO_MENU(task);
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int selection = menu->selection;
    Sint8 result;

    result = debug_menu_select(task, &selection, menu->count,
                               debug_info_menu_choose);
    menu->selection = selection;
    switch (result) {
    case DEBUG_MENU_IDLE:
    case DEBUG_MENU_NOTHING:
    case DEBUG_MENU_DONE:
    case DEBUG_MENU_MOVE:
    case DEBUG_MENU_START:
        break;
    case DEBUG_MENU_CLOSE:
        task_kill(task);
        break;
    }
    debug_info_menu_draw(task);
}

/* draw the menu, on a frame's first step only */
static void debug_info_menu_draw(struct task *task)
{
    struct debug_info_menu *menu;
    int column;
    int row;

    if (frame_step != 0) {
        return;
    }
    menu = INFO_MENU(task);
    column = menu->column;
    row = menu->row;
    njPrintColor(0xff00ff00);
    njPrintC(NJM_LOCATION(column, row), "<Debug info menu>");
    row++;
    njPrintColor(0xffff0000);
    njPrintC(NJM_LOCATION(column, row + menu->selection), ">");
    column++;
    njPrintColor(0xffffffff);
    njPrint(NJM_LOCATION(column, row), "DEBUG INFO: %s",
            debug_info ? "ON" : "OFF");
}

static enum debug_menu_result debug_info_menu_choose(struct task *task)
{
    struct debug_info_menu *menu = INFO_MENU(task);
    enum debug_menu_result result = DEBUG_MENU_NOTHING;

    /* @hack spelling: a switch of one case, which SHC tests with CMP/EQ
     * #0,R0 (0x8c0a77d2); as an if, it tests with TST R0,R0 and 1 of 13 words
     * differs */
    switch (menu->selection) {
    case 0:
        debug_info = !debug_info;
        result = DEBUG_MENU_DONE;
        break;
    }
    return result;
}
