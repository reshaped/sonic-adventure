/* the texture list of the garden's objects, and its texture names */
#include <ninja.h>

#include "al_garden00/textures.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 12
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 10
#define TEXT_10_LENGTH 10
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 9
#define TEXT_13_LENGTH 7
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 8
#define TEXT_16_LENGTH 9
#include "al_garden00/textures/text.inc"

static NJS_TEXNAME garden_objects_textures[16] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
};
NJS_TEXLIST garden_objects_texlist = {
#include "al_garden00/textures/garden_objects_texlist.inc"
};
