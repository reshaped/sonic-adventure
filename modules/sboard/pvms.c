/* the pvms the stage loads */
#include "sboard/pvms.h"
#include "hud/board.h"
#include "player/big_kaeru_textures.h"
#include "player/e102_bonus.h"
#include "sboard/object_textures.h"
#include "sboard/sand_textures.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 8
#include "sboard/pvms/text.inc"

/* texture_load_stage puts the first four entries' texlists in the stage's
 * slots, null or not. the list ends at the first null texlist after them */
#pragma section HEAD
struct texture_pvm sunaboard_pvms[9] = {
    {TEXT_1, &obj_sandboard_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_2, &eff_sandboard_texlist},
    {TEXT_3, &big_kaeru_texlist},
    {TEXT_4, &board_score_texlist},
    {TEXT_5, &e102time_texlist},
    {NULL, NULL},
};
#pragma section
