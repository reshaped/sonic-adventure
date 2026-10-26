/* the act menu, Change ACT, the one item of the cart course's debug menu.
 * it picks another act */
#ifndef MINICART_ACT_MENU_H
#define MINICART_ACT_MENU_H

struct task;

/* create the debug menu, with Change ACT its one item, and return its task,
 * or null */
struct task *act_menu_create(void);
/* act_menu_clear clears the menu's choice. act_menu_choice returns it: the
 * chosen act less the act that played. it is 0 when no act has been chosen
 * since the clear */
void act_menu_clear(void);
int act_menu_choice(void);

#endif
