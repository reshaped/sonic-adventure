/* event_clouds_texlist, which shared/clouds.h declares, and its three
 * texture names */
#include <ninja.h>

#include "shared/clouds.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#include "ev0030/cloud_textures/text.inc"

static NJS_TEXNAME ev0030_cloud_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST event_clouds_texlist = {
#include "ev0030/cloud_textures/event_clouds_texlist.inc"
};
