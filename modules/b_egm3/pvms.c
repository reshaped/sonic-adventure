/* the pvms the stage loads */
#include "b_egm3/pvms.h"
#include "b_egm3/effects.h"
#include "b_egm3/egg_viper.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 7
#include "b_egm3/pvms/text.inc"

/* texture_load_stage puts the first four entries' texlists in the stage's
 * slots. the fifth's null texlist ends the list. an entry with no name loads
 * its texlist with njLoadTexture, not from a pvm */
#pragma section HEAD
struct texture_pvm eggmobile3_pvms[5] = {
    {TEXT_1, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {TEXT_2, &egm3mdl_texlist},
    {TEXT_3, &egg_viper_effect_texlist},
    {NULL, NULL},
};
#pragma section
