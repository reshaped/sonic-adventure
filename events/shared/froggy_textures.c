/* the names of Froggy's 8 textures. each event that links them lists them in a
 * texlist of its own. all but EV00DF load it from FROG. frog_textures.c holds
 * the same names with the first two swapped */
#include <ninja.h>

#include "shared/froggy_textures.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 7
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 7
#include "shared/froggy_textures/text.inc"

NJS_TEXNAME event_froggy_textures[8] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0}, {TEXT_8, 0, 0},
};
