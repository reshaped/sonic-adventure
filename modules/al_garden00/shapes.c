/* five boxes that no code reads: a floor, two side walls, a back wall and a
 * ceiling. in the elevator's frame they would bound the cell behind its
 * doors, round the point elevator.c's ELEVATOR_SHUT_TEST tests */
#include "collision/collision.h"

static struct collision_shape dat_0cb93220[5] = {
#include "al_garden00/shapes/dat_0cb93220.inc"
};
