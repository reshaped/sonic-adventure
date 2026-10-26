/* the pad's rumble, as a missile blows up and as the boss lands from its
 * dive */
#include "b_egm1/rumble.h"
#include "vmu/vibration.h"

/* each rumble's power, freq and time, as vibration_start takes them. it
 * clamps STRONG_POWER to 4 */
#define WEAK_POWER 3
#define WEAK_FREQ 8
#define WEAK_TIME 1
#define STRONG_POWER 6
#define STRONG_FREQ 59
#define STRONG_TIME 3

void rumble_weak(void)
{
    vibration_start(0, WEAK_POWER, WEAK_FREQ, WEAK_TIME);
}

void rumble_strong(void)
{
    vibration_start(0, STRONG_POWER, STRONG_FREQ, STRONG_TIME);
}
