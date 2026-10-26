/* act changes in place, without ending the stage */
#ifndef STAGE_ACT_H
#define STAGE_ACT_H
#include <sg_xpt.h>

/* change act in place to act_current + by. it ends the streams as
 * music_end does, and moves the land, placements, camera and stage on.
 * player 0 then goes to its start, and a music of 0 or more starts as
 * music_start starts it. the act must be in the stage module already
 * loaded, as stage_change_act requires */
void act_change(Sint16 by, int music);

#endif
