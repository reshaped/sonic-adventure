/* the pad's rumble, as a missile blows up and as the boss lands from its
 * dive */
#ifndef B_EGM1_RUMBLE_H
#define B_EGM1_RUMBLE_H

/* shake pad 0's pack: rumble_weak for a missile's blast, and rumble_strong
 * for the boss's landing. rumble_strong shakes it with more power, a higher
 * freq and a longer time */
void rumble_weak(void);
void rumble_strong(void);

#endif
