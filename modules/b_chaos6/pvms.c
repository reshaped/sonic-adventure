/* the texture lists the stage loads, each with the PVM that fills it */
#include "b_chaos6/pvms.h"
#include "b_chaos6/eggman_textures.h"
#include "b_chaos6/sky.h"
#include "b_chaos6/textures.h"
#include "chaos/models.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "player/sonic_effect_models.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 15
#include "b_chaos6/pvms/text.inc"

/* texture_load_stage puts the first four entries' texlists in the stage's
 * slots. the list ends at the null texlist. an entry with no name loads its
 * textures by their own names */
#pragma section HEAD
struct texture_pvm chaos6_pvms[12] = {
    {TEXT_1, &chaos6_object_texlist},   {TEXT_2, &chaos_surface_texlist},
    {TEXT_3, &chaos_effect_texlist},    {TEXT_4, &chaos_emerald_texlist},
    {TEXT_5, &chaos6_texlist},          {TEXT_6, &chaos6_effect_texlist},
    {TEXT_7, &chaos6_eggman_texlist},   {TEXT_8, &chaos6_bg_texlist},
    {TEXT_9, &chaos_lifegauge_texlist}, {NULL, &b32ascii_texlist},
    {NULL, &sonic_ice_texlist},         {NULL, NULL},
};
#pragma section
