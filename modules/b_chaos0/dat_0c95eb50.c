/* 18 texture names, c2tx_ files, in six sets of three. each set holds a
 * water frame of its own, then the eye and the brain, which every set
 * repeats. dat_0c95ec28 lists all 18, and each of dat_0c95ec30's six
 * texlists lists one set. no word of the module or of
 * 1ST_READ.BIN holds any of the seven texlists' addresses */
#include <ninja.h>

#include "b_chaos0/dat_0c95eb50.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 14
#define TEXT_7_LENGTH 14
#define TEXT_8_LENGTH 14
#include "b_chaos0/dat_0c95eb50/text.inc"

static NJS_TEXNAME dat_0c95eb50[18] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_5, 0, 0}, {TEXT_2, 0, 0},
    {TEXT_3, 0, 0}, {TEXT_6, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_8, 0, 0},
    {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
};
NJS_TEXLIST dat_0c95ec28 = {
#include "b_chaos0/dat_0c95eb50/dat_0c95ec28.inc"
};
NJS_TEXLIST dat_0c95ec30[6] = {
#include "b_chaos0/dat_0c95eb50/dat_0c95ec30.inc"
};
