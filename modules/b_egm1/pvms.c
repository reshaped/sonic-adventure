/* the texture lists the stage loads, each with the PVM that fills it */
#include "b_egm1/pvms.h"
#include "b_egm1/barrier.h"
#include "b_egm1/effect_textures.h"
#include "b_egm1/jets.h"
#include "b_egm1/texlists.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 4
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 8
#include "b_egm1/pvms/text.inc"

/* texture_load_stage keeps the first four lists in the stage's four slots,
 * and loads the rest after them */
#pragma section HEAD
struct texture_pvm eggmobile1_pvms[11] = {
    {TEXT_1, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {TEXT_2, &egg_hornet_texlist},
    {TEXT_3, &egm1mis_texlist},
    {TEXT_4, &egm1eggman_texlist},
    {TEXT_5, &egm1tsuchi_texlist},
    {TEXT_6, &eggmobile1_sky_texlist},
    {TEXT_7, &egm1barrier_texlist},
    {TEXT_8, &egm1jet_texlist},
    {TEXT_9, &egm1jetb_texlist},
    {NULL, NULL},
};
#pragma section
