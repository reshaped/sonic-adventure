/* the textures and texlists of the CHAOS4_COMMON, CHAOS4_KAMA (sickle) and
 * CHAOS4_OBJECT pvms, and the first strips of a model after them */
#include <ninja.h>

#include "b_chaos4/textures.h"
#include <sg_xpt.h>

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 15
#define TEXT_12_LENGTH 14
#define TEXT_13_LENGTH 18
#define TEXT_14_LENGTH 18
#define TEXT_15_LENGTH 18
#define TEXT_16_LENGTH 14
#define TEXT_17_LENGTH 14
#include "b_chaos4/textures/text.inc"

/* @bug memory: Chaos 4's puddle takes texture 15 of chaos4_common_texlist
 * (src/chaos/chaos.c's chaos_form_textures), past these nine.
 * chaos_animate_surface writes entry 15's texaddr every step, onto
 * chaos4_object_textures[3]'s attr (0x0c908864), which nothing reads once the
 * textures load, and the puddle's draw reads it back */
#ifdef GCC
/* repairs the write the tag names: GCC's array runs to entry 15, past the
 * nine chaos4_common_texlist loads */
static NJS_TEXNAME chaos4_common_textures[16] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
#else
static NJS_TEXNAME chaos4_common_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
#endif
NJS_TEXLIST chaos4_common_texlist = {
#include "b_chaos4/textures/chaos4_common_texlist.inc"
};
static NJS_TEXNAME chaos4_kama_textures[2] = {
    {TEXT_10, 0, 0},
    {TEXT_11, 0, 0},
};
NJS_TEXLIST chaos4_kama_texlist = {
#include "b_chaos4/textures/chaos4_kama_texlist.inc"
};
static NJS_TEXNAME chaos4_object_textures[6] = {
    {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
    {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0},
};
NJS_TEXLIST chaos4_object_texlist = {
#include "b_chaos4/textures/chaos4_object_texlist.inc"
};
Sint16 dat_0c90888c[16] = {
#include "b_chaos4/textures/dat_0c90888c.inc"
};
Sint16 dat_0c9088ac[16] = {
#include "b_chaos4/textures/dat_0c9088ac.inc"
};
Sint16 dat_0c9088cc[12] = {
#include "b_chaos4/textures/dat_0c9088cc.inc"
};
Sint16 dat_0c9088e4[12] = {
#include "b_chaos4/textures/dat_0c9088e4.inc"
};
Sint16 dat_0c9088fc[12] = {
#include "b_chaos4/textures/dat_0c9088fc.inc"
};
Sint16 dat_0c908914[12] = {
#include "b_chaos4/textures/dat_0c908914.inc"
};
Sint16 dat_0c90892c[10] = {
#include "b_chaos4/textures/dat_0c90892c.inc"
};
