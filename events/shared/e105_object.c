/* event_e105_object, the root of the model e105.c draws while no motion is
 * queued */
#include "shared/e105.h"
#include <ninja.h>

extern NJS_OBJECT event_e105_1;

NJS_OBJECT event_e105_object = {
#include "shared/e105_object/event_e105_object.inc"
};
