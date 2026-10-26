/* the pvms the stage loads */
#include "b_chaos7/pvms.h"
#include "b_chaos7/textures.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 15
#define TEXT_3_LENGTH 13
#include "b_chaos7/pvms/text.inc"

/* the first four take the stage's texlist slots. past them, the list ends at
 * its first null texlist */
#pragma section HEAD
struct texture_pvm chaos7_pvms[8] = {
    {TEXT_1, &obj_chaos7_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_2, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {TEXT_3, &eff_wt_column_texlist},
    {NULL, NULL},
};
#pragma section
