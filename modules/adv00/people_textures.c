/* the texture lists of the field's people. 1ST_READ.BIN's station_square_pvms
 * names them for the pvms SS_PEOPLE, SS_EKIIN, SS_MIZUGI and SS_BURGER in
 * this order. their texture names are null: the pvms give the names */
#include <ninja.h>

#include "adv00/texlists.h"

static NJS_TEXNAME ss_people_textures[73] = {{NULL, 0, 0}};
NJS_TEXLIST ss_people_texlist = {
#include "adv00/people_textures/ss_people_texlist.inc"
};
static NJS_TEXNAME ss_ekiin_textures[8] = {{NULL, 0, 0}};
NJS_TEXLIST dat_0cb2c994 = {
#include "adv00/people_textures/dat_0cb2c994.inc"
};
static NJS_TEXNAME ss_mizugi_textures[7] = {{NULL, 0, 0}};
NJS_TEXLIST dat_0cb2c9f0 = {
#include "adv00/people_textures/dat_0cb2c9f0.inc"
};
static NJS_TEXNAME ss_burger_textures[17] = {{NULL, 0, 0}};
NJS_TEXLIST dat_0cb2cac4 = {
#include "adv00/people_textures/dat_0cb2cac4.inc"
};
