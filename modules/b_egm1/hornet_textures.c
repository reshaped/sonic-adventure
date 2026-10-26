/* the Egg Hornet's root node, its missile's model, and the texlists of both */
#include <ninja.h>

#include "b_egm1/hornet_textures.h"
#include <sg_xpt.h>

#include "b_egm1/texlists.h"

#define TEXT_1_LENGTH 7
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 7
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 9
#define TEXT_16_LENGTH 8
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 7
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 7
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 6
#define TEXT_24_LENGTH 6
#define TEXT_25_LENGTH 6
#define TEXT_26_LENGTH 9
#include "b_egm1/hornet_textures/text.inc"

extern NJS_OBJECT dat_0c90dc14;
extern NJS_MODEL dat_0c910860;

NJS_OBJECT egg_hornet_object = {
#include "b_egm1/hornet_textures/egg_hornet_object.inc"
};
static NJS_TEXNAME egg_hornet_textures[25] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0},
};
NJS_TEXLIST egg_hornet_texlist = {
#include "b_egm1/hornet_textures/egg_hornet_texlist.inc"
};
static NJS_MATERIAL dat_0c9109f0[5] = {
#include "b_egm1/hornet_textures/dat_0c9109f0.inc"
};
static Sint16 dat_0c910a54[24] = {
#include "b_egm1/hornet_textures/dat_0c910a54.inc"
};
static Sint16 dat_0c910a84[15] = {
#include "b_egm1/hornet_textures/dat_0c910a84.inc"
};
static Sint16 dat_0c910aa2[15] = {
#include "b_egm1/hornet_textures/dat_0c910aa2.inc"
};
static Sint16 dat_0c910ac0[15] = {
#include "b_egm1/hornet_textures/dat_0c910ac0.inc"
};
static Sint16 dat_0c910ade[7] = {
#include "b_egm1/hornet_textures/dat_0c910ade.inc"
};
static NJS_COLOR dat_0c910aec[20] = {
#include "b_egm1/hornet_textures/dat_0c910aec.inc"
};
static NJS_COLOR dat_0c910b3c[14] = {
#include "b_egm1/hornet_textures/dat_0c910b3c.inc"
};
static NJS_COLOR dat_0c910b74[14] = {
#include "b_egm1/hornet_textures/dat_0c910b74.inc"
};
static NJS_COLOR dat_0c910bac[14] = {
#include "b_egm1/hornet_textures/dat_0c910bac.inc"
};
static NJS_COLOR dat_0c910be4[5] = {
#include "b_egm1/hornet_textures/dat_0c910be4.inc"
};
static NJS_MESHSET dat_0c910bf8[5] = {
#include "b_egm1/hornet_textures/dat_0c910bf8.inc"
};
static NJS_POINT3 dat_0c910c70[31] = {
#include "b_egm1/hornet_textures/dat_0c910c70.inc"
};
static NJS_VECTOR dat_0c910de4[31] = {
#include "b_egm1/hornet_textures/dat_0c910de4.inc"
};
static NJS_MODEL dat_0c910f58 = {
#include "b_egm1/hornet_textures/dat_0c910f58.inc"
};
NJS_OBJECT egg_hornet_missile_object = {
#include "b_egm1/hornet_textures/egg_hornet_missile_object.inc"
};
static NJS_TEXNAME egm1mis_textures[4] = {
    {TEXT_26, 0, 0},
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_18, 0, 0},
};
NJS_TEXLIST egm1mis_texlist = {
#include "b_egm1/hornet_textures/egm1mis_texlist.inc"
};
NJS_MATERIAL dat_0c910fec[1] = {
#include "b_egm1/hornet_textures/dat_0c910fec.inc"
};
