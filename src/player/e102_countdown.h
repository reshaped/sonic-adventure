/* E-102's last seconds: his time in a big digit, pulsing */
#ifndef PLAYER_E102_COUNTDOWN_H
#define PLAYER_E102_COUNTDOWN_H

struct task;

/* the step of the countdown, a child of E-102's player task */
void e102_countdown_step(struct task *task);

#endif
