/* the objects' textures and texlist, and the sand gate's model */
#include <ninja.h>
#include <sg_xpt.h>

#include "sboard/object_textures.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 4
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 8
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 8
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 5
#define TEXT_12_LENGTH 5
#define TEXT_13_LENGTH 5
#define TEXT_14_LENGTH 6
#define TEXT_15_LENGTH 3
#define TEXT_16_LENGTH 3
#define TEXT_17_LENGTH 3
#define TEXT_18_LENGTH 3
#define TEXT_19_LENGTH 3
#define TEXT_20_LENGTH 3
#define TEXT_21_LENGTH 6
#define TEXT_22_LENGTH 7
#define TEXT_23_LENGTH 3
#define TEXT_24_LENGTH 3
#define TEXT_25_LENGTH 3
#define TEXT_26_LENGTH 3
#define TEXT_27_LENGTH 3
#define TEXT_28_LENGTH 3
#define TEXT_29_LENGTH 6
#define TEXT_30_LENGTH 4
#define TEXT_31_LENGTH 5
#define TEXT_32_LENGTH 6
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 5
#define TEXT_35_LENGTH 6
#define TEXT_36_LENGTH 7
#define TEXT_37_LENGTH 7
#define TEXT_38_LENGTH 9
#define TEXT_39_LENGTH 9
#define TEXT_40_LENGTH 9
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 5
#define TEXT_43_LENGTH 5
#define TEXT_44_LENGTH 5
#define TEXT_45_LENGTH 5
#define TEXT_46_LENGTH 5
#define TEXT_47_LENGTH 5
#include "sboard/object_textures/text.inc"

static NJS_TEXNAME obj_sandboard_textures[47] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0},
};
NJS_TEXLIST obj_sandboard_texlist = {
#include "sboard/object_textures/obj_sandboard_texlist.inc"
};
static NJS_MATERIAL dat_0c96c15c[4] = {
#include "sboard/object_textures/dat_0c96c15c.inc"
};
static Sint16 dat_0c96c1ac[40] = {
#include "sboard/object_textures/dat_0c96c1ac.inc"
};
static Sint16 dat_0c96c1fc[40] = {
#include "sboard/object_textures/dat_0c96c1fc.inc"
};
static Sint16 dat_0c96c24c[160] = {
#include "sboard/object_textures/dat_0c96c24c.inc"
};
static Sint16 dat_0c96c38c[20] = {
#include "sboard/object_textures/dat_0c96c38c.inc"
};
static NJS_COLOR dat_0c96c3b4[32] = {
#include "sboard/object_textures/dat_0c96c3b4.inc"
};
static NJS_COLOR dat_0c96c434[32] = {
#include "sboard/object_textures/dat_0c96c434.inc"
};
static NJS_COLOR dat_0c96c4b4[128] = {
#include "sboard/object_textures/dat_0c96c4b4.inc"
};
static NJS_COLOR dat_0c96c6b4[16] = {
#include "sboard/object_textures/dat_0c96c6b4.inc"
};
static NJS_MESHSET dat_0c96c6f4[4] = {
#include "sboard/object_textures/dat_0c96c6f4.inc"
};
static NJS_POINT3 dat_0c96c754[56] = {
#include "sboard/object_textures/dat_0c96c754.inc"
};
static NJS_VECTOR dat_0c96c9f4[56] = {
#include "sboard/object_textures/dat_0c96c9f4.inc"
};
static NJS_MODEL dat_0c96cc94 = {
#include "sboard/object_textures/dat_0c96cc94.inc"
};
NJS_OBJECT sand_gate_object = {
#include "sboard/object_textures/sand_gate_object.inc"
};
