/* act 2's cargo ride: the train's run through the tunnel, 10000 units up */
#ifndef STG12_CARGO_RIDE_H
#define STG12_CARGO_RIDE_H

#include <sg_xpt.h>

/* TRUE while the ride runs. the train's, the tunnel's and the world's tasks
 * end themselves once it is FALSE */
extern Bool cargo_ride_on;
/* TRUE from when the ride's end switch turns on until cargo_ride_start */
extern Bool cargo_ride_ending;

/* clear the ride's flags and its end switch, and make the ride's task. the
 * task starts the ride once a player comes near */
void cargo_ride_start(void);

#endif
