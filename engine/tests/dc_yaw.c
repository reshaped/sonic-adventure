/* Run the moving-blade geometry without a stage or player controller. */
#ifdef DREAMCAST
#include "engine/tests/dc_yaw.h"

volatile int result;
__attribute__((section(".text.boot"))) void start(void)
{
    result = fan_geometry() + 1;
    for (;;) {
        __asm__ volatile("nop");
    }
}
#else
#include "engine/native/intrinsics.h"
#include "play_math.h"
#include "system/util.h"
#undef LINKED_LAST
#define LINKED_LAST(name)
#include "engine/tests/dc_yaw.h"

void _builtin_fsca(Angle angle, Float *sine, Float *cosine)
{
    Float pair[2];
    sa_fsca((Uint32)angle, pair);
    *sine = pair[0];
    *cosine = pair[1];
}

Float njSqrt(Float x)
{
    return sa_fsqrt(x);
}

int main(void)
{
    return fan_geometry();
}
#endif
