/* the buyon's four collision shapes */
#ifndef STG03_BUYON_SHAPES_H
#define STG03_BUYON_SHAPES_H

#include "collision/collision.h"

/* the shapes buyon_step gives collision_init. it turns shapes 0, 2 and 3
 * off. shapes 2 and 3 are on again while the buyon grows and chases. buyon.c
 * moves shapes 1 to 3 with parts 0, 3 and 2 */
#define BUYON_SHAPES 4

extern struct collision_shape buyon_shapes[BUYON_SHAPES];

#endif
