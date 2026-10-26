/* the sky's models station_square.c draws, one for each time of day */
#include <ninja.h>

#include "adv00/sky_models.h"

extern NJS_OBJECT dat_0ca606a0;
extern NJS_OBJECT dat_0ca61880;
extern NJS_OBJECT dat_0ca62a60;

NJS_OBJECT *station_square_sky_models[3] = {&dat_0ca606a0, &dat_0ca61880,
                                            &dat_0ca62a60};
