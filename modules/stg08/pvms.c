/* the pvms the stage loads */
#include <ninja.h>

#include "animal/textures.h"
#include "enemy/sai_textures.h"
#include "enemy/snake.h"
#include "hud/board.h"
#include "player/e102_bonus.h"
#include "player/race.h"
#include "player/sonic_effect_models.h"
#include "stg01/fish_models.h"
#include "stg08/emerald_textures.h"
#include "stg08/pvms.h"
#include "stg08/snowman.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 11
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 4
#define TEXT_11_LENGTH 3
#define TEXT_12_LENGTH 4
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 4
#define TEXT_15_LENGTH 6
#define TEXT_16_LENGTH 4
#define TEXT_17_LENGTH 7
#define TEXT_18_LENGTH 4
#define TEXT_19_LENGTH 6
#define TEXT_20_LENGTH 5
#define TEXT_21_LENGTH 5
#include "stg08/pvms/text.inc"

extern NJS_TEXLIST obj_icecap_texlist;
extern NJS_TEXLIST obj_icecap2_texlist;

/* the first four entries fill the stage's texlist slots. texture_load_stage
 * loads the entries after them up to the first null texlist, where it
 * stops. an entry with no name loads its texlist with njLoadTexture rather
 * than from a pvm */
struct texture_pvm icecap_pvms[23] = {
    {TEXT_1, &obj_icecap_texlist},
    {TEXT_2, &obj_icecap2_texlist},
    {TEXT_3, &milesrace_texlist},
    {TEXT_4, &e_sai_texlist},
    {TEXT_5, &e_snake_texlist},
    {TEXT_6, &e_snowman_texlist},
    {TEXT_7, &icecap_emerald_texlist},
    {TEXT_8, &board_score_texlist},
    {TEXT_9, &e102time_texlist},
    {NULL, &sonic_ice_texlist},
    {TEXT_10, &goma_texlist},
    {TEXT_11, &pen_texlist},
    {TEXT_12, &rako_texlist},
    {TEXT_13, &gori_texlist},
    {TEXT_14, &mogu_texlist},
    {TEXT_15, &fish_bb_texlist},
    {TEXT_16, &fish_sake_texlist},
    {TEXT_17, &fish_pira_texlist},
    {TEXT_18, &fish_same_texlist},
    {TEXT_19, &fish_hh_texlist},
    {TEXT_20, &fish_ms_texlist},
    {TEXT_21, &fish_una_texlist},
    {NULL, NULL},
};
