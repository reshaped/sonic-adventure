/* run the compiled stick atan2 and the shipped runtime at each pad position
 * outside the dead zone, where the game takes a heading */
#include <ninja.h>
#include <shinobi.h>

#include "system/pad.h"
#include "system/util.h"

extern float stick_atan2f(float y, float x);
extern float atan2f(float y, float x);
int _errno;

static int axis(int raw)
{
    int value = raw * 256;
    return value < 0 ? value + PAD_DEAD_ZONE : value - PAD_DEAD_ZONE;
}

/* inspect these after done becomes 1, or dump differences for dreamcast.py */
short differences[256][256];
volatile int done;
int changed, worst;

__attribute__((section(".text.boot"))) void start(void)
{
    unsigned fpscr = 0x40001;

    __asm__ volatile("lds %0,fpscr" : : "r"(fpscr) : "memory");
    for (int y = -128; y < 128; y++) {
        for (int x = -128; x < 128; x++) {
            if ((x < -12 || x > 12) || (y < -12 || y > 12)) {
                float a = axis(y);
                float b = axis(x);
                int exact = (int)(atan2f(a, b) * ANGLE_PER_RADIAN);
                int fast = (int)(stick_atan2f(a, b) * ANGLE_PER_RADIAN);
                short delta = (short)(fast - exact);
                differences[y + 128][x + 128] = delta;
                int distance = delta < 0 ? -delta : delta;
                if (distance > worst) {
                    worst = distance;
                }
                changed += delta != 0;
            }
        }
    }
    __asm__ volatile("" : : : "memory");
    done = 1;
    for (;;) {
        __asm__ volatile("nop");
    }
}
