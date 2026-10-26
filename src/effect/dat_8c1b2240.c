/* four textures, a texlist of them and a texanim for each. they lie
 * between bomb.c's data and flame.c's, in the order an effect's source
 * keeps its own, and no code reads them */
#include <ninja.h>

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 5
#include "effect/dat_8c1b2240/text.inc"

static NJS_TEXNAME dat_8c1b2240[4] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
};
static NJS_TEXLIST dat_8c1b2270 = {
#include "effect/dat_8c1b2240/dat_8c1b2270.inc"
};
static NJS_TEXANIM dat_8c1b2278[4] = {
#include "effect/dat_8c1b2240/dat_8c1b2278.inc"
};
