/* a texlist of one texture, mr_064s_emegrenb, and a model of four nodes. a
 * second such texlist follows. no word in EV009D or 1ST_READ.BIN points at
 * either texlist or at the model's root. the file ends on the material of
 * another node, whose other parts start the data after this file */
#include <ninja.h>
#include <sg_xpt.h>

#define TEXT_1_LENGTH 16
#include "ev009d/dat_0cb8c520/text.inc"

static NJS_TEXNAME dat_0cb8c520[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb8c52c = {
#include "ev009d/dat_0cb8c520/dat_0cb8c52c.inc"
};
static NJS_MATERIAL dat_0cb8c534[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8c534.inc"
};
static Sint16 dat_0cb8c548[126] = {
#include "ev009d/dat_0cb8c520/dat_0cb8c548.inc"
};
static NJS_COLOR dat_0cb8c644[126] = {
#include "ev009d/dat_0cb8c520/dat_0cb8c644.inc"
};
static NJS_MESHSET dat_0cb8c83c[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8c83c.inc"
};
static NJS_POINT3 dat_0cb8c854[43] = {
#include "ev009d/dat_0cb8c520/dat_0cb8c854.inc"
};
static NJS_VECTOR dat_0cb8ca58[43] = {
#include "ev009d/dat_0cb8c520/dat_0cb8ca58.inc"
};
static NJS_MODEL dat_0cb8cc5c = {
#include "ev009d/dat_0cb8c520/dat_0cb8cc5c.inc"
};
static NJS_OBJECT dat_0cb8cc84 = {
#include "ev009d/dat_0cb8c520/dat_0cb8cc84.inc"
};
static NJS_MATERIAL dat_0cb8ccb8[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8ccb8.inc"
};
static Sint16 dat_0cb8cccc[48] = {
#include "ev009d/dat_0cb8c520/dat_0cb8cccc.inc"
};
static NJS_COLOR dat_0cb8cd2c[48] = {
#include "ev009d/dat_0cb8c520/dat_0cb8cd2c.inc"
};
static NJS_MESHSET dat_0cb8cdec[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8cdec.inc"
};
static NJS_POINT3 dat_0cb8ce04[24] = {
#include "ev009d/dat_0cb8c520/dat_0cb8ce04.inc"
};
static NJS_VECTOR dat_0cb8cf24[24] = {
#include "ev009d/dat_0cb8c520/dat_0cb8cf24.inc"
};
static NJS_MODEL dat_0cb8d044 = {
#include "ev009d/dat_0cb8c520/dat_0cb8d044.inc"
};
static NJS_OBJECT dat_0cb8d06c = {
#include "ev009d/dat_0cb8c520/dat_0cb8d06c.inc"
};
static NJS_MATERIAL dat_0cb8d0a0[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d0a0.inc"
};
static Sint16 dat_0cb8d0b4[12] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d0b4.inc"
};
static Sint16 dat_0cb8d0cc[16] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d0cc.inc"
};
static NJS_COLOR dat_0cb8d0ec[12] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d0ec.inc"
};
static NJS_COLOR dat_0cb8d11c[16] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d11c.inc"
};
static NJS_MESHSET dat_0cb8d15c[2] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d15c.inc"
};
static NJS_POINT3 dat_0cb8d18c[17] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d18c.inc"
};
static NJS_VECTOR dat_0cb8d258[17] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d258.inc"
};
static NJS_MODEL dat_0cb8d324 = {
#include "ev009d/dat_0cb8c520/dat_0cb8d324.inc"
};
static NJS_OBJECT dat_0cb8d34c = {
#include "ev009d/dat_0cb8c520/dat_0cb8d34c.inc"
};
static NJS_MATERIAL dat_0cb8d380[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d380.inc"
};
static Sint16 dat_0cb8d394[4] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d394.inc"
};
static NJS_MESHSET dat_0cb8d39c[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d39c.inc"
};
static NJS_POINT3 dat_0cb8d3b4[3] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d3b4.inc"
};
static NJS_VECTOR dat_0cb8d3d8[3] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d3d8.inc"
};
static NJS_MODEL dat_0cb8d3fc = {
#include "ev009d/dat_0cb8c520/dat_0cb8d3fc.inc"
};
static NJS_OBJECT dat_0cb8d424 = {
#include "ev009d/dat_0cb8c520/dat_0cb8d424.inc"
};
static NJS_TEXNAME dat_0cb8d458[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cb8d464 = {
#include "ev009d/dat_0cb8c520/dat_0cb8d464.inc"
};
static NJS_MATERIAL dat_0cb8d46c[1] = {
#include "ev009d/dat_0cb8c520/dat_0cb8d46c.inc"
};
