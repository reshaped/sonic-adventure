/* the pvms the stage loads */
#include "b_robo/pvms.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#include "b_robo/pvms/text.inc"

/* texture_load_stage puts the first four entries' texlists in the stage's
 * slots. all four are null, so the slots stay empty. the lists after them
 * end at the first null texlist. an entry with no name loads its texlist
 * with njLoadTexture, not from a pvm */
#pragma section HEAD
struct texture_pvm eggmanrobo_pvms[7] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_1, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {NULL, NULL},
};
#pragma section
