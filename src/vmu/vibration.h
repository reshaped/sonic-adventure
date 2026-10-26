/* vibration: shaking a pad through the pack in one of its sockets */
#ifndef VMU_VIBRATION_H
#define VMU_VIBRATION_H

/* vibrate the first ready pack in pad's sockets, pad 0 to 3, unless a demo
 * plays. power is clamped to -4..4, where 1 and -1 become 2 and -2, and freq
 * to 7..59. a positive time, up to 255, sets the pack's stop time and keeps
 * it shaking until then. the shaking starts on a task's second step, or never
 * if the task or its parameters can't be allocated */
void vibration_start(int pad, int power, int freq, int time);
/* vibrate as vibration_start does, without a stop time, and fade the shaking
 * out. the fade's inc is 255 / span, in whole numbers. span is (freq *
 * length) / (power * 4), from the clamped power and freq, and 1 where that
 * is below 1. power must not be 0, which span divides by */
void vibration_start_fading(int pad, int power, int freq, int length);

#endif
