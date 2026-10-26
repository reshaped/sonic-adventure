/* the pvms the stage loads */
#include "b_e101_r/pvms.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#include "b_e101_r/pvms/text.inc"

/* the first four entries take the stage's four texlist slots. their
 * texlists are null, so the slots stay empty. texture_load_stage loads the
 * entries after them, up to the null texlist that ends the list */
#pragma section HEAD
struct texture_pvm e101_r_pvms[7] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_1, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {NULL, NULL},
};
#pragma section
