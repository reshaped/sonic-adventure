/* the pictures' texture names and list, and the clouds' names and the list
 * that clouds.c's sprite draws from. the event loads only the pictures',
 * from ICM00C3 */
#include <ninja.h>

#include "ev00c3/textures.h"
#include "shared/clouds.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#include "ev00c3/textures/text.inc"

static NJS_TEXNAME ev00c3_picture_textures[5] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
};

NJS_TEXLIST ev00c3_picture_texlist = {
#include "ev00c3/textures/ev00c3_picture_texlist.inc"
};

static NJS_TEXNAME ev00c3_cloud_textures[3] = {
    {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},
    {TEXT_8, 0, 0},
};

NJS_TEXLIST event_clouds_texlist = {
#include "ev00c3/textures/event_clouds_texlist.inc"
};
