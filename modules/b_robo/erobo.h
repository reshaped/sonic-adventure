/* Zero, the boss of the Egg Carrier deck fight */
#ifndef B_ROBO_EROBO_H
#define B_ROBO_EROBO_H
#include <ninja.h>
#include <sg_xpt.h>

/* the height of the deck's floor, and the x and z of its middle */
#define DECK_FLOOR_Y 752.09F
#define DECK_MIDDLE_X 0.0F
#define DECK_MIDDLE_Z (-385.89F)

/* create Zero's task, and start its laser, arms, effects and fence with
 * it. erobo_load must have read both banks. when task_create fails, it
 * makes nothing. when motion_create fails, it writes through NULL */
void erobo_create(void);
/* copy into pos the end of arm 0 or 1, as Zero's last step posed it. the
 * arm's hand rests there at home. Zero's task must be alive */
void erobo_get_arm_end(int arm, NJS_POINT3 *pos);
/* move the sphere of hand 0 or 1 to pos, a point in the world. Zero's task
 * must be alive */
void erobo_set_hand_shape(int hand, NJS_POINT3 *pos);
/* Zero's heading. Zero faces along the negated cosine and sine of its
 * heading. Zero's task must be alive */
Angle erobo_get_heading(void);

#endif
