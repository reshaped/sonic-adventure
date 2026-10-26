/* O LETSGO spheres: entering one runs player 0 to the sphere with the next
 * ID */
#ifndef STG09_GUIDE_SPHERE_H
#define STG09_GUIDE_SPHERE_H

#include "system/task.h"

/* the object list's step. scl.x is the sphere's radius, and scl.y its ID, read
 * as a Sint8. a sphere whose scl.z has an odd whole part starts no run. a run
 * can still go on through it.
 *
 * player 0, entering a sphere with no run under way, runs toward the sphere
 * with the next ID. if no sphere has that ID, no run starts. entering another
 * sphere during a run removes for good the sphere the run left from. the run
 * then leaves from the sphere entered, toward the sphere with the ID after that
 * one's. with no such sphere, the run ends, and the sphere entered is removed
 * too. a run also ends when the sphere it left from is destroyed any other way.
 * as the run leaves each sphere, it holds player 0 beyond its control for 60
 * steps */
void guide_sphere_step(struct task *task);

#endif
