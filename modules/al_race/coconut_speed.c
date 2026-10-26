/* the velocity a coconut falls from its palm at. for each coconut,
 * palm_coconut_start sets x at random from -0.5 up to -0.25 and copies the
 * whole vector, so y and z stay 0 */
#include <ninja.h>

#include "al_race/coconut_speed.h"

NJS_VECTOR coconut_speed = {0.0F, 0.0F, 0.0F};
