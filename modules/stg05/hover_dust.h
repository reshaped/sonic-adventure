/* the dust an E-100 series robot raises as it hovers. it is a copy of
 * E-102's e102_hover_dust and its params */
#ifndef STG05_HOVER_DUST_H
#define STG05_HOVER_DUST_H
#include <sg_xpt.h>

struct task_work;

/* raise one puff of dust if work is above ground by less than 30. ground is
 * the height of the floor below work. the puff starts 1.5 above the floor,
 * and across it 7 from a point, in a random direction. the point lies 10
 * from work along x and z, in the direction (-cos a, -sin a). a is work's y
 * angle. the puff is bigger the lower work is */
void e100_hover_dust(struct task_work *work, Float ground);

#endif
