/* the PVMs texture_load_stage loads for the past */
#include <ninja.h>

#include "adv03/effect_textures.h"
#include "adv03/past.h"
#include "adv03/pvms.h"
#include "system/texture.h"

extern NJS_TEXLIST past_kn_fam_texlist;
extern NJS_TEXLIST k_patya_texlist;
extern NJS_TEXLIST ev_alife_texlist;

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 8
#include "adv03/pvms/text.inc"

/* rows 1 to 3 leave three of the stage's four slots empty. the row with a
 * null texlist ends the lists that load after the slots */
#pragma section HEAD
struct texture_pvm past_pvms[9] = {
    {TEXT_1, &obj_past_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_2, &eff_past_texlist},
    {TEXT_3, &past_kn_fam_texlist},
    {TEXT_4, &k_patya_texlist},
    {TEXT_5, &ev_alife_texlist},
    {NULL, NULL},
};
#pragma section
