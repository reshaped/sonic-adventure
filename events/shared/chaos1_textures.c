/* the names of Chaos 1's six textures. each event that links them lists
 * them in event_chaos1_texlist, which it loads from EV_CHAOS1 */
#include <ninja.h>

#include "shared/chaos1_textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 10
#include "shared/chaos1_textures/text.inc"

NJS_TEXNAME event_chaos1_textures[6] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
};
