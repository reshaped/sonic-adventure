/* the texture lists the stage loads, each with the PVM that fills it */
#include "b_egm2/pvms.h"
#include "b_egm2/baku_textures.h"
#include "b_egm2/legs.h"
#include "b_egm2/ornament_textures.h"
#include "b_egm2/texlists.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 9
#include "b_egm2/pvms/text.inc"

/* texture_load_stage copies the first four entries' texlists into its
 * stage slots. all four are null here, so the stage fills no slot. the list
 * ends at the first null texlist after them. an entry with no name loads its
 * texlist with njLoadTexture, not from a PVM */
#pragma section HEAD
struct texture_pvm egm2_pvms[16] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_1, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {TEXT_2, &egm2_common_texlist},
    {TEXT_3, &egm2_sky_texlist},
    {TEXT_4, &egm2_missile_texlist},
    {TEXT_5, &egm2_fire_texlist},
    {TEXT_6, &egm2_baku_texlist},
    {TEXT_7, &egm2_effect_texlist},
    {TEXT_8, &egm2_car_texlist},
    {TEXT_9, &egm2_hamon_texlist},
    {TEXT_10, &ornament_texlist},
    {NULL, NULL},
};
#pragma section
