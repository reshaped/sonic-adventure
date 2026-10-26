/* the pink bird's action and its motion, then the picture's texture name.
 * the model the action moves and the motion's NJS_MDATA2 table lie in the
 * data before this object. ev00c7_picture_texlist, which names the
 * picture's texture, lies in the data after it */
#include <ninja.h>

#include "ev00c7/pink_bird_action.h"

#define TEXT_1_LENGTH 7
#include "ev00c7/pink_bird_action/text.inc"

extern NJS_OBJECT dat_0cbd2874;
extern NJS_MDATA2 dat_0cbd9c50[];

static NJS_MOTION ev00c7_pink_bird_motion = {
#include "ev00c7/pink_bird_action/ev00c7_pink_bird_motion.inc"
};
NJS_ACTION ev00c7_pink_bird_action = {
#include "ev00c7/pink_bird_action/ev00c7_pink_bird_action.inc"
};
static NJS_TEXNAME ev00c7_picture_textures[1] = {
    {TEXT_1, 0, 0},
};
