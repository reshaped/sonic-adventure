/* the texture lists the stage loads, each with the PVM that fills it */
#include <ninja.h>

#include "animal/textures.h"
#include "enemy/saru.h"
#include "enemy/spina.h"
#include "enemy/uni.h"
#include "player/race.h"
#include "stg09/pvms.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 17
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 4
#define TEXT_7_LENGTH 4
#define TEXT_8_LENGTH 3
#define TEXT_9_LENGTH 3
#define TEXT_10_LENGTH 4
#include "stg09/pvms/text.inc"

extern NJS_TEXLIST casino_emerald_texlist;

/* texture_load_stage puts the first four in the stage's slots, null or not.
 * the list ends at the first null texlist after them */
struct texture_pvm casino_pvms[11] = {
    {TEXT_1, &uni_a_unibody_texlist},
    {TEXT_2, &supi_supi_texlist},
    {TEXT_3, &togeball_togeball_texlist},
    {TEXT_4, &milesrace_texlist},
    {TEXT_5, &casino_emerald_texlist},
    {TEXT_6, &kuja_texlist},
    {TEXT_7, &tuba_texlist},
    {TEXT_8, &oum_texlist},
    {TEXT_9, &usa_texlist},
    {TEXT_10, &koar_texlist},
    {NULL, NULL},
};
