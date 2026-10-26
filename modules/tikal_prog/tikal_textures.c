/* Tikal's 18 textures and tikal_texlist, which texture_load_character fills
 * from the PVM TIKAL.PRS */
#include <ninja.h>

#include "tikal_prog/tikal.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 6
#define TEXT_5_LENGTH 6
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 6
#define TEXT_8_LENGTH 6
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 6
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 6
#define TEXT_15_LENGTH 6
#define TEXT_16_LENGTH 6
#define TEXT_17_LENGTH 6
#define TEXT_18_LENGTH 6
#include "tikal_prog/tikal_textures/text.inc"

static NJS_TEXNAME tikal_textures[18] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
};
NJS_TEXLIST tikal_texlist = {
#include "tikal_prog/tikal_textures/tikal_texlist.inc"
};
