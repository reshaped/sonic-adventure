/* the buyon's four collision shapes, which buyon_step gives collision_init */
#include "stg03/buyon_shapes.h"
#include "collision/collision.h"

struct collision_shape buyon_shapes[BUYON_SHAPES] = {
#include "stg03/buyon_shapes/buyon_shapes.inc"
};
