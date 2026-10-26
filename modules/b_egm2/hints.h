/* the Egg Walker's smoke sound, Eggman's voice lines and Tikal's hints */
#ifndef B_EGM2_HINTS_H
#define B_EGM2_HINTS_H
#include <sg_xpt.h>

/* forget the smoke's sound without stopping it. the boss's EGG_WALKER_INIT
 * step calls it */
void egg_walker_smoke_reset(void);
/* have the smoke's sound play on for a fixed count of calls of
 * egg_walker_smoke_step from now. each hold starts the count over */
void egg_walker_smoke_hold(void);
/* start the smoke's sound once a hold asks for it. stop it once the hold
 * runs out. the boss calls it once a step */
void egg_walker_smoke_step(void);
/* play Eggman's voice line 0 to 4. any other line plays nothing */
void egg_walker_voice(Uint8 line);
/* show Tikal's hint on the legs after a retry, or once the time passes
 * 1:30:00. it shows at most once, and no call after one that finds a leg
 * hit shows it. each call also counts down the wait that follows either
 * hint. the boss's walk calls it each step */
void egg_walker_hint_legs(void);
/* show Tikal's hint on the cockpit after a third leg hit. it shows at most
 * once, and no call after one that finds the boss hit shows it. no call
 * shows it while egg_walker_hint_legs counts down the wait after a hint.
 * the boss's walk calls it each step */
void egg_walker_hint_cockpit(void);
/* end Tikal's hint, as tikal_hint_end does. the boss's destructor calls
 * it */
void egg_walker_hint_end(void);
/* count a hit on one of the boss's legs, for the hints */
void egg_walker_count_fall(void);

#endif
