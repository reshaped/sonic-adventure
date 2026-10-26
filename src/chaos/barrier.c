/* the models of Chaos 2's barrier and its child, then the texture names and
 * texlists of the Chaos bosses' effects, the emeralds and Chaos's surface.
 * the notes beside this file's rows in config/1st_read/splits.txt weigh
 * whether one object file holds them all */
#include <ninja.h>
#include <sg_xpt.h>

#include "chaos/models.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 12
#define TEXT_3_LENGTH 12
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 12
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 12
#define TEXT_16_LENGTH 12
#define TEXT_17_LENGTH 12
#define TEXT_18_LENGTH 12
#define TEXT_19_LENGTH 12
#define TEXT_20_LENGTH 12
#define TEXT_21_LENGTH 12
#define TEXT_22_LENGTH 12
#define TEXT_23_LENGTH 12
#define TEXT_24_LENGTH 12
#define TEXT_25_LENGTH 12
#define TEXT_26_LENGTH 5
#define TEXT_27_LENGTH 6
#define TEXT_28_LENGTH 5
#define TEXT_29_LENGTH 4
#define TEXT_30_LENGTH 12
#define TEXT_31_LENGTH 12
#define TEXT_32_LENGTH 12
#define TEXT_33_LENGTH 12
#define TEXT_34_LENGTH 12
#define TEXT_35_LENGTH 12
#define TEXT_36_LENGTH 12
#define TEXT_37_LENGTH 12
#define TEXT_38_LENGTH 12
#define TEXT_39_LENGTH 12
#define TEXT_40_LENGTH 12
#define TEXT_41_LENGTH 12
#define TEXT_42_LENGTH 13
#define TEXT_43_LENGTH 13
#define TEXT_44_LENGTH 13
#define TEXT_45_LENGTH 12
#define TEXT_46_LENGTH 12
#define TEXT_47_LENGTH 12
#define TEXT_48_LENGTH 12
#define TEXT_49_LENGTH 12
#define TEXT_50_LENGTH 12
#define TEXT_51_LENGTH 10
#define TEXT_52_LENGTH 10
#define TEXT_53_LENGTH 10
#define TEXT_54_LENGTH 10
#define TEXT_55_LENGTH 10
#define TEXT_56_LENGTH 10
#include "chaos/barrier/text.inc"

static NJS_MATERIAL dat_8c5aec18[1] = {
#include "chaos/barrier/dat_8c5aec18.inc"
};
static Sint16 dat_8c5aec2c[17] = {
#include "chaos/barrier/dat_8c5aec2c.inc"
};
static Sint16 dat_8c5aec4e[32] = {
#include "chaos/barrier/dat_8c5aec4e.inc"
};
static NJS_COLOR dat_8c5aec90[14] = {
#include "chaos/barrier/dat_8c5aec90.inc"
};
static NJS_COLOR dat_8c5aecc8[32] = {
#include "chaos/barrier/dat_8c5aecc8.inc"
};
static NJS_MESHSET dat_8c5aed48[2] = {
#include "chaos/barrier/dat_8c5aed48.inc"
};
static NJS_POINT3 dat_8c5aed78[17] = {
#include "chaos/barrier/dat_8c5aed78.inc"
};
static NJS_VECTOR dat_8c5aee44[17] = {
#include "chaos/barrier/dat_8c5aee44.inc"
};
static NJS_MODEL dat_8c5aef10 = {
#include "chaos/barrier/dat_8c5aef10.inc"
};
static NJS_OBJECT dat_8c5aef38 = {
#include "chaos/barrier/dat_8c5aef38.inc"
};
static NJS_MATERIAL dat_8c5aef6c[1] = {
#include "chaos/barrier/dat_8c5aef6c.inc"
};
static Sint16 dat_8c5aef80[17] = {
#include "chaos/barrier/dat_8c5aef80.inc"
};
static Sint16 dat_8c5aefa2[96] = {
#include "chaos/barrier/dat_8c5aefa2.inc"
};
static NJS_COLOR dat_8c5af064[14] = {
#include "chaos/barrier/dat_8c5af064.inc"
};
static NJS_COLOR dat_8c5af09c[96] = {
#include "chaos/barrier/dat_8c5af09c.inc"
};
static NJS_MESHSET dat_8c5af21c[2] = {
#include "chaos/barrier/dat_8c5af21c.inc"
};
static NJS_POINT3 dat_8c5af24c[33] = {
#include "chaos/barrier/dat_8c5af24c.inc"
};
static NJS_VECTOR dat_8c5af3d8[33] = {
#include "chaos/barrier/dat_8c5af3d8.inc"
};
static NJS_MODEL dat_8c5af564 = {
#include "chaos/barrier/dat_8c5af564.inc"
};
NJS_OBJECT chaos_barrier_object = {
#include "chaos/barrier/chaos_barrier_object.inc"
};
static NJS_TEXNAME chaos_effect_textures[29] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0},
};
NJS_TEXLIST chaos_effect_texlist = {
#include "chaos/barrier/chaos_effect_texlist.inc"
};
static NJS_TEXNAME chaos_emerald_textures[21] = {
    {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0},
    {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0}, {TEXT_37, 0, 0},
    {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0}, {TEXT_41, 0, 0},
    {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0}, {TEXT_45, 0, 0},
    {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0}, {TEXT_49, 0, 0},
    {TEXT_50, 0, 0},
};
NJS_TEXLIST chaos_emerald_texlist = {
#include "chaos/barrier/chaos_emerald_texlist.inc"
};
NJS_TEXNAME chaos_surface_textures[6] = {
    {TEXT_51, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0},
    {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
};
NJS_TEXLIST chaos_surface_texlist = {
#include "chaos/barrier/chaos_surface_texlist.inc"
};
