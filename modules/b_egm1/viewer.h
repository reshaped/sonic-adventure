/* the Egg Hornet on show, for a second pad to move and pose */
#ifndef B_EGM1_VIEWER_H
#define B_EGM1_VIEWER_H
#include <sg_xpt.h>

struct task_work;

/* start the viewer, with a boss of its own. nothing in B_EGM1 calls it */
void egg_hornet_viewer_create(void);

/* end the viewer's boss on its next step. viewer_step calls it as the
 * viewer closes */
void viewer_boss_kill(void);
/* ask the viewer's boss's motion for action. the boss must not have
 * ended */
void viewer_boss_set_action(Uint16 action);
/* the viewer's boss's work. the boss must not have ended */
struct task_work *viewer_boss_work(void);
/* the action the viewer's boss plays. the boss must not have ended, and
 * nothing calls it */
Uint16 viewer_boss_get_action(void);
/* set the mode of the jets egg_hornet_jets_create made last, which need not
 * be the viewer's. nothing calls it */
void viewer_set_jets_mode(Uint8 mode);
/* egg_hornet_jets_done_steps's count, for the jets egg_hornet_jets_create made
 * last. nothing calls it */
Uint32 viewer_jets_done_steps(void);

#endif
