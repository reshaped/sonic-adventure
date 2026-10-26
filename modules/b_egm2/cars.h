/* the street's parked cars. each blows up and breaks apart once the Egg
 * Walker or its attacks hit it */
#ifndef B_EGM2_CARS_H
#define B_EGM2_CARS_H

/* park the street's three cars, each a task at level 3. a car whose task
 * can't be made is left out. the boss's EGG_WALKER_INIT step calls it */
void egm2_cars_create(void);

#endif
