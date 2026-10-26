/* the pvms the stage loads */
#include "minicart/pvms.h"
#include "enemy/saru.h"
#include "enemy/spina.h"
#include "enemy/uni.h"
#include "minicart/object_textures.h"
#include "minicart/shareobj_textures.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 17
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 13
#include "minicart/pvms/text.inc"

/* texture_load_stage loads the first four into the stage's slots, and
 * the three nulls leave theirs empty. the list ends at the first null
 * texlist after them */
#pragma section HEAD
struct texture_pvm cart_pvms[12] = {
    {TEXT_1, &obj_mini_cart_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_2, &obj_shareobj_texlist},
    {TEXT_3, &e_saru_texlist},
    {TEXT_4, &e_bomb_texlist},
    {TEXT_5, &togeball_togeball_texlist},
    {TEXT_6, &supi_supi_texlist},
    {TEXT_7, &uni_a_unibody_texlist},
    {TEXT_8, &uni_c_unibody_texlist},
    {NULL, NULL},
};
#pragma section
