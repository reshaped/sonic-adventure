/* E-102's modes, set from outside his step: a leap, a fall, a skate. each
 * function takes his player task */
#ifndef PLAYER_E102_MODE_H
#define PLAYER_E102_MODE_H

struct task;

/* a leap, up at twice his jump speed and 1.5625 slower forward */
void e102_leap(struct task *task);
/* a fall, from the speed he has */
void e102_fall(struct task *task);
/* a skate's start, from a crawl of 0.1 forward, and a brake out of a
 * skate */
void e102_skate_start(struct task *task);
void e102_skate_brake(struct task *task);

#endif
