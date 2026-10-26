/* the texture lists the stage loads, each with the PVM that fills it */
#include <ninja.h>

#include "animal/textures.h"
#include "enemy/amenbo.h"
#include "enemy/sai_textures.h"
#include "enemy/saru.h"
#include "player/big_kaeru_textures.h"
#include "stg01/fish_models.h"
#include "stg01/obj_beach_textures.h"
#include "stg01/pvms.h"
#include "stg01/sea_surface.h"
#include "stg01/spray.h"
#include "system/texture.h"

extern NJS_TEXLIST beach_water_column_texlist;
extern NJS_TEXLIST beach_sky_texlist;

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 5
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 17
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 4
#define TEXT_10_LENGTH 3
#define TEXT_11_LENGTH 4
#define TEXT_12_LENGTH 3
#define TEXT_13_LENGTH 4
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 3
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 4
#define TEXT_19_LENGTH 6
#define TEXT_20_LENGTH 5
#define TEXT_21_LENGTH 6
#define TEXT_22_LENGTH 9
#include "stg01/pvms/text.inc"

/* texture_load_stage puts the first four entries' texlists in the stage's
 * slots, null or not. the list ends at the first null texlist after them. an
 * entry with no name loads its texlist with njLoadTexture, not from a PVM */
struct texture_pvm beach_pvms[25] = {
    {TEXT_1, &obj_beach_texlist},
    {TEXT_2, &e_saru_texlist},
    {TEXT_3, &e_sai_texlist},
    {TEXT_4, &e_amenbo_texlist},
    {NULL, &beach_water_column_texlist},
    {NULL, &beach_spray_texlist},
    {TEXT_5, &e_bomb_texlist},
    {TEXT_6, &togeball_togeball_texlist},
    {TEXT_7, &beach_sea_surface_texlist},
    {TEXT_8, &beach_sky_texlist},
    {TEXT_9, &goma_texlist},
    {TEXT_10, &pen_texlist},
    {TEXT_11, &rako_texlist},
    {TEXT_12, &usa_texlist},
    {TEXT_13, &koar_texlist},
    {TEXT_14, &fish_sb_texlist},
    {TEXT_15, &fish_tai_texlist},
    {TEXT_16, &fish_id_texlist},
    {TEXT_17, &fish_kaji_texlist},
    {TEXT_18, &fish_same_texlist},
    {TEXT_19, &fish_hh_texlist},
    {TEXT_20, &fish_ms_texlist},
    {TEXT_21, &fish_utu_texlist},
    {TEXT_22, &big_kaeru_texlist},
    {NULL, NULL},
};
