/* ev00bd_e102_action_2 and its motion, and seven texture names whose
 * texlist nothing reads */
#include <ninja.h>

#include "ev00bd/e102_action_2.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 7
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#include "ev00bd/e102_action_2/text.inc"

extern NJS_MDATA2 dat_0cb880e8[];
extern NJS_OBJECT e102_object;

static NJS_MOTION ev00bd_e102_motion_2 = {
#include "ev00bd/e102_action_2/ev00bd_e102_motion_2.inc"
};
NJS_ACTION ev00bd_e102_action_2 = {
#include "ev00bd/e102_action_2/ev00bd_e102_action_2.inc"
};
static NJS_TEXNAME dat_0cb885cc[7] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0},
};
