/* a story script that does nothing, which no story table names. it lies
 * between Sonic's SS SEWER - CASINO and WAKE scripts */
#include "adv00/script_sonic_unused.h"
#include "progress/story.h"

static void script_sonic_unused_init(void);
static void script_sonic_unused_step(int seq);

struct story_script script_sonic_unused = {script_sonic_unused_init,
                                           script_sonic_unused_step};

static void script_sonic_unused_init(void)
{
}

static void script_sonic_unused_step(int seq)
{
}
