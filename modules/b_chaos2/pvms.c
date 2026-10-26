/* the pvms the stage loads */
#include "b_chaos2/pvms.h"
#include "b_chaos2/textures.h"
#include "chaos/models.h"
#include "effect/brain.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 16
#define TEXT_9_LENGTH 15
#define TEXT_10_LENGTH 9
#include "b_chaos2/pvms/text.inc"

/* the first four take the stage's texlist slots. the first of them,
 * chaos2_object_texlist, is the list texture_set_stage makes current */
#pragma section HEAD
struct texture_pvm chaos2_pvms[12] = {
    {TEXT_1, &chaos2_object_texlist},   {TEXT_2, &chaos_surface_texlist},
    {TEXT_3, &chaos_effect_texlist},    {TEXT_4, &chaos_emerald_texlist},
    {TEXT_5, &chaos2_texlist},          {TEXT_6, &chaos2_effect_texlist},
    {TEXT_7, &chaos2_barrier_texlist},  {TEXT_8, &chaos_brainframe_texlist},
    {TEXT_9, &chaos_lifegauge_texlist}, {NULL, &b32ascii_texlist},
    {TEXT_10, &chaos2_land_texlist},    {NULL, NULL},
};
#pragma section
