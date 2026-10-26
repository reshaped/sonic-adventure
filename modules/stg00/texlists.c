/* the Hedgehog Hammer's texlist, and the list of pvms the stage loads */
#include <ninja.h>

#include "stg00/mole_textures.h"
#include "stg00/texlists.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 14
#define TEXT_2_LENGTH 16
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 11
#define TEXT_11_LENGTH 11
#define TEXT_12_LENGTH 7
#define TEXT_13_LENGTH 6
#define TEXT_14_LENGTH 7
#define TEXT_15_LENGTH 7
#define TEXT_16_LENGTH 12
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 6
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 12
#include "stg00/texlists/text.inc"

#pragma section HEAD
static NJS_TEXNAME practice_textures[21] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0},
};
NJS_TEXLIST practice_texlist = {
#include "stg00/texlists/practice_texlist.inc"
};

/* the first four entries fill the stage's texlist slots, and a null texlist
 * leaves its slot empty. texture_load_stage then loads the entries after them
 * up to the next null texlist */
struct texture_pvm practice_pvms[6] = {
    {TEXT_22, &practice_texlist}, {NULL, NULL}, {NULL, NULL}, {NULL, NULL},
    {TEXT_23, &mole_texlist},     {NULL, NULL},
};
#pragma section
