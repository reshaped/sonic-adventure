/* the debug menu: a page of items, each starting a task as the menu's
 * child */
#ifndef DEBUG_MENU_H
#define DEBUG_MENU_H
#include <sg_xpt.h>

struct task;
struct task_desc;

/* what a step of debug_menu_select returns. each result but
 * DEBUG_MENU_IDLE has a sound, which debug_menu_select plays */
enum debug_menu_result {
    DEBUG_MENU_IDLE, /* no B, no A and no move */
    DEBUG_MENU_NOTHING, /* choose did nothing */
    DEBUG_MENU_DONE, /* choose acted, and started no task */
    DEBUG_MENU_CLOSE, /* B was pressed, to close the menu */
    DEBUG_MENU_MOVE, /* the selection moved */
    DEBUG_MENU_START, /* choose started a task */
    DEBUG_MENU_RESULTS
};

/* an item of a page, whose task starts as the menu's child */
struct debug_menu_item {
    const char *label; /* can be null */
    const struct task_desc *desc; /* null ends the page */
    void *arg; /* for the task's init */
};

/* a page of items, and the column and row, in characters, where it shows */
struct debug_menu_page {
    Sint16 column;
    Sint16 row;
    struct debug_menu_item *items;
};

/* create the debug menu's task for page, and return it, or null. page's
 * items must outlive the task, and a null or empty page kills it.
 *
 * Start pressed with Y held on the fourth pad opens the menu, with
 * DEBUG_MENU_START's sound, and B closes it. the menu does nothing while
 * the set editor is open or X is held on the fourth pad.
 *
 * opening the menu turns the pads and pausing off, as pad_disable and
 * pause_disable do, and each step of the open menu turns pausing off
 * again. closing the menu, or destroying its task, turns both back on */
struct task *debug_menu_create(struct debug_menu_page *page);
/* run a step of a selection among count items, play the result's sound,
 * and return the result. the first of these inputs decides it. B pressed
 * on the fourth pad gives DEBUG_MENU_CLOSE. A pressed there gives choose's
 * result. down or up on the first pad moves *selection, which wraps, and
 * gives DEBUG_MENU_MOVE. the move repeats as debug_pad_repeat does. with
 * none of them, the result is DEBUG_MENU_IDLE */
Sint8 debug_menu_select(struct task *task, int *selection, int count,
                        enum debug_menu_result (*choose)(struct task *task));
/* the fourth pad's buttons held, and pressed, from per_info[3] */
Uint32 debug_pad_on(void);
Uint32 debug_pad_press(void);
/* the first pad's presses, with held directions repeating, which the game's
 * menus move by */
Uint32 debug_pad_repeat(void);

#endif
