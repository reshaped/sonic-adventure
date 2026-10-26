/* E-101's texlist of 35 textures and his jet's of 6, five of them E-101's
 * too. the material is one that each event's generated data points at */
#include <ninja.h>

#include "shared/e101_textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 10
#define TEXT_7_LENGTH 10
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 11
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 10
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 8
#define TEXT_20_LENGTH 8
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 12
#define TEXT_25_LENGTH 14
#define TEXT_26_LENGTH 12
#define TEXT_27_LENGTH 11
#define TEXT_28_LENGTH 13
#define TEXT_29_LENGTH 8
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 7
#define TEXT_32_LENGTH 7
#define TEXT_33_LENGTH 7
#define TEXT_34_LENGTH 7
#define TEXT_35_LENGTH 10
#define TEXT_36_LENGTH 9
#include "shared/e101_textures/text.inc"

static NJS_TEXNAME event_e101_textures[35] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},
};
NJS_TEXLIST event_e101_texlist = {
#include "shared/e101_textures/event_e101_texlist.inc"
};
static NJS_TEXNAME event_e101_jet_textures[6] = {
    {TEXT_15, 0, 0}, {TEXT_19, 0, 0}, {TEXT_31, 0, 0},
    {TEXT_32, 0, 0}, {TEXT_33, 0, 0}, {TEXT_36, 0, 0},
};
NJS_TEXLIST event_e101_jet_texlist = {
#include "shared/e101_textures/event_e101_jet_texlist.inc"
};
static NJS_MATERIAL event_e101_material[1] = {
#include "shared/e101_textures/event_e101_material.inc"
};
