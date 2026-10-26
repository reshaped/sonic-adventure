/* the crowd's cheer: a racer the camera turns to, and the presses on any pad
 * that cheer it on */
#ifndef AL_RACE_CHEER_H
#define AL_RACE_CHEER_H
#include <sg_xpt.h>

/* the cheer's states. CHEER_WAIT waits to choose a racer, and CHEER_CHOSEN
 * waits for a press once one is chosen. CHEER_CHEERING cheers it on, and
 * CHEER_DONE sets the next wait */
enum cheer_state { CHEER_WAIT, CHEER_CHOSEN, CHEER_CHEERING, CHEER_DONE };

struct race_cheer {
    /* in CHEER_WAIT, the wait ends once the game's timer passes it. in
     * CHEER_CHOSEN, it counts the steps since the choice, and in
     * CHEER_CHEERING, those since the round began */
    Uint16 timer;
    Uint8 state; /* an enum cheer_state */
    Uint8 rounds; /* the rounds of three presses done while cheering */
    Uint8 lane; /* the lane of the racer the crowd chose */
    Uint8 presses; /* the presses in the round so far */
    Sint8 sound_wait; /* the steps before a press sounds again */
    Uint8 unk_07;
};

#endif
