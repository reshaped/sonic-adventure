/* a texture name, its texlist and the materials of ev00f5_sonic_fingers's
 * model. nothing reads the texlist */
#include <ninja.h>

#define TEXT_1_LENGTH 10
#include "ev00f5/dat_0cb88818/text.inc"

static NJS_TEXNAME dat_0cb88818[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb88824 = {
#include "ev00f5/dat_0cb88818/dat_0cb88824.inc"
};
static NJS_MATERIAL dat_0cb8882c[3] = {
#include "ev00f5/dat_0cb88818/dat_0cb8882c.inc"
};
