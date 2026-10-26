/* streamers, the ribbons the crackers fire */
#ifndef AL_RACE_STREAMER_H
#define AL_RACE_STREAMER_H
#include <ninja.h>

/* color the four palettes the streamers draw with */
void streamers_init(void);
/* fire a streamer from pos, in a random one of the four palettes. its head
 * flies at speed, nudged by up to 0.2 along each axis. the streamer ends at
 * its 1000th step, or once both its ends are below height 10. it fires none
 * when it can't make its task or its state */
void streamer_create(NJS_POINT3 *pos, NJS_VECTOR *speed);

#endif
