/* EV00B7.PRS's picture of Froggy, high behind Eggman */
#ifndef EV00B7_FROG_SCREEN_H
#define EV00B7_FROG_SCREEN_H

/* make the task that draws Froggy six times his size and pressed flat,
 * turning. a second create before a kill leaves the first task running,
 * and the kill then reaches only the second */
void ev00b7_frog_screen_create(void);
/* kill that task, if there is one */
void ev00b7_frog_screen_kill(void);

#endif
