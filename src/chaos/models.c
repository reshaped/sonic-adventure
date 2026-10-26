/* the models of Chaos's puddle and blob, and of a sphere that Chaos 0 fades
 * with them. a texture name table and a texlist come before each model,
 * and a fourth table and texlist end the file. no code reads the tables or
 * the texlists, and nothing names the last texlist. the first three tables
 * name one string, as SHC merges a file's identical strings. the notes
 * beside this file's rows in config/1st_read/splits.txt weigh where its
 * object file starts and ends */
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/puddle.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 10
#include "chaos/models/text.inc"

static NJS_TEXNAME dat_8c5a6c40[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_8c5a6c4c = {
#include "chaos/models/dat_8c5a6c4c.inc"
};
static NJS_MATERIAL dat_8c5a6c54[1] = {
#include "chaos/models/dat_8c5a6c54.inc"
};
static Sint16 dat_8c5a6c68[75] = {
#include "chaos/models/dat_8c5a6c68.inc"
};
static Sint16 dat_8c5a6cfe[1400] = {
#include "chaos/models/dat_8c5a6cfe.inc"
};
static NJS_COLOR dat_8c5a77f0[75] = {
#include "chaos/models/dat_8c5a77f0.inc"
};
static NJS_COLOR dat_8c5a791c[1400] = {
#include "chaos/models/dat_8c5a791c.inc"
};
static NJS_MESHSET dat_8c5a8efc[2] = {
#include "chaos/models/dat_8c5a8efc.inc"
};
static NJS_POINT3 dat_8c5a8f2c[376] = {
#include "chaos/models/dat_8c5a8f2c.inc"
};
static NJS_VECTOR dat_8c5aa0cc[376] = {
#include "chaos/models/dat_8c5aa0cc.inc"
};
static NJS_MODEL dat_8c5ab26c = {
#include "chaos/models/dat_8c5ab26c.inc"
};
NJS_OBJECT chaos_puddle = {
#include "chaos/models/chaos_puddle.inc"
};
static NJS_TEXNAME dat_8c5ab2c8[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_8c5ab2d4 = {
#include "chaos/models/dat_8c5ab2d4.inc"
};
static NJS_MATERIAL dat_8c5ab2dc[1] = {
#include "chaos/models/dat_8c5ab2dc.inc"
};
static Sint16 dat_8c5ab2f0[36] = {
#include "chaos/models/dat_8c5ab2f0.inc"
};
static Sint16 dat_8c5ab338[288] = {
#include "chaos/models/dat_8c5ab338.inc"
};
static NJS_COLOR dat_8c5ab578[36] = {
#include "chaos/models/dat_8c5ab578.inc"
};
static NJS_COLOR dat_8c5ab608[288] = {
#include "chaos/models/dat_8c5ab608.inc"
};
static NJS_MESHSET dat_8c5aba88[2] = {
#include "chaos/models/dat_8c5aba88.inc"
};
static NJS_POINT3 dat_8c5abab8[85] = {
#include "chaos/models/dat_8c5abab8.inc"
};
static NJS_VECTOR dat_8c5abeb4[85] = {
#include "chaos/models/dat_8c5abeb4.inc"
};
static NJS_MODEL dat_8c5ac2b0 = {
#include "chaos/models/dat_8c5ac2b0.inc"
};
NJS_OBJECT chaos_blob = {
#include "chaos/models/chaos_blob.inc"
};
static NJS_TEXNAME dat_8c5ac30c[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_8c5ac318 = {
#include "chaos/models/dat_8c5ac318.inc"
};
static NJS_MATERIAL dat_8c5ac320[1] = {
#include "chaos/models/dat_8c5ac320.inc"
};
static Sint16 dat_8c5ac334[90] = {
#include "chaos/models/dat_8c5ac334.inc"
};
static Sint16 dat_8c5ac3e8[780] = {
#include "chaos/models/dat_8c5ac3e8.inc"
};
static NJS_COLOR dat_8c5aca00[90] = {
#include "chaos/models/dat_8c5aca00.inc"
};
static NJS_COLOR dat_8c5acb68[780] = {
#include "chaos/models/dat_8c5acb68.inc"
};
static NJS_MESHSET dat_8c5ad798[2] = {
#include "chaos/models/dat_8c5ad798.inc"
};
static NJS_POINT3 dat_8c5ad7c8[212] = {
#include "chaos/models/dat_8c5ad7c8.inc"
};
static NJS_VECTOR dat_8c5ae1b8[212] = {
#include "chaos/models/dat_8c5ae1b8.inc"
};
static NJS_MODEL dat_8c5aeba8 = {
#include "chaos/models/dat_8c5aeba8.inc"
};
NJS_OBJECT chaos_sphere = {
#include "chaos/models/chaos_sphere.inc"
};
static NJS_TEXNAME dat_8c5aec04[1] = {
    {TEXT_2, 0, 0},
};
static NJS_TEXLIST dat_8c5aec10 = {
#include "chaos/models/dat_8c5aec10.inc"
};
