/* the object the character preview draws in place of a character while the
 * last story is at the top of the wheel */
#include <ninja.h>

#include "advertise/last_story.h"

/* last_story_object's model */
extern NJS_MODEL dat_8c93d300;

NJS_OBJECT last_story_object = {
#include "advertise/last_story/last_story_object.inc"
};
