/* the menus' buttons: A or Start chooses, and B or X backs out */
#ifndef ADVERTISE_BUTTONS_H
#define ADVERTISE_BUTTONS_H

/* pad_choose_pressed is nonzero when the first pad has just pressed A or
 * Start, and pad_back_pressed when it has just pressed B or X */
int pad_choose_pressed(void);
int pad_back_pressed(void);

#endif
