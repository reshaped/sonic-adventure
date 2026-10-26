/* the names of the E GACHA robots' textures, their texlist, and the
 * materials, strips and uvs after it */
#include <ninja.h>
#include <sg_xpt.h>

#include "stg10/gacha_robot_textures.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 6
#define TEXT_11_LENGTH 9
#include "stg10/gacha_robot_textures/text.inc"

static NJS_TEXNAME gacha_robot_textures[11] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0}, {TEXT_11, 0, 0},
};
NJS_TEXLIST gacha_robot_texlist = {
#include "stg10/gacha_robot_textures/gacha_robot_texlist.inc"
};
static NJS_MATERIAL dat_0cb012cc[4] = {
#include "stg10/gacha_robot_textures/dat_0cb012cc.inc"
};
static Sint16 dat_0cb0131c[11] = {
#include "stg10/gacha_robot_textures/dat_0cb0131c.inc"
};
static Sint16 dat_0cb01332[6] = {
#include "stg10/gacha_robot_textures/dat_0cb01332.inc"
};
static Sint16 dat_0cb0133e[16] = {
#include "stg10/gacha_robot_textures/dat_0cb0133e.inc"
};
static Sint16 dat_0cb0135e[15] = {
#include "stg10/gacha_robot_textures/dat_0cb0135e.inc"
};
static NJS_COLOR dat_0cb0137c[9] = {
#include "stg10/gacha_robot_textures/dat_0cb0137c.inc"
};
