/* O KARTPASS pass that opens the Twinkle Park elevator and Twinkle Circuit for
 * Tails, Knuckles and E-102 */
#ifndef ADV00_KART_PASS_H
#define ADV00_KART_PASS_H

#include "system/task.h"

/* the step the object list names for O KARTPASS. once Sonic, Amy and Big
 * have cleared Twinkle Park, it sets the CART_PASSPORT flags of Tails,
 * Knuckles and E-102. the pass lies for each of the three from that flag
 * until its ENTRANCE_CIRCUIT flag is set. a dropped pass goes once it has lain
 * 1801 steps in all, counting every drop since the task started or the pass
 * opened the circuit. carried to the way into Twinkle Circuit, the pass sets
 * the character's ENTRANCE_CIRCUIT flag. carried to the Twinkle Park
 * elevator, it sets the character's TPARK_ELEVATOR flag */
void kart_pass_init(struct task *task);

#endif
