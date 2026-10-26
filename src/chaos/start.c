/* the flags that hand Chaos from his event to his fight */
#include "chaos/start.h"
#include "draw/motion.h"

void chaos_start(void)
{
    boss_waiting = 1;
    chaos_handed_over = 0;
}

void chaos_end_event(void)
{
    chaos_handed_over = 1;
}
