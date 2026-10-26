/* Zero's laser: a ray from a point Zero holds toward an aim that drifts
 * after the player. the ray passes through walls and stops at the floor.
 * it scorches the floor where it scrapes along it */
#ifndef B_ROBO_BEAM_H
#define B_ROBO_BEAM_H
#include <ninja.h>
#include <sg_xpt.h>

enum erobo_beam_mode {
    EROBO_BEAM_RESET, /* idle from the next step */
    EROBO_BEAM_FIRE, /* start a new shot, which is on from the next step */
    /* the laser's fading trail shortens a step at a time, then the laser
     * idles */
    EROBO_BEAM_STOP,
    EROBO_BEAM_IDLE,
    /* the laser grows to its full length, and its aim drifts after the
     * player */
    EROBO_BEAM_ON
};

/* reset the laser to fire from origin. its aim moves by velocity each
 * step, and a new shot starts out the way angle faces. the laser keeps the
 * three pointers, not copies of what they point to */
void erobo_beam_init(NJS_POINT3 *origin, NJS_VECTOR *velocity, Angle *angle);
/* forget the origin erobo_beam_init gave. the laser's steps in
 * EROBO_BEAM_FIRE and EROBO_BEAM_ON read through it, and so can
 * erobo_beam_reached. after this, a caller does neither until
 * erobo_beam_init gives an origin again */
void erobo_beam_detach(void);
void erobo_beam_set_mode(enum erobo_beam_mode mode);
/* whether the laser has reached the player since it last fired. a call that
 * finds it hasn't tests the laser against the player again, unless
 * erobo_beam_block blocked it. a hit holds until the laser fires again, and
 * nothing else runs the test */
Bool erobo_beam_reached(void);
/* keep the laser from reaching the player until it fires again. nothing
 * calls it */
void erobo_beam_block(void);
/* run the laser's mode a step */
void erobo_beam_step(void);
/* on a frame's first step, draw the laser while it is on, and its fading
 * trail */
void erobo_beam_draw(void);

#endif
