/* the act menu, Change ACT, the one item of the cart course's debug menu.
 * it picks another act */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/menu.h"
#include "minicart/act_menu.h"
#include "minicart/cart.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"

/* the act menu's record, which task_apply_desc puts in its task's motion */
struct act_menu {
    Uint8 act; /* the act that plays */
    Uint8 selection;
    Sint8 count; /* the acts the menu lists */
    Sint16 column;
    Sint16 row;
};

#define MENU(task) ((struct act_menu *)(task)->motion)

/* the column and row, in characters, where the act menu prints. its item
 * passes them to act_menu_init */
struct act_menu_place {
    Sint16 column;
    Sint16 row;
};

#define COLOR_GREEN 0xff00ff00
#define COLOR_RED 0xffff0000
#define COLOR_WHITE 0xffffffff
#define COLOR_GRAY 0xff7f7f7f

static void act_menu_init(struct task *task, struct act_menu_place *place);
static void act_menu_step(struct task *task);
static void act_menu_draw(struct task *task);
static enum debug_menu_result act_menu_choose(struct task *task);

static const char *const act_menu_labels[CART_ACTS] = {
    "ACT1", "ACT2", "ACT3", "ACT4", "ACT5", "ACT6",
};
/* the act menu's kind of task. its step draws the menu, so it has no draw.
 * act_menu_init takes its arg as a struct act_menu_place, through a cast to
 * init's type */
static const struct task_desc act_menu_desc = {
    sizeof(struct act_menu),
    0,
    (void (*)(struct task *, void *))act_menu_init,
    act_menu_step,
    NULL,
    NULL};
static const struct act_menu_place act_menu_place = {2, 5};

static struct debug_menu_item act_menu_items[2] = {
    {"Change ACT", &act_menu_desc, (void *)&act_menu_place},
    {NULL, NULL, NULL},
};
static struct debug_menu_page act_menu_page = {2, 5, act_menu_items};

/* the chosen act less the act that played. it is 0 when no act has been
 * chosen since act_menu_clear */
static Sint8 act_menu_change;

struct task *act_menu_create(void)
{
    return debug_menu_create(&act_menu_page);
}

void act_menu_clear(void)
{
    act_menu_change = 0;
}

int act_menu_choice(void)
{
    return act_menu_change;
}

/* start the menu at place, with the selection on the act that plays */
static void act_menu_init(struct task *task, struct act_menu_place *place)
{
    Uint32 stage_act = stage_get_stage_act();
    Uint8 act = stage_act;
    struct act_menu *menu = MENU(task);

    menu->act = act;
    menu->selection = act;
    menu->count = CART_ACTS;
    menu->column = place->column;
    menu->row = place->row;
}

/* run the selection, then draw the menu. the menu ends once another act is
 * chosen, or once B closes it */
static void act_menu_step(struct task *task)
{
    struct act_menu *menu = MENU(task);
    int selection = menu->selection;
    Sint8 result;

    result = debug_menu_select(task, &selection, menu->count, act_menu_choose);
    menu->selection = selection;
    switch (result) {
    case DEBUG_MENU_IDLE:
    case DEBUG_MENU_NOTHING:
    case DEBUG_MENU_MOVE:
        break;
    case DEBUG_MENU_DONE:
    case DEBUG_MENU_CLOSE:
    case DEBUG_MENU_START:
        task_kill(task);
        break;
    }
    act_menu_draw(task);
}

/* draw the title, the cursor at the selection and the acts' labels, the act
 * that plays in gray. it draws only on a frame's first step */
static void act_menu_draw(struct task *task)
{
    struct act_menu *menu;
    const char *const *label;
    int column;
    int row;
    Uint8 act;
    Uint8 count;
    Uint8 i;

    if (frame_step != 0) {
        return;
    }
    menu = MENU(task);
    column = menu->column;
    row = menu->row;
    njPrintColor(COLOR_GREEN);
    njPrintC(NJM_LOCATION(column, row), "<Change ACT menu>");
    row++;
    njPrintColor(COLOR_RED);
    njPrintC(NJM_LOCATION(column, row + menu->selection), ">");
    column++;
    count = menu->count;
    act = menu->act;
    i = 0;
    label = act_menu_labels;
    njPrintColor(COLOR_WHITE);
    for (; i < act; i++, row++, label++) {
        njPrintC(NJM_LOCATION(column, row), *label);
    }
    njPrintColor(COLOR_GRAY);
    njPrintC(NJM_LOCATION(column, row), *label);
    label++;
    i++;
    row++;
    njPrintColor(COLOR_WHITE);
    for (; i < count; i++, row++, label++) {
        njPrintC(NJM_LOCATION(column, row), *label);
    }
}

/* choose the selected act. the act that plays gives DEBUG_MENU_NOTHING. any
 * other gives DEBUG_MENU_DONE, and sets act_menu_change to the selected act
 * less the act that plays */
static enum debug_menu_result act_menu_choose(struct task *task)
{
    struct act_menu *menu = MENU(task);
    enum debug_menu_result result = DEBUG_MENU_NOTHING;

    if (menu->selection != menu->act) {
        result = DEBUG_MENU_DONE;
        act_menu_change = menu->selection - menu->act;
    }
    return result;
}
