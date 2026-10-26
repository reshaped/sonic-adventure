/* the PVMs texture_load_stage loads for Chaos 4's stage */
#include "b_chaos4/pvms.h"
#include "b_chaos4/column.h"
#include "b_chaos4/numa_textures.h"
#include "b_chaos4/textures.h"
#include "b_chaos4/util.h"
#include "chaos/models.h"
#include "effect/brain.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 16
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 14
#define TEXT_12_LENGTH 11
#include "b_chaos4/pvms/text.inc"

/* the first four rows leave the stage's four slots empty. after them, the
 * first row with a null texlist ends the list */
#pragma section HEAD
struct texture_pvm chaos4_pvms[18] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_1, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {TEXT_2, &chaos_surface_texlist},
    {TEXT_3, &chaos_effect_texlist},
    {TEXT_4, &chaos_emerald_texlist},
    {TEXT_5, &chaos_brainframe_texlist},
    {TEXT_6, &chaos4_common_texlist},
    {TEXT_7, &chaos4_kama_texlist},
    {TEXT_8, &chaos4_object_texlist},
    {TEXT_9, &column_texlist},
    {TEXT_10, &wake_texlist},
    {TEXT_11, &chaos4_shibuki_texlist},
    {TEXT_12, &chaos4_numa_texlist},
    {NULL, NULL},
};
#pragma section
