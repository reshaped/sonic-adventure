/* the pvms the stage loads */
#include <ninja.h>

#include "animal/textures.h"
#include "enemy/leon.h"
#include "enemy/sai_textures.h"
#include "enemy/snake.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "player/race.h"
#include "stg02/e103.h"
#include "stg02/pvms.h"
#include "stg05/explosion_textures.h"
#include "system/texture.h"

extern NJS_TEXLIST obj_windy_texlist;
extern NJS_TEXLIST windy_emerald_texlist;

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 7
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 19
#define TEXT_7_LENGTH 15
#define TEXT_8_LENGTH 4
#define TEXT_9_LENGTH 3
#define TEXT_10_LENGTH 4
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 8
#include "stg02/pvms/text.inc"

/* the first four entries fill the stage's texlist slots. texture_load_stage
 * loads the entries after them up to the first null texlist, where it
 * stops. an entry with no name loads its texlist with njLoadTexture rather
 * than from a pvm */
struct texture_pvm windy_pvms[16] = {
    {TEXT_1, &obj_windy_texlist},
    {TEXT_2, &e_snake_texlist},
    {TEXT_3, &e_sai_texlist},
    {TEXT_4, &e_leon_texlist},
    {TEXT_5, &windy_e103_texlist},
    {TEXT_6, &mexplosion_texlist},
    {TEXT_7, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {TEXT_8, &tuba_texlist},
    {TEXT_9, &oum_texlist},
    {TEXT_10, &kuja_texlist},
    {TEXT_11, &goma_texlist},
    {TEXT_12, &mogu_texlist},
    {TEXT_13, &milesrace_texlist},
    {TEXT_14, &windy_emerald_texlist},
    {NULL, NULL},
};
