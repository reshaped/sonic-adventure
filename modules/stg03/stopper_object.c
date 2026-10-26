/* the cart stopper's object, which cart_stopper.c copies into the collision
 * world */
#include <ninja.h>

#include "stg03/stopper_object.h"

extern NJS_OBJECT dat_0c9f308c;
extern NJS_MODEL dat_0c9f3968;

NJS_OBJECT stopper_object = {
#include "stg03/stopper_object/stopper_object.inc"
};
