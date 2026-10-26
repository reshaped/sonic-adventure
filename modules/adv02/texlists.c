/* the Mystic Ruins' texture lists, and the pvms the stage loads */
#include <ninja.h>

#include "adv02/egg.h"
#include "adv02/texlists.h"
#include "enemy/saru.h"
#include "object/level_item.h"
#include "progress/emblem.h"
#include "system/texture.h"

extern NJS_TEXLIST dat_0cb396d4;

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#include "adv02/texlists/text.inc"

/* the module's head holds the texture lists, their textures and the pvms.
 * 1ST_READ.BIN's mystic_ruins_act_pvms and stage_pvms name ten of the lists
 * and the pvms by address. a textures array stays in the head only for its
 * initializer: an array without one would go to bss, and what follows it
 * would move */
#pragma section HEAD
static NJS_TEXNAME adv_mr00_textures[155] = {{NULL, 0, 0}};
NJS_TEXLIST adv_mr00_texlist = {
#include "adv02/texlists/adv_mr00_texlist.inc"
};

static NJS_TEXNAME adv_mr01_textures[90] = {{NULL, 0, 0}};
NJS_TEXLIST adv_mr01_texlist = {
#include "adv02/texlists/adv_mr01_texlist.inc"
};

static NJS_TEXNAME adv_mr02_textures[126] = {{NULL, 0, 0}};
NJS_TEXLIST adv_mr02_texlist = {
#include "adv02/texlists/adv_mr02_texlist.inc"
};

static NJS_TEXNAME adv_mr03_textures[104] = {{NULL, 0, 0}};
NJS_TEXLIST adv_mr03_texlist = {
#include "adv02/texlists/adv_mr03_texlist.inc"
};

static NJS_TEXNAME mrobj_textures[207] = {{NULL, 0, 0}};
NJS_TEXLIST mrobj_texlist = {
#include "adv02/texlists/mrobj_texlist.inc"
};

static NJS_TEXNAME mr_finalegg_textures[25] = {{NULL, 0, 0}};
NJS_TEXLIST mr_finalegg_texlist = {
#include "adv02/texlists/mr_finalegg_texlist.inc"
};

static NJS_TEXNAME mr_train_textures[31] = {{NULL, 0, 0}};
NJS_TEXLIST mr_train_texlist = {
#include "adv02/texlists/mr_train_texlist.inc"
};

static NJS_TEXNAME mr_pyramid_textures[21] = {{NULL, 0, 0}};
NJS_TEXLIST mr_pyramid_texlist = {
#include "adv02/texlists/mr_pyramid_texlist.inc"
};

static NJS_TEXNAME tornado2_textures[28] = {{NULL, 0, 0}};
NJS_TEXLIST tornado2_texlist = {
#include "adv02/texlists/tornado2_texlist.inc"
};

static NJS_TEXNAME mr_sky00_textures[9] = {{NULL, 0, 0}};
NJS_TEXLIST mr_sky00_texlist = {
#include "adv02/texlists/mr_sky00_texlist.inc"
};

static NJS_TEXNAME mr_sky01_textures[9] = {{NULL, 0, 0}};
NJS_TEXLIST mr_sky01_texlist = {
#include "adv02/texlists/mr_sky01_texlist.inc"
};

static NJS_TEXNAME mr_sky02_textures[6] = {{NULL, 0, 0}};
NJS_TEXLIST mr_sky02_texlist = {
#include "adv02/texlists/mr_sky02_texlist.inc"
};

/* texture_load_stage loads the first four rows into the stage's slots, and the
 * rows after them up to the null texlist. rows 2 and 3 leave their slots
 * empty */
struct texture_pvm mystic_ruins_pvms[9] = {
    {TEXT_1, &mrobj_texlist},
    {TEXT_2, &e_saru_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_3, &ancient_light_texlist},
    {TEXT_4, &dat_0cb396d4},
    {TEXT_5, &emblem_texlist},
    {TEXT_6, &egg_texlist},
    {NULL, NULL},
};
#pragma section
