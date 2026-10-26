/* the dash panel, O AXPNL or O DUSH PANEL in the object lists. a player
 * who touches it on the ground is sent off along its angles */
#ifndef OBJECT_DASH_PANEL_H
#define OBJECT_DASH_PANEL_H

#include "system/task.h"

/* the object lists' step for O AXPNL and O DUSH PANEL */
void dash_panel_step(struct task *task);

#endif
