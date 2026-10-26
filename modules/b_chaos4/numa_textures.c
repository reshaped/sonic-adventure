/* a root node over the model dat_0c943440, the textures the swamp's calm
 * surface cycles through and their texlist, and a material */
#include <ninja.h>

#include "b_chaos4/numa_textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 10
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 10
#include "b_chaos4/numa_textures/text.inc"

extern NJS_MODEL dat_0c943440;

static NJS_OBJECT dat_0c943468[1] = {
#include "b_chaos4/numa_textures/dat_0c943468.inc"
};
NJS_TEXNAME chaos4_numa_textures[14] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
};
NJS_TEXLIST chaos4_numa_texlist = {
#include "b_chaos4/numa_textures/chaos4_numa_texlist.inc"
};
NJS_MATERIAL dat_0c94354c[1] = {
#include "b_chaos4/numa_textures/dat_0c94354c.inc"
};
