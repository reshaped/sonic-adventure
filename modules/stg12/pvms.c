/* the pvm lists for the stage and for each act */
#include "stg12/pvms.h"
#include "animal/textures.h"
#include "enemy/leon.h"
#include "enemy/robo.h"
#include "enemy/saru.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "stg01/fish_models.h"
#include "stg12/e105_textures.h"
#include "stg12/hotshelter0_textures.h"
#include "stg12/hotshelter1_textures.h"
#include "stg12/hotshelter2_textures.h"
#include "stg12/hotshelter3_textures.h"
#include "stg12/hotshelter4_textures.h"
#include "stg12/jellyfish.h"
#include "stg12/suimen_textures.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 17
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 4
#define TEXT_9_LENGTH 3
#define TEXT_10_LENGTH 4
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 15
#define TEXT_14_LENGTH 11
#define TEXT_15_LENGTH 14
#define TEXT_16_LENGTH 14
#define TEXT_17_LENGTH 5
#define TEXT_18_LENGTH 4
#define TEXT_19_LENGTH 6
#define TEXT_20_LENGTH 5
#define TEXT_21_LENGTH 4
#define TEXT_22_LENGTH 6
#define TEXT_23_LENGTH 6
#define TEXT_24_LENGTH 11
#define TEXT_25_LENGTH 6
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 4
#define TEXT_29_LENGTH 4
#include "stg12/pvms/text.inc"

/* the first four entries fill the stage's texlist slots. the entries after
 * them end at the first null texlist. an entry with no name loads its
 * texlist with njLoadTexture rather than from a pvm */
struct texture_pvm shelter_pvms[16] = {
    {TEXT_1, &hotshelter0_texlist},
    {TEXT_2, &e_robo_texlist},
    {TEXT_3, &e_leon_texlist},
    {TEXT_4, &e_saru_texlist},
    {TEXT_5, &e_bomb_texlist},
    {TEXT_6, &togeball_togeball_texlist},
    {TEXT_7, &shelter_spark_texlist},
    {TEXT_8, &goma_texlist},
    {TEXT_9, &pen_texlist},
    {TEXT_10, &rako_texlist},
    {TEXT_11, &banb_texlist},
    {TEXT_12, &suka_texlist},
    {TEXT_13, &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {NULL, &shelter_jellyfish_texlist},
    {NULL, NULL},
};
struct texture_pvm shelter_act0_pvms[10] = {
    {TEXT_14, &hotshelter1_texlist},    {TEXT_15, &shelter_column_texlist},
    {TEXT_16, &shelter_suimen_texlist}, {TEXT_17, &fish_sira_texlist},
    {TEXT_18, &fish_an_texlist},        {TEXT_19, &fish_an_ke_texlist},
    {TEXT_20, &fish_ryu_texlist},       {TEXT_21, &fish_same_texlist},
    {TEXT_22, &fish_hh_texlist},        {TEXT_23, &fish_utu_texlist},
};
struct texture_pvm shelter_act1_pvms[2] = {
    {TEXT_24, &hotshelter2_texlist},
    {TEXT_25, &robotv_texlist},
};
struct texture_pvm shelter_act2_pvms[4] = {
    {TEXT_26, &hotshelter3_texlist},
    {TEXT_27, &hotshelter4_texlist},
    {TEXT_28, &e105_texlist},
    {TEXT_29, &kuja_texlist},
};
struct texture_pvm shelter_act3_pvms[1] = {
    {TEXT_27, &hotshelter4_texlist},
};
