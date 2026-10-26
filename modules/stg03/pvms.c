/* the pvms the stage loads */
#include "stg03/pvms.h"
#include "animal/textures.h"
#include "enemy/amenbo.h"
#include "enemy/saru.h"
#include "stg01/fish_models.h"
#include "stg03/buyon_textures.h"
#include "stg03/effect_textures.h"
#include "stg03/land0_textures.h"
#include "stg03/land1_textures.h"
#include "stg03/land2_textures.h"
#include "stg03/object_textures.h"
#include "stg03/shareobj_textures.h"
#include "stg03/sky_textures.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 17
#define TEXT_8_LENGTH 3
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 4
#define TEXT_11_LENGTH 3
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 5
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 10
#include "stg03/pvms/text.inc"

/* the first four entries take the stage's texlist slots. the entries after
 * them end at the first null texlist */
struct texture_pvm twinkle_pvms[13] = {
    {TEXT_1, &obj_twinkle_texlist},
    {TEXT_2, &e_amenbo_texlist},
    {TEXT_3, &buyon_texlist},
    {TEXT_4, &e_saru_texlist},
    {TEXT_5, &bg_shareobj_texlist},
    {TEXT_6, &e_bomb_texlist},
    {TEXT_7, &togeball_togeball_texlist},
    {TEXT_8, &usa_texlist},
    {TEXT_9, &banb_texlist},
    {TEXT_10, &wara_texlist},
    {TEXT_11, &zou_texlist},
    {TEXT_12, &koar_texlist},
    {NULL, NULL},
};
/* each act's pvms in turn: two for act 0, five for act 1 and two for
 * act 2. 1ST_READ.BIN's twinkle_acts holds each act's count, so the list
 * has no null end */
struct texture_pvm twinkle_act_pvms[9] = {
    {TEXT_13, &twinkle_land0_texlist}, {TEXT_14, &obj_shareobj_texlist},
    {TEXT_15, &twinkle_land1_texlist}, {TEXT_16, &eff_twinkle_texlist},
    {TEXT_17, &fish_bb_texlist},       {TEXT_18, &fish_pira_texlist},
    {TEXT_19, &fish_ms_texlist},       {TEXT_20, &twinkle_land2_texlist},
    {TEXT_21, &eff_candle_texlist},
};
